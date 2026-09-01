# MTA:SA Lua Module — SDK

[![CI](https://github.com/acc-holo-dev/mta-sdk-module/actions/workflows/ci.yml/badge.svg)](https://github.com/acc-holo-dev/mta-sdk-module/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A solid foundation for [MTA:SA](https://multitheftauto.com) server modules:
a dynamic library (base.dll / base.so by default) that the MTA server loads
and that adds native Lua functions of its own. The binary name is
configurable at CMake time — see [Module identity](#module-identity).

Functions are written in **plain C++ with a body**; arguments are read by
type automatically — no manual check_number, no indices, no "this is a
number, this is a string":

```cpp
// source/functions/basics/my_sum.cpp
#include "sdk/registry/registry.hpp"

MTA_LUA_FUNCTION("my_sum", "Adds two numbers.")
{
    auto [a, b] = mta::lua::args<double, double>(L);   // types and checks automatic
    return mta::lua::push_results(L, a + b);           // return is automatic too
}
```

Rebuild and my_sum is available in every server resource. New .cpp files are
picked up automatically, and so is their registration.

---

## Table of contents

- [Architecture](#architecture)
- [Module identity](#module-identity)
- [Building](#building)
- [Server installation](#server-installation)
- [Writing functions](#writing-functions)
- [Argument and result types](#argument-and-result-types)
- [Safety rules](#safety-rules)
- [Testing](#testing)

> For the full design — layers, data flows, threading rules — see
> [other/documents/ARCHITECTURE.md](other/documents/ARCHITECTURE.md).

## Architecture

```
config/
└── module.toml    # single project configuration (identity, build, async)

source/
├── functions/    👉 THE ONLY folder you touch
│   ├── basics/   # sample_add, sample_echo, sample_greet, sample_tag,
│   │            #   sample_minmax, sample_range
│   ├── tables/   # sample_table_stats, sample_table_get/set
│   ├── info/     # sample_version, module_functions
│   ├── async/    # sample_async_add, sample_timer(+cancel)
│   ├── events/   # sample_trigger_event
│   ├── objects/  # counter_create — userdata methods example
│   ├── state/    # sample_session_hit — per-resource state
│   └── raw/      # sample_stack_dump — direct stack access
├── library/      # reusable C++ helpers (no Lua registration)
└── sdk/          # framework internals (module/lua/registry/runtime)
    ├── abi/      #   MTA contract: six entry points, export tables
    ├── lua/      #   Lua stack helpers: mta::lua
    │   ├── bind.hpp is at sdk/bind/  👉 binder: args<...>, pull/push
    │   ├── argument.hpp  #   Argument: one Lua value + tables
    │   ├── arguments.hpp  #   Arguments: a list of values
    │   └── protect.hpp   #   exceptions -> Lua errors
    ├── registry/ #   registry + MTA_LUA_FUNCTION / MTA_LUA_FUNC macros
    ├── runtime/  #   scheduler, callback
    ├── resources/#   per-resource state hub/store
    ├── objects/  #   userdata metatables
    ├── events/   #   module -> Lua events
    └── logging/  #   mta::log

other/
├── tests/        # lua/ (embedded harness + scripts), unit/, integration/
├── server/       # real-MTA-server test infrastructure
├── documents/    # API.md, ARCHITECTURE.md, GUIDES.md, TUTORIAL.md
├── tools/        # developer CLI (mta)
└── third_party/  # mta-sdk (SDK headers) + lua (Lua 5.1.5)
cmake/            # build infrastructure
```

Put your own functions into domain folders inside functions/ — for example
functions/crypto/ or functions/http/. A domain is created by simply adding a
folder with a .cpp: the whole source/**/*.cpp tree is built automatically.

## Configuration (config/module.toml)

The project is configured in **one file** — `config/module.toml`. CMake reads
it directly (before `project()`), so the identity and build options defined
there flow into the binary, the MTA registration, packaging and diagnostics:

```toml
[module]
name = "base"              # -> base.dll / base.so, mtaserver.conf <module src="base"/>
title = "Base Module"      # console name shown when the module loads
author = "Developer"       # console author
version = "2.0.0"          # single version source (project VERSION too)

[build]
cxx_standard = 20
unity = true
lto = true

[async]
workers = "auto"           # worker threads for background tasks
queue = 4096

[features]
async = true
userdata = true
events = true
objects = true
```

Change `name`, reconfigure, and the artifact becomes `my_mod.dll` — nothing
else to touch. Advanced users can still override any value per configure
with CMake cache variables (the TOML stays the default):

```bash
cmake --preset win-mingw -DSDK_MODULE_NAME=my_mod
cmake --preset win-mingw -DSDK_MODULE_TITLE="My Module" -DSDK_MODULE_AUTHOR="Jane Doe"
```

- `SDK_MODULE_NAME` — output binary name **without extension**; default
  `base` (produces `base.dll` on Windows, `base.so` on Linux). It also names
  the CPack ZIP and the Windows export table (module.def.in).
- `SDK_MODULE_TITLE` / `SDK_MODULE_AUTHOR` — the name/author the server
  shows in the console when the module loads.

The parser lives in `cmake/core/module-config.cmake`; `[async]` and
`[features]` are consumed by the subsystems that own them (async, objects).
See [other/documents/ARCHITECTURE.md](other/documents/ARCHITECTURE.md#6-configuration--no-source-edits-needed).

## Building

Requirements: CMake ≥ 3.27, Ninja and a compiler with C++20 and std::thread
(MinGW-w64 posix-threads, MSVC 2019+ or GCC/Clang on Linux).

```bash
# Windows (MinGW-w64)
cmake --preset win-mingw && cmake --build --preset win-mingw

# Windows (MSVC)
cmake --preset win-msvc && cmake --build --preset win-msvc

# Linux (GCC)
cmake --preset linux-gcc && cmake --build --preset linux-gcc
```

Artifact: `build/<preset>/module/<platform>-<arch>/<SDK_MODULE_NAME>.dll`
(e.g. `module/win-x64/base.dll`); the MinGW runtime is linked statically.

## Server installation

1. Copy `base.dll` (or your `SDK_MODULE_NAME`) into the server's
   mods/deathmatch/modules/.
2. Add <module src="base"/> (the file name without extension) to
   mtaserver.conf.
3. Restart the server; the console should show
   MODULE: Loaded "Base Module" (1.10) by "anon".

Module bitness = server bitness (modern MTA is x64).

## Writing functions

### A simple function

```cpp
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("my_sum", "Adds two numbers.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, a + b);
}
```

mta::lua::args<...>(L) reads arguments in order: each type is checked
automatically and a wrong type gives the Lua scripter
argument #N must be <type>, got <actual>. Extra arguments are ignored,
missing ones give …got no value.

### Types and several results

```cpp
MTA_LUA_FUNCTION("my_div", "Divides two numbers.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    if (b == 0.0)
    {
        mta::lua::raise_error("my_div: division by zero");  // -> Lua error
    }
    return mta::lua::push_results(L, a / b);
}

// several results per call:
MTA_LUA_FUNCTION("my_minmax", "Minimum and maximum.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, std::min(a, b), std::max(a, b));
}

// a variable number of results via Arguments:
MTA_LUA_FUNCTION("my_range", "Numbers from 'from' to 'to'.")
{
    auto [from, to] = mta::lua::args<std::int64_t, std::int64_t>(L);
    mta::lua::Arguments result;
    for (auto i = from; i <= to; ++i) result.push_number(static_cast<lua_Number>(i));
    return result.push(L);
}
```

### Optional arguments

```cpp
// std::optional<T>: nil or absence -> nullopt, default via value_or.
MTA_LUA_FUNCTION("my_greet", "Greets a name.")
{
    auto [name, greeting] = mta::lua::args<std::string, std::optional<std::string>>(L);
    return mta::lua::push_results(L, greeting.value_or("hello") + ", " + name);
}
```

### Tables

```cpp
MTA_LUA_FUNCTION("my_table_demo", "Table example.")
{
    auto [table] = mta::lua::args<mta::lua::Table>(L);   // non-table -> error
    double sum = 0.0;
    for (const auto &value : table.array)
    {
        if (value.type() == mta::lua::Argument::Type::Number)
        {
            sum += value.as_number();
        }
    }

    mta::lua::Table result;
    result.fields.emplace_back("sum", mta::lua::Argument(sum));
    return mta::lua::push_results(L, mta::lua::Argument(std::move(result)));
}
```

mta::lua::Argument accepts ANY value (tables recursively, up to 32 levels).

### Variadics (unknown argument count)

```cpp
MTA_LUA_FUNCTION("my_echo", "Returns every argument back.")
{
    mta::lua::Arguments arguments;
    arguments.read(L);
    lua_settop(L, 0);
    return arguments.push(L);
}
```

### Async function and timers

```cpp
#include <memory>
#include "lua/arguments.hpp"
#include "registry/registry.hpp"
#include "runtime/callback.hpp"
#include "runtime/logging.hpp"
#include "runtime/scheduler.hpp"

MTA_LUA_FUNCTION("my_async", "Computes in the background; callback(result) runs on DoPulse.")
{
    auto [value, callback] = mta::lua::args<double, mta::async::Callback>(L);
    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    mta::async::Scheduler::instance().post_task(
        [value] {                       // background thread: NO Lua!
            mta::lua::Arguments result;
            result.push_number(heavy(value));
            return result;
        },
        [cb](const mta::lua::Arguments &result, const char *error) {
            if (error != nullptr) { mta::log::error("my_async: ", error); return; }
            cb->call(result);           // main thread: safe
        });

    return mta::lua::push_results(L, true);
}
```

Callback is a parameter type: binding the Lua function, surviving resource
restarts and auto-release are handled by the framework. Timers — see
source/functions/async/timers.cpp (sample_timer/sample_timer_cancel). Callback
is move-only — wrap it in make_shared when capturing into a completion.

### Per-resource state

```cpp
namespace
{
struct MySession { std::string token; int requests = 0; };
mta::resources::Store<MySession> g_sessions;   // one static per .cpp
}

MTA_LUA_FUNCTION("my_session_hit", "Hit counter of the resource.")
{
    MySession &session = g_sessions.for_state(L);   // the VM is available in the body
    ++session.requests;
    return mta::lua::push_results(L, static_cast<lua_Number>(session.requests));
}
```

The record is erased automatically when the resource stops.

### Logging

```cpp
mta::log::info("cache loaded: ", count, " entries");
mta::log::error("load failed: ", code);
mta::log::debug(L, "called with ", n, " arguments");   // bound to the resource
```

### Direct stack access (exotic)

The function body has lua_State *L available — you can do anything with the
stack directly:

```cpp
MTA_LUA_FUNCTION("my_raw", "Description.")
{
    return lua_gettop(L);   // any low-level code; the framework catches exceptions
}
```

A live example is source/functions/raw/stack_dump.cpp.

### Lambda style (short one-liners)

For one-liners there is a second macro, MTA_LUA_FUNC with a lambda: types
are read from the signature and the return is automatic:

```cpp
MTA_LUA_FUNC("my_sum", "Adds two numbers.",
    [](double a, double b) { return a + b; });
```

## Argument and result types

| Argument in args<...> | From Lua |
|---|---|
| double, float | number |
| int, int64_t, … | integer (range-checked) |
| bool | boolean |
| std::string / std::string_view | string |
| mta::lua::Argument | any value (tables recursively) |
| mta::lua::Table | table |
| mta::async::Callback | function (stable reference) |
| std::optional<T> | T or nil/nothing |

| Result in push_results | In Lua |
|---|---|
| a value (number/string/bool/table/…) | one result |
| several values separated by commas | several results |
| mta::lua::Arguments (+ push) | a whole result list |
| nullptr | nil |

A wrong argument type becomes a readable Lua error like
argument #1 must be a number, got string — straight from args<...>;
no manual checks are needed.

## Safety rules

1. Never store lua_State * between calls. A resource's VM dies when the
   resource stops. For deferred calls use mta::async::Callback; for data use
   mta::resources::Store.
2. Touch Lua only on the main thread. From workers — pure C++, results via
   post_task -> DoPulse.
3. Functions are global for every resource — give them unique names.
4. Exceptions never leave the module: the macros translate everything into
   Lua errors; the C++ stack unwinds correctly.
5. Module bitness = server bitness.

## Testing

```bash
cmake --build --preset win-mingw --target sdk_tests
ctest --preset win-mingw            # or run sdk_tests.exe directly
```

The harness (other/tests/lua/harness.cpp) starts a clean Lua 5.1, installs a mock
manager and runs other/tests/lua/scripts/*.lua: basic functions, tables, async,
timers and every binder feature (optional, multiple results, variadics,
direct stack). Add your own scripts — they are picked up automatically.

## CI and releases

GitHub Actions builds and tests the module on Linux (GCC) and Windows
(MinGW-w64 and MSVC). Pushing a tag like v1.1.0 builds .dll/.so artifacts
and attaches them to a GitHub Release — see .github/workflows/.

The module title/author and binary name are configured via
SDK_MODULE_TITLE / SDK_MODULE_AUTHOR / SDK_MODULE_NAME (see
[Module identity](#module-identity)); the version comes from
`project(VERSION ...)` in CMakeLists.txt.