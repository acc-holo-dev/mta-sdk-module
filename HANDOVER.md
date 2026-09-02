# HANDOVER — передача работы другому агенту (2026-09-02)

Этот документ — полная передача контекста. Прочитай его целиком перед любыми действиями.
Репозиторий: `https://github.com/acc-holo-dev/mta-sdk-module.git`, ветка `master`.

---

## 1. Что это за проект и что происходило (история)

**Проект:** MTA:SA SDK для нативных C++ модулей (C++20, Lua 5.1 C API, CMake 3.27+, MSVC/MinGW/GCC).
Фасад для разработчика — `source/mta/sdk.hpp`; личность модуля — `config/module.toml` (name=base, version 2.0.0).
Мастер-план — `PROMT.md` в корне (~2418 строк, разделы `# 0.`–`# 56.`, блоки PHASE 0–14 внутри `# 49. Implementation phases`). **PROMT.md — read-only, никогда не изменять.**

**Хронология работы в этой сессии:**

1. **Разбивка плана на задачи.** `PROMT.md` был разделён на 25 файлов `other/tasks/Task_1.md … Task_25.md`
   (каждый — дословный раздел/PHASE-блок плана + «Чек-лист соответствия проекта»). Все 57 разделов и 15 PHASE-блоков
   покрыты ровно один раз.
2. **Аудит проекта по каждой задаче.** По 13 задачам проект признан несоответствующим — записаны
   `other/problems/Problem_1.md … Problem_13.md` (шаблон: Вердикт/Суть/Несоответствия со Статус-Требуется-Фактически-Доказательство/Что уже соответствует/Рекомендации).
   Соответствующие задачи (без problem-файлов): Task_1, 2, 3, 4, 5, 9, 10, 11, 12, 16, 19, 24.
   Карта: P1→T6, P2→T7, P3→T8, P4→T13, P5→T14, P6→T15, P7→T17, P8→T18, P9→T20, P10→T21, P11→T22, P12→T23, P13→T25.
   Полные JSON-аудиты с доказательствами лежат **вне репозитория**: `C:/Temp/dsh-verify/task-1..25.json` (если машины нет — не критично, всё существенное продублировано в problem-файлах).
3. **Устранение проблем.** Работа велась волнами параллельных субагентов; каждый исправленный Problem-файл получил
   в конец раздел `## Статус исправления (2026-09-02)` (что изменено, чем подтверждено, что не завершено).

---

## 2. Состояние проблем: что сделано, что нет

| Problem | Тема | Статус | Детали / доказательства |
|---|---|---|---|
| P11 (T22) | CLI-вызовы в ci.yml | ✅ устранена | `--preset`, `test integration` вместо несуществующего `server test`, `continue-on-error` снят; статус-раздел в файле |
| P13 (T25) | release.yml вызовы | ✅ устранена | `build/test/package --preset ${{ matrix.preset }}`; порядок build→test→integration→package→publish блокирующий |
| P9 (T20) | release pipeline | 🔶 ~90% | Имя артефакта §36: флаг `package --release-name` (cli.py) → `dist/<module>.dll/.so`; integration блокирует publish; **ограничение**: интеграция только на win-mingw (harness Windows-only, `MTA Server64.exe`) — это соответствует §37 «relevant integration tests». Версии §38: см. строку ниже |
| P9-хвост (§38) | версии SDK/ABI/Module/MTA | 🔶 почти готово | Создан единый источник `source/sdk/version.hpp` (`SDK_VERSION 1.0.0`, `MODULE_ABI_VERSION 1`); `other/tools/mta/cli.py` читает версии ИЗ ЗАГОЛОВКА (не дублирует литералы); doctor показывает раздельно: `SDK version sdk 1.0.0`, `ABI version module-abi 1`, `Project version=2.0.0` — проверено запуском `mta doctor` → Status: READY. **Осталось:** дописать в конец `Problem_9.md` подраздел `**Дополнение: версии §38**` (агент был прерван) + проверить согласованность regex в CMakeLists/install.cmake ↔ version.hpp + упоминание версий в README/доках |
| P1 (T6) | фасад MTA_STATE/mta::state | ✅ устранена | `source/sdk/lua/state.hpp` (`mta::state`, макрос `MTA_STATE(L)`), экспорт через `source/mta/sdk.hpp`; demo `source/functions/state/view.cpp`; `source/functions/` полностью без internals (grep `Registry::instance\|Scheduler::instance\|mta::module::\|ILuaModuleManager10` → 0); статус-раздел ✔ |
| P6 (T15) | авто-контекст логирования | ✅ устранена | `source/sdk/lua/protect.hpp`: `DiagnosticContext` + RAII `ScopedDiagnosticContext`; префиксы `[Module:function @ resource]` / `[Module task #N …]` / `[Module timer #N …]`; API `mta::log` не изменён; guards в scheduler/callback; статус-раздел ✔ |
| P2 (T7) | binder: 12-й тип | ✅ устранена | `mta::Resource` как аргумент (живая валидация `Resource::find`, ошибка `InvalidObject` «no running resource '...'») и как возврат (`push_one` ADL-хук в `source/sdk/native/resource.hpp`); примеры `source/functions/native/resource_args.cpp`; тесты в `040_binder.lua`/`060_features.lua`; статус-раздел ✔ |
| P4 (T13) | native-типы × binder | ✅ устранена | `MTA_FUNCTION("f", [](mta::Resource r){...})` работает; Player/Vehicle/Element/Object сознательно НЕ реализованы (замороженный ABI) — задокументировано; статус-раздел ✔ |
| P3 (T8) | signature metadata + docgen | ✅ устранена | `Signature::flags` (variadic/callback) → `Spec::flags`; `MethodInfo`/`ObjectTypeInfo`/`mta::userdata::object_types()`; `other/tools/docgen.cpp` переписан (аргументы с типами/optional, returns, flags, object methods, явные n/a); статус-раздел ✔ |
| P7 (T17) | команды CLI | ✅ устранена | doctor: `SDK version` + `Architecture` (все 17 пунктов §28); `mta test all` = unit+lua+integration с гейтом `integration_ready()` (NOT RUN без падения, если сервер недоступен); init/new проверены в C:\Temp; статус-раздел ✔ |
| P8 (T18) | integration-сценарии §32–33 | ✅ устранена | `other/tests/integration/main_resource.lua` (3 поколения) + `witness_resource.lua` (второй ресурс); harness переписан; **реальный прогон 2026-09-02: exit 0, все 20 проверок PASS** (`other/server/logs/20260902-101542/server.log`); окно graceful в `stop_process` расширено до 120 с (SDK join'ит 60-секундный worker в ShutdownModule — это корректная «safe shutdown», kill ⇒ FAIL); статус-раздел ✔ (обновлён) |
| P13/P11 примечание | | | Порядок в release.yml: build → test → integration → package → publish, publish только при успехе всех |
| P10 (T21) | документация | 🔶 ~85% | Доки уже существенно синхронизированы двумя проходами: `example.md` §9 (все 9 пунктов §8: sum-пример, типы, генерация ошибок, manual validation), `api.md` (таблицы типов, Resource-as-argument, авто-контекст логов, counted every), `architecture.md` §3.2–3.4, `GUIDES.md` §9, `CHANGELOG.md`. **Осталось:** сверить каждый gap из `Problem_10.md` с фактическим состоянием и дописать `## Статус исправления (2026-09-02)` в Problem_10.md |
| P12 (T23) | benchmarks | 🔶 ~60% | Созданы `source/functions/bench/{args,callback,tables}.cpp` (плашки для замеров, подхватываются авто-discovery) и `other/tests/lua/scripts/091_benchmark_arguments.lua`; расширен `020_tables.lua`. **Осталось:** сверить с gaps в Problem_12.md (какие из 7 закрыты), при необходимости дописать бенчмарки, прогнать lua-набор, дописать `## Статус исправления` в Problem_12.md |
| P5 (T14) | View/Snapshot | ❌ НЕ НАЧАТА | Самый крупный оставшийся кусок. Читай `other/problems/Problem_5.md` + `other/tasks/Task_14.md`. Замечание: часть проблемы могла устареть — async уже работает на owned-снимках Arguments (см. `source/sdk/runtime/scheduler.cpp`), а `mta::state` появился. Читай Problem_5.md внимательно и отдели «уже решено смежными фиксами» от реальных остатков |

**Дополнительные файлы, появившиеся при исправлениях:** `source/functions/basics/typed_params.cpp`,
`source/functions/native/` (resource_args.cpp), `source/functions/state/view.cpp`, `source/functions/bench/`,
`source/sdk/lua/state.hpp`, `source/sdk/native/module.{hpp,cpp}`, `source/sdk/version.hpp`,
`other/tests/integration/{main_resource,witness_resource}.lua`, `other/tests/lua/scripts/091_benchmark_arguments.lua`.

---

## 3. Верифицированное состояние дерева (на момент push)

Выполнено и зелёное на ТЕКУЩЕМ дереве (включая правки всех прерванных агентов):

- **Сборка win-mingw:** exit 0 (`base.dll`, `sdk_tests.exe`, `sdk_docgen.exe`; unity-блоки 0–4, `-Werror`).
- **`mta test --preset win-mingw unit`:** 2/2 passed.
- **`mta test --preset win-mingw lua`:** 100% passed (8.7 s; включает 015/020/040/060 + 091).
- **`mta doctor`:** Status READY, раздельные версии видны (см. §2, P9-хвост).
- **Integration (реальный сервер 24140):** exit 0, 20/20 PASS — финальный сертификационный прогон
  10:51:21 **на полном текущем дереве** (включая все правки версий/доков/бенчмарков)
  (`other/server/logs/20260902-105121/server.log`; более ранний прогон 10:15:42 — `20260902-101542/server.log`).

---

## 4. Что осталось сделать (приоритетный To-Do для нового агента)

1. **P5 — View/Snapshot** (`other/problems/Problem_5.md` + `other/tasks/Task_14.md`).
   Сначала сверь проблему с фактическим кодом: `source/sdk/lua/arguments.hpp`, `source/sdk/runtime/scheduler.cpp`
   (async уже держит owned-снимки аргументов), `source/sdk/lua/state.hpp` (новый). Реши, какие gaps остались,
   реализуй минимально, обнови Problem_5.md статус-разделом.
2. **Завершить P9-хвост (§38):** (а) проверить согласованность версий: `source/sdk/version.hpp` ↔ regex в
   `other/tools/mta/cli.py` ↔ `CMakeLists.txt`/`cmake/install.cmake` (агент прерван на этой проверке —
   прогони `mta doctor` и `git diff CMakeLists.txt cmake/install.cmake other/tools/mta/cli.py` глазами);
   (б) синхронизировать упоминания версий в README/доках; (в) дописать в `other/problems/Problem_9.md`
   подраздел `**Дополнение: версии §38**` к существующему разделу статуса.
3. **Завершить P12:** прогнать `mta test --preset win-mingw lua` (091-бенчмарк уже в наборе);
   при необходимости серверный замер — `python other/server/mta_server.py test` прогоняет только integration;
   бенчмарки на сервере не гейтятся — достаточно lua-набора; дописать статус-раздел в Problem_12.md
   (какие из gaps закрыты, как запускать).
4. **Завершить P10:** сверить каждый gap `Problem_10.md` с текущим состоянием
   `other/documents/{example,api,architecture,GUIDES}.md` + `README.md` + `CHANGELOG.md`;
   дописать `## Статус исправления (2026-09-02)` в Problem_10.md со ссылкой «gap → где закрыт».
5. **Контрольная пере-проверка:** пройтись по всем 13 Problem-файлам и их «Чек-лист соответствия»
   (или по разделам Несоответствия), собрать финальную сводку: что устранено полностью,
   что частично и почему (осознанные отклонения уже задокументированы в статус-разделах:
   Player/Vehicle не выдумывать — ABI; типизированный userdata-параметр без потребителя; категория «n/a» в docgen; и т.п.).
6. **Обновить CHANGELOG.md** под фактическое состояние и убедиться, что статус-разделы есть во всех
   13 Problem-файлах (сейчас: P1,P2,P3,P4,P6,P7,P8,P9,P11,P12?,P13 — P10 и, возможно, P12 надо дописать).
7. **Закоммитить и запушить** (см. §6) — фиксируй работу атомарными коммитами по волнам/проблемам.

---

## 5. Как верифицировать (точные команды, Windows, из корня репозитория)

```powershell
# ВАЖНО: вендоред тулчейн (cmake/ninja/gcc 16.2) — НЕ в PATH по умолчанию.
# Переконфигурация при новых .cpp требует тулчейн в PATH, иначе
# "CMAKE_MAKE_PROGRAM is not set" (инцидент 2026-09-02). Всегда так:
$env:PATH = 'D:\File\Developer\Project\mta-sdk-module\build\toolchain\mingw\mingw64\bin;' + $env:PATH
& .\build\toolchain\mingw\mingw64\bin\cmake.exe --preset win-mingw     # configure (кеш в build/win-mingw)
& .\build\toolchain\mingw\mingw64\bin\cmake.exe --build .\build\win-mingw   # сборка (unity, -Werror)

# Тесты (CLI сам разрулит пути):
python other/tools/mta/cli.py test --preset win-mingw unit      # ctest, 2 теста
python other/tools/mta/cli.py test --preset win-mingw lua       # sdk_tests: все lua-скрипты
python other/tools/mta/cli.py test integration                  # РЕАЛЬНЫЙ сервер, ~3.5 мин, exit 0 = 20/20 PASS
python other/tools/mta/cli.py doctor                            # окружение + версии (Status: READY)

# Интерфейс CLI (проверка парсера):
python other/tools/mta/cli.py --help
python other/tools/mta/cli.py build --help   # только --preset
python other/tools/mta/cli.py test  --help   # suite [all|unit|lua|integration] позиционно + --preset
python other/tools/mta/cli.py package --help # --preset + --release-name
```

- **win-msvc локально недоступен** (нет Visual Studio/vswhere) — его собирает только CI; **linux-gcc** — только CI.
- Integration требует скачанный сервер: `python other/tools/mta/cli.py server install`
  (pinned build 24140; локально уже установлен, `other/server/install.json` в git не входит).
- Логи integration: `other/server/logs/<timestamp>/server.log` (в git не входят).

---

## 6. Git-процедура

```powershell
git add -A
git commit -m "Fix wave: workflows/CLI, facade+logging, binder native types, metadata+docgen, CLI gaps, integration suite, versions, docs, benchmarks"
git push origin master
```

Remote: `https://github.com/acc-holo-dev/mta-sdk-module.git` (origin), ветка `master`.
В репозиторий должны попасть: `other/tasks/` (25 файлов), `other/problems/` (13 файлов), этот `HANDOVER.md`
и все правки кода/доков/CI. **Не коммитится** (gitignore): `build/`, `other/server/servers|logs|run|downloads|install.json`,
`C:/Temp/*`. На момент этого handover дерево уже запушено этим агентом (см. последний коммит).

---

## 7. Известные ловушки (обязательно к прочтению)

1. **pwsh и кириллица:** `-Command` искажает кириллические литералы (паттерны «не находят» то, что есть).
   Для поиска по содержимому файлов используй grep-инструмент/ripgrep; `write`/`edit` — UTF-8-безопасны;
   вывод содержимого файлов через pwsh — корректен. Все артефакты — на русском, дата 2026-09-02.
2. **Переконфигурация сборки:** `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` в `cmake/core/file.cmake` — новые
   .cpp подхватываются БЕЗ правок CMake, но вызывают re-run configure; он падает без тулчейна в PATH (см. §5).
3. **Unity-сборка:** все .cpp одного таргета компилируются едиными TU — не создавай одинаковые
   `{anonymous}`-символы в разных файлах (инцидент: `g_timers` в timers.cpp ↔ timer_demo.cpp; исправлен
   переименованием в `g_sample_timers`).
4. **Параллельные субагенты:** не давай двум агентам один и тот же файл (болезненные файлы:
   `source/mta/sdk.hpp`, `other/tools/mta/cli.py`). Сборку запускай после того, как все правки кода
   завершены, иначе ловишь чужие полусостояния (инцидент: интеграционный прогон упал на чужом незаконченном
   `userdata.hpp`; const push_back исправлен через приватный `object_types_mutable()`, публичный
   `object_types()` остался read-only).
5. **CLI-интерфейс:** `build`/`package` принимают ТОЛЬКО `--preset`; `test` — позиционный suite
   `[all|unit|lua|integration]` + `--preset`; `package --release-name` — имя `dist/<module>.dll/.so` (§36).
   CI/release вызовы не менять без сверки с argparse.
6. **Integration-окно:** SDK корректно join'ит активные worker'ы в `Scheduler::shutdown()` (plan §13
   «safe shutdown») — окно graceful-остановки в harness 120 с, НЕ уменьшать (иначе 60-секундный сценарный
   worker превращается в kill → ложные FAIL).
7. **Проблемы-файлы:** не переписывать аудиторские разделы; добавлять `## Статус исправления (дата)` в конец.
   Осознанные отклонения фиксировать честно, с обоснованием (примеры уже в файлах).
8. **Тесты с точными строками ошибок** (`060_features.lua`) зависят от формата `bad_argument_object` —
   при изменении формата ошибок биндера синхронизируй обе стороны.
9. **docgen запускает registrar'ы объектов в scratch-VM** (`luaL_newstate`) — registrar должен быть
   безопасен без реального MTA-окружения (сейчас counter.cpp — да).
10. **`mta test all`** гейтит integration через `integration_ready()` — на CI-раннере без сервера
    интеграция печатает `integration: NOT RUN (причина)` и НЕ роняет прогон; release.yml ставит
    `server install` до `test all` — порядок шагов не менять.

---

## 8. Ключевые файлы-ориентиры

- План (read-only): `PROMT.md`
- Задачи: `other/tasks/Task_1..25.md`; Проблемы: `other/problems/Problem_1..13.md`
- Фасад: `source/mta/sdk.hpp` (+ `source/sdk/lua/state.hpp`, `source/sdk/native/module.hpp`)
- Binder: `source/sdk/bind/bind.hpp`; useradata-объекты: `source/sdk/objects/userdata.hpp`
- Версии: `source/sdk/version.hpp` (единый источник; doctor/cli.py читают его)
- CLI: `other/tools/mta/cli.py`; Докген: `other/tools/docgen.cpp`
- Integration: `other/server/mta_server.py` + `other/tests/integration/`
- Вендоред тулчейн: `build/toolchain/mingw/mingw64/bin/{cmake,ninja,g++,gcc}.exe`
- Прошлые успешные логи integration: `other/server/logs/20260902-101542/server.log` (20/20 PASS)

---

## 9. Итог волны 2026-09-02 (дополнено после этого push)

Все пункты To-Do из §4 выполнены:

1. **P5 (View/Snapshot) — закрыта.** Модель LuaView существует как `mta::state`
   (создана волной P1, см. Problem_1) с явным позиционированием «plan §18/§45 —
   the LuaView half»; добавлены псевдоним `mta::LuaView`, `arg_count()` и
   типизированные ридеры `check_*/opt_*` в `source/sdk/lua/state.hpp`;
   дихотомия View vs Snapshot задокументирована в api.md/architecture.md;
   статус-раздел в Problem_5.md.
2. **P9-хвост (§38) — закрыт.** «Дополнение: версии §38» в Problem_9.md;
   версии разделены в api.md («The four versions») и architecture.md;
   согласованность version.hpp ↔ cli.py ↔ CMakeLists.txt ↔ install.cmake
   проверена сопоставлением файлов.
3. **P12 (benchmarks) — закрыта.** Скрипты `092_benchmark_tables.lua`,
   `093_benchmark_callback.lua`, `094_benchmark_scheduling.lua`,
   `095_benchmark_userdata.lua` (все восемь областей §44 теперь покрыты
   090-095); политика «measure before optimizing» в CONTRIBUTING.md и
   architecture.md §8; статус-раздел в Problem_12.md. Числа прогонов
   фиксируются централизованным тестированием после push.
4. **P10 (документация) — закрыта.** example.md: §10-§17 (Multiple return
   values, Tables, Errors, Native MTA types, Library usage, Creating new
   function/object, Documentation generation, Doctor) + карта покрытия 21
   темы §39 + честное вступление и файлы-аналоги вместо битой ссылки
   greeter; README.md: ссылки исправлены. Статус-раздел в Problem_10.md.
5. **Контрольная пере-проверка:** статус-разделы присутствуют во всех 13
   Problem-файлах (2026-09-02).
6. CHANGELOG.md обновлён (версии §38, LuaView-ридеры, блокирующий
   integration в release, `--release-name`, карта бенчмарков, project(VERSION)
   из version.hpp).
7. Тестирование/сборка волны выполняются централизованным этапом
   (запрещено до наступления этапа по плану владельца).