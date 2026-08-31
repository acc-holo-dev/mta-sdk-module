// Пер-ресурсное состояние: счётчик обращений ресурса. Сбрасывается
// автоматически при остановке ресурса (ResourceStopped).

#include "registry/registry.hpp"
#include "runtime/resources.hpp"

namespace
{
struct Session
{
    int hits = 0;
};

mta::resources::Store<Session> g_sessions;
} // namespace

MTA_LUA_FUNCTION("sample_session_hit",
    "Счётчик обращений ресурса; сбрасывается при остановке ресурса.")
{
    Session &session = g_sessions.for_state(L);
    ++session.hits;
    return mta::lua::push_results(L, static_cast<lua_Number>(session.hits));
}
