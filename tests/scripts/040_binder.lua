-- Typed binder: C++ defaults, optional, tuples, vectors, variadics and
-- RAW mode.

-- default straight in the signature
test_assert(sample_greet("World") == "hello, World", "default argument used")
test_assert(sample_greet("World", "hi") == "hi, World", "default overridden")

-- std::optional
test_assert(sample_tag("x") == "x/none", "optional omitted -> nullopt")
test_assert(sample_tag("x", "t") == "x/t", "optional provided")

-- tuple -> two results
local mn, mx = sample_minmax(3, 1)
test_assert(mn == 1 and mx == 3, "pair -> two results")

-- vector expands into a result list
local r = {sample_range(2, 5)}
test_assert(#r == 4 and r[1] == 2 and r[4] == 5, "vector -> multiple results")

-- variadic tail
local a, b, c = sample_echo(1, "x", true)
test_assert(a == 1 and b == "x" and c == true, "rest_args roundtrip")

-- RAW mode: direct stack
local n, t1, t2 = sample_stack_dump(1, "x")
test_assert(n == 2 and t1 == "number" and t2 == "string", "raw stack dump")

-- typing errors straight from the signature
local ok, err = pcall(sample_greet, {})
test_assert(not ok, "non-string raises")
test_assert(err and string.find(err, "must be a string", 1, true) ~= nil,
            "typed error message from signature")

local ok2, err2 = pcall(sample_range, "x", 1)
test_assert(not ok2 and string.find(err2, "must be an integer", 1, true) ~= nil,
            "integer type error")

-- missing required argument
local ok3, err3 = pcall(sample_minmax, 5)
test_assert(not ok3 and string.find(err3, "got no value", 1, true) ~= nil,
            "missing required argument reports typed error")
