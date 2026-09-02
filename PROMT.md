# MTA Module SDK V2 — Master Implementation Plan

## 0. Роль агента

Ты — senior/principal C++ engineer, специализирующийся на:

* Multi Theft Auto: San Andreas server modules;
* C++20;
* Lua C API;
* ABI-safe native modules;
* CMake;
* Windows/MSVC/MinGW;
* Linux/GCC/Clang;
* runtime lifecycle;
* multithreading;
* embedded testing;
* developer tooling.

Твоя задача — не просто «переписать проект», а реализовать **MTA Module SDK V2** как production-grade framework.

Работай с существующим репозиторием как с наследуемой системой.

Перед изменением кода обязательно:

1. изучи текущую реализацию;
2. найди существующие API и зависимости;
3. определи, что уже работает и что нельзя сломать;
4. проверь документацию;
5. проверь CMake;
6. проверь tests;
7. проверь module lifecycle;
8. проверь Lua ABI;
9. только после этого вноси изменения.

Не переписывай работающую архитектуру без причины.

---

# 1. Главная цель V2

V2 должна превратить текущий SDK в:

> Developer-first framework для создания native MTA modules на C++.

Главные свойства:

1. Минимум boilerplate.
2. Минимум файлов, которые developer обязан понимать.
3. Простая настройка.
4. Простое создание новых функций.
5. Безопасное resource lifecycle управление.
6. Безопасный async.
7. Удобный Lua binding.
8. Удобная работа с MTA userdata/native types.
9. Простая сборка.
10. Простое тестирование.
11. Хорошая диагностика.
12. Реальный integration testing через MTA server.
13. Автоматическая документация API.
14. Чёткое разделение framework code и developer code.

---

# 2. Критическое правило именования

SDK НЕ ДОЛЖЕН автоматически добавлять свои namespace/prefix к именам функций или модулей.

Неправильно:

```cpp
MTA_FUNCTION("holo.crypto.sha256")
```

если developer хочет назвать функцию:

```cpp
"sha256"
```

SDK должен зарегистрировать именно:

```text
sha256
```

То же самое относится к названию native module.

Имя module полностью определяется developer через `config/module.toml`.

SDK не должен автоматически добавлять:

```text
mta.
sdk.
holo.
module.
```

и другие namespace.

Namespacing может быть указан developer вручную:

```cpp
MTA_FUNCTION("crypto.sha256", ...)
```

но framework не должен изменять это имя.

---

# 3. Целевая структура проекта

Привести repository к следующей архитектуре:

```text
mta-module-sdk/
│
├── config/
│   └── module.toml
│
├── source/
│   │
│   ├── functions/
│   │   ├── base/
│   │   └── ...
│   │
│   ├── library/
│   │   └── base/
│   │
│   └── sdk/
│       ├── abi/
│       ├── lua/
│       ├── bind/
│       ├── runtime/
│       ├── registry/
│       ├── objects/
│       ├── resources/
│       ├── events/
│       └── logging/
│
├── other/
│   │
│   ├── documents/
│   │   ├── example.md
│   │   ├── api.md
│   │   ├── architecture.md
│   │   └── ...
│   │
│   ├── server/
│   │
│   ├── tests/
│   │   ├── unit/
│   │   ├── lua/
│   │   └── integration/
│   │
│   ├── tools/
│   │   └── mta
│   │
│   └── third_party/
│       ├── mta-sdk/
│       └── lua/
│
├── CMakeLists.txt
├── CMakePresets.json
└── README.md
```

## Назначение каталогов

### `source/functions/`

Основной developer code.

Здесь располагаются Lua-exposed functions.

Пример:

```text
source/functions/base/hello.cpp
source/functions/base/math.cpp
source/functions/http/get.cpp
```

Новая `.cpp` должна автоматически попадать в build без необходимости добавлять её вручную в CMake.

---

### `source/library/`

Расширяемая библиотечная функциональность.

Здесь располагается reusable C++ functionality, которая:

* не обязана непосредственно экспортироваться в Lua;
* может использоваться несколькими functions;
* может быть частью framework extensions.

Примеры:

```text
source/library/base/
source/library/http/
source/library/json/
source/library/crypto/
```

`library` не должен зависеть от developer functions.

Правильное направление:

```text
functions
    ↓
library
    ↓
sdk
```

Но:

```text
sdk
    ↓
functions
```

запрещено.

---

### `source/sdk/`

Основной framework.

Developer не должен быть обязан изучать внутренности этого каталога для создания обычной функции.

Внутренняя архитектура:

```text
abi/
lua/
bind/
runtime/
registry/
objects/
resources/
events/
logging/
```

Можно добавлять дополнительные подмодули, если это действительно улучшает separation of concerns.

Не создавай каталоги только ради формальности.

---

### `other/documents/`

Вся developer-facing документация, кроме основного README.

Обязателен:

```text
other/documents/example.md
```

Этот документ должен быть практической энциклопедией SDK.

Не писать документацию абстрактно.

Каждый механизм показывать через реальные code examples.

---

### `other/server/`

Используется для локального реального MTA integration testing.

Здесь НЕ хранить большие бинарные server builds в Git.

Каталог должен содержать инфраструктуру для:

* скачивания server;
* определения версии;
* хранения локального server;
* запуска server;
* установки/копирования built module;
* запуска тестового resource;
* чтения server output;
* завершения server;
* проверки exit code;
* проверки ожидаемого результата.

Server binaries должны быть в `.gitignore`.

---

### `other/tests/`

Три уровня:

```text
unit/
lua/
integration/
```

---

### `other/tools/mta`

CLI tool проекта.

Он должен быть executable entry point для developer tooling.

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

# 5. Module identity

Module name/title/author/version должны задаваться developer.

Не генерировать:

```text
holo_
mta_
sdk_
```

и другие prefixes.

Module identity должна использоваться:

* в CMake;
* в resulting DLL/SO;
* в metadata;
* в diagnostics;
* в packaging;
* в MTA module registration.

Не должно быть нескольких независимых источников истины.

---

# 6. Binder V2

Binder является одной из главных частей SDK.

Не уничтожать существующую typed-binding идею.

Нужно сохранить возможность писать:

```cpp
MTA_FUNCTION("sum", [](double a, double b)
{
    return a + b;
});
```

или аналогичный простой API.

Цель:

```text
Lua arguments
    ↓
automatic type conversion
    ↓
C++ function
    ↓
automatic result conversion
```

Поддержать минимум:

* integer;
* number;
* boolean;
* string;
* table;
* optional;
* variadic/rest arguments;
* callback;
* resource context;
* native MTA objects;
* multiple return values;
* errors.

---

# 7. Argument validation

Developer НЕ должен вручную проверять каждый простой argument.

Например:

```cpp
MTA_FUNCTION("sum",
    [](double a, double b)
    {
        return a + b;
    });
```

SDK должен автоматически:

1. проверить наличие аргументов;
2. проверить количество;
3. проверить тип;
4. конвертировать значение;
5. вызвать C++ function;
6. вернуть Lua result.

Если Lua вызвал:

```lua
sum(10)
```

framework должен выдать понятную Lua error.

Например:

```text
bad argument #2 to 'sum'
expected number, got nil
```

Если:

```lua
sum("hello", 10)
```

то:

```text
bad argument #1 to 'sum'
expected number, got string
```

Если требуется количество:

```lua
bad argument count to 'sum'
expected 2 arguments, got 1
```

Сообщения должны быть:

* короткими;
* понятными;
* диагностичными;
* пригодными для production logs.

---

# 8. Обязательная документация аргументов

`other/documents/example.md` должен подробно объяснить developer:

* как объявляется function;
* как понять какие аргументы принимает function;
* какие типы поддерживаются;
* как работают optional arguments;
* как работают variadic arguments;
* как возвращаются результаты;
* как генерируется error;
* как писать manual validation;
* когда automatic binding недостаточен.

Обязательно дать examples:

```cpp
MTA_FUNCTION("sum",
    [](double a, double b)
    {
        return a + b;
    });
```

и Lua:

```lua
sum(10, 20)
```

затем:

```lua
sum(10)
```

результат:

```text
argument #2 is missing
```

затем:

```lua
sum("10", 20)
```

результат:

```text
argument #1 has invalid type
```

---

# 9. Signature metadata

Каждая зарегистрированная function должна по возможности иметь metadata:

```text
name
description
arguments
return values
flags
category
```

Например:

```text
name:
    sum

description:
    Adds two numbers.

arguments:
    #1 number
    #2 number

returns:
    #1 number
```

Эта metadata должна использоваться для:

* diagnostics;
* `mta docs`;
* API inspection;
* future tooling.

Не заставлять developer описывать metadata дважды, если её можно вывести из C++ signature.

---

# 10. `mta docs`

Реализовать:

```bash
mta docs
```

Команда должна анализировать зарегистрированные functions/object APIs и генерировать developer documentation.

Результат должен быть читаемым markdown.

Например:

```text
generated/
    api.md
```

или другое логичное место.

Документация должна включать:

* function name;
* description;
* argument types;
* optional arguments;
* return types;
* object methods;
* errors/flags, если они доступны.

Если какую-либо информацию невозможно вывести автоматически — это должно быть явно отражено в архитектуре и документации.

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

---

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

# 13. Scheduler V2

Текущий Scheduler сохранить как internal implementation.

Developer-facing API должен быть выше уровнем.

Например:

```cpp
auto task = mta::async::run(...);
```

и:

```cpp
task.cancel();
task.done();
task.valid();
```

Поддержать:

* background work;
* main-thread completion;
* cancellation;
* task state;
* safe shutdown;
* resource ownership;
* queue limits.

Lua никогда не должен вызываться напрямую из worker thread.

---

# 14. Async resource lifecycle

Каждая async operation должна иметь понятный owner.

Если resource остановлен:

```text
resource stopped
    ↓
owned tasks
    ↓
invalidate/cancel completion
    ↓
NO Lua access
```

Нельзя допустить:

```text
old task
    ↓
resource restarted
    ↓
new VM
    ↓
old task calls new VM
```

Добавить отдельные tests.

---

# 15. Timers V2

Создать простой public API:

```cpp
auto timer = mta::timer::after(5000, [] {
    ...
});
```

и:

```cpp
auto timer = mta::timer::every(1000, [] {
    ...
});
```

Handle должен позволять:

```cpp
timer.cancel();
timer.valid();
```

Timers должны быть resource-aware.

При stop resource:

```text
all owned timers
    ↓
invalidated/cancelled
```

Не должно быть stale execution после restart.

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

---

# 19. Errors

Создать единый error model.

Минимум:

```text
InvalidArgument
InvalidType
MissingArgument
ResourceStopped
InvalidCallback
InvalidObject
AsyncCancelled
InternalError
```

Binder должен преобразовывать ошибки в понятные Lua errors.

Пример:

```text
bad argument #1 to 'foo':
expected number, got string
```

А internal error должен быть различим от user input error.

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

---

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

---

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

---

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

---

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

---

# 27. `mta new object`

Аналогично:

```bash
mta new object counter
```

создаёт skeleton native object.

---

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

---

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

# 30. Real MTA Server integration

`other/server/` является обязательной частью test infrastructure.

CLI должен уметь:

```bash
mta server install
mta server update
mta server version
mta server start
mta server stop
```

Server binaries НЕ коммитить в Git.

Не использовать blindly `latest`.

При установке фиксировать:

```text
platform
architecture
MTA branch
build/revision
download URL
checksum
```

Для nightly использовать конкретную выбранную сборку.

Nightly infrastructure MTA публикует отдельные Windows x64 и Linux x64 server builds; nightly являются development/unstable builds, поэтому test infrastructure должна хранить точную revision/build identity.

---

# 31. Server test harness

Integration harness должен уметь:

1. установить server;
2. подготовить temporary server directory;
3. установить module DLL/SO;
4. создать test resource;
5. создать необходимую configuration;
6. запустить server;
7. дождаться server ready state;
8. вызвать Lua integration tests;
9. собрать stdout/stderr/log;
10. проверить expected output;
11. корректно остановить server;
12. вернуть exit code;
13. очистить temporary files.

Tests должны быть deterministic.

Не использовать developer's global MTA installation.

---

# 32. Integration test scenarios

Добавить реальные tests минимум для:

```text
module load
module unload
resource start
resource stop
resource restart
function registration
argument validation
return values
callback
timer
async task
async completion
userdata
userdata invalidation
multiple resources
old callback after restart
old async task after restart
multiple timers after restart
shutdown with active workers
```

---

# 33. Самый важный regression test

Обязательно:

```text
Resource A generation 1
    ↓
create callback
    ↓
start async task
    ↓
stop resource
    ↓
restart same resource
    ↓
Resource A generation 2
```

Старые objects/tasks/callbacks должны быть полностью неспособны обратиться к generation 2.

Это должен быть автоматический regression test.

---

# 34. Third-party dependencies

Сохранить reproducible vendor approach:

```text
other/third_party/mta-sdk
other/third_party/lua
```

Не использовать системную Lua installation для module ABI, если это нарушает MTA ABI requirements.

Проверять соответствие:

```text
Lua headers
+
compiled Lua source
```

Так, чтобы случайная несовместимая Lua implementation не могла попасть в build.

---

# 35. Build system

Сохранить:

* CMake;
* CMake Presets;
* automatic source discovery;
* MSVC;
* MinGW;
* Linux;
* tests;
* LTO;
* unity build при необходимости.

Но сделать user-facing workflow:

```bash
mta build
```

Developer не должен знать детали CMake command line.

Advanced users всё ещё должны иметь возможность использовать CMake напрямую.

---

# 36. Release policy

GitHub Releases НЕ использовать как distribution system полноценного SDK.

Для release artifact оставлять только:

```text
base.dll
```

для Windows и:

```text
base.so
```

для Linux.

То есть release нужен как:

> proof-of-build / proof-of-validity.

Название файла должно соответствовать developer-defined module name.

Пример:

```text
my_module.dll
my_module.so
```

Не добавлять:

```text
sdk_
mta_
holo_
```

автоматически.

---

# 37. Release validation

Release pipeline должен:

1. build Windows MSVC;
2. build Windows MinGW;
3. build Linux;
4. run tests;
5. run relevant integration tests;
6. verify resulting DLL/SO exists;
7. publish only binary artifact(s).

Если artifact опубликован — pipeline должен гарантировать, что соответствующий build/test job успешно завершился.

---

# 38. Compatibility

Разделять:

```text
SDK version
Module version
ABI version
MTA compatibility version
```

Не смешивать эти понятия.

При необходимости в binary metadata или diagnostics указывать их отдельно.

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

---

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

# 41. Architectural boundaries

Обязательное направление зависимостей:

```text
functions
    ↓
library
    ↓
sdk
    ↓
MTA
```

SDK не должен зависеть от:

```text
functions
```

Registry не должен содержать конкретную бизнес-логику.

Runtime не должен знать детали конкретной feature.

Lua binder не должен знать HTTP/crypto/database implementation.

---

# 42. Что считать internal API

Следующие вещи являются internal implementation details:

```text
Registry internals
Scheduler internals
Lua reference bookkeeping
Resource Hub
Lua stack helpers
ABI glue
CMake implementation
thread queues
worker internals
```

Developer должен использовать public facade:

```cpp
<mta/sdk.hpp>
```

Цель:

```cpp
#include <mta/sdk.hpp>
```

как основной include.

Не заставлять developer подключать 10 внутренних headers.

---

# 43. Public API

Создать umbrella header:

```cpp
#include <mta/sdk.hpp>
```

Он должен экспортировать наиболее важные public APIs.

Например:

```cpp
MTA_FUNCTION
MTA_OBJECT
MTA_STATE

mta::args
mta::async
mta::timer
mta::log
mta::state
mta::Player
mta::Vehicle
...
```

---

# 44. Performance

Не оптимизировать «на глаз».

Создать benchmarks для:

```text
function call
argument conversion
table conversion
callback
timer scheduling
async scheduling
userdata creation
userdata access
```

Оптимизировать только после измерений.

Особое внимание:

* allocations;
* Lua stack operations;
* table snapshots;
* callback bookkeeping;
* task queue.

---

# 45. Lua table model

Предусмотреть:

```text
LuaView
Snapshot
```

Не разрешать worker threads обращаться к borrowed Lua values.

Async автоматически должен работать только с thread-safe owned values.

---

# 46. Source code quality

Требования:

* C++20;
* RAII;
* no unnecessary global mutable state;
* explicit ownership;
* no raw ownership;
* thread safety documented;
* lifetime documented;
* no hidden Lua cross-thread access;
* no undefined behavior;
* no compiler-specific tricks for public identity;
* warnings enabled;
* warnings treated as errors where practical.

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

---

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

# 49. Implementation phases

Работать строго фазами.

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

## PHASE 1 — Repository restructure

Перенести:

```text
src → source
tests → other/tests
tools → other/tools
docs → other/documents
vendor → other/third_party
```

Создать:

```text
source/library
```

Создать:

```text
other/server
```

Не менять behavior SDK больше необходимого для restructure.

Проверка:

```bash
mta build
mta test
```

must remain green.

---

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

## PHASE 3 — SDK public facade

Создать:

```cpp
<mta/sdk.hpp>
```

Сделать developer API.

Сохранить internal layers.

---

## PHASE 4 — Binder V2

Рефакторинг:

```text
argument conversion
result conversion
traits
invoke
errors
signature metadata
```

Не менять behavior без regression tests.

---

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

## PHASE 6 — Async V2

Добавить:

```text
task handle
cancel
state
resource ownership
safe shutdown
```

---

## PHASE 7 — Timer V2

Добавить:

```text
after()
every()
handle.cancel()
```

---

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

## PHASE 9 — Native MTA types

Добавить safe wrappers where possible.

---

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

## PHASE 11 — Real server harness

Реализовать:

```text
server download
server pinning
server install
server start
server stop
integration execution
logs
cleanup
```

---

## PHASE 12 — Documentation

Полностью написать:

```text
example.md
api.md
architecture.md
migration-v1-to-v2.md
```

---

## PHASE 13 — CI

CI должен выполнять:

```text
build
unit
lua
integration
doctor
```

где environment позволяет.

Matrix минимум:

```text
Windows MSVC
Windows MinGW
Linux GCC
```

При наличии разумной инфраструктуры добавить Clang.

---

## PHASE 14 — Release

Release pipeline:

```text
build
test
integration
package
publish DLL/SO
```

Release artifacts:

```text
<module-name>.dll
<module-name>.so
```

---

# 50. Definition of Done

V2 считается завершённой только если одновременно выполняются все условия.

## Architecture

```text
source/functions
source/library
source/sdk
other/tests
other/server
other/tools
other/documents
other/third_party
```

разделены.

---

## Developer workflow

Новый developer может:

```bash
mta init test-module
mta new function hello
mta build
mta test
```

без ручного редактирования CMake registry files.

---

## Function creation

Developer пишет одну `.cpp`.

Не обязан:

* добавлять function в registry;
* менять CMake;
* регистрировать function ещё где-то.

---

## Arguments

Binder автоматически обнаруживает:

* count;
* type;
* optional;
* variadic;
* callback.

Ошибки понятные.

---

## Lifecycle

После resource restart:

```text
old callback = invalid
old task completion = invalid
old timer = invalid
old state = cleaned
```

И не может попасть в новую resource generation.

---

## Async

Worker threads никогда не вызывают Lua напрямую.

---

## Userdata

Stable identity.

No `typeid(T).name()` as public identity.

---

## Documentation

`example.md` позволяет новому developer самостоятельно создать:

* function;
* async function;
* timer;
* callback;
* object;
* state;
* error handling.

---

## Doctor

```bash
mta doctor
```

реально проверяет environment.

---

## Server

```bash
mta test integration
```

может поднять controlled MTA server и проверить реальный module load/function execution.

---

## Release

GitHub Release публикует только:

```text
<module>.dll
<module>.so
```

после успешного build/test pipeline.

---

# 51. Rules for the coding agent

Не делать следующие вещи:

1. Не добавлять автоматически prefixes к function names.
2. Не добавлять автоматически prefixes к module name.
3. Не ломать public API без явной необходимости.
4. Не использовать raw Lua state между потоками.
5. Не обращаться к новой Resource generation из старого callback/task.
6. Не хранить server binaries в Git.
7. Не зависеть от developer's globally installed MTA server для tests.
8. Не использовать `typeid(T).name()` как stable public identity.
9. Не заставлять developer вручную регистрировать каждую function.
10. Не дублировать configuration.
11. Не создавать abstraction только ради abstraction.
12. Не усложнять API, если задачу можно решить проще.
13. Не оставлять TODO вместо implementation.
14. Не считать compile success достаточным для lifecycle changes.
15. Не завершать phase без соответствующих tests.

---

# 52. Работа агента по каждому изменению

Для каждого substantial change агент должен:

1. понять existing implementation;
2. изменить минимально необходимое количество компонентов;
3. написать/обновить tests;
4. собрать project;
5. запустить tests;
6. проверить integration, если затронут lifecycle/ABI/Lua;
7. обновить documentation;
8. проверить, что архитектурные границы не нарушены.

После каждой phase создавать краткий внутренний отчёт:

```text
PHASE:
CHANGED:
ADDED:
REMOVED:
TESTS:
RISKS:
NEXT:
```

---

# 53. Приоритет инженерных решений

Если есть конфликт между:

```text
simplicity
performance
backward compatibility
architecture purity
```

приоритет по умолчанию:

```text
1. correctness
2. lifecycle safety
3. ABI safety
4. developer usability
5. maintainability
6. performance
7. abstraction purity
```

Но performance regressions нельзя игнорировать.

---

# 54. Главная UX-цель

Финальный developer experience должен быть таким:

```text
Create function
      ↓
one .cpp
      ↓
MTA_FUNCTION(...)
      ↓
mta build
      ↓
DLL/SO
```

и:

```text
Need help?
      ↓
mta docs
mta doctor
mta test
```

Developer должен заниматься:

```text
function logic
library logic
business logic
```

а не:

```text
Lua registry references
module lifecycle glue
CMake source lists
thread queues
Lua stack bookkeeping
MTA callback invalidation
```

---

# 55. Final architectural target

Целевая модель:

```text
                     Developer
                         │
                         ▼
                  <mta/sdk.hpp>
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
      Functions       Objects        Services
          │              │              │
          └──────────────┼──────────────┘
                         ▼
                    Framework
                         │
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
       Bind            Runtime           ABI
        │                │                │
        └────────────────┼────────────────┘
                         ▼
                         Lua
                         │
                         ▼
                        MTA
```

---

# 56. Final success criterion

V2 должна ощущаться не как:

> "Новая версия старого MTA module template."

Она должна ощущаться как:

> "Я установил C++ framework для MTA и могу писать native functionality, не думая о внутренностях module runtime."

Главная метрика V2:

```text
time from empty project
to first working Lua-exposed C++ function
```

должно быть минимальным.

Внутренняя сложность framework допустима.

Developer-facing сложность — нет.
