// Built-in introspection: a list of every module function with its description.

#include <mta/sdk.hpp>

#include <string>

MTA_LUA_FUNCTION("module_functions",
    "Returns a table {name = description} with all module functions.")
{
    mta::lua::Table table;
    for (const auto &spec : mta::registry::Registry::instance().functions())
    {
        table.fields.emplace_back(
            spec.name, mta::lua::Argument(std::string(spec.description ? spec.description : "")));
    }
    return mta::lua::push_results(L, mta::lua::Argument(std::move(table)));
}