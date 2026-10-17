#include "constants_bindings.h"
#include "gmThread.h"
#include <cmath>
#include <cctype>
#include <string>

namespace {

int GM_CDECL gmf_NOTES_GetNoteNumber(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(name, 0, "");
    if (!name || !name[0]) return GM_EXCEPTION;

    const char letter = (char)std::toupper((unsigned char)name[0]);
    int semitone;
    switch (letter) {
        case 'C': semitone = 0; break;
        case 'D': semitone = 2; break;
        case 'E': semitone = 4; break;
        case 'F': semitone = 5; break;
        case 'G': semitone = 7; break;
        case 'A': semitone = 9; break;
        case 'B': semitone = 11; break;
        default: return GM_EXCEPTION;
    }
    size_t cursor = 1;
    if (name[cursor] == 's' || name[cursor] == 'S' || name[cursor] == '#') {
        ++semitone;
        ++cursor;
    } else if (name[cursor] == 'b' || name[cursor] == 'B') {
        --semitone;
        ++cursor;
    }
    if (!std::isdigit((unsigned char)name[cursor]) || name[cursor + 1] != '\0')
        return GM_EXCEPTION;
    const int octave = name[cursor] - '0';
    if (octave < 0 || octave > 8 || (octave == 8 && semitone != 0))
        return GM_EXCEPTION;
    if (semitone < 0) { semitone += 12; if (octave == 0) return GM_EXCEPTION; }
    if (semitone >= 12) { semitone -= 12; if (octave == 8) return GM_EXCEPTION; }
    a_thread->PushInt(octave * 12 + semitone);
    return GM_OK;
}

} // namespace

void RegisterConstantsBindings(gmMachine& a_machine) {
    gmTableObject* g = a_machine.GetGlobals();
    a_machine.RegisterLibraryFunction("NOTES_GetNoteNumber", gmf_NOTES_GetNoteNumber);

    g->Set(&a_machine, "STEP_OFF", gmVariable((gmint)0));
    g->Set(&a_machine, "STEP_ON", gmVariable((gmint)1));
    g->Set(&a_machine, "STEP_SUSTAIN", gmVariable((gmint)2));

    g->Set(&a_machine, "SOUNDEFFECTTYPE_LOWPASS", gmVariable((gmint)3));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_LOWPASSSIMPLE", gmVariable((gmint)20));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_LOWPASSIT", gmVariable((gmint)4));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_HIGHPASS", gmVariable((gmint)5));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_ECHO", gmVariable((gmint)6));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_ECHOIT", gmVariable((gmint)17));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_FLANGE", gmVariable((gmint)7));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_DISTORTION", gmVariable((gmint)8));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_NORMALIZE", gmVariable((gmint)9));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_PITCHSHIFT", gmVariable((gmint)12));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_CHORUS", gmVariable((gmint)13));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_SFXREVERB", gmVariable((gmint)19));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_COMPRESSOR", gmVariable((gmint)18));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_DELAY", gmVariable((gmint)21));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_TREMOLO", gmVariable((gmint)22));
    g->Set(&a_machine, "SOUNDEFFECTTYPE_TIMESTRETCH", gmVariable((gmint)0x10000));

    g->Set(&a_machine, "FILTER_CUTOFF", gmVariable((gmint)0));
    g->Set(&a_machine, "FILTER_RES", gmVariable((gmint)1));

    g->Set(&a_machine, "SW", gmVariable(0.61f));

    g->Set(&a_machine, "VERSION_MAJOR", gmVariable((gmint)1));
    g->Set(&a_machine, "VERSION_MINOR", gmVariable((gmint)0));
    g->Set(&a_machine, "VERSION_TIMESTAMP", gmVariable(a_machine.AllocStringObject("")));

    g->Set(&a_machine, "BASE_MS_PER_STEP", gmVariable(125.0f));

    g->Set(&a_machine, "PX2SW", gmVariable(0.61f / 15.0f));

    g->Set(&a_machine, "PI", gmVariable((gmfloat)M_PI));
    g->Set(&a_machine, "PIBYTWO", gmVariable((gmfloat)(M_PI / 2.0)));
    g->Set(&a_machine, "TWOPI", gmVariable((gmfloat)(M_PI * 2.0)));

    g->Set(&a_machine, "TEXTALIGN_LEFT", gmVariable((gmint)0));
    g->Set(&a_machine, "TEXTALIGN_CENTER", gmVariable((gmint)2));
    g->Set(&a_machine, "TEXTALIGN_RIGHT", gmVariable((gmint)1));

    g->Set(&a_machine, "FILEHELPER_AREA_GAMEDATA", gmVariable((gmint)1));
    g->Set(&a_machine, "FILEHELPER_AREA_SAVEDATA", gmVariable((gmint)0));
    g->Set(&a_machine, "FILEHELPER_AREA_HOST", gmVariable((gmint)2));
    g->Set(&a_machine, "FILEHELPER_AREA_NULL", gmVariable((gmint)3));

    g->Set(&a_machine, "GROUND_COLLISION_CATEGORY", gmVariable((gmint)1));
    g->Set(&a_machine, "CREATURE_COLLISION_CATEGORY", gmVariable((gmint)2));
    g->Set(&a_machine, "PLAYER_COLLISION_CATEGORY", gmVariable((gmint)4));
    g->Set(&a_machine, "DANGER_COLLISION_CATEGORY", gmVariable((gmint)8));
    g->Set(&a_machine, "INTERACTIVE_COLLISION_CATEGORY", gmVariable((gmint)16));
    g->Set(&a_machine, "VISUAL_COLLISION_CATEGORY", gmVariable((gmint)32));
    g->Set(&a_machine, "BOULDER_COLLISION_CATEGORY", gmVariable((gmint)64));
    g->Set(&a_machine, "NOTE_COLLISION_CATEGORY", gmVariable((gmint)128));
    g->Set(&a_machine, "STRUCTURE_COLLISION_CATEGORY", gmVariable((gmint)256));
    g->Set(&a_machine, "PLAYERSENSOR_COLLISION_CATEGORY", gmVariable((gmint)512));
    g->Set(&a_machine, "SCREENWALL_COLLISION_CATEGORY", gmVariable((gmint)1024));
    g->Set(&a_machine, "LINE_COLLISION_CATEGORY", gmVariable((gmint)2048));
    g->Set(&a_machine, "BULLET_COLLISION_CATEGORY", gmVariable((gmint)4096));
    g->Set(&a_machine, "PLAYERPLUSPLAYER_COLLISION_CATEGORY", gmVariable((gmint)8192));
    // No retail registration recovered for this script-only legacy name.
    g->Set(&a_machine, "MOVING_ENTITY_COLLISION_CATEGORY", gmVariable((gmint)(1 << 14)));
    g->Set(&a_machine, "ALL_COLLISION_CATEGORIES", gmVariable((gmint)0xFFFF));

    g->Set(&a_machine, "PHYSICSJOINTTYPE_REVOLUTE", gmVariable((gmint)1));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_PRISMATIC", gmVariable((gmint)2));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_DISTANCE", gmVariable((gmint)3));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_PIN", gmVariable((gmint)9));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_RELATIVE", gmVariable((gmint)10));

    g->Set(&a_machine, "PHYSICSTYPE_MULTIBODY", gmVariable((gmint)1));

    g->Set(&a_machine, "BASE_GROUND_FRICTION", gmVariable((gmfloat)0.3f));

    g->Set(&a_machine, "GROUND_ENTITY_DESCRIPTION", gmVariable((gmint)(1 << 0)));
    g->Set(&a_machine, "SELECTED_ENTITY_DESCRIPTION", gmVariable((gmint)(1 << 1)));

    g->Set(&a_machine, "ZINCER", gmVariable((gmfloat)0.03f)); // 0x3cf5c28f, was guessed 0.01
    g->Set(&a_machine, "BACK_CONVERSION", gmVariable((gmfloat)0.01f));
    g->Set(&a_machine, "BG_Z", gmVariable((gmfloat)-50.0f));
    g->Set(&a_machine, "VIBE_Z", gmVariable((gmfloat)-60.0f));
    g->Set(&a_machine, "BGENTITY_Z", gmVariable((gmfloat)-30.0f));
    g->Set(&a_machine, "FIREGROUND_Z", gmVariable((gmfloat)-13.0f));
    g->Set(&a_machine, "GROUND_Z", gmVariable((gmfloat)-10.0f));
    g->Set(&a_machine, "ENTITY_Z", gmVariable((gmfloat)0.0f));
    // Present in the retail bootstrap but missing from our set entirely:
    g->Set(&a_machine, "NOTE_KICKER_Z", gmVariable((gmfloat)-16.0f));
    g->Set(&a_machine, "BOIDS_BACK_Z", gmVariable((gmfloat)-10.0f));
    g->Set(&a_machine, "BOIDS_FRONT_Z", gmVariable((gmfloat)-10.0f));
    g->Set(&a_machine, "GROUND_OUT_FILL_Z", gmVariable((gmfloat)-2.0f));
    g->Set(&a_machine, "UI_Z", gmVariable((gmfloat)30.0f));
    // GROUND_HIGHLIGHT_STENCIL_VALUE registers in the same bootstrap run
    // but as the INTEGER 0x64 (100), not a float -- it is a stencil ref
    // value, not a depth.
    g->Set(&a_machine, "GROUND_HIGHLIGHT_STENCIL_VALUE", gmVariable((gmint)100));
    // CRAIG_DRONES_Z / NOCLIMB_CLEAR_Z / FIRE_CLEAR_Z are NOT in retail's
    // bootstrap string table (0x39838e..0x39847c) -- kept only because
    // scripts here reference them; values remain unverified guesses.
    g->Set(&a_machine, "CRAIG_DRONES_Z", gmVariable((gmfloat)-11.0f));
    g->Set(&a_machine, "NOCLIMB_CLEAR_Z", gmVariable((gmfloat)-3.0f));
    g->Set(&a_machine, "FIRE_CLEAR_Z", gmVariable((gmfloat)-2.5f));

    g->Set(&a_machine, "NORMAL_GROUNDSTYLEID", gmVariable((gmint)0));
    g->Set(&a_machine, "NOCLIMB_GROUNDSTYLEID", gmVariable((gmint)1));
    g->Set(&a_machine, "FIRE_GROUNDSTYLEID", gmVariable((gmint)2));
    g->Set(&a_machine, "DANGER_GROUNDSTYLEID", gmVariable((gmint)3));
    g->Set(&a_machine, "ICE_GROUNDSTYLEID", gmVariable((gmint)4));
    g->Set(&a_machine, "MUD_GROUNDSTYLEID", gmVariable((gmint)5));
    g->Set(&a_machine, "BOUNCY_GROUNDSTYLEID", gmVariable((gmint)6));

    {
        static const char* kNoteNames[12] = {"C", "Cs", "D", "Ds", "E", "F", "Fs", "G", "Gs", "A", "As", "B"};
        for (int octave = 0; octave <= 8; ++octave) {
            for (int semitone = 0; semitone < 12; ++semitone) {
                if (octave == 8 && semitone != 0) break;
                int retailNote = octave * 12 + semitone;
                std::string name = std::string(kNoteNames[semitone]) + std::to_string(octave);
                g->Set(&a_machine, name.c_str(), gmVariable((gmint)retailNote));
            }
        }
    }
    g->Set(&a_machine, "BASE_NOTE", gmVariable((gmint)96));
    // Real values from the retail bootstrap at 0x19b1c0..0x19b4f0 -- see
    // the long comment on the *_Z block above for how they were read.
    // NOTE: this block runs AFTER that one and previously re-set UI_Z to a
    // different (guessed) value, silently overriding it.
    g->Set(&a_machine, "PLAYER_Z", gmVariable((gmfloat)-17.8f));        // 0xc18e6666
    g->Set(&a_machine, "PLAYER_MODE_NOTE_Z", gmVariable((gmfloat)-20.0f));
    g->Set(&a_machine, "WIZARD_MODE_NOTE_Z", gmVariable((gmfloat)-7.0f));
    g->Set(&a_machine, "NOTE_GOT_Z", gmVariable((gmfloat)-40.0f));
    g->Set(&a_machine, "FRONT_Z", gmVariable((gmfloat)20.0f));
    g->Set(&a_machine, "UI_Z", gmVariable((gmfloat)30.0f));
    // CLEAR_Z / OUTLINE_Z / KEYBOARD_Z are NOT in retail's bootstrap
    // string table (0x39838e..0x39847c), so these remain unverified.
    // GroundProcessNEW.gm computes its own this.CLEAR_Z/this.OUTLINE_Z
    // from OUTLINE_Z + n*BACK_CONVERSION, so the globals may not be real
    // at all. Placed just in front of GROUND_Z (-10) to keep the ground's
    // own sub-layers ordered the way that script expects.
    g->Set(&a_machine, "OUTLINE_Z", gmVariable((gmfloat)-9.0f));
    g->Set(&a_machine, "CLEAR_Z", gmVariable((gmfloat)-8.0f));
    g->Set(&a_machine, "KEYBOARD_Z", gmVariable((gmfloat)40.0f));

    g->Set(&a_machine, "VIDEOPARAMS_NEVER", gmVariable((gmint)0));
    g->Set(&a_machine, "VIDEOPARAMS_LESS", gmVariable((gmint)1));
    g->Set(&a_machine, "VIDEOPARAMS_EQUAL", gmVariable((gmint)2));
    g->Set(&a_machine, "VIDEOPARAMS_LEQUAL", gmVariable((gmint)3));
    g->Set(&a_machine, "VIDEOPARAMS_GREATER", gmVariable((gmint)4));
    g->Set(&a_machine, "VIDEOPARAMS_NOTEQUAL", gmVariable((gmint)5));
    g->Set(&a_machine, "VIDEOPARAMS_GEQUAL", gmVariable((gmint)6));
    g->Set(&a_machine, "VIDEOPARAMS_ALWAYS", gmVariable((gmint)7));
    g->Set(&a_machine, "VIDEOPARAMS_DISABLE", gmVariable((gmint)8));

    g->Set(&a_machine, "VIDEOPARAMS_KEEP", gmVariable((gmint)100));
    g->Set(&a_machine, "VIDEOPARAMS_REPLACE", gmVariable((gmint)101));
    g->Set(&a_machine, "VIDEOPARAMS_INCR", gmVariable((gmint)102));
    g->Set(&a_machine, "VIDEOPARAMS_DECR", gmVariable((gmint)103));
    g->Set(&a_machine, "VIDEOPARAMS_INVERT", gmVariable((gmint)104));
    g->Set(&a_machine, "VIDEOPARAMS_INCR_WRAP", gmVariable((gmint)105));
    g->Set(&a_machine, "VIDEOPARAMS_DECR_WRAP", gmVariable((gmint)106));
    g->Set(&a_machine, "VIDEOPARAMS_ZERO", gmVariable((gmint)107));

    // Retail drawList.clear(mode) consumes four independent mode bits in
    // GE3_Internal_DrawListReplayCommands: bit 0 selects colour writes, bit 1
    // selects depth state, bit 2 selects stencil state, and bit 3 selects
    // the alternate colour-surface render config. The retail global bootstrap
    // requires these names, while the desktop previously left them undefined;
    // shipped MainMenu5 scripts pass VIDEO_ALLSURFACES to clear().
    g->Set(&a_machine, "VIDEO_COLOURSURFACE", gmVariable((gmint)0x1));
    g->Set(&a_machine, "VIDEO_DEPTHSURFACE", gmVariable((gmint)0x2));
    g->Set(&a_machine, "VIDEO_STENCILSURFACE", gmVariable((gmint)0x4));
    g->Set(&a_machine, "VIDEO_COLOURSURFACEBLEND", gmVariable((gmint)0x8));
    g->Set(&a_machine, "VIDEO_ALLSURFACES", gmVariable((gmint)0xf));
}
