#include "process_manager.h"
#include "../bindings/audio_bindings.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/gamemonkeyscript_bindings.h"
#include "../bindings/animation_bindings.h"
#include "../bindings/texture_bindings.h"
#include "gmCall.h"
#include "gmTableObject.h"
#include "../bindings/process_base_bindings.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/foe_compat_bindings.h"
#include "../render/draw_list.h"
#include "../render/draw_context.h"
#include "../render/renderer.h"
#include "engine_context.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <utility>
#include <unordered_set>

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
static void QueueDrawList(gmMachine& a_machine, const gmVariable& a_listVar,
                          std::vector<DrawListSubmission>& a_lists) {
    // Every registered user type (VMDRAWLIST, VMSHAPE, ...) gets its own
    // distinct gmType id counting up FROM GM_USER (gmVariable.h: "User
    // types continue from here") -- comparing against the bare GM_USER
    // constant itself matches nothing real, same reasoning as
    // gmThread::ThisUserObject()'s own `>= GM_USER` check.
    if (a_listVar.m_type < GM_USER) return;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_listVar.m_value.m_ref);
    if (!obj || obj->m_userType != GetDrawListType()) return;
    a_lists.push_back(CaptureDrawListSubmission((DrawListData*)obj->m_user));
}

ProcessManager::ProcessManager(gmMachine& a_machine) : m_machine(a_machine) {}

namespace {

// Retail GE3_Internal_AnimationPingPongScalarUpdate (FUN_00016810).
// Preserve the PPC helper boundary: scale delta in double, round to float,
// clamp by ordered comparisons, then compute the next state in double and
// round once before endpoint reflection.
void StepRetailPingPongScalar(float a_deltaMs, float a_scale,
                              float& a_phase, float& a_rate) {
    const float scaledDelta = static_cast<float>(
        static_cast<double>(a_deltaMs) * static_cast<double>(a_scale));
    const double scaledDeltaWide = static_cast<double>(scaledDelta);
    double clampedDelta = 0.0;
    if (0.0 - scaledDeltaWide < 0.0) clampedDelta = scaledDeltaWide;
    if (1.0 - clampedDelta < 0.0) clampedDelta = 1.0;
    const float next = static_cast<float>(
        clampedDelta * static_cast<double>(a_rate) +
        static_cast<double>(a_phase));
    a_phase = next;
    if (next < 0.0f) {
        a_rate = -a_rate;
        a_phase = std::fabs(next);
    } else if (next > 1.0f) {
        a_rate = -a_rate;
        a_phase = 1.0f - next;
    }
}

} // namespace

int ProcessManager::StartInitialProcess(const std::string& a_name) {
    if (!m_processes.empty() || !m_queuedOperations.empty()) return -1;
    return CreateProcess(a_name);
}

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
    if (!a_cleanupPrevious) {
        // FUN_00186cf8's false branch queues a transition with a null target;
        // it does not resolve or construct the requested factory. Its queue
        // dispatcher still flushes the current process registry.
        m_queuedOperations.push_back({QueuedOperation::Type::ResetProcessList, -1, false});
        return -1;
    }

    const int id = CreateProcess(a_name);
    // FUN_00186cf8 constructs the cleanup controller and queues the list
    // transition even when resolving the target returns null. Controller
    // construction enters state 0 and requests the entity clear-all.
    RequestEntityManagerDeleteAll();
    if (Process* target = Get(id))
        target->activationGeneration = std::numeric_limits<std::uint64_t>::max();
    // FUN_00186cf8 queues a distinct start-list transition carrying the
    // cleanup controller. Its dispatcher resets the old registry, inserts
    // the controller with updatesEntities=true, and only the controller's
    // final state queues the target process.
    m_queuedOperations.push_back({QueuedOperation::Type::StartProcessListCleanup, id, false});
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
            const Process* current = Get(a_id);
            std::fprintf(stderr,
                         ": expecting 'init()' to return a boolean.  Assuming true. "
                         "[process=%s id=%d]\n",
                         current ? current->name.c_str() : "<removed>", a_id);
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
    for (const NativeCleanupController& controller : m_cleanupControllers) {
        if (controller.processFacade.id == a_id) return true;
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
    for (auto it = m_cleanupControllers.begin(); it != m_cleanupControllers.end();) {
        if (!it->terminationQueued) {
            ++it;
            continue;
        }
        UnrootGmVariable(m_machine, it->processFacade.table);
        it = m_cleanupControllers.erase(it);
    }
    for (const QueuedOperation& op : queued) {
        if (op.type == QueuedOperation::Type::Terminate) {
            DestroyProcess(op.processId);
        } else if (op.type == QueuedOperation::Type::ResetProcessList) {
            ResetProcessList();
        } else if (op.type == QueuedOperation::Type::StartProcessListCleanup) {
            ResetProcessList();
            NativeCleanupController controller;
            controller.targetProcessId = op.processId;
            controller.processFacade.id = m_nextId++;
            controller.processFacade.name = "ParanoidCleanUpProcess";
            controller.processFacade.constructed = true;
            controller.processFacade.initialized = true;
            controller.processFacade.updatesEntities = true;
            controller.processFacade.activationGeneration = m_dispatchGeneration;
            controller.processFacade.table = CreateProcessHandle(
                m_machine, controller.processFacade.id);
            RootGmVariable(m_machine, controller.processFacade.table);
            m_cleanupControllers.push_back(std::move(controller));
        } else if (op.type == QueuedOperation::Type::ActivateProcess) {
            if (Process* process = Get(op.processId))
                // This run entry is being consumed in the current drain.
                // A later list-reset entry in the same buffer must therefore
                // see and remove it, just as retail dispatches entries in
                // insertion order.
                process->activationGeneration = m_dispatchGeneration;
        } else if (!op.active || op.processId < 0 || Get(op.processId)) {
            ApplyPauseState(op.active, op.processId);
        }
    }
}

void ProcessManager::UpdateCleanupControllers() {
    for (NativeCleanupController& controller : m_cleanupControllers) {
        if (controller.terminationQueued) continue;
        const int state = controller.state;
        const int stateUpdates = ++controller.stateUpdates;
        if (state == 0) {
            if (GetEntityManagerEntityCount() < 1) {
                controller.state = 1;
                controller.stateUpdates = 0;
            }
        } else if (state == 1) {
            if (stateUpdates > 8) {
                // Retail's state setter performs subsystem cleanup when it
                // enters state 2, before the next controller update executes
                // the command-buffer/GC phase.
                CleanupAudioManagerTrackedResources();
                controller.state = 2;
                controller.stateUpdates = 0;
            }
        } else if (state == 2) {
            // FUN_0004a6bc resets two native Video command buffers. This
            // renderer submits immediately and has no corresponding queues;
            // DAT_00f67ba8 also has no local counterpart. Do not clear the
            // renderer's whole GL cache here: retail resource objects remain
            // alive until their own references drain in state 6.
            RequestGameMonkeyScriptCleanPasses(4);
            controller.state = 3;
            controller.stateUpdates = 0;
        } else if (state == 3) {
            if (GetGameMonkeyScriptCleanPassesRemaining() <= 0) {
                controller.state = 4;
                controller.stateUpdates = 0;
            }
        } else if (state == 4) {
            // Ghidra finds only the state-4 read of DAT_00432d04 in EBOOT;
            // any indirect/external writer remains unidentified. Desktop
            // resource loading is synchronous, so there is no local work
            // counter to wait on here.
            controller.state = 5;
            controller.stateUpdates = 0;
        } else if (state == 5) {
            controller.state = 6;
            controller.stateUpdates = 0;
        } else if (state == 6 && stateUpdates > 1) {
            // Retail traverses the sentinel-based tree at DAT_0043b824, then
            // reaches this transition regardless of whether the tree had
            // entries. Do not turn that traversal into a resource-empty wait:
            // the tree contents do not gate activation. Shared ownership
            // preserves target resources while old process tables retire.
            if (Process* target = Get(controller.targetProcessId))
                target->updatesEntities = true;
            // Retail queues controller termination followed by target
            // runProcess. Both effects become visible at the next queue
            // drain, preserving the manager's double-buffer boundary.
            controller.terminationQueued = true;
            m_queuedOperations.push_back(
                {QueuedOperation::Type::ActivateProcess, controller.targetProcessId, false});
        }
    }
}

void ProcessManager::ResetProcessList() {
    // FUN_00186af4 first detaches the complete active registry, then invokes
    // each native process's destruction slot, and only after that clears the
    // pause-process pointer. Keep the GM objects rooted while their script
    // destructors run, but make the registry empty to any reentrant lookup.
    const bool wasResettingProcessList = m_resettingProcessList;
    m_resettingProcessList = true;
    std::vector<Process> retired;
    for (auto it = m_processes.begin(); it != m_processes.end();) {
        if (!it->constructed || it->activationGeneration > m_dispatchGeneration) {
            ++it;
            continue;
        }
        retired.push_back(std::move(*it));
        it = m_processes.erase(it);
    }

    for (Process& proc : retired) {
        gmVariable tableVar = proc.table;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(tableVar.m_value.m_ref);
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "destructor", table, tableVar)) call.End();
    }

    m_pauseExemptProcessId = -1;
    m_paused = false;
    for (const Process& proc : retired) {
        UnrootGmVariable(m_machine, proc.table);
        UnrootGmVariable(m_machine, proc.preDrawList);
        UnrootGmVariable(m_machine, proc.postDrawList);
    }
    m_resettingProcessList = wasResettingProcessList;
    // The native full-list reset invokes every registered Process destructor,
    // including an in-flight ParanoidCleanUpProcess controller.
    for (const NativeCleanupController& controller : m_cleanupControllers)
        UnrootGmVariable(m_machine, controller.processFacade.table);
    m_cleanupControllers.clear();
}

void ProcessManager::DestroyProcess(int a_id) {
    for (auto it = m_cleanupControllers.begin();
         it != m_cleanupControllers.end(); ++it) {
        if (it->processFacade.id != a_id) continue;
        UnrootGmVariable(m_machine, it->processFacade.table);
        m_cleanupControllers.erase(it);
        return;
    }

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
    // A list transition empties the old active registry, then inserts the
    // cleanup controller. It therefore precedes processes activated while
    // cleanup is running.
    for (NativeCleanupController& controller : m_cleanupControllers) {
        if (slot-- == 0) return &controller.processFacade;
    }
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
    // The retail startNewProcessList dispatcher inserts its cleanup
    // controller into the same counted process registry as ordinary
    // processes (FUN_0033ece8 -> FUN_0033aeac). Desktop keeps the controller
    // in a separate vector, so include each controller until it is removed
    // by the next queue drain.
    count += static_cast<int>(m_cleanupControllers.size());
    return count;
}

Process* ProcessManager::GetByName(const std::string& a_name) {
    for (auto& proc : m_processes) {
        // Retail name lookup searches registered processes and queued
        // transitions. A factory has a local ID reservation before it returns,
        // but that reservation is not a native process/transition yet.
        if (proc.constructed && proc.name == a_name) return &proc;
    }
    for (NativeCleanupController& controller : m_cleanupControllers) {
        if (controller.processFacade.name == a_name)
            return &controller.processFacade;
    }
    return nullptr;
}

Process* ProcessManager::GetEntityUpdateProcess() {
    // Retail FUN_00187d24 searches the active process registry first. Only
    // when that has no match does it inspect queued runProcess and
    // startNewProcessList entries. Locally, activationGeneration marks the
    // equivalent queue boundary: processes created after the last drain are
    // constructed synchronously but are not active until the next update.
    for (NativeCleanupController& controller : m_cleanupControllers) {
        if (controller.processFacade.updatesEntities)
            return &controller.processFacade;
    }
    for (auto& proc : m_processes) {
        if (proc.activationGeneration <= m_dispatchGeneration &&
            proc.constructed && proc.updatesEntities) return &proc;
    }
    for (auto& proc : m_processes) {
        if (proc.activationGeneration > m_dispatchGeneration &&
            proc.constructed && proc.updatesEntities) return &proc;
    }
    return nullptr;
}

bool ProcessManager::GetGroundRegistration(int a_entityId, int& a_style,
                                           bool& a_selected) const {
    for (const Process& process : m_processes) {
        if (process.name != "Platformer_GroundProcessNEW" ||
            !process.constructed || process.activationGeneration > m_dispatchGeneration)
            continue;
        const auto found = process.groundRegistrations.find(a_entityId);
        if (found == process.groundRegistrations.end()) continue;
        a_style = found->second.style;
        a_selected = found->second.selected;
        return true;
    }
    return false;
}

void ProcessManager::GetGroundRegistrationsForCamera(float a_cameraX, float a_cameraY,
    std::vector<std::pair<int, Process::GroundRegistration>>& a_out) const {
    a_out.clear();
    // PPC FUN_001ce4a0 constants at 0x001ce478..0x001ce49c, read directly
    // from PS3 exes/EBOOT.elf: half-view x/y extents and reciprocal screen
    // cell dimensions. Keep the same truncation plus negative-coordinate
    // decrement used by EntityRegistration.registerEntity.
    constexpr float kHalfViewX = 18.91f;
    constexpr float kHalfViewY = 10.37f;
    constexpr float kInvCellX = 0.025614753365516663f;
    constexpr float kInvCellY = 0.04553733766078949f;
    auto cellIndex = [](float position, float reciprocal) {
        int result = static_cast<int>(position * reciprocal);
        if (position < 0.0f) --result;
        return result;
    };
    const int minX = cellIndex(a_cameraX - kHalfViewX, kInvCellX);
    const int maxX = cellIndex(a_cameraX + kHalfViewX, kInvCellX);
    const int minY = cellIndex(a_cameraY - kHalfViewY, kInvCellY);
    const int maxY = cellIndex(a_cameraY + kHalfViewY, kInvCellY);

    for (const Process& process : m_processes) {
        if (process.name != "Platformer_GroundProcessNEW" ||
            !process.constructed || process.activationGeneration > m_dispatchGeneration)
            continue;
        a_out.reserve(process.groundRegistrationOrder.size());
        std::unordered_set<int> seen;
        // FUN_001ce4a0 scans x cells outside y cells, then appends entries
        // from each cell's registered vector, suppressing duplicates in its
        // temporary draw vector. The per-cell vector preserves registration
        // insertion order (FUN_001cd798 inserts; unregister removes).
        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                for (int entityId : process.groundRegistrationOrder) {
                    const auto found = process.groundRegistrations.find(entityId);
                    if (found == process.groundRegistrations.end() ||
                        seen.find(entityId) != seen.end()) continue;
                    const Process::GroundRegistration& ground = found->second;
                    if (x < ground.minCellX || x > ground.maxCellX ||
                        y < ground.minCellY || y > ground.maxCellY) continue;
                    seen.insert(entityId);
                    Process::GroundRegistration visibleGround = ground;
                    visibleGround.fireAmplitudePhase = process.fireAmplitudePhase;
                    visibleGround.fireColourPhase = process.fireColourPhase;
                    a_out.emplace_back(entityId, visibleGround);
                }
            }
        }
        return;
    }
}

int ProcessManager::GetGroundCountOnScreen(int a_screenX, int a_screenY) const {
    int count = 0;
    for (const Process& process : m_processes) {
        if (process.name != "Platformer_GroundProcessNEW" ||
            !process.constructed || process.activationGeneration > m_dispatchGeneration)
            continue;
        for (const auto& entry : process.groundRegistrations) {
            const Process::GroundRegistration& ground = entry.second;
            if (a_screenX >= ground.minCellX && a_screenX <= ground.maxCellX &&
                a_screenY >= ground.minCellY && a_screenY <= ground.maxCellY)
                ++count;
        }
    }
    return count;
}

void ProcessManager::UpdateAll(float a_deltaMs, const std::function<void()>& a_updateEntities) {
    DrainOperationQueue();
    ++m_dispatchGeneration;
    TickAlarms(m_machine, a_deltaMs);
    UpdateCleanupControllers();
    // FUN_0033ece8 re-registers the cleanup controller with updatesEntities
    // set. The native manager dispatches the entity event immediately after
    // that controller's update turn.
    bool entitiesDispatched = !m_cleanupControllers.empty();
    if (entitiesDispatched) a_updateEntities();
    // FUN_00188608 snapshots the first process with native byte +9 set
    // before it starts the registry walk. Its event adapter then dispatches
    // entities immediately after that process, even if the process just ran
    // init rather than update.
    const Process* selected = GetEntityUpdateProcess();
    const int entityProcessId = selected ? selected->id : -1;
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

        // The fire owner is installed with style-2 ground rendering. Its
        // first native update dispatch invokes vtable +0x34 (one-time shader
        // setup, returning ready); later eligible dispatches invoke +0x38 and
        // advance +0xbc then +0xdc. The callback itself checks GameSystem
        // +0x350; shouldUpdateEntities belongs to the separate entity-update
        // adapter, so it must not freeze this process-owned animation. Keep
        // state advancement out of rendering so camera visibility cannot
        // change animation time. The existing process pause filter above
        // determines whether this process receives its update event.
        for (Process& liveProc : m_processes) {
            if (liveProc.id != processId || !liveProc.hasNativeGroundFireOwner)
                continue;
            if (!liveProc.nativeGroundFireUpdateReady) {
                liveProc.nativeGroundFireUpdateReady = true;
            } else if (!GetEngineContext().gameSystem.nativeFireGroundUpdateSuppressed) {
                StepRetailPingPongScalar(a_deltaMs, liveProc.fireAmplitudeScale,
                    liveProc.fireAmplitudePhase, liveProc.fireAmplitudeRate);
                StepRetailPingPongScalar(a_deltaMs, liveProc.fireColourScale,
                    liveProc.fireColourPhase, liveProc.fireColourRate);
            }
            break;
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
    std::vector<DrawListSubmission> orderedLists;
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
        if (Process* p = Get(id)) QueueDrawList(m_machine, p->preDrawList, orderedLists);
    }
    ReplayDrawListsInOrder(orderedLists);
}

void ProcessManager::PostDrawAll() {
    // Same reentrancy/iteration reasoning as PreDrawAll.
    std::vector<DrawListSubmission> orderedLists;
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
        if (Process* p = Get(id)) QueueDrawList(m_machine, p->postDrawList, orderedLists);
    }
    ReplayDrawListsInOrder(orderedLists);
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
    for (auto& controller : m_cleanupControllers) {
        if (controller.processFacade.id == a_id)
            return &controller.processFacade;
    }
    return nullptr;
}
