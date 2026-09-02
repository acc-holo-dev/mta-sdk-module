#pragma once

// Borrowed state view (plan §18/§45 -- the LuaView half of the value model).
//
// mta::state wraps the CURRENT lua_State for synchronous operations: reading
// typed arguments, pushing results, asking which resource owns the VM. It
// never owns the VM and must never be cached or moved across threads: a
// resource's VM dies when the resource stops and a restarted resource runs
// in a fresh VM (plan §14). Values that travel across async boundaries use
// the owned Snapshot model instead (mta::lua::{Argument, Table, Arguments}).
//
//     MTA_FUNCTION("describe", "Describes the calling resource.")
//     {
//         mta::state s = MTA_STATE(L);
//         return mta::lua::push_results(L, s.resource_name());
//     }

#include "sdk/abi/module.hpp"
#include "sdk/bind/bind.hpp"
#include "sdk/lua/common.hpp"
#include "sdk/lua/stack.hpp"

#include <string>
#include <tuple>
#include <utility>

namespace mta
{
// Borrowed, non-owning view of a Lua 5.1 VM (plan §18: LuaView).
class state
{
public:
    state() noexcept = default;

    // Wraps the VM a module function runs in -- usually spelled MTA_STATE(L).
    explicit state(lua_State *lua_vm) noexcept
        : lua_vm_(lua_vm)
    {
    }

    // The wrapped VM. Valid only for the current synchronous call: never
    // store the handle past the call, across threads, or into async work.
    [[nodiscard]] lua_State *handle() const noexcept { return lua_vm_; }

    // false for a default-constructed view (or a null VM).
    [[nodiscard]] bool valid() const noexcept { return lua_vm_ != nullptr; }
    explicit operator bool() const noexcept { return lua_vm_ != nullptr; }

    // Number of values currently on the stack.
    [[nodiscard]] int top() const noexcept { return lua_gettop(lua_vm_); }

    // The name of the resource owning this VM right now ("" when it cannot
    // be determined).
    [[nodiscard]] std::string resource_name() const noexcept
    {
        return mta::module::current_resource_name(lua_vm_);
    }

    // Typed argument readers -- the same conversions as mta::lua::args:
    //
    //     auto [a, b] = s.args<double, double>();
    template <typename... Ts>
    [[nodiscard]] std::tuple<Ts...> args() const
    {
        return mta::lua::args<Ts...>(lua_vm_);
    }

    // Pushes one or more results -- the same conversions as
    // mta::lua::push_results; returns how many were pushed.
    template <typename... Values>
    int push_results(Values &&...values) const
    {
        return mta::lua::push_results(lua_vm_, std::forward<Values>(values)...);
    }

private:
    lua_State *lua_vm_ = nullptr;
};
} // namespace mta

// The calling VM as a state view (plan §18): expression form, name the view
// yourself. The view is borrowed -- it dies with the call:
//
//     mta::state s = MTA_STATE(L);
#define MTA_STATE(LuaVm) (::mta::state{(LuaVm)})