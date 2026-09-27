#pragma once
#include "gmMachine.h"

// MATHUTIL_* -- confirmed real (bare global functions, never assigned by
// any script), a math-helper library used ~2000 times combined across the
// corpus (MATHUTIL_Rotate: 312, MATHUTIL_GetAngleRad: 243,
// MATHUTIL_CalculateExponentialDecay(Reversed): 145+128,
// MATHUTIL_GetRadiansBetween: 112, MATHUTIL_GETINRANGE: 110, ...) but
// entirely unregistered until now -- found while chasing why the real
// title screen's "start" button didn't visibly do anything:
// Platformer/MainMenu5PS3/MainMenu5PS3.gm's screen-fade-out logic
// (`this.niceExitData.fadePerc += (1-fadePerc) *
// MATHUTIL_CalculateExponentialDecayReversed(1.0/131.0, msElapsed);`)
// silently no-op'd (gmThread.cpp's null-call leniency patch) on every
// call, so `fadePerc` never left 0 and the fade (and the
// `this.niceExitData.execute()` that only fires once fadePerc>0.99, which
// is what actually launches the tutorial level) never happened.
//
// showed up as a named class) -- every formula here is inferred from
// real call-site usage patterns (argument count/types, what the result
// feeds into) rather than a confirmed decompiled body, same evidence bar
// as SEQUENCERACTION/the process-handle methods.
void RegisterMathUtilBindings(gmMachine& a_machine);
