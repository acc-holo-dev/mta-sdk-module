-- Новые возможности: userdata, события, хелперы таблиц.

-- userdata: объект-счётчик с методами
local c = counter_create(42)
test_assert(c:get() == 42, "counter get")
c:set(100)
test_assert(c:get() == 100, "counter set")
test_assert(c:add(5) == 105, "counter add returns new value")
test_assert(c:get() == 105, "counter add persisted")

-- события: модуль триггерит Lua-событие
local events = {}
root = {}
function triggerEvent(name, source, ...)
    events[#events + 1] = {name = name, args = {...}}
end
sample_trigger_event("onTest", 1, "two")
test_assert(events[1] ~= nil, "event triggered")
test_assert(events[1].name == "onTest", "event name")
test_assert(events[1].args[1] == 1 and events[1].args[2] == "two", "event args")

-- хелперы таблиц: чтение полей
local name, hp = sample_table_get({name = "Вася", hp = 100})
test_assert(name == "Вася" and hp == 100, "table get fields")

-- хелперы таблиц: дефолт при отсутствии поля
local name2, hp2 = sample_table_get({})
test_assert(name2 == "unknown" and hp2 == 0, "table get defaults")

-- хелперы таблиц: запись поля
local t = sample_table_set({hp = 50}, "Петя")
test_assert(t.name == "Петя" and t.hp == 50, "table set field")
