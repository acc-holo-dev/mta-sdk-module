// Несколько результатов: push_results принимает несколько значений.

#include <algorithm>

#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_minmax", "Возвращает минимум и максимум из двух чисел (два результата).")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, std::min(a, b), std::max(a, b));
}
