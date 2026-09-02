# Task 12 — Userdata/Objects V2

**Источник:** PROMT.md — разделы: 16 — Userdata V2; фазы: PHASE 8 — Object/Userdata V2
**Область задачи:** Задача покрывает stable явные идентификаторы userdata-типов, регистрацию object'ов с методами и их ownership/lifecycle в рамках ресурса.

---

# 16. Userdata V2

Сделать stable explicit userdata type identifiers.

Не использовать `typeid(T).name()` как основной public identity.

Developer API:

```cpp
MTA_OBJECT("counter", Counter)
```

или эквивалентный механизм.

Metatable identity должна быть:

* deterministic;
* stable;
* compiler-independent;
* module-aware where necessary.

Поддержать:

* constructor/create;
* methods;
* type validation;
* invalid object handling;
* ownership/lifetime;
* resource lifecycle.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 8 — Object/Userdata V2

Добавить:

```text
stable type IDs
object registration
methods
validation
resource lifecycle
```

---

## Чек-лист соответствия проекта

- [ ] Введены stable explicit userdata type identifiers.
- [ ] `typeid(T).name()` не используется как основной public identity userdata-типов.
- [ ] Предусмотрен developer API вида `MTA_OBJECT("counter", Counter)` или эквивалентный механизм регистрации object'ов.
- [ ] Metatable identity является deterministic, stable, compiler-independent и module-aware where necessary.
- [ ] Поддержаны constructor/create и methods для object'ов.
- [ ] Реализованы type validation и invalid object handling.
- [ ] Поддержаны ownership/lifetime и resource lifecycle userdata-объектов.
- [ ] Добавлены stable type IDs, object registration, methods, validation и resource lifecycle (PHASE 8).