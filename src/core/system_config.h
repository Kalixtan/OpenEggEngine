#pragma once

// back to scripts.
//
// Originally defaulted to pretending to be a retail PS3 build (Sound
// Shapes' own game_data was extracted from that build). This project also
// now runs Flowers of Error, whose retail binary IS a real PC (WIN32) exe
// (`flowers of error.exe`) -- confirmed via Ghidra (reveng_pc/) against
// that actual binary, not inferred -- so osId defaults to WIN32 now. No
// FoE script (game_data_min/) reads System.getOS()/OSID_* directly, but
// getting this right matters for anything that keys off it going forward
// (e.g. keyboard input semantics -- FoE's Player.gm uses
// Input.isKeyPressed(KEYBOARD_*) extensively, which only makes sense on a
// PC target, not the PS3-no-keyboard assumption elsewhere in this file).
// Deliberately kept as one small, easy-to-edit struct so region/language/OS
// can be changed without hunting through binding code.
enum class OsId {
    WIN32 = 0,
    OSX = 1, // Unused/unreleased
    PS3 = 2,
    PS4 = 3, // Is this used?
    NGP = 4, // PS Vita -- "NGP" is the pre-launch codename
    VITATV = 5, // Is this used?
};

struct SystemConfig {
    OsId osId = OsId::WIN32;

    // Matches the real ELanguageCode_* enum confirmed in game_data/FontHelper.gm
    // (JAPANESE=0 .. SPANISH_MEXICAN=19). Default: English (US).
    int languageCode = 1;

    bool isBetaBuild = false;    // SYSTEM_IsBeta always returns false in the confirmed decompiled body
    bool isRetailBuild = true;

    static SystemConfig& Get() {
        static SystemConfig instance;
        return instance;
    }
};
