# Contributing

Thanks for improving the module SDK! This guide explains where things live,
which conventions to keep and how to verify a change.

## Project layout

- `source/sdk/` — the framework. `abi/` (MTA lifecycle hooks), `lua/`
  (value/stack layer), `bind/` (typed binder), `runtime/` (scheduler,
  callbacks, timers), `registry/` (function registry + registration
  macros), `objects/` (userdata), `resources/` (resource identity and
  generations), `events/`, `logging/`, `errors/`.
- `source/mta/sdk.hpp` — the public facade. Developer code includes only
  this header.
- `source/functions/` — developer functions, grouped by domain. The bundled
  samples live here too and double as the Lua suite's fixtures.
- `source/library/` — reusable, module-agnostic C++ helpers. Functions may
  use the library; the library must never depend on functions or the SDK.
- `config/module.toml` — module identity and build options (single source).
- `config/cmake/` — the CMake implementation (core, lua, platform, install).
- `other/tests/` — `lua/` (embedded harness + scripts), `unit/`
  (CMake-script tests), `integration/` (real-server scenarios).
- `other/server/` — pinned MTA server test infrastructure (not tests
  themselves; no binaries are committed).
- `other/tools/` — the `mta` CLI and the docs generator.
- `other/third_party/` — vendored Lua 5.1 and MTA SDK server headers.
  Never modify vendored sources without a concrete compatibility reason.

## Where new code goes

| You are adding... | Location |
| --- | --- |
| a Lua-callable function | `source/functions/<domain>/<name>.cpp` |
| a reusable object type | `source/functions/<domain>/` (`MTA_OBJECT` + `MTA_METHOD`) |
| a non-Lua utility shared by functions | `source/library/<topic>/` |
| an SDK framework capability | `source/sdk/<layer>/` |
| a test | `other/tests/lua/scripts/`, `other/tests/unit/`, `other/tests/integration/` |

## Adding a function

1. Run `mta new function <name>` (or drop a `.cpp` under
   `source/functions/<domain>/` with one of the registration macros — see
   `other/documents/example.md`).
2. Rebuild: new files and their registrations are picked up automatically.
3. Add a Lua test script under `other/tests/lua/scripts/` (naming:
   `NNN_name.lua`); the harness runs every script automatically.

## Code style

- Follow `.clang-format` and `.editorconfig` (LF line endings).
- Comments in English; explain why, lifetimes, ownership, thread rules and
  ABI constraints — not what the code visibly does.
- User-facing strings (descriptions, error messages) in English.
- C++20; no dependencies beyond the standard library, the vendored Lua and
  the MTA SDK headers.
- Keep the module ABI-clean: never store a `lua_State*` between calls and
  never touch Lua from worker threads (use the async completion / callback
  APIs).
- Prefer `mta::errors::raise_error` / `raise` over raw `luaL_error` so
  errors stay renderable at the trampoline boundary.

## Testing

```bash
mta test unit            # configuration parser tests
mta test lua             # embedded Lua harness: functions + regressions
mta test integration     # pinned real MTA server (after `mta server install`)
mta doctor               # environment readiness
```

Run the suites for the platforms you touched before opening a pull request;
CI covers Linux GCC/Clang and Windows MinGW-w64/MSVC plus the real-server
integration.

### Performance changes (measure before optimizing)

Optimize only what a benchmark proves is slow. The benchmark scripts live
in `other/tests/lua/scripts/09*-benchmark*.lua` and run through the Lua
suite (`mta test lua` prints an ops/s rate for every benchmark). A pull
request that claims a performance improvement records the affected
benchmark, the preset it was measured on and the before/after numbers in
the description; optimizations without measured numbers are not accepted.

## Documentation

- User-facing behavior changes: update the relevant document under
  `other/documents/` (`example.md` for features, `api.md` for the public
  surface, `architecture.md` for internals).
- Every documented command and example must work: run it before committing.
- Update `CHANGELOG.md` under a new `[Unreleased]` heading.

## Releases

- Bump the version in `config/module.toml` (`[module] version`) only — the
  module reports it as the Module version float; the SDK's own version
  lives separately in `source/sdk/version.hpp`
  (`SDK_VERSION` / `SDK_ABI_VERSION`). Add a CHANGELOG entry.
- Pushing a tag like `v2.1.0` triggers the Release workflow: all three
  platform legs build and run the test suites, the win-mingw leg runs the
  blocking real-server integration, and the release attaches exactly the
  module binaries — `<module>.dll` / `<module>.so`, produced by
  `mta package --release-name` — to a GitHub Release (see
  `.github/workflows/release.yml`). Nothing else ships.