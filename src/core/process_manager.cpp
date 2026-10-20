#include "process_manager.h"
#include "../bindings/audio_bindings.h"
#include "../bindings/sequencer_bindings.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/gamemonkeyscript_bindings.h"
#include "../bindings/animation_bindings.h"
#include "../bindings/texture_bindings.h"
#include "gmCall.h"
#include "gmTableObject.h"
#include "../bindings/process_base_bindings.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/script_compat_bindings.h"
#include "../render/draw_list.h"
#include "../render/draw_context.h"
#include "../render/renderer.h"
#include "engine_context.h"
#include "retail_object_id.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <utility>
#include <unordered_set>

static void QueueDrawList(gmMachine& a_machine, const gmVariable& a_listVar,
                          const char* a_debugLabel,
                          std::vector<DrawListSubmission>& a_lists) {
    // Every registered user type (VMDRAWLIST, VMSHAPE, ...) gets its own
    // distinct gmType id counting up FROM GM_USER (gmVariable.h: "User
    // types continue from here") -- comparing against the bare GM_USER
    // constant itself matches nothing real, same reasoning as
    // gmThread::ThisUserObject()'s own `>= GM_USER` check.
    if (a_listVar.m_type < GM_USER) return;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_listVar.m_value.m_ref);
    if (!obj || obj->m_userType != GetDrawListType()) return;
    a_lists.push_back(CaptureDrawListSubmission(
        (DrawListData*)obj->m_user, a_debugLabel));
}

ProcessManager::ProcessManager(gmMachine& a_machine) : m_machine(a_machine) {}

namespace {

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

bool ProcessManager::TryAutoLoadScript(const std::string& a_processName) {
    std::string rest = a_processName;
    const std::string prefix = "Platformer_";
    bool hasPrefix = a_processName.compare(0, prefix.size(), prefix) == 0;
    if (hasPrefix) rest = a_processName.substr(prefix.size());

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
        m_queuedOperations.push_back({QueuedOperation::Type::ResetProcessList, -1, false});
        return -1;
    }

    const int id = CreateProcess(a_name);
    RequestEntityManagerDeleteAll();
    if (Process* target = Get(id))
        target->activationGeneration = std::numeric_limits<std::uint64_t>::max();
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

    int id = static_cast<int>(AllocateRetailObjectId());
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

    gmCall initCall;
    gmint result = 1;
    if (initCall.BeginTableFunction(&m_machine, "init", table, tableVar)) {
        initCall.End();
        if (initCall.DidReturnVariable() && initCall.GetReturnedVariable().m_type == GM_INT) {
            initCall.GetReturnedInt(result);
        } else {
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
            controller.processFacade.id = static_cast<int>(AllocateRetailObjectId());
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
            RequestGameMonkeyScriptCleanPasses(4);
            controller.state = 3;
            controller.stateUpdates = 0;
        } else if (state == 3) {
            if (GetGameMonkeyScriptCleanPassesRemaining() <= 0) {
                controller.state = 4;
                controller.stateUpdates = 0;
            }
        } else if (state == 4) {
            controller.state = 5;
            controller.stateUpdates = 0;
        } else if (state == 5) {
            controller.state = 6;
            controller.stateUpdates = 0;
        } else if (state == 6 && stateUpdates > 1) {
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
    const bool wasResettingProcessList = m_resettingProcessList;
    m_resettingProcessList = true;
    std::vector<Process> retired;
    bool retiringPlatformerMain = false;
    for (auto it = m_processes.begin(); it != m_processes.end();) {
        if (!it->constructed || it->activationGeneration > m_dispatchGeneration) {
            ++it;
            continue;
        }
        retiringPlatformerMain |= it->name == "Platformer_Main";
        retired.push_back(std::move(*it));
        it = m_processes.erase(it);
    }

    for (Process& proc : retired) {
        gmVariable tableVar = proc.table;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(tableVar.m_value.m_ref);
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "destructor", table, tableVar)) call.End();
    }

    // Platformer_Main owns the global sequencer and level voices. Its retail
    // destructor stops both before leaving gameplay. Keep the same cleanup in
    // the engine lifecycle path so archive-backed stock scripts receive it
    // even if a script overlay lacks the desktop workaround.
    if (retiringPlatformerMain) {
        StopMainSequencerForProcessListReset(m_machine);
        StopAllVoicesForProcessListReset();
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
    bool entitiesDispatched = !m_cleanupControllers.empty();
    if (entitiesDispatched) a_updateEntities();
    const Process* selected = GetEntityUpdateProcess();
    const int entityProcessId = selected ? selected->id : -1;
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
    if (!entitiesDispatched && !m_paused) a_updateEntities();
}

void ProcessManager::PostUpdateAll(float a_deltaMs) {
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
        if (Process* p = Get(id)) {
            const size_t queuedBefore = orderedLists.size();
            const std::string debugLabel = "pre:" + p->name;
            QueueDrawList(m_machine, p->preDrawList, debugLabel.c_str(), orderedLists);
            if (std::getenv("GE3_TRACE_DRAWLISTS") &&
                orderedLists.size() > queuedBefore) {
                const DrawListSubmission& submission = orderedLists.back();
                const size_t commandCount = submission.commandBuffer
                    ? submission.commandBuffer->commands.size() : 0;
                std::fprintf(stderr,
                    "[drawlist-queue] phase=pre id=%d name=%s key=%08x commands=%zu\n",
                    id, p->name.c_str(), submission.orderKey, commandCount);
            }
        }
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
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        int id = proc.id;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "postDraw", table, tableVar)) {
            call.End();
        }
        // See PreDrawAll's identical re-fetch -- same reentrancy hazard.
        if (Process* p = Get(id)) {
            const size_t queuedBefore = orderedLists.size();
            const std::string debugLabel = "post:" + p->name;
            QueueDrawList(m_machine, p->postDrawList, debugLabel.c_str(), orderedLists);
            if (std::getenv("GE3_TRACE_DRAWLISTS") &&
                orderedLists.size() > queuedBefore) {
                const DrawListSubmission& submission = orderedLists.back();
                const size_t commandCount = submission.commandBuffer
                    ? submission.commandBuffer->commands.size() : 0;
                std::fprintf(stderr,
                    "[drawlist-queue] phase=post id=%d name=%s key=%08x commands=%zu\n",
                    id, p->name.c_str(), submission.orderKey, commandCount);
            }
        }
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
