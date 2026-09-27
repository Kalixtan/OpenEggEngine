#pragma once
#include "gmMachine.h"

// MATH_* -- confirmed real (bare global functions, never assigned by any
// script, ~19 distinct names across hundreds of call sites, e.g.
// Platformer/Player.gm:112's `local targetMixValue=MATH_SQRT(0.2029);`)
// but entirely unregistered until now -- silently no-op'd via GM's
// null-call leniency (gmThread.cpp) at every call site, until a result
// got used in arithmetic (`null*null`), which throws rather than being
// forgiven. Plain wrappers over the C math library. Now cross-checked
// addresses/arg-count dispatch/return-type tags) plus float constants
// pulled directly from the retail EBOOT.elf where the decompile only
// showed an opaque DAT_ label -- see per-function comments in
// math_bindings.cpp for what's confirmed vs. still inferred (mainly: the
// exact libm routine behind each opaque FUN_ call, since none of those
// bodies are in the decompiled corpus, and the MATH_LN vs. MATH_LOG split).
void RegisterMathBindings(gmMachine& a_machine);
