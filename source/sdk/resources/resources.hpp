#pragma once

// Per-resource state with automatic cleanup.
//
// Every MTA resource lives in its own VM, and that VM dies when the resource
// stops. Everything the module stores per resource must be reset in
// ResourceStopped. Store does this for you:
//
//     namespace
//     {
//     mta::resources::Store<MySession> g_sessions;
//     }
//
//     MTA_LUA_FUNCTION("session_get", "...")
//     {
//         MySession &session = g_sessions.for_state(L);
//         ...
//     }
//
// Use only from module functions (main thread, live VM): the store
// determines the calling resource through the module manager.

#include "sdk/lua/protect.hpp"
#include "sdk/abi/module.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace mta::resources
{
// Lifecycle-notification recipient; the module core calls the hub from
// ResourceStopping/ResourceStopped/ShutdownModule on the main thread.
class Sink
{
public:
    virtual ~Sink() = default;
    virtual void on_resource_stopping(const std::string &resource)
    {
        (void)resource;
    }
    virtual void on_resource_stopped(const std::string &resource) = 0;
    virtual void on_all_released() = 0;
};

class Hub
{
public:
    static Hub &instance();

    void add(Sink &sink);
    void remove(Sink &sink) noexcept;

    void notify_resource_stopping(const std::string &resource);
    void notify_resource_stopped(const std::string &resource);
    void notify_all_released();

private:
    Hub() = default;
    std::vector<Sink *> sinks_{};
};

// Per-resource data of type T. T must be default-constructible.
template <typename T>
class Store final : public Sink
{
    static_assert(std::is_default_constructible_v<T>, "per-resource state type must be default-constructible");

public:
    Store()
    {
        Hub::instance().add(*this);
    }

    ~Store() override
    {
        Hub::instance().remove(*this);
    }

    Store(const Store &) = delete;
    Store &operator=(const Store &) = delete;

    // Data of the resource that is calling now. Created on first access.
    T &for_state(lua_State *lua_vm)
    {
        const std::string resource = mta::module::current_resource_name(lua_vm);
        if (resource.empty())
        {
            mta::lua::raise_error("could not determine the calling resource");
        }
        return data_[resource];
    }

    [[nodiscard]] T *try_find(const std::string &resource) noexcept
    {
        const auto it = data_.find(resource);
        return it == data_.end() ? nullptr : &it->second;
    }

    void on_resource_stopped(const std::string &resource) override
    {
        data_.erase(resource);
    }

    void on_all_released() override
    {
        data_.clear();
    }

private:
    std::unordered_map<std::string, T> data_{};
};
} // namespace mta::resources
