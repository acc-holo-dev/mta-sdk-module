#pragma once

// Lua-function registry and registration macros.
//
// The main style is body-style with typed arguments:
//
//     MTA_LUA_FUNCTION("my_sum", "Adds two numbers.")
//     {
//         auto [a, b] = mta::lua::args<double, double>(L);
//         return mta::lua::push_results(L, a + b);
//     }
//
// Short one-liners use the lambda style (types from the signature, automatic
// return):
//
//     MTA_LUA_FUNC("my_sum", "Adds two numbers.",
//         [](double a, double b) { return a + b; });
//
// Both macros work in any .cpp under source/ -- the build picks up the file
// and registration happens automatically when the module loads.

#include "sdk/bind/bind.hpp"

#include <cstddef>
#include <vector>

class ILuaModuleManager10;

namespace mta::registry
{
// Raw entry point (C++ linkage); the framework wraps it into a Lua function.
using module_function = int (*)(lua_State *lua_vm);

struct Spec
{
    const char *name;
    const char *description;
    module_function function;
};

class Registry
{
public:
    static Registry &instance();

    void add(Spec spec);

    // Registers every function into the resource's VM (at each resource start).
    void register_all(ILuaModuleManager10 &manager, lua_State *lua_vm) const;

    [[nodiscard]] const std::vector<Spec> &functions() const noexcept { return functions_; }
    [[nodiscard]] std::size_t size() const noexcept { return functions_.size(); }

private:
    Registry() = default;
    std::vector<Spec> functions_{};
};
} // namespace mta::registry

#define MTA_CAT_(a, b) a##b
#define MTA_CAT(a, b) MTA_CAT_(a, b)

// --- lambda style (short) -----------------------------------------------------

#define MTA_LUA_FUNC_IMPL(Name, Description, Function, Counter)     [[maybe_unused]] static const bool MTA_CAT(mta_func_registered_, Counter) =         ::mta::lua::detail::register_typed<Counter>((Name), (Description), (Function))

// MTA_LUA_FUNC("name", "description", lambda-function);
#define MTA_LUA_FUNC(Name, Description, Function)     MTA_LUA_FUNC_IMPL((Name), (Description), (Function), __COUNTER__)

// --- body style (main) ---------------------------------------------------------

#define MTA_LUA_FUNCTION_IMPL(Name, Description, Counter)     static int MTA_CAT(mta_body_, Counter)(lua_State * L);     [[maybe_unused]] static const bool MTA_CAT(mta_registered_, Counter) =         ::mta::lua::detail::register_function(             (Name), (Description),             +[](lua_State * L) -> int {                 return ::mta::lua::protected_call(L, &MTA_CAT(mta_body_, Counter));             });     static int MTA_CAT(mta_body_, Counter)(lua_State * L)

// MTA_LUA_FUNCTION("name", "description") { body; arguments via args<...> }
#define MTA_LUA_FUNCTION(Name, Description)     MTA_LUA_FUNCTION_IMPL((Name), (Description), __COUNTER__)