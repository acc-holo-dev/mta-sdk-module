#pragma once

// Stable reference to a Lua function that survives DoPulse frames and
// resource restarts.
//
// Storing a raw lua_State* or a function index between calls is forbidden:
// a resource's VM is destroyed when the resource stops and recreated on
// restart. A Callback pins the function through luaL_ref in the registry of
// its resource's VM and remembers the resource name. On call, the VM is
// looked up again by name: a stopped resource never fires, and a restarted
// one receives a fresh VM instead of a stale reference. The module core
// releases every reference when its resource stops.
//
// Main thread only (like everything touching lua_State).

#include "sdk/lua/arguments.hpp"

#include <string>

struct lua_State;

namespace mta::async
{
class Callback
{
public:
    Callback() = default;
    Callback(const Callback &) = delete;
    Callback &operator=(const Callback &) = delete;
    Callback(Callback &&other) noexcept;
    Callback &operator=(Callback &&other) noexcept;
    ~Callback();

    // Binds the function on the stack (throws on a non-function).
    [[nodiscard]] static Callback from_stack(lua_State *lua_vm, int index);

    [[nodiscard]] bool valid() const noexcept { return ref_ != LUA_NOREF && !resource_.empty(); }
    [[nodiscard]] const std::string &resource() const noexcept { return resource_; }

    // Calls the bound function with the given arguments. Returns false if the
    // resource is already gone or the Lua call itself failed (logged).
    bool call(const mta::lua::Arguments &arguments) const;

private:
    void release() noexcept;

    std::string resource_{};
    int ref_ = LUA_NOREF;
};

// Module-core hooks: mark/release everything tied to a resource.
void invalidate_resource_callbacks(const std::string &resource);
void release_all_callbacks();
} // namespace mta::async
