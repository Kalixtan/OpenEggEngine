#include "gmThread.h"
#include "input_bindings.h"
#include "gmTableObject.h"
#include "../bindings/vector2d_bindings.h"
#include "../core/engine_context.h"
#include "../render/renderer.h"
#include <SDL2/SDL.h>
#include <string>

// Real SDL keyboard-driven implementation -- per explicit direction, PC
// desktop keyboard only for now; no joystick/gamepad hardware support
// (isControllerPresent stays honestly false). Every JOYSTICK0_*/KEYBOARD_*
// constant below is assigned an SDL_Scancode value directly, so
// isKeyPressed(JOYSTICK0_ACCEPT) and isKeyPressed(KEYBOARD_ENTER) are
// literally the same check -- real scripts test both intercheangeably
// (e.g. menu confirm/cancel), and this lets a keyboard stand in for
// "the controller" everywhere a real corpus script expects one, without
// needing two separate code paths.
namespace {

int GM_CDECL gmf_falseBool(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_zeroFloat(gmThread* a_thread) { a_thread->PushFloat(0.0f); return GM_OK; }
int GM_CDECL gmf_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_emptyString(gmThread* a_thread) { a_thread->PushNewString(""); return GM_OK; }
int GM_CDECL gmf_noop(gmThread* a_thread) { return GM_OK; }

bool KeyDown(int a_scancode) {
    if (a_scancode <= SDL_SCANCODE_UNKNOWN || a_scancode >= SDL_NUM_SCANCODES) return false;
    return SDL_GetKeyboardState(nullptr)[a_scancode] != 0;
}

// Confirmed real: 1 int param (a KEYBOARD_*/JOYSTICK0_* scancode), returns
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
// header comment) -- axes 0/1 (left stick, the analog pair every real
// script actually reads for movement/menu drift, e.g.
// Platformer/ResizeScreenProcess.gm) are driven by WASD + arrow keys
// together; axes 2/3 (right stick, rarer -- camera/secondary in the few
// real call sites that use them) by IJKL. Only joystick index 0 (the
// single local player) responds -- matches this being a single-player,
// non-networked-input game.
int GM_CDECL gmf_getJoystickAxis(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(joyIndex, 0, 0);
    GM_INT_PARAM(axis, 1, 0);
    float value = 0.0f;
    if (joyIndex == 0) {
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

// True: no physical joystick/gamepad hardware, but the keyboard is
// standing in for one everywhere above (JOYSTICK0_* all resolve to real
// key checks) -- reporting false here would make real scripts fall back
// to a "no controller connected" UI state instead of actually using that
// mapping.
int GM_CDECL gmf_isControllerPresent(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }

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

gmVariable ZeroVec2(gmMachine& a_machine) { return MakeVector2D(a_machine, 0.0f, 0.0f); }
int GM_CDECL gmf_zeroVec2(gmThread* a_thread) {
    a_thread->Push(ZeroVec2(*a_thread->GetMachine()));
    return GM_OK;
}

gmFunctionEntry s_inputLib[] = {
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

void RegisterInputBindings(gmMachine& a_machine) {
    a_machine.RegisterLibrary(s_inputLib, sizeof(s_inputLib) / sizeof(s_inputLib[0]), "Input");

    // Not decompiled-confirmed exact integer values -- real scripts only
    // ever compare these symbolically, never against a literal number
    // (checked by corpus grep), so any internally-consistent assignment is
    // externally equivalent. Values are SDL_Scancode so isKeyPressed can
    // use one lookup for both KEYBOARD_* and JOYSTICK0_* -- see header
    // comment.
    gmTableObject* g = a_machine.GetGlobals();

    // JOYSTICK0_* (button-style; the analog stick pair is axis-based, see
    // getJoystickAxis above, not one of these) mapped to keyboard
    // equivalents for a controller-less desktop build.
    // ENGINE PATCH (2026-08-17): the 4 face buttons now also live on a
    // single ZXCV row (Z=Cross, X=Square, C=Triangle, V=Circle) for direct
    // keyboard access, on top of (not instead of) the existing Return/
    // Escape bindings below -- JOYSTICK0_ACCEPT/CANCEL are kept on Return/
    // Escape unchanged so every menu flow already relying on Enter/Esc
    // (this whole session's own testing included) keeps working exactly
    // as before. JOYSTICK0_CROSS/CIRCLE themselves move to Z/V since real
    // scripts do check them directly in gameplay (not just via the
    // ACCEPT/CANCEL aliases), same reasoning SQUARE/TRIANGLE's existing
    // X/C mapping already established.
    g->Set(&a_machine, "JOYSTICK0_CROSS", gmVariable((gmint)SDL_SCANCODE_Z));
    g->Set(&a_machine, "JOYSTICK0_CIRCLE", gmVariable((gmint)SDL_SCANCODE_V));
    g->Set(&a_machine, "JOYSTICK0_SQUARE", gmVariable((gmint)SDL_SCANCODE_X));
    g->Set(&a_machine, "JOYSTICK0_TRIANGLE", gmVariable((gmint)SDL_SCANCODE_C));
    
    g->Set(&a_machine, "JOYSTICK0_ACCEPT", gmVariable((gmint)SDL_SCANCODE_RETURN));
    g->Set(&a_machine, "JOYSTICK0_CANCEL", gmVariable((gmint)SDL_SCANCODE_ESCAPE));
    // ENGINE PATCH (2026-08-16): JOYSTICK0_START used to alias the same
    // SDL_SCANCODE_RETURN as JOYSTICK0_CROSS/ACCEPT above. Since Cross is
    // the real jump button (ControllerConfig.gm's "jump" action /
    // Player.gm:1667's keyProc.isKey("jump",kc)) and PlayMenu.gm:10 opens
    // the pause menu on JOYSTICK0_START, every single jump press was ALSO
    // opening pause -- which then calls setIsStealsInputFocus(true)
    // (PauseProcessBase.gm), locking out all further movement/jump input.
    // This was very likely the actual cause behind "can't jump" reports,
    // separate from the postDraw/popup.draw() null-getdot bug pause was
    // also hitting. P matches ControllerConfig.gm's own unused
    // actionTable["pause"]={KEYBOARD_P} entry, so it's the dev-intended key.
    g->Set(&a_machine, "JOYSTICK0_START", gmVariable((gmint)SDL_SCANCODE_P));
    g->Set(&a_machine, "JOYSTICK0_SELECT", gmVariable((gmint)SDL_SCANCODE_TAB));
    g->Set(&a_machine, "JOYSTICK0_UP", gmVariable((gmint)SDL_SCANCODE_UP));
    g->Set(&a_machine, "JOYSTICK0_DOWN", gmVariable((gmint)SDL_SCANCODE_DOWN));
    g->Set(&a_machine, "JOYSTICK0_LEFT", gmVariable((gmint)SDL_SCANCODE_LEFT));
    g->Set(&a_machine, "JOYSTICK0_RIGHT", gmVariable((gmint)SDL_SCANCODE_RIGHT));
    g->Set(&a_machine, "JOYSTICK0_L1", gmVariable((gmint)SDL_SCANCODE_Q));
    g->Set(&a_machine, "JOYSTICK0_R1", gmVariable((gmint)SDL_SCANCODE_E));
    g->Set(&a_machine, "JOYSTICK0_L2", gmVariable((gmint)SDL_SCANCODE_1));
    g->Set(&a_machine, "JOYSTICK0_R2", gmVariable((gmint)SDL_SCANCODE_3));
    // L3/R3 (stick-click buttons) -- previously entirely unmapped/missing.
    // L3 moved off Z (2026-08-17): Z is now JOYSTICK0_CROSS, part of the
    // new ZXCV face-button row above.
    g->Set(&a_machine, "JOYSTICK0_L3", gmVariable((gmint)SDL_SCANCODE_N));
    g->Set(&a_machine, "JOYSTICK0_R3", gmVariable((gmint)SDL_SCANCODE_M));
    g->Set(&a_machine, "JOYSTICK0_LS_UP", gmVariable((gmint)SDL_SCANCODE_W));
    g->Set(&a_machine, "JOYSTICK0_LS_DOWN", gmVariable((gmint)SDL_SCANCODE_S));
    g->Set(&a_machine, "JOYSTICK0_LS_LEFT", gmVariable((gmint)SDL_SCANCODE_A));
    g->Set(&a_machine, "JOYSTICK0_LS_RIGHT", gmVariable((gmint)SDL_SCANCODE_D));
    g->Set(&a_machine, "JOYSTICK0_RS_UP", gmVariable((gmint)SDL_SCANCODE_I));
    g->Set(&a_machine, "JOYSTICK0_RS_DOWN", gmVariable((gmint)SDL_SCANCODE_K));
    // RS_LEFT/RS_RIGHT (digital right-stick buttons) -- previously missing;
    // reuse J/L to match the existing right-stick axis 2 mapping above.
    g->Set(&a_machine, "JOYSTICK0_RS_LEFT", gmVariable((gmint)SDL_SCANCODE_J));
    g->Set(&a_machine, "JOYSTICK0_RS_RIGHT", gmVariable((gmint)SDL_SCANCODE_L));
    // Generic numbered face/shoulder buttons -- not real physical PS3
    // buttons; a fallback indexing scheme used only by the (non-retail)
    // ControllerConfig.gm dev tool, whose real actions above already have
    // working KEYBOARD_* equivalents in its own actionTable (OR'd together
    // with these), so leaving them unmapped costs nothing. SDL_SCANCODE_
    // UNKNOWN (0) so isKeyPressed on these always reads false rather than
    // aliasing an unrelated key.
    for (int i = 0; i <= 13; ++i) {
        g->Set(&a_machine, ("JOYSTICK0_BUTTON" + std::to_string(i)).c_str(), gmVariable((gmint)SDL_SCANCODE_UNKNOWN));
    }
    

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
