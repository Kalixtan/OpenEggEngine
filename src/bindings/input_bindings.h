#pragma once
#include "gmMachine.h"

// Minimal INPUT bindings. Confirmed real native type (Ghidra, EBOOT.elf's
// methods) -- registered here as a plain GM library table instead
// (RegisterLibrary, same simplification already used for ColourManager),
// not a true native type; no real SDL input polling wired up yet, every
// function returns a safe default (false/0/null) so scripts that read
// input state don't crash. Added because real game_data/Main.gm's
// splash-skip logic reads `Input.isKeyPressed(JOYSTICK0_CROSS)` with no
// '?' guard -- an undefined `Input` global would otherwise hard-crash the
// instant that code path is reached.
void RegisterInputBindings(gmMachine& a_machine);
