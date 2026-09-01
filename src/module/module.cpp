#include "module/module.hpp"

#include "ILuaModuleManager10.h"
#include "module/export.hpp"
#include "registry/registry.hpp"
#include "runtime/callback.hpp"
#include "runtime/logging.hpp"
#include "runtime/resources.hpp"
#include "runtime/scheduler.hpp"

#include <cstdlib>
#include <cstring>

namespace mta::module
{
namespace
{
#ifndef SDK_MODULE_VERSION
#error "SDK_MODULE_VERSION is missing: derive it from project(VERSION) in CMakeLists.txt"
#endif
#ifndef SDK_MODULE_TITLE
#error "SDK_MODULE_TITLE is missing: set it via SDK_MODULE_TITLE in CMakeLists.txt"
#endif
#ifndef SDK_MODULE_AUTHOR
#error "SDK_MODULE_AUTHOR is missing: set it via SDK_MODULE_AUTHOR in CMakeLists.txt"
#endif

// Version float ("1.1" for project version 1.1.0), provided by the build
// system so the module and the CMake package report the same version.
const float module_version = static_cast<float>(std::atof(SDK_MODULE_VERSION));

// Module identity is configured at build time (SDK_MODULE_TITLE /
// SDK_MODULE_AUTHOR cache variables) -- renaming the module never requires
// touching C++.
const Info module_details{
    SDK_MODULE_TITLE,
    SDK_MODULE_AUTHOR,
    module_version,
};

ILuaModuleManager10 *g_module_manager = nullptr;

// The server provides MAX_INFO_LENGTH (128 byte) buffers -- copy with a
// guaranteed terminating NUL. A hand-written loop avoids both the MSVC
// deprecation warning for strncpy and any platform-specific _s variant.
void copy_info_string(char *destination, const char *source) noexcept
{
    if (destination == nullptr || source == nullptr)
    {
        return;
    }
    std::size_t i = 0;
    for (; i + 1 < MAX_INFO_LENGTH && source[i] != '\0'; ++i)
    {
        destination[i] = source[i];
    }
    destination[i] = '\0';
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
    mta::log::info("module: loaded ", module_details.name, " (MTA ",
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
    mta::log::debug(lua_vm, "module: registered ",
                    mta::registry::Registry::instance().size(), " functions");
}

bool pulse() noexcept
{
    // Dispatch background-task results and fire timers; never throws.
    mta::async::Scheduler::instance().pump();
    return true;
}

bool shutdown() noexcept
{
    // Order matters: stop the workers first (completions may still hold
    // callbacks), then release Lua references while the VMs are reachable.
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
// MTA entry points. The server loader (CLuaModule in mtasa-blue) resolves
// them by name through LoadLibrary/dlsym; every one of them must exist.
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