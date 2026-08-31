// Вариадика: читаем все аргументы списком и возвращаем обратно (включая
// таблицы) как список результатов.

#include "lua/arguments.hpp"
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_echo", "Возвращает все аргументы без изменений (включая таблицы).")
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
