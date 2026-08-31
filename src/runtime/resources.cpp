#include "runtime/resources.hpp"

#include <algorithm>

namespace mta::resources
{
Hub &Hub::instance()
{
    static Hub hub;
    return hub;
}

void Hub::add(Sink &sink)
{
    if (std::find(sinks_.begin(), sinks_.end(), &sink) == sinks_.end())
    {
        sinks_.push_back(&sink);
    }
}

void Hub::remove(Sink &sink) noexcept
{
    const auto it = std::find(sinks_.begin(), sinks_.end(), &sink);
    if (it != sinks_.end())
    {
        sinks_.erase(it);
    }
}

void Hub::notify_resource_stopping(const std::string &resource)
{
    // Iterate over a copy: a sink may unsubscribe itself inside a callback.
    const auto sinks = sinks_;
    for (auto *sink : sinks)
    {
        if (sink != nullptr)
        {
            sink->on_resource_stopping(resource);
        }
    }
}

void Hub::notify_resource_stopped(const std::string &resource)
{
    const auto sinks = sinks_;
    for (auto *sink : sinks)
    {
        if (sink != nullptr)
        {
            sink->on_resource_stopped(resource);
        }
    }
}

void Hub::notify_all_released()
{
    const auto sinks = sinks_;
    for (auto *sink : sinks)
    {
        if (sink != nullptr)
        {
            sink->on_all_released();
        }
    }
}
} // namespace mta::resources
