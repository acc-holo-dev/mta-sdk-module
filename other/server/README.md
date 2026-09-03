# other/server

Infrastructure for real MTA-server integration testing.

The directory is intentionally **not** in Git beyond this file and small
helper scripts: server binaries are downloaded and pinned locally, never
committed (see the root `.gitignore`).

The pinned server has a **Windows** build (MTA Server64.exe, extracted with a
locally provisioned 7-Zip) and a **Linux** build (multitheftauto_linux_*
tarball, extracted with `tar`); `pinned_build()` selects the one for the
current host. On Windows the harness drives the server's console (Win32 key
injection); on Linux the headless server is driven through a stdin/stdout
pipe.

Responsibilities (implemented by the `mta server` CLI and the integration
harness):

- download a specific, pinned MTA server build (platform + architecture +
  revision + URL + checksum recorded in a lock file);
- unpack it into a local, throwaway server directory;
- copy the built module (`<module-name>.dll` / `.so`) into
  `x64/modules/`;
- install a minimal test resource and server configuration;
- start the server, wait for readiness, capture console/output log;
- stop the server and check the expected results and exit codes.

The integration test suite lives in `other/tests/integration/`.
Run it with `python other/server/mta_server.py test` (installs the pinned
server first via `python other/server/mta_server.py install`).