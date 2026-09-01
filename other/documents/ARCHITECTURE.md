# Architecture

This document describes the architecture of the MTA:SA Lua module SDK at a
level that lets a new person understand the whole system in one sitting: what
every directory is for, how the pieces talk to each other, and the rules that
keep the module crash-safe inside the MTA server process.

> For usage see [README.md](../README.md), for the API see [API.md](API.md),
> for deep dives see [GUIDES.md](GUIDES.md).

---

## 1. What this project is

MTA:SA server modules are **dynamic libraries** (`.dll` / `.so`) loaded into
the server process. The server (CLuaModule in mtasa-blue) resolves six C
entry points by name, calls them at well-defined moments, and expects the
module to register native Lua functions into every resource's Lua VM.

This project is a **framework + starter template** for such a module:

```
+---------------------------------------------------------------+
|                    MTA:SA server process                        |
|                                                                |
|  +----------------------------+       +---------------------+  |
|  |  module binary (base.dll)  |       |  Lua 5.1 VM (per    |  |
|  |  = sdk_base (this repo)    |       |  resource)          |  |
|  |                            |       |                     |  |
|  |  InitModule                |-----> |  registered module  |  |
|  |  RegisterFunctions  -------------> |  functions          |  |
|  |  DoPulse (every frame)     |       |  scripts call them  |  |
|  |  ResourceStopping/Stopped  |       |                     |  |
|  |  ShutdownModule            |       +---------------------+  |
|  +----------------------------+                                |
|         |  ^                                                     |
|         |  | triggerEvent (module -> Lua)                      |
|         v  |                                                     |
|  +----------------------------+                                |
|  |  ILuaModuleManager10       |                                |
|  |  (MTA SDK, server side)    |                                |
|  +----------------------------+                                |
+---------------------------------------------------------------+
```

The framework part gives you, out of the box:

- a **typed binder** — write plain C++ functions, arguments are checked and
  results are returned automatically;
- an **async scheduler** — background threads for pure C++ work, results
  delivered on the server's main thread;
- **stable callbacks** — references to Lua functions that survive resource
  restarts;
- **per-resource state** with automatic cleanup;
- a **test harness** that runs everything without an MTA server.

---

## 2. Directory layout at a glance

```
mta-sdk-module/
├── config/                  # module.toml (project configuration)
├── CMakeLists.txt           # targets, options, module identity
├── CMakePresets.json        # win-mingw / win-msvc / linux-gcc
├── cmake/                   # build infrastructure
│   ├── core/                #   platform tags, common flags, source glob
│   ├── config/              #   per-compiler settings (msvc/mingw/linux)
│   ├── lua/                 #   vendored Lua 5.1 compilation
│   └── install.cmake        #   install rules + CPack ZIP
├── source/
│   ├── sdk/                 # framework internals (never edit)
│   │   ├── abi/             #   MTA contract — the six entry points
│   │   ├── lua/             #   Lua-stack helpers (Argument, stack, protect)
│   │   ├── bind/            #   typed binder
│   │   ├── registry/        #   function registry + registration macros
│   │   ├── runtime/         #   scheduler, callback
│   │   ├── resources/       #   per-resource state (Hub, Store)
│   │   ├── objects/         #   userdata metatables
│   │   ├── events/          #   module -> Lua events
│   │   └── logging/         #   mta::log
│   ├── library/             # reusable C++ helpers (no Lua registration)
│   └── functions/           # 👉 YOUR functions, grouped by domain
├── other/
│   ├── tests/               # lua/ (harness + scripts), unit/, integration/
│   ├── server/              # real-MTA-server test infrastructure
│   ├── documents/            # this doc, API.md, GUIDES.md, TUTORIAL.md
│   ├── tools/                # developer CLI
│   └── third_party/         # frozen third-party code (never edit)
│       ├── mta-sdk/         #   ILuaModuleManager10.h + Lua headers
│       └── lua/src/         #   Lua 5.1 (MTA-patched) sources
```

---

## 3. The layers of `source/sdk/`

Dependencies must point **downward only**: a layer may use anything below it,
never above it. The arrows show "uses".

```
                 ┌──────────────────────┐
                 │   functions/         │   ← the only layer you extend
                 │   your Lua functions │
                 └──────────┬───────────┘
                            │ MTA_LUA_FUNCTION / MTA_LUA_FUNC
                 ┌──────────▼───────────┐
                 │   registry/          │   ← collects functions, registers
                 │   Registry, Spec     │     them into each resource VM
                 └──────────┬───────────┘
          ┌─────────────────┼──────────────────┐
          ▼                 ▼                  ▼
  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
  │   lua/       │  │  runtime/    │  │  abi/        │
  │ stack, args; │  │ scheduler,  │  │ MTA entry    │
  │ bind/,       │  │ callback;   │  │ points, info │
  │ objects/,    │  │ resources/, │  │              │
  │ events/      │  │ logging/    │  │              │
  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘
         │                 │                 │
         └─────────────────┴───────┬─────────┘
                                   ▼
                        ┌───────────────────┐
                        │ other/third_party/ (Lua 5.1, │
                        │ MTA SDK headers)  │
                        └───────────────────┘
```

### 3.1 `source/sdk/abi/` — the MTA contract

The only place that talks to the server loader. It contains the six exported
entry points (`InitModule`, `RegisterFunctions`, `DoPulse`,
`ShutdownModule`, `ResourceStopping`, `ResourceStopped`) and forwards them to
small internal functions in `mta::module`. It also owns the module identity
(`Info`), which is injected at compile time from CMake, and the module
manager pointer.

**Exported surface.** On Windows exports are pinned by a generated `.def`
file, on Linux by a GNU ld version script (`module.ver`). Both allow exactly
the six entry points — nothing else leaks from the DLL, so module symbols
cannot collide with other modules in the server process.

### 3.2 `source/sdk/registry/` — the function registry

A process-wide singleton (`Registry`) holding `Spec{name, description,
function}` entries, filled at static-init time by the registration macros.
When the server starts a resource it calls `RegisterFunctions`; the module
replays the registry into that resource's VM through
`ILuaModuleManager10::RegisterFunction`.

The two author-facing macros live here:

| Macro | Style | Use |
|---|---|---|
| `MTA_LUA_FUNCTION(name, desc) { ... }` | body style | primary: body has `lua_State *L` |
| `MTA_LUA_FUNC(name, desc, lambda)` | lambda style | short one-liners |

Each macro creates a static registrar that adds the function once; new `.cpp`
files are compiled and picked up automatically (a recursive glob in
`cmake/core/file.cmake`).

### 3.3 `source/sdk/lua/` — stack helpers & the typed binder

The framework's core value. Everything is in `namespace mta::lua`:

- `common.hpp` — one include point for the vendored Lua headers.
- `stack.hpp` — type-safe `check_*`/`opt_*` readers and `push_results` that
  **throw C++ exceptions** instead of calling `luaL_error` directly, so local
  C++ objects are destroyed properly on bad input.
- `protect.hpp` — exception → Lua error conversion (`raise_error`) and the
  `protected_call` trampoline that every registered function runs through.
- `argument.hpp` / `arguments.cpp` — `Argument` (a snapshot of one Lua value,
  tables read recursively up to 32 levels) and `Table` (array + fields).
- `arguments.hpp` / `arguments.cpp` — `Arguments`: a flat result/argument
  list used for marshaling and for calling Lua.
- `table_helpers.hpp` — `get_field`/`set_field`/`convert` for `Table`.

The typed binder itself lives next door in `sdk/bind/bind.hpp`: the template
machinery that reads typed parameters from the Lua stack, synthesizes missing
optional ones, applies C++ defaults, dispatches on the reduced arity, and
pushes results. This is where `args<double, double>(L)` comes from.

Lua-facing objects and events have their own folders:

- `sdk/objects/userdata.hpp` — `Registry<T>`: Lua objects as userdata with
  methods and a `__gc` destructor.
- `sdk/events/events.hpp` / `events.cpp` — `mta::events::trigger`: fires an
  MTA event into the resource's scripts (module → Lua).

### 3.4 `source/sdk/runtime/` and friends — the engine

Background work and long-lived state, kept out of the per-call path:

- `sdk/runtime/scheduler` — a small thread pool (3 workers). `post_task(work,
  completion)` runs pure C++ on a worker; results are queued and delivered
  on the **main thread** during `DoPulse` → `pump()`. Timers live here too,
  fire on the main thread, and are cancelled automatically when their
  resource stops.
- `sdk/runtime/callback` — `mta::async::Callback`: a stable reference to a
  Lua function (`luaL_ref` + resource name). Survives `DoPulse` frames and
  resource restarts; never fires into a stopped resource.
- `sdk/resources/resources` — `mta::resources::Hub` + `Store<T>`:
  per-resource data that is erased automatically on `ResourceStopped`. This
  is the safe replacement for "stored global state" in a multi-VM world.
- `sdk/logging/logging` — `mta::log::{debug,info,warn,error}` with levels;
  goes to the MTA console via the manager, falls back to stdout in the
  harness.

### 3.5 `source/functions/` — your code

Domains are freeform folders: `basics/`, `tables/`, `async/`, `events/`,
`objects/`, `state/`, `raw/`, `info/`. Dropping a `.cpp` into any of them
(including a **new folder**) is all that is needed to ship a new Lua
function — no central registration list, no CMake edits. The shipped
`sample_*` functions double as executable documentation; remove or replace
them as you build your own.

---

## 4. Key flows

### 4.1 Startup & registration

```
server loads base.dll
        │  LoadLibrary / dlsym
        ▼
InitModule(manager, name, author, version)
        │  store manager pointer
        │  start scheduler workers
        ▼
server starts a resource  ──►  new Lua VM created
        │
        ▼
RegisterFunctions(vm)
        │  registry.register_all(manager, vm)
        ▼
for each Spec: manager.RegisterFunction(vm, name, fn)   ← Lua scripts can now call it
```

### 4.2 A single call (typed binder)

```
Lua: sample_add(2, 3)
        │
        ▼
trampoline (registry macro) ──► protected_call(L, &body)
        │
        ▼
holder<Tag,F>::entry(L)
        │  choose dispatch slot from lua_gettop(L)
        │  pull params: pull_param<U>(L)             ← type-checked readers
        ▼
body: auto [a, b] = mta::lua::args<double, double>(L);
        │  ...
        ▼
push_results(L, a + b)   →  5 returned to Lua
        │
        ▼
exceptions anywhere  ──►  caught at the trampoline  ──►  luaL_error (readable message)
```

Any C++ exception becomes a Lua error. The server process never sees a
thrown exception escape the module.

### 4.3 Async completion & timers

```
main thread (DoPulse)                    worker threads
─────────────────────                    ──────────────
function call                            work()  (pure C++, no Lua)
post_task(work, completion)  ──────────► does the job
        │                                 builds mta::lua::Arguments
        ▼                                        │
... wait for frames                            result
        │                                        ▼
DoPulse ──► Scheduler::pump()
        │  swap completed queue
        │  completion(results, error)  ◄─────── (mutex)
        │  callback->call(args) — Lua again, safe ✓
        ▼
timers: due timers snapshotted, fired, re-armed or dropped
```

### 4.4 Resource lifecycle & shutdown

```
ResourceStopping(vm) ──► Hub::notify_resource_stopping(name)
ResourceStopped(vm)  ──► Hub::notify_resource_stopped(name)
                        │  Store<T> erases the resource's record
                        ▼  Scheduler::handle_resource_stopped(name)
                        │     ─ cancels the resource's timers
                        ▼  Callback refs marked dead
ShutdownModule        ──► Scheduler::stop()      (join workers first)
                        │  release_all_callbacks (unref while VMs alive)
                        │  Hub::notify_all_released()
                        ▼  manager pointer = nullptr
```

Order in shutdown matters: workers stop first (their completions may still
hold callbacks), then Lua references are released while the VMs are
reachable.

---

## 5. Threading rules

The Lua VM is **not thread-safe**, and every resource owns its own VM.

1. Touch Lua **only on the server's main thread**.
2. Workers run **pure C++**; results travel back through `pump()`.
3. Never store `lua_State *` between calls — the VM dies with the resource.
   Use `mta::async::Callback` (deferred calls) or
   `mta::resources::Store` (data).
4. Module bitness must equal server bitness (modern MTA is x64).
5. Functions are registered into **every** resource — keep names unique and
   prefix them if your domain is generic.

---

## 6. Configuration (no source edits needed)

The project configuration lives in **`config/module.toml`** — the single
source of truth for module identity and build options. CMake parses it
directly (`cmake/core/module-config.cmake`) before `project()`, so even
`project(VERSION)` comes from the TOML. The same file is read by the `mta`
CLI and by `mta doctor`.

```toml
[module]
name = "base"        # binary name -> base.dll / base.so
title = "Base Module" # console name
author = "Developer"
version = "2.0.0"    # single version source

[build]
cxx_standard = 20
unity = true
lto = true

[async]
workers = "auto"     # consumed by the async subsystem
queue = 4096

[features]
async = true         # consumed by the owning subsystems
userdata = true
events = true
objects = true
```

Every value can be overridden per configure with a CMake cache variable —
an explicit `-D` always wins over the TOML default:

| Variable | TOML default | Effect |
|---|---|---|
| `SDK_MODULE_NAME` | `[module] name` | Output binary name → `base.dll` / `base.so` |
| `SDK_MODULE_TITLE` | `[module] title` | Module name shown in the server console |
| `SDK_MODULE_AUTHOR` | `[module] author` | Author shown in the server console |
| `CMAKE_CXX_STANDARD` | `[build] cxx_standard` | C++ standard |
| `SDK_UNITY` | `[build] unity` | Unity build |
| `SDK_LTO` | `[build] lto` | Link-time optimization |
| `SDK_BUILD_TESTS` | `ON` | Build the `sdk_tests` harness |
| `SDK_SANITIZE` | `OFF` | ASan/UBSan (GCC/Clang, debug only) |
| `SDK_LUA_WARNINGS_OFF` | `ON` | Silence third-party Lua warnings |

Examples:

```bash
cmake --preset win-mingw -DSDK_MODULE_NAME=my_mod
cmake --preset win-mingw -DSDK_MODULE_NAME=my_mod -DSDK_MODULE_TITLE="My Module" -DSDK_MODULE_AUTHOR="Jane"
```

The version comes from `config/module.toml` → `project(VERSION ...)` — bump
only there. The module binary lands at
`build/<preset>/module/<platform>-<arch>/<name>.dll` (or `.so`).

Presets: `win-mingw`, `win-msvc`, `linux-gcc` (see `CMakePresets.json`).

---

## 7. Testing & CI

- `other/tests/lua/harness.cpp` boots a clean Lua 5.1 (the MTA-patched ABI, verified
  for the `mtasaowner` argument), installs a mock `ILuaModuleManager10`, and
  runs every `other/tests/lua/scripts/*.lua` — no MTA server needed. Helpers:
  `test_assert`, `test_pump`, `test_resource_stop/start`.
- Scripts are numbered (`010_basic.lua`, …, `090_benchmark.lua`); new ones
  are picked up automatically.
- Run with `ctest --preset <preset>` (or execute `sdk_tests.exe` directly —
  exit code 0 = pass).
- CI (`.github/workflows/ci.yml`) builds and tests Linux/GCC, Windows/MinGW
  and Windows/MSVC; a CI step verifies the vendored Lua headers never drift
  from the compiled Lua.
- Releasing: push a `v*` tag; `.github/workflows/release.yml` builds both
  platforms, packages a ZIP (CPack) and attaches everything to a GitHub
  Release, using the configured `SDK_MODULE_NAME` for the artifact name.

---

## 8. Conventions for contributors

- **Code style** — `.clang-format` (4 spaces, Allman braces); comments and
  user-facing strings in English.
- **No ABI surprises** — exceptions stay inside the module; the six entry
  points are the only exported symbols; all text crossing the manager
  boundary uses the `char*` overloads.
- **New subsystems** get a namespace (`mta::<sub>`) and live in their own
  `source/sdk/<sub>/` folder with a `_test`-style coverage where practical.
- **New Lua-facing features** must work in the harness (mock manager) before
  they can be trusted on a real server.

---

## 9. Where to go next

- Add your first function: [TUTORIAL.md](TUTORIAL.md) step 4.
- Deep dives: threads & async, userdata objects, events, tables —
  [GUIDES.md](GUIDES.md).
- Full reference of every public type and macro: [API.md](API.md).