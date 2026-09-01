# other/tools

Project tooling. The main entry point is the `mta` developer CLI
(`other/tools/mta`), invoked from the repository root or through your PATH:

```bash
mta init            # scaffold a new module project
mta build           # configure + build (wraps the CMake presets)
mta test            # unit + Lua + integration tests
mta docs            # generate API documentation from the registry
mta doctor          # verify the whole development environment
mta package         # package the built module
mta server ...      # download/pin/start/stop the test MTA server
mta new function X  # scaffold a new Lua-exposed function
mta new object X    # scaffold a new native object
```

See `other/tools/mta/README.md` for the full command reference.