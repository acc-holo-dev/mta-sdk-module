-- New capabilities: userdata, events, table helpers.

-- userdata: counter object with methods
local c = counter_create(42)
test_assert(c:get() == 42, "counter get")
c:set(100)
test_assert(c:get() == 100, "counter set")
test_assert(c:add(5) == 105, "counter add returns new value")
test_assert(c:get() == 105, "counter add persisted")

-- stable type identity (plan §16): type validation names the declared type
local unbound_get = c.get
local ok_t, err_t = pcall(unbound_get, 42)
test_assert(ok_t == false, "calling get without a counter fails")
test_assert(tostring(err_t):find("counter", 1, true) ~= nil,
    "the error names the declared type: " .. tostring(err_t))

-- events: the module triggers a Lua event
local events = {}
root = {}
function triggerEvent(name, source, ...)
    events[#events + 1] = {name = name, args = {...}}
end
sample_trigger_event("onTest", 1, "two")
test_assert(events[1] ~= nil, "event triggered")
test_assert(events[1].name == "onTest", "event name")
test_assert(events[1].args[1] == 1 and events[1].args[2] == "two", "event args")

-- table helpers: reading fields
local name, hp = sample_table_get({name = "Alice", hp = 100})
test_assert(name == "Alice" and hp == 100, "table get fields")

-- table helpers: default when the field is absent
local name2, hp2 = sample_table_get({})
test_assert(name2 == "unknown" and hp2 == 0, "table get defaults")

-- table helpers: writing a field
local t = sample_table_set({hp = 50}, "Alice")
test_assert(t.name == "Alice" and t.hp == 50, "table set field")
