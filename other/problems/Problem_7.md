# Problem 7 — mta doctor не проверяет «SDK version» и «Architecture»; `mta test` не запускает integration-тесты по умолчанию

**Задача:** `other/tasks/Task_17.md` — MTA CLI: команды init, build, test, docs, doctor, package, server, new function, new object
**Вердикт:** Проект не соответствует задаче (статическая проверка, 2026-09-02)
**Серьёзность:** P1 — CLI реализован практически полностью, но нарушены значимые требования задачи: doctor не выполняет 2 из 17 минимальных обязательных проверок, а `mta test` без аргументов не запускает integration-тесты.

## Суть проблемы
CLI-инструмент mta полностью реализован в other/tools/mta/cli.py (лаунчеры mta/mta.cmd) со всеми девятью командами и реальными реализациями: init копирует SDK-шаблон с переписыванием имени модуля, build/test/docs работают через CMake-пресеты и ctest, doctor выполняет настоящие проверки окружения, package пишет в dist/ с sha256, server делегирует реальному harness'у other/server/mta_server.py. Несоответствий два: mta doctor не проверяет пункты «SDK version» и «Architecture» из минимального обязательного списка §28, а mta test без аргументов запускает только unit+lua через ctest — integration выполняется лишь отдельной командой mta test integration. Остальные пункты чек-листа подтверждены статически.

## Несоответствия

### 1. mta doctor проверяет минимум: Project, Module configuration, SDK version, Compiler, Compiler version, Architecture, CMake, Ninja/MSBuild where applicable, Lua headers, MTA SDK headers, Lua ABI compatibility, C++ standard, Build system, Server test environment, Server version/build, Output/build directories, Git state where useful
- **Статус:** PARTIAL
- **Требуется:** doctor должен проверять SDK version и Architecture (в примере вывода «architecture x64» стоит в секции Compiler).
- **Фактически:** из 17 минимальных пунктов реализованы 15; SDK version не проверяется вовсе (в проекте нет идентификатора версии SDK, doctor его не выводит), Architecture компилятора/сборки не определяется — из install.json печатается только архитектура установленного сервера. Проверка MSBuild отсутствует, но это допустимо: все пресеты CMakePresets.json используют генератор Ninja, MSBuild неприменим.
- **Доказательство:** other/tools/mta/cli.py, cmd_doctor (строки 423–547): есть проверки Project+module.toml (427–446), SDK headers (449–456), Lua ABI побайтовым сравнением (458–473), CMake (476–478), Ninja (477–479), Compiler+version (481–487), C++ standard из module.toml (489–490), Presets/Source discovery (493–502), Build output (504–509), Git (512–518), MTA server по install.json (521–535). Проверок «SDK version» и «Architecture» в cmd_doctor нет; grep по source/ на SDK_VERSION|MTA_SDK_VERSION — 0 совпадений; архитектура жёстко задана «x64» в output_binary (строка 176), а не определяется.

### 2. mta test выполняет unit tests, Lua tests и integration tests с понятным разделением
- **Статус:** PARTIAL
- **Требуется:** `mta test` выполняет unit + Lua + integration с понятным разделением.
- **Фактически:** `mta test` (all) запускает только unit и lua через ctest, integration в запуск по умолчанию не входит (только отдельной командой), а в режиме all нет явного разделения вывода по suite'ам (разделение есть лишь при выборочных вызовах через -R).
- **Доказательство:** other/tools/mta/cli.py, cmd_test (строки 354–379): ветка по умолчанию (suite="all") запускает только `ctest --preset ...` без -R; integration выполняется только в отдельной ветке `mta test integration` (строки 360–366). CMakeLists.txt строки 264–271: ctest-тесты только sdk_tests, module_config_parse, module_config_rejects_garbage — интеграционного ctest-теста нет.

## Что уже соответствует
- Существует CLI mta (other/tools/mta/cli.py + лаунчеры other/tools/mta/mta и mta.cmd) со всеми командами: init, build, test, docs, doctor, package, server, new function, new object — каждая реализована реально, не заглушка.
- mta init создаёт структуру модульного проекта копированием SDK-шаблона (cmd_init, строки 199–226): в шаблоне есть config/module.toml, source/functions/, source/library/, source/sdk/, other/, CMakeLists.txt, CMakePresets.json, README.md; имя и title модуля переписываются в module.toml.
- mta init внутри существующего проекта не разрушает файлы: отказ при наличии config/module.toml (строки 192–193) и при непустом целевом каталоге (строки 196–197).
- mta new function создаёт минимальный compile-ready шаблон с #include <mta/sdk.hpp> и MTA_FUNCTION("имя", lambda) (FUNCTION_TEMPLATE, строки 241–251); макрос MTA_FUNCTION существует (source/sdk/registry/registry.hpp:105), новые .cpp подхватываются source discovery (cmake/core/file.cmake: GLOB_RECURSE CONFIGURE_DEPENDS).
- Имя из mta new function регистрируется дословно: crypto.sha256 → MTA_FUNCTION("crypto.sha256"), меняется только имя файла (cmd_new, строки 303–306).
- mta new object создаёт skeleton native object с MTA_OBJECT/MTA_METHOD/mta::userdata::Registry (OBJECT_TEMPLATE, строки 253–290); API существуют (source/sdk/objects/userdata.hpp:285–291) и согласованы с рабочим примером source/functions/objects/counter.cpp.
- mta doctor реально проверяет работоспособность: парсит TOML, побайтово сравнивает Lua-заголовки (совместимость ABI), запускает cmake/ninja/g++ для версий, проверяет source discovery, git-состояние и install.json; итог READY/NOT READY с кодом возврата (строки 545–547).
- Поддерживаются вызовы mta test, mta test unit, mta test lua, mta test integration (argparse choices [all,unit,lua,integration], строки 618–621): unit → ctest -R module_config, lua → ctest -R sdk_tests, integration → other/server/mta_server.py test (реальный harness: сборка модуля, временный сервер, тестовый ресурс).
- mta docs реально собирает цель sdk_docgen (CMakeLists.txt:278) и запускает бинарник; mta package копирует бинарник в dist/ с именем name-version-platform и sha256; mta server делегирует реальным командам install/update/version/start/stop в other/server/mta_server.py.

## Рекомендации по устранению
1. Ввести идентификатор версии SDK в проекте (например, константу/макрос SDK_VERSION в заголовках SDK — сейчас grep по source/ на SDK_VERSION|MTA_SDK_VERSION даёт 0 совпадений) и добавить в cmd_doctor (other/tools/mta/cli.py, строки 423–547) проверку «SDK version», которая считывает и выводит эту версию.
2. Добавить в cmd_doctor проверку «Architecture»: определять целевую архитектуру компилятора/сборки, а не брать её из install.json (сейчас архитектура жёстко задана «x64» в output_binary, строка 176, а не определяется), и выводить её в секции Compiler, как в примере вывода задачи («architecture x64»).
3. Расширить cmd_test (other/tools/mta/cli.py, строки 354–379) так, чтобы ветка по умолчанию suite="all" выполняла все три suite'а — unit, lua и integration (сейчас integration выполняется только отдельной веткой `mta test integration`, строки 360–366).
4. Обеспечить явное разделение вывода по suite'ам в режиме all (сейчас разделение есть лишь при выборочных вызовах через -R).
5. Рассмотреть добавление интеграционного ctest-теста в CMakeLists.txt (строки 264–271, сейчас там только sdk_tests, module_config_parse, module_config_rejects_garbage), чтобы integration-тесты были видимы и в едином ctest-прогоне.

## Статус исправления (2026-09-02)

Все несоответствия устранены; единственный изменённый файл — `other/tools/mta/cli.py` (companion-файлы `mta`/`mta.cmd` правок не требовали; вызовы из `.github/workflows/ci.yml` и `release.yml` сохранены).

### 1. doctor: «SDK version» и «Architecture» — исправлено
- Введён идентификатор версии SDK: константа `SDK_VERSION = "1.0.0"` в other/tools/mta/cli.py (рядом с SDK_ROOT, с комментарием). Вариант «макрос в заголовках source/sdk» (рекомендация 1) недоступен в рамках этой задачи — source/ вне разрешённого списка правок, поэтому идентификатор размещён в CLI, поставляемом внутри SDK-чекaута.
- В cmd_doctor добавлена проверка «SDK version»: наличие + формат X.Y.Z, вывод вида `sdk 1.0.0`.
- Добавлена проверка «Architecture»: целевая архитектура определяется пробой самого компилятора (`g++ -dumpmachine`; для MSVC — `VSCMD_ARG_TGT_ARCH`, который экспортирует msvc-dev-cmd из CI), нормализация тега повторяет cmake/core/platform.cmake (x86_64|amd64 → x64). Это архитектура тулчейна, а не install.json сервера; выводится рядом с Compiler: `x64 (g++ -dumpmachine: x86_64-w64-mingw32)`. Не-x64 → WARN с пояснением про x64-раскладку build/package. Хардкод «x64» в output_binary (строка 176) намеренно не тронут — его изменение меняло бы поведение работающих build/docs/package.
- Итог: реализованы все 17 минимальных пунктов §28 (MSBuild неприменим — все пресеты Ninja, что аудитом допускалось).

### 2. mta test: integration в режиме all + разделение — исправлено
- cmd_test переписан: suite="all" выполняет все три suite'а с явными секциями `== unit ==`, `== lua ==`, `== integration ==` и итоговой строкой (`All test suites passed` / `FAILED suites: ...`); код возврата 1 только при реальном падении выполненного suite'а (рекомендации 3 и 4).
- Явные вызовы сохранены без изменений: `unit` → `ctest -R module_config`, `lua` → `ctest -R sdk_tests` (команды ctest идентичны прежним), `integration` — прежняя отдельная блокирующая команда с делегированием в other/server/mta_server.py test (как её вызывают ci.yml и release.yml).
- integration внутри `all` гейтируется новой `integration_ready()`: harness присутствует + install.json существует + платформа из install.json совпадает с хостом; иначе печатается `integration: NOT RUN (причина)` без падения прогона — соответствует «integration NOT RUN» из примера §28. Это сохраняет совместимость: в release.yml `test --preset X all` идёт до `server install` на всех трёх runner'ах (install.json в checkout отсутствует — .gitignore), в ci.yml вызываются только `test unit`/`test lua`. На машине с установленным pinned-сервером `all` выполняет и integration.
- Рекомендация 5 (интеграционный ctest-тест в CMakeLists.txt) сознательно не выполнялась: файл вне разрешённого списка правок, рекомендация необязательная; integration остаётся доступным через реальный harness.

### Чем подтверждено (без сборок)
- `python -m py_compile other/tools/mta/cli.py` — OK.
- `mta --help` и `--help` всех 8 подкоманд — OK; опция package `--release-name` не тронута; choices теста `[all, unit, lua, integration]` сохранены.
- `mta doctor` в корне репозитория — Status: READY, код 0; среди проверок новые «SDK version: sdk 1.0.0» и «Architecture: x64 (g++ -dumpmachine: x86_64-w64-mingw32)».
- `mta test unit` — 2/2 passed, код 0. `mta test lua` падает в 015_facade.lua (`sample_state` nil) — pre-existing и вне скоупа: ветка `lua` даёт побайтово ту же команду ctest, что раньше, а в рабочем дереве есть незакоммиченные правки other/tests/lua/scripts/015_facade.lua и новых source-файлов при устаревшем бинарнике sdk_tests.
- `integration_ready()` проверена прямыми вызовами: без harness → NOT RUN; без install.json → NOT RUN (состояние CI-раннера на шаге `test --preset X all`); репозиторий на Windows с установленным сервером → готова; имитация linux-хоста → NOT RUN («the installed server is windows-only»).
- `mta init sdk-gap-check`, `mta new function crypto.sha256`, `mta new object counter`, `mta doctor` — выполнены в C:\Temp (имя регистрируется дословно: `MTA_FUNCTION("crypto.sha256", ...)`); в самом репозитории init/new не запускались, временные каталоги удалены.