# MTA:SA Module SDK

[![CI](https://github.com/acc-holo-dev/mta-sdk-module/actions/workflows/ci.yml/badge.svg)](https://github.com/acc-holo-dev/mta-sdk-module/actions/workflows/ci.yml)

A C++20 SDK for writing native MTA:SA server modules: register typed Lua
functions with one macro, let the binder validate arguments and translate
errors, and get async work, timers, callbacks and userdata objects with
resource-safe lifetimes — without touching the raw Lua stack.

## What it is

MTA:SA loads native modules (`.dll` / `.so`) through a fixed six-function C
ABI. This SDK wraps that ABI in a modern C++ layer so you write ordinary C++
functions that become ordinary Lua-callable functions:

```cpp
#include <mta/sdk.hpp>

MTA_FUNCTION("greet", "Greets a player by name.",
    [](std::string name)
    {
        return "Hello, " + name + "!";
    });
```

```lua
local message = greet("world")     -- "Hello, world!"
local ok, err = pcall(greet, 42)
-- err == "bad argument #1 to 'greet' (expected string, got number)"
```

## What it gives you

- **Typed registration macros** — `MTA_FUNCTION`, `MTA_LUA_FUNCTION` (body
  style), `MTA_LUA_FUNC` (lambda style); the binder derives argument
  validation, result conversion and error messages from the C++ signature.
- **Clear errors** — every mismatch becomes a Lua error in the documented
  `bad argument #N to 'name' (expected X, got Y)` format; no C++ knowledge
  required to diagnose.
- **Value model with an async rule** — owned snapshots
  (`mta::lua::Argument/Table/Arguments`) are the only values that cross the
  async boundary; the borrowed view (`mta::state`, `MTA_STATE`) reads and
  pushes inside the synchronous call.
- **Async runtime** — `mta::async::run` worker tasks with cancellation and
  completion delivery on the main thread; `mta::timer::after` /
  `mta::timer::every` timers; worker count and queue limit from the config.
- **Native objects** — `MTA_OBJECT` + `MTA_METHOD` register userdata types
  with stable metatable identities and compiler-independent metadata.
- **Resource-safe lifetimes** — every callback, task and timer is owned by
  `(resource, generation)`; a restarted resource can never observe objects
  from its previous VM.
- **Logging with context** — `mta::log` prefixes messages with the module
  name and the current call site automatically.
- **Tooling** — the `mta` CLI (init, new, build, test, docs, doctor,
  package, server), a generated-function docs reference (`mta docs`), an
  embedded Lua test harness, and a pinned real-server integration runner.

## Quick start

Requirements: CMake 3.27+, Ninja (or any CMake generator), a supported
compiler (see [Supported platforms](#supported-platforms)), and Python 3.11+
for the `mta` CLI.

```bash
mta init my_mod        # copy this SDK into a new module project
cd my_mod
mta doctor             # verify the environment
mta build              # build/my_mod.dll (win-mingw preset by default)
mta test               # unit + embedded-Lua suites
```

Copy the produced `build/win-mingw/module/win-x64/my_mod.dll` (or the
`.so` from the linux-gcc preset) into your MTA server's `modules/` directory
and list it in `mtaserver.conf`. The module logs its identity at load:

```text
module: loaded my_mod (module 2.1, sdk 1.0.0, abi 1; MTA 1.6.0-9.21788.0)
```

## Create your first function

`mta new function player_bonus` generates a compile-ready skeleton in
`source/functions/` — source discovery picks it up on the next build, no
CMake edits needed. Body style gives you direct control of the Lua stack
through the typed reader:

```cpp
MTA_FUNCTION("player_bonus", "Computes a bonus from level and multiplier.")
{
    auto [level, factor] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, level * factor);
}
```

```lua
player_bonus(10, 1.5)      -- 15
```

## Function arguments and errors

The binder derives everything from the declared parameter types. Missing
arguments, wrong types and invalid objects raise errors a Lua developer can
read without opening C++:

```lua
player_bonus(10)     -- error: bad argument #2 to 'player_bonus'
                     -- (expected number, got no value)
```

Optional parameters use `std::optional<T>`, variadic tails use
`mta::lua::args` with `rest_args`, and `mta::Resource` parameters are
validated live against the running server. The full behavior — including
return values, multiple results and tables — is specified in
[other/documents/example.md](other/documents/example.md).

## Tables / callbacks / async / timers

```cpp
// Async: heavy work on a worker thread, completion on the main thread.
MTA_FUNCTION("sum_slow", "Sums two numbers in the background.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    mta::async::run([a, b] { return a + b; },
                    [](const mta::lua::Arguments &results, const char *error)
                    {
                        // Arguments owns its values: safe across threads.
                    });
    return 0;
}

// One-shot timer, cancelled automatically when the resource stops.
mta::timer::after(L, 1500, [] { mta::log::info("1500 ms tick"); });
```

Callbacks are held by reference with the owning resource recorded; they
survive across calls and are invalidated when their resource stops.

## Project structure

```text
config/module.toml     module identity + build options (single source)
config/cmake/          the CMake implementation (core, lua, platform, install)
source/mta/sdk.hpp     the public facade developers include
source/sdk/            the framework: abi, lua, bind, runtime, registry,
                       objects, resources, events, logging, errors
source/functions/      developer functions (samples included)
source/library/base/   reusable, module-agnostic utilities
other/documents/       example.md, api.md, architecture.md, TUTORIAL.md
other/tests/           unit + embedded-Lua + real-server integration suites
other/server/          pinned MTA server test infrastructure
other/tools/           docgen + the `mta` CLI
other/third_party/     vendored Lua 5.1 + MTA SDK server headers
```

## Configuration

`config/module.toml` is the single configuration file. The build reads it
directly; the CLI reads the same file.

```toml
[module]
name = "base"            # -> base.dll / base.so
title = "Base Module"    # shown in the load diagnostic
author = "Developer"
version = "2.1.0"        # the Module version float (2.1)

[build]
cxx_standard = 20
unity = true
lto = true

[async]
workers = "auto"         # or a fixed worker count
queue = 4096

[features]
async = true
userdata = true
events = true
objects = true
```

Every value can be overridden per configure with CMake cache variables
(`-DSDK_MODULE_NAME=my_mod`, `-DSDK_UNITY=OFF`, ...).

## Build

```bash
mta build                    # default preset for this OS
mta build --preset win-msvc  # or a specific one
cmake --preset win-mingw     # raw CMake works too
cmake --build build/win-mingw
```

Presets: `win-mingw` (default on Windows), `win-msvc`, `linux-gcc`. All
presets build with unity + LTO and treat warnings as errors.

## Test

```bash
mta test unit           # configuration parser tests (CMake script mode)
mta test lua            # embedded Lua 5.1 harness, every function + regressions
mta test integration    # pinned real MTA server (see below)
mta test                # all of the above
ctest --preset win-mingw --output-on-failure   # the same via CTest
```

## MTA Server integration

The integration runner provisions a pinned MTA server build
(other/server/), installs the built module, choreographs resource
start/stop/restart across generations and verifies that no stale callback,
task or timer can reach the new VM:

```bash
mta server install      # once: download + unpack the pinned build
mta test integration
```

No developer-installed server is used; the build identity is recorded in
`other/server/install.json` (not committed).

## Documentation

- [example.md](other/documents/example.md) — the practical manual: every
  feature with C++ + Lua + expected result.
- [api.md](other/documents/api.md) — the full API reference (facade,
  binder, runtime, objects, config, CLI).
- [architecture.md](other/documents/architecture.md) — layers, value
  model, lifetimes, build system, test pyramid.
- [TUTORIAL.md](other/documents/TUTORIAL.md) — a guided first module.
- [GUIDES.md](other/documents/GUIDES.md) — task-oriented recipes.
- [migration-v1-to-v2.md](other/documents/migration-v1-to-v2.md) — upgrading
  a V1-style module.

## Supported platforms

| Platform | Compiler | Status |
| --- | --- | --- |
| Windows x64 | MinGW-w64 (GCC 13+) | built + tested in CI, releases |
| Windows x64 | MSVC (VS 2022+) | built + tested in CI, release artifact |
| Linux x64 | GCC | built + tested in CI, release artifact |
| Linux x64 | Clang | built + tested in CI |

The MTA server ABI is the 64-bit module ABI (`ILuaModuleManager10`); the
SDK bundles byte-identical Lua 5.1 headers with the server's `luaL_newstate`
extension and verifies that identity in CI.

## Release status

`v2.1.0` — refinement release: canonical repository layout
(`source/`, `config/`, `other/`), the full `mta` CLI, doctor diagnostics,
an embedded Lua suite (216 checks) plus benchmarks, and a pinned real-server
integration suite (20 scenarios) running in CI on every push and in the
release pipeline before packaging. Releases attach exactly
`<module>.dll` / `<module>.so` — no source archives, no debug artifacts.

## License

MIT — Copyright (c) 2026 HoloDev