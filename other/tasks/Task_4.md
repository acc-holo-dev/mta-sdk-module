# Task 4 — Аудит, обратная совместимость и миграция

**Источник:** PROMT.md — разделы: 47 — Обратная совместимость; 48 — Migration strategy; фазы: PHASE 0 — Audit
**Область задачи:** Покрывает правила обратной совместимости при изменении существующего API, документ миграции V1 → V2 и первичный аудит текущей реализации без изменения архитектуры (PHASE 0).

---

# 47. Обратная совместимость

Перед изменением существующего API определить:

```text
KEEP
CHANGE
DEPRECATE
REMOVE
```

Не удалять существующее поведение без причины.

Если V2 intentionally breaking:

* documented;
* tested;
* explained;
* migration examples provided.

# 48. Migration strategy

Создать документ:

```text
other/documents/migration-v1-to-v2.md
```

Он должен показывать:

```text
V1
↓
V2
```

для:

* functions;
* callbacks;
* timers;
* scheduler;
* userdata;
* registry;
* configuration;
* build.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 0 — Audit

Не изменять архитектуру.

Изучить:

* source;
* CMake;
* tests;
* third_party;
* docs;
* CI;
* release;
* current runtime.

Результат:

```text
other/documents/v2-audit.md
```

С описанием:

* current architecture;
* dependencies;
* risks;
* APIs;
* migration concerns.

---

## Чек-лист соответствия проекта

- [ ] Существует файл `other/documents/v2-audit.md` (результат PHASE 0).
- [ ] `v2-audit.md` описывает current architecture, dependencies, risks, APIs и migration concerns.
- [ ] Аудит (PHASE 0) выполнен без изменения архитектуры.
- [ ] Существует файл `other/documents/migration-v1-to-v2.md`.
- [ ] Документ миграции показывает переход V1 → V2 для functions, callbacks, timers, scheduler, userdata, registry, configuration и build.
- [ ] Перед изменением существующих API определена политика KEEP / CHANGE / DEPRECATE / REMOVE.
- [ ] Intentional breaking changes V2 задокументированы, покрыты tests, объяснены и снабжены migration examples; существующее поведение не удаляется без причины.