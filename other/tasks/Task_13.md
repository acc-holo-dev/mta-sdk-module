# Task 13 — Native MTA types

**Источник:** PROMT.md — разделы: 17 — Native MTA types; фазы: PHASE 9 — Native MTA types
**Область задачи:** Задача покрывает high-level обёртки над нативными MTA-сущностями (Element, Player, Vehicle, Object, Resource и др.) с безопасной валидацией.

---

# 17. Native MTA types

Добавить high-level types:

```text
mta::Element
mta::Player
mta::Vehicle
mta::Object
mta::Resource
...
```

Только если соответствующий MTA API доступен и может быть представлен безопасно.

Пример:

```cpp
MTA_FUNCTION("get_name",
    [](mta::Player player)
    {
        return player.name();
    });
```

Type wrappers должны валидировать underlying MTA entity.

Invalid/deleted entities должны обрабатываться безопасно.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 9 — Native MTA types

Добавить safe wrappers where possible.

---

## Чек-лист соответствия проекта

- [ ] Добавлены high-level types: `mta::Element`, `mta::Player`, `mta::Vehicle`, `mta::Object`, `mta::Resource` (и другие по необходимости).
- [ ] Типы добавляются только если соответствующий MTA API доступен и может быть представлен безопасно.
- [ ] Type wrappers валидируют underlying MTA entity.
- [ ] Invalid/deleted entities обрабатываются безопасно.
- [ ] Native-типы применимы как аргументы функций в стиле `MTA_FUNCTION("get_name", [](mta::Player player) { return player.name(); })`.
- [ ] Добавлены safe wrappers where possible (PHASE 9).