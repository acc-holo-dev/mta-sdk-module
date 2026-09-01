# other/tests/integration

Integration tests that run the module inside a **real MTA server**
(provisioned by `other/server/` infrastructure; the server binary is
downloaded locally, never committed).

Each test:

1. prepares a throwaway server directory with a pinned server build;
2. installs the freshly built module and a test resource;
3. starts the server and drives scenarios (module load, resource
   start/stop/restart, callbacks, timers, async tasks, userdata, shutdown
   with active workers);
4. asserts on captured console/log output and the server exit code.

The most important regression: a resource of generation N stops, restarts as
generation N+1, and every callback/timer/async task from generation N must
be provably unable to touch the new generation's VM.