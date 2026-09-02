# Task 18 — Integration testing с реальным MTA server

**Источник:** PROMT.md — разделы: 30 — Real MTA Server integration; 31 — Server test harness; 32 — Integration test scenarios; 33 — Самый важный regression test; фазы: PHASE 11 — Real server harness
**Область задачи:** Задача покрывает infrastructure интеграционного тестирования с реальным MTA server — установку и фиксацию версии server через CLI, server test harness, набор integration test scenarios и обязательный regression test на restart resource.

---

# 30. Real MTA Server integration

`other/server/` является обязательной частью test infrastructure.

CLI должен уметь:

```bash
mta server install
mta server update
mta server version
mta server start
mta server stop
```

Server binaries НЕ коммитить в Git.

Не использовать blindly `latest`.

При установке фиксировать:

```text
platform
architecture
MTA branch
build/revision
download URL
checksum
```

Для nightly использовать конкретную выбранную сборку.

Nightly infrastructure MTA публикует отдельные Windows x64 и Linux x64 server builds; nightly являются development/unstable builds, поэтому test infrastructure должна хранить точную revision/build identity.

# 31. Server test harness

Integration harness должен уметь:

1. установить server;
2. подготовить temporary server directory;
3. установить module DLL/SO;
4. создать test resource;
5. создать необходимую configuration;
6. запустить server;
7. дождаться server ready state;
8. вызвать Lua integration tests;
9. собрать stdout/stderr/log;
10. проверить expected output;
11. корректно остановить server;
12. вернуть exit code;
13. очистить temporary files.

Tests должны быть deterministic.

Не использовать developer's global MTA installation.

# 32. Integration test scenarios

Добавить реальные tests минимум для:

```text
module load
module unload
resource start
resource stop
resource restart
function registration
argument validation
return values
callback
timer
async task
async completion
userdata
userdata invalidation
multiple resources
old callback after restart
old async task after restart
multiple timers after restart
shutdown with active workers
```

# 33. Самый важный regression test

Обязательно:

```text
Resource A generation 1
    ↓
create callback
    ↓
start async task
    ↓
stop resource
    ↓
restart same resource
    ↓
Resource A generation 2
```

Старые objects/tasks/callbacks должны быть полностью неспособны обратиться к generation 2.

Это должен быть автоматический regression test.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 11 — Real server harness

Реализовать:

```text
server download
server pinning
server install
server start
server stop
integration execution
logs
cleanup
```

---

## Чек-лист соответствия проекта

- [ ] Каталог `other/server/` существует и является обязательной частью test infrastructure.
- [ ] CLI поддерживает команды `mta server install`, `mta server update`, `mta server version`, `mta server start`, `mta server stop`.
- [ ] Server binaries не коммитятся в Git.
- [ ] Не используется blindly `latest` — при установке фиксируются platform, architecture, MTA branch, build/revision, download URL и checksum.
- [ ] Для nightly используется конкретная выбранная сборка; test infrastructure хранит точную revision/build identity.
- [ ] Integration harness выполняет все 13 шагов: установить server, подготовить temporary server directory, установить module DLL/SO, создать test resource, создать configuration, запустить server, дождаться server ready state, вызвать Lua integration tests, собрать stdout/stderr/log, проверить expected output, корректно остановить server, вернуть exit code, очистить temporary files.
- [ ] Integration tests детерминированы и не используют developer's global MTA installation.
- [ ] Существуют реальные integration tests минимум для всех сценариев: module load, module unload, resource start, resource stop, resource restart, function registration, argument validation, return values, callback, timer, async task, async completion, userdata, userdata invalidation, multiple resources, old callback after restart, old async task after restart, multiple timers after restart, shutdown with active workers.
- [ ] Реализован автоматический regression test: Resource A generation 1 → create callback → start async task → stop resource → restart same resource → generation 2; старые objects/tasks/callbacks полностью неспособны обратиться к generation 2.