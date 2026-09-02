// The state view (plan §18): mta::state / MTA_STATE wraps the calling VM for
// synchronous operations -- borrowed access, never stored.

#include <mta/sdk.hpp>

MTA_LUA_FUNCTION("sample_state",
    "Returns a table {resource = ..., top = ...} describing the calling VM "
    "through the mta::state view.")
{
    mta::state view = MTA_STATE(L);

    mta::lua::Table data;
    data.fields.emplace_back("resource", mta::lua::Argument(view.resource_name()));
    data.fields.emplace_back("top", mta::lua::Argument(static_cast<lua_Number>(view.top())));
    return mta::lua::push_results(L, mta::lua::Argument(std::move(data)));
}