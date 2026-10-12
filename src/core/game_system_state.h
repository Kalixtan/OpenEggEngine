#pragma once
#include <string>
#include <chrono>
#include <array>
#include <cstddef>
#include <cstdint>
#include "gmVariable.h"

// GameSystem.c) that aren't process-management (see ProcessManager) --
// bookkeeping flags, timers, RNG-adjacent settings, autoload config, and the
// tiny "temporary storage" (entity copy/paste clipboard) buffer. Split out
// from EngineContext to keep that struct focused on cross-subsystem wiring.
struct GameSystemState {
    // Retail GameSystem owns one MT19937-compatible stream, initialized with
    // seed 1 by the GameSystem manager constructor (EBOOT.elf 0x188bec ->
    // FUN_000c9258). EntityManager random selection consumes this same stream.
    std::array<uint32_t, 624> randomState{};
    uint32_t randomIndex = 1;

    GameSystemState() {
        // FUN_000c91d0's seed recurrence: state[0] = seed, then
        // state[i] = 0x6c078965 * (state[i-1] ^ (state[i-1] >> 30)) + i.
        randomState[0] = 1;
            for (uint32_t i = 1; i < randomState.size(); ++i) {
            const uint32_t previous = randomState[i - 1];
            randomState[i] = 0x6c078965u * (previous ^ (previous >> 30)) + i;
        }
    }

    uint32_t NextRandomUInt() {
        if (randomIndex >= randomState.size()) {
            // FUN_000c8d24's two MT twist passes, including the wrapped
            // final word. Use a snapshot so each word reads the old state.
            const auto oldState = randomState;
            for (std::size_t i = 0; i < randomState.size(); ++i) {
                const uint32_t joined = (oldState[i] & 0x80000000u) |
                    (oldState[(i + 1) % randomState.size()] & 0x7fffffffu);
                randomState[i] = oldState[(i + 397) % randomState.size()] ^
                    (joined >> 1) ^ ((joined & 1u) ? 0x9908b0dfu : 0u);
            }
            randomIndex = 0;
        }

        uint32_t value = randomState[randomIndex++];
        value ^= value >> 11;
        value ^= (value << 7) & 0x9d2c5680u;
        value ^= (value << 15) & 0xefc60000u;
        value ^= value >> 18;
        return value;
    }

    float NextRandom01() {
        // FUN_000c8e90: float(rawUInt32) * 2^-32, with single precision at
        // each operation (the retail PPC code explicitly rounds via frsp).
        constexpr float kUInt32ToUnit = 2.3283064365386962890625e-10f;
        return static_cast<float>(NextRandomUInt()) * kUInt32ToUnit;
    }

    int NextRandomInteger(int minValue, int maxValue) {
        // FUN_000c8ee0 computes min + trunc((max-min+1) * rand01()).
        const uint32_t rangeWord = static_cast<uint32_t>(maxValue) -
            static_cast<uint32_t>(minValue) + 1u;
        const float range = static_cast<float>(static_cast<int32_t>(rangeWord));
        const int32_t offset = static_cast<int32_t>(range * NextRandom01());
        return static_cast<int32_t>(static_cast<uint32_t>(minValue) +
                                    static_cast<uint32_t>(offset));
    }

    // isBeingNice/setIsBeingNice - only draws the "BEING NICE" debug overlay text when this is true, unknown use.
    bool isBeingNice = false;

    // 0x000701e8, real call site TouchEditProcessBase.gm:2733's
    // enforceLimits(): `local tick=GameSystem.getCurrentTick(); if(...||
    // this.lastEnforceLimitsTick==tick){ return; }`) -- a real monotonic
    // simulation-update counter used as a "did this already run this
    // update" dedupe key, not a wall-clock time. Incremented for each
    // recovered fixed simulation tick in main.cpp.
    int currentTick = 0;

    // isShouldUpdateEntities/setIsShouldUpdateEntities -- retail manager
    // byte +0xda2, checked by the native entity update dispatchers. The
    // PhysicsManager call site does not test this byte; its regular-loop
    // gates are pause (+0x48), loading (+0xf3c), and a separate +0x350 byte
    // whose only startup setter branch is dormant in the traced launch path.
    bool shouldUpdateEntities = true;
    // Retail GameSystem byte +0x350 gates the native fire-ground scalar
    // callback. The inspected EBOOT startup path initializes it to zero and
    // has no writer, so desktop keeps the default clear value unless a later
    // trace identifies a live setter.
    bool nativeFireGroundUpdateSuppressed = false;

    // setALevelLoaded -- confirmed real setter, no matching getter in this
    // class (either write-only/notification-only, or the getter lives
    // elsewhere e.g. on a level-manager type we haven't bound yet).
    bool isALevelLoaded = false;

    // isLoading/setIsLoading -- confirmed real getter/setter pair over
    // retail GameSystem byte +0xf3c. The main loop forces exactly one fixed
    // simulation update per outer frame while it is set, and skips the
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
    // getter/setter. The retail game manager scales elapsed wall time by this
    // value before deciding how many fixed simulation ticks to dispatch.
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
