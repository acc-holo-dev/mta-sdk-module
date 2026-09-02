# Task 10 — Async V2 и scheduler

**Источник:** PROMT.md — разделы: 13 — Scheduler V2; 14 — Async resource lifecycle; фазы: PHASE 6 — Async V2
**Область задачи:** Задача покрывает высокоуровневый developer-facing async API поверх internal Scheduler и безопасный lifecycle async-операций при остановке ресурса.

---

# 13. Scheduler V2

Текущий Scheduler сохранить как internal implementation.

Developer-facing API должен быть выше уровнем.

Например:

```cpp
auto task = mta::async::run(...);
```

и:

```cpp
task.cancel();
task.done();
task.valid();
```

Поддержать:

* background work;
* main-thread completion;
* cancellation;
* task state;
* safe shutdown;
* resource ownership;
* queue limits.

Lua никогда не должен вызываться напрямую из worker thread.

# 14. Async resource lifecycle

Каждая async operation должна иметь понятный owner.

Если resource остановлен:

```text
resource stopped
    ↓
owned tasks
    ↓
invalidate/cancel completion
    ↓
NO Lua access
```

Нельзя допустить:

```text
old task
    ↓
resource restarted
    ↓
new VM
    ↓
old task calls new VM
```

Добавить отдельные tests.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 6 — Async V2

Добавить:

```text
task handle
cancel
state
resource ownership
safe shutdown
```

---

## Чек-лист соответствия проекта

- [ ] Текущий Scheduler сохранён как internal implementation, а developer-facing async API находится уровнем выше (например, `mta::async::run(...)`).
- [ ] Task handle поддерживает `cancel()`, `done()` и `valid()`.
- [ ] Поддержаны background work, main-thread completion, cancellation, task state, safe shutdown, resource ownership и queue limits.
- [ ] Lua никогда не вызывается напрямую из worker thread.
- [ ] Каждая async operation имеет понятного owner.
- [ ] При остановке ресурса owned tasks инвалидируются/cancel completion без Lua access.
- [ ] Исключён сценарий, при котором old task после restart ресурса обращается к новой VM.
- [ ] Добавлены отдельные tests на async resource lifecycle.
- [ ] Реализованы task handle, cancel, state, resource ownership и safe shutdown (PHASE 6).