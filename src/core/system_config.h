#pragma once
#include <string>

// back to scripts.
//
// Originally defaulted to pretending to be a retail PS3 build (Sound
// Shapes' own game_data was extracted from that build). This project also
// now runs Flowers of Error, whose retail binary IS a real PC (WIN32) exe
// (`flowers of error.exe`) -- confirmed via Ghidra (reveng_pc/) against
// that actual binary, not inferred -- so osId defaults to WIN32 now. No
// FoE script in the diagnostic scene reads System.getOS()/OSID_* directly, but
// getting this right matters for anything that keys off it going forward
// (e.g. keyboard input semantics -- FoE's Player.gm uses
// Input.isKeyPressed(KEYBOARD_*) extensively, which only makes sense on a
// PC target, not the PS3-no-keyboard assumption elsewhere in this file).
// Deliberately kept as one small, easy-to-edit struct so region/language/OS
// can be changed without hunting through binding code.
enum class OsId {
    WINDOWS = 0,
    OSX = 1, // Unused/unreleased
    PS3 = 2,
    PS4 = 3, // Is this used?
    NGP = 4, // PS Vita -- "NGP" is the pre-launch codename
    VITATV = 5, // Is this used?
};

struct SystemConfig {
    // PS3. This is a reimplementation of the PS3 build (title NPUA80543) --
    // there is no other platform to be. It is also load-bearing, not
    // cosmetic: the corpus branches on SYSTEM_GetOSID() to pick which
    // platform's scripts to run, and the default of WIN32 silently selected
    // the WRONG UI everywhere it is checked.
    //
    // Platformer/MainMenuScriptLoader.gm is the clearest case --
    //     local os = SYSTEM_GetOSID();
    //     if (os == OSID_PS3) { ...load Platformer/MainMenu5PS3/*... }
    // -- so with WIN32 the engine loaded the base/NGP (PS Vita) menu
    // instead. Those two trees are driven completely differently: the PS3
    // screens are controller-driven (HomeScreenPS3.gm binds
    // `onKeyPress = function(kc) { if (this.selected && kc ==
    // JOYSTICK0_ACCEPT) ... }` on each button), while the NGP ones are
    // touch/pointer-driven (HomeScreenBase.gm's panels use onTouchDown/
    // onTouchUp). The result was a main menu that rendered correctly but
    // could not be navigated by a controller at all, because the screens
    // being run were waiting for touches that a PS3 build never produces.
    //
    // retail rather than assumed from enum order).
    OsId osId = OsId::PS3;

    // Matches the real ELanguageCode_* enum confirmed in game_data/FontHelper.gm
    // (JAPANESE=0 .. SPANISH_MEXICAN=19). Default: English (US).
    int languageCode = 1;

    bool isBetaBuild = false;    // SYSTEM_IsBeta always returns false in the confirmed decompiled body
    bool isRetailBuild = true;
    // Development convenience: report the fifteen known DLC pack IDs as
    // available to scripts. This does not add content or emulate Sony DRM.
    bool debugAllDlcAvailable = false;

    // Desktop presentation option. When enabled, preserve the configured
    // game aspect ratio and draw black bars in the unused window area.
    // Disable to use the full host window, matching the pre-letterbox port.
    bool letterbox = true;

    // Reads optional settings from OpenEgg.ini beside the executable.
    // Missing or invalid values leave the retail-compatible defaults above.
    bool LoadFromIni(const std::string& path);

    const char* OsIdName() const;

    static SystemConfig& Get() {
        static SystemConfig instance;
        return instance;
    }
};
