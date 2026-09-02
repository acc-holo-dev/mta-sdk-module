# Task 15 — Logging

**Источник:** PROMT.md — разделы: 20 — Logging
**Область задачи:** Задача покрывает простой logging API (info/warn/error/debug), требование к контексту log messages (module, function, resource, task, error) и автоматическое добавление контекста фреймворком.

---

# 20. Logging

Сохранить простой API:

```cpp
mta::log::info(...)
mta::log::warn(...)
mta::log::error(...)
mta::log::debug(...)
```

Добавить structured logging API при необходимости.

Log messages должны содержать достаточно context для:

* module;
* function;
* resource;
* task;
* error.

Не заставлять обычного developer явно передавать все эти значения — framework должен добавлять context автоматически там, где возможно.

---

## Чек-лист соответствия проекта

- [ ] Реализован простой logging API с функциями `mta::log::info(...)`, `mta::log::warn(...)`, `mta::log::error(...)`, `mta::log::debug(...)`.
- [ ] Structured logging API добавлен при необходимости, при этом простой API из четырёх функций остаётся основным путём логирования.
- [ ] Log messages содержат достаточно context для module, function, resource, task и error.
- [ ] Framework автоматически добавляет context (module, function, resource, task, error) в log messages там, где возможно.
- [ ] Обычный developer не обязан явно передавать все контекстные значения (module, function, resource, task, error) при каждом вызове логирования.