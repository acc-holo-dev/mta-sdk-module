# V2 Audit — Current State of the Repository

> PHASE 0 deliverable of the V2 master plan (PROMT.md).
> Audit date: baseline commit `01e4111` ("changelog entry, gitignore/gitattributes
> hygiene"). No architecture was changed during the audit.

## 1. Verified baseline

- Build: **green** with the local MinGW toolchain
  (`build/toolchain/mingw/mingw64`, git-ignored: CMake 4.4.2, GCC 16.2.0
  ucrt-posix-seh, Ninja 1.13.2), preset `win-mingw`.
- Tests: **green** — `ctest --preset win-mingw`: `sdk_tests` Passed (3.72 s,
  10 Lua scripts, embedded Lua 5.1 + mock manager).
- CI: GitHub Actions matrix `linux-gcc` / `win-mingw` (MSYS2 UCRT64) /
  `win-msvc`; includes the vendored-Lua-headers drift check (`cmp` against
  compiled sources).
- An accidentally deleted `README.md` (deleted in the working tree, still in
  HEAD) was restored with `git checkout` before any other change.

## 2. Current architecture

```
CMakeLists.txt            project mta_sdk_module 1.1.0, C++20, unity + LTO
CMakePresets.json         win-mingw / win-msvc / linux-gcc
cmake/
  core/platform.cmake     SDK_PLATFORM_TAG / SDK_ARCH_TAG
  core/common.cmake       baseline warnings (W4/WX or -Wall -Wextra -Werror),
                          sdk_enable_lto()
  core/file.cmake         file(GLOB_RECURSE src/*.cpp CONFIGURE_DEPENDS)
  lua/lua.cmake           vendored Lua 5.1 (MTA-patched) as OBJECT lib mta_lua
  config/{msvc,mingw,linux}.cmake   per-toolchain flags
  install.cmake           install + CPack ZIP (references README.md, LICENSE)
src/
  module/                 MTA ABI: six entry points, .def.in, .ver, Info
  lua/                    binder (bind.hpp), Argument/Table snapshots,
                          stack helpers, protect (exception boundary),
                          userdata (typeid-keyed metatables), events, args
  registry/               Registry singleton + MTA_LUA_FUNCTION / MTA_LUA_FUNC
  runtime/                 Scheduler (workers + timers), Callback (luaL_ref),
                          resources::Hub/Store, logging
  functions/              sample functions in 8 domains (auto-discovered)
tests/                    harness.cpp (mock ILuaModuleManager10) + scripts/*.lua
vendor/                   mta-sdk (ILuaModuleManager10.h + lua headers),
                          lua/src (Lua 5.1.5 MTA-patched sources)
docs/                     API.md, ARCHITECTURE.md, GUIDES.md, TUTORIAL.md
.github/                  ci.yml, release.yml, dependabot.yml
```

### Module lifecycle (current)

- `InitModule` → stores the manager, starts the Scheduler (3 workers),
  reports name/author/version (from CMake cache variables).
- `RegisterFunctions` → per-resource-VM registration of every registry Spec.
- `DoPulse` → `Scheduler::pump()`: deliver worker completions + fire timers.
- `ResourceStopping/ResourceStopped` → notify `resources::Hub` (Store erase),
  cancel timers by resource name, mark callback refs dead.
- `ShutdownModule` → stop workers, release all Lua refs, clear stores.

### Lua ABI safety (existing, must keep)

- Module-facing Lua headers: `vendor/mta-sdk/lua/*.h` must stay
  **byte-identical** to the compiled `vendor/lua/src/*.h` (CI check; harness
  asserts `luaL_newstate(nullptr)` + `lua_getmtasaowner` — MTA's patched ABI
  with the extra `mtasaowner` argument).
- Export hygiene: only the six MTA entry points are exported (module.def /
  version script); no `LUA_BUILD_AS_DLL` (a past bug exported 157 symbols).
- No `std::string` crosses the DLL boundary (char*/size_t manager overload).

## 3. Existing public API (what V2 must not silently break)

| API | Verdict for V2 |
|---|---|
| `MTA_LUA_FUNCTION("name", "desc") { body }` | KEEP (body style; plan keeps it) |
| `MTA_LUA_FUNC("name", "desc", lambda)` | KEEP, add `MTA_FUNCTION` alias (plan §6 spelling) |
| `mta::lua::args<...>(L)` / `push_results(L, ...)` | KEEP |
| `mta::lua::{Argument, Table, Arguments}` (snapshots) | KEEP → becomes the "Snapshot" half of §18 |
| `mta::lua::{check,opt}_*` stack helpers | KEEP |
| `mta::lua::raise_error` | KEEP (format of messages CHANGEs in PHASE 4 — see risks) |
| `mta::async::Callback` | KEEP (re-based on ResourceContext in PHASE 5) |
| `mta::async::Scheduler::post_task/post_timer` | KEEP as internal; new facade `mta::async::run`, `mta::timer::{after,every}` on top |
| `mta::resources::Store/Hub` | KEEP (re-keyed by generation in PHASE 5) |
| `mta::log::{info,warn,error,debug}` | KEEP |
| `mta::userdata::Registry<T>` + `MTA_METHOD` | REWORK → `MTA_OBJECT` with stable string type IDs (plan §16 forbids `typeid(T).name()`) |
| `mta::lua::events::trigger` | KEEP |
| `module_functions`, `sample_*` functions | KEEP (regression anchors) |
| CMake presets + cache identity vars | CHANGE → single `config/module.toml` (PHASE 2) |

Naming rule (plan §2) is already satisfied: the registry registers the exact
developer-provided name (`RegisterFunction(vm, spec.name, ...)`), and the
binary name is the raw `SDK_MODULE_NAME`. V2 must add a regression test that
pins this (e.g. `crypto.sha256` stays verbatim).

## 4. Test inventory (baseline)

`tests/harness.cpp` runs `tests/scripts/*.lua` in a mock environment with
helpers `test_assert`, `test_pump`, `test_resource_stop`, `test_resource_start`:

| Script | Covers |
|---|---|
| 010_basic | basic calls, error translation |
| 020_tables | table marshalling |
| 030_async | worker→pump delivery, callback, timers, cancellation |
| 040_binder | C++ defaults, optional, pair/vector results, rest_args, raw stack, typed errors |
| 050_edge | nil, extra args, empty ranges, limits |
| 060_features | userdata counter, events, table helpers |
| 070_lifecycle | per-resource state reset on stop/restart |
| 080_stress | 1000 async tasks |
| 085_timer_mutation | timer create/cancel during dispatch (regression) |
| 090_benchmark | informational throughput |

Gaps vs plan §32: no unit tests (C++ level), no integration tests against a
real MTA server, no restart-generation regression (the single most important
test, plan §33 — the mock `test_resource_start` re-registers into the same VM,
which cannot expose generation confusion), no shutdown-with-active-workers test.

## 5. Findings: P0 issues and risks

### 5.1 Generation confusion in Callback tracking (P0, plan §12/§33)

Callback identity is `(resource name, luaL_ref index)`. On resource stop the
refs are marked dead but the VM dies **without unref**; on restart the fresh
VM starts its registry in the same state, so a *new* callback commonly gets
the **same ref index**. `from_stack` overwrites the dead entry:

```cpp
tracked_refs()[resource][ref] = TrackedRef{false};  // gen-2 ref 4 replaces gen-1's dead ref 4
```

A gen-1 `Callback::call()` then sees `ref_is_dead == false`, looks the
resource up by **name**, receives the **gen-2 VM** and calls whatever sits at
that ref — a stale callback fires the wrong function in the new VM. The
reproduce path exists today in the harness (single mock resource). This is
exactly the scenario plan §33 requires to be impossible. Fix in PHASE 5:
generation counter per resource (name + generation identity), callback identity
= (resource, generation, ref), refs unref'd/released on stop, VM lookup
checks generation.

### 5.2 `resources::Store` keyed by name only

State is erased on stop (correct), but there is no generation identity, so
APIs that hand out long-lived per-resource handles cannot distinguish
generations. Must ride the same ResourceContext (PHASE 5).

### 5.3 Async tasks have no owner, cancel or state (plan §13/§14)

`post_task` returns void; completions run regardless; queue is unbounded;
worker count hard-coded 3; shutdown drops pending tasks silently (acceptable)
but there is no cancellation, no task handle, no "resource stopped → cancel
owned tasks" contract (timers have it by name; tasks do not).

### 5.4 Userdata identity via `typeid(T).name()` (plan §16)

Metatable name = `"mta.userdata." + typeid(T).name()` — compiler-dependent,
also prefixed with SDK's own namespace (violates §2 in spirit). V2 needs
explicit stable IDs: `MTA_OBJECT("counter", Counter)`.

### 5.5 Error messages vs plan §7

Current: `argument #1 must be a string, got table` — readable, but tests pin
the exact wording (`must be a string`, `got no value`). Plan asks for
`bad argument #1 to 'sum' (expected number, got string)`. Changing the format
in PHASE 4 requires updating the Lua tests **in the same phase** (regression
rule §52).

### 5.6 Registry metadata is name+description only (plan §9/§21)

No per-argument/return type info, no flags/category — `mta docs` (PHASE 10)
needs signature metadata derived from the C++ signature at registration time.

### 5.7 Release artifacts violate plan §36

`release.yml` publishes the CPack ZIP (+ LICENSE) alongside the module
binary. V2 releases must publish **only** `<module-name>.dll` / `<module-name>.so`,
after a full build+test pipeline, with MSVC in the matrix (currently only
linux-gcc + win-mingw).

### 5.8 Misc risks

- `install.cmake` references `README.md` (restored now; keep the file).
- Local toolchain lives in `build/toolchain` (git-ignored) — CI must not
  depend on it; keep MSYS2/choco installers in workflows.
- Unity build + `-Werror=subobject-linkage` constrains where Impl structs
  live (keep Impl types at namespace scope).
- The harness mock has exactly one resource (`test_resource`); integration
  tests need real multi-resource scenarios (plan §32).
- Module version is a **float** in the MTA ABI (1.1.0 → "1.1") — keep, but
  plan §38 wants SDK/Module/ABI/MTA versions reported separately in
  diagnostics (not in the float).

## 6. Dependencies

- Lua 5.1.5, MTA-patched (vendored, compiled statically into the module).
- ILuaModuleManager10 (official module contract; vtable frozen).
- Threads (std::thread / winpthreads), no other third-party code.
- Build: CMake ≥ 3.27 + Ninja; compilers MSVC 2019+, MinGW-w64 (posix
  threads), GCC/Clang; optional ASan/UBSan via `SDK_SANITIZE`.
- CI: actions/checkout@v7, msys2/setup-msys2@v2, ilammy/msvc-dev-cmd@v1,
  softprops/action-gh-release@v2, dependabot.

## 7. Migration concerns (restructure → V2)

| Concern | Decision |
|---|---|
| Include paths | Keep internal include-root style (`"lua/bind.hpp"`) by making `source/sdk` the include root; add `source` as a second root for `<mta/sdk.hpp>`. Zero include churn in PHASE 1; umbrella header in PHASE 3 lives at `source/mta/sdk.hpp`. |
| `git mv` history | Move whole directories (`src→source`, `tests→other/tests`, `docs→other/documents`, `vendor→other/third_party`) so history follows. |
| CMake paths | Update `CMakeLists.txt`, `cmake/*.cmake`, presets (binaryDir unchanged: `build/<preset>`), CI paths (Lua header check, def/ver locations), install.cmake docs path. |
| Test harness include dir | Switch `src` → `source/sdk` (+ `other/third_party/mta-sdk` for `ILuaModuleManager10.h`). |
| vendored Lua ABI check | New path `other/third_party/{mta-sdk,lua}` — CI cmp job must be updated in the same commit. |
| Presets | Keep the same names (`win-mingw`, `win-msvc`, `linux-gcc`) — they are part of the advanced-user interface (plan §35). |
| Back-compat macros | `MTA_LUA_FUNCTION`/`MTA_LUA_FUNC` keep working for the whole V2 line; new `MTA_FUNCTION`/`MTA_OBJECT` are additive. |
| Sample functions | Move as-is to `source/functions/**` — they are the regression anchors; add new V2-API samples only when their subsystem lands. |

## 8. Phase-by-phase work implied by the audit

1. **PHASE 1** restructure (mechanical, build must stay green after).
2. **PHASE 2** `config/module.toml` — CMake reads it (TOML parse in CMake
   script); deprecate (keep as overrides) the identity cache variables.
3. **PHASE 3** umbrella `<mta/sdk.hpp>` + `MTA_FUNCTION`.
4. **PHASE 4** binder: plan-§7 error format (+ test updates), signature
   metadata capture, unified error model (plan §19).
5. **PHASE 5** ResourceContext + generation (P0, §5.1 above) + §33 regression
   test (needs a mock that can simulate a *fresh VM per generation* — extend
   the harness).
6. **PHASE 6** async facade: `mta::async::run` → handle (cancel/done/valid),
   ownership by resource, queue limits.
7. **PHASE 7** `mta::timer::{after,every}` handles.
8. **PHASE 8** `MTA_OBJECT` + stable type IDs + resource lifecycle for
   objects.
9. **PHASE 9** native MTA types — note: ILuaModuleManager10 exposes **no**
   element API; only VM lookup by resource name. Wrappers must therefore be
   conservative (validated lightuserdata handles obtained via Lua calls, or
   postponed per plan §17 "only if the API is available and safe").
10. **PHASE 10** `mta` CLI (python, `other/tools/mta`), incl. `mta docs` from
    registry metadata (a small introspection build or a generated JSON dump).
11. **PHASE 11** server harness in `other/server/` (download pinned nightly,
    temp server dir, module install, test resource, output capture).
12. **PHASE 12** docs rewrite (`example.md` first-class, per plan §39/§40).
13. **PHASE 13** CI: add `mta doctor`/CLI job, keep matrix, add integration
    job where feasible.
14. **PHASE 14** release: binaries-only artifacts, MSVC in matrix, tests
    gating publish.

## 9. Audit conclusion

The codebase is a healthy, well-tested V1: typed binder, async scheduler,
timers, per-resource state, Lua-ABI hygiene and CI all exist and are green.
The V2 work is therefore mostly **additive** (facade, CLI, config, docs,
server harness, metadata) with two deep changes: the P0 generation-identity
fix (§5.1) and the userdata identity rework (§5.4). Nothing in the current
architecture blocks the target structure; nothing requires a rewrite of the
scheduler or binder internals beyond the identified items.