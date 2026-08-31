#pragma once

// Удобная работа с полями таблиц: чтение/запись по строковому ключу без
// ручного обхода Table.fields, плюс конвертация Argument → C++-тип.
//
//     auto [t] = mta::lua::args<mta::lua::Table>(L);
//     std::string name = mta::lua::get_field<std::string>(t, "name", "unknown");
//     double hp = mta::lua::get_field<double>(t, "hp", 100.0);
//     mta::lua::set_field(t, "alive", mta::lua::Argument(true));

#include "lua/argument.hpp"
#include "lua/protect.hpp"

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace mta::lua
{
// Имя типа Argument для сообщений об ошибках.
inline const char *type_name_of(const Argument &value) noexcept
{
    switch (value.type())
    {
    case Argument::Type::None: return "no value";
    case Argument::Type::Nil: return "nil";
    case Argument::Type::Boolean: return "boolean";
    case Argument::Type::LightUserData: return "userdata";
    case Argument::Type::Number: return "number";
    case Argument::Type::String: return "string";
    case Argument::Type::Table: return "table";
    }
    return "unknown";
}

namespace detail
{
template <typename T>
inline constexpr bool is_optional = false;
template <typename T>
inline constexpr bool is_optional<std::optional<T>> = true;
} // namespace detail

// Конвертирует Argument в T; бросает понятную ошибку при несовпадении типа.
template <typename T>
T convert(const Argument &value)
{
    using U = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<U, Argument>)
    {
        return value;
    }
    else if constexpr (std::is_same_v<U, Table>)
    {
        if (!value.is_table())
        {
            raise_error("поле должно быть таблицей, а это ", type_name_of(value));
        }
        return value.as_table();
    }
    else if constexpr (std::is_same_v<U, bool>)
    {
        if (value.type() != Argument::Type::Boolean)
        {
            raise_error("поле должно быть boolean, а это ", type_name_of(value));
        }
        return value.as_boolean();
    }
    else if constexpr (std::is_same_v<U, std::string>)
    {
        if (value.type() != Argument::Type::String)
        {
            raise_error("поле должно быть строкой, а это ", type_name_of(value));
        }
        return value.as_string();
    }
    else if constexpr (std::is_floating_point_v<U>)
    {
        if (value.type() != Argument::Type::Number)
        {
            raise_error("поле должно быть числом, а это ", type_name_of(value));
        }
        return static_cast<U>(value.as_number());
    }
    else if constexpr (std::is_integral_v<U> && !std::is_same_v<U, bool>)
    {
        if (value.type() != Argument::Type::Number)
        {
            raise_error("поле должно быть числом, а это ", type_name_of(value));
        }
        return static_cast<U>(value.as_number());
    }
    else if constexpr (detail::is_optional<U>)
    {
        if (value.type() == Argument::Type::Nil || value.type() == Argument::Type::None)
        {
            return U{};
        }
        return U{convert<typename U::value_type>(value)};
    }
    else
    {
        static_assert(!sizeof(U), "неподдерживаемый тип поля таблицы");
    }
}

// Ищет поле по строковому ключу; nullptr, если нет.
inline const Argument *find_field(const Table &table, const char *key) noexcept
{
    if (key == nullptr)
    {
        return nullptr;
    }
    for (const auto &entry : table.fields)
    {
        if (entry.first == key)
        {
            return &entry.second;
        }
    }
    return nullptr;
}

// Читает поле с дефолтом (если поля нет — дефолт).
template <typename T>
T get_field(const Table &table, const char *key, T default_value)
{
    const Argument *field = find_field(table, key);
    if (field == nullptr)
    {
        return default_value;
    }
    return convert<T>(*field);
}

// Читает поле; бросает, если поля нет.
template <typename T>
T get_field(const Table &table, const char *key)
{
    const Argument *field = find_field(table, key);
    if (field == nullptr)
    {
        raise_error("в таблице нет поля '", key ? key : "?", "'");
    }
    return convert<T>(*field);
}

// Записывает (или перезаписывает) поле.
inline void set_field(Table &table, const char *key, Argument value)
{
    if (key == nullptr)
    {
        return;
    }
    for (auto &entry : table.fields)
    {
        if (entry.first == key)
        {
            entry.second = std::move(value);
            return;
        }
    }
    table.fields.emplace_back(key, std::move(value));
}
} // namespace mta::lua
