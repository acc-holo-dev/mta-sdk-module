// Встроенная интроспекция: список всех функций модуля с их описаниями.

#include <string>

#include "lua/argument.hpp"
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("module_functions",
    "Возвращает таблицу {имя = описание} со всеми функциями модуля.")
{
    mta::lua::Table table;
    for (const auto &spec : mta::registry::Registry::instance().functions())
    {
        table.fields.emplace_back(
            spec.name, mta::lua::Argument(std::string(spec.description ? spec.description : "")));
    }
    return mta::lua::push_results(L, mta::lua::Argument(std::move(table)));
}
