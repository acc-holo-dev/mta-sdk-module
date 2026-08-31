// Список результатов: для переменного количества используем Arguments.
// Целочисленные параметры проверяются на диапазон автоматически.

#include <cstdint>

#include "lua/arguments.hpp"
#include "lua/protect.hpp"
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_range", "Возвращает числа от from до to (несколько результатов).")
{
    auto [from, to] = mta::lua::args<std::int64_t, std::int64_t>(L);

    if (to - from > 1000)
    {
        mta::lua::raise_error("диапазон слишком большой: максимум 1000 чисел");
    }

    mta::lua::Arguments result;
    for (std::int64_t i = from; i <= to; ++i)
    {
        result.push_number(static_cast<lua_Number>(i));
    }
    return result.push(L);
}
