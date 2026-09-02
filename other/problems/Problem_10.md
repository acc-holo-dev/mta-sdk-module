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