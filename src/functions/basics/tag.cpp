// Тот же std::optional, но nil здесь — осмысленное значение.

#include <optional>
#include <string>

#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_tag", "value/tag; tag необязателен (nil).")
{
    auto [value, tag] = mta::lua::args<std::string, std::optional<std::string>>(L);
    return mta::lua::push_results(L, value + "/" + tag.value_or("none"));
}
