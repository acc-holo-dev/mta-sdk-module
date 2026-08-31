#include "runtime/logging.hpp"

#include "ILuaModuleManager10.h"
#include "module/module.hpp"

#include <cstdio>

namespace mta::log
{
namespace
{
// Вывод, пока менеджер модуля не подключён (тест-харнесс, ранние ошибки).
void fallback_write(std::FILE *target, std::string_view message)
{
    std::fwrite(message.data(), 1, message.size(), target);
    std::fputc('\n', target);
    std::fflush(target);
}
} // namespace

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
