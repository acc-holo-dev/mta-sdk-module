# other/tests/integration

Integration test suite that runs the module inside a **real MTA server**
(provisioned by `other/server/` infrastructure; the pinned server binary is
downloaded locally, never committed). This directory is the suite:
`other/server/mta_server.py` installs these Lua files as the test resources
`sdkintegration` (main suite) and `sdkintegration2` (witness), drives their
lifecycle through the server console and asserts on the reported markers.

## Run

```bash
python other/server/mta_server.py test     # full run (builds the module first)
# or
python other/tools/mta/cli.py test integration
```

Requires the pinned server once: `python other/server/mta_server.py install`.
Each run creates a throwaway server directory, keeps the console log in
`other/server/logs/<timestamp>/server.log`, and exits 0 only when **every**
scenario below reports PASS.

## How scenarios are reported

Every scenario prints its own marker line to the server log:

```text
SCENARIO <name>: PASS | FAIL            # one marker per scenario
INTEGRATION_RESULT: PASS | FAIL (...)   # per-generation summary
```

The harness (`mta_server.py`) parses these markers and fails the run if any
required scenario is missing, any scenario reports FAIL, any per-generation
`INTEGRATION_RESULT` reports FAIL, or any stale-delivery marker appears
(`STALE_TASK_DELIVERED*`, `OLD_CALLBACK_FIRED*`, `STALE_COLLISION_FIRED`,
`SHOULD_NEVER_FIRE`).

The main resource runs three generations, choreographed by the harness:

```text
generation 1  auto-start      function-level scenarios + §33 arming
         -> INTEGRATION: STOP_NOW     harness: stop sdkintegration
generation 2  stop + start    resource stop/start + stale windows
         -> INTEGRATION: RESTART_NOW  harness: restart sdkintegration
generation 3  after restart   restart scenarios, multiple timers,
                              shutdown worker, final result
         -> INTEGRATION: RUN_COMPLETE
```

## Scenario coverage (plan PROMT.md §32 + §33)

| # | Scenario | Reported by | Check |
|---|----------|-------------|-------|
| 1 | module load | gen 1 | module functions exist in the resource VM (and the module logged its load at boot) |
| 2 | module unload | harness | graceful console shutdown with the module loaded (`ShutdownModule` path; crash/kill fails) |
| 3 | resource start | gen 2 | fresh VM after the harness's explicit `start`; functions re-registered, own resource name |
| 4 | resource stop | gen 2 | generation 1's `onResourceStop` handler ran during the stop (marker file) |
| 5 | resource restart | gen 3 | console `restart` completed; generation 2 stopped cleanly, generation 3 is fresh |
| 6 | function registration | gen 1 | all module entry points registered in the VM |
| 7 | argument validation | gen 1 | type/count violations rejected as catchable Lua errors |
| 8 | return values | gen 1 | sums, greetings and multi-return values correct |
| 9 | callback | gen 1 | `sample_async_add` delivers 3 through the callback |
| 10 | timer | gen 1 | module timer fires all 3 repeats |
| 11 | async task | gen 1 | `sample_task_run` returns a valid task handle |
| 12 | async completion | gen 1 | worker result (30) delivered on the main thread |
| 13 | userdata | gen 1 | counter object create/get/add works |
| 14 | userdata invalidation | gen 2 + gen 3 | previous generation's userdata value does not leak into the fresh VM; the old timer handle is dead in the new VM |
| 15 | multiple resources | gen 1 + witness | `sdkintegration` and `sdkintegration2` run side by side; per-resource state is isolated (witness session starts at 1) |
| 16 | old callback after restart | gen 2 + gen 3 | module-timer callback armed before the stop/restart never fires afterwards |
| 17 | old async task after restart | gen 2 + gen 3 | async task armed before the stop/restart never delivers afterwards |
| 18 | multiple timers after restart | gen 3 | three fresh timers all fire twice in the restarted VM |
| 19 | shutdown with active workers | harness | 60 s task still pending at graceful shutdown; never delivered |
| 20 | stale generation regression (§33) | gen 2 + gen 3 | Resource generation N arms callback + async task → stop → restart same resource → generation N+1; old objects provably cannot reach it (negative markers verified over the whole log) |

Scenarios 16/17/20 combine both sides: the Lua suite prints its marker after
the stale window elapsed inside the next generation, and the harness
independently verifies the absence of the delivery markers in the entire log
(the stale callbacks are additionally wired through the fresh VM's first
`luaL_ref` slot — the collision the module must never allow, plan §11/§12/§14).

The most important regression (§33) runs automatically twice per invocation:
once through the explicit `stop` + `start` cycle (generation 1 → 2) and once
through the console `restart` (generation 2 → 3).