# Task 14 — Lua values: View vs Snapshot

**Источник:** PROMT.md — разделы: 18 — Lua values: View vs Snapshot; 45 — Lua table model
**Область задачи:** Задача покрывает разделение двух моделей работы с Lua-значениями — borrowed LuaView для synchronous operations и owned Snapshot для async — запрет передачи raw Lua state/value references между потоками и модель Lua table для worker threads.

---

# 18. Lua values: View vs Snapshot

Разделить две модели.

## LuaView

Borrowed access к текущему Lua state.

Используется для synchronous operations.

```text
Lua
 ↓
LuaView
```

## Snapshot

Owned/copyable representation.

Используется для async.

```text
Lua
 ↓
Snapshot
 ↓
worker
```

Никогда не передавать raw Lua state/value references между threads.

# 45. Lua table model

Предусмотреть:

```text
LuaView
Snapshot
```

Не разрешать worker threads обращаться к borrowed Lua values.

Async автоматически должен работать только с thread-safe owned values.

---

## Чек-лист соответствия проекта

- [ ] В SDK реализованы две разделённые модели Lua-значений: LuaView и Snapshot.
- [ ] LuaView — borrowed access к текущему Lua state — используется для synchronous operations.
- [ ] Snapshot — owned/copyable representation — используется для async (передача в worker выполняется через Snapshot, а не через Lua state).
- [ ] Отсутствует передача raw Lua state/value references между потоками.
- [ ] Worker threads не обращаются к borrowed Lua values.
- [ ] Async автоматически работает только с thread-safe owned values.