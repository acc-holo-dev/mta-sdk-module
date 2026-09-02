# Contributing

Thanks for improving the module SDK! This short guide explains where things
live and what conventions to keep.

## Project layout

- source/sdk/abi/ — MTA lifecycle hooks (init, pulse, resource events).
- source/sdk/lua/ — Lua-stack helpers; the typed binder is at
  source/sdk/bind/bind.hpp.
- source/sdk/registry/ — the function registry and the
  MTA_LUA_FUNCTION / MTA_LUA_FUNC macros.
- source/sdk/runtime/ — scheduler and callbacks; per-resource state lives in
  source/sdk/resources/, logging in source/sdk/logging/.
- source/functions/ — your functions, grouped by domain.
- source/library/ — reusable non-Lua C++ helpers (functions may use them;
  they must not depend on functions).
- other/tests/ — lua/ (embedded harness + scripts), unit/, integration/.
- other/third_party/ — vendored Lua 5.1 and the MTA SDK headers (keep
  untouched).
- cmake/ — build infrastructure.

## Adding a function

1. Drop a .cpp anywhere under source/functions/<domain>/ using one of the two
   registration macros (see README.md, "Writing functions").
2. Rebuild — new files and their registration are picked up automatically.
3. Add a Lua test script under other/tests/lua/scripts/ (naming:
   NNN_name.lua); the harness runs every script automatically.

## Requirements

- C++20, no dependencies beyond std::thread and Lua 5.1.
- Keep the module ABI-clean: never store lua_State* between calls and never
  touch Lua from worker threads (use mta::async::Callback / Store).
- Error messages should be human-readable English; prefer raise_error over
  luaL_error.

## Code style

- Follow .clang-format (and .editorconfig for line endings).
- Comments in English.
- New user-facing strings (descriptions, error messages) in English.

## Testing

```bash
ctest --preset win-mingw          # or any supported preset
```

Run the full suite locally and in CI (Linux GCC, Windows MinGW and MSVC,
plus a unity build) before opening a pull request.

## Performance changes (measure before optimizing)

Optimize only what a benchmark proves is slow (plan §44): the benchmark
scripts live in `other/tests/lua/scripts/09*-benchmark*.lua` and run through
the Lua harness — `mta test --preset <preset> lua` prints an ops/s rate for
every benchmark. A pull request that claims a performance improvement
records the affected benchmark, the preset it was measured on and the
before/after numbers in the PR description; optimizations without measured
numbers are not accepted.

## Releases

- Bump the version in `config/module.toml` (`[module] version`) only — the
  module reports it as the Module version float (plan §38: `source/sdk/abi/
  module.cpp` compiles it into the binary metadata); the SDK's own version
  lives separately in `source/sdk/version.hpp` (`SDK_VERSION`/`SDK_ABI_VERSION`).
  Add a CHANGELOG.md entry.
- Pushing a tag like v2.0.0 triggers the Release workflow, which builds,
  tests, runs the (blocking, win-mingw) real-server integration and attaches
  exactly the module binaries — `<module>.dll` / `<module>.so`, produced by
  `mta package --release-name` — to a GitHub Release (see
  .github/workflows/release.yml; plan §36: nothing else ships).
