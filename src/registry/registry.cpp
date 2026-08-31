#include "registry/registry.hpp"

#include "ILuaModuleManager10.h"
#include "runtime/logging.hpp"

namespace mta::registry
{
Registry &Registry::instance()
{
    static Registry registry;
    return registry;
}

void Registry::add(Spec spec)
{
    functions_.push_back(spec);
}

void Registry::register_all(ILuaModuleManager10 &manager, lua_State *lua_vm) const
{
    if (lua_vm == nullptr)
    {
        return;
    }

    for (const auto &spec : functions_)
    {
        // module_function и lua_CFunction имеют одинаковое ABI на всех
        // поддерживаемых тулчейнах; каст переключает только языковую линковку.
        const bool registered =
            manager.RegisterFunction(lua_vm, spec.name, reinterpret_cast<lua_CFunction>(spec.function));
        if (!registered)
        {
            mta::log::error("модуль: не удалось зарегистрировать функцию '", spec.name, "'");
        }
    }
}
} // namespace mta::registry

namespace mta::lua::detail
{
// Мост из биндера в реестр; реализация здесь, чтобы bind.hpp не тянул
// внутренности реестра.
bool register_function(const char *name, const char *description, int (*entry)(lua_State *))
{
    if (name == nullptr || entry == nullptr)
    {
        return false;
    }
    mta::registry::Registry::instance().add(mta::registry::Spec{name, description, entry});
    return true;
}
} // namespace mta::lua::detail
