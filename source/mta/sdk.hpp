#pragma once

// ===========================================================================
// <mta/sdk.hpp> -- the public facade of the MTA Module SDK (plan §42/§43).
//
// This is the ONLY header a module developer normally needs:
//
//     #include <mta/sdk.hpp>
//
//     MTA_FUNCTION("sum", [](double a, double b) { return a + b; });
//
// Everything below is grouped by subsystem. The directories under
// source/sdk/ (abi, lua, bind, registry, runtime, resources, objects,
// events, logging) are internal implementation layers: they exist so the
// framework can be maintained, not because a developer has to understand
// them. Internal headers stay reachable through the same include root, but
// the facade is the supported surface.
//
// Exported here (additions land together with their subsystem):
//   MTA_FUNCTION / MTA_LUA_FUNCTION / MTA_LUA_FUNC   -- function registration
//   mta::lua::args / push_results / raise_error       -- values & errors
//   mta::lua::{Argument, Table, Arguments}            -- snapshots (async-safe)
//   mta::lua::{check_*, opt_*}                        -- manual stack access
//   mta::async::Callback / mta::async::Scheduler      -- background work
//   mta::resources::{Hub, Store}                      -- per-resource state
//   mta::log::{info, warn, error, debug}              -- logging
//   mta::events::trigger                              -- module -> Lua events
//   mta::userdata::Registry<T> + MTA_METHOD           -- native objects
// ===========================================================================

// --- registration (macros live in the registry) -----------------------------
#include "sdk/registry/registry.hpp"

// --- Lua values: snapshots and stack helpers --------------------------------
#include "sdk/lua/argument.hpp"
#include "sdk/lua/arguments.hpp"
#include "sdk/lua/stack.hpp"
#include "sdk/lua/table_helpers.hpp"
#include "sdk/lua/protect.hpp"

// --- runtime services ---------------------------------------------------------
#include "sdk/runtime/callback.hpp"
#include "sdk/runtime/scheduler.hpp"
#include "sdk/resources/resources.hpp"
#include "sdk/logging/logging.hpp"
#include "sdk/events/events.hpp"

// --- native objects -----------------------------------------------------------
#include "sdk/objects/userdata.hpp"