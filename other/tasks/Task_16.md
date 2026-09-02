# Task 16 — Registry V2, function discovery и library

**Источник:** PROMT.md — разделы: 21 — Registry V2; 22 — Function discovery; 23 — Library architecture
**Область задачи:** Задача покрывает внутренний Registry V2 с полным descriptor'ом функций, автоматическое обнаружение `.cpp`-файлов в source/functions/** без ручной регистрации и архитектуру каталога source/library/ для reusable code.

---

# 21. Registry V2

Registry остаётся internal mechanism.

Внутренний descriptor должен хранить максимум полезной информации:

```text
name
description
signature
arguments
returns
flags
category
lua entry
```

При этом developer API должен быть простым.

Не требовать ручной регистрации в нескольких местах.

Добавленная функция должна автоматически попадать в registry.

# 22. Function discovery

Система build/registration должна автоматически обнаруживать `.cpp` в:

```text
source/functions/**
```

Developer workflow:

```text
create file
    ↓
write MTA_FUNCTION
    ↓
mta build
    ↓
function available
```

Не требовать ручного изменения:

* registry.cpp;
* CMake source lists;
* central function list.

# 23. Library architecture

`source/library/` предназначен для reusable code.

Example:

```text
source/library/http/client.hpp
source/library/http/client.cpp
```

Function:

```text
source/functions/http/get.cpp
```

использует library:

```text
functions
   ↓
library/http
   ↓
sdk
```

Но library не должна напрямую зависеть от specific function implementation.

---

## Чек-лист соответствия проекта

- [ ] Registry остаётся internal mechanism — ручная регистрация через registry не является частью public developer API.
- [ ] Внутренний descriptor функции хранит name, description, signature, arguments, returns, flags, category и lua entry.
- [ ] Developer API регистрации прост — не требует ручной регистрации в нескольких местах.
- [ ] Добавленная через MTA_FUNCTION функция автоматически попадает в registry.
- [ ] Система build/registration автоматически обнаруживает `.cpp` в `source/functions/**`.
- [ ] Для добавления новой функции не требуется ручное изменение registry.cpp, CMake source lists или central function list.
- [ ] Существует каталог `source/library/` для reusable code, отдельный от `source/functions/`.
- [ ] Library не зависит напрямую от specific function implementation (направление зависимостей: functions → library → sdk).