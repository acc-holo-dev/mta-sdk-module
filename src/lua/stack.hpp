#pragma once

// Удобный типобезопасный доступ к стеку Lua для тел функций модуля.
//
// Все check_* / opt_* бросают исключения (см. protect.hpp), а не зовут
// luaL_error напрямую — локальные C++-объекты корректно разрушаются при
// плохих аргументах. push-хелперы безопасно расширяют стек через
// lua_checkstack.
//
// Использование внутри MTA_LUA_FUNCTION:
//     const double a = mta::lua::check_number(L, 1);
//     return mta::lua::push_results(L, a + b);

#include "lua/common.hpp"
#include "lua/protect.hpp"

#include <string>
#include <string_view>

namespace mta::lua
{
namespace detail
{
[[nodiscard]] inline int normalize_index(lua_State *L, int index) noexcept
{
    if (index >= 0)
    {
        return index;
    }
    return lua_gettop(L) + index + 1;
}

[[nodiscard]] inline const char *type_name(int lua_type_value) noexcept
{
    switch (lua_type_value)
    {
    case LUA_TNONE: return "no value";
    case LUA_TNIL: return "nil";
    case LUA_TBOOLEAN: return "boolean";
    case LUA_TLIGHTUSERDATA: return "userdata";
    case LUA_TNUMBER: return "number";
    case LUA_TSTRING: return "string";
    case LUA_TTABLE: return "table";
    case LUA_TFUNCTION: return "function";
    case LUA_TUSERDATA: return "userdata";
    default: return "unknown";
    }
}
} // namespace detail

[[nodiscard]] inline int arg_count(lua_State *L) noexcept
{
    return lua_gettop(L);
}

// --- Чтение типизированных аргументов (бросают при несоответствии) --------

[[nodiscard]] inline double check_number(lua_State *L, int index)
{
    const int normalized = detail::normalize_index(L, index);
    if (lua_type(L, normalized) != LUA_TNUMBER)
    {
        raise_error("argument #", index, " must be a number, got ",
                    detail::type_name(lua_type(L, normalized)));
    }
    return lua_tonumber(L, normalized);
}

[[nodiscard]] inline double opt_number(lua_State *L, int index, double default_value)
{
    const int normalized = detail::normalize_index(L, index);
    const int type_value = lua_type(L, normalized);
    if (type_value == LUA_TNONE || type_value == LUA_TNIL)
    {
        return default_value;
    }
    if (type_value != LUA_TNUMBER)
    {
        raise_error("argument #", index, " must be a number or nil, got ",
                    detail::type_name(type_value));
    }
    return lua_tonumber(L, normalized);
}

[[nodiscard]] inline lua_Integer check_integer(lua_State *L, int index)
{
    const int normalized = detail::normalize_index(L, index);
    if (lua_type(L, normalized) != LUA_TNUMBER)
    {
        raise_error("argument #", index, " must be an integer, got ",
                    detail::type_name(lua_type(L, normalized)));
    }
    const lua_Number value = lua_tonumber(L, normalized);
    if (value != static_cast<lua_Number>(static_cast<lua_Integer>(value)))
    {
        raise_error("argument #", index, " must be a whole number, got ", value);
    }
    return static_cast<lua_Integer>(value);
}

[[nodiscard]] inline lua_Integer opt_integer(lua_State *L, int index, lua_Integer default_value)
{
    const int normalized = detail::normalize_index(L, index);
    const int type_value = lua_type(L, normalized);
    if (type_value == LUA_TNONE || type_value == LUA_TNIL)
    {
        return default_value;
    }
    if (type_value != LUA_TNUMBER)
    {
        raise_error("argument #", index, " must be an integer or nil, got ",
                    detail::type_name(type_value));
    }
    const lua_Number value = lua_tonumber(L, normalized);
    if (value != static_cast<lua_Number>(static_cast<lua_Integer>(value)))
    {
        raise_error("argument #", index, " must be a whole number, got ", value);
    }
    return static_cast<lua_Integer>(value);
}

[[nodiscard]] inline bool check_boolean(lua_State *L, int index)
{
    const int normalized = detail::normalize_index(L, index);
    if (lua_type(L, normalized) != LUA_TBOOLEAN)
    {
        raise_error("argument #", index, " must be a boolean, got ",
                    detail::type_name(lua_type(L, normalized)));
    }
    return lua_toboolean(L, normalized) != 0;
}

[[nodiscard]] inline bool opt_boolean(lua_State *L, int index, bool default_value)
{
    const int normalized = detail::normalize_index(L, index);
    const int type_value = lua_type(L, normalized);
    if (type_value == LUA_TNONE || type_value == LUA_TNIL)
    {
        return default_value;
    }
    if (type_value != LUA_TBOOLEAN)
    {
        raise_error("argument #", index, " must be a boolean or nil, got ",
                    detail::type_name(type_value));
    }
    return lua_toboolean(L, normalized) != 0;
}

// Принимает строки (числа конвертируются, как в стандартном luaL_checkstring).
[[nodiscard]] inline std::string check_string(lua_State *L, int index)
{
    const int normalized = detail::normalize_index(L, index);
    if (lua_isstring(L, normalized) == 0)
    {
        raise_error("argument #", index, " must be a string, got ",
                    detail::type_name(lua_type(L, normalized)));
    }
    std::size_t length = 0;
    const char *text = lua_tolstring(L, normalized, &length);
    return std::string(text ? text : "", length);
}

[[nodiscard]] inline std::string opt_string(lua_State *L, int index, const char *default_value)
{
    const int normalized = detail::normalize_index(L, index);
    const int type_value = lua_type(L, normalized);
    if (type_value == LUA_TNONE || type_value == LUA_TNIL)
    {
        return std::string(default_value ? default_value : "");
    }
    if (lua_isstring(L, normalized) == 0)
    {
        raise_error("argument #", index, " must be a string or nil, got ",
                    detail::type_name(type_value));
    }
    std::size_t length = 0;
    const char *text = lua_tolstring(L, normalized, &length);
    return std::string(text ? text : "", length);
}

[[nodiscard]] inline void *check_light_userdata(lua_State *L, int index)
{
    const int normalized = detail::normalize_index(L, index);
    if (lua_type(L, normalized) != LUA_TLIGHTUSERDATA)
    {
        raise_error("argument #", index, " must be userdata, got ",
                    detail::type_name(lua_type(L, normalized)));
    }
    return lua_touserdata(L, normalized);
}

// --- Выкладка результатов ---------------------------------------------------

inline void push_one(lua_State *L, lua_Number value) noexcept
{
    lua_pushnumber(L, value);
}

inline void push_one(lua_State *L, lua_Integer value) noexcept
{
    lua_pushnumber(L, static_cast<lua_Number>(value));
}

inline void push_one(lua_State *L, int value) noexcept
{
    lua_pushnumber(L, static_cast<lua_Number>(value));
}

inline void push_one(lua_State *L, bool value) noexcept
{
    lua_pushboolean(L, value ? 1 : 0);
}

inline void push_one(lua_State *L, std::nullptr_t) noexcept
{
    lua_pushnil(L);
}

inline void push_one(lua_State *L, const char *value) noexcept
{
    lua_pushstring(L, value ? value : "");
}

inline void push_one(lua_State *L, const std::string &value) noexcept
{
    lua_pushlstring(L, value.data(), value.size());
}

inline void push_one(lua_State *L, std::string_view value) noexcept
{
    lua_pushlstring(L, value.data(), value.size());
}

inline void push_one(lua_State *L, void *value) noexcept
{
    lua_pushlightuserdata(L, value);
}

// Объявлены ниже (argument.hpp/arguments.hpp): позволяет передавать
// аргументы и таблицы прямо в push_results.
class Argument;
class Arguments;
struct Table;
void push_one(lua_State *L, const Argument &value);
void push_one(lua_State *L, const Arguments &value);
void push_one(lua_State *L, const Table &value);

// Кладёт одно или несколько значений и возвращает их количество.
// Расширяет стек безопасно; бросает исключение, если стек не растянется.
template <typename... Values>
int push_results(lua_State *L, Values &&...values)
{
    if constexpr (sizeof...(values) == 0)
    {
        return 0;
    }
    else
    {
        if (lua_checkstack(L, static_cast<int>(sizeof...(values)) + 8) == 0)
        {
            raise_error("переполнение стека Lua: не удалось положить ", sizeof...(values), " значений");
        }
        (push_one(L, std::forward<Values>(values)), ...);
        return static_cast<int>(sizeof...(values));
    }
}
} // namespace mta::lua
