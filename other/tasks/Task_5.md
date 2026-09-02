# Task 5 — Конфигурация module.toml

**Источник:** PROMT.md — разделы: 4 — `module.toml`; фазы: PHASE 2 — Configuration
**Область задачи:** Вводит `config/module.toml` как единственный основной файл конфигурации проекта, читаемый CMake, и устраняет дублирование module identity (PHASE 2).

---

# 4. `module.toml`

Использовать:

```text
config/module.toml
```

как единственный основной project configuration file.

Не дублировать module identity по множеству CMake переменных.

Пример:

```toml
[module]
name = "base"
title = "Base Module"
author = "Developer"
version = "2.0.0"

[build]
cxx_standard = 20
unity = true
lto = true

[async]
workers = "auto"
queue = 4096

[features]
async = true
userdata = true
events = true
objects = true
```

Точная схема может быть изменена, если текущие требования MTA требуют других полей.

Главное правило:

> developer должен менять конфигурацию проекта преимущественно в одном месте.

CMake должен читать эту конфигурацию.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 2 — Configuration

Создать:

```text
config/module.toml
```

Подключить configuration parser.

Убрать duplicate module identity sources.

Проверить:

```text
name
title
author
version
```

---

## Чек-лист соответствия проекта

- [ ] Существует файл `config/module.toml`.
- [ ] В `config/module.toml` заданы `name`, `title`, `author`, `version` (проверка по PHASE 2).
- [ ] Подключён configuration parser — CMake читает конфигурацию из `config/module.toml`.
- [ ] Module identity не дублируется по множеству CMake переменных — duplicate module identity sources отсутствуют.
- [ ] `config/module.toml` является единственным основным project configuration file.
- [ ] Схема конфигурации включает секции `[module]`, `[build]`, `[async]`, `[features]` (по примеру раздела 4).