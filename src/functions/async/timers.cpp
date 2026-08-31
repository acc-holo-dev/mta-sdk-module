// Таймеры: callback(tick) срабатывает в главном потоке каждые delay_ms,
// repeat_count раз (0 = пока не отменят). Таймер привязан к ресурсу
// callback-а и отменяется автоматически при остановке ресурса.

#include <cstdint>
#include <memory>
#include <utility>

#include "lua/arguments.hpp"
#include "registry/registry.hpp"
#include "runtime/callback.hpp"
#include "runtime/scheduler.hpp"

MTA_LUA_FUNCTION("sample_timer",
    "Вызывает callback(tick) каждые delay_ms, repeat_count раз (0 = бесконечно). Возвращает id таймера.")
{
    auto [delay, repeats, callback] =
        mta::lua::args<std::int64_t, std::int64_t, mta::async::Callback>(L);

    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    const auto timer_id = mta::async::Scheduler::instance().post_timer(
        cb->resource(), static_cast<int>(delay), static_cast<int>(repeats),
        [cb](std::uint64_t tick) {
            mta::lua::Arguments arguments;
            arguments.push_number(static_cast<lua_Number>(tick));
            cb->call(arguments);
        });

    return mta::lua::push_results(L, static_cast<lua_Number>(timer_id));
}

MTA_LUA_FUNCTION("sample_timer_cancel",
    "Отменяет таймер, созданный sample_timer. true, если таймер был отменён.")
{
    auto [timer_id] = mta::lua::args<std::int64_t>(L);
    return mta::lua::push_results(
        L, mta::async::Scheduler::instance().cancel_timer(static_cast<std::uint64_t>(timer_id)));
}
