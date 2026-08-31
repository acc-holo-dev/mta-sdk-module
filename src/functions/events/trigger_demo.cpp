// События: модуль триггерит событие в Lua-скрипты ресурса.
//
//     -- Lua:
//     addEventHandler("onSampleEvent", root, function(...) ... end)
//     sample_trigger_event("onSampleEvent", 1, "two")

#include "lua/arguments.hpp"
#include "lua/events.hpp"
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_trigger_event",
    "Триггерит событие с заданным именем и аргументами (источник — root).")
{
    auto [name] = mta::lua::args<std::string>(L);

    mta::lua::Arguments arguments;
    arguments.read(L, 2); // все аргументы после имени

    const bool ok = mta::events::trigger(L, name.c_str(), arguments);
    return mta::lua::push_results(L, ok);
}
