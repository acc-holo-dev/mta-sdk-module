# AGENTS.md

Guidance for AI coding agents working in this repository. Keep it current:
when a rule here conflicts with the code, fix the code or fix this file —
never work around a rule silently.

## What this project is

MTA:SA Module SDK — a C++20 framework for native MTA:SA server modules.
Developers include only the facade `<mta/sdk.hpp>`; module identity and
build options come from `config/module.toml`; the `mta` CLI lives in
`other/tools/mta/`; documentation in `other/documents/`.

## Non-negotiable invariants

- **ABI cleanliness** — never store a `lua_State*` between calls and never
  touch Lua from worker threads. Values cross the async boundary only as
  owned snapshots (`mta::lua::Argument/Table/Arguments`); the borrowed view
  (`mta::state`) is for the synchronous call only.
- **Generation invariant** — every callback, task and timer is owned by
  `(resource, generation)` and must be dropped before any Lua access once
  its resource restarted. Never weaken the checks in
  `source/sdk/runtime/` or `source/sdk/resources/`.
- **No `noexcept` on the Lua trampoline boundary** (`protected_call` /
  `protected_call_named`) — the catch blocks end in `luaL_error`, which
  longjmps; `noexcept` there is an MSVC fail-fast (`0xc0000409`).
- **Vendored code is frozen** — `other/third_party/` (Lua 5.1 with MTA's
  `luaL_newstate` extension + MTA SDK server headers) is byte-identity
  checked in CI. Edit it only with a concrete compatibility reason.
- **Errors** go through `mta::errors`; user-facing messages are English and
  readable to Lua scripters (`bad argument #N to 'name' (expected X, got
  Y)`); internal failures must render as `internal module error: ...`.
- **Registration names are verbatim** — `MTA_FUNCTION("name", ...)`
  registers exactly that name; never add prefixes, namespaces or renaming.

## Build and test

On the Windows dev machine there is no system Python/CMake — use the
toolchain under `build/toolchain/` and put
`build/toolchain/mingw/mingw64/bin` on `PATH` (the test binary dies with
`0xC0000135` without it):

```bash
mta doctor                # environment readiness (PASS/WARN/FAIL/SKIP)
mta build                 # presets: win-mingw (default), win-msvc, linux-gcc
mta test unit             # configuration parser tests
mta test lua              # embedded Lua harness, scripts 010-095
mta test integration      # pinned real MTA server (after `mta server install`)
mta docs                  # generated function reference into generated/
mta package               # dist/<module>-<version>-<platform>.dll/.so + sha256
```

Raw CMake works too: `cmake --preset win-mingw`, `ctest --preset
win-mingw --output-on-failure`. The integration suite is Windows-only (the
MTA server binary is) and never uses a developer-installed server.

## Conventions

- C++20; follow `.clang-format` and `.editorconfig`; LF line endings;
  comments and user-facing strings in English — explain why, lifetimes,
  ownership and ABI constraints, not what the code visibly does.
- `config/module.toml` is the single module configuration (identity, build
  options, `[async]`, `[features]`); CMake `-D` cache entries are overrides
  only. Disabled features exclude their bundled samples from the build.
- Version flow: Module version in `config/module.toml` (`[module] version`);
  `SDK_VERSION` / `SDK_ABI_VERSION` in `source/sdk/version.hpp` feed
  `project(VERSION)` and `mta doctor`. Keep the two version entities
  distinct.
- New functions: drop a `.cpp` under `source/functions/<domain>/` (picked up
  automatically by source discovery) and add a Lua script
  `other/tests/lua/scripts/NNN_name.lua`. The bundled samples double as the
  Lua suite's fixtures — changing one may change test expectations.
- Performance work requires measured numbers: the `09*-benchmark*.lua`
  scripts print ops/s through the Lua suite; before/after rates belong in
  the PR description.

## Documentation and hygiene

- Every documented command and example must be run before committing the
  change that touches it. User-facing behavior changes update the matching
  document under `other/documents/` in the same change, plus a
  `CHANGELOG.md` entry under `[Unreleased]`.
- Never commit: `build/`, `dist/`, `generated/`,
  `other/server/{downloads,servers,logs,run,tools}/`,
  `other/server/install.json` (all gitignored), or any agent-planning
  artifacts — task lists, phase reports, handover notes, audit scratch.
- CI has six jobs (Linux GCC/Clang, Windows MinGW-w64/MSVC, doctor,
  real-server integration) and must stay green. The tag-triggered release
  workflow ships exactly `<module>.dll` / `<module>.so` — nothing else.

## Verify before finishing

`mta doctor`, `mta build`, `mta test unit`, `mta test lua` — and
`mta test integration` whenever runtime code (`source/sdk/runtime/`,
`source/sdk/resources/`, `source/sdk/abi/`) changed.