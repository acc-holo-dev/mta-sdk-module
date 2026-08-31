#include "lua/events.hpp"

#include "lua/common.hpp"
#include "runtime/logging.hpp"

namespace mta::events
{
bool trigger(lua_State *lua_vm, const char *event_name, const mta::lua::Arguments &arguments)
{
    if (lua_vm == nullptr || event_name == nullptr)
    {
        return false;
    }

    lua_getglobal(lua_vm, "triggerEvent");
    if (lua_isfunction(lua_vm, -1) == 0)
    {
        lua_pop(lua_vm, 1);
        return false;
    }

    lua_pushstring(lua_vm, event_name);
    lua_getglobal(lua_vm, "root"); // источник события — корневой элемент

    const int pushed = arguments.push(lua_vm);
    if (lua_pcall(lua_vm, 2 + pushed, 0, 0) != LUA_OK)
    {
        const char *message = lua_tostring(lua_vm, -1);
        mta::log::error("событие '", event_name, "' не удалось: ", message ? message : "?");
        lua_pop(lua_vm, 1);
        return false;
    }

    return true;
}
} // namespace mta::events
