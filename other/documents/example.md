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

## 9. The canonical `sum` example: types, errors, manual validation

This section is the compact reference for how arguments, results and errors
work end to end; §2 above showed the same mechanics inside a feature.

### The function

```cpp
#include <mta/sdk.hpp>

// Lambda style: the C++ signature IS the documentation -- the binder checks
// the arguments and pushes the result automatically.
MTA_FUNCTION("sum",
    [](double a, double b)
    {
        return a + b;
    });
```

### The calls and their results

```lua
sum(10, 20)        -- 30
```

```lua
sum(10)            -- the second argument is absent
```

The conceptual result of this call is: **argument #2 is missing**. The SDK
renders the same condition in its own diagnostic format (plan §7):

```text
bad argument #2 to 'sum' (expected number, got no value)
```

```lua
sum("10", 20)      -- the first argument has the wrong type
```

The conceptual result of this call is: **argument #1 has invalid type**,
rendered as:

```text
bad argument #1 to 'sum' (expected number, got string)
```

Both concrete formats are pinned by regression tests
(`other/tests/lua/scripts/045_errors.lua`); every message names the argument
position, the function and what was expected/found, so it is usable in
production logs.

### Which types the binder accepts

You can always see what a function accepts: `module_signature("sum")`
returns the derived metadata (argument/return types), and `mta docs` renders
it for every registered function and object method. The supported parameter
types:

| C++ parameter | Lua side |
|---|---|
| `bool` | boolean |
| `double`, `float`, integers | number (integers range-checked) |
| `std::string`, `std::string_view` | string |
| `std::optional<T>` | value or `nil`/absent → `nullopt` |
| `mta::lua::Table` / `Argument` / `rest_args` | table / any value / the tail |
| `mta::async::Callback` | function (stable reference) |
| `mta::lua::context` | — (consumes no Lua argument) |
| `mta::Resource` | name of a running resource, validated live (plan §17) |

Result types: scalars, `std::optional<T>` (`nil` when absent), tuple/pair and
`std::vector<T>` (expanded into several results), `mta::lua::Arguments`,
`void` — and `mta::Resource`, which is pushed as its name.

### How errors are generated

Every registered function runs through a protected trampoline
(`source/sdk/lua/protect.hpp`): C++ exceptions become proper Lua errors and
never escape into the server process (local C++ objects are destroyed
first). Argument problems are raised by the typed readers as categorized
errors (`InvalidType`, `MissingArgument`, `InvalidObject`, …) and rendered as
`bad argument #N to 'name' (…)`; anything categorized `InternalError` is
rendered as `internal module error: …`, so a scripter mistake can never
masquerade as a framework bug (plan §19). The same model covers object
validation (`expected counter, got table`) and native entities
(`no running resource 'xyz'`).

### Manual validation

Automatic binding checks presence, count and type. Anything beyond that —
ranges, cross-field constraints, business rules — you validate in the body
and raise with `mta::lua::raise_error`:

```cpp
MTA_FUNCTION("divide", "Divides a by b and rejects zero.",
    [](double a, double b)
    {
        if (b == 0)
        {
            mta::lua::raise_error("argument #2 must not be zero");
        }
        return a / b;
    });
```

```lua
divide(1, 0)   -- error: argument #2 must not be zero
```

Body style keeps the automatic typed reading (`args<...>`) and adds manual
checks on top:

```cpp
MTA_LUA_FUNCTION("clamp", "Clamps value into [lo, hi].")
{
    auto [value, lo, hi] = mta::lua::args<double, double, double>(L);
    if (lo > hi)
    {
        mta::lua::raise_error("lo must not exceed hi");
    }
    return mta::lua::push_results(L, value < lo ? lo : (value > hi ? hi : value));
}
```

### When automatic binding is not enough

* The parameter list is not known at compile time — use body style with
  `mta::lua::rest_args` / `Argument` snapshots (see `greet_many` above).
* You need raw stack access or a custom calling convention — body style with
  `check_*`/`opt_*` (api.md, "Direct stack access").
* Body-style registration cannot derive signature metadata: `module_signature`
  reports `derived == false` and `mta docs` says so explicitly (plan §9/§10).
* Per-function error declarations are not part of the metadata (the binder
  converts raised errors uniformly); document special errors in the
  description.
* The `self` parameter of object methods is bound by `MTA_METHOD`, not by the
  free-function binder.

---

## Where each concept is exercised in the repo

| Concept | Sample | Tests |
|---|---|---|
| body/lambda registration | `functions/basics/add.cpp`, `hello.cpp` | `010_basic.lua`, `015_facade.lua` |
| tables deep-reading | `functions/tables/table_stats.cpp` | `020_tables.lua` |
| async task + handle | `functions/async/task_demo.cpp` | `035_task.lua` |
| timers + handles | `functions/async/timer_demo.cpp` | `038_timer.lua` |
| callback delivery | `functions/async/async_add.cpp` | `030_async.lua` |
| typed rest_args / context | `functions/basics/typed_params.cpp` | `040_binder.lua` |
| objects | `functions/objects/counter.cpp` | `060_features.lua` |
| events | `functions/basics/trigger_demo.cpp` | `060_features.lua` |
| per-resource state | `functions/state/session.cpp` | `070_lifecycle.lua` |
| restart generations | — (harness-level) | `072_restart.lua` |
| native Resource | `functions/info/resource_info.cpp` | `060_features.lua` |
| native Resource as binder argument/result | `functions/native/resource_args.cpp` | `060_features.lua` |
| stress | — | `080_stress.lua` |
| full restart + shutdown on a real server | `other/server/mta_server.py` | `mta test integration` |