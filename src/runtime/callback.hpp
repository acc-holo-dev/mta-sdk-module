#pragma once

// Стабильная ссылка на Lua-функцию, переживающая кадры DoPulse.
//
// Хранить lua_State* или индекс функции между вызовами нельзя: VM ресурса
// умирает при остановке и пересоздаётся при рестарте. Callback привязывает
// функцию через luaL_ref в реестре VM её ресурса и запоминает имя ресурса.
// При вызове VM снова ищется по имени: остановленный ресурс не стреляет
// никогда, а перезапущенный получает свежий VM вместо битой ссылки.
// Ядро модуля освобождает все ссылки при остановке ресурса.
//
// Только главный поток (как и всё, что касается lua_State).

#include "lua/arguments.hpp"

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

    // Привязывает функцию со стека (бросает исключение на не-функции).
    [[nodiscard]] static Callback from_stack(lua_State *lua_vm, int index);

    [[nodiscard]] bool valid() const noexcept { return ref_ != LUA_NOREF && !resource_.empty(); }
    [[nodiscard]] const std::string &resource() const noexcept { return resource_; }

    // Вызывает привязанную функцию с аргументами. false — если ресурс уже
    // мёртв или сам вызов Lua не удался (логируется).
    bool call(const mta::lua::Arguments &arguments) const;

private:
    void release() noexcept;

    std::string resource_{};
    int ref_ = LUA_NOREF;
};

// Хуки ядра модуля: пометить/освободить всё, что связано с ресурсом.
void invalidate_resource_callbacks(const std::string &resource);
void release_all_callbacks();
} // namespace mta::async
