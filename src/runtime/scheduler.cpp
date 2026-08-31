#include "runtime/scheduler.hpp"

#include "runtime/logging.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace mta::async
{
namespace
{
constexpr int worker_count = 3;
constexpr int minimum_timer_delay_ms = 1;

using clock_type = std::chrono::steady_clock;

struct Task
{
    std::function<mta::lua::Arguments()> work;
    std::function<void(const mta::lua::Arguments &, const char *)> completion;
};

struct Completion
{
    mta::lua::Arguments results;
    std::string error;
    std::function<void(const mta::lua::Arguments &, const char *)> completion;
};

struct Timer
{
    std::uint64_t id = 0;
    std::string resource;
    clock_type::time_point next_fire{};
    int interval_ms = 1;
    std::int64_t repeats_left = 0; // 0 = бесконечно
    std::uint64_t fired = 0;
    std::function<void(std::uint64_t)> completion;
};
} // namespace

struct Scheduler::Impl
{
    std::mutex queue_mutex{};
    std::condition_variable queue_signal{};
    std::deque<Task> tasks{};
    std::vector<Completion> completions{};
    std::vector<std::thread> workers{};
    std::atomic<bool> stopping{false};

    // Таймеры живут только в главном потоке (pump/handle_resource_stopped/
    // post_timer), мьютекс не нужен.
    std::vector<Timer> timers{};
    std::uint64_t next_timer_id = 1;
};

Scheduler &Scheduler::instance()
{
    static Scheduler scheduler;
    return scheduler;
}

Scheduler::Scheduler()
    : impl_(std::make_unique<Impl>())
{
}

Scheduler::~Scheduler()
{
    stop();
}

void Scheduler::start()
{
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    if (!impl_->workers.empty())
    {
        return;
    }

    impl_->stopping.store(false);
    impl_->workers.reserve(static_cast<std::size_t>(worker_count));
    for (int i = 0; i < worker_count; ++i)
    {
        impl_->workers.emplace_back([this] { worker_loop(); });
    }
}

void Scheduler::worker_loop()
{
    while (true)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock(impl_->queue_mutex);
            impl_->queue_signal.wait(lock, [this] {
                return impl_->stopping.load() || !impl_->tasks.empty();
            });

            if (impl_->stopping.load())
            {
                return; // незавершённые задачи сбрасываются при остановке
            }

            task = std::move(impl_->tasks.front());
            impl_->tasks.pop_front();
        }

        Completion completion;
        try
        {
            completion.results = task.work();
        }
        catch (const std::exception &e)
        {
            completion.error = e.what();
        }
        catch (...)
        {
            completion.error = "unknown C++ exception in async task";
        }

        completion.completion = std::move(task.completion);

        {
            std::lock_guard<std::mutex> lock(impl_->queue_mutex);
            if (impl_->stopping.load())
            {
                return; // сервер выключается: ничего не доставляем
            }
            impl_->completions.push_back(std::move(completion));
        }
    }
}

void Scheduler::stop()
{
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->stopping.store(true);
    }
    impl_->queue_signal.notify_all();

    for (auto &worker : impl_->workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
    impl_->workers.clear();

    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->tasks.clear();
        impl_->completions.clear();
    }
    impl_->timers.clear();
}

void Scheduler::pump()
{
    // Раздаём готовые результаты фоновых задач.
    std::vector<Completion> ready;
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        ready.swap(impl_->completions);
    }

    for (auto &entry : ready)
    {
        try
        {
            entry.completion(entry.results, entry.error.empty() ? nullptr : entry.error.c_str());
        }
        catch (const std::exception &e)
        {
            mta::log::error("сбой асинхронного completion: ", e.what());
        }
        catch (...)
        {
            mta::log::error("сбой асинхронного completion: неизвестное исключение C++");
        }
    }

    // Срабатываем таймеры, у которых подошло время.
    const auto now = clock_type::now();
    std::vector<Timer> keep;
    keep.reserve(impl_->timers.size());

    for (auto &timer : impl_->timers)
    {
        if (timer.next_fire <= now)
        {
            ++timer.fired;
            try
            {
                timer.completion(timer.fired);
            }
            catch (const std::exception &e)
            {
                mta::log::error("сбой таймера: ", e.what());
            }
            catch (...)
            {
                mta::log::error("сбой таймера: неизвестное исключение C++");
            }

            timer.next_fire = now + std::chrono::milliseconds(timer.interval_ms);
            if (timer.repeats_left > 0 && --timer.repeats_left == 0)
            {
                continue; // лимит повторов исчерпан — убрать таймер
            }
        }
        keep.push_back(std::move(timer));
    }

    impl_->timers.swap(keep);
}

void Scheduler::post_task(std::function<mta::lua::Arguments()> work,
                          std::function<void(const mta::lua::Arguments &, const char *)> completion)
{
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        if (impl_->stopping.load())
        {
            return;
        }
        impl_->tasks.push_back(Task{std::move(work), std::move(completion)});
    }
    impl_->queue_signal.notify_one();
}

std::uint64_t Scheduler::post_timer(std::string resource, int delay_ms, int repeat_count,
                                    std::function<void(std::uint64_t)> completion)
{
    Timer timer;
    timer.id = impl_->next_timer_id++;
    timer.resource = std::move(resource);
    timer.interval_ms = std::max(delay_ms, minimum_timer_delay_ms);
    timer.repeats_left = repeat_count < 0 ? 0 : repeat_count;
    timer.next_fire = clock_type::now() + std::chrono::milliseconds(timer.interval_ms);
    timer.completion = std::move(completion);

    impl_->timers.push_back(std::move(timer));
    return impl_->next_timer_id - 1;
}

bool Scheduler::cancel_timer(std::uint64_t timer_id)
{
    const auto it = std::find_if(impl_->timers.begin(), impl_->timers.end(),
                                 [timer_id](const Timer &timer) { return timer.id == timer_id; });
    if (it == impl_->timers.end())
    {
        return false;
    }
    impl_->timers.erase(it);
    return true;
}

void Scheduler::handle_resource_stopped(const std::string &resource)
{
    const auto it = std::remove_if(impl_->timers.begin(), impl_->timers.end(),
                                   [&resource](const Timer &timer) { return timer.resource == resource; });
    impl_->timers.erase(it, impl_->timers.end());
}

bool Scheduler::running() const noexcept
{
    return !impl_->stopping.load() && !impl_->workers.empty();
}
} // namespace mta::async
