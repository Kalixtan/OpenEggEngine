#pragma once
#include "gmMachine.h"

// real corpus (942 `vector2D(...)` constructor call sites) for player
// position/velocity, level geometry, touch/joystick input, etc -- e.g.
// Platformer/ResizeScreenProcess.gm's `local input =
// vector2D(Input.getJoystickAxis(0,0), Input.getJoystickAxis(0,1));` then
// `input.y`.
//
// Retail bakes this directly into the 12-byte gmVariable as a third
// fundamental value type alongside int/float (tag 3) rather than a normal
// heap-allocated object -- reproducing that exactly would mean teaching
// every switch-on-type-tag site in the VM (arithmetic, comparison, the
// disassembler, serialization, ...) about a new fundamental type, which
// is a much larger and riskier change than the gameplay-facing behavior
// actually requires. Implemented here as a native user type instead (see
// engine/vendor/gmscript/gmsrc/src/binds/gmVector3Lib.cpp, GameMonkeyScript's
// own vendored Vector3 sample -- this follows that exact pattern for 2D):
// GM scripts can't tell the difference, since GM_CDECL user types support
// the same `.x`/`.y` property syntax (via RegisterTypeOperator's
// O_GETDOT/O_SETDOT, same mechanism the sample uses) and operator
// overloading (+/-/*//) real scripts actually use
// (`t.pos+vector2D(ang)*t.radius`, `tt.pos+=tt.vel*msElapsed`,
// `p1.getVectorToPoint(p2)/MATH_REAL(q-1)`, confirmed by corpus grep).
void RegisterVector2DBindings(gmMachine& a_machine);

// For other bindings (INPUT's touch/mouse/joystick-position getters,
// PhysicsShape, ...) that need to return a real vector2D instead of the
// plain {x=,y=} table stand-ins used before this type existed.
gmVariable MakeVector2D(gmMachine& a_machine, float a_x, float a_y);

// For other bindings (MATHUTIL_*, ...) that take a real vector2D as a
// parameter and need its components. Returns false (and leaves the
// outputs untouched) if a_var isn't actually a vector2D.
bool GetVector2D(gmMachine& a_machine, const gmVariable& a_var, float& a_outX, float& a_outY);

extern gmType g_vector2DType;
