#pragma once
#include "gmMachine.h"

// MATH_* -- confirmed real (bare global functions, never assigned by any
// script, ~19 distinct names across hundreds of call sites, e.g.
// Platformer/Player.gm:112's `local targetMixValue=MATH_SQRT(0.2029);`)
// but entirely unregistered until now -- silently no-op'd via GM's
// null-call leniency (gmThread.cpp) at every call site, until a result
// got used in arithmetic (`null*null`), which throws rather than being
// forgiven. Plain wrappers over the C math library; no reveng evidence for
// any of these (a whole family of one-line utility wrappers, not worth a
// dedicated decompile pass), so behavior is inferred purely from each
// name/call-site shape.
void RegisterMathBindings(gmMachine& a_machine);
