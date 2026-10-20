#pragma once

#include <cstdint>

// Native processes, sequencers, triggers, physics bodies, and shapes share
// this counter. Add other constructors here once their ID use is confirmed.
inline std::uint32_t AllocateRetailObjectId() {
    static std::uint32_t nextId = 1;
    return nextId++;
}
