#include "module/module.hpp"

#include "ILuaModuleManager10.h"
#include "module/export.hpp"
#include "registry/registry.hpp"
#include "runtime/callback.hpp"
#include "runtime/logging.hpp"
#include "runtime/resources.hpp"
#include "runtime/scheduler.hpp"

#include <cstring>

namespace mta::module
{
namespace
{
constexpr Info module_details{
    "Base Module",
    "anon",
    1.0F,
};

ILuaModuleManager10 *g_module_manager = nullptr;

// Сервер передаёт буферы MAX_INFO_LENGTH (128 байт) — копируем с гарантией
// завершающего нуля.
void copy_info_string(char *destination, const char *source) noexcept
{
    if (destination == nullptr || source == nullptr)
    {
        return;
    }
    std::strncpy(destination, source, MAX_INFO_LENGTH);
    destination[MAX_INFO_LENGTH - 1] = '\0';
}
} // namespace

Info info() noexcept
{
    return module_details;
}

ILuaModuleManager10 *manager() noexcept
{
    return g_module_manager;
}

std::string current_resource_name(lua_State *lua_vm) noexcept
{
    if (g_module_manager == nullptr || lua_vm == nullptr)
    {
        return std::string{};
    }

    char buffer[MAX_INFO_LENGTH * 2]{};
    if (g_module_manager->GetResourceName(lua_vm, buffer, sizeof(buffer)))
    {
        return std::string(buffer);
    }
    return std::string{};
}

bool initialize(ILuaModuleManager10 *manager, char *module_name, char *author, float *version) noexcept
{
    if (!manager || !module_name || !author || !version)
    {
        return false;
    }

    g_module_manager = manager;

    copy_info_string(module_name, module_details.name);
    copy_info_string(author, module_details.author);
    *version = module_details.version;

    mta::async::Scheduler::instance().start();

    const char *server_version = manager->GetVersionString();
    mta::log::info("модуль: загружен ", module_details.name, " (MTA ",
                   server_version ? server_version : "?", ")");
    return true;
}

void register_functions(lua_State *lua_vm) noexcept
{
    if (!g_module_manager || !lua_vm)
    {
        return;
    }

    mta::registry::Registry::instance().register_all(*g_module_manager, lua_vm);
    mta::log::debug(lua_vm, "модуль: зарегистрировано ",
                    mta::registry::Registry::instance().size(), " функций");
}

bool pulse() noexcept
{
    // Раздаём результаты фоновых задач и срабатываем таймеры; бросаний нет.
    mta::async::Scheduler::instance().pump();
    return true;
}

bool shutdown() noexcept
{
    // Порядок важен: сначала останавливаем воркеров (в completions ещё могут
    // жить callback-и), затем освобождаем Lua-ссылки, пока VM достижимы.
    mta::async::Scheduler::instance().stop();
    mta::async::release_all_callbacks();
    mta::resources::Hub::instance().notify_all_released();

    g_module_manager = nullptr;
    return true;
}

bool resource_stopping(lua_State *lua_vm) noexcept
{
    const std::string resource = current_resource_name(lua_vm);
    if (!resource.empty())
    {
        mta::resources::Hub::instance().notify_resource_stopping(resource);
    }
    return true;
}

bool resource_stopped(lua_State *lua_vm) noexcept
{
    const std::string resource = current_resource_name(lua_vm);
    if (!resource.empty())
    {
        mta::resources::Hub::instance().notify_resource_stopped(resource);
        mta::async::Scheduler::instance().handle_resource_stopped(resource);
        mta::async::invalidate_resource_callbacks(resource);
    }
    return true;
}
} // namespace mta::module

// ---------------------------------------------------------------------------
// Точки входа MTA. Загрузчик сервера (CLuaModule в mtasa-blue) резолвит их
// по имени через LoadLibrary/dlsym; каждая обязана существовать.
// ---------------------------------------------------------------------------

MTAEXPORT bool InitModule(ILuaModuleManager10 *manager, char *module_name, char *author, float *version)
{
    return mta::module::initialize(manager, module_name, author, version);
}

MTAEXPORT void RegisterFunctions(lua_State *lua_vm)
{
    mta::module::register_functions(lua_vm);
}

MTAEXPORT bool DoPulse()
{
    return mta::module::pulse();
}

MTAEXPORT bool ShutdownModule()
{
    return mta::module::shutdown();
}

MTAEXPORT bool ResourceStopping(lua_State *lua_vm)
{
    return mta::module::resource_stopping(lua_vm);
}

MTAEXPORT bool ResourceStopped(lua_State *lua_vm)
{
    return mta::module::resource_stopped(lua_vm);
}
