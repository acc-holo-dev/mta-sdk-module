# V2 Phase Reports

Per-phase implementation reports (plan PROMT.md §52). One entry per phase,
newest last.

---

## PHASE 0 — Audit

- **PHASE**: 0 — audit of the inherited system (no architecture changes).
- **CHANGED**: nothing in the code; restored the accidentally deleted
  `README.md` from HEAD (it was deleted in the working tree only).
- **ADDED**: `other/documents/v2-audit.md` — current architecture, Lua-ABI
  safety inventory, public-API KEEP/CHANGE table, test inventory, P0 findings
  (generation confusion in callback tracking, name-keyed resource state,
  typeid-based userdata identity, missing task ownership) and the
  migration decisions for every later phase.
- **REMOVED**: nothing.
- **TESTS**: baseline verified before any change: `cmake --build --preset
  win-mingw` green, `ctest --preset win-mingw` green (sdk_tests, 10 Lua
  scripts, ~3.7 s). Local toolchain: build/toolchain/mingw (CMake 4.4.2,
  GCC 16.2.0 ucrt-posix-seh, Ninja 1.13.2) — git-ignored.
- **RISKS**: documented in the audit (§5): the (resource, ref) identity
  collision across generations is a live bug reproducible in the harness;
  userdata uses `typeid(T).name()`; error-message wording is pinned by tests
  and will change in PHASE 4; release currently ships a ZIP (violates the
  binaries-only policy of plan §36).
- **NEXT**: PHASE 1 restructure.

## PHASE 1 — Repository restructure

- **PHASE**: 1 — mechanical move to the V2 target layout.
- **CHANGED**:
  - `src/` → `source/`: `functions/` (developer code), `library/base/`
    (created), `sdk/{abi,lua,bind,registry,runtime,resources,objects,
    events,logging}` (framework internals; bind/userdata/events/resources/
    logging hoisted out of the old lua/ and runtime/ dirs).
  - `vendor/` → `other/third_party/{mta-sdk,lua}`.
  - `tests/` → `other/tests/lua/` (harness + scripts); `other/tests/unit/`,
    `other/tests/integration/` created with READMEs.
  - `docs/` → `other/documents/`.
  - All `#include "..."` paths rewritten to the `sdk/...` scheme (38 files);
    include root is `source/` (so `<mta/sdk.hpp>` can be added later at
    `source/mta/`).
  - CMake: glob `source/**/*.cpp`; include dirs `source` +
    `other/third_party/mta-sdk`; def/ver templates at `source/sdk/abi/`;
    harness paths; stale `LUA_SRC_DIR` cache entries self-heal now.
  - CI header-drift check + README/CONTRIBUTING/ARCHITECTURE path references
    updated; `.gitignore` extended for `other/server` artifacts.
- **ADDED**: README placeholders for `source/library/base`, `other/server`,
  `other/tests/unit`, `other/tests/integration`, `other/tools`.
- **REMOVED**: no files (pure moves + edits; history preserved via renames).
- **TESTS**: build green and `sdk_tests` green both in the incremental
  (`build/win-mingw`) and a **fresh configure** (`build/check-v2`), proving
  no stale-cache dependency.
- **RISKS**: none new; behavior untouched (identical sources, identical
  binary name `base.dll`). The `LUA_SRC_DIR` cache self-heal is the only
  build-behavior change, and only for stale caches.
- **NEXT**: PHASE 2 — `config/module.toml` as the single configuration
  source, consumed by CMake.

## PHASE 2 — Configuration

- **PHASE**: 2 — single configuration source (`config/module.toml`), parser
  wired into CMake.
- **CHANGED**:
  - `CMakeLists.txt`: `read_module_toml()` runs before `project()`;
    `project(VERSION)` now comes from `[module] version`; `CMAKE_CXX_STANDARD`
    defaults to `[build] cxx_standard`; `SDK_UNITY`/`SDK_LTO` default to
    `[build] unity`/`[build] lto`; identity variables `SDK_MODULE_NAME/
    TITLE/AUTHOR` are TOML values with `-D` cache entries as explicit
    overrides (single source of truth, plan §4/§5).
  - `CMakePresets.json`: removed `SDK_LTO`/`SDK_UNITY` from presets so the
    TOML governs them; compiler settings unchanged.
  - Version bumped to 2.0.0 (module float "2.0"); author "Developer".
- **ADDED**:
  - `config/module.toml` — `[module]`, `[build]`, `[async]`, `[features]`
    sections exactly per plan §4.
  - `cmake/core/module-config.cmake` — pragmatic TOML-subset reader
    (sections, quoted strings, booleans, ints, comments incl. `;` and `#`
    inside quotes) + `module_toml_bool()`.
  - `other/tests/unit/fixtures/{module,garbage}.toml`,
    `module_config_parse.cmake` (12 value assertions + bool normalization),
    `module_config_rejects_garbage.cmake` (ctest WILL_FAIL).
  - Lua test assertions pinning the TOML identity end-to-end
    (`sample_version`: title/author/version 2.0).
- **REMOVED**: CMake cache defaults for identity (duplicated source of
  truth).
- **TESTS**: `ctest --preset win-mingw`: 3/3 passed (sdk_tests incl. new
  identity assertions, module_config_parse, module_config_rejects_garbage);
  also verified in a fresh configure (`build/check-v2`) and after purging
  stale identity cache entries in the incremental dir (`cmake -U
  "SDK_MODULE_*"`).
- **RISKS**: existing build directories configured before this change carry
  cached identity values (documented in CHANGELOG; self-heal is impossible
  because CMake cannot distinguish user-set `-D` from old defaults).
  `[async]`/`[features]` are parsed and tested but consumed only when their
  subsystems land (PHASE 6 async workers/queue, PHASE 8 feature gating) —
  no dead compile-time defines are emitted meanwhile.
- **NEXT**: PHASE 3 — public facade `<mta/sdk.hpp>` + `MTA_FUNCTION`.

## PHASE 3 — SDK public facade

- **PHASE**: 3 — umbrella header and developer-facing registration spelling.
- **CHANGED**:
  - All 16 sample functions under `source/functions/**` now include ONLY
    `<mta/sdk.hpp>` (the developer contract, plan §42); internal
    `sdk/...` includes removed from developer code. `version.cpp` keeps its
    `ILuaModuleManager10.h` include (it calls manager methods directly).
  - README: facade shown as the primary include; registration examples use
    `MTA_FUNCTION`; install example reflects the 2.0 identity.
- **ADDED**:
  - `source/mta/sdk.hpp` — umbrella header grouping registration, Lua
    values/stack helpers, runtime services (callback/scheduler/resources/
    logging/events) and userdata. Internal layers untouched and still
    reachable (no behavior change).
  - `MTA_FUNCTION(name, function)` and `MTA_FUNCTION(name, "description",
    function)` in `sdk/registry/registry.hpp` — plan §6 spelling, built on
    the existing `MTA_LUA_FUNC_IMPL` machinery (arg-count selection macro).
    Registered names are verbatim (plan §2 rule already held; now pinned by
    tests).
  - `source/functions/basics/hello.cpp` — facade-style samples
    (`sample_hello`, `sample_hello_desc`).
  - `other/tests/lua/scripts/015_facade.lua` — exact-name registration,
    description handling, typed errors, missing-argument errors.
- **REMOVED**: nothing (MTA_LUA_FUNCTION / MTA_LUA_FUNC stay, KEEP per
  audit §3).
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (sdk_tests now 92
  assertions incl. the facade script).
- **RISKS**: the macro arg-count selection requires descriptions without
  bare commas (wrap in parens) — documented at the macro. A test initially
  expected a type error for `sample_hello(42)`; numbers convert to strings
  by design (`luaL_checkstring` semantics, documented) — the test now pins
  that documented behavior with a table argument instead.
- **NEXT**: PHASE 4 — binder V2 (plan §7 error format, signature metadata,
  unified error model) with same-phase test updates.

## PHASE 4 — Binder V2 (error format, error model, signature metadata)

- **PHASE**: 4 — argument validation per plan §7, unified error model §19,
  registry signature metadata §9/§21.
- **CHANGED**:
  - `sdk/lua/stack.hpp`: all check_*/opt_* render the plan §7 format
    `bad argument #N to '<name>' (expected <T>, got <G>)`; missing args give
    `(expected <T>, got no value)`; whole-number/range violations render
    `(value out of range)` / `(<value> is not a whole number)`. NIL at a
    position reports `got nil` (distinguishes explicit nil from absence).
  - `sdk/lua/protect.hpp`: `raise_error` now throws the unified
    `mta::errors::Error` (Generic); the boundary renders InternalError and
    foreign std exceptions as `internal module error: ...`; added
    `protected_call_named` + thread-local `current_function_name` context
    (set by every registration trampoline; purely diagnostic).
  - `sdk/bind/bind.hpp`: Table/string_view/Callback/range errors migrated;
    Callback params now report `(expected function, got ...)`; the
    error_probe walks the pull order so missing/invalid positions get exact
    messages; `holder::entry` delegates to the single protected_call boundary
    (duplicate catch chains removed).
  - `sdk/objects/userdata.hpp`: method trampolines name the running method;
    `check()` reports `(expected module object, got ...)` with the
    invalid-object category.
  - Lua tests 010/015/020/040/050 updated in the same phase (plan rule: no
    stale pinned behavior).
- **ADDED**:
  - `sdk/errors/errors.hpp` — categories per plan §19 with
    `category_name()` for logs.
  - Signature metadata (§9/§21): `mta::lua::ArgumentInfo`/`Signature`,
    `Spec::signature` (+ reserved `category`, `flags`); derived
    automatically for lambda-style; body-style explicitly `derived == false`
    (documented underivable). New built-in `module_signature(name)`.
  - Sample `sample_hello_len` (tuple-returning lambda) to pin metadata.
  - `other/tests/lua/scripts/045_errors.lua` — §7 matrix (type mismatch,
    missing at position, integer, optional, callback) + metadata checks.
- **REMOVED**: nothing public; `mta::lua::raise` semantics preserved (now
  Generic category).
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (sdk_tests 130 assertions).
- **RISKS**: the name context is a thread-local set by trampolines (stays set
  after a call — diagnostic only, overwritten by the next call); the
  `bad argument count` fallback for exhausted probes is unreachable in
  practice but kept well-formed; message text is now a contract — further
  changes need a versioned migration note (PHASE 12 docs).
- **NEXT**: PHASE 5 (P0) — ResourceContext/generation: fix the callback
  generation-confusion bug (audit §5.1), harness fresh-VM simulation, §33
  restart regression tests.

## PHASE 5 — ResourceContext/generation (P0)

- **PHASE**: 5 — VM-generation safety for callbacks, async tasks and timers
  (plan §11/§12/§14/§33; closes the audit §5.1 P0 finding).
- **CHANGED**:
  - `sdk/resources/resources.{hpp,cpp}`: `Hub` now owns the ResourceContext
    identity — `generation(resource)` (starts at 1, +1 per completed stop)
    and `bump_generation(resource)`, called at the top of
    `notify_resource_stopped` so every sink sees the finished generation.
  - `sdk/runtime/callback.{hpp,cpp}`: `Callback` records the resource's
    generation at `from_stack` time. `TrackedRef` carries the generation;
    `ref_is_dead` treats a foreign-generation entry as dead; `untrack_ref`
    only touches the callback's OWN generation (a stale callback releasing
    itself can no longer untrack the live callback that reused its index);
    `call()` re-checks the Hub generation after the VM lookup (second line
    of defense) and drops stale callbacks with a debug log;
    `release_all_callbacks` unrefs only live current-generation refs.
  - `sdk/runtime/scheduler.cpp`: timers store the owner's generation;
    `pump()` drops (and logs) timers of a stale generation;
    `handle_resource_stopped` still cancels all resource timers immediately.
  - `sdk/logging/logging.hpp`: `debug(...)` overload without a VM context.
- **ADDED**:
  - Harness restart simulation (plan §33): `test_resource_restart()` runs
    the stop hooks and swaps in a REAL fresh Lua VM (fresh registry, fresh
    luaL_ref space) under the new generation;
    `test_fresh_vm_dostring`/`test_fresh_vm_get` drive/observe it;
    `test_resource_restore()` reattaches the script VM afterwards (harness
    bookkeeping only). All VMs are closed at harness shutdown.
  - `other/tests/lua/scripts/072_restart.lua` — the §33 regression: an async
    completion and timers from generation N-1 face a generation-N callback
    that holds the SAME luaL_ref index 1 in the fresh registry; the stale
    completion must never fire the new function, the new completions must
    arrive, and multiple fresh-generation timers must fire.
  - CHANGELOG entries for the phase.
- **REMOVED**: nothing.
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (sdk_tests 125+
  assertions, including 072). Verified the regression catches the bug: with
  the generation checks temporarily disabled the 072 assertion
  "stale generation-2 completion never fired in the fresh VM" fails
  (audited §5.1 behavior reproduced), and passes again with the fix
  restored.
- **RISKS**: generation bookkeeping lives in `Hub` (main-thread only, like
  the lifecycle hooks) — no locking added because every writer runs on the
  main thread; the harness `test_resource_restore` helper is simulation
  plumbing, documented as such.
- **NEXT**: PHASE 6 — Async V2: task handle (cancel/done/valid), queue
  limits consuming `[async] queue`, worker count from `[async] workers`,
  resource ownership and safe shutdown.

## PHASE 6 — Async V2 (task handle, queue limits, ownership)

- **PHASE**: 6 — developer-facing task API over the internal Scheduler
  (plan §13), resource ownership §14, queue limits and worker config §4.
- **CHANGED**:
  - `sdk/runtime/scheduler.{hpp,cpp}` reworked: `post_task` returns a
    `Task` handle, accepts optional (resource, generation) ownership; the
    worker loop honors cooperative cancellation (a cancelled queued task is
    skipped without running; a cancelled running one never delivers);
    completions carry their (resource, generation) and `pump()` drops them
    when the VM is gone or the generation ended (no Lua access, §14);
    `stop()` cancels every queued task so handles report `done()`;
    `configure(queue_limit)` allows runtime adjustment (used by tests);
    worker count from `[async] workers` ("auto" = hardware probe clamped
    1..8), queue limit from `[async] queue` — both compiled in via
    `SDK_ASYNC_WORKERS_AUTO/_N` and `SDK_ASYNC_QUEUE_N`.
  - `sample_async_add` migrated to `mta::async::run` and returns whether the
    task was accepted.
  - `CMakeLists.txt`: `[async]` values passed as compile definitions.
  - API.md: Scheduler/Task sections rewritten.
- **ADDED**:
  - `sdk/runtime/task.hpp` — `TaskState`/`Task` shared handle (cancel/done/
    valid/id) with documented semantics.
  - `mta::async::run(lua_State*, work, completion)` — the developer API:
    resource attribution + cancellable handle.
  - `source/functions/async/task_demo.cpp` — `sample_task_run`
    (per-resource task registry via `resources::Store`) and
    `sample_task_cancel`.
  - `other/tests/lua/scripts/035_task.lua` — cancel-before-delivery,
    delivery, unknown-id cancel, ownership across a resource stop.
  - Harness C++ regressions (`run_async_regressions`): valid/done flow,
    cancellation suppression, ownership drop, queue limit (20 slow posts,
    limit 2 -> >= 10 rejected deterministically for any worker count <= 8).
- **REMOVED**: nothing public; `post_task`'s void return replaced by Task
  (old fire-and-forget callers keep working).
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (143 assertions + C++
  regressions; suite now ~7.5s due to timing margins).
- **RISKS**: cancellation is cooperative — running work cannot be
  interrupted, only its delivery suppressed (documented on the handle and in
  API.md); the queue limit counts only QUEUED tasks (running ones are
  already being paid for); worker count is fixed at start (module load).
- **NEXT**: PHASE 7 — Timer V2: `mta::timer::after()/every()` with
  `.cancel()/.valid()` handles (plan §15).

## PHASE 7 — Timer V2 (public timer API with handles)

- **PHASE**: 7 — developer-facing timer API over the internal scheduler
  timers (plan §15): after/every, cancel/valid handles, resource-aware
  invalidation, no stale execution after restart.
- **CHANGED**:
  - `sdk/runtime/scheduler.{hpp,cpp}`: every scheduled timer now owns a
    shared `mta::timer::TimerState`; new `post_timer_handle` returns the
    public handle (raw-id `post_timer` delegates to `post_timer_impl`);
    all scheduler-side drops mark the state finished — explicit cancel,
    resource stop (`handle_resource_stopped`), stale-generation drop and
    repeat-limit expiry in `pump()`, so handles report invalid without
    guessing.
  - `source/mta/sdk.hpp`: facade exports `mta::timer`.
  - API.md: `mta::timer` section added.
- **ADDED**:
  - `sdk/runtime/timer.{hpp,cpp}` — `TimerState`/`Timer` handle and
    `mta::timer::after(L, delay, fn)` / `every(L, delay, fn)`; resource
    attribution via the calling resource; negative delays raise errors.
  - `source/functions/async/timer_demo.cpp` — `sample_after`,
    `sample_every`, `sample_after_cancel`, `sample_timer_valid` (per-
    resource registry via `resources::Store`).
  - `other/tests/lua/scripts/038_timer.lua` — one-shot fires exactly once
    then invalid; cancel-before-fire (double cancel false, no delivery);
    every() repeats until cancelled and stops exactly at cancel; resource
    stop invalidates owned timers and nothing fires afterwards.
- **REMOVED**: nothing; the internal raw-id `post_timer`/`cancel_timer`
  API stays for existing callers.
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (160 assertions).
- **RISKS**: timer handles are main-thread-only state (documented) —
  cancelling from a worker is impossible by design; the repeat-limit expiry
  marks the handle finished only after the final callback returns (a
  callback cancelling itself sees valid() true — harmless and documented
  behaviour).
- **NEXT**: PHASE 8 — Userdata V2: stable explicit userdata type ids
  (plan §16), `[features]` consumption.

## PHASE 8 — Userdata V2 (stable type identity, [features] consumption)

- **PHASE**: 8 — stable explicit userdata type identifiers (plan §16) and
  the `[features]` subsystem switches of config/module.toml (plan §4).
- **CHANGED**:
  - `sdk/objects/userdata.hpp`: the metatable identity is no longer
    `typeid(T).name()`. `Registry<T>::set_type_name()` sets an explicit
    id; `identity()` composes `mta.<module>.<type>` (module-aware — two
    SDK modules in one server cannot collide on the same type name) and
    falls back to a compiler-dependent name with a warning when no explicit
    id was declared. `check()` argument errors name the declared type
    ("expected counter, got number") instead of a generic "module object".
  - `source/functions/objects/counter.cpp`: the sample type is declared
    with `MTA_OBJECT("counter", Counter)`.
  - `CMakeLists.txt`: `[async]` and `[features]` values now honor explicit
    cache overrides (`-DSDK_ASYNC_WORKERS=4`, `-DSDK_FEATURE_ASYNC=OFF`,
    mirroring the SDK_UNITY/SDK_LTO pattern — a plain TOML normal variable
    would otherwise shadow -D entries).
- **ADDED**:
  - `MTA_OBJECT(Name, Type)` macro (namespace-scope declaration returning
    true, usable as a static initializer; the first id wins, conflicting
    re-declarations log an error and are ignored).
  - `[features]` consumption: switches compile in as
    `SDK_FEATURE_ASYNC/_USERDATA/_EVENTS/_OBJECTS` and exclude the bundled
    sample functions of a disabled subsystem from the module AND the test
    binary (`functions/async`, `functions/objects`, `functions/events`).
  - `060_features.lua`: type validation names the declared type.
- **REMOVED**: nothing public; `typeid(T).name()` remains only as a
  documented, warning-logged fallback for types without MTA_OBJECT.
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (162 assertions).
  Feature gating verified by configure with `-DSDK_FEATURE_EVENTS=OFF
  -DSDK_FEATURE_OBJECTS=OFF`: the unity file lost trigger_demo/counter and
  the defines flipped to 0; restored afterwards.
- **RISKS**: the metatable identity is computed once per process from the
  module name — modules built with a name change must not mix object types
  across builds (acceptable: the module name comes from module.toml, the
  same source of truth as the DLL name); MTA_OBJECT must appear in exactly
  one TU per type.
- **NEXT**: PHASE 9 — Native MTA types (conservative: the ILuaModuleManager10
  ABI exposes no element API; only VM lookup by resource name).

## PHASE 9 — Native MTA types (safe subset)

- **PHASE**: 9 — safe native-type wrappers (plan §17) "where possible": the
  frozen ILuaModuleManager10 ABI exposes NO element/player/vehicle API, so
  only the piece that can be represented safely ships: resource lookup.
- **CHANGED**:
  - `source/mta/sdk.hpp`: facade exports `mta::Resource` and documents the
    native-types scope.
- **ADDED**:
  - `sdk/native/resource.{hpp,cpp}` — `mta::Resource` with
    `find(name)`/`current(L)` (nothing when unknown/absent) and `vm()`:
    a LIVE `GetResourceFromName` lookup on every call, never a cached
    lua_State (the server can destroy VMs at any time; plan §14). The
    header documents why `mta::Player`/`mta::Vehicle`/`mta::Element` are
    NOT provided: they would require calling engine functions by name in a
    foreign VM with no type guarantee across server versions and nothing
    verifiable in the harness — the plan's safety condition is not met.
  - `source/functions/info/resource_info.cpp` — `sample_resource_name`
    (calling resource) and `sample_resource_find` (live running check).
  - `060_features.lua`: found/unknown resource assertions.
- **REMOVED**: nothing.
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (165 assertions).
- **RISKS**: `vm()` results are valid only for the immediate call on the
  main thread (by design, documented); a future MTA ABI exposing an element
  API can extend this layer without breaking the shipped surface.
- **NEXT**: PHASE 10 — CLI (`mta init/build/test/docs/doctor/package/
  server/new`) in other/tools/mta.

## PHASE 10 — CLI (`mta`)

- **PHASE**: 10 — the `mta` project CLI (plan §24-§29): init, new
  function/object, build, test, docs, doctor, package, server.
- **CHANGED**:
  - `CMakeLists.txt`: new `sdk_docgen` executable target (links sdk_core,
    dumps `Registry::instance().functions()` with PHASE 4 signature
    metadata as markdown) — consumed by `mta docs`.
- **ADDED**:
  - `other/tools/mta/cli.py` + launchers (`mta.cmd`, POSIX `mta`): the full
    command set on stdlib Python only.
  - `other/tools/docgen.cpp` — registry documentation generator; needs no
    Lua VM (specs are populated by the registration macros' static
    initializers; body-style functions render `name(...)` with the explicit
    "signature not derived" note, plan §9).
  - `mta init <name>`: copies the SDK checkout as the project template,
    excludes build/.git/dist/toolchain outputs, rewrites module.toml
    identity, refuses inside an existing project / into a non-empty target,
    guards against copying the checkout into its own subtree.
  - `mta new function <name>` / `mta new object <name>`: compile-ready
    skeletons; registered names verbatim (`crypto.sha256` stays
    `crypto.sha256`), C++ identifiers sanitized (`crypto_sha256.cpp`,
    `Weapon`), overwrite refused; verified by compiling generated files.
  - `mta doctor`: real checks with OK/WARN/FAIL per section — TOML validity
    and identity, SDK headers, Lua ABI (byte-compare of every
    mta-sdk/lua/*.h against lua/src/*.h), cmake/ninja/g++ versions from the
    local toolchain or PATH, C++ standard from TOML, presets, source
    discovery, build output, git state, server env; prints READY/NOT READY.
  - `mta test all|unit|lua|integration` via ctest name filters;
    `mta package` (dist/<name>-<version>-<platform> + sha256);
    `mta server` reports that the harness lands in PHASE 11.
- **REMOVED**: nothing.
- **TESTS**: `ctest --preset win-mingw` 3/3 passed (165 assertions).
  CLI verified end-to-end: doctor READY (4 headers byte-identical, toolchain
  probes), docs markdown (30 functions), new function `crypto.sha256` and
  new object `weapon` compiled with zero warnings, test lua/unit suites
  green, package wrote dist/base-2.0.0-win-x64.dll (sha256 printed), init
  scaffolded a module project in a temp dir (identity rewritten), guards
  and server stubs behave. Generated skeletons removed after verification.
- **RISKS**: the CLI assumes the local toolchain lives under
  build/toolchain/<name>/<flavour>/bin (git-ignored) or a normal PATH;
  `mta init` copies the whole checkout (large but explicit); Windows
  subprocess resolution needed a PATH-aware which() because CreateProcess
  ignores the child env's PATH (handled in run_tool).
- **NEXT**: PHASE 11 — real server harness in other/server/ (pinned MTA
  server download, temp server dir, module install, start/stop, logs),
  wired into `mta server` and `mta test integration`.