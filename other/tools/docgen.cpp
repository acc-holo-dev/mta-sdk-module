// Registry documentation generator (plan §9/§24): dumps every module
// function registered through MTA_FUNCTION/MTA_LUA_FUNCTION with its
// signature metadata as markdown. Consumed by `mta docs`.
//
// The specs are added by the registration macros' static initializers, so
// simply linking the module sources populates the registry -- no Lua VM or
// module manager is needed here.

#include "sdk/registry/registry.hpp"

#include <cstdio>
#include <string>

int main()
{
    const auto &functions = mta::registry::Registry::instance().functions();
    if (functions.empty())
    {
        std::printf("no functions registered\n");
        return 1;
    }

    std::printf("# Module functions\n\n");
    std::printf("%zu function(s) registered through MTA_FUNCTION.\n\n", functions.size());

    for (const auto &fn : functions)
    {
        std::printf("## %s\n\n", fn.name);

        if (fn.description != nullptr && *fn.description != '\0')
        {
            std::printf("%s\n\n", fn.description);
        }
        else
        {
            std::printf("_(no description)_\n\n");
        }

        if (!fn.signature.derived)
        {
            std::printf("```\n%s(...)\n```\n\n", fn.name);
            std::printf("Signature not derived: body-style registration reads its arguments "
                        "itself (plan §9).\n\n");
            continue;
        }

        std::string signature = fn.name;
        signature += "(";
        for (std::size_t i = 0; i < fn.signature.arguments.size(); ++i)
        {
            if (i != 0)
            {
                signature += ", ";
            }
            signature += fn.signature.arguments[i].type;
            if (fn.signature.arguments[i].optional)
            {
                signature += "?";
            }
        }
        signature += ")";
        if (fn.signature.variadic)
        {
            signature += " (variadic)";
        }
        if (!fn.signature.returns.empty())
        {
            signature += " -> ";
            for (std::size_t i = 0; i < fn.signature.returns.size(); ++i)
            {
                if (i != 0)
                {
                    signature += ", ";
                }
                signature += fn.signature.returns[i];
            }
        }

        std::printf("```\n%s\n```\n\n", signature.c_str());
        if (fn.signature.variadic)
        {
            std::printf("Accepts trailing arguments.\n\n");
        }
    }

    return 0;
}