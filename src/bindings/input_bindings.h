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

// Virtual (script-injected) key state, used by the headless harness
// (platform/headless_harness.h) so a scripted key press is visible to
// Input.isKeyPressed the same way a physical one is. Real menus poll
// Input.isKeyPressed rather than relying only on the onKeyPress/onKeyType
// dispatch, so without this a harness run can render the game but never
// navigate it. Physical keys are unaffected -- KeyDown ORs the two sources.
void SetVirtualKeyDown(int a_scancode, bool a_down);
void ClearVirtualKeys();

