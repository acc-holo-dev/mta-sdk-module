# other/server

Infrastructure for real MTA-server integration testing.

The directory is intentionally **not** in Git beyond this file and small
helper scripts: server binaries are downloaded and pinned locally, never
committed (see the root `.gitignore`).

Responsibilities (implemented by the `mta server` CLI and the integration
harness):

- download a specific, pinned MTA server build (platform + architecture +
  revision + URL + checksum recorded in a lock file);
- unpack it into a local, throwaway server directory;
- copy the built module (`<module-name>.dll` / `.so`) into
  `mods/deathmatch/modules/`;
- install a minimal test resource and server configuration;
- start the server, wait for readiness, capture console/log output;
- stop the server and check the expected results and exit codes.

The integration test suite lives in `other/tests/integration/`.