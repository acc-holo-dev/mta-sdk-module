// Пример прямого доступа к стеку Lua, когда args<...> не подходит
// (переменная логика, тонкий разбор). Тело получает lua_State* L и делает
// что угодно со стеком — каркас лишь ловит исключения.

#include "lua/common.hpp"
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_stack_dump", "Возвращает количество аргументов и тип каждого.")
{
    const int count = lua_gettop(L);
    lua_pushnumber(L, static_cast<lua_Number>(count));
    for (int i = 1; i <= count; ++i)
    {
        lua_pushstring(L, lua_typename(L, lua_type(L, i)));
    }
    return count + 1;
}
