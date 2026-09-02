# Task 9 — ResourceContext и callback safety

**Источник:** PROMT.md — разделы: 11 — Resource Context; 12 — Callback safety; фазы: PHASE 5 — ResourceContext/generation
**Область задачи:** Задача покрывает уникальную identity ресурса (ResourceContext с generation) и безопасность callback'ов при restart/stop ресурса.

---

# 11. Resource Context

Не использовать только resource name для identity.

Ввести concept:

```text
ResourceContext
```

с уникальной generation/version identity.

Например:

```text
resource name: test
generation: 41
```

После restart:

```text
resource name: test
generation: 42
```

Старые async callback/timer/state objects не должны иметь возможность обратиться к новой VM resource с тем же именем.

# 12. Callback safety

Это P0 requirement.

Старый callback после resource restart:

```text
OLD RESOURCE
    ↓
callback
    ↓
restart
    ↓
NEW RESOURCE
```

не должен случайно использовать новую Lua VM.

Callback identity должна быть привязана минимум к:

```text
resource identity
+
generation
+
Lua reference
```

При resource stop:

* callback становится invalid;
* Lua reference освобождается корректно;
* async completion не должен вызывать Lua;
* stale callback не должен обращаться к новой generation.

Добавить regression tests.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 5 — ResourceContext/generation

Это P0.

Реализовать:

```text
ResourceContext
generation
safe callback ownership
safe task ownership
safe timer ownership
```

Добавить restart regression tests.

---

## Чек-лист соответствия проекта

- [ ] Для identity ресурса не используется только resource name — введён concept `ResourceContext` с уникальной generation/version identity.
- [ ] После restart ресурса с тем же именем generation изменяется (например, 41 → 42).
- [ ] Старые async callback/timer/state objects не имеют возможности обратиться к новой VM resource с тем же именем.
- [ ] Callback identity привязана минимум к тройке: resource identity + generation + Lua reference.
- [ ] При resource stop callback становится invalid и Lua reference освобождается корректно.
- [ ] После resource stop async completion не вызывает Lua, а stale callback не обращается к новой generation.
- [ ] Реализованы ResourceContext, generation, safe callback ownership, safe task ownership и safe timer ownership.
- [ ] Добавлены regression tests на restart ресурса (callback safety, P0).