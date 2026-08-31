// Async pattern: compute on a worker, deliver the result through a callback
// via DoPulse. The Callback is just a typed parameter; the framework binds the
// Lua function to the resource and survives its restarts.

#include <memory>
#include <utility>

#include "lua/arguments.hpp"
#include "registry/registry.hpp"
#include "runtime/callback.hpp"
#include "runtime/logging.hpp"
#include "runtime/scheduler.hpp"

MTA_LUA_FUNCTION("sample_async_add",
    "Adds two numbers on a worker; callback(sum) fires on the next DoPulse.")
{
    auto [a, b, callback] = mta::lua::args<double, double, mta::async::Callback>(L);

    // std::function needs copyable targets, so the move-only Callback is
    // wrapped in make_shared when captured into the completion.
    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    mta::async::Scheduler::instance().post_task(
        [a, b]() -> mta::lua::Arguments {
            mta::lua::Arguments result;
            result.push_number(a + b);
            return result;
        },
        [cb](const mta::lua::Arguments &result, const char *error) {
            if (error != nullptr)
            {
                mta::log::error("sample_async_add failed: ", error);
                return;
            }
            cb->call(result);
        });

    return mta::lua::push_results(L, true);
}
