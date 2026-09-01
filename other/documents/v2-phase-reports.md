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