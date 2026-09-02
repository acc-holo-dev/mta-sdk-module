# Changelog

All notable changes to this project are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions
follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.0] — 2026-09-02

### Added

- `mta new function` / `mta new object` verified end to end: both templates
  compile out of the box in a fresh `mta init` project and are picked up by
  automatic source discovery.
- `mta doctor` status model: every check reports PASS/WARN/FAIL/SKIP, Lua
  ABI comparison is reported as SKIPPED (with a reason) when the vendored
  header directories are absent, and failures carry a fix hint.
- Benchmark suite for the remaining runtime areas (informational, printed
  rates, sanity-asserted): table snapshot roundtrips, callback
  hold/call/release bookkeeping, async task + timer scheduling, userdata
  creation/access — joining the function-call and argument-conversion
  benchmarks in the Lua suite.

### Changed

- Canonical repository layout finalized: the CMake implementation moved
  from `cmake/` to `config/cmake/` (core, lua, platform, install), so the
  root contains only project entry points and `config/module.toml` stays
  the single configuration file.
- Module version bumped to 2.1.0.
- README rewritten as a product introduction (quick start, first function,
  build/test/install, project structure, platforms); deep technical content
  stays in `other/documents/`.
- Development-planning artifacts removed from the repository; every kept
  source file, document and test now stands on its own without referencing
  the implementation process.

### Fixed

- CI: the real-server integration job invoked the Python CLI under
  `shell: msys2 {0}`, where no Python exists (exit 127). The steps now run
  under pwsh with the MinGW toolchain on PATH, matching the release
  workflow.
- CI / MSVC: `sdk_tests` died with fail-fast `0xc0000409` on recent MSVC
  runners: the Lua trampoline boundary (`protected_call` /
  `protected_call_named`) was `noexcept` while its catch blocks end in
  `luaL_error`, which longjmps to the Lua pcall protection point — a
  longjmp out of a `noexcept` function is a fail-fast on MSVC. The
  `noexcept` is removed from the boundary and the harness prints
  unbuffered progress so a crash point is always visible in CI logs.

## [2.0.0] — 2026-09-02

### Added

- Public facade `<mta/sdk.hpp>`: the single developer-facing include
  exporting the registration macros, the `mta::lua` value/stack layer,
  async, per-resource state, logging, events and userdata.
- Registration macros: `MTA_FUNCTION(name, function)` /
  `MTA_FUNCTION(name, "description", function)` (lambda style, registered
  under exactly the given name — no prefixes or namespaces),
  `MTA_LUA_FUNCTION` (body style) and `MTA_LUA_FUNC` (lambda one-liners).
- Typed binder: arguments are read and validated from the C++ signature;
  supported parameters include scalars, `std::string`, `std::optional<T>`,
  `mta::lua::rest_args`, a call `context`, `mta::lua::Table` snapshots and
  the native `mta::Resource`. Errors render as
  `bad argument #N to 'name' (expected X, got Y)`.
- Unified error model (`sdk/errors/errors.hpp`): `mta::errors::Error` with
  categories; internal failures render as `internal module error: ...` so
  framework bugs can never masquerade as scripter mistakes.
- Async task API: `mta::async::run(L, work, completion)` returns a
  cancellable `Task` handle; cooperative cancellation suppresses the
  completion; tasks are owned by the calling resource — a stop cancels
  queued tasks and drops completions of the finished generation before any
  Lua access. Worker count and queue limit come from `config/module.toml`.
- Timer API: `mta::timer::after` / `mta::timer::every` with `Timer` handles
  (`cancel()/valid()/id()`); timers are resource-aware and
  stale-generation timers can never fire into a restarted VM.
- Resource generations: every resource stop ends the VM generation;
  callbacks, task completions and timers record the generation they were
  created in and are structurally unable to operate across a restart of
  their resource — even when the fresh registry hands out the same
  `luaL_ref` index. Regression-tested in `072_restart.lua` and by the
  real-server restart choreography.
- Userdata objects: `MTA_OBJECT("counter", Counter)` declares a stable,
  compiler-independent type identity with a module-aware metatable name
  (`mta.<module>.<type>`); `MTA_METHOD` registers methods with
  self-excluded signature metadata.
- Native MTA types (safe subset): `mta::Resource` — live, ABI-backed
  resource lookup validated on every call; usable directly as a typed
  binder parameter/result. Element/player wrappers stay intentionally
  absent behind the frozen module ABI.
- Signature metadata: derived argument/return/variadic info per function
  plus object method metadata; exposed through the built-in
  `module_signature(name)` and rendered by `mta docs`.
- Borrowed state view: `mta::state` / `MTA_STATE` with typed readers
  (`check_number/opt_number`, `check_integer`, `check_boolean`,
  `check_string` + optional variants) mirroring the free `mta::lua::check_*`
  helpers; `mta::LuaView` is an alias.
- `source/library/base/handle_map.hpp` — the reusable id → handle map the
  async-task and timer samples use for Lua-facing handle bookkeeping.
- `config/module.toml` — the single project configuration file: module
  identity, build options (`cxx_standard`/`unity`/`lto`), `[async]` runtime
  knobs and `[features]` subsystem switches (disabled subsystems exclude
  their bundled samples from the build). CMake parses the TOML before
  `project()`; `project(VERSION)` follows `SDK_VERSION` in
  `source/sdk/version.hpp`, keeping the SDK version separate from the
  Module version. `-D` cache entries still work as overrides.
- `mta` project CLI (`other/tools/mta/`, stdlib Python): `init` (full
  project scaffold with identity rewrite), `new function`/`new object`
  (compile-ready skeletons, registered names verbatim), `build` (CMake
  presets, platform default), `test [all|unit|lua|integration]`, `docs`
  (markdown from registry metadata via the `sdk_docgen` target), `doctor`
  (TOML validity, SDK/ABI versions, Lua ABI byte-compare, toolchain,
  architecture, presets, source discovery, build output, git state, server
  environment), `package` (dist/ + sha256) and `server` (pinned-server
  management).
- Real-server integration harness (`other/server/mta_server.py`, stdlib
  Python): identity-pinned MTA server install into an isolated directory
  with the build identity recorded in `install.json`, direct installer
  payload extraction (7-Zip bootstrapped via an `msiexec /a` administrative
  image, nothing requires elevation), end-to-end runs that build the
  module, prepare a throwaway server tree, drive the integration resource
  through function/argument/return/table/callback/timer/async/userdata
  scenarios plus the restart choreography across resource generations, and
  stop the server gracefully with a pending long task that must never fire.
  Console output is captured into `other/server/logs/`.
- CI: Linux GCC + Clang, Windows MinGW-w64 + MSVC, `mta doctor`, CLI smoke,
  a Lua-ABI byte-identity check and a blocking real-server integration job.
- Release workflow: tag-triggered builds for linux-gcc, win-mingw and
  win-msvc; the win-mingw leg runs the real-server integration before
  packaging; releases attach exactly `<module>.dll` / `<module>.so` with a
  printed sha256.
- Documentation set in `other/documents/`: `example.md` (complete feature
  walkthrough with expected results), `api.md` (full public surface),
  `architecture.md` (layers, value model, lifetimes, threading, build and
  test system), `TUTORIAL.md`, `GUIDES.md` and `migration-v1-to-v2.md`.
- Benchmark scripts (informational rates, sanity-asserted) for function
  call throughput and argument conversion.

### Changed

- Binder error rendering uses the unified
  `bad argument #N to 'name' (expected X, got Y)` format; the running
  function's registered name is set by every registration trampoline.
- Timer scheduling internals: every timer shares a `TimerState`, so all
  drops (cancel, resource stop, stale generation, repeat limit) mark the
  state finished and public handles report `valid() == false`.
- `Scheduler::post_task` returns a `Task` handle and takes optional
  (resource, generation) ownership; `Callback` identity is now
  (resource, generation, ref) and releasing a stale callback never untracks
  the live callback that reused its index.
- The win-mingw preset enables unity builds and LTO like the other presets.

### Fixed

- Module-facing Lua headers now match the vendored MTA Lua 5.1 exactly: the
  stock `luaL_newstate` declaration hid the server's extra `mtasaowner`
  argument, so a module-created Lua state was built with an undefined owner
  pointer. The test harness passes `nullptr` explicitly and CI verifies the
  header sets cannot drift again.
- Export hygiene: the module exported the whole statically linked Lua API
  because `LUA_BUILD_AS_DLL` was defined while building the Lua sources.
  The module now exports only its six MTA entry points — a generated
  `.def` on Windows and a version script on Linux pin the exact list.
- Callback reference tracking: registry indices are only unique per VM, so
  equal ref numbers from different resources could collide. References are
  keyed by (resource, ref, generation).
- `Scheduler::pump()` could invalidate its timer iterator when a timer
  callback created or cancelled timers; due timers are dispatched from a
  snapshot.
- `sample_range`: the size guard could overflow for extreme int64 bounds;
  the comparison uses unsigned arithmetic.
- `Callback::call` grows the Lua stack explicitly before pushing arguments.
- Comment encoding: every text file is clean ASCII/UTF-8 on any toolchain,
  including MSVC builds without `/utf-8`.

## [1.1.0] — 2026-08-30

### Fixed

- Callback reference tracking: registry indices are only unique per VM, so
  equal ref numbers from different resources could collide in the global
  tracking map. References are now keyed by (resource, ref).
- `Scheduler::pump()` could invalidate its timer iterator: a timer callback
  that created or cancelled timers while the loop ran could trigger
  undefined behavior. Due timers are dispatched from a snapshot.
- sample_range: the size guard could overflow for extreme int64 bounds; the
  comparison now uses unsigned arithmetic.
- Callback::call: the Lua stack is grown explicitly before pushing
  arguments, avoiding a hard stack overflow for large argument lists.

### Changed

- All code comments, user-facing messages and documentation are in English.
- Module version bumped to 1.1.0.

### Added

- CI job for MSVC (win-msvc preset) on GitHub Actions.
- Release workflow: pushing a v* tag builds Windows and Linux artifacts and
  attaches them to a GitHub Release.
- Install rules and CPack ZIP packaging.

## [1.0.0] — 2026-08-30

### Added

- Solid MTA:SA Lua-module base: the official SDK contract
  (ILuaModuleManager10.h), exception boundaries, self-registration.
- Typed binder: MTA_LUA_FUNCTION (body style) and MTA_LUA_FUNC (lambda
  style), argument reading via mta::lua::args<...>.
- Table support (Argument/Table), std::optional, C++ defaults, rest_args,
  context.
- Async scheduler (workers + DoPulse), timers, stable Lua callbacks.
- Per-resource state with automatic cleanup.
- Logging.
- Embedded-Lua test harness with a mock manager.
- Builds for Windows (MinGW/MSVC) and Linux (GCC) via CMake presets.
- CI, .clang-format, .editorconfig, sanitizer option.