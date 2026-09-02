# Problem 5 — Отсутствует модель LuaView: реализована только Snapshot-половина дихотомии «View vs Snapshot»

**Задача:** `other/tasks/Task_14.md` — Lua-значения: View vs Snapshot
**Вердикт:** Проект не соответствует задаче (статическая проверка, 2026-09-02)
**Серьёзность:** P1 — значимые нереализованные требования: именованная модель LuaView отсутствует и оба затронутых пункта чек-листа выполнены лишь частично, при этом критические требования потокобезопасности (async только через owned Snapshot) соблюдены.

## Суть проблемы
SDK реализует owned Snapshot-модель (Argument/Table/Arguments) и async-конвейер, строго работающий с owned values: worker-функции не получают lua_State, результаты передаются как Arguments-снапшоты, доставка выполняется на главном потоке. Однако вторая именованная модель задачи — LuaView (borrowed) — в проекте отсутствует: borrowed-доступ для synchronous операций реализован только свободными функциями над raw lua_State* (stack.hpp, bind.hpp), без типа/модели LuaView, как требует §18/§45. Из шести пунктов чек-листа два выполнены частично (PARTIAL), остальные четыре выполнены полностью, поэтому итог — NON_COMPLIANT.

## Несоответствия

### 1. В SDK реализованы две разделённые модели Lua-значений: LuaView и Snapshot
- **Статус:** PARTIAL
- **Требуется:** Требуется (PROMT.md §18 и §45: «Предусмотреть: LuaView / Snapshot»), чтобы в SDK существовали обе именованные модели Lua-значений.
- **Фактически:** Реализована только owned Snapshot-модель (Argument/Table/Arguments); тип/абстракция LuaView в SDK отсутствует — borrowed-доступ существует лишь как свободные функции над raw lua_State* (stack.hpp, bind.hpp), без выделенной модели LuaView.
- **Доказательство:** grep по source/ на `LuaView|View` — 0 совпадений в коде. Snapshot-модель реализована: `source/sdk/lua/argument.hpp:32-79` (класс Argument с owned variant-хранилищем, без члена lua_State), `argument.hpp:26-30` (структура Table: array+fields), `arguments.hpp:13-41` (Arguments); `source/mta/sdk.hpp:22`: «mta::lua::{Argument, Table, Arguments} -- snapshots (async-safe)»; `other/documents/v2-audit.md:76`: «mta::lua::{Argument, Table, Arguments} (snapshots) | KEEP -> becomes the "Snapshot" half of §18».

### 2. LuaView — borrowed access к текущему Lua state — используется для synchronous operations
- **Статус:** PARTIAL
- **Требуется:** Синхронные операции должны выполняться через модель LuaView — borrowed-доступ к текущему Lua state, как предписано задачей (§18, §45).
- **Фактически:** Семантика соблюдена: synchronous operations выполняются через borrowed-доступ к стеку текущего lua_State. Но требуемая модель LuaView как отдельный компонент SDK не реализована — доступ оформлен функциями над raw lua_State*, а не типом/моделью LuaView.
- **Доказательство:** borrowed-доступ для sync-операций есть: `source/sdk/lua/stack.hpp:99-249` (check_number/check_boolean/check_string/... принимают lua_State* L), `source/sdk/bind/bind.hpp:664` (args&lt;Ts...&gt;(lua_State*)), `protect.hpp:59-85`, `bind.hpp:70-74` (context { lua_State* vm; }); при этом сущность с именем LuaView в проекте отсутствует (grep — 0 совпадений в source/).

## Что уже соответствует
- Owned/copyable Snapshot-модель Lua-значений реализована: Argument/Table/Arguments не хранят lua_State, таблицы копируются рекурсивно с защитой от циклов (max_table_depth=32) — `source/sdk/lua/argument.hpp`, `argument.cpp:145-226`, `arguments.hpp`.
- Передача в worker выполняется через Snapshot, а не через Lua state: Scheduler::post_task/run принимают std::function&lt;mta::lua::Arguments()&gt; и completion(const mta::lua::Arguments&, const char*) — `scheduler.hpp:61-64, 103-104`; примеры: `source/functions/async/async_add.cpp:19-33`, `task_demo.cpp:34-49` (work-лямбды не трогают Lua).
- Отсутствует передача raw Lua state/value references между потоками: TaskJob/Completion (`scheduler.cpp:57-73`) не содержат lua_State; run() преобразует lua_State* в имя ресурса (std::string) на главном потоке (`scheduler.cpp:382-393`); Callback хранит имя ресурса + generation вместо raw state — `callback.hpp:6-20` («Storing a raw lua_State* ... is forbidden»).
- Worker threads не обращаются к borrowed Lua values: worker_loop исполняет только job.work() без Lua (`scheduler.cpp:147-209`); `scheduler.hpp:4-9` фиксирует «The Lua VM is NOT thread-safe ... Lua is never called from a worker thread»; Argument::read/push требуют явный lua_State и вызываются только в main-thread путях.
- Async автоматически работает только с thread-safe owned values на уровне типов API: сигнатуры допускают только mta::lua::Arguments, доставку результатов pump() выполняет на главном потоке (`scheduler.cpp:248-290`); модель Lua table (§45) представлена owned Table-снапшотом (array+fields, `argument.hpp:26-30`) с helpers get_field/set_field (`table_helpers.hpp`).

## Рекомендации по устранению
1. Ввести в SDK явную именованную модель LuaView — borrowed-обёртку над текущим lua_State* для synchronous operations, чтобы выполнялось требование PROMT.md §18/§45 о наличии обеих моделей «LuaView / Snapshot».
2. Оформить существующий borrowed-доступ под модель LuaView: функции чтения стека (check_number/check_boolean/check_string/... в `source/sdk/lua/stack.hpp:99-249`), args&lt;Ts...&gt;(lua_State*) (`source/sdk/bind/bind.hpp:664`) и protect (`protect.hpp:59-85`) — сохранив их семантику, но привязав к выделенному типу, а не к raw lua_State* в сигнатурах.
3. Явно разделить и задокументировать зоны применения моделей: LuaView — только synchronous операции на главном потоке; Snapshot (Argument/Table/Arguments) — перенос значений через async-конвейер и между потоками (как уже зафиксировано в `source/mta/sdk.hpp:22`).
4. Гарантировать, что LuaView не выходит за главный поток и не попадает в async-пути: TaskJob/Completion (`scheduler.cpp:57-73`), run() (`scheduler.cpp:382-393`) и Callback (`callback.hpp:6-20`) должны по-прежнему оперировать только mta::lua::Arguments и именем ресурса + generation.
5. Обновить документацию (`source/mta/sdk.hpp`, `other/documents/`) с описанием обеих моделей и их границ, после чего перепроверить критерий задачи: grep по source/ на `LuaView|View` (сейчас — 0 совпадений) должен находить реализованную модель LuaView.