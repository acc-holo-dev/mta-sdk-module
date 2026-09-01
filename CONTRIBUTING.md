# Contributing

Thanks for improving ml_base! This short guide explains where things live and
what conventions to keep.

## Project layout

- src/module/ — MTA lifecycle hooks (init, pulse, resource events).
- src/lua/ — Lua-stack helpers and the typed binder (bind.hpp).
- src/registry/ — function registry and the MTA_LUA_FUNCTION / MTA_LUA_FUNC
  macros.
- src/runtime/ — scheduler, callbacks, per-resource state, logging.
- src/functions/ — your functions, grouped by domain.
- tests/ — embedded Lua harness (harness.cpp) + scripts.
- vendor/ — vendored Lua 5.1 and the MTA SDK headers (keep untouched).
- cmake/ — build infrastructure.

## Adding a function

1. Drop a .cpp anywhere under src/functions/<domain>/ using one of the two
   registration macros (see README.md, "Writing functions").
2. Rebuild — new files and their registration are picked up automatically.
3. Add a Lua test script under tests/scripts/ (naming: NNN_name.lua); the
   harness runs every script automatically.

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

## Releases

- Bump the version in CMakeLists.txt (project VERSION) only — the module
  reports it automatically (src/module/module.cpp reads it at configure
  time); add a CHANGELOG.md entry.
- Pushing a tag like v1.1.0 triggers the Release workflow, which builds
  ml_base.dll / ml_base.so, packages ZIP archives and attaches everything to
  a GitHub Release (see .github/workflows/release.yml).
