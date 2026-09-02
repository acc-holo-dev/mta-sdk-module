# Task 11 — Timers V2

**Источник:** PROMT.md — разделы: 15 — Timers V2; фазы: PHASE 7 — Timer V2
**Область задачи:** Задача покрывает простой public API таймеров (`after`/`every` с handle) и их resource-aware поведение при остановке/рестарте ресурса.

---

# 15. Timers V2

Создать простой public API:

```cpp
auto timer = mta::timer::after(5000, [] {
    ...
});
```

и:

```cpp
auto timer = mta::timer::every(1000, [] {
    ...
});
```

Handle должен позволять:

```cpp
timer.cancel();
timer.valid();
```

Timers должны быть resource-aware.

При stop resource:

```text
all owned timers
    ↓
invalidated/cancelled
```

Не должно быть stale execution после restart.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 7 — Timer V2

Добавить:

```text
after()
every()
handle.cancel()
```

---

## Чек-лист соответствия проекта

- [ ] Реализован public API `mta::timer::after(...)` для создания таймера с задержкой.
- [ ] Реализован public API `mta::timer::every(...)` для создания повторяющегося таймера.
- [ ] Timer handle позволяет вызывать `cancel()` и `valid()`.
- [ ] Timers являются resource-aware (привязаны к owner-ресурсу/ResourceContext).
- [ ] При stop ресурса все owned timers инвалидируются/cancel'ются.
- [ ] После restart ресурса отсутствует stale execution таймеров.
- [ ] Добавлены `after()`, `every()` и `handle.cancel()` (PHASE 7).