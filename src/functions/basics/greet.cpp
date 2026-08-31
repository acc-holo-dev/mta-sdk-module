// Необязательный аргумент через std::optional: nil или отсутствие дают
// nullopt, дефолт достаётся value_or.

#include <optional>
#include <string>

#include "registry/registry.hpp"

MTA_LUA_FUNCTION("sample_greet", "Приветствие; greeting можно не передавать (nil).")
{
    auto [name, greeting] = mta::lua::args<std::string, std::optional<std::string>>(L);
    return mta::lua::push_results(L, greeting.value_or("привет") + ", " + name);
}
