#pragma once

// Реестр Lua-функций и макросы регистрации.
//
// Основной способ — тело-стиль с типизированными аргументами:
//
//     MTA_LUA_FUNCTION("my_sum", "Складывает два числа.")
//     {
//         auto [a, b] = mta::lua::args<double, double>(L);
//         return mta::lua::push_results(L, a + b);
//     }
//
// Короткие однострочники — лямбда-стиль (типы из сигнатуры, возврат авто):
//
//     MTA_LUA_FUNC("my_sum", "Складывает два числа.",
//         [](double a, double b) { return a + b; });
//
// Оба макроса можно писать в любом .cpp под src/ — сборка подхватит файл,
// а регистрация произойдёт автоматически при загрузке модуля.

#include "lua/bind.hpp"

#include <cstddef>
#include <vector>

struct ILuaModuleManager10;

namespace mta::registry
{
// Сырая точка входа (C++-linkage); каркас оборачивает её в Lua-функцию.
using module_function = int (*)(lua_State *lua_vm);

struct Spec
{
    const char *name;
    const char *description;
    module_function function;
};

class Registry
{
public:
    static Registry &instance();

    void add(Spec spec);

    // Регистрирует все функции в VM ресурса (при старте каждого ресурса).
    void register_all(ILuaModuleManager10 &manager, lua_State *lua_vm) const;

    [[nodiscard]] const std::vector<Spec> &functions() const noexcept { return functions_; }
    [[nodiscard]] std::size_t size() const noexcept { return functions_.size(); }

private:
    Registry() = default;
    std::vector<Spec> functions_{};
};
} // namespace mta::registry

#define MTA_CAT_(a, b) a##b
#define MTA_CAT(a, b) MTA_CAT_(a, b)

// --- лямбда-стиль (короткий) -------------------------------------------------

#define MTA_LUA_FUNC_IMPL(Name, Description, Function, Counter) \
    [[maybe_unused]] static const bool MTA_CAT(mta_func_registered_, Counter) = \
        ::mta::lua::detail::register_typed<Counter>((Name), (Description), (Function))

// MTA_LUA_FUNC("имя", "описание", функция-лямбда);
#define MTA_LUA_FUNC(Name, Description, Function) \
    MTA_LUA_FUNC_IMPL((Name), (Description), (Function), __COUNTER__)

// --- тело-стиль (основной) ----------------------------------------------------

#define MTA_LUA_FUNCTION_IMPL(Name, Description, Counter) \
    static int MTA_CAT(mta_body_, Counter)(lua_State * L); \
    [[maybe_unused]] static const bool MTA_CAT(mta_registered_, Counter) = \
        ::mta::lua::detail::register_function( \
            (Name), (Description), \
            +[](lua_State * L) -> int { \
                return ::mta::lua::protected_call(L, &MTA_CAT(mta_body_, Counter)); \
            }); \
    static int MTA_CAT(mta_body_, Counter)(lua_State * L)

// MTA_LUA_FUNCTION("имя", "описание") { тело; аргументы через args<...> }
#define MTA_LUA_FUNCTION(Name, Description) \
    MTA_LUA_FUNCTION_IMPL((Name), (Description), __COUNTER__)
