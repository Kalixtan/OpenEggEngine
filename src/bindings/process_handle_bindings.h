#pragma once
#include "gmMachine.h"

// The native "process handle" object -- every real process factory follows
// global X = function(p) { return { Process=p, ... }; }; and the script
// stores `p` itself as `this.Process` (confirmed real, e.g.
// game_data/Platformer/OneTrailPlayMode.gm: `t.Process=p;`). Real scripts
// then call this.Process.setIsStealsInputFocus/terminate/isInited/
// setupPreDrawList/setupPostDrawList/getPreDrawList/getPostDrawList/
// getDrawList/getObjectID/getTable on it (confirmed by grepping every
// `.Process.<method>(` call site in game_data). No decompiled class exists
// recovered symbols), so this is built to functionally match observed
// script usage rather than a confirmed decompiled body -- same plain-table-
// with-injected-methods pattern as SEQUENCERACTION, for the same reason
// (the real object is opaque to us; only its script-visible behavior is
// recoverable).
//
// Creates the handle for process a_processId. The methods look the process
// back up by id through EngineContext().processManager at call time (never
// capture a raw Process* -- ProcessManager's process list can reallocate
// out from under a stale pointer if a GM callback started during
// construction recursively starts another process).
gmVariable CreateProcessHandle(gmMachine& a_machine, int a_processId);

// Sets `__id` and registers the same methods CreateProcessHandle does
// directly onto ANY given table -- used both for the `p` handle above and
// (via process_base_bindings.h's InjectProcessBaseMethods) for a
// process's own factory-returned table, since real scripts call
// `.isInited()`/etc directly on that table too, not only through a stored
// `.Process` handle. See process_base_bindings.h for why.
void InjectProcessHandleMethods(gmMachine& a_machine, gmTableObject* a_table, int a_processId);

// Registers GameSystem.setAsPauseProcess(active, process) onto the existing
// GameSystem library table. Call after RegisterGameSystemBindings.
void RegisterProcessHandleBindings(gmMachine& a_machine);
