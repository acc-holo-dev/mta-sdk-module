# Problem 10 — example.md переработан в 8-шаговый walkthrough вместо требуемых ~21 тематических разделов

**Задача:** `other/tasks/Task_21.md` — Документация SDK
**Вердикт:** Проект не соответствует задаче (статическая проверка, 2026-09-02)
**Серьёзность:** P1 — значимые нереализованные требования: ключевой документ example.md не содержит более десятка требуемых тематических разделов и нарушает заявленную схему изложения, хотя остальные документы задачи выполнены.

## Суть проблемы
PHASE 12 выполнена частично: README.md, api.md, architecture.md и migration-v1-to-v2.md переписаны и полны, но ключевой документ example.md переработан в 8-шаговый walkthrough вместо требуемых ~21 тематических разделов §39. Отсутствуют разделы Basic function, Multiple/Optional/Variadic arguments, Return values, Multiple return values, Tables, Errors, Native MTA types, Library usage, Testing, Creating a new function/object, Documentation generation, Doctor. Схема «C++ → Lua → expected result → failure example» выдержана лишь в 2 из 7 содержательных разделов, а §2 ссылается на несуществующий файл source/functions/greeter/greet.cpp.

## Несоответствия

### 1. В example.md присутствуют разделы: Basic function, Multiple arguments, Optional arguments, Variadic arguments, Return values, Multiple return values, Tables
- **Статус:** NOT_MET
- **Требуется:** отдельные разделы с полным показом каждой темы.
- **Фактически:** ни один из 7 заголовков не существует; часть тем встроена внутрь §2 (optional — greet_custom, стр. 80–86; variadic — greet_many, стр. 88–97), но Multiple return values и Tables не показаны вообще.
- **Доказательство:** other/documents/example.md (345 строк) содержит только заголовки: «1. Identity: config/module.toml» (стр. 13), «2. A synchronous function» (47), «3. Background work with a callback» (109), «4. A cancellable timer» (166), «5. An object type» (196), «6. Per-resource state» (247), «7. Events and resources» (274), «8. Build, test, install» (304) + финальная таблица (330). Grep «^#{1,4} » не находит ни одного из 7 требуемых заголовков. Темы «Multiple return values» и «Tables» отсутствуют целиком: нет ни одного примера нескольких результатов (tuple/pair/multi-push) и ни одного примера работы с таблицами (mta::lua::Table упоминается только в карте понятий, стр. 335).

### 2. В example.md присутствуют разделы: Callbacks, Async, Timers, Errors, Resource state, Objects, Native MTA types, Library usage
- **Статус:** PARTIAL
- **Требуется:** 8 именованных разделов.
- **Фактически:** 5 тем покрыты walkthrough-разделами с иными заголовками; разделы Errors, Native MTA types и Library usage отсутствуют полностью.
- **Доказательство:** покрыты с другими заголовками: Callbacks/Async — §3 (стр. 109–163), Timers — §4 (166–193), Objects — §5 (196–243), Resource state — §6 (247–271). Отсутствуют разделы: «Errors» (raise_error только inline в стр. 145, 181), «Native MTA types» (Resource::find лишь сниппетом в §7, стр. 287–293), «Library usage» (grep «(?i)library» по example.md — 0 совпадений).

### 3. В example.md присутствуют разделы: Testing, Creating a new function, Creating a new object, Building, Documentation generation, Doctor
- **Статус:** PARTIAL
- **Требуется:** 6 разделов.
- **Фактически:** 2 темы покрыты частично списком команд, 4 отсутствуют.
- **Доказательство:** Building/Testing частично покрыты §8 «Build, test, install» (стр. 304–327: команды mta build/test/package/doctor). Раздела «Documentation generation» нет (grep «mta docs|docgen|documentation generation» — 0 совпадений; команда mta docs описана только в api.md, стр. 525), «Creating a new function»/«Creating a new object» отсутствуют (mta new function|object описана только в api.md, стр. 528), «Doctor» — только однострочная команда в стр. 307 без раздела.

### 4. Раздел Basic function в example.md содержит код с MTA_FUNCTION(...)
- **Статус:** NOT_MET
- **Требуется:** раздел «Basic function» с кодом MTA_FUNCTION(...).
- **Фактически:** раздел отсутствует, хотя сам код MTA_FUNCTION в документе имеется.
- **Доказательство:** раздела «Basic function» в example.md нет (grep заголовков «^#{1,4} »). Код с MTA_FUNCTION есть только внутри §2 «A synchronous function» (стр. 73–75), вне требуемого раздела.

### 5. Каждый раздел example.md построен по схеме: C++ code → Lua usage → expected result → failure example where relevant
- **Статус:** PARTIAL
- **Требуется:** в каждом разделе цепочка C++ → Lua → ожидаемый результат → пример ошибки (где уместно).
- **Фактически:** полная схема только в 2 из 7 содержательных разделов, остальные частично.
- **Доказательство:** схеме следуют §2 (C++ 52–105; Lua 63–67 с ожидаемым результатом и ошибкой «greet(42) → bad argument #1») и §5 (198–243, включая failure «greeter_create({}) → error», стр. 238–239). Нарушения: §4 Timers (стр. 166–193) — только C++, блока Lua usage нет; §6 Per-resource state (стр. 252–270) — только C++, Lua usage нет; §3 (115–162) — нет failure example; §7 (276–299) — C++ + Lua, но нет expected result/failure example.

### 6. example.md написан для developer, впервые видящего SDK, и отвечает на вопрос «Я впервые вижу SDK. Как мне этим воспользоваться?»
- **Статус:** PARTIAL
- **Требуется:** достоверный документ-введение для новичка.
- **Фактически:** ориентация на новичка есть и базовый путь показан, но сквозной пример §2 ссылается на несуществующий файл сэмпла, ломая обещание «see each one compiled and tested in place».
- **Доказательство:** введение (стр. 3–9) заявляет walkthrough с нуля (mta init → config → функции → build → install), но §2 (стр. 49) ссылается на несуществующий файл «source/functions/greeter/greet.cpp»: glob source/functions/** не содержит greeter/*, grep «(?i)greeter» по source/ — 0 совпадений. Заявление «Every construct shown here exists in the bundled sample module (source/functions/), so you can see each one compiled and tested in place» (стр. 5–7) для greeter-примеров не соответствует действительности.

## Что уже соответствует
- README.md, other/documents/example.md, other/documents/api.md, other/documents/architecture.md существуют и обновлены: git-коммит ca5a443 «PHASE 12: documentation rewrite (example/api/architecture/migration)», затем 05434e2 «DoD sweep».
- other/documents/migration-v1-to-v2.md полностью написан: 255 строк, 12 разделов (layout, build/config, includes, registration, async, timers, objects, state, native types, tooling, пошаговый чек-лист из 10 шагов, «что не изменилось») с примерами кода V1→V2.
- Правило качества документации (§40) соблюдено: утверждения подкреплены кодом C++/Lua; поведение при ошибках показано примерами — example.md:65–66 (greet(42) → bad argument), example.md:238–239 (greeter_create({}) → error), api.md:179–181 (форматы ошибок bad argument #N), README.md:192–195 (лишние/отсутствующие аргументы).
- api.md полный: 17 разделов от регистрации функций до CLI (`mta`), с примерами C++ и Lua на каждый API (528 строк).
- architecture.md полный: слои source/ (§3, включая library), потоки (§4), threading rules (§5), конфигурация (§6), три уровня тестирования (§7), конвенции (§8).
- README.md переписан под V2: примеры MTA_FUNCTION/MTA_LUA_FUNCTION, таблицы типов аргументов/результатов, safety rules, сборка/установка/тестирование/CI (416 строк).
- Ссылки документации на сэмплы и тесты существуют: source/functions/** (20 файлов), other/tests/lua/scripts/*.lua (15 скриптов, включая 072_restart.lua), other/server/mta_server.py; sample_after/sample_task_run/sample_resource_find найдены в исходниках.

## Рекомендации по устранению
1. Переструктурировать example.md: вместо 8-шагового walkthrough завести именованные тематические разделы по §39 — Basic function, Multiple arguments, Optional arguments, Variadic arguments, Return values, Multiple return values, Tables, Callbacks, Async, Timers, Errors, Resource state, Objects, Native MTA types, Library usage, Testing, Creating a new function, Creating a new object, Building, Documentation generation, Doctor.
2. Добавить отсутствующие целиком темы: «Multiple return values» (примеры tuple/pair/multi-push), «Tables» (примеры работы с mta::lua::Table), «Errors» (вынести raise_error из inline-упоминаний в стр. 145 и 181 в отдельный раздел), «Native MTA types» (развернуть сниппет Resource::find из §7), «Library usage» (сейчас grep «library» по example.md даёт 0 совпадений).
3. Добавить разделы по инструментарию, описанному только в api.md: «Documentation generation» (команда mta docs, см. api.md стр. 525), «Creating a new function» и «Creating a new object» (mta new function|object, см. api.md стр. 528), отдельный раздел «Doctor» (сейчас только однострочная команда в стр. 307), расширить «Testing» и «Building» за пределы списка команд §8.
4. В разделе «Basic function» привести код с MTA_FUNCTION(...) (сейчас он есть только в §2 «A synchronous function», стр. 73–75).
5. Довести каждый раздел до схемы «C++ code → Lua usage → expected result → failure example where relevant»: добавить блоки Lua usage в §4 Timers (стр. 166–193) и §6 Per-resource state (стр. 252–270), failure example в §3 (стр. 115–162), expected result и failure example в §7 (стр. 276–299).
6. Устранить ссылку §2 (стр. 49) на несуществующий файл «source/functions/greeter/greet.cpp»: либо добавить greeter-сэмпл в source/functions/, либо переписать примеры на реально существующие сэмплы, чтобы заявление «Every construct shown here exists in the bundled sample module (source/functions/), so you can see each one compiled and tested in place» (стр. 5–7) стало достоверным.
7. Мелкое замечание вне чек-листа: в README.md привести ссылки на документы к фактическим именам файлов — заменить ARCHITECTURE.md → architecture.md и API.md → api.md.

## Статус исправления (2026-09-02)

Исправление выполнено по рекомендациям выше при сохранении walkthrough-структуры документа — это осознанное решение: документ ведёт читателя через один фича-модуль от `mta init` до установки, а каждая тема §39 теперь либо имеет собственный тематический раздел, либо демонстрируется внутри walkthrough-раздела с прямым указанием файла-аналога; полнота покрытия закреплена финальной картой покрытия. Изменённые файлы: `other/documents/example.md`, `README.md`. Сборка и тесты не запускались (по условию задачи); все имена функций/файлов в новых разделах сверены с исходниками (`source/functions/**`, `source/sdk/**`, `source/library/**`, `other/tools/mta/cli.py`, `other/documents/api.md`).

### 1. Разделы Basic function, Multiple arguments, Optional arguments, Variadic arguments, Return values, Multiple return values, Tables
- **Статус:** CLOSED (по содержимому).
- **Что закрыто:** «Multiple return values» — новый §10 (tuple-возврат `sample_hello_len` из `source/functions/basics/hello.cpp`, стр. 495–499; body-вариант `sample_minmax` из `basics/minmax.cpp`; Lua-usage с результатом и failure-примером, стр. 509–514). «Tables» — новый §11 (снапшот `mta::lua::Table`, `get_field`/`set_field` из `source/sdk/lua/table_helpers.hpp`, сэмплы `tables/table_fields.cpp` и `tables/table_stats.cpp`, C++ → Lua → результат → failure «expected table, got string», стр. 522–568). Basic function — код `MTA_FUNCTION(...)` в §2 (стр. 85) и §9; Multiple/Optional/Variadic arguments и Return values — в §2 с явными файлами-аналогами (стр. 53–58, 77–78, 119–122) и в §9.
- **Осталось осознанно:** заголовки-разделы названы по walkthrough-логике («A synchronous function» и т.д.), а не дословно «Basic function»; соответствие каждой теме §39 фиксирует карта покрытия (конец example.md, стр. 804–833).

### 2. Разделы Callbacks, Async, Timers, Errors, Resource state, Objects, Native MTA types, Library usage
- **Статус:** CLOSED.
- **Что закрыто:** «Errors» — новый §12: `mta::lua::raise_error` (Generic), таблица категорий из `source/sdk/errors/errors.hpp`, правило рендеринга `internal module error:` только для `InternalError` (`source/sdk/lua/protect.hpp`), bundled-аналог `basics/range.cpp` с failure-примером `sample_range(1, 5000)` (стр. 572–609). «Native MTA types» — новый §13: `MTA_FUNCTION` с параметром `mta::Resource` (`native/resource_args.cpp`), ручной `Resource::find`/`Resource::current` (`source/sdk/native/resource.hpp`), failure «no running resource 'no_such_resource'» (стр. 613–658). «Library usage» — новый §14: `mta::library::base::HandleMap` из `source/library/base/handle_map.hpp` на реальном использовании в `async/task_demo.cpp` (`sample_task_run`/`sample_task_cancel`), направление зависимостей из `source/library/base/README.md` (стр. 662–706). Callbacks/Async — §3, Timers — §4, Resource state — §6, Objects — §5 (как и было, walkthrough-заголовки).

### 3. Разделы Testing, Creating a new function, Creating a new object, Building, Documentation generation, Doctor
- **Статус:** CLOSED (Testing/Building — частично, как осознанное отклонение).
- **Что закрыто:** «Creating a new function»/«Creating a new object» — новый §15: `mta new function|object <name>` (синтаксис сверен с `other/tools/mta/cli.py`, `cmd_new`: имена verbatim, точки → подчёркивания в имени файла, отказ перезаписи) + `mta init` (стр. 710–733). «Documentation generation» — новый §16: `mta docs` с флагами `--output`/`--preset` (сверены с cli.py и api.md), маркер `n/a` для body-style (стр. 737–750). «Doctor» — новый §17: перечень проверок (TOML, SDK-заголовки, Lua ABI byte-compare, тулчейн, пресеты, build output, git), формат вывода из `cmd_doctor` (`[ok]/[warn]/[FAIL]`, `Status: READY/NOT READY`), failure-пример «config/module.toml not found» (стр. 754–779).
- **Осталось осознанно:** «Building»/«Testing» остались внутри §8 «Build, test, install» компактным списком команд (стр. 321–343); расширение их в отдельные простыни не выполнялось — подробности и так в api.md/architecture.md, на которые §8 опирается.

### 4. Раздел Basic function содержит код с MTA_FUNCTION(...)
- **Статус:** CLOSED (по содержимому).
- **Что закрыто:** код `MTA_FUNCTION(name, description, lambda)` показан в §2 (стр. 85–86, с аналогами `sample_hello`/`sample_hello_desc`), §9 (стр. 359, канонический `sum`), §10 (`sample_hello_len`) и §13 (`sample_resource_arg`).
- **Осталось осознанно:** выделенного заголовка «Basic function» нет — код показан внутри §2 «A synchronous function» и §9 (walkthrough-структура сохранена, см. ниже).

### 5. Схема «C++ → Lua → expected result → failure example where relevant»
- **Статус:** CLOSED для новых разделов; прежние walkthrough-разделы оставлены без дотягивания (осознанно).
- **Что закрыто:** все новые разделы построены строго по схеме: §10 (C++ 493–507 → Lua 509–514, результат + failure), §11 (530–552 → 554–561 + failure), §12 (582–588 → 590–592 failure + таблица категорий), §13 (622–648 → 650–655 + failure), §14 (671–696 → 698–702), §15 (failure «refusing to overwrite an existing file»), §16, §17 (failure `[FAIL] Project … NOT READY`).
- **Осталось осознанно:** §4 «A cancellable timer» и §6 «Per-resource state» по-прежнему без отдельного блока Lua usage, §3 — без failure example (walkthrough-формат: механика уже показана в §2/§5/§9–§13; рекомендация 5 аудита в этой части не применялась сознательно, чтобы не раздувать walkthrough).

### 6. Достоверность документа для новичка (битая ссылка §2)
- **Статус:** CLOSED.
- **Что закрыто:** ссылка §2 на несуществующий `source/functions/greeter/greet.cpp` удалена (стр. 53–58): перечислены реальные аналоги `source/functions/basics/greet.cpp` (`sample_greet`), `hello.cpp` (`sample_hello*`), `typed_params.cpp` (`sample_rest_count`/`sample_context_caller`); учебный код с именами `greet`/`greet_custom`/`greet_many`/`greet_where` сохранён, под каждым приёмом добавлена ссылка на реальный файл (стр. 77–78, 81–82, 119–122). Вступление (стр. 3–11) переписано честно: walkthrough использует учебные имена (`greet`, `greeter`), каждый C++-конструкт существует в `source/functions/` под своим `sample_*`-именем, финальные разделы покрывают инструментарий `mta` — обещание «Every construct … compiled and tested in place» теперь соответствует действительности.
- **Примечание:** walkthrough-имена (`greet_*`, `greeter`) не переименованы в `sample_*` — сопоставление с реальными файлами задано явно в тексте и в карте покрытия.

### 7. README.md — ссылки на фактические имена файлов (мелкое замечание аудита)
- **Статус:** CLOSED.
- **Что закрыто:** строки 46 и 139: `other/documents/ARCHITECTURE.md` → `other/documents/architecture.md`; строка 83: список документов заменён на фактические имена `api.md, architecture.md, example.md, GUIDES.md, TUTORIAL.md`. Дополнительно исправлены обнаруженные при проверке битые ссылки: якоря `#module-identity` (строки 9, 37, 415 — заголовка «Module identity» в README нет) → `#configuration-configmoduletoml`; include-блок async-примера ссылался на несуществующие файлы (`runtime/logging.hpp`, V1-пути `lua/arguments.hpp` и т.п.) — заменён на фактический вид из сэмплов (`<mta/sdk.hpp>` + `<memory>`; `Scheduler::post_task` существует в V2, `source/sdk/runtime/scheduler.hpp`). Прочие файловые ссылки проверены (grep + glob): `LICENSE`, `.github/workflows/ci.yml`, `cmake/core/module-config.cmake`, `other/tests/lua/harness.cpp`, `other/server/mta_server.py`, `source/functions/async/timers.cpp`, `source/functions/raw/stack_dump.cpp` — существуют.

### Отклонения, оставленные осознанно
1. **Walkthrough-структура вместо строгих ~21 именованных разделов.** Сохранены §1–§9 с их заголовками; отсутствовавшие темы добавлены как буквальные тематические разделы §10–§17 (Multiple return values, Tables, Errors, Native MTA types, Library usage, Creating a new function/object, Documentation generation, Doctor). Обоснование: документ — обучающий сценарий «один фича-модуль от init до установки», его ценность — сквозная связность; дробление §2–§9 на однострочные тематические заголовки («Optional arguments» и т.п.) разрушило бы повествование без добавления информации. Требование §39 «каждая тема показана полностью» выполняется по содержанию, а адресация «тема → раздел/файл» — явная, через карту покрытия в конце документа (таблица всех 21 тем Task_21 → раздел example.md / файл-аналог / команда).
2. **§4/§6 без блоков Lua usage, §3 без failure example** — walkthrough-разделы не дотягивались до полной схемы (см. п. 5 выше); полные схемы сосредоточены в тематических разделах §10–§13.
3. **Building/Testing остались списком команд в §8** (см. п. 3) — детальные описания команд уже в api.md, дублировать их в example.md нецелесообразно.
4. Ссылки на реальные файлы теперь достоверны: каждый файл, упомянутый в example.md и README.md, проверен через glob/чтение исходников.