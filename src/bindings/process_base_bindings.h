#pragma once
#include "gmThread.h"
#include "gmMachine.h"

// Every process table gets a handful of native methods injected onto it by
// the engine before constructor()/init() run -- confirmed real methods
// (setupPreDrawList, setupPostDrawList, getPreDrawList, getPostDrawList) are
// in video_bindings.cpp since they're really "give me a drawList".
//
// Also injects the SAME process-handle methods CreateProcessHandle puts on
// the separate `p` object passed into the factory (isInited/terminate/
// setIsStealsInputFocus/getObjectID/getTable/etc, process_handle_bindings.cpp)
// directly onto the factory's own RETURNED table (via the same `__id`
// convention those methods already look up by) -- confirmed necessary by
// real usage, not just the `t.Process=p;` convention: e.g.
// Platformer/MainMenu5PS3/MainMenu5PS3.gm calls `this.isInited()` and
// `this[i].isInited()` (on a pushed `GameSystem.runProcess(...)` return
// value) directly on the process's own table, never through a
// separately-stored `.Process` handle. Without this, every real script
// that calls `.isInited()`/etc directly on its own `this` (or on a stored
// sub-process's returned table) got a "setdot failed"-free but silently
// no-op call (see gmThread.cpp's null-call leniency patch) -- e.g.
// MainMenu5PS3.gm's `this.processes.areInited()` guard for its one-time
// input setup block (`setupInputPS3MainMenu(this)`, wiring up onKeyPress)
// never actually passed, so onKeyPress was never registered at all.
void InjectProcessBaseMethods(gmMachine& a_machine, gmTableObject* a_table, int a_processId);
