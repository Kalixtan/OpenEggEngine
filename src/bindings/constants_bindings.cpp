#include "constants_bindings.h"
#include "gmThread.h"
#include <cmath>
#include <cctype>
#include <string>

namespace {

// Retail NOTES_GetNoteNumber delegates entirely to FUN_000b43c0.  The
// shipped instrument names are pitch strings such as C4, Cs4 and As6; the
// resulting numbering is C0=0 through C8=96, with C4=48.
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

    // Native sequencer states used by EntityBase's custom pattern table.
    // Retail registers these exact values at 0x00063ef0, 0x00063f10,
    // and 0x00063f34. SEQUENCE_ToString's alphabet is "012" at 0x003726f4.
    g->Set(&a_machine, "STEP_OFF", gmVariable((gmint)0));
    g->Set(&a_machine, "STEP_ON", gmVariable((gmint)1));
    g->Set(&a_machine, "STEP_SUSTAIN", gmVariable((gmint)2));

    // FMOD DSP type IDs consumed by Audio.createEffectChain/addEffect.
    // FUN_0005f704 confirms these names are expected as globals, while the
    // effect dispatch in Audio.c switches on the corresponding FMOD type
    // values (3..22); the custom Dirac time-stretch route is explicitly
    // selected by type 0x10000 in the retail parameter dispatcher.
    // Values follow the FMOD DSP type enum used by the retail integration.
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

    // FMOD low-pass parameter slots referenced by the chain setup in Main.gm.
    // FUN_0001e770 forwards the parameter ID unchanged to the DSP backend;
    // FMOD's low-pass enum assigns cutoff=0 and resonance=1.
    g->Set(&a_machine, "FILTER_CUTOFF", gmVariable((gmint)0));
    g->Set(&a_machine, "FILTER_RES", gmVariable((gmint)1));

    // Confirmed live on real RPCS3/retail via game_data_testcart's TEST 101.
    g->Set(&a_machine, "SW", gmVariable(0.61f));

    // VERSION_MAJOR/VERSION_MINOR/VERSION_TIMESTAMP -- confirmed real (the
    // only live call site, CreditsScreenBase.gm:710's version string) but
    // these are native ENGINE/binary build-stamp constants, distinct from
    // the GM content bundle's own SCRIPT_VERSION_* (game_data/autogen/
    // version.gm) -- an embedded data literal baked into the real EBOOT.elf
    // at build time, not something Ghidra's decompilation recovers (it
    // reads code, not this kind of constant), so the exact original numbers
    // aren't recoverable from this corpus. Left undefined, the version
    // string's %3/%4/%5 substitution args ({SCRIPT_VERSION_MAJOR,
    // SCRIPT_VERSION_MINOR, VERSION_MAJOR, VERSION_MINOR, VERSION_TIMESTAMP,
    // SCRIPT_VERSION_NAME}) hit a null (draw_context.cpp's
    // SubstituteFormatArgs gathering loop stops at the first GM_NULL entry
    // it finds), truncating the whole "Code: %3.%4 / %5" tail of the
    // credits/settings version line -- registered here so the line
    // completes; the values themselves are placeholders, not confirmed.
    g->Set(&a_machine, "VERSION_MAJOR", gmVariable((gmint)1));
    g->Set(&a_machine, "VERSION_MINOR", gmVariable((gmint)0));
    g->Set(&a_machine, "VERSION_TIMESTAMP", gmVariable(a_machine.AllocStringObject("")));

    // BASE_MS_PER_STEP -- confirmed real by widespread usage (22 real .gm
    // files, e.g. EntityPacks/SwayingTree.gm:70's
    // `this.headAni.setPlayTime(BASE_MS_PER_STEP * 4)`) but never assigned
    // anywhere in the corpus (same "native bootstrap global" pattern as SW
    // above). Was previously undefined entirely, so every one of those
    // expressions evaluated to `null * N`, which itself evaluates quietly
    // to null rather than throwing -- the actual crash lands one level up,
    // in whatever native call receives that null (setPlayTime's strict
    // GM_FLOAT_OR_INT_PARAM), with a blank exception message that's easy
    // to miss and that kills the calling script thread (here, an entity's
    // constructor/draw, breaking that entity and anything drawn after it
    // in the same flat draw list -- SwayingTree specifically was blanking
    // out entire tutorial-level frames). Value derived from this engine's
    // own SequencerData default (sequencer_bindings.cpp: 120 BPM, 4 steps
    // per beat -- `60000/120/4`), the same baseline tempo used before any
    // level's own track data overrides it, matching every real call site's
    // use of this as a "reference"/non-tempo-scaled duration unit.
    g->Set(&a_machine, "BASE_MS_PER_STEP", gmVariable(125.0f));

    // PX2SW ("pixels to SW units") -- confirmed real (ENTITYBASE_InstallAnimation's
    // `width=PX2SW*MATH_REAL(anim.getSourcePixelWidth());`, every .fsa/.bci
    // animated entity's on-screen size) but never assigned anywhere in the
    // corpus (same "native bootstrap global" pattern as SW itself -- no
    // game_data_testcart TEST case pins this one down, so no live-confirmed
    // exact value exists). Derived from the SAME reference-resolution
    // convention EntityPacks/TutorialGraphicsEntity.gm already uses for
    // its own pixel-to-world sizing (`screenPixelWidthReference=960; ...
    // this.width=(tli.width/960)*this.gameProcess.getScreenWidth();`) --
    // i.e. "960 real pixels spans one full screen width" -- rather than an
    // arbitrary guess: PX2SW = getScreenWidth()/960 = (SW*2*2*16)/960 = SW/15.
    g->Set(&a_machine, "PX2SW", gmVariable(0.61f / 15.0f));

    // Never assigned by any `global X=...` anywhere in game_data (checked),
    // used purely as bare math constants (294/242 real call sites) --
    // native globals, high-confidence standard values. Exact digits
    // confirmed live via game_data_testcart's TEST 102/103.
    g->Set(&a_machine, "PI", gmVariable((gmfloat)M_PI));
    g->Set(&a_machine, "PIBYTWO", gmVariable((gmfloat)(M_PI / 2.0)));
    g->Set(&a_machine, "TWOPI", gmVariable((gmfloat)(M_PI * 2.0)));

    // Confirmed live via game_data_testcart's TEST 104/105/106 -- NOT the
    // intuitive 0/1/2 order. Must match render/bitmap_font.cpp's
    // kAlignLeft/Center/Right.
    g->Set(&a_machine, "TEXTALIGN_LEFT", gmVariable((gmint)0));
    g->Set(&a_machine, "TEXTALIGN_CENTER", gmVariable((gmint)2));
    g->Set(&a_machine, "TEXTALIGN_RIGHT", gmVariable((gmint)1));

    // GAMEDATA=1 confirmed live via game_data_testcart's TEST 107.
    // SAVEDATA/HOST/NULL values are NOT confirmed -- still guesses.
    g->Set(&a_machine, "FILEHELPER_AREA_GAMEDATA", gmVariable((gmint)1));
    g->Set(&a_machine, "FILEHELPER_AREA_SAVEDATA", gmVariable((gmint)0));
    g->Set(&a_machine, "FILEHELPER_AREA_HOST", gmVariable((gmint)2));
    g->Set(&a_machine, "FILEHELPER_AREA_NULL", gmVariable((gmint)3));

    // Recovered from the retail PS3 native global registrar (see
    // native raycasts and rendering, so merely being self-consistent in GM
    // is insufficient.
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

    // Physics joint enum values are consumed directly by
    // Physics.createJoint(type). The retail constructor dispatches types
    // 1/2/3 to revolute/prismatic/distance configs (FUN_00034460), and its
    // type-9 pin and type-10 relative setters validate those exact IDs
    // (FUN_0003517c/FUN_00034e00 and FUN_0003507c). The names are present
    // in the retail global-constant registration function FUN_0005f704.
    // Only values independently confirmed by the native type checks are
    // installed here; the other retail joint-name constants (pulley,
    // mouse, gear, line) still need their numeric values established.
    g->Set(&a_machine, "PHYSICSJOINTTYPE_REVOLUTE", gmVariable((gmint)1));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_PRISMATIC", gmVariable((gmint)2));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_DISTANCE", gmVariable((gmint)3));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_PIN", gmVariable((gmint)9));
    g->Set(&a_machine, "PHYSICSJOINTTYPE_RELATIVE", gmVariable((gmint)10));

    // Entity.createEntity forwards this mode to its native entity factory
    // (retail FUN_000c3334 -> FUN_000c1710). The shipped level metadata uses
    // physicsType=1 for the multi-body entities, and retail registers the
    // PHYSICSTYPE_MULTIBODY global in FUN_0005f704.
    g->Set(&a_machine, "PHYSICSTYPE_MULTIBODY", gmVariable((gmint)1));

    // BASE_GROUND_FRICTION -- `global BASE_GROUND_FRICTION;` (forward
    // declared, never assigned) in Platformer/Player.gm:114, used as a
    // plain Box2D friction coefficient elsewhere (`sd.setFriction(BASE_GROUND_FRICTION)`,
    // e.g. EntityPacks/SingleCircleGround.gm) -- must be a native bootstrap
    // global like the *_COLLISION_CATEGORY constants above. No decompiled
    // value; 0.3 is a plausible mid-range Box2D friction default (Box2D's
    // own shape default is also 0.3) since nothing in the corpus compares
    // it against a specific literal.
    g->Set(&a_machine, "BASE_GROUND_FRICTION", gmVariable((gmfloat)0.3f));

    // GROUND_ENTITY_DESCRIPTION/SELECTED_ENTITY_DESCRIPTION -- same
    // never-assigned-anywhere native-bootstrap-bitmask situation as the
    // *_COLLISION_CATEGORY constants above, used against
    // Entity.getDescriptionBits()/setDescriptionBits() (a per-entity
    // bitmask -- see entity_bindings.cpp's InjectEntityHandleMethods)
    // via plain `&`/`|`/`^` bitwise ops (e.g. TouchEditProcessBaseOld.gm:
    // `e.setDescriptionBits(e.getDescriptionBits()|SELECTED_ENTITY_DESCRIPTION)`),
    // confirming a single-bit-per-flag scheme with no decompiled evidence
    // for the exact bit assignment.
    g->Set(&a_machine, "GROUND_ENTITY_DESCRIPTION", gmVariable((gmint)(1 << 0)));
    g->Set(&a_machine, "SELECTED_ENTITY_DESCRIPTION", gmVariable((gmint)(1 << 1)));

    // *_Z / ZINCER -- REAL VALUES, read out of the retail binary. These
    // were previously guessed ("plausible back-to-front order with even
    // spacing"), and every single one of them was wrong -- not just in
    // magnitude but in SIGN and in ORDER, which put the player in front of
    // the ground instead of behind it.
    //
    // Recovered from the native bootstrap that registers them, at
    // 0x19b1c0..0x19b4f0. Each registration is the same shape: the name
    // address is built into r5 (`lis r5,0x3a` + `addic r5,r5,<imm>`), the
    // float's BITS are built into a GPR with `lis`(+`ori`) and stored to
    // 0x74(r1), then FUN_000f2670 is called. So the values are readable
    // directly, e.g. PLAYER_Z:
    //     lis r3, -0x3e72      ; r3 = 0xC18E0000
    //     ori r7, r3, 0x6666   ; r7 = 0xC18E6666  == -17.8f
    //     addic r5, r5, -0x7b9c; name = 0x398464 == "PLAYER_Z"
    //
    // The scheme is "larger = nearer the camera", and most layers are
    // NEGATIVE:
    //     VIBE_Z             -60      GROUND_Z           -10
    //     BG_Z               -50      WIZARD_MODE_NOTE_Z  -7
    //     NOTE_GOT_Z         -40      GROUND_OUT_FILL_Z   -2
    //     BGENTITY_Z         -30      ENTITY_Z             0
    //     PLAYER_MODE_NOTE_Z -20      FRONT_Z             20
    //     PLAYER_Z           -17.8    UI_Z                30
    //     NOTE_KICKER_Z      -16
    //     FIREGROUND_Z       -13
    //     BOIDS_BACK_Z       -10      BOIDS_FRONT_Z      -10
    //
    // The load-bearing one: PLAYER_Z (-17.8) is BEHIND GROUND_Z (-10), so
    // the ground and platforms occlude the player. The old guess had
    // PLAYER_Z 60 against GROUND_Z 30 -- the player drew over everything.
    g->Set(&a_machine, "ZINCER", gmVariable((gmfloat)0.03f)); // 0x3cf5c28f, was guessed 0.01
    // BACK_CONVERSION -- confirmed real, same "native bootstrap global,
    // never assigned in any script" situation as ZINCER/the *_Z constants
    // above (e.g. GroundProcessNEW.gm: `this.CLEAR_Z=this.OUTLINE_Z+60*
    // BACK_CONVERSION;`). No decompiled evidence for the real value --
    // given every real use multiplies it by a small integer (8/20/30/60)
    // to nudge sub-layers within one already-small Z band apart from each
    // other (never compared to a literal), reusing ZINCER's value is a
    // reasonable placeholder of the same "small nudge unit" kind.
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

    // *_GROUNDSTYLEID -- confirmed real (never assigned in game_data, only
    // compared against, e.g. GroundBase.gm's `this.configureData.style==
    // NORMAL_GROUNDSTYLEID`), but no game_data_testcart TEST case pins the
    // actual integer values down. Assigned sequentially in the same order
    // Platformer/GroundProcessNEW.gm's own (pre-native-port, left in the
    // file as a reference -- see engine/src/ported_native/ground_renderer.cpp)
    // `this.styleInstallers[...]=...` block registers them -- an inferred
    // guess, not confirmed, but self-consistent since nothing in the real
    // corpus ever compares these against a literal integer either.
    // campaignTutorial's own level XML uses style values 0/1/2 (127/19/37
    // ground pieces respectively) -- consistent with 0=NORMAL being the
    // overwhelming majority (plain platforms) and a smaller secondary
    // style being taught deliberately (this tutorial's own hint text,
    // confirmed via screenshot: "Red things = bad. Press X to JUMP over
    // them."), which fits 2=DANGER far better than 1=FIRE for a beginner
    // tutorial's most-common hazard.
    // ORDER CORRECTED: NOCLIMB is 1, FIRE 2, DANGER 3 -- previously FIRE=1,
    // DANGER=2, NOCLIMB=3, guessed from the order GroundProcessNEW.gm
    // happens to REGISTER its style installers in, which is not the same
    // thing as the ids' values.
    //
    // The real declaration order survives verbatim in game_data/Platformer/
    // GroundBase.gm, in a commented-out copy of retail's own C++ variable
    // registrations:
    //     DEFINE_GM_VARIABLE1(NORMAL_GROUNDSTYLEID);
    //     DEFINE_GM_VARIABLE1(NOCLIMB_GROUNDSTYLEID);
    //     DEFINE_GM_VARIABLE1(FIRE_GROUNDSTYLEID);
    //     DEFINE_GM_VARIABLE1(DANGER_GROUNDSTYLEID);
    //     DEFINE_GM_VARIABLE1(ICE_GROUNDSTYLEID);
    //     DEFINE_GM_VARIABLE1(MUD_GROUNDSTYLEID);
    //     DEFINE_GM_VARIABLE1(BOUNCY_GROUNDSTYLEID);
    //
    // Confirmed independently against real level data and a real capture:
    // every ground piece in the tutorial carries style 1, and
    // ground drawn in rgb(42,33,15) -- which is what installNoClimbGround
    // produces (its "ground_outline" has no colours.dat entry in this
    // scheme, so it resolves to the sampled (0.165,0.129,0.059) =
    // rgb(42,32,15)), not what installFireGround produces. With the old
    // numbering that ground was drawn as FIRE: a thin coral edge over a
    // fill the same colour as the background, i.e. the level rendered as
    // "just lines".
    g->Set(&a_machine, "NORMAL_GROUNDSTYLEID", gmVariable((gmint)0));
    g->Set(&a_machine, "NOCLIMB_GROUNDSTYLEID", gmVariable((gmint)1));
    g->Set(&a_machine, "FIRE_GROUNDSTYLEID", gmVariable((gmint)2));
    g->Set(&a_machine, "DANGER_GROUNDSTYLEID", gmVariable((gmint)3));
    g->Set(&a_machine, "ICE_GROUNDSTYLEID", gmVariable((gmint)4));
    g->Set(&a_machine, "MUD_GROUNDSTYLEID", gmVariable((gmint)5));
    g->Set(&a_machine, "BOUNCY_GROUNDSTYLEID", gmVariable((gmint)6));

    // Retail NOTES_GetNoteNumber (FUN_000b43c0) maps C0..C8 to 0..96,
    // with C4=48. Its literal note-name table starts at EBOOT.elf
    // 0x00374d30. These globals use the same pitch numbering; using MIDI's
    // C4=60 shifts every instrument sound by an octave.
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
    // The retail registrar at 0x00062490 stores integer 96 under
    // "BASE_NOTE" immediately after B7=95. Missing this global passed
    // GM_NULL to Audio.playSound during checkpoint contacts and aborted the
    // checkpoint's sound callback with a GameMonkey exception.
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

    // VIDEOPARAMS_* -- confirmed real (drawList.setAttrStencil/
    // Video.setStencilFunc/setStencilOp real call sites across the corpus,
    // e.g. GroundProcessNEW.gm's outline-then-clear-fill stencil trick,
    // PopupProcessBase.gm's modal-region clipping) but never assigned
    // anywhere -- native bootstrap globals, same situation as ZINCER/the
    // *_Z constants above. The comparison-function half (NEVER..ALWAYS) IS
    // decompiled-confirmed, though: Ghidra's DrawList.setAttrDepthTest
    // (0x0007e514) pushes literal 2 for true and 7 for false, which only
    // makes sense as GL_EQUAL(2)/GL_ALWAYS(7) in the standard
    // GL_NEVER..GL_ALWAYS 0-7 ordering -- reused verbatim here since
    // setAttrStencil's func argument is confirmed the same
    // VIDEOPARAMS_EQUAL/GREATER/ALWAYS/etc. names. DISABLE has no GL
    // equivalent (it's this engine's own "turn the stencil test off
    // entirely" sentinel, see gmf_setAttrStencil/gmf_setStencilFunc) so it
    // just needs to be distinct from 0-7, not match anything real.
    // The stencil-OP half (KEEP/REPLACE/INCR/...) has no natural small-
    // integer ordering to infer (GL's own GL_KEEP..GL_DECR_WRAP enum
    // values aren't sequential) and nothing in the corpus compares these
    // to a literal either -- arbitrary distinct values, offset well past
    // the func range so a mixed-up func/op argument fails loudly instead
    // of silently aliasing a real value.
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
