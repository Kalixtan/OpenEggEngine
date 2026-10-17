#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <utility>
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
// Implemented hooks include constructor/init/update/postUpdate/preDraw/
// postDraw, keyboard dispatch, and entity contact dispatch in physics_world.
// initialDraw/finalDraw and process-level touch dispatch still need a retail
// caller-path audit; native cleanup-controller resource ownership remains
struct Process {
    struct GroundRegistration {
        int style = 0;
        bool selected = false;
        float fireAmplitudePhase = 0.0f;
        float fireColourPhase = 0.0f;
        int minCellX = 0;
        int maxCellX = 0;
        int minCellY = 0;
        int maxCellY = 0;
    };
    int id = 0;
    std::string name;
    // Retail PROCESS.isHasErrors reads the native error-state byte at +0x38
    // through vtable slot +0x70. The desktop currently raises process errors
    // through the VM log, so this remains clear unless a future traced path
    // establishes a corresponding persistent state transition.
    bool hasErrors = false;
    // Native fire-ground owner state. Retail vtable +0x34 initializes the
    // shader owner once; later update callbacks at +0x38 step these scalars.
    bool hasNativeGroundFireOwner = false;
    bool nativeGroundFireUpdateReady = false;
    // EntityRegistration native fields recovered at +0xb8/+0x198. The
    // shipped GroundProcessNEW sets fireThickness during construction; the
    // constructor's pre-script default is still being checked against PPC.
    float fireThickness = 0.0f;
    bool refreshGroundDrawListsOnChange = false;
    float fireAmplitudePhase = 0.0f;
    float fireAmplitudeRate = 1.0f;
    float fireAmplitudeScale = 2.0f / 800.0f;
    float fireColourPhase = 0.0f;
    float fireColourRate = 1.0f;
    float fireColourScale = 2.0f / 600.0f;
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
    // Retail terminate() queues an operation for the next manager queue
    // drain. The process remains registered and receives the rest of this
    // frame's dispatches until that queued entry is consumed.
    bool terminationQueued = false;

    // The manager registry is populated by queued run/list operations.
    // Factories still return a usable table synchronously, but an object
    // created after a manager queue drain cannot receive lifecycle, draw, or
    // input dispatch until the following drain. This is the local equivalent
    // of the retail queue-entry boundary recovered in
    // GameSystem_Process_Flow_Analysis.md.
    std::uint64_t activationGeneration = 0;

    // Backing state for the native process handle (`p`/`this.Process`,
    // see process_handle_bindings.h) -- confirmed real methods
    // setIsStealsInputFocus/setupPreDrawList/setupPostDrawList/
    // getPreDrawList/getPostDrawList read and write these.
    bool stealsInputFocus = false;
    // Entity.setIsUpdatesEntities -- native process flag at object byte +9
    // FUN_0018c558 zeros it; GameSystem.getEntityUpdateProcess's
    // FUN_00187d24 returns a registered process with this flag set.
    bool updatesEntities = false;
    // EntityRegistration.registerEntity/unregisterEntity own the ground draw
    // list in retail. Keep only stable script entity IDs here; the GM table
    // itself remains rooted by the entity manager and is resolved from the
    // live physics body at draw time.
    std::unordered_map<int, GroundRegistration> groundRegistrations;
    // GroundProcessNEW's live list is append ordered: registerEntity first
    // unregisters an existing entry, then pushBacks the replacement. Keep
    // that order alongside the ID lookup map for painter-order rendering.
    std::vector<int> groundRegistrationOrder;
    // gmVariable's default ctor leaves m_type uninitialized unless
    // GMMACHINE_NULL_VAR_CTOR is defined (it isn't here) -- must nullify
    // explicitly rather than relying on the default member initializer.
    gmVariable preDrawList = gmVariable::s_null;  // until setupPreDrawList() is called
    gmVariable postDrawList = gmVariable::s_null; // until setupPostDrawList() is called
};

class ProcessManager {
public:
    explicit ProcessManager(gmMachine& a_machine);

    // Desktop cold-start entry: instantiate and activate the root process
    // directly because there is no old process list to transition away from.
    // This is bootstrap plumbing, not the script-facing retail API.
    int StartInitialProcess(const std::string& a_name);

    // Mirrors GameSystem.startNewProcessList(name, [cleanupPrevious]):
    // true resolves/constructs a new GM process, resets the old process list,
    // and runs the native cleanup controller phases before activating the
    // target; false queues only the retail process-list reset and returns no
    // process. The native resource registry mapping remains approximate.
    int StartNewProcessList(const std::string& a_name, bool a_cleanupPrevious = true);

    // Mirrors GameSystem.runProcess(name, [replaceExisting]): always creates
    // a new process instance; the retail-default-false flag optionally
    // requests termination of an existing same-name instance first.
    int RunProcess(const std::string& a_name, bool a_replaceExisting = false);

    // GameSystem.isProcessRunning/getProcess/getProcessCount/getProcessByIndex
    bool IsRunning(int a_id) const;
    // Mirrors Entity.terminate: queue removal for the next manager drain
    // rather than ending the process in the current dispatch frame.
    void QueueTerminate(int a_id);
    // Retail's count is the active registry count. A factory-created process
    // awaiting its queued run/list operation is not in that registry yet.
    int GetCount() const;
    // Walks the active retail registry as a circular list. The sentinel
    // counts as an indexable null slot, so out-of-range indices wrap;
    // negative indices walk backward starting from the first item.
    Process* GetByIndex(int a_index);
    // getProcess(id_or_name) accepts either type in the confirmed decompiled
    // body (branches on GM_INT vs. the retail string tag) -- this is the
    // string half.
    Process* GetByName(const std::string& a_name);
    // Returns the first registered process enabled by
    // Entity.setIsUpdatesEntities, matching FUN_00187d24's primary search.
    Process* GetEntityUpdateProcess();
    // Desktop lookup for the retail EntityRegistration registry. The wrapper
    // arguments/entity-ID flow are bytecode verified at 0x001d0430/0x001d04bc;
    // the private native registry's exact storage format remains unknown.
    bool GetGroundRegistration(int a_entityId, int& a_style, bool& a_selected) const;
    void GetGroundRegistrationsForCamera(float a_cameraX, float a_cameraY,
        std::vector<std::pair<int, Process::GroundRegistration>>& a_out) const;
    int GetGroundCountOnScreen(int a_screenX, int a_screenY) const;

    // Runs the retail update event once per frame, after draining the
    // termination queue. Each registered process receives either init() or
    // update(msElapsed) during its turn in the manager walk. The entity pass
    // follows the process selected by setIsUpdatesEntities, or runs after
    // the walk when no selected process triggers it.
    void UpdateAll(float a_deltaMs, const std::function<void()>& a_updateEntities);

    // Runs the distinct retail postUpdate event after the entity callbacks
    // of the update event. This may call constructed processes whose init()
    // has not yet completed.
    void PostUpdateAll(float a_deltaMs);

    // Calls preDraw() on every initialized process that defines it.
    void PreDrawAll();

    // Calls postDraw() on every initialized process that defines it --
    void PostDrawAll();

    // EntityManager's retail deletion notifier sends a native event containing
    // the still-registered entity to the live process list before unlinking
    // and destruction. Process scripts may expose onEntityDelete(entity).
    void NotifyEntityDeleted(const gmVariable& a_entity);

    // Dispatches a key-code event to whichever process(es) should see it.
    void DispatchKeyEvent(int a_keycode, bool a_isPress);

    // onKeyRelease
    void DispatchKeyReleaseEvent(int a_keycode);

    bool AnyWantQuit() const; // true while queued process termination remains

    Process* Get(int a_id);

    // GameSystem.setAsPauseProcess(active, process) / process.setAsPauseProcess(active)
    // actual mechanism every real pause/menu/popup overlay in the corpus
    // uses to freeze gameplay (PauseProcessBase.gm, PopupProcessBase.gm,
    // WinProcess.gm, DebugMenu*.gm, ...): the process that should keep
    // running while everything else freezes marks itself (or is marked by
    // GameSystem) as THE pause process. a_exemptProcessId < 0 means "no
    // exemption" -- freeze every process (WinProcess.gm's bare
    // `GameSystem.setAsPauseProcess(true)`, no process arg, for the
    // win-screen's total freeze). Retail queues this operation for the next
    // manager drain. Draw calls keep showing the frozen world.
    void QueuePauseState(bool a_active, int a_exemptProcessId);
    bool IsPaused() const { return m_paused; }
    // True only while the full-list reset is invoking process destructors.
    // Retail's DAT_0042489c guards its temporary native-object teardown set
    // across this interval; entity teardown uses the same scope.
    bool IsResettingProcessList() const { return m_resettingProcessList; }

private:
    gmMachine& m_machine;
    std::vector<Process> m_processes;
    int m_nextId = 1;
    // Incremented immediately after each retail-style operation-queue drain.
    // CreateProcess assigns the following generation, so creation before a
    // drain becomes eligible during that frame and creation during a frame
    // waits until the next one.
    std::uint64_t m_dispatchGeneration = 0;
    bool m_paused = false;
    bool m_resettingProcessList = false;
    int m_pauseExemptProcessId = -1;
    struct QueuedOperation {
        enum class Type {
            Terminate,
            Pause,
            ResetProcessList,
            StartProcessListCleanup,
            ActivateProcess
        } type;
        int processId;
        bool active;
    };
    std::vector<QueuedOperation> m_queuedOperations;
    struct NativeCleanupController {
        int targetProcessId = -1;
        int state = 0;
        int stateUpdates = 0;
        bool terminationQueued = false;
        // Retail inserts this native object into the public process registry.
        // This facade lets indexed APIs return a process handle while cleanup
        // state transitions remain driven by NativeCleanupController.
        Process processFacade;
    };
    std::vector<NativeCleanupController> m_cleanupControllers;

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

    // Runs one process's deferred init() at its turn in the update walk.
    // See Process::needsInit for why it runs after FinishBringUp.
    void RunInit(int a_id);
    void DrainOperationQueue();
    void ResetProcessList();
    void UpdateCleanupControllers();
    void ApplyPauseState(bool a_active, int a_exemptProcessId);
    void DestroyProcess(int a_id);

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
    int CreateProcess(const std::string& a_name);
};
