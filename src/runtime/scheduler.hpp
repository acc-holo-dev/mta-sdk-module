#pragma once

// Фоновые задачи с доставкой результатов в главный поток.
//
// VM Lua НЕ потокобезопасен: любое касание lua_State должно происходить в
// главном потоке сервера. Планировщик поэтому гоняет чистый C++ на воркерах
// и доставляет результаты в главном потоке внутри DoPulse (см. pump()),
// куда ядро модуля заходит каждый кадр сервера.
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
// Таймеры срабатывают тоже в главном потоке и автоматически отменяются
// при остановке ресурса-владельца.

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

    // Поднимает воркеры. Повторный вызов безопасен.
    void start();
    // Останавливает воркеров и сбрасывает все очереди. Вызывается при
    // завершении работы модуля.
    void stop();
    // Главный поток: раздаёт готовые результаты и срабатывает таймеры.
    // Никогда не бросает.
    void pump();

    // Выполняет work() на воркере; затем completion(results, error)
    // вызывается в главном потоке во время pump(); error == nullptr при
    // успехе.
    void post_task(std::function<mta::lua::Arguments()> work,
                   std::function<void(const mta::lua::Arguments &, const char *)> completion);

    // Вызывает completion(tick) каждые delay_ms, repeat_count раз
    // (0 = пока не отменят или не остановится ресурс). Возвращает id > 0.
    [[nodiscard]] std::uint64_t post_timer(std::string resource, int delay_ms, int repeat_count,
                                            std::function<void(std::uint64_t)> completion);
    bool cancel_timer(std::uint64_t timer_id);

    // Отменяет таймеры ресурса, который только что остановился.
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
