#pragma once

// Native MTA types (plan §17) -- safe wrappers only.
//
// The frozen ILuaModuleManager10 ABI exposes exactly one VM lookup:
// GetResourceFromName. There is NO element/player/vehicle API behind the
// module boundary, so this layer ships only what can be represented safely.
// Wrappers like mta::Player would have to call engine functions by NAME
// inside a foreign VM (getPlayerFromName, ...) with no type guarantee
// across server versions and nothing verifiable in the harness -- the
// plan's condition ("only if the corresponding MTA API is available and can
// be represented safely") is documented as NOT met for elements, and met
// for resources:
//
//     if (auto res = mta::Resource::find("play"); res && res->vm() != nullptr)
//     {
//         // res->vm() is the live lua_State of that resource
//     }
//
//     if (auto self = mta::Resource::current(L))
//     {
//         mta::log::info("called from resource ", self->name());
//     }

#include "sdk/lua/common.hpp"

#include <optional>
#include <string>

namespace mta
{
class Resource
{
public:
    // Looks a RUNNING resource up through the module manager ABI. Returns
    // nothing when the module manager is absent (before InitModule / after
    // ShutdownModule) or the resource is not running.
    [[nodiscard]] static std::optional<Resource> find(std::string name);

    // The resource owning lua_vm (mta::module::current_resource_name);
    // nothing when it cannot be determined.
    [[nodiscard]] static std::optional<Resource> current(lua_State *lua_vm);

    [[nodiscard]] const std::string &name() const noexcept { return name_; }

    // The VM of the resource, looked up LIVE on every call: never cache a
    // lua_State, the server can destroy it at any time (plan §14). Returns
    // nullptr once the resource stopped -- callers must handle that.
    [[nodiscard]] lua_State *vm() const noexcept;

    // Convenience: vm() != nullptr.
    [[nodiscard]] bool alive() const noexcept { return vm() != nullptr; }

private:
    explicit Resource(std::string name) noexcept
        : name_(std::move(name))
    {
    }

    std::string name_;
};
} // namespace mta