# Task 21 — Документация SDK

**Источник:** PROMT.md — разделы: 39 — Documentation; 40 — Documentation quality rule; фазы: PHASE 12 — Documentation
**Область задачи:** Документация SDK — обновление README.md, example.md, api.md, architecture.md и migration-v1-to-v2.md, построение example.md как полного руководства для developer, впервые видящего SDK, и правило качества документации: примеры кода вместо общих фраз.

---

# 39. Documentation

Обновить:

```text
README.md
other/documents/example.md
other/documents/api.md
other/documents/architecture.md
```

## `example.md` особенно важен.

Он должен быть написан для developer, который никогда не видел внутренности SDK.

Обязательно показать полностью:

### Basic function

```cpp
MTA_FUNCTION(...)
```

### Multiple arguments

### Optional arguments

### Variadic arguments

### Return values

### Multiple return values

### Tables

### Callbacks

### Async

### Timers

### Errors

### Resource state

### Objects

### Native MTA types

### Library usage

### Testing

### Creating a new function

### Creating a new object

### Building

### Testing

### Documentation generation

### Doctor

Каждый раздел должен содержать:

```text
C++ code
↓
Lua usage
↓
expected result
↓
failure example where relevant
```

# 40. Documentation quality rule

Нельзя писать:

> "The SDK supports optional arguments."

Нужно писать:

```cpp
MTA_FUNCTION("foo",
    [](std::string name,
       std::optional<int> value)
    {
        ...
    });
```

Lua:

```lua
foo("test")
foo("test", 10)
```

и объяснить, что произойдёт при:

```lua
foo()
foo("test", "invalid")
```

Документация должна отвечать на вопрос:

> "Я впервые вижу SDK. Как мне этим воспользоваться?"

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 12 — Documentation

Полностью написать:

```text
example.md
api.md
architecture.md
migration-v1-to-v2.md
```

---

## Чек-лист соответствия проекта

- [ ] Существуют и обновлены файлы `README.md`, `other/documents/example.md`, `other/documents/api.md`, `other/documents/architecture.md`.
- [ ] Существует полностью написанный документ `other/documents/migration-v1-to-v2.md`.
- [ ] В `example.md` присутствуют разделы: Basic function, Multiple arguments, Optional arguments, Variadic arguments, Return values, Multiple return values, Tables.
- [ ] В `example.md` присутствуют разделы: Callbacks, Async, Timers, Errors, Resource state, Objects, Native MTA types, Library usage.
- [ ] В `example.md` присутствуют разделы: Testing, Creating a new function, Creating a new object, Building, Documentation generation, Doctor.
- [ ] Раздел Basic function в `example.md` содержит код с `MTA_FUNCTION(...)`.
- [ ] Каждый раздел `example.md` построен по схеме: C++ code → Lua usage → expected result → failure example where relevant.
- [ ] В документации отсутствуют общие утверждения вида "The SDK supports optional arguments." без примеров кода: требования объясняются примерами C++ и Lua, включая поведение при ошибках (`foo()`, `foo("test", "invalid")`).
- [ ] `example.md` написан для developer, который никогда не видел внутренности SDK, и отвечает на вопрос «Я впервые вижу SDK. Как мне этим воспользоваться?».