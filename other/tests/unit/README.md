# other/tests/unit

C++ unit tests that exercise SDK internals directly (no Lua involved):
binder type conversion, registry metadata, resource-generation bookkeeping,
scheduler queues.

Run with:

```bash
cmake --build --preset win-mingw --target sdk_unit_tests
ctest --preset win-mingw
```

Tests here register through the same CTest suite as the Lua harness.