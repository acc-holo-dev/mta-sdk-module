#pragma once

// Граница исключений для Lua-функций модуля.
//
// Тела функций пишутся обычным C++ и могут свободно бросать исключения:
// макрос MTA_LUA_FUNCTION пропускает каждый вызов через protected_call(),
// который превращает любое убежавшее исключение C++ в Lua-ошибку.
// Непойманное исключение никогда не пересекает границу модуля —
// серверный процесс защищён.
//
// Внутри тел функций предпочтительнее mta::lua::raise_error(...), а не
// luaL_error(...): raise_error корректно раскручивает стек C++ (деструкторы
// вызываются), тогда как luaL_error делает longjmp поверх локальных объектов.

#include "lua/common.hpp"

#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace mta::lua
{
// Бросить ошибку, которая станет Lua-ошибкой на границе.
[[noreturn]] inline void raise(std::string message)
{
    throw std::runtime_error(std::move(message));
}

// Потоковый вариант: raise_error("аргумент #", 2, " должен быть числом").
template <typename... Args>
[[noreturn]] void raise_error(Args &&...args)
{
    std::ostringstream stream;
    (stream << ... << std::forward<Args>(args));
    raise(stream.str());
}

// Трамплин-граница: выполнить fn(L), поймать всё и перевести в luaL_error.
// Сама функция не владеет C++-ресурсами, поэтому longjmp из luaL_error
// всегда чист.
inline int protected_call(lua_State *L, int (*fn)(lua_State *)) noexcept
{
    try
    {
        return fn(L);
    }
    catch (std::exception &e)
    {
        return luaL_error(L, "%s", e.what());
    }
    catch (...)
    {
        return luaL_error(L, "unknown C++ exception in module function");
    }
}
} // namespace mta::lua
