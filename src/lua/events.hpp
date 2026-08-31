#pragma once

// Триггер MTA-событий из модуля.
//
// Модуль может «бросить» событие в Lua-скрипты ресурса через штатный
// глобальный triggerEvent. Скрипт ловит его обычным addEventHandler:
//
//     -- Lua:
//     addEventHandler("onMyModuleReady", root, function(...) ... end)
//
//     // C++ (внутри функции модуля):
//     mta::lua::Arguments args;
//     args.push_string("hello");
//     mta::events::trigger(L, "onMyModuleReady", args);
//
// Источник события — глобальный корневой элемент root. Вызывать только из
// главного потока (как и всё, что касается lua_State).

#include "lua/arguments.hpp"

struct lua_State;

namespace mta::events
{
// Триггерит событие в VM ресурса. false, если triggerEvent недоступен или
// вызов не удался (логируется).
bool trigger(lua_State *lua_vm, const char *event_name, const mta::lua::Arguments &arguments);
} // namespace mta::events
