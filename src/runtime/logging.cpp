#include "runtime/logging.hpp"

#include "ILuaModuleManager10.h"
#include "module/module.hpp"

#include <cstdio>

namespace mta::log
{
namespace
{
Level g_level = Level::Info;

// Output while the module manager is not attached (test harness, early
// errors).
void fallback_write(std::FILE *target, std::string_view message)
{
    std::fwrite(message.data(), 1, message.size(), target);
    std::fputc('\n', target);
    std::fflush(target);
}
} // namespace

void set_level(Level level) noexcept
{
    g_level = level;
}

Level get_level() noexcept
{
    return g_level;
}

void write_info(std::string_view message)
{
    if (auto *manager = mta::module::manager())
    {
        const std::string text(message);
        manager->Printf("%s", text.c_str());
        return;
    }
    fallback_write(stdout, message);
}

void write_warn(std::string_view message)
{
    if (auto *manager = mta::module::manager())
    {
        const std::string text(message);
        manager->Printf("[WARN] %s", text.c_str());
        return;
    }
    fallback_write(stdout, message);
}

void write_error(std::string_view message)
{
    if (auto *manager = mta::module::manager())
    {
        const std::string text(message);
        manager->ErrorPrintf("%s", text.c_str());
        return;
    }
    fallback_write(stderr, message);
}

void write_debug(lua_State *lua_vm, std::string_view message)
{
    if (auto *manager = mta::module::manager())
    {
        const std::string text(message);
        manager->DebugPrintf(lua_vm, "%s", text.c_str());
        return;
    }
    fallback_write(stdout, message);
}
} // namespace mta::log
