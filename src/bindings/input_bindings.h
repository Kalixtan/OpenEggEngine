#pragma once
#include "gmMachine.h"
#include <SDL2/SDL.h>

// Partial SDL-backed INPUT API. Keyboard, mouse, and compatible
// SDL_GameController axes/buttons are live, with virtual-key support for
// headless execution; touch and IME remain incomplete.
// Retail registers INPUT as a 49-method native type; this port exposes a
// library table while its native method coverage is being reconstructed.
void RegisterInputBindings(gmMachine& a_machine);

// Virtual (script-injected) key state, used by the headless harness
// (platform/headless_harness.h) so a scripted key press is visible to
// Input.isKeyPressed the same way a physical one is. Real menus poll
// Input.isKeyPressed rather than relying only on the onKeyPress/onKeyType
// dispatch, so without this a harness run can render the game but never
// navigate it. Physical keys are unaffected -- KeyDown ORs the two sources.
void SetVirtualKeyDown(int a_scancode, bool a_down);
void ClearVirtualKeys();

// Desktop adapter codes for physical controller and keyboard events.
void InitializeDesktopControllers();
int ControllerEventCodes(SDL_JoystickID instance, Uint8 button, int* codes, int capacity);
int KeyboardJoystickAliasCodes(SDL_Scancode scancode, int* codes, int capacity);
int HeldControllerCodes(int* codes, int capacity);
