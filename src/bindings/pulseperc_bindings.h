#pragma once
#include "gmMachine.h"

// percussive envelope generator: pulse() triggers an attack-hold-decay
// ramp from 0 up to 1 and back down to 0, read back via getValue() as a
// 0..1 float, most commonly driven into a colour/scale interpolation (see
// game_data/Main.gm:667's ColourManager.makeInterpolatedColour call).
//
// update()/getValue()/isDone()/reset() are confirmed real by script usage
// across the corpus (e.g. game_data/Main.gm:636,659,667) but have no
// include them) -- implemented here as a straightforward attack/sustain/
// decay ramp built to functionally match every real call site, documented
// as an approximation rather than a confirmed decompiled port.
void RegisterPulsePercBindings(gmMachine& a_machine);
