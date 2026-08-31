#pragma once

// Логирование через менеджер модуля: вывод уходит в консоль MTA-сервера.
// Пока менеджер не подключён (например, в тестовом харнессе) — fallback
// в стандартный вывод/ошибку.
//
//     mta::log::info("подключено ", host, ":", port);
//     mta::log::error("запрос не удался: ", reason);
//     mta::log::debug(L, "вызов из контекста ресурса");

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

struct lua_State;

namespace mta::log
{
void write_info(std::string_view message);
void write_error(std::string_view message);
void write_debug(lua_State *lua_vm, std::string_view message);

template <typename... Args>
void info(Args &&...args)
{
    if constexpr (sizeof...(args) > 0)
    {
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        write_info(stream.str());
    }
}

template <typename... Args>
void error(Args &&...args)
{
    if constexpr (sizeof...(args) > 0)
    {
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        write_error(stream.str());
    }
}

template <typename... Args>
void debug(lua_State *lua_vm, Args &&...args)
{
    if constexpr (sizeof...(args) > 0)
    {
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        write_debug(lua_vm, stream.str());
    }
}
} // namespace mta::log
