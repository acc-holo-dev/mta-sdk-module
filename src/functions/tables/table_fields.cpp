// Хелперы таблиц: чтение/запись полей по строковому ключу.
//
//     local t = {name = "Вася", hp = 100}
//     local name, hp = sample_table_get(t)   -- "Вася", 100

#include <string>

#include "lua/table_helpers.hpp"
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_table_get",
    "Читает поля name (строка) и hp (число) из таблицы; возвращает оба.")
{
    auto [table] = mta::lua::args<mta::lua::Table>(L);

    const std::string name = mta::lua::get_field<std::string>(table, "name", "unknown");
    const double hp = mta::lua::get_field<double>(table, "hp", 0.0);

    return mta::lua::push_results(L, name, hp);
}

MTA_LUA_FUNCTION("sample_table_set",
    "Записывает поле name в таблицу и возвращает её обратно.")
{
    auto [table, name] = mta::lua::args<mta::lua::Table, std::string>(L);

    mta::lua::set_field(table, "name", mta::lua::Argument(name));

    return mta::lua::push_results(L, mta::lua::Argument(std::move(table)));
}
