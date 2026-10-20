#pragma once

#include <cstdint>

// Native process, sequencer and trigger constructors use the same counter.
// Other native constructors need to use this too as their bindings are matched.
inline std::uint32_t AllocateRetailObjectId() {
    static std::uint32_t nextId = 1;
    return nextId++;
}
