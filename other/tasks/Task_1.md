# Task 1 — Роль агента, цель V2 и правила кодинг-агента

**Источник:** PROMT.md — разделы: 0 — Роль агента; 1 — Главная цель V2; 51 — Rules for the coding agent; 52 — Работа агента по каждому изменению; 53 — Приоритет инженерных решений; 54 — Главная UX-цель; 55 — Final architectural target; 56 — Final success criterion
**Область задачи:** Задаёт роль агента и главную цель V2 (developer-first framework), обязательные правила кодинг-агента, порядок работы по каждому изменению, приоритет инженерных решений, UX-цель, целевую архитектуру и финальный критерий успеха.

---

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

---

## Чек-лист соответствия проекта

- [ ] Реализован umbrella header `<mta/sdk.hpp>` как основной include для developer (разделы 54, 55).
- [ ] CLI предоставляет команды `mta build`, `mta docs`, `mta doctor`, `mta test` (раздел 54).
- [ ] Отсутствует код, автоматически добавляющий prefixes к именам функций или к имени модуля (раздел 51, пункты 1–2).
- [ ] Не используется `typeid(T).name()` как stable public identity (раздел 51, пункт 8).
- [ ] Не используется raw Lua state между потоками (раздел 51, пункт 4).
- [ ] Отсутствуют TODO вместо implementation в исходном коде SDK (раздел 51, пункт 13).
- [ ] Server binaries не хранятся в Git (раздел 51, пункт 6).
- [ ] Конфигурация проекта не дублируется — основной источник конфигурации один (раздел 51, пункт 10).
- [ ] Для каждой завершённой phase существуют соответствующие tests (раздел 51, пункт 15).