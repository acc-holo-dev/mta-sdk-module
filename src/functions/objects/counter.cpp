// userdata/метатаблицы: объект-счётчик с методами get/set/add и __gc.
//
//     local c = counter_create(42)
//     c:get()   -- 42
//     c:set(100)
//     c:add(5)  -- 105
//     c = nil   -- __gc вызовет ~Counter()

#include "lua/userdata.hpp"
#include "registry/registry.hpp"

namespace
{
struct Counter
{
    double value = 0;
};

// Регистрирует методы типа. Вызывается Registry один раз на каждый VM
// (у каждого ресурса свой lua_State и своя метатаблица).
void register_counter_methods(lua_State *L)
{
    MTA_METHOD(Counter, "get", [](Counter &self) { return self.value; });
    MTA_METHOD(Counter, "set", [](Counter &self, double v) { self.value = v; });
    MTA_METHOD(Counter, "add", [](Counter &self, double v) {
        self.value += v;
        return self.value;
    });
}

// Один раз на процесс: привязываем регистратор методов к типу.
const bool counter_methods_registered = [] {
    mta::userdata::Registry<Counter>::set_methods(&register_counter_methods);
    return true;
}();
} // namespace

MTA_LUA_FUNCTION("counter_create", "Создаёт объект-счётчик с методами get/set/add.")
{
    auto [value] = mta::lua::args<double>(L);
    mta::userdata::Registry<Counter>::create(L, Counter{value});
    return 1;
}
