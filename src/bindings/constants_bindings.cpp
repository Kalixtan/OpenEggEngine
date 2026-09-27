#include "constants_bindings.h"
#include <cmath>
#include <string>

void RegisterConstantsBindings(gmMachine& a_machine) {
    gmTableObject* g = a_machine.GetGlobals();

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

    // *_COLLISION_CATEGORY -- never assigned by any `global X=...` anywhere
    // in game_data (checked), used purely as bitmask arguments to
    // PhysicsBody.setCollisionCategory/setCategoriesToCollideWith (e.g.
    // Platformer/Player.gm: `pb.setCategoriesToCollideWith(ALL_COLLISION_CATEGORIES^PLAYER_COLLISION_CATEGORY)`)
    // -- must be native engine bootstrap globals, exactly like
    // MIXER_AUTO_ID_START (entity_bindings.cpp). No decompiled evidence for
    // the exact bit assignment, but the XOR/bitwise usage confirms a
    // distinct-single-bit-per-category scheme matching Box2D's uint16
    // b2Filter.categoryBits exactly -- assigned in the order first seen
    // across the corpus grep, arbitrary but self-consistent since nothing
    // ever compares these values against a literal.
    g->Set(&a_machine, "PLAYER_COLLISION_CATEGORY", gmVariable((gmint)(1 << 0)));
    g->Set(&a_machine, "GROUND_COLLISION_CATEGORY", gmVariable((gmint)(1 << 1)));
    g->Set(&a_machine, "STRUCTURE_COLLISION_CATEGORY", gmVariable((gmint)(1 << 2)));
    g->Set(&a_machine, "SCREENWALL_COLLISION_CATEGORY", gmVariable((gmint)(1 << 3)));
    g->Set(&a_machine, "CREATURE_COLLISION_CATEGORY", gmVariable((gmint)(1 << 4)));
    g->Set(&a_machine, "BOULDER_COLLISION_CATEGORY", gmVariable((gmint)(1 << 5)));
    g->Set(&a_machine, "BULLET_COLLISION_CATEGORY", gmVariable((gmint)(1 << 6)));
    g->Set(&a_machine, "DANGER_COLLISION_CATEGORY", gmVariable((gmint)(1 << 7)));
    g->Set(&a_machine, "INTERACTIVE_COLLISION_CATEGORY", gmVariable((gmint)(1 << 8)));
    g->Set(&a_machine, "LINE_COLLISION_CATEGORY", gmVariable((gmint)(1 << 9)));
    g->Set(&a_machine, "MOVING_ENTITY_COLLISION_CATEGORY", gmVariable((gmint)(1 << 10)));
    g->Set(&a_machine, "NOTE_COLLISION_CATEGORY", gmVariable((gmint)(1 << 11)));
    g->Set(&a_machine, "PLAYERSENSOR_COLLISION_CATEGORY", gmVariable((gmint)(1 << 12)));
    g->Set(&a_machine, "VISUAL_COLLISION_CATEGORY", gmVariable((gmint)(1 << 13)));
    g->Set(&a_machine, "ALL_COLLISION_CATEGORIES", gmVariable((gmint)0xFFFF));

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

    // *_Z / ZINCER -- confirmed real (18 distinct draw-depth constants,
    // e.g. Platformer/EntityPacks/SpawnPoint.gm:
    // `this.drawZ = PLAYER_Z-ZINCER;`, PauseProcessBase.gm:
    // `drawList.translate(this.cx,this.cy,-5*ZINCER);`) but never assigned
    // anywhere -- native bootstrap globals, same situation as the
    // *_COLLISION_CATEGORY constants. No decompiled evidence for the real
    // depth values; assigned in a plausible back-to-front draw order
    // (background < ground < entities < overlays < player < UI) with
    // even spacing, and ZINCER as a small per-step nudge (real usage
    // multiplies it by small integers, e.g. `-5*ZINCER`, to separate
    // near-coplanar elements) -- nothing compares these to a literal
    // value, only ever adds/subtracts/orders them against each other.
    g->Set(&a_machine, "ZINCER", gmVariable((gmfloat)0.01f));
    // BACK_CONVERSION -- confirmed real, same "native bootstrap global,
    // never assigned in any script" situation as ZINCER/the *_Z constants
    // above (e.g. GroundProcessNEW.gm: `this.CLEAR_Z=this.OUTLINE_Z+60*
    // BACK_CONVERSION;`). No decompiled evidence for the real value --
    // given every real use multiplies it by a small integer (8/20/30/60)
    // to nudge sub-layers within one already-small Z band apart from each
    // other (never compared to a literal), reusing ZINCER's value is a
    // reasonable placeholder of the same "small nudge unit" kind.
    g->Set(&a_machine, "BACK_CONVERSION", gmVariable((gmfloat)0.01f));
    g->Set(&a_machine, "BG_Z", gmVariable((gmfloat)0.0f));
    g->Set(&a_machine, "VIBE_Z", gmVariable((gmfloat)10.0f));
    g->Set(&a_machine, "BGENTITY_Z", gmVariable((gmfloat)20.0f));
    g->Set(&a_machine, "FIREGROUND_Z", gmVariable((gmfloat)28.0f));
    g->Set(&a_machine, "GROUND_Z", gmVariable((gmfloat)30.0f));
    g->Set(&a_machine, "CRAIG_DRONES_Z", gmVariable((gmfloat)35.0f));
    g->Set(&a_machine, "ENTITY_Z", gmVariable((gmfloat)40.0f));
    g->Set(&a_machine, "NOCLIMB_CLEAR_Z", gmVariable((gmfloat)48.0f));
    g->Set(&a_machine, "FIRE_CLEAR_Z", gmVariable((gmfloat)49.0f));

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
    g->Set(&a_machine, "NORMAL_GROUNDSTYLEID", gmVariable((gmint)0));
    g->Set(&a_machine, "FIRE_GROUNDSTYLEID", gmVariable((gmint)1));
    g->Set(&a_machine, "DANGER_GROUNDSTYLEID", gmVariable((gmint)2));
    g->Set(&a_machine, "NOCLIMB_GROUNDSTYLEID", gmVariable((gmint)3));
    g->Set(&a_machine, "ICE_GROUNDSTYLEID", gmVariable((gmint)4));
    g->Set(&a_machine, "MUD_GROUNDSTYLEID", gmVariable((gmint)5));
    g->Set(&a_machine, "BOUNCY_GROUNDSTYLEID", gmVariable((gmint)6));

    // Note-name constants (C0..B8) -- confirmed real (never assigned
    // anywhere in game_data, only used as bare identifiers passed straight
    // through to Audio.playSound's own confirmed "MIDI-style base note"
    // int argument, e.g. PauseProcessBase.gm:96's
    // `Audio.playSound(...,C4,0,0,0.10);`) -- standard scientific-pitch-
    // notation -> MIDI note number mapping (C4=60, middle C), the
    // universal convention this kind of name obviously follows. Missing
    // entirely before this was a genuine crash source, not just a wrong
    // pitch: with strict param macros (gmThread.h's GM_DEBUG build),
    // GM_INT_PARAM fails hard (returns GM_EXCEPTION) when the argument
    // isn't actually an int/float at all, and an unregistered global
    // evaluates to null -- so every real Audio.playSound(...,C4,...) call
    // site was throwing, not just mis-playing a note.
    {
        static const char* kNoteNames[12] = {"C", "CS", "D", "DS", "E", "F", "FS", "G", "GS", "A", "AS", "B"};
        for (int octave = 0; octave <= 8; ++octave) {
            for (int semitone = 0; semitone < 12; ++semitone) {
                int midiNote = (octave + 1) * 12 + semitone;
                std::string name = std::string(kNoteNames[semitone]) + std::to_string(octave);
                g->Set(&a_machine, name.c_str(), gmVariable((gmint)midiNote));
            }
        }
    }
    g->Set(&a_machine, "CLEAR_Z", gmVariable((gmfloat)50.0f));
    g->Set(&a_machine, "PLAYER_Z", gmVariable((gmfloat)60.0f));
    g->Set(&a_machine, "PLAYER_MODE_NOTE_Z", gmVariable((gmfloat)61.0f));
    g->Set(&a_machine, "WIZARD_MODE_NOTE_Z", gmVariable((gmfloat)62.0f));
    g->Set(&a_machine, "NOTE_GOT_Z", gmVariable((gmfloat)63.0f));
    g->Set(&a_machine, "OUTLINE_Z", gmVariable((gmfloat)70.0f));
    g->Set(&a_machine, "FRONT_Z", gmVariable((gmfloat)80.0f));
    g->Set(&a_machine, "UI_Z", gmVariable((gmfloat)90.0f));
    g->Set(&a_machine, "KEYBOARD_Z", gmVariable((gmfloat)100.0f));

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
}
