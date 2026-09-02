# other/tests/unit

Unit tests that run without the embedded Lua harness: CMake-script tests of
the configuration reader (plan §4, `cmake/core/module-config.cmake`):

- `module_config_parse` — `config/module.toml` values are read correctly;
- `module_config_rejects_garbage` — malformed TOML is rejected (negative
  test, registered with `WILL_FAIL`).

The SDK internals themselves (binder conversions, registry metadata,
resource generations, scheduler queues and task ownership) are exercised by
the embedded harness in `../lua/`: the `sdk_tests` target
(`other/tests/lua/harness.cpp`) links the real module core into a clean Lua
5.1 VM with a mock module manager and runs every `scripts/*.lua` plus
C++-level async regressions.

Run with:

```bash
ctest --preset win-mingw --output-on-failure
```

or `mta test unit`. Tests here register through the same CTest suite as the
Lua harness.