// The plan-style facade: <mta/sdk.hpp> + MTA_FUNCTION with a lambda.
// The function registers under exactly the given name -- no prefixes.

#include <mta/sdk.hpp>

#include <string>

MTA_FUNCTION("sample_hello",
    [](std::string name)
    {
        return "Hello, " + name;
    });

// The described form: MTA_FUNCTION(name, "description", function).
MTA_FUNCTION("sample_hello_desc", "Greets politely.",
    [](std::string name)
    {
        return "Good evening, " + name;
    });