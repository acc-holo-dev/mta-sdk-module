#pragma once

// Background tasks with results delivered on the main thread.
//
// The Lua VM is NOT thread-safe: any lua_State access must happen on the
// server's main thread. The scheduler therefore runs pure C++ on workers and
// delivers results on the main thread inside DoPulse (see pump()), where the
// module core enters on every server frame.
//
//     MTA_LUA_FUNCTION("fetch", "...")
//     {
//         auto callback = std::make_shared<mta::async::Callback>(
//             mta::async::Callback::from_stack(L, 3));
//         mta::async::Scheduler::instance().post_task(
//             [url = mta::lua::check_string(L, 1)] {
//                 mta::lua::Arguments result;
//                 result.push_string(do_http_get(url));
//                 return result;
//             },
//             [callback](const mta::lua::Arguments &result, const char *error) {
//                 if (error != nullptr) { mta::log::error("fetch: ", error); return; }
//                 callback->call(result);
//             });
//         return mta::lua::push_results(L, true);
//     }
//
// Timers also fire on the main thread and are cancelled automatically when
// the owning resource stops.

#include "lua/arguments.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace mta::async
{
class Scheduler
{
public:
    static Scheduler &instance();

    // Spawns the workers. Repeated calls are safe.
    void start();
    // Stops the workers and clears every queue. Called on module shutdown.
    void stop();
    // Main thread: dispatches finished results and fires timers. Never throws.
    void pump();

    // Runs work() on a worker; then completion(results, error) runs on the
    // main thread during pump(); error == nullptr on success.
    void post_task(std::function<mta::lua::Arguments()> work,
                   std::function<void(const mta::lua::Arguments &, const char *)> completion);

    // Calls completion(tick) every delay_ms, repeat_count times
    // (0 = until cancelled or the resource stops). Returns an id > 0.
    [[nodiscard]] std::uint64_t post_timer(std::string resource, int delay_ms, int repeat_count,
                                            std::function<void(std::uint64_t)> completion);
    bool cancel_timer(std::uint64_t timer_id);

    // Cancels the timers of a resource that just stopped.
    void handle_resource_stopped(const std::string &resource);

    [[nodiscard]] bool running() const noexcept;

private:
    Scheduler();
    ~Scheduler();
    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

    void worker_loop();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mta::async
