#pragma once
#include <string>
#include <chrono>
#include <array>
#include <cstddef>
#include <cstdint>
#include "gmVariable.h"

struct GameSystemState {
    std::array<uint32_t, 624> randomState{};
    uint32_t randomIndex = 1;

    GameSystemState() {
        randomState[0] = 1;
            for (uint32_t i = 1; i < randomState.size(); ++i) {
            const uint32_t previous = randomState[i - 1];
            randomState[i] = 0x6c078965u * (previous ^ (previous >> 30)) + i;
        }
    }

    uint32_t NextRandomUInt() {
        if (randomIndex >= randomState.size()) {
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
        constexpr float kUInt32ToUnit = 2.3283064365386962890625e-10f;
        return static_cast<float>(NextRandomUInt()) * kUInt32ToUnit;
    }

    int NextRandomInteger(int minValue, int maxValue) {
        const uint32_t rangeWord = static_cast<uint32_t>(maxValue) -
            static_cast<uint32_t>(minValue) + 1u;
        const float range = static_cast<float>(static_cast<int32_t>(rangeWord));
        const int32_t offset = static_cast<int32_t>(range * NextRandom01());
        return static_cast<int32_t>(static_cast<uint32_t>(minValue) +
                                    static_cast<uint32_t>(offset));
    }

    // isBeingNice/setIsBeingNice - only draws the "BEING NICE" debug overlay text when this is true, unknown use.
    bool isBeingNice = false;

    int currentTick = 0;

    // isShouldUpdateEntities/setIsShouldUpdateEntities -- retail manager
    // byte +0xda2, checked by the native entity update dispatchers. The
    // PhysicsManager call site does not test this byte; its regular-loop
    // gates are pause (+0x48), loading (+0xf3c), and a separate +0x350 byte
    // whose only startup setter branch is dormant in the traced launch path.
    bool shouldUpdateEntities = true;
    bool nativeFireGroundUpdateSuppressed = false;

    bool isALevelLoaded = false;

    bool isLoadingFlag = false;

    bool debugStopped = false;

    bool isInputEnabled = true;

    float simulationSpeed = 1.0f;

    // isAutoLoadLevel/getAutoLoadLevel/getAutoLoadPath/getAutoLoadMode/
    // getAutoLoadArea -- debug "jump straight into a level" getters
    bool isAutoLoadLevel = false;
    std::string autoLoadLevel = "editorTutorial";
    std::string autoLoadPath = "Platformer/levels";
    std::string autoLoadMode;
    int autoLoadArea = 0;

    std::chrono::steady_clock::time_point engineStartTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point playStartTime = std::chrono::steady_clock::now();

    float GetMsAlive() const {
        return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - engineStartTime).count();
    }
    float GetMsSincePlayStart() const {
        return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - playStartTime).count();
    }
    void ResetPlayTimer() { playStartTime = std::chrono::steady_clock::now(); }

    gmVariable temporaryStorage = gmVariable::s_null;
};
