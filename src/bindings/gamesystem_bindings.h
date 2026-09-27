#pragma once
#include "gmMachine.h"

// Every function below is implemented against that file's decompiled body,
// not guessed from the name alone -- see the per-function comments in
// gamesystem_bindings.cpp for what was actually confirmed vs. where the
// decompiled body only pins down the argument/return shape and the real
// backing behavior had to be reconstructed (e.g. getRandomDir returning a
// vector2D we don't have a native type for yet, so it returns a plain
// {x=,y=} table instead -- flagged inline).
void RegisterGameSystemBindings(gmMachine& a_machine);
