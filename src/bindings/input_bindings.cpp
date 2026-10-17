#include "gmThread.h"
#include "input_bindings.h"
#include "gmTableObject.h"
#include "../bindings/vector2d_bindings.h"
#include "../core/engine_context.h"
#include "../render/renderer.h"
#include <string>
#include <cctype>
#include <cmath>
#include <limits>

// Frontend-neutral input bindings. Keyboard usage codes and logical pad
// buttons are translated by the active platform frontend.
namespace {

constexpr int kNumberedButtonBase = 1024;
constexpr int kAliasBase = 2048;
struct ControllerAlias {
    const char* name;
    int button;
    int fallback;
};
const ControllerAlias kAliases[] = {
    {"JOYSTICK0_CROSS", PlatformPadButton::A, KeyCode::Z},
    {"JOYSTICK0_CIRCLE", PlatformPadButton::B, KeyCode::V},
    {"JOYSTICK0_SQUARE", PlatformPadButton::X, KeyCode::X},
    {"JOYSTICK0_TRIANGLE", PlatformPadButton::Y, KeyCode::C},
    {"JOYSTICK0_ACCEPT", PlatformPadButton::A, KeyCode::Return},
    {"JOYSTICK0_CANCEL", PlatformPadButton::B, KeyCode::Escape},
    {"JOYSTICK0_START", PlatformPadButton::Start, KeyCode::P},
    {"JOYSTICK0_SELECT", PlatformPadButton::Back, KeyCode::Tab},
    {"JOYSTICK0_UP", PlatformPadButton::DpadUp, KeyCode::Up},
    {"JOYSTICK0_DOWN", PlatformPadButton::DpadDown, KeyCode::Down},
    {"JOYSTICK0_LEFT", PlatformPadButton::DpadLeft, KeyCode::Left},
    {"JOYSTICK0_RIGHT", PlatformPadButton::DpadRight, KeyCode::Right},
    {"JOYSTICK0_L1", PlatformPadButton::LeftShoulder, KeyCode::Q},
    {"JOYSTICK0_R1", PlatformPadButton::RightShoulder, KeyCode::E},
    {"JOYSTICK0_L3", PlatformPadButton::LeftStick, KeyCode::N},
    {"JOYSTICK0_R3", PlatformPadButton::RightStick, KeyCode::M},
    {"JOYSTICK0_LS_UP", PlatformPadButton::DpadUp, KeyCode::W},
    {"JOYSTICK0_LS_DOWN", PlatformPadButton::DpadDown, KeyCode::S},
    {"JOYSTICK0_LS_LEFT", PlatformPadButton::DpadLeft, KeyCode::A},
    {"JOYSTICK0_LS_RIGHT", PlatformPadButton::DpadRight, KeyCode::D},
};
constexpr int kNumberedButtons[] = {
    PlatformPadButton::A, PlatformPadButton::B,
    PlatformPadButton::X, PlatformPadButton::Y,
    PlatformPadButton::LeftShoulder, PlatformPadButton::RightShoulder,
    PlatformPadButton::Back, PlatformPadButton::Start,
    PlatformPadButton::LeftStick, PlatformPadButton::RightStick,
    PlatformPadButton::DpadUp, PlatformPadButton::DpadDown,
    PlatformPadButton::DpadLeft, PlatformPadButton::DpadRight
};
constexpr int kNumberedButtonCount = sizeof(kNumberedButtons) / sizeof(kNumberedButtons[0]);
constexpr int kAliasCount = sizeof(kAliases) / sizeof(kAliases[0]);

int AliasCode(int a_index) {
    const std::string name = kAliases[a_index].name;
    if (name == "JOYSTICK0_ACCEPT") {
        for (int i = 0; i < kAliasCount; ++i)
            if (std::string(kAliases[i].name) == "JOYSTICK0_CROSS") return kAliasBase + i;
    } else if (name == "JOYSTICK0_CANCEL") {
        for (int i = 0; i < kAliasCount; ++i)
            if (std::string(kAliases[i].name) == "JOYSTICK0_CIRCLE") return kAliasBase + i;
    }
    return kAliasBase + a_index;
}

int GM_CDECL gmf_falseBool(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_trueBool(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }
int GM_CDECL gmf_zeroFloat(gmThread* a_thread) { a_thread->PushFloat(0.0f); return GM_OK; }
int GM_CDECL gmf_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_emptyString(gmThread* a_thread) { a_thread->PushNewString(""); return GM_OK; }
int GM_CDECL gmf_noop(gmThread* a_thread) { return GM_OK; }

// These retail Input methods target the DualShock speaker, which has no
// desktop equivalent. Preserve the argument contracts used by the shipped
// scripts while making the unavailable hardware output a no-op.
int GM_CDECL gmf_loadPadSpeakerSound(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(filename, 0, "");
    (void)filename;
    return GM_OK;
}
int GM_CDECL gmf_playPadSpeakerSound(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(filename, 0, "");
    (void)filename;
    return GM_OK;
}
int GM_CDECL gmf_getPadSpeakerVolume(gmThread* a_thread) {
    a_thread->PushFloat(0.0f);
    return GM_OK;
}
int GM_CDECL gmf_setPadSpeakerVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(volume, 0, 0.0f);
    (void)volume;
    return GM_OK;
}

// Virtual key state -- see SetVirtualKeyDown's header comment. File-scope
// (not in the anonymous namespace) so the setters below can reach it.
bool g_virtualKeys[KeyCode::Count] = {false};
bool g_virtualMouseValid = false;
int g_virtualMouseX = 0, g_virtualMouseY = 0;
bool g_virtualMouseButtons[3] = {false, false, false};
bool g_inputEnabled = true;
bool g_disabledUntilAllKeysUp = false;
bool g_doingRealtimeUpdate = false;
float g_typeRate = 0.0f;
float g_typeDelay = 0.0f;
float g_joystickDeadzone = 0.0f;
float g_joystickAsButtonThreshold = 0.0f;
PlatformFrontend* g_frontend = nullptr;

bool AllInputsReleased();

PlatformFrontend& Frontend() { return *g_frontend; }

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
    // Retail executes `fadds value, 0.5f` followed by `fctiwz`. Force the
    // addition to binary32, then model PPC's signed-word conversion results:
    // overflows saturate, while NaN produces the indefinite INT_MIN value.
    volatile float adjusted = value + 0.5f;
    gmint result;
    if (std::isnan(adjusted) || adjusted < -2147483648.0f) {
        result = std::numeric_limits<gmint>::min();
    } else if (adjusted >= 2147483648.0f) {
        result = std::numeric_limits<gmint>::max();
    } else {
        result = static_cast<gmint>(adjusted); // truncation toward zero
    }
    a_thread->PushInt(result);
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

int GM_CDECL gmf_setSoftRumble(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    // Retail forwards argument 0 but the concrete PS3 vtable method ignores
    // it and writes argument 1's boolean value to the currently active pad.
    // Preserve the raw GM payload comparison used by the retail wrapper.
    const bool enabled = a_thread->GetBase()[1].m_value.m_int != 0;
    if (g_frontend) Frontend().SetActiveControllerSoftRumble(enabled);
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
    // Retail clears this flag in its per-frame Input update once all 155
    // digital input slots are up. This port has no equivalent shared input
    // object update, so refresh the same observable state when queried.
    if (g_disabledUntilAllKeysUp && AllInputsReleased())
        g_disabledUntilAllKeysUp = false;
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
    if (!g_frontend) return false;
    if (code >= 0 && code < 3)
        return g_virtualMouseButtons[code] || Frontend().IsMouseButtonDown(code);
    if (code >= kNumberedButtonBase && code < kNumberedButtonBase + kNumberedButtonCount)
        return Frontend().IsControllerButtonDown(0, kNumberedButtons[code - kNumberedButtonBase]);
    if (code >= kAliasBase && code < kAliasBase + kAliasCount) {
        const ControllerAlias& alias = kAliases[code - kAliasBase];
        if (Frontend().IsControllerButtonDown(0, alias.button)) return true;
        code = alias.fallback;
    }
    if (code <= KeyCode::Unknown || code >= KeyCode::Count) return false;
    if (g_virtualKeys[code]) return true;
    return Frontend().IsKeyDown(code);
}

bool AllInputsReleased() {
    for (int code = 0; code < KeyCode::Count; ++code) {
        if (g_virtualKeys[code]) return false;
    }
    if (!g_frontend) return true;
    for (int code = 0; code < KeyCode::Count; ++code) {
        if (KeyDown(code)) return false;
    }
    for (int button = 0; button < 3; ++button) {
        if (g_virtualMouseButtons[button] || Frontend().IsMouseButtonDown(button))
            return false;
    }
    for (int i = 0; i < kNumberedButtonCount; ++i) {
        if (Frontend().IsControllerButtonDown(0, kNumberedButtons[i]))
            return false;
    }
    return true;
}

// Confirmed real: 1 int param (a KEYBOARD_*/JOYSTICK0_* code), returns
int GM_CDECL gmf_isKeyPressed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(code, 0, 0);
    a_thread->PushInt(KeyDown(code) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_isShiftPressed(gmThread* a_thread) {
    a_thread->PushInt((KeyDown(KeyCode::LeftShift) || KeyDown(KeyCode::RightShift)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_isControlPressed(gmThread* a_thread) {
    a_thread->PushInt((KeyDown(KeyCode::LeftControl) || KeyDown(KeyCode::RightControl)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_isAltPressed(gmThread* a_thread) {
    a_thread->PushInt((KeyDown(KeyCode::LeftAlt) || KeyDown(KeyCode::RightAlt)) ? 1 : 0);
    return GM_OK;
}

// Confirmed real: 2 int params (joystick index, axis index), returns
// as the left stick and 2/3 as the right stick. The platform frontend maps
// physical controller layouts into these logical axes;
// keyboard emulation remains available when no compatible pad is attached.
int GM_CDECL gmf_getJoystickAxis(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(joyIndex, 0, 0);
    GM_INT_PARAM(axis, 1, 0);
    float value = 0.0f;
    if (g_frontend && Frontend().IsControllerPresent(joyIndex)) {
        value = Frontend().ControllerAxis(joyIndex, axis);
    } else if (joyIndex == 0) {
        if (axis == 0) {
            if (KeyDown(KeyCode::D) || KeyDown(KeyCode::Right)) value += 1.0f;
            if (KeyDown(KeyCode::A) || KeyDown(KeyCode::Left)) value -= 1.0f;
        } else if (axis == 1) {
            if (KeyDown(KeyCode::W) || KeyDown(KeyCode::Up)) value += 1.0f;
            if (KeyDown(KeyCode::S) || KeyDown(KeyCode::Down)) value -= 1.0f;
        } else if (axis == 2) {
            if (KeyDown(KeyCode::L)) value += 1.0f;
            if (KeyDown(KeyCode::J)) value -= 1.0f;
        } else if (axis == 3) {
            if (KeyDown(KeyCode::I)) value += 1.0f;
            if (KeyDown(KeyCode::K)) value -= 1.0f;
        }
    }
    a_thread->PushFloat(value);
    return GM_OK;
}

int GM_CDECL gmf_isControllerPresent(gmThread* a_thread) {
    // Platformer_PlayMenu treats a false result as a controller-disconnect
    // event and starts the pause process every update. Desktop has a fully
    // supported keyboard input path (including stick emulation), so it is a
    // usable local gameplay device even when no game controller is attached.
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
    int x = 0, y = 0;
    if (g_frontend) Frontend().MousePosition(x, y);
    if (g_virtualMouseValid) { x = g_virtualMouseX; y = g_virtualMouseY; }
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
// USB HID key usage values directly (see header comment above), this maps
// those stable key codes -> ASCII the same way, respecting the shift flag for
// letter case and the standard US-QWERTY shifted symbols.
int GM_CDECL gmf_KEYCODE_GetAscii(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(code, 0, 0);
    bool shift = false;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(s, 1, 0); shift = (s != 0); }

    char c = 0;
    if (code >= KeyCode::A && code <= KeyCode::Z) {
        c = (char)('a' + (code - KeyCode::A));
        if (shift) c = (char)std::toupper((unsigned char)c);
    } else if (code >= KeyCode::Num1 && code <= KeyCode::Num9) {
        static const char kShifted[] = "!@#$%^&*(";
        c = shift ? kShifted[code - KeyCode::Num1] : (char)('1' + (code - KeyCode::Num1));
    } else if (code == KeyCode::Num0) {
        c = shift ? ')' : '0';
    } else if (code == KeyCode::Space) {
        c = ' ';
    } else {
        switch (code) {
            case KeyCode::Minus: c = shift ? '_' : '-'; break;
            case KeyCode::Equals: c = shift ? '+' : '='; break;
            case KeyCode::Comma: c = shift ? '<' : ','; break;
            case KeyCode::Period: c = shift ? '>' : '.'; break;
            case KeyCode::Slash: c = shift ? '?' : '/'; break;
            case KeyCode::Backslash: c = shift ? '|' : '\\'; break;
            case KeyCode::Semicolon: c = shift ? ':' : ';'; break;
            case KeyCode::Apostrophe: c = shift ? '"' : '\''; break;
            case KeyCode::LeftBracket: c = shift ? '{' : '['; break;
            case KeyCode::RightBracket: c = shift ? '}' : ']'; break;
            case KeyCode::Grave: c = shift ? '~' : '`'; break;
            case KeyCode::Keypad0: c = '0'; break;
            case KeyCode::Keypad1: c = '1'; break;
            case KeyCode::Keypad2: c = '2'; break;
            case KeyCode::Keypad3: c = '3'; break;
            case KeyCode::Keypad4: c = '4'; break;
            case KeyCode::Keypad5: c = '5'; break;
            case KeyCode::Keypad6: c = '6'; break;
            case KeyCode::Keypad7: c = '7'; break;
            case KeyCode::Keypad8: c = '8'; break;
            case KeyCode::Keypad9: c = '9'; break;
            case KeyCode::KeypadPlus: c = '+'; break;
            case KeyCode::KeypadMinus: c = '-'; break;
            case KeyCode::KeypadMultiply: c = '*'; break;
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
    // The retail touch wrappers return integer zero for force/duration and a
    // zero Vector2D for the initial position. Desktop has no touch surface.
    {"getTouchForce", gmf_zeroInt, nullptr},
    {"getTouchDuration", gmf_zeroInt, nullptr},
    {"getInitialTouchPos", gmf_zeroVec2, nullptr},
    {"getMousePosVec", gmf_getMousePosVec, nullptr},
    {"getMousePos", gmf_getMousePos, nullptr},
    {"getMousePosX", gmf_getMousePosX, nullptr},
    {"getMousePosY", gmf_getMousePosY, nullptr},
    {"getFrontTouchCount", gmf_zeroInt, nullptr},
    {"getBackTouchCount", gmf_zeroInt, nullptr},
    {"getWasTouchTap", gmf_falseBool, nullptr},
    {"getIsTouchMoving", gmf_falseBool, nullptr},
    {"getIsTouchHold", gmf_falseBool, nullptr},
    // The selected retail PS3 input vtable's +0x74 target is a bare `blr`.
    // The wrapper booleanizes its unchanged non-null `this` pointer, so the
    // observable result is always true for this concrete retail input type.
    {"getIsShaking", gmf_trueBool, nullptr},
    {"openIME", gmf_noop, nullptr},
    // Retail getIMEResults is currently a literal integer-zero stub (Input.c
    // 0x74148), not an empty string. Keep the GM value type identical.
    {"getIMEResults", gmf_zeroInt, nullptr},
    {"getIMEStatus", gmf_zeroInt, nullptr},
    {"getStringIME", gmf_emptyString, nullptr},
    {"disableShaking", gmf_noop, nullptr},
    {"enableShaking", gmf_noop, nullptr},
    {"enableBackPanel", gmf_noop, nullptr},
    {"disableBackPanel", gmf_noop, nullptr},
    {"setSoftRumble", gmf_setSoftRumble, nullptr},
    {"loadPadSpeakerSound", gmf_loadPadSpeakerSound, nullptr},
    {"playPadSpeakerSound", gmf_playPadSpeakerSound, nullptr},
    {"getPadSpeakerVolume", gmf_getPadSpeakerVolume, nullptr},
    {"setPadSpeakerVolume", gmf_setPadSpeakerVolume, nullptr},
    // Retail Input.suppressSharing at EBOOT 0x74838 has an empty body and
    // returns status 0. BaseNotePlant calls it for notes marked as sharing-
    // suppressed; keep the call registered without inventing desktop policy.
    {"suppressSharing", gmf_noop, nullptr},
};

} // namespace

void RefreshPlatformControllers() {
    if (g_frontend) Frontend().RefreshControllers();
}

int ControllerEventCodes(int instance, int button, int* codes, int capacity) {
    if (!g_frontend) return 0;
    const int controllerIndex = Frontend().ControllerIndexForInstance(instance);
    if (controllerIndex < 0) return 0;
    int count = 0;
    for (int i = 0; i < kNumberedButtonCount; ++i) {
        if (kNumberedButtons[i] == button && count < capacity)
            codes[count++] = kNumberedButtonBase + i;
    }
    for (int i = 0; i < kAliasCount; ++i) {
        if (std::string(kAliases[i].name).find("JOYSTICK0_LS_") == 0) continue;
        if (kAliases[i].button != button) continue;
        const int code = AliasCode(i);
        bool alreadyAdded = false;
        for (int j = 0; j < count; ++j) alreadyAdded |= codes[j] == code;
        if (!alreadyAdded && count < capacity) codes[count++] = code;
    }
    return count;
}

int KeyboardJoystickAliasCodes(int scancode, int* codes, int capacity) {
    int count = 0;
    for (int i = 0; i < kAliasCount; ++i) {
        if (kAliases[i].fallback == scancode && count < capacity) {
            const int code = AliasCode(i);
            bool alreadyAdded = false;
            for (int j = 0; j < count; ++j) alreadyAdded |= codes[j] == code;
            if (!alreadyAdded) codes[count++] = code;
        }
    }
    return count;
}

int HeldControllerCodes(int* codes, int capacity) {
    if (!g_frontend || !Frontend().IsControllerPresent(0)) return 0;
    int count = 0;
    for (int i = 0; i < kNumberedButtonCount; ++i) {
        if (Frontend().IsControllerButtonDown(0, kNumberedButtons[i]) && count < capacity)
            codes[count++] = kNumberedButtonBase + i;
    }
    for (int i = 0; i < kAliasCount; ++i) {
        if (std::string(kAliases[i].name).find("JOYSTICK0_LS_") == 0) continue;
        if (!Frontend().IsControllerButtonDown(0, kAliases[i].button)) continue;
        const int code = AliasCode(i);
        bool alreadyAdded = false;
        for (int j = 0; j < count; ++j) alreadyAdded |= codes[j] == code;
        if (!alreadyAdded && count < capacity) codes[count++] = code;
    }
    return count;
}

void RegisterInputBindings(gmMachine& a_machine, PlatformFrontend& a_frontend) {
    g_frontend = &a_frontend;
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
    int crossCode = kAliasBase;
    int circleCode = kAliasBase + 1;
    for (int i = 0; i < kAliasCount; ++i) {
        if (std::string(kAliases[i].name) == "JOYSTICK0_CROSS") crossCode = kAliasBase + i;
        if (std::string(kAliases[i].name) == "JOYSTICK0_CIRCLE") circleCode = kAliasBase + i;
    }
    for (int i = 0; i < kAliasCount; ++i) {
        int code = AliasCode(i);
        // Retail menus use CROSS/ACCEPT and CIRCLE/CANCEL as platform
        // synonyms. They must be one logical key event for one physical
        // button; otherwise a handler that accepts either spelling runs
        // twice when desktop input emits both aliases.
        if (std::string(kAliases[i].name) == "JOYSTICK0_ACCEPT") code = crossCode;
        if (std::string(kAliases[i].name) == "JOYSTICK0_CANCEL") code = circleCode;
        g->Set(&a_machine, kAliases[i].name, gmVariable((gmint)code));
    }

    // Remaining aliases still use keyboard fallbacks until trigger and
    // right-stick digital thresholds are implemented.
    g->Set(&a_machine, "JOYSTICK0_L2", gmVariable((gmint)KeyCode::Num1));
    g->Set(&a_machine, "JOYSTICK0_R2", gmVariable((gmint)KeyCode::Num3));
    g->Set(&a_machine, "JOYSTICK0_RS_UP", gmVariable((gmint)KeyCode::I));
    g->Set(&a_machine, "JOYSTICK0_RS_DOWN", gmVariable((gmint)KeyCode::K));
    g->Set(&a_machine, "JOYSTICK0_RS_LEFT", gmVariable((gmint)KeyCode::J));
    g->Set(&a_machine, "JOYSTICK0_RS_RIGHT", gmVariable((gmint)KeyCode::L));

    g->Set(&a_machine, "MOUSE_LEFTBUTTON", gmVariable((gmint)0));
    g->Set(&a_machine, "MOUSE_RIGHTBUTTON", gmVariable((gmint)1));
    g->Set(&a_machine, "MOUSE_MIDDLEBUTTON", gmVariable((gmint)2));

    // KEYBOARD_* -- stable USB HID keyboard-page usage values.
    struct { const char* name; int code; } kKeys[] = {
        {"KEYBOARD_A", KeyCode::A}, {"KEYBOARD_B", KeyCode::B}, {"KEYBOARD_C", KeyCode::C},
        {"KEYBOARD_D", KeyCode::D}, {"KEYBOARD_E", KeyCode::E}, {"KEYBOARD_F", KeyCode::F},
        {"KEYBOARD_G", KeyCode::G}, {"KEYBOARD_H", KeyCode::H}, {"KEYBOARD_I", KeyCode::I},
        {"KEYBOARD_J", KeyCode::J}, {"KEYBOARD_K", KeyCode::K}, {"KEYBOARD_L", KeyCode::L},
        {"KEYBOARD_M", KeyCode::M}, {"KEYBOARD_N", KeyCode::N}, {"KEYBOARD_O", KeyCode::O},
        {"KEYBOARD_P", KeyCode::P}, {"KEYBOARD_Q", KeyCode::Q}, {"KEYBOARD_R", KeyCode::R},
        {"KEYBOARD_S", KeyCode::S}, {"KEYBOARD_T", KeyCode::T}, {"KEYBOARD_U", KeyCode::U},
        {"KEYBOARD_V", KeyCode::V}, {"KEYBOARD_W", KeyCode::W}, {"KEYBOARD_X", KeyCode::X},
        {"KEYBOARD_Y", KeyCode::Y}, {"KEYBOARD_Z", KeyCode::Z},
        {"KEYBOARD_0", KeyCode::Num0}, {"KEYBOARD_1", KeyCode::Num1}, {"KEYBOARD_2", KeyCode::Num2},
        {"KEYBOARD_3", KeyCode::Num3}, {"KEYBOARD_4", KeyCode::Num4}, {"KEYBOARD_5", KeyCode::Num5},
        {"KEYBOARD_6", KeyCode::Num6}, {"KEYBOARD_7", KeyCode::Num7}, {"KEYBOARD_8", KeyCode::Num8},
        {"KEYBOARD_9", KeyCode::Num9},
        {"KEYBOARD_UP", KeyCode::Up}, {"KEYBOARD_DOWN", KeyCode::Down},
        {"KEYBOARD_LEFT", KeyCode::Left}, {"KEYBOARD_RIGHT", KeyCode::Right},
        {"KEYBOARD_ENTER", KeyCode::Return}, {"KEYBOARD_ESCAPE", KeyCode::Escape},
        {"KEYBOARD_BACKSPACE", KeyCode::Backspace}, {"KEYBOARD_DELETE", KeyCode::Delete},
        {"KEYBOARD_TAB", KeyCode::Tab}, {"KEYBOARD_SPACE", KeyCode::Space},
        {"KEYBOARD_HOME", KeyCode::Home}, {"KEYBOARD_END", KeyCode::End},
        {"KEYBOARD_INSERT", KeyCode::Insert},
        {"KEYBOARD_PAGEUP", KeyCode::PageUp}, {"KEYBOARD_PAGEDOWN", KeyCode::PageDown},
        {"KEYBOARD_LEFTSHIFT", KeyCode::LeftShift}, {"KEYBOARD_RIGHTSHIFT", KeyCode::RightShift},
        {"KEYBOARD_LEFTCONTROL", KeyCode::LeftControl}, {"KEYBOARD_RIGHTCONTROL", KeyCode::RightControl},
        {"KEYBOARD_LEFTALT", KeyCode::LeftAlt},
        {"KEYBOARD_MINUS", KeyCode::Minus}, {"KEYBOARD_PLUS", KeyCode::Equals},
        {"KEYBOARD_COMMA", KeyCode::Comma}, {"KEYBOARD_PERIOD", KeyCode::Period},
        {"KEYBOARD_SLASH", KeyCode::Slash}, {"KEYBOARD_BACKSLASH", KeyCode::Backslash},
        {"KEYBOARD_LEFTSQUAREBRACKET", KeyCode::LeftBracket},
        {"KEYBOARD_RIGHTSQUAREBRACKET", KeyCode::RightBracket},
        {"KEYBOARD_F1", KeyCode::F1}, {"KEYBOARD_F2", KeyCode::F2}, {"KEYBOARD_F3", KeyCode::F3},
        {"KEYBOARD_F4", KeyCode::F4}, {"KEYBOARD_F5", KeyCode::F5}, {"KEYBOARD_F6", KeyCode::F6},
        {"KEYBOARD_F7", KeyCode::F7}, {"KEYBOARD_F8", KeyCode::F8}, {"KEYBOARD_F9", KeyCode::F9},
        {"KEYBOARD_F10", KeyCode::F10}, {"KEYBOARD_F11", KeyCode::F11}, {"KEYBOARD_F12", KeyCode::F12},
        {"KEYBOARD_KEYPAD_1", KeyCode::Keypad1}, {"KEYBOARD_KEYPAD_2", KeyCode::Keypad2},
        {"KEYBOARD_KEYPAD_3", KeyCode::Keypad3}, {"KEYBOARD_KEYPAD_4", KeyCode::Keypad4},
        {"KEYBOARD_KEYPAD_5", KeyCode::Keypad5}, {"KEYBOARD_KEYPAD_6", KeyCode::Keypad6},
        {"KEYBOARD_KEYPAD_7", KeyCode::Keypad7}, {"KEYBOARD_KEYPAD_8", KeyCode::Keypad8},
        {"KEYBOARD_KEYPAD_9", KeyCode::Keypad9}, {"KEYBOARD_KEYPAD_0", KeyCode::Keypad0},
        {"KEYBOARD_KEYPAD_PLUS", KeyCode::KeypadPlus}, {"KEYBOARD_KEYPAD_MINUS", KeyCode::KeypadMinus},
        {"KEYBOARD_KEYPAD_ASTERISK", KeyCode::KeypadMultiply},
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
    if (a_scancode <= KeyCode::Unknown || a_scancode >= KeyCode::Count) return;
    g_virtualKeys[a_scancode] = a_down;
}

void ClearVirtualKeys() {
    for (int i = 0; i < KeyCode::Count; ++i) g_virtualKeys[i] = false;
}

void SetVirtualMousePosition(int a_x, int a_y) {
    g_virtualMouseValid = true;
    g_virtualMouseX = a_x;
    g_virtualMouseY = a_y;
}

void SetVirtualMouseButtonDown(int a_buttonCode, bool a_down) {
    if (a_buttonCode >= 0 && a_buttonCode < 3)
        g_virtualMouseButtons[a_buttonCode] = a_down;
}
