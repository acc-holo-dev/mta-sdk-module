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