# Problem 12 — Отсутствуют требуемые benchmarks производительности SDK

**Задача:** `other/tasks/Task_23.md` — Performance и benchmarks SDK
**Вердикт:** Проект не соответствует задаче (статическая проверка, 2026-09-02)
**Серьёзность:** P1 — значимые нереализованные требования: из восьми требуемых benchmark-областей реализована только одна (5 NOT_MET, 2 PARTIAL), но нарушения не затрагивают критические требования безопасности/корректности.

## Суть проблемы
Из восьми требуемых benchmark'ов в проекте существует только один — `other/tests/lua/scripts/090_benchmark.lua` (function call throughput), интегрированный в Lua-харнесс и CMake. Benchmarks для argument conversion, table conversion, callback, timer/async scheduling и userdata creation/access отсутствуют; смежные области покрыты лишь функциональными тестами (020_tables, 030_async, 038_timer, 080_stress, 060_features) без измерений. Правило «оптимизировать только после измерений» не зафиксировано в документации/конфигурации проекта, записанные результаты измерений отсутствуют, а отдельный перф-анализ allocations, Lua stack operations, table snapshots, callback bookkeeping и task queue нигде не выполнен. Вердикт аудита: NON_COMPLIANT (2 PARTIAL, 5 NOT_MET).

## Несоответствия

### 1. Benchmarks для function call и argument conversion
- **Статус:** PARTIAL
- **Требуется:** Benchmarks для function call И argument conversion как отдельные измерения производительности.
- **Фактически:** Измеряется только сквозной вызов функции модуля (function call); конвертация аргументов (`source/sdk/lua/argument.cpp`, `arguments.cpp`) никак не измеряется отдельно.
- **Доказательство:** `other/tests/lua/scripts/090_benchmark.lua:1-11` — единственный benchmark: «module call throughput», 200000 вызовов sample_add, os.clock, печать calls/s. Benchmark для argument conversion как отдельного измерения отсутствует: grep 'benchmark' по всему проекту даёт только этот файл (плюс упоминания в docs/таске); в `source/` и `other/tools` совпадений нет.

### 2. Benchmarks для table conversion и callback
- **Статус:** NOT_MET
- **Требуется:** Benchmarks для table conversion и callback.
- **Фактически:** Ни одного benchmark'а для конвертации таблиц (Table snapshot, `source/sdk/lua/argument.hpp:24`) и для callback (`source/sdk/runtime/callback.cpp`) не существует.
- **Доказательство:** Файлов benchmark с измерениями времени нет: единственный os.clock/chrono-замер — `other/tests/lua/scripts/090_benchmark.lua` (function call). `other/tests/lua/scripts/020_tables.lua` и `030_async.lua` — функциональные тесты с test_assert, без замеров.

### 3. Benchmarks для timer scheduling и async scheduling
- **Статус:** NOT_MET
- **Требуется:** Benchmarks планирования таймеров и async-задач (`source/sdk/runtime/timer.cpp`, `scheduler.cpp`).
- **Фактически:** Существуют только функциональный стресс-тест и тесты корректности; измерений производительности планировщиков нет.
- **Доказательство:** `other/tests/lua/scripts/080_stress.lua:1-11` — 'Scheduler stress test: 1000 async tasks' с test_assert(count == 1000), без каких-либо измерений времени; `038_timer.lua`/`035_task.lua` — функциональные тесты. Grep os.clock|chrono|steady_clock по `other/tests`: единственный осмысленный замер — `090_benchmark.lua`.

### 4. Benchmarks для userdata creation и userdata access
- **Статус:** NOT_MET
- **Требуется:** Benchmarks создания и доступа к userdata.
- **Фактически:** Их нет ни в Lua-скриптах, ни на C++ уровне.
- **Доказательство:** Glob '{source,other/tools,other/tests,cmake,config,docs}/**/*{bench,perf}*' находит только `090_benchmark.lua`; userdata-код `source/sdk/objects/userdata.hpp` и `functions/objects/counter.cpp` покрыт только функциональным тестом `060_features.lua` без замеров.

### 5. Оптимизации выполняются только после измерений на benchmarks, а не «на глаз»
- **Статус:** PARTIAL
- **Требуется:** Процессная норма «оптимизировать только после измерений», подтверждённая артефактами (документированная политика, recorded measurements, полноценная benchmark-инфраструктура).
- **Фактически:** Минимальный informational-benchmark есть, но документированной политики и зафиксированных результатов измерений нет, поэтому соответствие процесса статически не подтверждается.
- **Доказательство:** Правило существует только в `PROMT.md:1705-1707` и `other/tasks/Task_23.md` — в самом проекте не зафиксировано: grep 'performance|benchmark|оптимиз|измерен' по `README.md`, `CHANGELOG.md`, `CONTRIBUTING.md` и `other/documents/*.md` — 0 совпадений; в `.github/workflows` нет benchmark-этапа; в `CMakeLists.txt` нет benchmark-цели; база измерений покрывает 1 из 8 требуемых областей; записанных baseline-результатов измерений в репозитории нет.

### 6. При оптимизациях отдельно рассмотрены allocations и Lua stack operations
- **Статус:** NOT_MET
- **Требуется:** Отдельное рассмотрение allocations и Lua stack operations при оптимизациях.
- **Фактически:** Перф-анализ этих аспектов в проекте отсутствует полностью (оптимизационных материалов нет вовсе).
- **Доказательство:** Grep 'allocation' по `other/documents` — 0 совпадений; по `source/` совпадения только функциональные (`source/sdk/runtime/scheduler.cpp:369` 'task queue is full' — ошибка лимита, не перф-анализ). Никаких оптимизационных заметок/разделов в документации нет (`other/documents`: architecture.md, api.md, GUIDES.md, TUTORIAL.md, v2-phase-reports.md, v2-audit.md — ни слова о performance/allocation).

### 7. При оптимизациях отдельно рассмотрены table snapshots, callback bookkeeping и task queue
- **Статус:** NOT_MET
- **Требуется:** Отдельное рассмотрение стоимости table snapshots, callback bookkeeping и task queue в контексте оптимизаций.
- **Фактически:** Существует только функциональная семантика этих механизмов; анализ производительности/оптимизационные материалы отсутствуют.
- **Доказательство:** Упоминания в коде функциональны, а не перф: `source/sdk/runtime/scheduler.cpp:293` 'snapshot BEFORE any callback runs' (корректность), `source/sdk/resources/resources.hpp:75` 'Clears the generation bookkeeping' (жизненный цикл), `source/sdk/lua/argument.hpp:24` 'Table snapshot: the integer sequence part…' (описание формата). Grep 'perf|Performance|оптимиз|измерен' по `other/documents/v2-phase-reports.md` — 0 совпадений.

## Что уже соответствует
- Файл задачи `other/tasks/Task_23.md` прочитан полностью; источник PROMT.md §44 соответствует формулировкам задачи.
- Существует benchmark function call throughput: `other/tests/lua/scripts/090_benchmark.lua` (informational, 200000 вызовов sample_add, вывод calls/s).
- Benchmark интегрирован в тестовую инфраструктуру: `other/tests/lua/harness.cpp` запускает все scripts/*.lua включая 090, CMake-цель sdk_tests (`CMakeLists.txt:249-264`).
- Наличие и назначение benchmark задокументированы: `other/documents/v2-audit.md:109` ('090_benchmark | informational throughput'), `other/documents/architecture.md:299`.

## Рекомендации по устранению
1. Расширить benchmark-покрытие до всех восьми требуемых областей, используя существующую инфраструктуру (`other/tests/lua/harness.cpp` запускает все scripts/*.lua, CMake-цель sdk_tests — `CMakeLists.txt:249-264`): добавить в `other/tests/lua/scripts/` отдельные benchmarks помимо `090_benchmark.lua`.
2. Добавить отдельный benchmark argument conversion (`source/sdk/lua/argument.cpp`, `arguments.cpp`), отделив его от сквозного замера function call в `090_benchmark.lua`.
3. Добавить benchmarks table conversion (Table snapshot, `source/sdk/lua/argument.hpp:24`) и callback (`source/sdk/runtime/callback.cpp`) — сейчас эти области покрыты только функциональными тестами `020_tables.lua` и `030_async.lua` без замеров.
4. Добавить benchmarks timer scheduling и async scheduling (`source/sdk/runtime/timer.cpp`, `scheduler.cpp`), дополнив функциональный стресс-тест `080_stress.lua` (1000 async tasks без измерений времени) замерами производительности планировщиков.
5. Добавить benchmarks userdata creation и userdata access (`source/sdk/objects/userdata.hpp`, `functions/objects/counter.cpp`) на уровне Lua-скриптов и/или C++ — сейчас есть только функциональный тест `060_features.lua` без замеров.
6. Зафиксировать процессную норму «оптимизировать только после измерений» в документации проекта (`README.md`/`CONTRIBUTING.md`/`other/documents/*.md`), поскольку сейчас она есть только в `PROMT.md:1705-1707` и `other/tasks/Task_23.md`; добавить benchmark-этап в `.github/workflows` и отдельную benchmark-цель в `CMakeLists.txt`.
7. Записать baseline-результаты измерений в репозиторий (записанных результатов сейчас нет), чтобы соответствие процессной норме подтверждалось артефактами.
8. Добавить в документацию отдельный перф-анализ: (а) allocations и Lua stack operations — grep 'allocation' по `other/documents` даёт 0 совпадений; (б) стоимость table snapshots, callback bookkeeping и task queue — сейчас в коде и документах есть только функциональные описания этих механизмов (`scheduler.cpp:293`, `resources.hpp:75`, `argument.hpp:24`), без анализа производительности.