// Асинхронный паттерн: считаем на воркере, результат уходит в callback через
// DoPulse. Callback — просто типизированный параметр; каркас сам привяжет
// Lua-функцию к ресурсу и переживёт его рестарты.

#include <memory>
#include <utility>

#include "lua/arguments.hpp"
#include "registry/registry.hpp"
#include "runtime/callback.hpp"
#include "runtime/logging.hpp"
#include "runtime/scheduler.hpp"

MTA_LUA_FUNCTION("sample_async_add",
    "Складывает два числа на воркере; callback(sum) вызывается на ближайшем DoPulse.")
{
    auto [a, b, callback] = mta::lua::args<double, double, mta::async::Callback>(L);

    // std::function требует копируемых целей — оборачиваем move-only
    // Callback в make_shared при захвате в completion.
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
                mta::log::error("sample_async_add не удался: ", error);
                return;
            }
            cb->call(result);
        });

    return mta::lua::push_results(L, true);
}
