#pragma once

#include "platform_window.h"
#include <memory>

// Supplied by the selected platform frontend at link time. The game entry
// point depends on this factory and the neutral interface, never on SDL.
std::unique_ptr<PlatformFrontend> CreatePlatformFrontend();
