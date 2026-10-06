#include "process_manager.h"
#include "gmCall.h"
#include "gmTableObject.h"
#include "../bindings/process_base_bindings.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/foe_compat_bindings.h"
#include "../render/draw_list.h"
#include "../render/draw_context.h"
#include "../render/renderer.h"
#include "engine_context.h"
#include <cstdio>
#include <cstdint>

// Real drawList instances now record rather than draw immediately (see
// draw_list.h's RecordingTarget/DrawListData header comment) -- a process's
// own preDrawList/postDrawList (set up via setupPreDrawList/
// setupPostDrawList, process_handle_bindings.cpp) is exactly that kind of
// instance, so whatever its preDraw()/postDraw() hook just recorded needs
// an explicit replay to actually reach the screen. Every real hook already
// calls `this.preDrawList.reset()` as its first statement (confirmed real
// convention, see draw_list.cpp's gmf_reset header comment), which clears
// the list for the NEXT frame's rebuild -- so no clear is needed here,
// only the replay.
static void FlushDrawList(gmMachine& a_machine, const gmVariable& a_listVar) {
    // Every registered user type (VMDRAWLIST, VMSHAPE, ...) gets its own
    // distinct gmType id counting up FROM GM_USER (gmVariable.h: "User
    // types continue from here") -- comparing against the bare GM_USER
    // constant itself matches nothing real, same reasoning as
    // gmThread::ThisUserObject()'s own `>= GM_USER` check.
    if (a_listVar.m_type < GM_USER) return;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_listVar.m_value.m_ref);
    if (!obj || obj->m_userType != GetDrawListType()) return;
    ReplayDrawList((DrawListData*)obj->m_user);
}

ProcessManager::ProcessManager(gmMachine& a_machine) : m_machine(a_machine) {}

// Convention-derived on-demand script load: confirmed for every real case
// found (Platformer_MainMenu5 -> Platformer/MainMenu5.gm,
// Platformer_ResizeScreenProcess -> Platformer/ResizeScreenProcess.gm,
// Platformer_PopupProcess -> Platformer/PopupProcess.gm) by grepping every
// GameSystem.startNewProcessList/runProcess call site in the corpus
// against the actual .gm filenames that exist -- but the exact mechanism
// retail itself uses to reach these ISN'T confirmed: checked via Ghidra
// string search (EBOOT.elf), and neither "MainMenu5.gm" nor
// "ResizeScreenProcess.gm" appear anywhere in the retail binary as a
// loadScript-style string literal the way every other real loadScript
// argument in the corpus does (they show up as embedded GM source text,
// e.g. Platformer/MainMenuScriptLoader.gm's own loadScript calls are
// directly greppable in the binary's string table; these three names
// are not). Some other mechanism (a directory auto-scan at boot, a
// precompiled bytecode manifest, ...) is more likely real. This
// on-demand convention-based load is a substitute: it gets the script
// loaded before it's needed without hardcoding a native-preload list
// that risks loading things unrelated scenes don't want -- see
// main.cpp's isFullRetailTree guard, added after exactly that kind of
// guess broke game_data_min.
bool ProcessManager::TryAutoLoadScript(const std::string& a_processName) {
    std::string rest = a_processName;
    const std::string prefix = "Platformer_";
    bool hasPrefix = a_processName.compare(0, prefix.size(), prefix) == 0;
    if (hasPrefix) rest = a_processName.substr(prefix.size());

    // Try flat first (e.g. Platformer_ResizeScreenProcess ->
    // Platformer/ResizeScreenProcess.gm -- confirmed real for every single-
    // segment case grepped). Some real process names embed an actual
    // subdirectory level though (Platformer_MainMenu5_PreTutorialScreen ->
    // Platformer/MainMenu5/PreTutorialScreen.gm, confirmed by the file
    // only existing at that nested path) -- if flat doesn't exist, retry
    // with every remaining underscore turned into a path separator.
    std::string source;
    std::string flatPath = hasPrefix ? ("Platformer/" + rest + ".gm") : (a_processName + ".gm");
    std::string path = flatPath;
    if (GetEngineContext().fileHelper->ReadTextFile(path, source)) {
        int errors = m_machine.ExecuteString(source.c_str(), nullptr, true, path.c_str());
        return errors == 0;
    }
    if (hasPrefix) {
        std::string nested = rest;
        for (char& c : nested) if (c == '_') c = '/';
        path = "Platformer/" + nested + ".gm";
        if (path != flatPath && GetEngineContext().fileHelper->ReadTextFile(path, source)) {
            int errors = m_machine.ExecuteString(source.c_str(), nullptr, true, path.c_str());
            return errors == 0;
        }
        return false;
    }

    // Generic form of the same convention, without a fixed "Platformer/"
    // root to key off of -- same on-demand process-factory-file loading,
    // just for process names that don't happen to start with that one
    // prefix (confirmed necessary for "PekkoAndJon_Main" -> PekkoAndJon/
    // Main.gm; the underscore-as-path-separator convention itself isn't
    // Platformer-specific, this engine's auto-load just never generalized
    // past the one prefix it was built against). Try turning each
    // underscore into a slash, one at a time from left to right, e.g. for
    // "Foo_Bar_Baz": "Foo/Bar_Baz.gm", then "Foo/Bar/Baz.gm" -- least- to
    // most-nested, first hit wins. (Flat "a_processName.gm" was already
    // tried above and failed, so this starts one level deeper.)
    std::string candidate = a_processName;
    for (size_t i = 0; i < candidate.size(); ++i) {
        if (candidate[i] != '_') continue;
        candidate[i] = '/';
        std::string tryPath = candidate + ".gm";
        if (GetEngineContext().fileHelper->ReadTextFile(tryPath, source)) {
            int errors = m_machine.ExecuteString(source.c_str(), nullptr, true, tryPath.c_str());
            return errors == 0;
        }
    }
    return false;
}

int ProcessManager::StartNewProcessList(const std::string& a_name, bool a_cleanupPrevious) {
    const int id = CreateProcess(a_name);
    if (id >= 0 && a_cleanupPrevious) {
        // Retail creates a ParanoidCleanUpProcess controller for this path.
        // Its exact per-frame teardown timing is not yet reproduced; mark
        // old processes for removal through the next manager queue drain.
        std::vector<int> oldIds;
        for (const Process& process : m_processes) if (process.id != id) oldIds.push_back(process.id);
        for (int oldId : oldIds) QueueTerminate(oldId);
    }
    return id;
}

int ProcessManager::RunProcess(const std::string& a_name, bool a_replaceExisting) {
    if (a_replaceExisting) {
        if (Process* existing = GetByName(a_name)) QueueTerminate(existing->id);
    }
    return CreateProcess(a_name);
}

int ProcessManager::CreateProcess(const std::string& a_name) {
    gmVariable globalFunc = m_machine.GetGlobals()->Get(&m_machine, a_name.c_str());
    if (globalFunc.m_type != GM_FUNCTION && TryAutoLoadScript(a_name)) {
        globalFunc = m_machine.GetGlobals()->Get(&m_machine, a_name.c_str());
    }
    if (globalFunc.m_type != GM_FUNCTION) {
        std::fprintf(stderr, "ProcessManager::StartNewProcessList: global '%s' is not a function\n", a_name.c_str());
        return -1;
    }

    // Reserve the process slot (and id) before calling the factory, since
    // the factory needs a real process handle bound to that id as its `p`
    // argument -- confirmed real convention: every retail process factory
    // is `function(p) { return { Process=p, ... }; };` and stores it
    // itself (e.g. Platformer/OneTrailPlayMode.gm: `t.Process=p;`).
    int id = m_nextId++;
    Process proc;
    proc.id = id;
    proc.name = a_name;
    proc.activationGeneration = m_dispatchGeneration + 1;
    m_processes.push_back(proc);

    // The script factory can call back into GameSystem and create another
    // process before returning. Remove this reservation by id on failure;
    // pop_back() would erase that nested process if it became the new tail.
    auto discardReservation = [this, id]() {
        for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
            if (it->id == id) {
                m_processes.erase(it);
                return;
            }
        }
    };

    gmVariable handle = CreateProcessHandle(m_machine, id);

    gmCall call;
    if (!call.BeginGlobalFunction(&m_machine, a_name.c_str())) {
        std::fprintf(stderr, "ProcessManager::StartNewProcessList: failed to begin call to '%s'\n", a_name.c_str());
        discardReservation();
        return -1;
    }
    call.AddParam(handle);
    call.End();

    if (!call.DidReturnVariable() || call.GetReturnedVariable().m_type != GM_TABLE) {
        std::fprintf(stderr, "ProcessManager::StartNewProcessList: '%s(p)' did not return a table\n", a_name.c_str());
        discardReservation();
        return -1;
    }

    Process* p = Get(id);
    p->table = call.GetReturnedVariable();
    RootGmVariable(m_machine, p->table); // see process_manager.h -- GC can't see this otherwise
    FinishBringUp(id);
    return id;
}

void ProcessManager::FinishBringUp(int a_id) {
    Process* proc = Get(a_id);
    if (!proc) return; // already gone (shouldn't happen -- just reserved)
    gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc->table.m_value.m_ref);
    gmVariable tableVar = proc->table;

    InjectProcessBaseMethods(m_machine, table, a_id);

    // constructor() only -- init() is deferred, see Process::needsInit.
    gmCall ctorCall;
    if (ctorCall.BeginTableFunction(&m_machine, "constructor", table, tableVar)) {
        ctorCall.End();
    }

    proc = Get(a_id); // constructor() may have reallocated m_processes
    if (proc) proc->constructed = true;
}

void ProcessManager::RunInit(int a_id) {
    Process* proc = Get(a_id);
    if (!proc || !proc->constructed || !proc->needsInit) return;
    gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc->table.m_value.m_ref);
    gmVariable tableVar = proc->table;

    // "expecting init() to return a boolean. Assuming true."
    // Confirmed real async multi-step init convention (Platformer/Main.gm's
    // own init(): `if (!?this.groundProcess) { this.groundProcess=this.runProc(...);
    // return false; }` -- deliberately incomplete on this call, expecting
    // to be called AGAIN next frame once the dependency it just started
    // is ready) -- init() returning false must NOT be treated as done.
    // Previously marked needsInit=false unconditionally after a single
    // call, so this early-return half of Main.gm's init() ran exactly
    // once and the rest of its setup (camera position, deleteAllEntities,
    // etc, gated behind the SAME `if (!?this.groundProcess)` on the next
    // call) never executed -- update()/preDraw() still ran anyway since
    // `initialized` was set true regardless, leaving the process in a
    // half-initialized state for the rest of its life.
    gmCall initCall;
    gmint result = 1;
    if (initCall.BeginTableFunction(&m_machine, "init", table, tableVar)) {
        initCall.End();
        if (initCall.DidReturnVariable() && initCall.GetReturnedVariable().m_type == GM_INT) {
            initCall.GetReturnedInt(result);
        } else {
            // Retail FUN_000b02f8 logs this exact warning when init()
            // exists but does not return the GM boolean/int tag, then
            // assumes initialization succeeded. Keep that malformed
            // return case observable while preserving its default.
            std::fprintf(stderr,
                         ": expecting 'init()' to return a boolean.  Assuming true.\n");
        }
    }

    Process* p = Get(a_id); // init() may have reallocated m_processes
    if (p && result) {
        p->needsInit = false;
        p->initialized = true;
    }
    // On false, leave needsInit set and retry on the next manager dispatch.
}

bool ProcessManager::IsRunning(int a_id) const {
    for (const auto& proc : m_processes) {
        if (proc.id == a_id) return true;
    }
    return false;
}

void ProcessManager::QueueTerminate(int a_id) {
    Process* proc = Get(a_id);
    if (!proc || proc->terminationQueued) return;
    proc->terminationQueued = true;
    m_queuedOperations.push_back({QueuedOperation::Type::Terminate, a_id, false});
}

void ProcessManager::QueuePauseState(bool a_active, int a_exemptProcessId) {
    m_queuedOperations.push_back({QueuedOperation::Type::Pause, a_exemptProcessId, a_active});
}

void ProcessManager::DrainOperationQueue() {
    // Retail swaps its active queue buffer before dispatch. Operations
    // enqueued by a destructor during this drain therefore wait for the
    // following frame, rather than being consumed recursively here. Pause
    // changes and terminations share that queue, preserving insertion order.
    std::vector<QueuedOperation> queued;
    queued.swap(m_queuedOperations);
    for (const QueuedOperation& op : queued) {
        if (op.type == QueuedOperation::Type::Terminate) {
            DestroyProcess(op.processId);
        } else if (!op.active || op.processId < 0 || Get(op.processId)) {
            ApplyPauseState(op.active, op.processId);
        }
    }
}

void ProcessManager::DestroyProcess(int a_id) {
    Process* proc = Get(a_id);
    if (!proc) return;

    if (a_id == m_pauseExemptProcessId) ApplyPauseState(false, -1);
    gmVariable tableVar = proc->table;
    gmTableObject* table = (gmTableObject*)m_machine.GetObject(tableVar.m_value.m_ref);
    gmCall call;
    if (call.BeginTableFunction(&m_machine, "destructor", table, tableVar)) call.End();

    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it->id != a_id) continue;
        UnrootGmVariable(m_machine, it->table);
        UnrootGmVariable(m_machine, it->preDrawList);
        UnrootGmVariable(m_machine, it->postDrawList);
        m_processes.erase(it);
        break;
    }
}

Process* ProcessManager::GetByIndex(int a_index) {
    // The retail circular list contains only processes whose queued
    // run/list operation has entered the active manager registry. A process
    // created during a callback is available by name but absent from this
    // list until the following drain.
    const std::int64_t count = static_cast<std::int64_t>(GetCount());
    const std::int64_t ringSize = count + 1;
    const std::int64_t index = static_cast<std::int64_t>(a_index);
    std::int64_t slot = index >= 0
        ? index % ringSize
        : (ringSize - ((-index) % ringSize)) % ringSize;
    if (slot == count) return nullptr; // linked-list sentinel
    for (Process& process : m_processes) {
        if (!process.constructed || process.activationGeneration > m_dispatchGeneration)
            continue;
        if (slot-- == 0) return &process;
    }
    return nullptr;
}

int ProcessManager::GetCount() const {
    int count = 0;
    for (const Process& process : m_processes) {
        if (process.constructed && process.activationGeneration <= m_dispatchGeneration)
            ++count;
    }
    return count;
}

Process* ProcessManager::GetByName(const std::string& a_name) {
    for (auto& proc : m_processes) {
        if (proc.name == a_name) return &proc;
    }
    return nullptr;
}

Process* ProcessManager::GetEntityUpdateProcess() {
    for (auto& proc : m_processes) {
        if (proc.updatesEntities) return &proc;
    }
    return nullptr;
}

void ProcessManager::UpdateAll(float a_deltaMs, const std::function<void()>& a_updateEntities) {
    DrainOperationQueue();
    ++m_dispatchGeneration;
    TickAlarms(m_machine, a_deltaMs);
    // FUN_00188608 snapshots the first process with native byte +9 set
    // before it starts the registry walk. Its event adapter then dispatches
    // entities immediately after that process, even if the process just ran
    // init rather than update.
    const Process* selected = GetEntityUpdateProcess();
    const int entityProcessId = selected ? selected->id : -1;
    bool entitiesDispatched = false;

    // Retail FUN_00188608 walks the process registry once per update event.
    // Its update adapter calls FUN_0018c8d4, which dispatches init() OR
    // update() for each process. A separate init pass would initialize later
    // processes before an earlier process's update callback runs.
    // Re-read size after callbacks because either callback can add processes.
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (proc.activationGeneration > m_dispatchGeneration) continue;
        if (m_paused && proc.id != m_pauseExemptProcessId) continue; // see QueuePauseState
        if (!proc.constructed) continue;
        const int processId = proc.id; // callbacks may reallocate m_processes
        if (proc.needsInit) {
            RunInit(processId);
        } else if (proc.initialized) {
            gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
            gmVariable tableVar = proc.table;
            gmCall call;
            if (call.BeginTableFunction(&m_machine, "update", table, tableVar)) {
                call.AddParamFloat(a_deltaMs);
                call.End();
            }
        }
        if (!entitiesDispatched && processId == entityProcessId) {
            a_updateEntities();
            entitiesDispatched = true;
        }
    }
    // FUN_00188608's no-selected-process fallback is guarded by manager
    // field +0x48, the active pause-process pointer. Without this check,
    // entities keep updating when a pause process is active but does not
    // itself own the entity-update flag.
    if (!entitiesDispatched && !m_paused) a_updateEntities();
}

void ProcessManager::PostUpdateAll(float a_deltaMs) {
    // postUpdate() is a distinct retail manager event, dispatched after the
    // process and entity callbacks of the update event
    // (FUN_0018a2e8 -> FUN_00188608). Its native wrapper
    // checks only whether the cached callback exists; it does not check the
    // process init-state byte. Platformer/Main.gm's postUpdate() is where
    // `this.camera.update(msElapsed)` lives -- the per-frame camera-follow/
    // pan logic that reads the player's current screen and interpolates
    // Video.setCameraPos toward it. Without this call, the camera is
    // positioned exactly once at level load (via finalizeLoad's own direct
    // setCameraPos call) and then never moves again for the rest of the
    // level, no matter how far the player actually walks -- confirmed via a
    // live trace: the player's tracked body position moved ~19 world units
    // over several seconds of real movement while Video.getCameraPosX/Y()
    // stayed byte-identical the entire time. Run as its own pass after
    // update(), and allow it on a constructed process even when init() is
    // still pending. The same retail dispatcher applies the process pause
    // selection to both update and postUpdate events.
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (proc.activationGeneration > m_dispatchGeneration) continue;
        if (!proc.constructed) continue;
        if (m_paused && proc.id != m_pauseExemptProcessId) continue;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "postUpdate", table, tableVar)) {
            call.AddParamFloat(a_deltaMs);
            call.End();
        }
    }
}

void ProcessManager::NotifyEntityDeleted(const gmVariable& a_entity) {
    // The retail EntityManager packages the target entity in its delete event
    // and delivers that event through the process manager before removing
    // the entity's indexes. Call script hooks in process registration order.
    // Snapshot ids because an onEntityDelete callback can start or terminate
    // processes and invalidate m_processes storage while it runs.
    std::vector<int> processIds;
    processIds.reserve(m_processes.size());
    for (const Process& process : m_processes) processIds.push_back(process.id);

    for (int id : processIds) {
        Process* process = Get(id);
        if (!process || process->table.m_type != GM_TABLE) continue;
        gmVariable tableVar = process->table;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(tableVar.m_value.m_ref);
        if (!table || table->Get(&m_machine, "onEntityDelete").m_type != GM_FUNCTION) continue;

        gmCall call;
        if (call.BeginTableFunction(&m_machine, "onEntityDelete", table, tableVar)) {
            call.AddParam(a_entity);
            call.End();
        }
    }
}

void ProcessManager::DispatchKeyEvent(int a_keycode, bool a_isPress) {
    DispatchKeyEventNamed(a_keycode, a_isPress ? "onKeyPress" : "onKeyType");
}

void ProcessManager::DispatchKeyReleaseEvent(int a_keycode) {
    DispatchKeyEventNamed(a_keycode, "onKeyRelease");
}

void ProcessManager::DispatchKeyEventNamed(int a_keycode, const char* methodName) {
    // GameSystem.setIsInputEnabled(false) -- real blocking-OS-dialog input
    // gate (see game_system_state.h). While disabled, no key event reaches
    // any process/entity handler at all.
    if (!GetEngineContext().gameSystem.isInputEnabled) return;

    // Topmost (most-recently-created) process currently stealing focus, if
    // any -- see header comment.
    int focusId = -1;
    for (const auto& proc : m_processes) {
        if (proc.activationGeneration <= m_dispatchGeneration &&
            proc.initialized && proc.stealsInputFocus) focusId = proc.id;
    }

    if (focusId >= 0) {
        Process* proc = Get(focusId);
        if (!proc) return;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc->table.m_value.m_ref);
        gmVariable tableVar = proc->table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, methodName, table, tableVar)) {
            call.AddParamInt(a_keycode);
            call.End();
        }
        return;
    }

    // Nobody's stealing focus -- broadcast to every live process. Index-
    // based with a re-read size: a handler could start/stop further
    // processes mid-iteration (same reentrancy hazard as UpdateAll).
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (proc.activationGeneration > m_dispatchGeneration) continue;
        if (!proc.initialized) continue;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, methodName, table, tableVar)) {
            call.AddParamInt(a_keycode);
            call.End();
        }
    }
}

void ProcessManager::PreDrawAll() {
    // Index-based with a re-read size, not a range-based for over
    // m_processes: preDraw() can recursively start further processes
    // (same reentrancy hazard as FinishBringUp) and reallocate the vector
    // mid-iteration.
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (proc.activationGeneration > m_dispatchGeneration) continue;
        if (!proc.initialized) continue;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        int id = proc.id;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "preDraw", table, tableVar)) {
            call.End();
        }
        // Re-fetch by id -- preDraw() may have recursively started/reaped
        // processes and reallocated m_processes (same hazard as
        // FinishBringUp), so `proc` itself can no longer be trusted here.
        if (Process* p = Get(id)) FlushDrawList(m_machine, p->preDrawList);
    }
}

void ProcessManager::PostDrawAll() {
    // Same reentrancy/iteration reasoning as PreDrawAll.
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (proc.activationGeneration > m_dispatchGeneration) continue;
        if (!proc.initialized) continue;
        // Each process overlay begins with writable colour. Gameplay lists
        // use colour masking for stencil setup and can leave it disabled;
        // a later pause or loading list has no setColourMask call of its
        // own, so inheriting that state makes its entire overlay invisible.
        GetDrawContext().colourMask = false;
        if (Renderer* renderer = GetEngineContext().renderer)
            renderer->SetColourMask(false);
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        int id = proc.id;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "postDraw", table, tableVar)) {
            call.End();
        }
        // See PreDrawAll's identical re-fetch -- same reentrancy hazard.
        if (Process* p = Get(id)) FlushDrawList(m_machine, p->postDrawList);
    }
}

void ProcessManager::ApplyPauseState(bool a_active, int a_exemptProcessId) {
    m_paused = a_active;
    m_pauseExemptProcessId = a_active ? a_exemptProcessId : -1;
}

bool ProcessManager::AnyWantQuit() const {
    for (const QueuedOperation& op : m_queuedOperations) {
        if (op.type == QueuedOperation::Type::Terminate) return true;
    }
    return false;
}

Process* ProcessManager::Get(int a_id) {
    for (auto& proc : m_processes) {
        if (proc.id == a_id) return &proc;
    }
    return nullptr;
}
