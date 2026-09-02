# Task 17 — MTA CLI

**Источник:** PROMT.md — разделы: 24 — MTA CLI; 25 — `mta init`; 26 — `mta new function`; 27 — `mta new object`; 28 — `mta doctor`; 29 — `mta test`; фазы: PHASE 10 — CLI
**Область задачи:** Задача покрывает CLI-инструмент `mta` и требования к его командам — init, build, test, docs, doctor, package, server, new function, new object — включая поведение каждой команды.

---

# 24. MTA CLI

Создать:

```bash
mta
```

с командами:

```text
mta init
mta build
mta test
mta docs
mta doctor
mta package
mta server
mta new function
mta new object
```

Можно добавить aliases, если это улучшает UX.

# 25. `mta init`

Создаёт новый module project.

Пример:

```bash
mta init my-module
```

Должно появиться:

```text
my-module/
    config/module.toml
    source/functions/
    source/library/
    source/sdk/
    other/
    CMakeLists.txt
    CMakePresets.json
    README.md
```

Если `init` запускается внутри существующего проекта, команда не должна разрушать существующие файлы.

# 26. `mta new function`

Команда:

```bash
mta new function hello
```

должна создать минимальный compile-ready function.

Например:

```cpp
#include <mta/sdk.hpp>

MTA_FUNCTION("hello",
    [](std::string name)
    {
        return "Hello, " + name;
    });
```

Имя `"hello"` не модифицируется.

Если developer передал:

```bash
mta new function crypto.sha256
```

зарегистрировать:

```text
crypto.sha256
```

без изменения.

# 27. `mta new object`

Аналогично:

```bash
mta new object counter
```

создаёт skeleton native object.

# 28. `mta doctor`

Реализовать:

```bash
mta doctor
```

Цель:

дать полную картину состояния development environment.

Проверять минимум:

```text
Project
Module configuration
SDK version
Compiler
Compiler version
Architecture
CMake
Ninja/MSBuild where applicable
Lua headers
MTA SDK headers
Lua ABI compatibility
C++ standard
Build system
Server test environment
Server version/build
Output/build directories
Git state where useful
```

Пример:

```text
MTA Module SDK Doctor
────────────────────────────

Project:
  name: base
  version: 2.0.0

Compiler:
  MSVC 2026
  x64
  C++20
  OK

CMake:
  4.x
  OK

Lua ABI:
  bundled headers      OK
  compiled source      OK
  compatibility        OK

MTA Server:
  detected             OK
  build                26837
  architecture         x64

Build:
  configuration        OK
  source discovery     OK

Tests:
  unit                  OK
  lua                   OK
  integration           NOT RUN

Status:
  READY
```

Команда не должна просто печатать случайный набор version strings.

Она должна реально проверять работоспособность.

# 29. `mta test`

Выполняет:

```text
unit tests
+
Lua tests
+
integration tests
```

с понятным разделением.

Поддержать:

```bash
mta test
mta test unit
mta test lua
mta test integration
```

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 10 — CLI

Реализовать:

```bash
mta init
mta build
mta test
mta docs
mta doctor
mta package
mta server
mta new function
mta new object
```

---

## Чек-лист соответствия проекта

- [ ] Существует CLI `mta` с командами: init, build, test, docs, doctor, package, server, new function, new object.
- [ ] `mta init` создаёт структуру module project: config/module.toml, source/functions/, source/library/, source/sdk/, other/, CMakeLists.txt, CMakePresets.json, README.md.
- [ ] `mta init`, запущенный внутри существующего проекта, не разрушает существующие файлы.
- [ ] `mta new function` создаёт минимальный compile-ready function (с `#include <mta/sdk.hpp>` и MTA_FUNCTION).
- [ ] Имя, переданное в `mta new function` (например `crypto.sha256`), регистрируется без модификации.
- [ ] `mta new object` создаёт skeleton native object.
- [ ] `mta doctor` проверяет минимум: Project, Module configuration, SDK version, Compiler, Compiler version, Architecture, CMake, Ninja/MSBuild where applicable, Lua headers, MTA SDK headers, Lua ABI compatibility, C++ standard, Build system, Server test environment, Server version/build, Output/build directories, Git state where useful.
- [ ] `mta doctor` реально проверяет работоспособность, а не просто печатает случайный набор version strings.
- [ ] `mta test` выполняет unit tests, Lua tests и integration tests с понятным разделением.
- [ ] Поддерживаются вызовы `mta test`, `mta test unit`, `mta test lua`, `mta test integration`.