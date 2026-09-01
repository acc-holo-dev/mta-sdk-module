// An argument-less function returning a table. Facts come from the manager.

#include <mta/sdk.hpp>

#include "ILuaModuleManager10.h"

#include <string>

MTA_LUA_FUNCTION("sample_version",
    "Returns a table with module and server version information.")
{
    const mta::module::Info info = mta::module::info();
    auto *manager = mta::module::manager();

    mta::lua::Table data;
    data.fields.emplace_back("module", mta::lua::Argument(std::string(info.name)));
    data.fields.emplace_back("module_author", mta::lua::Argument(std::string(info.author)));
    data.fields.emplace_back("module_version",
                             mta::lua::Argument(static_cast<lua_Number>(info.version)));

    if (manager != nullptr)
    {
        const char *mta = manager->GetVersionString();
        const char *os = manager->GetOperatingSystemName();
        data.fields.emplace_back("mta", mta::lua::Argument(std::string(mta ? mta : "?")));
        data.fields.emplace_back(
            "netcode", mta::lua::Argument(static_cast<lua_Number>(manager->GetNetcodeVersion())));
        data.fields.emplace_back("os", mta::lua::Argument(std::string(os ? os : "?")));
    }

    return mta::lua::push_results(L, mta::lua::Argument(std::move(data)));
}