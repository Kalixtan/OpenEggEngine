#pragma once

#include "gmMachine.h"
#include "../render/draw_context.h"
#include <memory>

// The retail MATRIX4 API used by the shipped scripts exposes translation,
// rotation about Z, and scale. Keep the value alive while a recorded draw
// list references it, then read its current transform at replay time.
using Matrix4Handle = std::shared_ptr<DrawContext::Mat>;

void RegisterMatrix4Bindings(gmMachine& a_machine);
Matrix4Handle GetMatrix4Handle(gmMachine& a_machine,
                               const gmVariable& a_value);
