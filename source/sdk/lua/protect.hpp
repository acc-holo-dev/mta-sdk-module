#pragma once

// Exception-to-Lua-error conversion.
//
// Module functions must never let a C++ exception escape: the macros wrap
// every entry point in a trampoline that catches everything and turns it
// into a proper Lua error (longjmp across the C boundary is safe because the
// module does not own C++ resources at that point).
//
// Error rendering rules (plan §19):
//   mta::errors::Error with InternalError -> "internal module error: ..."
//   any other mta::errors::Error          -> message verbatim
//   other std::exception                  -> "internal module error: ..."
//   non-std exception                     -> "internal module error: ..."
//
// While a module function runs, the current function name is available for
// error messages (set by the registration trampolines); see
// current_function_name(). It stays set after the call returns -- it is only
// diagnostic context, overwritten by the next module call.

#include "sdk/errors/errors.hpp"
#include "sdk/lua/common.hpp"

#include <exception>
#include <sstream>
#include <string>
#include <utility>

namespace mta::lua
{
namespace detail
{
// Diagnostic context: the registered name of the module function that is
// currently running (nullptr outside a module call). Set by the registration
// trampolines; used to render "bad argument #2 to 'name'".
inline const char *&current_function_name() noexcept
{
    thread_local const char *name = nullptr;
    return name;
}
} // namespace detail

// Raise an error that becomes a Lua error at the boundary.
[[noreturn]] inline void raise(std::string message)
{
    throw ::mta::errors::Error(::mta::errors::Category::Generic, std::move(message));
}

// Streaming variant: raise_error("argument #", 2, " must be a number").
template <typename... Args>
[[noreturn]] void raise_error(Args &&...args)
{
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args));
    raise(stream.str());
}

// Trampoline boundary: run fn(L), catch everything and convert to luaL_error.
inline int protected_call(lua_State *L, int (*fn)(lua_State *)) noexcept
{
    try
    {
        return fn(L);
    }
    catch (const ::mta::errors::Error &e)
    {
        if (e.category() == ::mta::errors::Category::InternalError)
        {
            return luaL_error(L, "internal module error: %s", e.what());
        }
        return luaL_error(L, "%s", e.what());
    }
    catch (std::exception &e)
    {
        return luaL_error(L, "internal module error: %s", e.what());
    }
    catch (...)
    {
        return luaL_error(L, "internal module error: unknown C++ exception");
    }
}

// Same as protected_call, but first names the running function so argument
// errors can render "bad argument #N to '<name>'".
inline int protected_call_named(lua_State *L, int (*fn)(lua_State *), const char *name) noexcept
{
    detail::current_function_name() = name;
    return protected_call(L, fn);
}
} // namespace mta::lua