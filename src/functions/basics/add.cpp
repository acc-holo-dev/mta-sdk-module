// Простейшая функция: аргументы читаются по типам из args<...>, проверки и
// понятные ошибки — автоматически.

#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_add", "Возвращает сумму двух чисел.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, a + b);
}
