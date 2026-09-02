# Changelog

All notable changes to this project are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions
follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Documentation rewrite (plan PHASE 12) in `other/documents/`: `example.md`
  (a complete feature walkthrough — sync function, async task, timer,
  object, per-resource state, events, resources — mapped to the bundled
  samples and tests), `api.md` (the full V2 public surface: registration
  macros, typed arguments/results, errors, snapshots, stack helpers,
  logging, tasks, callbacks, timers, per-resource state, objects, events,
  native types, module identity, `config/module.toml` and the `mta` CLI),
  `architecture.md` (layers, key flows incl. restart generations, threading
  rules, configuration, the three test layers incl. the real-server
  harness, conventions) and `migration-v1-to-v2.md` (layout/config/API
  changes, semantics to audit, a step-by-step checklist). The legacy
  `API.md`/`ARCHITECTURE.md` were superseded by the lowercase set (git
  history keeps the old long-form); `GUIDES.md`/`TUTORIAL.md` stay as
  supplements with pointers to the new set.
- Server integration harness (plan §30-§33, PHASE 11) in `other/server/mta_server.py`
  (stdlib Python): identity-pinned MTA server install (`install`/`update`/
  `version`/`start`/`stop`) into an isolated directory under `other/server/`
  — the installer payload is extracted directly (a registered MTA:SA
  installation on the machine would otherwise silently redirect the
  install), with the pinned build identity and sha256 recorded in
  `install.json`; the extraction tool (7-Zip) is bootstrapped locally via an
  `msiexec /a` administrative image, so nothing requires elevation.
- `mta server test` / `mta test integration`: end-to-end run on the REAL
  pinned x64 server — builds the module, prepares a throwaway server tree
  with unique ports per run, loads the module (`<module src=...>` from
  `x64/modules`), and drives the `sdkintegration` resource through the §32
  scenarios (module load, registration, return values, argument validation,
  userdata + validation, MTA timers, async tasks, callbacks) and the §33
  restart scenario (the harness restarts the resource mid-run; the
  generation-1 stale task must never deliver into generation 2), then stops
  the server gracefully with a 60-second task still queued (§32 shutdown)
  and asserts it never fires.
- Console driving: the Windows server requires a real console for output and
  input, so the harness shares its console (CONIN$/CONOUT$ handles), types
  `restart`/`exit` as key events, consumes the console scrollback as the run
  log and keeps it under `other/server/logs/`.
- `mta` project CLI (plan §24-§29, PHASE 10) in `other/tools/mta/`
  (stdlib Python + launchers): `init` (full project scaffold, non-destructive
  guards, identity rewritten from the name), `new function`/`new object`
  (compile-ready skeletons, registered names verbatim — `crypto.sha256`
  stays `crypto.sha256`), `build` (CMake presets, platform default), `test
  [all|unit|lua|integration]` (separated suites), `docs` (markdown from the
  PHASE 4 registry metadata via a new `sdk_docgen` target), `doctor` (real
  environment checks: TOML validity, toolchain/cmake/ninja/compiler
  versions, C++ standard, presets, source discovery, build output, git
  state, server env, Lua headers byte-compared against the bundled Lua),
  `package` (dist/ + sha256), `server` (delegates to other/server, lands
  in PHASE 11).
- Native types, safe subset (plan §17/PHASE 9): `mta::Resource` — live,
  ABI-backed resource lookup (`find(name)`, `current(L)`, `vm()` re-looked
  up on every call so a stopped resource can never hand out a dangling
  lua_State, `alive()`); samples `sample_resource_name`/`sample_resource_find`
  and tests. Player/Vehicle/Element wrappers are intentionally absent: the
  frozen module ABI exposes no element API, so they cannot be represented
  safely (documented in the header).
- Userdata V2 (plan §16): `MTA_OBJECT("counter", Counter)` declares a
  stable, deterministic, compiler-independent type identity; the metatable
  name is module-aware (`mta.<module>.<type>`) so two modules cannot
  collide, argument errors name the declared type, and a type without an
  explicit name falls back to a compiler-dependent identity with a warning.
- `[features]` consumption (plan §4): subsystem switches compile in as
  `SDK_FEATURE_*` definitions and exclude the bundled sample functions of a
  disabled subsystem from the module and the test binary; explicit cache
  overrides win (`-DSDK_FEATURE_ASYNC=OFF`, `-DSDK_ASYNC_WORKERS=4`,
  `-DSDK_ASYNC_QUEUE=1024`).
- Timer V2 (plan §15): `mta::timer::after(L, delay, fn)` (one-shot) and
  `mta::timer::every(L, delay, fn)` (repeating) return a `Timer` handle
  (`cancel()/valid()/id()`); timers are resource-aware — a resource stop
  invalidates every owned timer and stale-generation timers can never fire
  into a restarted VM.
- Samples `sample_after`/`sample_every`/`sample_after_cancel`/
  `sample_timer_valid` (per-resource timer registry) and Lua test
  `038_timer.lua`.
- Async V2 task API (plan §13/§14): `mta::async::run(L, work, completion)`
  returns a cancellable `Task` handle (`cancel()/done()/valid()/id()`);
  cooperative cancellation suppresses the completion; the task is owned by
  the calling resource — a stop cancels queued tasks and drops completions
  of the finished generation before any Lua access.
- Queue limits and worker count from `config/module.toml` `[async]`
  (`queue = 4096`, `workers = "auto"` compiled into the module): a full
  queue rejects the task with an invalid handle instead of blocking.
- Samples `sample_task_run`/`sample_task_cancel` (per-resource task
  registry) and Lua test `035_task.lua`; C++ harness regressions for the
  handle semantics, ownership and the queue limit.
- ResourceContext generations (plan §11/§33, P0): every resource stop ends
  the VM generation (`Hub::generation`/`bump_generation`). Callbacks, async
  task completions and timers record the generation they were created in and
  are structurally unable to operate across a restart of their resource --
  even when the fresh registry hands out the same `luaL_ref` index.
- Harness restart simulation (plan §33): `test_resource_restart()` swaps in
  a REAL fresh Lua VM (fresh registry) under a new generation;
  `test_fresh_vm_dostring`/`test_fresh_vm_get` drive and observe it; a new
  regression script `072_restart.lua` reproduces the audited §5.1
  generation-confusion bug without the fix and passes with it (verified).
- `mta::log::debug(...)` overload for messages outside a VM context.

### Changed

- Timer scheduling internals: `Scheduler::post_timer_impl` creates shared
  `mta::timer::TimerState` for every timer; all scheduler-side drops
  (cancel, resource stop, stale generation, repeat limit) mark the state
  finished so public handles report `valid() == false`.
- `Scheduler::post_task` returns a `Task` handle and takes optional
  (resource, generation) ownership; `sample_async_add` migrated to
  `mta::async::run` and reports queue-full rejections.
- `Callback` identity is now (resource, generation, ref): stale callbacks
  are dropped with a debug log, and releasing a stale callback never
  untracks the live callback that reused its index.
- Timers store the owner's generation; the pump drops timers of a stale
  generation; `handle_resource_stopped` still cancels them immediately.
- `release_all_callbacks` unrefs only live references of the current
  generation.

### Added

- Unified error model `sdk/errors/errors.hpp` (plan §19): `mta::errors::Error`
  with categories (Generic, InvalidArgument, InvalidType, MissingArgument,
  ResourceStopped, InvalidCallback, InvalidObject, AsyncCancelled,
  InternalError). Internal failures render as `internal module error: ...` —
  framework bugs can never masquerade as scripter mistakes.
- Signature metadata (plan §9/§21): the registry stores derived
  argument/return/variadic info per function (`Spec::signature`, derived for
  lambda-style, explicitly not derived for body-style); new built-in
  `module_signature(name)` exposes it; `module_functions` unchanged.
- Argument errors carry the function name and position (plan §7):
  `bad argument #2 to 'sample_minmax' (expected number, got no value)` /
  `bad argument #1 to 'sample_greet' (expected string, got table)` /
  `bad argument #1 to 'sample_range' (expected integer, got string)`.
- New sample `sample_hello_len` (tuple-returning lambda; exercises metadata).
- `other/tests/lua/scripts/045_errors.lua` — the plan §7 error matrix and
  `module_signature` metadata checks.

### Changed

- Binder error rendering moved to the plan §7 format; the running function's
  registered name is set by every registration trampoline (body style, lambda
  style, userdata methods) and used in messages. Old texts
  (`argument #N must be a ...`) replaced everywhere, including all pinned Lua
  test assertions.
- The protected_call boundary re-renders unexpected std exceptions as
  internal errors; deliberate `raise_error` messages stay verbatim.

### Added

- Public facade `<mta/sdk.hpp>` (plan §42/§43): the single developer-facing
  include exporting the registration macros, `mta::lua` values/stack helpers,
  async (Callback/Scheduler), per-resource state, logging, events and
  userdata. All sample functions now include only the facade.
- `MTA_FUNCTION(name, function)` / `MTA_FUNCTION(name, "description",
  function)` — the plan §6 registration spelling; the function is registered
  under exactly the given name (no prefixes/namespaces, plan §2).

### Added

- `config/module.toml` — the single project configuration file (plan §4/§5):
  module identity (`[module]` name/title/author/version), build options
  (`[build]` cxx_standard/unity/lto) and forward-looking `[async]` /
  `[features]` sections. CMake parses the TOML directly before `project()`
  (`cmake/core/module-config.cmake`), so even `project(VERSION)` comes from
  the TOML. Explicit `-D` cache entries keep working as overrides.
- CTest tests for the TOML reader (`module_config_parse`,
  `module_config_rejects_garbage`).

### Changed

- Module identity is no longer defined by CMake cache defaults: the cache
  variables `SDK_MODULE_NAME/TITLE/AUTHOR` are now optional overrides of
  `config/module.toml` values. A previously configured build directory
  caches its old values — run `cmake -U "SDK_MODULE_*" ...` once (or
  configure a fresh directory) to re-read the TOML.
- `SDK_UNITY` / `SDK_LTO` defaults come from `config/module.toml` `[build]`
  (removed from the presets so the TOML stays the single source).
- Module version bumped to 2.0.0 (V2 line).

### Added

- Configurable module identity: `SDK_MODULE_NAME` (default `base` →
  `base.dll` / `base.so`), `SDK_MODULE_TITLE` and `SDK_MODULE_AUTHOR` are
  plain CMake cache variables — renaming the module no longer requires
  editing C++ sources. The Windows export table follows the name via a
  generated module.def (template: src/module/module.def.in).
- docs/ARCHITECTURE.md: a full architecture document (layers, flows,
  threading rules, configuration) for understanding the system at a glance.

### Changed

- The module export files were renamed to be name-neutral
  (src/module/module.def.in, src/module/module.ver).
- The CMake project was renamed from ml_base to mta_sdk_module (the output
  binary name is controlled by SDK_MODULE_NAME, not the project name).

### Fixed

- Module-facing Lua headers (vendor/mta-sdk/lua) now match the vendored MTA
  Lua 5.1 exactly: the stock declarations of `luaL_newstate`/`luaL_newstate`
  hid the server's extra `mtasaowner` argument, so a module-created Lua state
  was built with an undefined owner pointer. The test harness now passes
  `nullptr` explicitly, and CI verifies the header sets cannot drift again.
- Export hygiene: ml_base exported the whole statically linked Lua API
  (157 symbols) because LUA_BUILD_AS_DLL was defined while building the Lua
  sources even though Lua is linked statically into the module. The module
  now exports only its six MTA entry points — ml_base.def on Windows and a
  version script on Linux pin the exact list.
- Comment encoding: non-ASCII dashes in sources, tests and .clang-format are
  gone, so every text file is clean ASCII/UTF-8 on any toolchain, including
  MSVC builds without /utf-8.

### Changed

- The module version is derived from the single `project()` version in
  CMakeLists.txt instead of being hard-coded in src/module/module.cpp.
- The win-mingw preset now enables unity builds and LTO like the other
  presets (verified against the same MinGW-w64 toolchain).

### Added

- CI check that vendor/mta-sdk/lua/*.h stay byte-identical to the compiled
  vendor/lua/src/*.h headers (guards the module/server Lua ABI).

## [1.1.0] — 2026-08-30

### Fixed

- Callback reference tracking: registry indices are only unique per VM, so
  equal ref numbers from different resources could collide in the global
  tracking map. References are now keyed by (resource, ref), preventing
  cross-resource luaL_unref and stale-entry overlap.
- Scheduler::pump() could invalidate its timer iterator: a timer callback that
  created (post_timer) or cancelled (cancel_timer) timers while the loop ran
  could trigger undefined behavior. Due timers are now dispatched from a
  snapshot, so callbacks may safely mutate the timer list.
- sample_range: the size guard could overflow for extreme int64 bounds; the
  comparison now uses unsigned arithmetic.
- Callback::call: the Lua stack is now grown explicitly before pushing
  arguments, avoiding a hard stack overflow for large argument lists.

### Changed

- All code comments, user-facing messages and documentation are now in
  English (README, API.md, GUIDES.md, TUTORIAL.md, CHANGELOG).
- Module version bumped to 1.1.0.

### Added

- CI job for MSVC (win-msvc preset) on GitHub Actions.
- Release workflow: pushing a v* tag builds Windows and Linux artifacts
  (ml_base.dll / ml_base.so) and attaches them to a GitHub Release.
- Install rules and CPack ZIP packaging (cmake/install.cmake).

## [1.0.0] — 2026-08-30

### Added

- Solid MTA:SA Lua-module base: the official SDK contract
  (ILuaModuleManager10.h), exception boundaries, self-registration.
- Typed binder: MTA_LUA_FUNCTION (body style) and MTA_LUA_FUNC (lambda
  style), argument reading via mta::lua::args<...>.
- Table support (Argument/Table), std::optional, C++ defaults, rest_args,
  context.
- Async scheduler (workers + DoPulse), timers, stable Lua callbacks
  (mta::async::Callback).
- Per-resource state (mta::resources::Store) with automatic cleanup.
- Logging (mta::log).
- Embedded-Lua test harness (sdk_tests) with a mock manager.
- Builds for Windows (MinGW/MSVC) and Linux (GCC) via CMake presets.
- Documentation: README.md, docs/API.md, docs/GUIDES.md.
- CI (GitHub Actions), .clang-format, .editorconfig, sanitizer option.