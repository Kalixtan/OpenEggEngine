#pragma once
#include <string>
#include <chrono>
#include "gmVariable.h"

// GameSystem.c) that aren't process-management (see ProcessManager) --
// bookkeeping flags, timers, RNG-adjacent settings, autoload config, and the
// tiny "temporary storage" (entity copy/paste clipboard) buffer. Split out
// from EngineContext to keep that struct focused on cross-subsystem wiring.
struct GameSystemState {
    // isBeingNice/setIsBeingNice - only draws the "BEING NICE" debug overlay text when this is true, unknown use.
    bool isBeingNice = false;

    // 0x000701e8, real call site TouchEditProcessBase.gm:2733's
    // enforceLimits(): `local tick=GameSystem.getCurrentTick(); if(...||
    // this.lastEnforceLimitsTick==tick){ return; }`) -- a real monotonic
    // per-frame counter used purely as a "did this already run this
    // frame" dedupe key, not a wall-clock time. Incremented once per
    // engine frame in main.cpp's loop (right alongside vm.Tick()).
    int currentTick = 0;

    // isShouldUpdateEntities/setIsShouldUpdateEntities -- confirmed real
    // bool flag pair; ProcessManager doesn't consult this yet (no entity
    // system built out), stored so scripts reading it back get a consistent
    // answer.
    bool shouldUpdateEntities = true;

    // setALevelLoaded -- confirmed real setter, no matching getter in this
    // class (either write-only/notification-only, or the getter lives
    // elsewhere e.g. on a level-manager type we haven't bound yet).
    bool isALevelLoaded = false;

    // isLoading/setIsLoading -- confirmed real getter/setter pair.
    bool isLoadingFlag = false;

    // debug_setStop/debug_isStopped -- confirmed real getter/setter pair.
    bool debugStopped = false;

    // Input.c: Input.setIsInputEnabled, 0x000724f0). No decompiled getter
    // in the corpus, but the real call-site contract (Main.gm's save state
    // machine: false on entering SAVESTATE_ERRORSTATE, true on entering
    // SAVESTATE_SAVECOMPLETE) is a blocking-OS-dialog input gate -- while
    // false, key events must not reach process/entity script handlers.
    // Gated in ProcessManager::DispatchKeyEvent/DispatchKeyReleaseEvent.
    bool isInputEnabled = true;

    // getSimulationSpeed/setSimulationSpeed -- confirmed real float
    // getter/setter. Not yet wired into the actual tick scaling in main.cpp
    // (GmVm::Tick always uses real elapsed ms) -- stored/reported correctly,
    // applying it to the simulation clock is a follow-up.
    float simulationSpeed = 1.0f;

    // isAutoLoadLevel/getAutoLoadLevel/getAutoLoadPath/getAutoLoadMode/
    // getAutoLoadArea -- debug "jump straight into a level" getters
    bool isAutoLoadLevel = false;
    std::string autoLoadLevel = "editorTutorial";
    std::string autoLoadPath = "Platformer/levels";
    std::string autoLoadMode; // empty => getAutoLoadMode() returns null, matches confirmed decompiled code
    int autoLoadArea = 0;

    // getMSAlive (since engine start) / getMSSincePlayStart+resetPlayTimer
    // (since last reset) / resetSimulationTimer -- three confirmed real
    // timers. resetSimulationTimer's exact relationship to the other two
    // isn't nailed down by the decompiled body alone (no observable
    // side-channel), so it resets the same play-start clock as
    // resetPlayTimer -- the two most plausible readings collapse to the
    // same behavior when there's only one simulation clock, which is all
    // we model right now.
    std::chrono::steady_clock::time_point engineStartTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point playStartTime = std::chrono::steady_clock::now();

    float GetMsAlive() const {
        return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - engineStartTime).count();
    }
    float GetMsSincePlayStart() const {
        return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - playStartTime).count();
    }
    void ResetPlayTimer() { playStartTime = std::chrono::steady_clock::now(); }

    // TemporaryStorageCopy/TemporaryStoragePaste -- confirmed real 0-GM-arg
    // functions (no gmThread parameter reads, no gmThread pushes in the
    // operate on an implicit value rather than an explicit GM argument. The
    // real retail feature this backs (per real script usage context) is the
    // level editor's entity copy/paste clipboard, which naturally operates
    // on "whatever's implicitly current" -- we use the calling table
    // (gmThread::GetThis()) as that implicit value, which is the only
    // implicit value a native binding actually has access to.
    gmVariable temporaryStorage = gmVariable::s_null;
};
