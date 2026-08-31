#include "runtime/callback.hpp"

#include "ILuaModuleManager10.h"
#include "lua/protect.hpp"
#include "lua/stack.hpp"
#include "runtime/logging.hpp"
#include "module/module.hpp"

#include <unordered_map>
#include <utility>

namespace mta::async
{
namespace
{
struct TrackedRef
{
    std::string resource;
    bool dead = false;
};

// Учёт живых ссылок. Только главный поток.
std::unordered_map<int, TrackedRef> &tracked_refs()
{
    static std::unordered_map<int, TrackedRef> refs;
    return refs;
}

bool ref_is_dead(int ref) noexcept
{
    const auto &refs = tracked_refs();
    const auto it = refs.find(ref);
    return it == refs.end() || it->second.dead;
}

// Убирает ссылку из учёта; luaL_unref делает, пока VM ещё достижим.
void untrack_ref(int ref) noexcept
{
    auto &refs = tracked_refs();
    const auto it = refs.find(ref);
    if (it == refs.end())
    {
        return;
    }

    if (!it->second.dead)
    {
        if (auto *manager = mta::module::manager())
        {
            if (lua_State *vm = manager->GetResourceFromName(it->second.resource.c_str()))
            {
                luaL_unref(vm, LUA_REGISTRYINDEX, ref);
            }
        }
    }

    refs.erase(it);
}
} // namespace

Callback::Callback(Callback &&other) noexcept
    : resource_(std::move(other.resource_)),
      ref_(other.ref_)
{
    other.ref_ = LUA_NOREF;
    other.resource_.clear();
}

Callback &Callback::operator=(Callback &&other) noexcept
{
    if (this != &other)
    {
        release();
        resource_ = std::move(other.resource_);
        ref_ = other.ref_;
        other.ref_ = LUA_NOREF;
        other.resource_.clear();
    }
    return *this;
}

Callback::~Callback()
{
    release();
}

Callback Callback::from_stack(lua_State *lua_vm, int index)
{
    if (lua_isfunction(lua_vm, index) == 0)
    {
        mta::lua::raise_error("callback must be a function, got ",
                              mta::lua::detail::type_name(lua_type(lua_vm, index)));
    }

    const std::string resource = mta::module::current_resource_name(lua_vm);
    if (resource.empty())
    {
        mta::lua::raise_error("не удалось определить вызывающий ресурс для callback");
    }

    lua_pushvalue(lua_vm, index);
    const int ref = luaL_ref(lua_vm, LUA_REGISTRYINDEX);

    tracked_refs()[ref] = TrackedRef{resource, false};

    Callback callback;
    callback.resource_ = resource;
    callback.ref_ = ref;
    return callback;
}

bool Callback::call(const mta::lua::Arguments &arguments) const
{
    if (!valid() || ref_is_dead(ref_))
    {
        return false;
    }

    auto *manager = mta::module::manager();
    if (manager == nullptr)
    {
        return false;
    }

    lua_State *vm = manager->GetResourceFromName(resource_.c_str());
    if (vm == nullptr)
    {
        return false; // ресурс остановлен (рестартнувший дал бы свежий VM)
    }

    const int base = lua_gettop(vm);
    lua_rawgeti(vm, LUA_REGISTRYINDEX, ref_);
    if (lua_isfunction(vm, -1) == 0)
    {
        lua_settop(vm, base);
        return false;
    }

    const int pushed = arguments.push(vm);
    if (lua_pcall(vm, pushed, 0, 0) != LUA_OK)
    {
        const char *message = lua_tostring(vm, -1);
        mta::log::error("сбой callback модуля: ", message ? message : "неизвестная ошибка Lua");
        lua_settop(vm, base);
        return false;
    }

    lua_settop(vm, base);
    return true;
}

void Callback::release() noexcept
{
    if (ref_ != LUA_NOREF)
    {
        untrack_ref(ref_);
        ref_ = LUA_NOREF;
        resource_.clear();
    }
}

void invalidate_resource_callbacks(const std::string &resource)
{
    for (auto &[ref, tracked] : tracked_refs())
    {
        if (tracked.resource == resource)
        {
            tracked.dead = true; // VM умрёт сразу после — unref не нужен
        }
    }
}

void release_all_callbacks()
{
    auto &refs = tracked_refs();
    for (auto it = refs.begin(); it != refs.end();)
    {
        if (!it->second.dead)
        {
            if (auto *manager = mta::module::manager())
            {
                if (lua_State *vm = manager->GetResourceFromName(it->second.resource.c_str()))
                {
                    luaL_unref(vm, LUA_REGISTRYINDEX, it->first);
                }
            }
        }
        it = refs.erase(it);
    }
}
} // namespace mta::async
