#pragma once

// Пер-ресурсное состояние с автоматической очисткой.
//
// Каждый ресурс MTA живёт в своём VM, и этот VM умирает при остановке
// ресурса. Всё, что модуль хранит на ресурс, обязано сбрасываться в
// ResourceStopped. Store делает это сам:
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
// Использовать только из функций модуля (главный поток, живой VM): хранилище
// определяет вызывающий ресурс через менеджер модуля.

#include "lua/protect.hpp"
#include "module/module.hpp"

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace mta::resources
{
// Получатель уведомлений жизненного цикла; ядро модуля зовёт хаб из
// ResourceStopping/ResourceStopped/ShutdownModule в главном потоке.
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

// Данные на ресурс типа T. T должен быть default-constructible.
template <typename T>
class Store final : public Sink
{
    static_assert(std::is_default_constructible_v<T>, "тип состояния ресурса должен иметь конструктор по умолчанию");

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

    // Данные ресурса, который сейчас вызывает. Создаётся при первом обращении.
    T &for_state(lua_State *lua_vm)
    {
        const std::string resource = mta::module::current_resource_name(lua_vm);
        if (resource.empty())
        {
            mta::lua::raise_error("не удалось определить вызывающий ресурс");
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
