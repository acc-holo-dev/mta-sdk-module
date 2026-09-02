# Task 23 — Performance и benchmarks

**Источник:** PROMT.md — разделы: 44 — Performance
**Область задачи:** Производительность SDK — создание benchmarks для ключевых операций (function call, argument/table conversion, callback, timer/async scheduling, userdata creation/access) и правило «оптимизировать только после измерений».

---

# 44. Performance

Не оптимизировать «на глаз».

Создать benchmarks для:

```text
function call
argument conversion
table conversion
callback
timer scheduling
async scheduling
userdata creation
userdata access
```

Оптимизировать только после измерений.

Особое внимание:

* allocations;
* Lua stack operations;
* table snapshots;
* callback bookkeeping;
* task queue.

---

## Чек-лист соответствия проекта

- [ ] Созданы benchmarks для function call и argument conversion.
- [ ] Созданы benchmarks для table conversion и callback.
- [ ] Созданы benchmarks для timer scheduling и async scheduling.
- [ ] Созданы benchmarks для userdata creation и userdata access.
- [ ] Оптимизации выполняются только после измерений на benchmarks, а не «на глаз».
- [ ] При оптимизациях отдельно рассмотрены allocations и Lua stack operations.
- [ ] При оптимизациях отдельно рассмотрены table snapshots, callback bookkeeping и task queue.