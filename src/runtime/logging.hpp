#pragma once

// Логирование через менеджер модуля: вывод уходит в консоль MTA-сервера.
// Пока менеджер не подключён (например, в тестовом харнессе) — fallback
// в стандартный вывод/ошибку.
//
// Уровни (по возрастанию серьёзности): Debug < Info < Warn < Error < Off.
// Сообщение печатается, если его уровень >= текущего (set_level).
// По умолчанию уровень Info: debug скрыт, info/warn/error видны.
//
//     mta::log::set_level(mta::log::Level::Debug);
//     mta::log::info("подключено ", host, ":", port);
//     mta::log::warn("подозрительное значение: ", value);
//     mta::log::error("запрос не удался: ", reason);
//     mta::log::debug(L, "вызов из контекста ресурса");

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

struct lua_State;

namespace mta::log
{
enum class Level
{
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
    Off = 4,
};

void set_level(Level level) noexcept;
[[nodiscard]] Level get_level() noexcept;

void write_info(std::string_view message);
void write_warn(std::string_view message);
void write_error(std::string_view message);
void write_debug(lua_State *lua_vm, std::string_view message);

template <typename... Args>
void info(Args &&...args)
{
    if constexpr (sizeof...(args) > 0)
    {
        if (get_level() > Level::Info)
        {
            return;
        }
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        write_info(stream.str());
    }
}

template <typename... Args>
void warn(Args &&...args)
{
    if constexpr (sizeof...(args) > 0)
    {
        if (get_level() > Level::Warn)
        {
            return;
        }
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        write_warn(stream.str());
    }
}

template <typename... Args>
void error(Args &&...args)
{
    if constexpr (sizeof...(args) > 0)
    {
        if (get_level() > Level::Error)
        {
            return;
        }
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
        if (get_level() > Level::Debug)
        {
            return;
        }
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        write_debug(lua_vm, stream.str());
    }
}
} // namespace mta::log
