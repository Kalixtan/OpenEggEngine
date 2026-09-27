#pragma once
#include <string>
#include <vector>
#include "gmThread.h"
#include "gmMachine.h"

// The incremental GC (GM_USE_INCGC, gmConfig.h) only scans roots it knows
// about: thread stacks, the globals table, and anything registered via
// AddCPPOwnedGMObject (gmMachine::ScanRootsCallBack, gmMachine.cpp) -- it
// has no way to see gmVariables held inside plain C++ structs like
// Process::table below. Confirmed the hard way: a large script (see
// game_data_testcart/) compiles enough new allocations in one pass to
// trigger an incremental GC step between a process's constructor() and
// init(), and without this, the process's own table gets swept out from
// under it -- init()/update() then silently can't find any of the
// process's own methods (BeginTableFunction just returns false, table
// enumerates as empty) because the table object was freed. Any gmVariable
// a C++ struct holds onto past the call frame that produced it needs this.
// Idempotent in both directions: the underlying registry is a plain set
// (gmMachine::m_cppOwnedGMObjs), not a refcount, and AddCPPOwnedGMObject
// asserts (GM_DEBUG_BUILD) if called twice for the same object -- e.g. the
// same table can legitimately end up passed to RootGmVariable more than
// once (a process's own table doubling as its "handle" now that
// InjectProcessHandleMethods puts handle methods directly on it too, see
// process_base_bindings.h).
inline void RootGmVariable(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.IsReference() && !a_machine.IsCPPOwnedGMObject(a_machine.GetObject(a_var.m_value.m_ref))) {
        a_machine.AddCPPOwnedGMObject(a_machine.GetObject(a_var.m_value.m_ref));
    }
}
inline void UnrootGmVariable(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.IsReference() && a_machine.IsCPPOwnedGMObject(a_machine.GetObject(a_var.m_value.m_ref))) {
        a_machine.RemoveCPPOwnedGMObject(a_machine.GetObject(a_var.m_value.m_ref));
    }
}

// Owns every live script "process" and drives its lifecycle hooks each tick
// -- this is the native counterpart to the retail GameSystem.* API confirmed
// init() to return a boolean. Assuming true." plus the reserved-key list:
// init, destructor, update, postUpdate, preDraw, postDraw, initialDraw,
// finalDraw, onEntityCollide*, onTouchDown/Up, constructor).
//
// Scope for this milestone: constructor / init / update / preDraw /
// destructor. postDraw/collision hooks are still stubbed in for later --
// nothing in the real corpus needed them to reach the live render loop.
struct Process {
    int id = 0;
    std::string name;
    gmVariable table; // GM_TABLE -- the object returned by e.g. Main(p)

    // constructor() has run and `table` is safe for the caller to reach
    // into (e.g. call .configure(...) on it) -- true as soon as
    // StartNewProcessList()/RunProcess() returns.
    bool constructed = false;
    // init() hasn't run yet. Deliberately NOT run inside FinishBringUp
    // alongside constructor() -- confirmed real convention,
    // Platformer/PopupProcess.gm's own header comment: "1. start the
    // process  2. call configure". Callers are expected to synchronously
    // configure the just-returned table before init() ever sees it (e.g.
    // PopupProcessBase.gm's init() unconditionally reads
    // this.popupsTable, which only exists after configure() sets it) --
    // running init() immediately, as this engine originally did, breaks
    // that ordering. Consumed by ProcessManager::UpdateAll's init pass.
    bool needsInit = true;
    // init() has run (successfully or not "assume true")
    // update()/preDraw() only run once this is true.
    bool initialized = false;
    bool wantsQuit = false;

    // Backing state for the native process handle (`p`/`this.Process`,
    // see process_handle_bindings.h) -- confirmed real methods
    // setIsStealsInputFocus/setupPreDrawList/setupPostDrawList/
    // getPreDrawList/getPostDrawList read and write these.
    bool stealsInputFocus = false;
    // Entity.c 0x0009de10, a plain per-object bool flag), real call site
    // Main.gm:169 `this.setIsUpdatesEntities(true);` in the main process's
    // own constructor. Bookkeeping only, same as isBeingNice/
    // isShouldUpdateEntities (game_system_state.h) -- this engine's entity
    // update pass (UpdateEntitiesWithNativeUpdateHook) isn't scoped
    // per-process, so nothing reads this back yet.
    bool updatesEntities = true;
    // gmVariable's default ctor leaves m_type uninitialized unless
    // GMMACHINE_NULL_VAR_CTOR is defined (it isn't here) -- must nullify
    // explicitly rather than relying on the default member initializer.
    gmVariable preDrawList = gmVariable::s_null;  // until setupPreDrawList() is called
    gmVariable postDrawList = gmVariable::s_null; // until setupPostDrawList() is called
};

class ProcessManager {
public:
    explicit ProcessManager(gmMachine& a_machine);

    // Mirrors GameSystem.startNewProcessList(name): looks up the global GM
    // function `a_name`, calls it with a real native process handle (see
    // process_handle_bindings.h) per the `Process=p;` convention confirmed
    // real across the corpus (e.g. Platformer/OneTrailPlayMode.gm), expects
    // a table back, runs its constructor() (init() is deferred -- see
    // Process::needsInit). Returns the process id, or -1 on failure
    // (logged to stderr).
    int StartNewProcessList(const std::string& a_name);

    // GameSystem.isProcessRunning/getProcess/getProcessCount/getProcessByIndex
    bool IsRunning(int a_id) const;
    int GetCount() const { return (int)m_processes.size(); }
    // Confirmed real: the decompiled body walks a linked list forward for
    // a positive index and backward for negative -- modelled here as
    // Python-style negative indexing (from the end) since we store
    // processes in a plain vector, not a linked list.
    Process* GetByIndex(int a_index);
    // getProcess(id_or_name) accepts either type in the confirmed decompiled
    // body (branches on GM_INT vs. the retail string tag) -- this is the
    // string half.
    Process* GetByName(const std::string& a_name);

    // Runs, in order, once per frame:
    //  1. init() on every constructed process still needing it (deferred
    //     from creation -- see Process::needsInit).
    //  2. update(msElapsed) on every initialized, non-quitting process that
    //     defines it (confirmed real signature/key name, real usage
    //     game_data/Main.gm:1159 `update=function(msElapsed) {...}`) --
    //     this is what actually drives state machines, timers, and content
    //     loading; without it real scripts sit permanently in their
    //     constructor()/init() state (e.g. Main.gm's loading-screen
    //     `this.stateIndex` never advances past -1, so its preDraw() never
    //     has a texture to draw).
    //  3. destructor() + removal for anything that called
    //     Process.terminate() (process_handle_bindings.cpp) this frame or
    //     earlier.
    void UpdateAll(float a_deltaMs);

    // Calls preDraw() on every initialized process that defines it.
    void PreDrawAll();

    // Calls postDraw() on every initialized process that defines it --
    void PostDrawAll();

    // Dispatches a key-code event to whichever process(es) should see it.
    void DispatchKeyEvent(int a_keycode, bool a_isPress);

    // onKeyRelease
    void DispatchKeyReleaseEvent(int a_keycode);

    bool AnyWantQuit() const;

    Process* Get(int a_id);

    // GameSystem.setAsPauseProcess(active, process) / process.setAsPauseProcess(active)
    // actual mechanism every real pause/menu/popup overlay in the corpus
    // uses to freeze gameplay (PauseProcessBase.gm, PopupProcessBase.gm,
    // WinProcess.gm, DebugMenu*.gm, ...): the process that should keep
    // running while everything else freezes marks itself (or is marked by
    // GameSystem) as THE pause process. a_exemptProcessId < 0 means "no
    // exemption" -- freeze every process (WinProcess.gm's bare
    // `GameSystem.setAsPauseProcess(true)`, no process arg, for the
    // win-screen's total freeze). Draw calls are untouched -- the frozen
    // game world should still be visible behind the overlay, only its
    // update()/postUpdate() stop advancing.
    void SetPauseState(bool a_active, int a_exemptProcessId);
    bool IsPaused() const { return m_paused; }

private:
    gmMachine& m_machine;
    std::vector<Process> m_processes;
    int m_nextId = 1;
    bool m_paused = false;
    int m_pauseExemptProcessId = -1;

    // Shared bring-up tail: inject process-base methods, run constructor()
    // on the process already reserved under a_id (init() is deferred, see
    // Process::needsInit). Looks the process up fresh by id both before and
    // after the GM callback rather than holding a Process& across it -- a
    // constructor() body can recursively start further processes, which
    // can reallocate m_processes out from under a held reference or
    // pointer.
    void FinishBringUp(int a_id);

    // Shared implementation for DispatchKeyEvent/DispatchKeyReleaseEvent --
    // both are the same focus-stealing/broadcast dispatch, just calling a
    // different method name on the target process table(s).
    void DispatchKeyEventNamed(int a_keycode, const char* a_methodName);

    // Runs init() on every constructed process still needing it. Called at
    // the start of UpdateAll() -- see Process::needsInit for why init() is
    // deferred this far rather than run inside FinishBringUp.
    void RunInitPass();

    // If a_processName's global factory function isn't defined yet, tries
    // an on-demand load of the .gm file that conventionally defines it
    // (e.g. "Platformer_MainMenu5" -> "Platformer/MainMenu5.gm") --
    // several real process names are used by GameSystem.
    // startNewProcessList/runProcess without ever being loaded via an
    // explicit GameMonkeyScript.loadScript(...) call anywhere in the
    // corpus. See the .cpp for why this is a convention-derived
    // substitute rather than a confirmed native mechanism. Returns true
    // if a script was found and loaded without compile errors (the
    // caller still needs to re-check the global afterward).
    bool TryAutoLoadScript(const std::string& a_processName);
};
