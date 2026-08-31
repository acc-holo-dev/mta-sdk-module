# Changelog

All notable changes to this project are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions
follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
