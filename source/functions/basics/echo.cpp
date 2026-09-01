// Variadic: read every argument as a list and return them unchanged
// (including tables) as a list of results.

#include "sdk/lua/arguments.hpp"
#include "sdk/registry/registry.hpp"

MTA_LUA_FUNCTION("sample_echo", "Returns all arguments unchanged (including tables).")
{
    mta::lua::Arguments arguments;
    arguments.read(L);

    if (arguments.empty())
    {
        return mta::lua::push_results(L, nullptr);
    }

    lua_settop(L, 0);
    return arguments.push(L);
}
