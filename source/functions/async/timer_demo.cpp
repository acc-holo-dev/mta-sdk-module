// Timer handles (plan §15): mta::timer::after fires once, every repeats
// until cancelled or the owning resource stops. Both return a handle with
// cancel()/valid(); timers are resource-aware and never survive a restart
// of their resource.
//
//     local id = sample_after(500, function() ... end)
//     sample_after_cancel(id)  -- true: the callback will never fire

#include <mta/sdk.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

namespace
{
// Live timer handles of the calling resource (main thread only; cleared
// when the resource stops).
using TimerMap = std::unordered_map<std::uint64_t, mta::timer::Timer>;
mta::resources::Store<TimerMap> g_timers;

// Finds the handle of the calling resource; nullptr when unknown.
mta::timer::Timer *find_timer(lua_State *L, std::uint64_t id)
{
    auto &timers = g_timers.for_state(L);
    const auto it = timers.find(id);
    return it == timers.end() ? nullptr : &it->second;
}
} // namespace

MTA_LUA_FUNCTION("sample_after",
    "Calls callback() once after delay_ms on the main thread. Returns the "
    "timer id for sample_after_cancel/sample_timer_valid.")
{
    auto [delay_ms, callback] = mta::lua::args<std::int64_t, mta::async::Callback>(L);

    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));
    mta::timer::Timer timer = mta::timer::after(
        L, static_cast<int>(delay_ms), [cb] {
            mta::lua::Arguments result;
            result.push_boolean(true);
            cb->call(result);
        });

    if (!timer.valid())
    {
        mta::lua::raise_error("timer was not accepted");
    }

    const std::uint64_t id = timer.id();
    g_timers.for_state(L).emplace(id, std::move(timer));
    return mta::lua::push_results(L, static_cast<lua_Number>(id));
}

MTA_LUA_FUNCTION("sample_every",
    "Calls callback() every delay_ms until cancelled or the resource "
    "stops. Returns the timer id.")
{
    auto [delay_ms, callback] = mta::lua::args<std::int64_t, mta::async::Callback>(L);

    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));
    mta::timer::Timer timer = mta::timer::every(L, static_cast<int>(delay_ms), [cb] {
        mta::lua::Arguments result;
        result.push_boolean(true);
        cb->call(result);
    });

    if (!timer.valid())
    {
        mta::lua::raise_error("timer was not accepted");
    }

    const std::uint64_t id = timer.id();
    g_timers.for_state(L).emplace(id, std::move(timer));
    return mta::lua::push_results(L, static_cast<lua_Number>(id));
}

MTA_LUA_FUNCTION("sample_after_cancel",
    "Cancels a timer created by sample_after/sample_every: its callback "
    "will never fire again. true if cancelled.")
{
    auto [id] = mta::lua::args<std::int64_t>(L);

    auto *timer = find_timer(L, static_cast<std::uint64_t>(id));
    if (timer == nullptr)
    {
        return mta::lua::push_results(L, false);
    }

    const bool cancelled = timer->cancel();
    if (cancelled)
    {
        g_timers.for_state(L).erase(static_cast<std::uint64_t>(id));
    }
    return mta::lua::push_results(L, cancelled);
}

MTA_LUA_FUNCTION("sample_timer_valid",
    "true while the timer is still scheduled (will fire again).")
{
    auto [id] = mta::lua::args<std::int64_t>(L);

    auto *timer = find_timer(L, static_cast<std::uint64_t>(id));
    return mta::lua::push_results(L, timer != nullptr && timer->valid());
}