#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include "gmThread.h"
#include "gmMachine.h"

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
    bool needsInit = true;
    // init() has run (successfully or not "assume true")
    // update()/preDraw() only run once this is true.
    bool initialized = false;
    // Retail terminate() queues an operation for the next manager queue
    // drain. The process remains registered and receives the rest of this
    // frame's dispatches until that queued entry is consumed.
    bool terminationQueued = false;

    std::uint64_t activationGeneration = 0;

    bool stealsInputFocus = false;
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
    Process* GetByName(const std::string& a_name);
    Process* GetEntityUpdateProcess();
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

    void QueuePauseState(bool a_active, int a_exemptProcessId);
    bool IsPaused() const { return m_paused; }
    bool IsResettingProcessList() const { return m_resettingProcessList; }

private:
    gmMachine& m_machine;
    std::vector<Process> m_processes;
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

    bool TryAutoLoadScript(const std::string& a_processName);
    int CreateProcess(const std::string& a_name);
};
