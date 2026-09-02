# Task 6 — Public API, internal API и архитектурные границы

**Источник:** PROMT.md — разделы: 41 — Architectural boundaries; 42 — Что считать internal API; 43 — Public API; фазы: PHASE 3 — SDK public facade
**Область задачи:** Определяет обязательное направление зависимостей между слоями, перечень internal implementation details, скрытых от developer, и umbrella header `<mta/sdk.hpp>` как основной public API (PHASE 3).

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

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 3 — SDK public facade

Создать:

```cpp
<mta/sdk.hpp>
```

Сделать developer API.

Сохранить internal layers.

---

## Чек-лист соответствия проекта

- [ ] Существует umbrella header `<mta/sdk.hpp>` (include вида `#include <mta/sdk.hpp>`).
- [ ] Umbrella header экспортирует наиболее важные public APIs: `MTA_FUNCTION`, `MTA_OBJECT`, `MTA_STATE` и `mta::*` (например, `mta::args`, `mta::async`, `mta::timer`, `mta::log`, `mta::state`, `mta::Player`, `mta::Vehicle`).
- [ ] `#include <mta/sdk.hpp>` используется как основной include в developer code (`source/functions/`).
- [ ] Developer code не подключает внутренние headers напрямую — Registry/Scheduler/Lua stack helpers/ABI glue не используются в `source/functions/`.
- [ ] Направление зависимостей `functions → library → sdk → MTA` соблюдено.
- [ ] Зависимость SDK от `functions` отсутствует.
- [ ] Registry не содержит конкретную бизнес-логику.
- [ ] Runtime не знает деталей конкретной feature.
- [ ] Lua binder не знает HTTP/crypto/database implementation.
- [ ] Internal layers сохранены при создании public facade (PHASE 3).