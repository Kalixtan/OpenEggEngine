#pragma once
#include "gmMachine.h"
#include "../platform/platform_window.h"

// Partial platform-neutral INPUT API. Keyboard, mouse, and compatible
// controller state are supplied through PlatformFrontend, with virtual-key
// support for headless execution; touch and IME remain incomplete.
// Retail registers INPUT as a 49-method native type; this port exposes a
// library table while its native method coverage is being reconstructed.
void RegisterInputBindings(gmMachine& a_machine, PlatformFrontend& a_frontend);

// Virtual (script-injected) key state, used by the headless harness
// (platform/headless_harness.h) so a scripted key press is visible to
// Input.isKeyPressed the same way a physical one is. Real menus poll
// Input.isKeyPressed rather than relying only on the onKeyPress/onKeyType
// dispatch, so without this a harness run can render the game but never
// navigate it. Physical keys are unaffected -- KeyDown ORs the two sources.
void SetVirtualKeyDown(int a_scancode, bool a_down);
void ClearVirtualKeys();
// Pixel position supplied by the headless input harness. A host frontend may
// not receive a physical cursor position in headless mode, so Input reads this.
void SetVirtualMousePosition(int a_x, int a_y);
void SetVirtualMouseButtonDown(int a_buttonCode, bool a_down);

// Desktop adapter codes for physical controller and keyboard events.
void RefreshPlatformControllers();
int ControllerEventCodes(int instance, int button, int* codes, int capacity);
int KeyboardJoystickAliasCodes(int scancode, int* codes, int capacity);
int HeldControllerCodes(int* codes, int capacity);
