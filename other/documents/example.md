# Example — a complete module feature, end to end

This walkthrough builds a small but realistic feature — a **player greeting
service** with a synchronous API, background work, timers, an object type
and per-resource state — using only the public V2 surface. Every construct
shown here exists in the bundled sample module (`source/functions/`), so you
can see each one compiled and tested in place.

*Prerequisites: `mta doctor` reports READY (see api.md, "The `mta` CLI").*

---

## 1. Identity: config/module.toml

The module's identity lives in exactly one file. For a new project run
`mta init` (full scaffold) or edit the bundled one:

```toml
[module]
name = "greeter"       # -> greeter.dll / greeter.so, greeter_* metadata
title = "Greeter Module"
author = "You"
version = "0.1.0"

[build]
cxx_standard = 20
unity = true
lto = true

[async]
workers = "auto"
queue = 1024

[features]
async = true
userdata = true
events = true
objects = true
```

The binary name, the CMake target names, the registration metadata, `mta
package` output and `mta doctor` diagnostics all derive from `[module]` —
nothing is duplicated in C++ sources.

---

## 2. A synchronous function

`source/functions/greeter/greet.cpp` — files under `source/` are picked up
automatically; registration happens at static-init time:

```cpp
#include <mta/sdk.hpp>

// Body style: full control, explicit argument reading.
MTA_LUA_FUNCTION("greet", "Builds a greeting for a name.")
{
    auto [name] = mta::lua::args<std::string>(L);
    return mta::lua::push_results(L, "Hello, " + name + "!");
}
```

```lua
print(greet("World"))          -- Hello, World!
greet(42)                      -- error: bad argument #1 to 'greet'
                               --        (expected string, got number)
```

The same function in lambda style — signatures are introspected, argument
checks and result pushing are automatic:

```cpp
MTA_FUNCTION("greet", "Builds a greeting for a name.",
    [](std::string name) { return "Hello, " + name + "!"; });
```

Optional arguments, variadic tails and the call context:

```cpp
MTA_LUA_FUNCTION("greet_custom", "Greets with an optional custom salutation.")
{
    auto [name, salutation] =
        mta::lua::args<std::string, std::optional<std::string>>(L);
    return mta::lua::push_results(L,
        salutation.value_or("Hello") + ", " + name);
}

MTA_LUA_FUNCTION("greet_many", "Greets several names at once.")
{
    auto [rest] = mta::lua::args<mta::lua::rest_args>(L);
    mta::lua::Arguments results;
    for (std::size_t i = 0; i < rest.values.count(); ++i)
    {
        results.push_string("Hello, " + rest.values.at(i).as_string() + "!");
    }
    return mta::lua::push_results(L, results);
}

MTA_LUA_FUNCTION("greet_where", "Greets and reports the calling resource.")
{
    auto [name, ctx] = mta::lua::args<std::string, mta::lua::context>(L);
    mta::log::info("greet_where called by resource ", ctx.resource);
    return mta::lua::push_results(L, "Hello, " + name);
}
```

---

## 3. Background work with a callback

Heavy work runs on a worker thread and delivers on the main thread inside
`DoPulse`. The worker closure must not touch Lua — hand values through
`mta::lua::Arguments` and deliver through an `mta::async::Callback`:

```cpp
// greet_later(name, delay_ms, callback) -- callback(text) on the main thread
MTA_LUA_FUNCTION("greet_later", "Greets after delay_ms on a worker thread.")
{
    auto [name, delay_ms, callback] =
        mta::lua::args<std::string, std::int64_t, mta::async::Callback>(L);

    // std::function needs copyable targets; Callback is move-only.
    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    mta::async::Task task = mta::async::run(
        L,
        [name, delay_ms]() -> mta::lua::Arguments {   // worker: no Lua here
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(delay_ms)));
            mta::lua::Arguments result;
            result.push_string("Hello, " + name + " (from a worker)");
            return result;
        },
        [cb](const mta::lua::Arguments &result, const char *error) {
            if (error != nullptr)
            {
                mta::log::error("greet_later failed: ", error);
                return;
            }
            cb->call(result);                          // main thread
        });

    if (!task.valid())
    {
        mta::lua::raise_error("task queue is full: task not accepted");
    }
    return mta::lua::push_results(L);   // the completion arrives later
}
```

```lua
greet_later("async", 500, function(text)
    print(text)          -- Hello, async (from a worker)
end)
```

Handles: `run()` returns a `Task` — `cancel()` (queued tasks never run;
a running task completes but its delivery is suppressed), `done()`,
`valid()`, `id()`. The task belongs to the calling resource: if it stops,
queued tasks are cancelled and stale completions are dropped before any Lua
access; a restart of the same resource runs a fresh VM generation that can
never receive them.

---

## 4. A cancellable timer

```cpp
MTA_LUA_FUNCTION("greet_after", "Greets once after delay_ms. Returns the timer id.")
{
    auto [delay_ms, callback] = mta::lua::args<std::int64_t, mta::async::Callback>(L);
    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    mta::timer::Timer timer = mta::timer::after(L, static_cast<int>(delay_ms), [cb] {
        mta::lua::Arguments args;
        args.push_string("Hello from a timer!");
        cb->call(args);
    });
    if (!timer.valid())
    {
        mta::lua::raise_error("timer was not accepted");
    }
    return mta::lua::push_results(L, static_cast<lua_Number>(timer.id()));
}
```

`mta::timer::every(L, delay_ms, fn)` repeats; both return a `Timer` handle
(`cancel()/valid()/id()`). Timers are resource-aware: a resource stop
invalidates every owned timer, and no timer of an older generation can ever
fire into a restarted resource. The bundled `sample_after`/
`sample_after_cancel` show the full handle bookkeeping with a
`Store<TimerMap>`.

---

## 5. An object type

```cpp
struct Greeter
{
    std::string salutation = "Hello";
};

// Declares the identity ONCE: metatable "mta.greeter.greeter", stable
// across compilers, collision-free across modules.
MTA_OBJECT("greeter", Greeter)

void register_greeter_methods(lua_State *L)
{
    MTA_METHOD(Greeter, "greet", [](Greeter &self, std::string name) {
        return self.salutation + ", " + name + "!";
    });
    MTA_METHOD(Greeter, "set_salutation",
        [](Greeter &self, std::string s) { self.salutation = std::move(s); });
}
const bool greeter_methods_registered = [] {
    mta::userdata::Registry<Greeter>::set_methods(&register_greeter_methods);
    return true;
}();

MTA_LUA_FUNCTION("greeter_create", "Creates a greeter object.")
{
    auto [salutation] = mta::lua::args<std::optional<std::string>>(L);
    Greeter g;
    if (salutation.has_value()) { g.salutation = *salutation; }
    mta::userdata::Registry<Greeter>::create(L, std::move(g));
    return 1;
}
```

```lua
local g = greeter_create("Hi")
print(g:greet("Sam"))                  -- Hi, Sam!
g:set_salutation("Greetings")
print(g:greet("Sam"))                  -- Greetings, Sam!
g = nil                                -- __gc destroys the C++ object

greeter_create({})                     -- error: bad argument #1 to
                                       --   'greeter_create' (expected string, got table)
```

Wrong-typed objects are caught by identity:
`bad argument #1 to 'greet' (expected greeter, got table)`.

---

## 6. Per-resource state

State that belongs to a resource disappears when the resource stops —
`mta::resources::Store<T>` does the bookkeeping:

```cpp
namespace
{
struct GreetStats { std::uint64_t total = 0; };
mta::resources::Store<GreetStats> g_stats;
}

MTA_LUA_FUNCTION("greet_count", "How many greetings this resource made.")
{
    GreetStats &stats = g_stats.for_state(L);
    ++stats.total;
    return mta::lua::push_results(L, static_cast<lua_Number>(stats.total));
}
```

After `restart resource greeter` the count starts from 1 again — the store
was erased on stop. Long-lived handles that outlive a single call (task ids,
timer ids) live in the same kind of store; see `sample_task_run` and
`sample_after`.

---

## 7. Events and resources

```cpp
// Trigger an MTA event the resource's scripts can handle:
MTA_LUA_FUNCTION("greet_broadcast", "Fires onGreeterGreet with a message.")
{
    auto [message] = mta::lua::args<std::string>(L);
    mta::lua::Arguments args;
    args.push_string(message);
    const bool ok = mta::events::trigger(L, "onGreeterGreet", args);
    return mta::lua::push_results(L, ok);
}

// Native (safe) resource lookup:
MTA_LUA_FUNCTION("greet_find", "true if a resource is running.")
{
    auto [name] = mta::lua::args<std::string>(L);
    auto resource = mta::Resource::find(name);
    return mta::lua::push_results(L, resource.has_value() && resource->alive());
}
```

```lua
addEventHandler("onGreeterGreet", root, function(message)
    outputServerLog("greeter event: " .. message)
end)
```

---

## 8. Build, test, install

```text
mta doctor                 -- environment ready?
mta build                  -- configure + build (default preset)
mta test all               -- unit (C++) + lua (embedded VM) suites
mta test integration       -- real pinned MTA server run (PHASE 11 harness)
mta package                -- dist/greeter-0.1.0-<platform>/ + sha256
mta server install         -- isolated pinned server (first time only)
mta server test            -- the same integration through the harness
```

Manual install: copy the built binary into `<mta-server>/server/x64/modules/`
and add to `mtaserver.conf`:

```xml
<module src="greeter.dll" />
```

Restart the server (or `restart <resource>` after re-copying) and call the
functions from any resource. The integration harness in
`other/server/mta_server.py` does exactly this automatically — read its
generated `sdkintegration` resource for a complete runnable script.

---

## Where each concept is exercised in the repo

| Concept | Sample | Tests |
|---|---|---|
| body/lambda registration | `functions/basics/add.cpp`, `hello.cpp` | `010_basic.lua`, `015_facade.lua` |
| tables deep-reading | `functions/tables/table_stats.cpp` | `020_tables.lua` |
| async task + handle | `functions/async/task_demo.cpp` | `035_task.lua` |
| timers + handles | `functions/async/timer_demo.cpp` | `038_timer.lua` |
| callback delivery | `functions/async/async_add.cpp` | `030_async.lua` |
| objects | `functions/objects/counter.cpp` | `060_features.lua` |
| events | `functions/basics/trigger_demo.cpp` | `060_features.lua` |
| per-resource state | `functions/state/session.cpp` | `070_lifecycle.lua` |
| restart generations | — (harness-level) | `072_restart.lua` |
| native Resource | `functions/info/resource_info.cpp` | `060_features.lua` |
| stress | — | `080_stress.lua` |
| full restart + shutdown on a real server | `other/server/mta_server.py` | `mta test integration` |