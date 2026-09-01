#pragma once

// Logging through the module manager: output goes to the MTA server console.
// While the manager is not attached (e.g. in the test harness) it falls back
// to standard output/error.
//
// Levels (in ascending severity): Debug < Info < Warn < Error < Off.
// A message is printed when its level >= the current one (set_level).
// The default level is Info: debug is hidden, info/warn/error are visible.
//
//     mta::log::set_level(mta::log::Level::Debug);
//     mta::log::info("connected to ", host, ":", port);
//     mta::log::warn("suspicious value: ", value);
//     mta::log::error("request failed: ", reason);
//     mta::log::debug(L, "called in resource context");

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

// Debug message outside a VM context (no resource attribution).
template <typename... Args>
void debug(Args &&...args)
{
    if constexpr (sizeof...(args) > 0)
    {
        if (get_level() > Level::Debug)
        {
            return;
        }
        std::ostringstream stream;
        (stream << ... << std::forward<Args>(args));
        write_debug(nullptr, stream.str());
    }
}
} // namespace mta::log
