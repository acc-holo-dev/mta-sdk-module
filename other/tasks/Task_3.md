# Task 3 — Целевая структура проекта и restructure

**Источник:** PROMT.md — разделы: 3 — Целевая структура проекта; фазы: PHASE 1 — Repository restructure
**Область задачи:** Определяет целевую структуру repository (config/, source/, other/), назначение каталогов, правила зависимостей и автоматическое включение новых .cpp в build, а также фазу переноса каталогов (PHASE 1).

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
├── README.md
└── LICENSE
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

## Фазы реализации (из раздела 49 PROMT.md)

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

## Чек-лист соответствия проекта

- [ ] Repository приведён к целевой структуре: существуют каталоги `config/`, `source/functions/`, `source/library/`, `source/sdk/` и `other/` с подкаталогами `documents/`, `server/`, `tests/`, `tools/`, `third_party/`.
- [ ] В `source/sdk/` существуют подкаталоги `abi/`, `lua/`, `bind/`, `runtime/`, `registry/`, `objects/`, `resources/`, `events/`, `logging/`.
- [ ] Выполнен перенос: `src → source`, `tests → other/tests`, `tools → other/tools`, `docs → other/documents`, `vendor → other/third_party` (PHASE 1).
- [ ] Созданы каталоги `source/library` и `other/server` (PHASE 1).
- [ ] Новая `.cpp` в `source/functions/` автоматически попадает в build — ручное добавление файла в CMake не требуется.
- [ ] Существует файл `other/documents/example.md`.
- [ ] `other/tests/` содержит три уровня: `unit/`, `lua/`, `integration/`.
- [ ] Существует CLI entry point `other/tools/mta`.
- [ ] Server binaries находятся в `.gitignore` и не хранятся в Git.
- [ ] Направление зависимостей соблюдено: `library` не зависит от developer functions; зависимость `sdk → functions` отсутствует.