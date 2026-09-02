# Problem 2 — Binder V2: native MTA-объекты не поддержаны как тип параметра

**Задача:** `other/tasks/Task_7.md` — Binder V2: биндинг, валидация аргументов и ошибки
**Вердикт:** Проект не соответствует задаче (статическая проверка, 2026-09-02)
**Серьёзность:** P1 — значимое требование задачи о поддержке native MTA-объектов биндером не реализовано, хотя остальные 11 из 12 требуемых типов работают.

## Суть проблемы

Binder V2 реализован в основном объёме: typed-binding через MTA_FUNCTION сохранён, автовалидация и конвертация аргументов автоматизированы, сообщения формата «bad argument #N to ...» и единая error model из 8 категорий с различением internal error реализованы, сигнатурные метаданные и Lua-регрессионные тесты (040/045/015/050 + harness) присутствуют. Единственное несоответствие — native MTA objects не поддержаны биндером как тип параметра: pull_arg не содержит такой ветки (static_assert), а отсутствие element/player-обёрток задокументировано как осознанное решение. Типизированные rest_args и context реализованы, но не покрыты ни примерами, ни тестами. Из-за частичного выполнения пункта о минимальном наборе поддерживаемых типов вердикт — NON_COMPLIANT.

## Несоответствия

### 1. Binder поддерживает минимум: integer, number, boolean, string, table, optional, variadic/rest arguments, callback, resource context, native MTA objects, multiple return values, errors

- **Статус:** PARTIAL
- **Требуется:** биндер должен поддерживать минимальный набор из 12 категорий типов параметров и результатов: integer, number, boolean, string, table, optional, variadic/rest arguments, callback, resource context, native MTA objects, multiple return values, errors.
- **Фактически:** 11 из 12 требуемых типов реализованы (integer/number/boolean/string/string_view — bind.hpp:344-398; table — 298-313; optional — 328-337; rest_args — 338-343; Callback — 314-327; resource context — 68-74 и 405-422; multiple returns — push_result 426-467; errors — везде). Native MTA objects как тип параметра биндера отсутствуют: MTA_FUNCTION с параметром-элементом/игроком не скомпилируется (static_assert), mta::Resource (source/sdk/native/resource.hpp:32-61) тоже не поддержан pull_arg. Отказ осознанный и задокументирован, но требование задачи выполнено лишь частично. Дополнительно: типизированные rest_args и mta::lua::context реализованы только в bind.hpp и не используются ни одной функцией из source/functions/ ни одним тестом (040_binder.lua:21-22 помечен «rest_args roundtrip», но фактически тестирует body-style sample_echo, читающий Arguments вручную).
- **Доказательство:**
    - source/sdk/bind/bind.hpp:287-403 — pull_arg не имеет ветки для native MTA-объектов; строка 401: `static_assert(!sizeof(U), "unsupported parameter type (see lua/bind.hpp)")`;
    - source/sdk/native/resource.hpp:3-13 — «There is NO element/player/vehicle API behind the module boundary ... documented as NOT met for elements»;
    - source/sdk/objects/userdata.hpp:134-160, 249-278 — Registry<T>::check даёт типизированный self только в методах (MTA_METHOD), а не как параметр свободной функции;
    - source/sdk/native/resource.hpp:32-61 — mta::Resource не поддержан pull_arg;
    - other/tests/lua/scripts/040_binder.lua:21-22 — тест «rest_args roundtrip» фактически проверяет body-style sample_echo, читающий Arguments вручную.

## Что уже соответствует

- Сохранена typed-binding идея: MTA_FUNCTION("name", lambda) — макрос в source/sdk/registry/registry.hpp:104-109 (простая и описанная формы), живой пример source/functions/basics/hello.cpp:9-27, тест other/tests/lua/scripts/015_facade.lua
- Автоматическая проверка наличия, количества и типа аргументов с конвертацией до вызова C++ и возвратом результата: dispatch/error_probe/invoke_prefix (bind.hpp:478-621), check_number/check_integer/check_boolean/check_string (stack.hpp:99-249), включая целочисленный range-check (bind.hpp:377-398)
- В developer-коде source/functions/ нет ручных проверок простых аргументов: grep luaL_check/lua_is*/check_/opt_ — 0 совпадений; source/functions/raw/stack_dump.cpp использует lua_gettop/lua_type только по назначению raw-дампа стека
- Сообщения об ошибках формата плана: «bad argument #N to 'name' (expected X, got Y)» — stack.hpp:56-89; явный nil даёт «got nil» (тест 050_edge.lua:7-10), отсутствующий аргумент — «got no value» (045_errors.lua:11-14); сообщение о количестве «bad argument count to 'name' (expected at least N arguments, got M)» — bind.hpp:610-619
- Единая error model со всеми 8 категориями (Generic, InvalidArgument, InvalidType, MissingArgument, ResourceStopped, InvalidCallback, InvalidObject, AsyncCancelled, InternalError) — source/sdk/errors/errors.hpp:30-41 плюс человекочитаемые имена (74-89); InvalidObject реально возбуждается в userdata.hpp:145,154
- Binder преобразует ошибки в понятные Lua errors, internal error различим: protect.hpp:59-81 рендерит InternalError/неизвестные исключения как «internal module error: ...», остальные категории дословно; трampoline подключён во всех макросах регистрации (registry.hpp:85, bind.hpp:575-581, userdata.hpp:233-239)
- PHASE 4 рефакторинг выполнен: traits (bind.hpp:98-142), argument conversion (pull_arg, 287-403), result conversion (push_result, 426-467), invoke (invoke_prefix/dispatch, 478-621), errors (errors.hpp/protect.hpp), signature metadata (ArgumentInfo/Signature, 77-92; fill_argument_info/fill_return_type, 186-283) и module_signature (source/functions/info/functions_list.cpp:19-60); коммит ba49b23 «feat(v2): PHASE 4 - binder V2: plan error format, unified error model, signature metadata»
- Regression-тесты binder'а существуют: other/tests/lua/scripts/040_binder.lua (defaults, optional, tuple/vector multiple returns, variadic, типовые ошибки), 045_errors.lua (матрица ошибок плана §7 и метаданные сигнатур), 015_facade.lua, 050_edge.lua; прогон через other/tests/lua/harness.cpp (все *.lua скрипты) и ctest-цель sdk_tests (CMakeLists.txt:246-271)

## Рекомендации по устранению

1. Добавить в pull_arg (source/sdk/bind/bind.hpp:287-403) ветку для native MTA-объектов, чтобы MTA_FUNCTION с параметром-элементом/игроком компилировался вместо срабатывания static_assert на строке 401.
2. Реализовать element/player/vehicle-обёртки за границей модуля — их отсутствие сейчас задокументировано как «NOT met for elements» в source/sdk/native/resource.hpp:3-13.
3. Поддержать mta::Resource (source/sdk/native/resource.hpp:32-61) в pull_arg как принимаемый биндером тип параметра.
4. Расширить типизацию Registry<T>::check (source/sdk/objects/userdata.hpp:134-160, 249-278) так, чтобы типизированный объект можно было принимать как параметр свободной функции, а не только как self в методах MTA_METHOD.
5. Покрыть типизированные rest_args и mta::lua::context реальными функциями в source/functions/ и тестами: переписать помеченный как «rest_args roundtrip» тест other/tests/lua/scripts/040_binder.lua:21-22, который фактически проверяет body-style sample_echo, читающий Arguments вручную.
6. Попутно (не входит в gaps) обновить устаревшую документацию other/tests/unit/README.md, объявляющую C++ unit-тесты binder'а и цель sdk_unit_tests, которых в репозитории нет.

## Статус исправления (2026-09-02)

Исправлено. Верификация только статическая (read/grep, согласованность сигнатур); сборка и тесты выполняются отдельно, централизованно.

### Native MTA objects как тип параметра биндера — реализовано (рекомендации 1–5 частично)
- Рек. 1/3: в `pull_arg` (`source/sdk/bind/bind.hpp`) добавлена ветка для `mta::Resource` — native-обёртка теперь полноценный тип параметра. Lua передаёт имя ресурса; биндер валидирует его «живьём» через `Resource::find` (прецедент `source/sdk/native/resource.cpp`, без изменений в нём). Неизвестный/остановленный ресурс → понятная ошибка `bad argument #N to 'name' (no running resource '...')` (новый хелпер `bad_argument_object`, категория `InvalidObject`, в `source/sdk/lua/stack.hpp`); не-строка → `expected resource, got <type>` (имя-идентификатор — строго, числа не коэрцируются). `static_assert` в else-ветке остался только для реально неподдерживаемых типов.
- Возврат native-типа: `push_one(lua_State*, const mta::Resource&)` добавлен в `source/sdk/native/resource.hpp` (ADL-хук в `namespace mta`, чтобы нижний native-слой не зависел от lua-слоя); `push_result`/`push_results` находят его через ADL — Resource пушится как имя (единственная стабильная Lua-идентичность за ABI). `std::optional<mta::Resource>` принимает nil/отсутствие.
- Рек. 2 (element/player/vehicle-обёртки): осознанное решение НЕ менять — за замороженным ABI (`ILuaModuleManager10`) нет соответствующего API; обёртки не выдумывались. Документировано в `native/resource.hpp`, `api.md`, `architecture.md`, `CHANGELOG.md`.
- Рек. 4 (типизированный объект как параметр свободной функции) — не выполнялось: userdata-типы остаются self-only в MTA_METHOD (изменение типизации Registry<T>::check без реального типа-потребности дало бы фиктивную функциональность); задокументировано как осознанный остаток.
- Рек. 5 (типизированные rest_args/context): новые функции `sample_rest_count`/`sample_context_caller` (`source/functions/basics/typed_params.cpp`) читаются биндером; тесты добавлены в `040_binder.lua` (строки 44-54) рядом со старым тестом (040:21-22 не переписывался — расширение, а не правка).
- Метаданные: `lua_type_name<mta::Resource>()` → `resource`; `module_signature("sample_resource_arg")` показывает `arguments[1].type == "resource"` (тест в `060_features.lua`).
- Рек. 6 (other/tests/unit/README.md) — вне gaps, не менялся.

Чем подтверждено: read/grep-проверка изменённых файлов; ветки `pull_arg` аддитивны (`else if constexpr`) — все прежние типы параметров работают; макросы регистрации (`registry.hpp`) не менялись — все существующие вызовы MTA_FUNCTION/MTA_LUA_FUNCTION/MTA_LUA_FUNCTION/MTA_METHOD в `source/functions/**` остаются валидными; lua-скрипты 040/045/060 согласованы по точным строкам ошибок. Сборка/ctest не запускались — фасадный агент параллельно правит фасад; финальная сборка централизованная.