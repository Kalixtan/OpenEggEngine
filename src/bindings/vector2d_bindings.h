#pragma once
#include "gmMachine.h"

void RegisterVector2DBindings(gmMachine& a_machine);

// For other bindings (INPUT's touch/mouse/joystick-position getters,
// PhysicsShape, ...) that need to return a real vector2D instead of the
// plain {x=,y=} table stand-ins used before this type existed.
gmVariable MakeVector2D(gmMachine& a_machine, float a_x, float a_y);
gmVariable MakeRealPoint(gmMachine& a_machine, float a_x, float a_y);

// Construct a vector2D proxy whose component writes are reflected back to a
// native object. The owner handle is rooted for the proxy's lifetime.
using Vector2DRefRead = void (*)(void*, float&, float&);
using Vector2DRefWrite = void (*)(void*, float, float);
gmVariable MakeVector2DRef(gmMachine& a_machine, const gmVariable& a_owner,
                           void* a_context, Vector2DRefRead a_read,
                           Vector2DRefWrite a_write);

// For other bindings (MATHUTIL_*, ...) that take a real vector2D as a
// parameter and need its components. Returns false (and leaves the
// outputs untouched) if a_var isn't actually a vector2D.
bool GetVector2D(gmMachine& a_machine, const gmVariable& a_var, float& a_outX, float& a_outY);

extern gmType g_vector2DType;
