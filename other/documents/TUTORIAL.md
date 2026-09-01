# Tutorial: a module from scratch

A step-by-step path from an empty folder to a working module loaded on an
MTA server. You need a compiler (MinGW-w64 or MSVC on Windows, GCC on Linux),
CMake ≥ 3.27 and Ninja.

## Step 1. Clone the base

```bash
git clone <your-repository> my-module
cd my-module
```

## Step 2. Build as-is

```bash
# Windows (MinGW-w64)
cmake --preset win-mingw
cmake --build --preset win-mingw

# Linux
cmake --preset linux-gcc
cmake --build --preset linux-gcc
```

Result: build/win-mingw/module/win-x64/base.dll (or .so on Linux).

## Step 3. Run the tests

```bash
ctest --preset win-mingw
```

You should see 100% tests passed. That means the base works.

## Step 4. Add your first function

Create source/functions/basics/my_sum.cpp:

```cpp
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("my_sum", "Adds two numbers.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, a + b);
}
```

Rebuild — the my_sum function is already available. No CMake or central file
edits needed: the build picks up new .cpp files automatically.

## Step 5. Rename the module

Name, author and the output binary are CMake cache variables — rename with
one configure line, no C++ edits:

```bash
cmake --preset win-mingw \
    -DSDK_MODULE_NAME=my_mod \          # -> my_mod.dll / my_mod.so
    -DSDK_MODULE_TITLE="My Module" \    # console name
    -DSDK_MODULE_AUTHOR="Jane Doe"      # console author
cmake --build --preset win-mingw
```

Default values are `base` / `Base Module` / `anon`.

## Step 6. Install on the server

1. Copy `base.dll` (or your chosen name) into the server's
   mods/deathmatch/modules/.
2. Add to mtaserver.conf:

```xml
<module src="base"/>
```

3. Restart the server. The console shows:

```
MODULE: Loaded "Base Module" (1.10) by "anon"
```

## Step 7. Check in Lua

In any server resource:

```lua
outputChatBox("2 + 3 = " .. my_sum(2, 3))  -- 2 + 3 = 5
```

## What's next

- [README.md](../README.md) — every function recipe.
- [API.md](API.md) — the complete API reference.
- [GUIDES.md](GUIDES.md) — advanced topics (threads, async, tables).
- [ARCHITECTURE.md](ARCHITECTURE.md) — how the system is put together.
- source/functions/ — live examples of every feature.
