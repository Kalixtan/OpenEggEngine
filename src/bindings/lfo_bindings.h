#pragma once
#include "gmMachine.h"

// constructor/setPhase/setRandomPhase/setRate/getRate/setRange/setContour)
// -- a free-running triangle-wave oscillator. update()/getValue() are
// confirmed real by widespread corpus usage (34/19 call sites respectively,
// e.g. game_data/Platformer/Main.gm:6707-7609's worldmapcurlfo,
// methods, not implemented) -- implemented here as a straightforward
// triangle ramp built to functionally match every real call site:
// setRate(periodMs) sets the full 0->1->0 cycle length in ms (real usage
// values 100-1000 match msElapsed-scale time, not Hz), setRange(min,max)
// sets the output bounds (default 0..1), setPhase/setRandomPhase seed the
// starting point within the cycle, update(msElapsed) advances it, getValue()
// reads the current interpolated output. Documented as an approximation
// rather than a confirmed decompiled port, same caveat as pulseperc_bindings.
void RegisterLfoBindings(gmMachine& a_machine);
