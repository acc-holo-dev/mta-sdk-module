# Problem 9 — Имя релизного артефакта не соответствует §36, integration-тесты в release неблокирующие, версии SDK/ABI не разделены с Module/MTA

**Задача:** `other/tasks/Task_20.md` — Release-политика, валидация release pipeline и разделение версий (SDK/Module/ABI/MTA)
**Вердикт:** Проект не соответствует задаче (статическая проверка, 2026-09-02)
**Серьёзность:** P1 — значимые нереализованные требования (формат имени релизного файла §36, блокирующие integration-тесты на всех целях, отдельные SDK version и ABI version из §38), при этом критические требования не нарушены: запрещённые префиксы не добавляются, публикация выполняется только после успешных build/test.

## Суть проблемы

Release-пайплайн в целом соответствует задаче: матрица из трёх сборок (MSVC, MinGW, Linux), unit+lua тесты, проверка существования бинарника, публикация только бинарных артефактов без префиксов sdk_/mta_/holo_, причём публикация — шаг того же job после успешных build/test/package. Однако имя публикуемого файла — `base-2.0.0-win-x64.dll` (other/tools/mta/cli.py:573), а не требуемое §36 `<module-name>.dll/.so`. Integration-тесты в release — non-blocking canary только на одной из трёх целей. Разделение четырёх версий реализовано лишь для Module version и MTA-версии, а SDK version и ABI version не представлены нигде (подтверждено own-документом other/documents/v2-audit.md:186-188 как известный пробел).

## Несоответствия

### 1. Название релизного файла соответствует developer-defined module name: <module-name>.dll / <module-name>.so (§36: «для release artifact оставлять только base.dll / base.so», пример my_module.dll)
- **Статус:** PARTIAL
- **Требуется:** релизный файл называется ровно `<module-name>.dll`/`.so` (для модуля base — base.dll / base.so).
- **Фактически:** `mta package` копирует бинарник в dist/ под именем `<name>-<version>-<platform>-x64.dll` (base-2.0.0-win-x64.dll), и именно эти файлы публикуются в GitHub Release. Имя модуля используется как основа, запрещённые префиксы не добавляются, но точный формат `<module-name>.dll/.so` не соблюдается.
- **Доказательство:** other/tools/mta/cli.py:573 — `target = dist / f"{module_tbl['name']}-{version}-{platform_tag}-x64{binary.suffix}"`; .github/workflows/release.yml:83-89 — publish `files: dist/*`; фактический артефакт dist/base-2.0.0-win-x64.dll; собранный бинарник называется base.dll (CMakeLists.txt:226-229 — `OUTPUT_NAME "${SDK_MODULE_NAME}"`, `PREFIX ""`); комментарий release.yml:4-5 обещает '<module-name>.dll / <module-name>.so — nothing else ships', но package переименовывает файл.

### 2. Release pipeline выполняет relevant integration tests перед публикацией артефактов (§37, шаг 5; чек-лист: «выполняет tests и relevant integration tests перед публикацией»)
- **Статус:** PARTIAL
- **Требуется:** интеграционные тесты как полноценный шаг release pipeline перед публикацией.
- **Фактически:** integration запускается только на одной из трёх целей (win-mingw; MSVC и Linux его не запускают вовсе) и с `continue-on-error: true` — упавшие integration-тесты не препятствуют публикации артефакта.
- **Доказательство:** .github/workflows/release.yml:72-81 — шаг 'Integration canary (non-blocking)': `continue-on-error: true` и только `if: matrix.preset == 'win-mingw'`; комментарий: 'on release it is a non-blocking canary ...; the blocking gates are build + tests'.

### 3. Понятия SDK version, Module version, ABI version и MTA compatibility version разделены и не смешиваются; при необходимости указываются отдельно в binary metadata или diagnostics (§38)
- **Статус:** PARTIAL
- **Требуется:** разделять 4 понятия (SDK version, Module version, ABI version, MTA compatibility version) и при необходимости указывать их отдельно в binary metadata или diagnostics.
- **Фактически:** представлены и разделены только Module version (float в Info/sample_version) и MTA-версия сервера (диагностика при загрузке, поля mta/netcode). Отдельного SDK version и ABI version нет нигде — ни констант, ни полей метаданных, ни diagnostics; версия CMake-проекта (SDK-шаблона) совмещена с версией модуля через единый source module.toml.
- **Доказательство:** Module version — config/module.toml:16 (`version = "2.0.0"`) → CMakeLists.txt:133-137 (SDK_MODULE_VERSION) → source/sdk/abi/module.cpp:30,35-39 (module_details.version, binary metadata); MTA-версия отдельно — module.cpp:101-103 (лог "module: loaded <name> (MTA <server_version>)") и source/functions/info/version.cpp:18-28 (поля module_version и mta/netcode раздельно). Отсутствует: grep `SDK_VERSION|ABI_VERSION` по source/ — 0 совпадений; CMakeLists.txt:14 — `project(mta_sdk_module VERSION "${MODULE_CFG_module_version}")`, т.е. версия SDK-проекта совмещена с версией модуля; source/sdk/abi/module.ver — это linker version script экспортов (символы InitModule и т.д.), а не метаданные версий; other/documents/v2-audit.md:186-188 прямо признаёт: 'Module version is a float in the MTA ABI ... plan §38 wants SDK/Module/ABI/MTA versions reported separately in diagnostics'.

## Что уже соответствует

- Release pipeline выполняет build для Windows MSVC, Windows MinGW и Linux: матрица presets win-msvc/win-mingw/linux-gcc (.github/workflows/release.yml:24-31), все три пресета определены в CMakePresets.json (configure/build/test presets)
- Перед публикацией выполняются тесты: `mta test <preset> all` → ctest без фильтра, т.е. все тесты — sdk_tests (Lua-харнесс) + module_config_parse + module_config_rejects_garbage (release.yml:69-70; cli.py cmd_test:375-379; файлы тестов существуют: other/tests/lua/harness.cpp, other/tests/lua/scripts/*.lua, other/tests/unit/*.cmake)
- Существование итогового DLL/SO проверяется: после build — output_binary(...).is_file(), при отсутствии return 1 (cli.py:342-347), и перед package — при отсутствии бинарника пересборка и die, если всё ещё нет (cli.py:559-566); упавший шаг → job падает → публикация не выполняется
- В релиз публикуются только бинарные артефакты: files: dist/* (release.yml:86-89), а cmd_package кладёт в dist/ ровно один бинарник модуля; полный SDK (заголовки/исходники) через GitHub Releases не распространяется; sha256 выводится только в лог, файлом не публикуется
- Автоматические префиксы sdk_/mta_/holo_ не добавляются: имя бинарника = SDK_MODULE_NAME дословно (CMakeLists.txt:226-229 OUTPUT_NAME/PREFIX ""), cmd_package берёт имя из config/module.toml как есть (cli.py:573), mta init записывает developer-имя дословно (cli.py:224)
- Публикация артефакта выполняется только после успешного завершения соответствующего build/test: build/test/package/publish — последовательные шаги одного job (release.yml:32-89), при падении любого шага оставшиеся (включая publish) пропускаются
- Шаги PHASE 14 build → test → integration → package → publish DLL/SO присутствуют в release.yml именно в этом порядке (строки 66-67, 69-70, 76-81, 83-84, 86-89)
- Module version и MTA compatibility не смешиваются там, где реализованы: единый источник версии module.toml → project(VERSION) → SDK_MODULE_VERSION → module_details/version.cpp; MTA-версия сервера получается отдельно от manager (GetVersionString/GetNetcodeVersion) и указывается в diagnostics отдельно от module_version (module.cpp:101-103, version.cpp:23-28)

## Рекомендации по устранению

1. Изменить `cmd_package` в other/tools/mta/cli.py (строка 573): копировать бинарник в dist/ под именем ровно `<module-name>.dll`/`.so` (например base.dll / base.so) вместо `<name>-<version>-<platform>-x64.dll`; при необходимости публиковать version/platform-суффиксы отдельным полем release notes, а не в имени файла.
2. Согласовать комментарий .github/workflows/release.yml:4-5 с фактическим именем публикуемого артефакта (после исправления — `<module-name>.dll / <module-name>.so — nothing else ships` станет истинным; до исправления — привести текст к действительности).
3. Сделать integration-тесты блокирующим шагом release pipeline перед publish: убрать `continue-on-error: true` из шага Integration (release.yml:72-81) и запускать его на всех трёх целях матрицы (win-msvc, win-mingw, linux-gcc), а не только на `win-mingw`.
4. Ввести отдельные SDK version и ABI version: определить константы/поля (например SDK_VERSION и ABI_VERSION) в source/sdk/, добавить их в binary metadata (module_details) и в diagnostics, чтобы все четыре версии (SDK, Module, ABI, MTA compatibility) выводились раздельно.
5. Развязать версию CMake-проекта с версией модуля: CMakeLists.txt:14 (`project(mta_sdk_module VERSION "${MODULE_CFG_module_version}")`) не должен совмещать версию SDK-шаблона с Module version из config/module.toml — задать SDK version собственным источником.
6. Устранить зафиксированный в own-документе пробел other/documents/v2-audit.md:186-188: реализовать раздельную отчётность SDK/Module/ABI/MTA версий в diagnostics и обновить документ по факту.

## Статус исправления (2026-09-02)

Несоответствие 1 устранено: в cmd_package добавлена одна опциональная опция `--release-name` (cli.py:573-578, 636-644), release.yml:87 пакует с ней, артефакт публикации называется ровно `&lt;module&gt;.dll`/`.so` (имя — из config/module.toml, без префиксов), комментарий release.yml:3-10 теперь соответствует факту. Несоответствие 2 устранено частично: `continue-on-error` убран из release.yml, integration (строки 80-84) — блокирующий шаг перед package/publish в порядке build → test → integration → package → publish, но остался на win-mingw: pinned-сервер harness Windows-only (other/server/mta_server.py, platform "windows", MTA Server64.exe) и собирает модуль default-пресетом win-mingw, поэтому на linux-gcc невыполним, а на win-msvc лишь дублировал бы mingw-прогон (§37 «relevant integration tests»). Несоответствие 3 (раздельные SDK/Module/ABI/MTA версии, §38) не устранялось — вне разрешённого объёма правок (только .github/workflows/*.yml + cmd_package). Подтверждено: parse_args всех вызовов против build_parser() и `yaml.safe_load(release.yml)` — YAML_OK.

**Дополнение: версии §38 (2026-09-02)**

Несоответствие 3 устранено полностью — четыре версии (SDK / Module / ABI / MTA server) разделены и выводятся раздельно в diagnostics и metadata. Проверено статическим сопоставлением файлов (перечисленные файлы перечитаны, номера строк сверены с актуальным кодом):

- Единый источник — `source/sdk/version.hpp`: `SDK_VERSION "1.0.0"` (строка 30) и `SDK_ABI_VERSION "1"` (строка 31); комментарий шапки фиксирует разделение четырёх сущностей §38 и перечень потребителей (C++ SDK, CMake `project(VERSION)`, CLI `mta doctor`) — значения нигде не дублируются литералами.
- `mta::SdkInfo` / `mta::sdk_info()` — source/sdk/native/module.hpp:35-39 (структура с полями `version` и `abi_version`) и :46 (declaration), реализация source/sdk/native/module.cpp:21-24 (`SdkInfo{SDK_VERSION, SDK_ABI_VERSION}`); compile-time, доступны до InitModule. Runtime-факты сервера вынесены в отдельную структуру `mta::ServerInfo` / `mta::server_info()` (version/netcode/OS, module.hpp:23-29/51, module.cpp:26-42) — Module/ABI версии с ними не смешиваются.
- Лог загрузки — source/sdk/abi/module.cpp:108-110: `module: loaded <name> (module <SDK_MODULE_VERSION>, sdk <SDK_VERSION>, abi <SDK_ABI_VERSION>; MTA <GetVersionString()>)` — четыре версии раздельно с подписями; InitModule float (module_details.version) остаётся только Module-версией — это вся поверхность, которую MTA server ABI определяет для binary metadata (комментарий module.cpp:103-107).
- CLI — `read_sdk_version_header` (other/tools/mta/cli.py:522-540) парсит version.hpp как единый источник; `cmd_doctor` выводит две раздельные проверки: «SDK version: sdk 1.0.0 (source/sdk/version.hpp)» и «ABI version: module-abi 1 (source/sdk/version.hpp)» (cli.py:574-589), рядом со строкой Project, несущей Module-версию, и строкой MTA server — doctor сообщает те же факты, что компилируются в сборку.
- CMakeLists.txt:16-20: `file(READ)` + regex достают SDK_VERSION из version.hpp для `project(mta_sdk_module VERSION ...)` — версия проекта (SDK-шаблона) развязана с версией модуля. Module version идёт отдельным путём: config/module.toml:16 (`version = "2.0.0"`) → SDK_MODULE_VERSION float `"2.0.0"` → `"2.0"` (CMakeLists.txt:141-145) → binary metadata/InitModule.
- cmake/install.cmake:27-29: имя CPack-пакета (`CPACK_PACKAGE_FILE_NAME`) собирается из MODULE version — ZIP является module-артефактом, что зафиксировано §38-комментарием; project(VERSION) на имя пакета не влияет.
- `sample_version` (source/functions/info/version.cpp:8-34) возвращает отдельные поля: `module`/`module_author`/`module_version`, `sdk_version`, `abi_version` и (при подключённом module manager) `mta`/`netcode`/`os` — все сущности §38 видны из Lua раздельно.

Тем самым реализованы рекомендации 4 и 5 раздела «Рекомендации по устранению»; зафиксированный в v2-audit.md пробел раздельной отчётности версий закрыт на уровне кода и diagnostics (обновление самого v2-audit.md — вне объёма этого дополнения).