#include "gmThread.h"
#include "input_bindings.h"
#include "gmTableObject.h"
#include "../bindings/vector2d_bindings.h"
#include "../core/engine_context.h"
#include "../render/renderer.h"
#include <SDL2/SDL.h>
#include <string>
#include <cctype>
#include <vector>

// SDL-backed desktop input. Keyboard scancodes and controller events use
// separate integer ranges so a physical face button cannot accidentally
// trigger a KEYBOARD_* action with the same desktop fallback key.
namespace {

constexpr int kNumberedButtonBase = 1024;
constexpr int kAliasBase = 2048;
struct ControllerAlias {
    const char* name;
    SDL_GameControllerButton button;
    SDL_Scancode fallback;
};
const ControllerAlias kAliases[] = {
    {"JOYSTICK0_CROSS", SDL_CONTROLLER_BUTTON_A, SDL_SCANCODE_Z},
    {"JOYSTICK0_CIRCLE", SDL_CONTROLLER_BUTTON_B, SDL_SCANCODE_V},
    {"JOYSTICK0_SQUARE", SDL_CONTROLLER_BUTTON_X, SDL_SCANCODE_X},
    {"JOYSTICK0_TRIANGLE", SDL_CONTROLLER_BUTTON_Y, SDL_SCANCODE_C},
    {"JOYSTICK0_ACCEPT", SDL_CONTROLLER_BUTTON_A, SDL_SCANCODE_RETURN},
    {"JOYSTICK0_CANCEL", SDL_CONTROLLER_BUTTON_B, SDL_SCANCODE_ESCAPE},
    {"JOYSTICK0_START", SDL_CONTROLLER_BUTTON_START, SDL_SCANCODE_P},
    {"JOYSTICK0_SELECT", SDL_CONTROLLER_BUTTON_BACK, SDL_SCANCODE_TAB},
    {"JOYSTICK0_UP", SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_SCANCODE_UP},
    {"JOYSTICK0_DOWN", SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_SCANCODE_DOWN},
    {"JOYSTICK0_LEFT", SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_SCANCODE_LEFT},
    {"JOYSTICK0_RIGHT", SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_SCANCODE_RIGHT},
    {"JOYSTICK0_L1", SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_SCANCODE_Q},
    {"JOYSTICK0_R1", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_SCANCODE_E},
    {"JOYSTICK0_L3", SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_SCANCODE_N},
    {"JOYSTICK0_R3", SDL_CONTROLLER_BUTTON_RIGHTSTICK, SDL_SCANCODE_M},
    {"JOYSTICK0_LS_UP", SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_SCANCODE_W},
    {"JOYSTICK0_LS_DOWN", SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_SCANCODE_S},
    {"JOYSTICK0_LS_LEFT", SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_SCANCODE_A},
    {"JOYSTICK0_LS_RIGHT", SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_SCANCODE_D},
};
constexpr SDL_GameControllerButton kNumberedButtons[] = {
    SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};
constexpr int kNumberedButtonCount = sizeof(kNumberedButtons) / sizeof(kNumberedButtons[0]);
constexpr int kAliasCount = sizeof(kAliases) / sizeof(kAliases[0]);

int GM_CDECL gmf_falseBool(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_zeroFloat(gmThread* a_thread) { a_thread->PushFloat(0.0f); return GM_OK; }
int GM_CDECL gmf_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_emptyString(gmThread* a_thread) { a_thread->PushNewString(""); return GM_OK; }
int GM_CDECL gmf_noop(gmThread* a_thread) { return GM_OK; }

// Virtual key state -- see SetVirtualKeyDown's header comment. File-scope
// (not in the anonymous namespace) so the setters below can reach it.
bool g_virtualKeys[SDL_NUM_SCANCODES] = {false};
bool g_inputEnabled = true;
bool g_disabledUntilAllKeysUp = false;
bool g_doingRealtimeUpdate = false;
float g_typeRate = 0.0f;
float g_typeDelay = 0.0f;
float g_joystickDeadzone = 0.0f;
float g_joystickAsButtonThreshold = 0.0f;
std::vector<SDL_GameController*> g_controllers;
int g_controllerDeviceCount = -1;
bool g_controllerSubsystemInitialized = false;

bool RefreshControllers() {
    if (!g_controllerSubsystemInitialized) {
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) return false;
        SDL_GameControllerEventState(SDL_ENABLE);
        g_controllerSubsystemInitialized = true;
    }

    const int deviceCount = SDL_NumJoysticks();
    if (deviceCount < 0) return false;
    if (deviceCount != g_controllerDeviceCount) {
        for (SDL_GameController* controller : g_controllers) {
            if (controller) SDL_GameControllerClose(controller);
        }
        g_controllers.assign(deviceCount, nullptr);
        g_controllerDeviceCount = deviceCount;
    }

    for (int i = 0; i < deviceCount; ++i) {
        SDL_GameController*& controller = g_controllers[i];
        if (controller && !SDL_GameControllerGetAttached(controller)) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
        }
        if (!controller && SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
        }
    }
    return true;
}

SDL_GameController* ControllerAt(int a_joystickIndex) {
    if (a_joystickIndex < 0 || !RefreshControllers() ||
        a_joystickIndex >= (int)g_controllers.size()) return nullptr;
    return g_controllers[a_joystickIndex];
}

// Confirmed from Input.c: the getter/setter read and write the INPUT
// object's byte at +0xf3e (FUN_00188278/FUN_0018c21c). Keep that state on
// the engine's singleton Input library object.
int GM_CDECL gmf_isInputEnabled(gmThread* a_thread) {
    a_thread->PushInt(g_inputEnabled ? 1 : 0);
    return GM_OK;
}

// Retail adds the 0x3f000000 float constant (0.5f) then converts to int;
// on PPC this conversion truncates toward zero. Preserve that behavior.
int GM_CDECL gmf_float2Int(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    // Retail reads the raw value word as a float without checking the GM
    // tag, so use the stack union directly instead of numeric coercion.
    const gmfloat value = a_thread->GetBase()[0].m_value.m_float;
    a_thread->PushInt((gmint)(value + 0.5f));
    return GM_OK;
}
int GM_CDECL gmf_setIsInputEnabled(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(enabled, 0, 0);
    g_inputEnabled = enabled != 0;
    return GM_OK;
}

int GM_CDECL gmf_setTypeRate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(rate, 0, 0.0f);
    g_typeRate = rate;
    return GM_OK;
}
int GM_CDECL gmf_getTypeRate(gmThread* a_thread) {
    a_thread->PushFloat(g_typeRate);
    return GM_OK;
}
int GM_CDECL gmf_setTypeDelay(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(delay, 0, 0.0f);
    g_typeDelay = delay;
    return GM_OK;
}
int GM_CDECL gmf_getTypeDelay(gmThread* a_thread) {
    a_thread->PushFloat(g_typeDelay);
    return GM_OK;
}
int GM_CDECL gmf_setJoystickDeadzone(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(value, 0, 0.0f);
    g_joystickDeadzone = value;
    return GM_OK;
}
int GM_CDECL gmf_getJoystickDeadzone(gmThread* a_thread) {
    a_thread->PushFloat(g_joystickDeadzone);
    return GM_OK;
}
int GM_CDECL gmf_setJoystickAsButtonThreshold(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(value, 0, 0.0f);
    g_joystickAsButtonThreshold = value;
    return GM_OK;
}
int GM_CDECL gmf_getJoystickAsButtonThreshold(gmThread* a_thread) {
    a_thread->PushFloat(g_joystickAsButtonThreshold);
    return GM_OK;
}
int GM_CDECL gmf_disableUntilAllKeysUp(gmThread*) {
    g_disabledUntilAllKeysUp = true;
    return GM_OK;
}
int GM_CDECL gmf_isDisabledUntilAllKeysUp(gmThread* a_thread) {
    a_thread->PushInt(g_disabledUntilAllKeysUp ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsDoingRealtimeUpdate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(enabled, 0, 0);
    g_doingRealtimeUpdate = enabled != 0;
    return GM_OK;
}
int GM_CDECL gmf_isDoingRealtimeUpdate(gmThread* a_thread) {
    a_thread->PushInt(g_doingRealtimeUpdate ? 1 : 0);
    return GM_OK;
}

bool KeyDown(int code) {
    if (code >= kNumberedButtonBase && code < kNumberedButtonBase + kNumberedButtonCount) {
        SDL_GameController* controller = ControllerAt(0);
        return controller && SDL_GameControllerGetButton(controller, kNumberedButtons[code - kNumberedButtonBase]);
    }
    if (code >= kAliasBase && code < kAliasBase + kAliasCount) {
        const ControllerAlias& alias = kAliases[code - kAliasBase];
        SDL_GameController* controller = ControllerAt(0);
        if (controller && SDL_GameControllerGetButton(controller, alias.button)) return true;
        code = alias.fallback;
    }
    if (code <= SDL_SCANCODE_UNKNOWN || code >= SDL_NUM_SCANCODES) return false;
    if (g_virtualKeys[code]) return true;
    return SDL_GetKeyboardState(nullptr)[code] != 0;
}

// Confirmed real: 1 int param (a KEYBOARD_*/JOYSTICK0_* code), returns
int GM_CDECL gmf_isKeyPressed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(code, 0, 0);
    a_thread->PushInt(KeyDown(code) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_isShiftPressed(gmThread* a_thread) {
    a_thread->PushInt((KeyDown(SDL_SCANCODE_LSHIFT) || KeyDown(SDL_SCANCODE_RSHIFT)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_isControlPressed(gmThread* a_thread) {
    a_thread->PushInt((KeyDown(SDL_SCANCODE_LCTRL) || KeyDown(SDL_SCANCODE_RCTRL)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_isAltPressed(gmThread* a_thread) {
    a_thread->PushInt((KeyDown(SDL_SCANCODE_LALT) || KeyDown(SDL_SCANCODE_RALT)) ? 1 : 0);
    return GM_OK;
}

// Confirmed real: 2 int params (joystick index, axis index), returns
// as the left stick and 2/3 as the right stick. SDL's standardized
// GameController layout supplies these axes for compatible attached pads;
// keyboard emulation remains available when no compatible pad is attached.
int GM_CDECL gmf_getJoystickAxis(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(joyIndex, 0, 0);
    GM_INT_PARAM(axis, 1, 0);
    float value = 0.0f;
    if (SDL_GameController* controller = ControllerAt(joyIndex)) {
        if (axis >= 0 && axis < 4) {
            static const SDL_GameControllerAxis kAxes[4] = {
                SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY,
                SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY
            };
            value = (float)SDL_GameControllerGetAxis(controller, kAxes[axis]) / 32767.0f;
            // The scripts treat positive Y as up; SDL reports up as negative.
            if (axis == 1 || axis == 3) value = -value;
            if (value < -1.0f) value = -1.0f;
        }
    } else if (joyIndex == 0) {
        if (axis == 0) {
            if (KeyDown(SDL_SCANCODE_D) || KeyDown(SDL_SCANCODE_RIGHT)) value += 1.0f;
            if (KeyDown(SDL_SCANCODE_A) || KeyDown(SDL_SCANCODE_LEFT)) value -= 1.0f;
        } else if (axis == 1) {
            if (KeyDown(SDL_SCANCODE_W) || KeyDown(SDL_SCANCODE_UP)) value += 1.0f;
            if (KeyDown(SDL_SCANCODE_S) || KeyDown(SDL_SCANCODE_DOWN)) value -= 1.0f;
        } else if (axis == 2) {
            if (KeyDown(SDL_SCANCODE_L)) value += 1.0f;
            if (KeyDown(SDL_SCANCODE_J)) value -= 1.0f;
        } else if (axis == 3) {
            if (KeyDown(SDL_SCANCODE_I)) value += 1.0f;
            if (KeyDown(SDL_SCANCODE_K)) value -= 1.0f;
        }
    }
    a_thread->PushFloat(value);
    return GM_OK;
}

int GM_CDECL gmf_isControllerPresent(gmThread* a_thread) {
    // Platformer_PlayMenu treats a false result as a controller-disconnect
    // event and starts the pause process every update. Desktop has a fully
    // supported keyboard input path (including stick emulation), so it is a
    // usable local gameplay device even when SDL has no game controller.
    // Reporting that availability here avoids applying the handheld/console
    // disconnect policy to a native PC session.
    a_thread->PushInt(1);
    return GM_OK;

}

// getMousePosX/Y -- confirmed NORMALIZED [0,1], not raw pixels: real usage
// (PekkoAndJon/Main.gm:15/19) is
// `GameBoard.getMinPos().x + Input.getMousePosX() * GameBoard.getRangeX()`,
// a lerp that only makes sense if getMousePosX() is a 0..1 fraction of the
// screen. Ghidra's own decompile of retail (FUN_00435550/4355c0) is an
// opaque vtable call whose internal scale can't be recovered directly, but
// the corpus math above pins it down unambiguously. Y is left top=0..1=
// bottom (not pre-flipped) -- Main.gm's own getMouseY() negates the result
// itself, so flipping here too would double-invert.
void NormalizedMousePos(float& a_x, float& a_y) {
    int x, y; SDL_GetMouseState(&x, &y);
    Renderer* r = GetEngineContext().renderer;
    int w = r ? r->GetWidth() : 0;
    int h = r ? r->GetHeight() : 0;
    a_x = (w > 0) ? (float)x / (float)w : 0.0f;
    a_y = (h > 0) ? (float)y / (float)h : 0.0f;
}
int GM_CDECL gmf_getMousePosX(gmThread* a_thread) {
    float x, y; NormalizedMousePos(x, y);
    a_thread->PushFloat(x);
    return GM_OK;
}
int GM_CDECL gmf_getMousePosY(gmThread* a_thread) {
    float x, y; NormalizedMousePos(x, y);
    a_thread->PushFloat(y);
    return GM_OK;
}
int GM_CDECL gmf_getMousePosVec(gmThread* a_thread) {
    float x, y; NormalizedMousePos(x, y);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), x, y));
    return GM_OK;
}
// getMousePosVec -- no corpus call sites for either to disambiguate a
// different return shape, so treated as an alias.
int GM_CDECL gmf_getMousePos(gmThread* a_thread) { return gmf_getMousePosVec(a_thread); }

// classes/KeyCode.c @ 0xa9ef0): 1 or 2 args, returns a length-1 string for
// a printable key or null for a non-printable one (real call sites --
// GenericTextInputProcess.gm/2.gm, ColourEditorProcess*.gm's text-entry
// widgets -- check `?cha` for "was this printable" before appending it to
// a text buffer). Since this engine's KEYBOARD_*/JOYSTICK0_* constants are
// SDL_Scancode values directly (see header comment above), this maps
// SDL_Scancode -> ASCII the same way, respecting the shift flag for
// letter case and the standard US-QWERTY shifted symbols.
int GM_CDECL gmf_KEYCODE_GetAscii(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(code, 0, 0);
    bool shift = false;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(s, 1, 0); shift = (s != 0); }

    char c = 0;
    if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z) {
        c = (char)('a' + (code - SDL_SCANCODE_A));
        if (shift) c = (char)std::toupper((unsigned char)c);
    } else if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9) {
        static const char kShifted[] = "!@#$%^&*(";
        c = shift ? kShifted[code - SDL_SCANCODE_1] : (char)('1' + (code - SDL_SCANCODE_1));
    } else if (code == SDL_SCANCODE_0) {
        c = shift ? ')' : '0';
    } else if (code == SDL_SCANCODE_SPACE) {
        c = ' ';
    } else {
        switch (code) {
            case SDL_SCANCODE_MINUS: c = shift ? '_' : '-'; break;
            case SDL_SCANCODE_EQUALS: c = shift ? '+' : '='; break;
            case SDL_SCANCODE_COMMA: c = shift ? '<' : ','; break;
            case SDL_SCANCODE_PERIOD: c = shift ? '>' : '.'; break;
            case SDL_SCANCODE_SLASH: c = shift ? '?' : '/'; break;
            case SDL_SCANCODE_BACKSLASH: c = shift ? '|' : '\\'; break;
            case SDL_SCANCODE_SEMICOLON: c = shift ? ':' : ';'; break;
            case SDL_SCANCODE_APOSTROPHE: c = shift ? '"' : '\''; break;
            case SDL_SCANCODE_LEFTBRACKET: c = shift ? '{' : '['; break;
            case SDL_SCANCODE_RIGHTBRACKET: c = shift ? '}' : ']'; break;
            case SDL_SCANCODE_GRAVE: c = shift ? '~' : '`'; break;
            case SDL_SCANCODE_KP_0: c = '0'; break;
            case SDL_SCANCODE_KP_1: c = '1'; break;
            case SDL_SCANCODE_KP_2: c = '2'; break;
            case SDL_SCANCODE_KP_3: c = '3'; break;
            case SDL_SCANCODE_KP_4: c = '4'; break;
            case SDL_SCANCODE_KP_5: c = '5'; break;
            case SDL_SCANCODE_KP_6: c = '6'; break;
            case SDL_SCANCODE_KP_7: c = '7'; break;
            case SDL_SCANCODE_KP_8: c = '8'; break;
            case SDL_SCANCODE_KP_9: c = '9'; break;
            case SDL_SCANCODE_KP_PLUS: c = '+'; break;
            case SDL_SCANCODE_KP_MINUS: c = '-'; break;
            case SDL_SCANCODE_KP_MULTIPLY: c = '*'; break;
            default: c = 0; break;
        }
    }
    if (c == 0) { a_thread->PushNull(); return GM_OK; }
    char buf[2] = { c, 0 };
    a_thread->PushNewString(buf);
    return GM_OK;
}

gmVariable ZeroVec2(gmMachine& a_machine) { return MakeVector2D(a_machine, 0.0f, 0.0f); }
int GM_CDECL gmf_zeroVec2(gmThread* a_thread) {
    a_thread->Push(ZeroVec2(*a_thread->GetMachine()));
    return GM_OK;
}

gmFunctionEntry s_inputLib[] = {
    {"isInputEnabled", gmf_isInputEnabled, nullptr},
    {"setIsInputEnabled", gmf_setIsInputEnabled, nullptr},
    {"float2Int", gmf_float2Int, nullptr},
    {"setTypeRate", gmf_setTypeRate, nullptr},
    {"getTypeRate", gmf_getTypeRate, nullptr},
    {"setTypeDelay", gmf_setTypeDelay, nullptr},
    {"getTypeDelay", gmf_getTypeDelay, nullptr},
    {"setJoystickDeadzone", gmf_setJoystickDeadzone, nullptr},
    {"getJoystickDeadzone", gmf_getJoystickDeadzone, nullptr},
    {"setJoystickAsButtonThreshold", gmf_setJoystickAsButtonThreshold, nullptr},
    {"getJoystickAsButtonThreshold", gmf_getJoystickAsButtonThreshold, nullptr},
    {"disableUntilAllKeysUp", gmf_disableUntilAllKeysUp, nullptr},
    {"isDisabledUntilAllKeysUp", gmf_isDisabledUntilAllKeysUp, nullptr},
    {"setIsDoingRealtimeUpdate", gmf_setIsDoingRealtimeUpdate, nullptr},
    {"isDoingRealtimeUpdate", gmf_isDoingRealtimeUpdate, nullptr},
    {"isKeyPressed", gmf_isKeyPressed, nullptr},
    {"isShiftPressed", gmf_isShiftPressed, nullptr},
    {"isControlPressed", gmf_isControlPressed, nullptr},
    {"isAltPressed", gmf_isAltPressed, nullptr},
    {"getJoystickAxis", gmf_getJoystickAxis, nullptr},
    {"isControllerPresent", gmf_isControllerPresent, nullptr},
    {"getFrontTouchPos", gmf_zeroVec2, nullptr},
    {"getBackTouchPos", gmf_zeroVec2, nullptr},
    {"getMousePosVec", gmf_getMousePosVec, nullptr},
    {"getMousePos", gmf_getMousePos, nullptr},
    {"getMousePosX", gmf_getMousePosX, nullptr},
    {"getMousePosY", gmf_getMousePosY, nullptr},
    {"getFrontTouchCount", gmf_zeroInt, nullptr},
    {"getBackTouchCount", gmf_zeroInt, nullptr},
    {"getWasTouchTap", gmf_falseBool, nullptr},
    {"getIsTouchMoving", gmf_falseBool, nullptr},
    {"getIsTouchHold", gmf_falseBool, nullptr},
    {"getIsShaking", gmf_falseBool, nullptr},
    {"openIME", gmf_noop, nullptr},
    {"getIMEResults", gmf_emptyString, nullptr},
    {"getIMEStatus", gmf_zeroInt, nullptr},
    {"getStringIME", gmf_emptyString, nullptr},
    {"disableShaking", gmf_noop, nullptr},
    {"enableShaking", gmf_noop, nullptr},
    {"enableBackPanel", gmf_noop, nullptr},
    {"disableBackPanel", gmf_noop, nullptr},
    {"setSoftRumble", gmf_noop, nullptr},
};

} // namespace

void InitializeDesktopControllers() { RefreshControllers(); }

int ControllerEventCodes(SDL_JoystickID instance, Uint8 button, int* codes, int capacity) {
    SDL_GameController* controller = ControllerAt(0);
    if (!controller || SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) != instance) return 0;
    int count = 0;
    for (int i = 0; i < kNumberedButtonCount; ++i) {
        if (kNumberedButtons[i] == button && count < capacity) codes[count++] = kNumberedButtonBase + i;
    }
    for (int i = 0; i < kAliasCount; ++i) {
        if (kAliases[i].button == button && count < capacity) codes[count++] = kAliasBase + i;
    }
    return count;
}

int KeyboardJoystickAliasCodes(SDL_Scancode scancode, int* codes, int capacity) {
    int count = 0;
    for (int i = 0; i < kAliasCount; ++i) {
        if (kAliases[i].fallback == scancode && count < capacity) codes[count++] = kAliasBase + i;
    }
    return count;
}

int HeldControllerCodes(int* codes, int capacity) {
    SDL_GameController* controller = ControllerAt(0);
    if (!controller) return 0;
    int count = 0;
    for (int i = 0; i < kNumberedButtonCount; ++i) {
        if (SDL_GameControllerGetButton(controller, kNumberedButtons[i]) && count < capacity)
            codes[count++] = kNumberedButtonBase + i;
    }
    for (int i = 0; i < kAliasCount; ++i) {
        if (SDL_GameControllerGetButton(controller, kAliases[i].button) && count < capacity)
            codes[count++] = kAliasBase + i;
    }
    return count;
}

void RegisterInputBindings(gmMachine& a_machine) {
    a_machine.RegisterLibrary(s_inputLib, sizeof(s_inputLib) / sizeof(s_inputLib[0]), "Input");

    // KeyCode.c), not under "Input." or "KeyCode." -- matches every real
    // call site (`local cha=KEYCODE_GetAscii(kc);`).
    a_machine.RegisterLibraryFunction("KEYCODE_GetAscii", gmf_KEYCODE_GetAscii);

    // Retail's exact integer values are not yet recovered. Keep keyboard
    // and controller codes in distinct ranges so their actions stay distinct.
    gmTableObject* g = a_machine.GetGlobals();
    // Numbered buttons follow ControllerConfig.gm's active Xbox mapping:
    // A/B/X/Y, LB/RB, Back/Start, stick clicks, then the D-pad.
    for (int i = 0; i < kNumberedButtonCount; ++i)
        g->Set(&a_machine, ("JOYSTICK0_BUTTON" + std::to_string(i)).c_str(), gmVariable((gmint)(kNumberedButtonBase + i)));
    for (int i = 0; i < kAliasCount; ++i)
        g->Set(&a_machine, kAliases[i].name, gmVariable((gmint)(kAliasBase + i)));

    // Remaining aliases still use keyboard fallbacks until trigger and
    // right-stick digital thresholds are implemented.
    g->Set(&a_machine, "JOYSTICK0_L2", gmVariable((gmint)SDL_SCANCODE_1));
    g->Set(&a_machine, "JOYSTICK0_R2", gmVariable((gmint)SDL_SCANCODE_3));
    g->Set(&a_machine, "JOYSTICK0_RS_UP", gmVariable((gmint)SDL_SCANCODE_I));
    g->Set(&a_machine, "JOYSTICK0_RS_DOWN", gmVariable((gmint)SDL_SCANCODE_K));
    g->Set(&a_machine, "JOYSTICK0_RS_LEFT", gmVariable((gmint)SDL_SCANCODE_J));
    g->Set(&a_machine, "JOYSTICK0_RS_RIGHT", gmVariable((gmint)SDL_SCANCODE_L));

    g->Set(&a_machine, "MOUSE_LEFTBUTTON", gmVariable((gmint)0));
    g->Set(&a_machine, "MOUSE_RIGHTBUTTON", gmVariable((gmint)1));
    g->Set(&a_machine, "MOUSE_MIDDLEBUTTON", gmVariable((gmint)2));

    // KEYBOARD_* -- direct SDL_Scancode values.
    struct { const char* name; SDL_Scancode code; } kKeys[] = {
        {"KEYBOARD_A", SDL_SCANCODE_A}, {"KEYBOARD_B", SDL_SCANCODE_B}, {"KEYBOARD_C", SDL_SCANCODE_C},
        {"KEYBOARD_D", SDL_SCANCODE_D}, {"KEYBOARD_E", SDL_SCANCODE_E}, {"KEYBOARD_F", SDL_SCANCODE_F},
        {"KEYBOARD_G", SDL_SCANCODE_G}, {"KEYBOARD_H", SDL_SCANCODE_H}, {"KEYBOARD_I", SDL_SCANCODE_I},
        {"KEYBOARD_J", SDL_SCANCODE_J}, {"KEYBOARD_K", SDL_SCANCODE_K}, {"KEYBOARD_L", SDL_SCANCODE_L},
        {"KEYBOARD_M", SDL_SCANCODE_M}, {"KEYBOARD_N", SDL_SCANCODE_N}, {"KEYBOARD_O", SDL_SCANCODE_O},
        {"KEYBOARD_P", SDL_SCANCODE_P}, {"KEYBOARD_Q", SDL_SCANCODE_Q}, {"KEYBOARD_R", SDL_SCANCODE_R},
        {"KEYBOARD_S", SDL_SCANCODE_S}, {"KEYBOARD_T", SDL_SCANCODE_T}, {"KEYBOARD_U", SDL_SCANCODE_U},
        {"KEYBOARD_V", SDL_SCANCODE_V}, {"KEYBOARD_W", SDL_SCANCODE_W}, {"KEYBOARD_X", SDL_SCANCODE_X},
        {"KEYBOARD_Y", SDL_SCANCODE_Y}, {"KEYBOARD_Z", SDL_SCANCODE_Z},
        {"KEYBOARD_0", SDL_SCANCODE_0}, {"KEYBOARD_1", SDL_SCANCODE_1}, {"KEYBOARD_2", SDL_SCANCODE_2},
        {"KEYBOARD_3", SDL_SCANCODE_3}, {"KEYBOARD_4", SDL_SCANCODE_4}, {"KEYBOARD_5", SDL_SCANCODE_5},
        {"KEYBOARD_6", SDL_SCANCODE_6}, {"KEYBOARD_7", SDL_SCANCODE_7}, {"KEYBOARD_8", SDL_SCANCODE_8},
        {"KEYBOARD_9", SDL_SCANCODE_9},
        {"KEYBOARD_UP", SDL_SCANCODE_UP}, {"KEYBOARD_DOWN", SDL_SCANCODE_DOWN},
        {"KEYBOARD_LEFT", SDL_SCANCODE_LEFT}, {"KEYBOARD_RIGHT", SDL_SCANCODE_RIGHT},
        {"KEYBOARD_ENTER", SDL_SCANCODE_RETURN}, {"KEYBOARD_ESCAPE", SDL_SCANCODE_ESCAPE},
        {"KEYBOARD_BACKSPACE", SDL_SCANCODE_BACKSPACE}, {"KEYBOARD_DELETE", SDL_SCANCODE_DELETE},
        {"KEYBOARD_TAB", SDL_SCANCODE_TAB}, {"KEYBOARD_SPACE", SDL_SCANCODE_SPACE},
        {"KEYBOARD_HOME", SDL_SCANCODE_HOME}, {"KEYBOARD_END", SDL_SCANCODE_END},
        {"KEYBOARD_INSERT", SDL_SCANCODE_INSERT},
        {"KEYBOARD_PAGEUP", SDL_SCANCODE_PAGEUP}, {"KEYBOARD_PAGEDOWN", SDL_SCANCODE_PAGEDOWN},
        {"KEYBOARD_LEFTSHIFT", SDL_SCANCODE_LSHIFT}, {"KEYBOARD_RIGHTSHIFT", SDL_SCANCODE_RSHIFT},
        {"KEYBOARD_LEFTCONTROL", SDL_SCANCODE_LCTRL}, {"KEYBOARD_RIGHTCONTROL", SDL_SCANCODE_RCTRL},
        {"KEYBOARD_LEFTALT", SDL_SCANCODE_LALT},
        {"KEYBOARD_MINUS", SDL_SCANCODE_MINUS}, {"KEYBOARD_PLUS", SDL_SCANCODE_EQUALS},
        {"KEYBOARD_COMMA", SDL_SCANCODE_COMMA}, {"KEYBOARD_PERIOD", SDL_SCANCODE_PERIOD},
        {"KEYBOARD_SLASH", SDL_SCANCODE_SLASH}, {"KEYBOARD_BACKSLASH", SDL_SCANCODE_BACKSLASH},
        {"KEYBOARD_LEFTSQUAREBRACKET", SDL_SCANCODE_LEFTBRACKET},
        {"KEYBOARD_RIGHTSQUAREBRACKET", SDL_SCANCODE_RIGHTBRACKET},
        {"KEYBOARD_F1", SDL_SCANCODE_F1}, {"KEYBOARD_F2", SDL_SCANCODE_F2}, {"KEYBOARD_F3", SDL_SCANCODE_F3},
        {"KEYBOARD_F4", SDL_SCANCODE_F4}, {"KEYBOARD_F5", SDL_SCANCODE_F5}, {"KEYBOARD_F6", SDL_SCANCODE_F6},
        {"KEYBOARD_F7", SDL_SCANCODE_F7}, {"KEYBOARD_F8", SDL_SCANCODE_F8}, {"KEYBOARD_F9", SDL_SCANCODE_F9},
        {"KEYBOARD_F10", SDL_SCANCODE_F10}, {"KEYBOARD_F11", SDL_SCANCODE_F11}, {"KEYBOARD_F12", SDL_SCANCODE_F12},
        {"KEYBOARD_KEYPAD_1", SDL_SCANCODE_KP_1}, {"KEYBOARD_KEYPAD_2", SDL_SCANCODE_KP_2},
        {"KEYBOARD_KEYPAD_3", SDL_SCANCODE_KP_3}, {"KEYBOARD_KEYPAD_4", SDL_SCANCODE_KP_4},
        {"KEYBOARD_KEYPAD_5", SDL_SCANCODE_KP_5}, {"KEYBOARD_KEYPAD_6", SDL_SCANCODE_KP_6},
        {"KEYBOARD_KEYPAD_7", SDL_SCANCODE_KP_7}, {"KEYBOARD_KEYPAD_8", SDL_SCANCODE_KP_8},
        {"KEYBOARD_KEYPAD_9", SDL_SCANCODE_KP_9}, {"KEYBOARD_KEYPAD_0", SDL_SCANCODE_KP_0},
        {"KEYBOARD_KEYPAD_PLUS", SDL_SCANCODE_KP_PLUS}, {"KEYBOARD_KEYPAD_MINUS", SDL_SCANCODE_KP_MINUS},
        {"KEYBOARD_KEYPAD_ASTERISK", SDL_SCANCODE_KP_MULTIPLY},
    };
    // ENGINE PATCH (this session): the PS3-no-keyboard reasoning below was
    // Sound Shapes-only -- FoE's real binary is `flowers of error.exe`, a PC
    // build with a real keyboard, and PekkoAndJon/Player.gm reads
    // Input.isKeyPressed(KEYBOARD_W/A/S/D/UP/DOWN/LEFT/RIGHT) directly (not
    // via joystick axis), plus Main.gm's key-event dispatch
    // (KEYBOARD_ESCAPE/F/M/P/B/I/R/0/6/7) drives pause/fullscreen/mute/debug
    // toggles including showingBoundingBox. With every KEYBOARD_* dead,
    // none of that ever fired -- title screen loaded but gameplay input did
    // nothing. Reverted to live scancodes, exactly the "one-line revert"
    // this comment used to promise. (Original PS3 reasoning, kept for
    // context: retail PS3 hardware has no keyboard peripheral, so
    // isKeyPressed(KEYBOARD_*) should never see a real one on that
    // platform -- still true for Sound Shapes, not for FoE.)
    for (auto& k : kKeys) { g->Set(&a_machine, k.name, gmVariable((gmint)k.code)); }
}

void SetVirtualKeyDown(int a_scancode, bool a_down) {
    if (a_scancode <= SDL_SCANCODE_UNKNOWN || a_scancode >= SDL_NUM_SCANCODES) return;
    g_virtualKeys[a_scancode] = a_down;
}

void ClearVirtualKeys() {
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) g_virtualKeys[i] = false;
}
