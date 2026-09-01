#pragma once

// Exception-to-Lua-error conversion.
//
// Module functions must never let a C++ exception escape: the macros wrap
// every entry point in a trampoline that catches everything and turns it
// into a proper Lua error (longjmp across the C boundary is safe because the
// module does not own C++ resources at that point).

#include "sdk/lua/common.hpp"

#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace mta::lua
{
// Raise an error that becomes a Lua error at the boundary.
[[noreturn]] inline void raise(std::string message)
{
    throw std::runtime_error(std::move(message));
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
    catch (std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
    catch (...)
    {
        return luaL_error(L, "unknown C++ exception in module function");
    }
}
} // namespace mta::lua
