// Async task handle (plan §13): run() returns a Task that can be cancelled
// before its completion is delivered, and reports its state.
//
//     local id = sample_task_run(100, 2, 3, function(sum) ... end)
//     sample_task_cancel(id)  -- true: the completion will never run

#include <mta/sdk.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>

namespace
{
// Live task handles of the calling resource (main thread only; cleared when
// the resource stops).
using TaskMap = std::unordered_map<std::uint64_t, mta::async::Task>;
mta::resources::Store<TaskMap> g_tasks;
} // namespace

MTA_LUA_FUNCTION("sample_task_run",
    "Computes a + b on a worker after delay_ms; callback(sum) fires on the "
    "main thread. Returns the task id for sample_task_cancel.")
{
    auto [delay_ms, a, b, callback] =
        mta::lua::args<std::int64_t, double, double, mta::async::Callback>(L);

    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    mta::async::Task task = mta::async::run(
        L,
        [delay_ms, a, b]() -> mta::lua::Arguments {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay_ms)));
            mta::lua::Arguments result;
            result.push_number(a + b);
            return result;
        },
        [cb](const mta::lua::Arguments &result, const char *error) {
            if (error != nullptr)
            {
                mta::log::error("sample_task_run failed: ", error);
                return;
            }
            cb->call(result);
        });

    if (!task.valid())
    {
        mta::lua::raise_error("task queue is full: task not accepted");
    }

    const std::uint64_t id = task.id();
    g_tasks.for_state(L).emplace(id, std::move(task));
    return mta::lua::push_results(L, static_cast<lua_Number>(id));
}

MTA_LUA_FUNCTION("sample_task_cancel",
    "Cancels a queued task: its completion will never run. true if cancelled.")
{
    auto [id] = mta::lua::args<std::int64_t>(L);

    auto &tasks = g_tasks.for_state(L);
    const auto it = tasks.find(static_cast<std::uint64_t>(id));
    if (it == tasks.end())
    {
        return mta::lua::push_results(L, false);
    }

    const bool cancelled = it->second.cancel();
    tasks.erase(it);
    return mta::lua::push_results(L, cancelled);
}