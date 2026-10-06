#include <cstdio>
#include <cstdlib>
#include <string>
#include <set>
#include "player_renderer.h"
#include "../core/engine_context.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/physics_body_bindings.h"
#include "../bindings/colour_bindings.h"
#include "../bindings/vector2d_bindings.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include <box2d/box2d.h>
#include <GL/glew.h>
#include <vector>
#include <cmath>
#include <algorithm>

// Ported from Platformer/PlayerOLD.gm's `testDraw(drawList)` -- see
// player_renderer.h for why that's the source instead of a Ghidra-
// recovered native body. Every constant/field name below (radius,
// maxSpeedX, squintPerc, stickGraphicPerc, lastJumpNormal, drawz) is
// confirmed still real on the CURRENT live Player.gm too (same formulas:
// `this.radius = (SW*2-8*BACK_CONVERSION)/2 - 8*BACK_CONVERSION`,
// `this.maxSpeedX = SW/(60.0-9.0)`, `this.drawz = PLAYER_Z` at
// construction) -- read directly off the live entity table below rather
// than reimplemented as a parallel state machine, so this stays correct
// even where retail's own logic changes those values conditionally
// (e.g. drawz switching to GROUND_Z+200*BACK_CONVERSION in some states).
//
// stickGraphicPerc/lastJumpNormal (wall/ceiling-stick squash) and
// squintPerc (blink/squint) are all read live off the entity table below,
// same as radius/maxSpeedX/drawz -- Player.gm's own movement/animation
// state machine (~5700 lines) computes and stores these on `this` every
// frame regardless of which code draws them, so reading them here gets
// the real, currently-live value without reimplementing whatever set
// them. Each still defaults to its neutral value (matching the
// reference's own `!?this.X` fallback) for the brief window before a
// fresh player entity's constructor has run.
namespace {

// lastEyeDirDrawn -- confirmed real persistent per-player field
// (`this.lastEyeDirDrawn`), but the logic that updates it
// (`if (this.getDirPerc()>0) { this.lastEyeDirDrawn=this.getLogicalDir(); }`)
// depends on getDirPerc()/getLogicalDir(), two more Player.gm-internal
// helpers outside this port's scope. Approximated with the same real
// fallback the reference itself uses when there's no meaningful direction
// yet (`vector2D(1,0)`), updated here directly from velocity direction
// instead -- visually equivalent for the overwhelming majority of frames
// (this field only diverges from raw velocity direction while airborne
// with no stick input, a narrow real case).
b2Vec2 g_lastEyeDir(1.0f, 0.0f);

// The native player draw never picks a colour literally -- it selects a
// NAME, and the active scheme decides what that name looks like. Decoded
// from FUN_001a1660's three colour-set sites (0x1a1c30, 0x1a22c8,
// 0x1a2660), each a call to 0x1d11e8(name, drawListHandle); see
//
// This is why the aura looked absent from the retail capture and was
// wrongly disabled here for a while: it is NOT drawn in player_out, it is
// drawn in ground_fill, and in the tutorial's scheme 150
// (game_data/Platformer/colours.dat) ground_fill is 255,255,255 -- pure
// white. Searching the capture for player_out pixels therefore found none
// and the aura was written off as "retail does not draw this". It does.
//
//     s150_ground_fill          255,255,255   <-- the white halo AND the eye
//     s150_player_fill          215,195,160   <-- the cream body
//     s150_player_out            76, 59, 52   <-- 0 px in the capture
//     s150_ground_noClimb_fill   50, 40, 19
//     s150_danger_fill          220, 88, 82
static const char* AuraColourName(gmMachine& a_machine, gmTableObject* a_player) {
    gmVariable deadVar = a_player->Get(&a_machine, "dead");
    if (deadVar.m_type == GM_INT && deadVar.m_value.m_int != 0) return "danger_fill";
    gmVariable stickyVar = a_player->Get(&a_machine, "stickyOn");
    const bool sticky = (stickyVar.m_type == GM_INT && stickyVar.m_value.m_int == 1);
    return sticky ? "ground_fill" : "ground_noClimb_fill";
}

void SetColourByName(Renderer& a_r, const char* a_name) {
    // GetNamedColourRGB's return value is NOT optional: on a miss it leaves
    // r/g/b untouched, so ignoring it fed UNINITIALISED stack floats
    // straight into SetColour. That is a real, observed failure, not a
    // theoretical one -- an unresolved "ground_fill" rendered the tutorial's
    // ground pure white (measured rgb(255,255,255) across 21% of the frame),
    // which is simply whatever happened to be on the stack.
    //
    // On a miss, leave the current draw colour alone (the caller's previous
    // SetColour stands) and say so once per name, so a missing scheme entry
    // surfaces as a diagnosable message instead of a random colour.
    float r, g, b;
    if (!GetNamedColourRGB(a_name, r, g, b)) {
        static std::set<std::string> reported;
        if (reported.insert(a_name).second)
            std::fprintf(stderr, "[colour] unresolved name '%s' -- keeping previous draw colour\n", a_name);
        return;
    }
    a_r.SetColour(r, g, b, 1.0f);
}

void DrawFilledFan(Renderer& a_r, const std::vector<b2Vec2>& a_verts, float a_z) {
    if (a_verts.size() < 3) return;
    std::vector<float> xy;
    xy.reserve(a_verts.size() * 2);
    for (const b2Vec2& v : a_verts) { xy.push_back(v.x); xy.push_back(v.y); }
    a_r.DrawPolygon(GL_TRIANGLE_FAN, xy.data(), (int)a_verts.size(), a_z);
}

float GetFloatField(gmMachine& a_m, gmTableObject* a_t, const char* a_key, float a_default) {
    gmVariable v = a_t->Get(&a_m, a_key);
    if (v.m_type == GM_FLOAT) return v.m_value.m_float;
    if (v.m_type == GM_INT) return (float)v.m_value.m_int;
    return a_default;
}

} // namespace

void DrawPlayerNative() {
    EngineContext& ctx = GetEngineContext();
    Renderer* renderer = ctx.renderer;
    gmMachine* machine = ctx.machine;
    if (!renderer || !machine) return;

    // Colour writes ON for this native pass -- same reasoning as
    // DrawGroundsNative (see its comment): script draw lists mask colour to
    // lay down stencil and do not reliably balance it back, and a native
    // pass must not inherit that.
    renderer->SetColourMask(false);

    gmVariable playerVar = FindPlayerEntity(*machine);
    if (playerVar.m_type != GM_TABLE) return; // no player yet (menu, between levels)
    gmTableObject* player = (gmTableObject*)machine->GetObject(playerVar.m_value.m_ref);

    // Real testDraw()'s own first-line early-out (`this.Entity.isInvisible()`,
    // see entity_bindings.cpp's gmf_entity_isInvisible) -- without this,
    // FIRSTSPAWN_STATEID's `parent.Entity.setIsInvisible(true)` (Player.gm)
    // stored the flag correctly but nothing ever consumed it, so the player
    // was visible for the entire spawn-in sequence instead of only
    // appearing once the spawner's animation finished.
    {
        gmVariable entityVar = player->Get(machine, "Entity");
        if (entityVar.m_type == GM_TABLE) {
            gmTableObject* entity = (gmTableObject*)machine->GetObject(entityVar.m_value.m_ref);
            gmVariable invisible = entity->Get(machine, "__invisible");
            if (invisible.m_type == GM_INT && invisible.m_value.m_int != 0) return;
        }
    }

    // Real testDraw()'s own second early-out, `if (this.dead) { return; }`
    // (Player.gm:5427-5429) -- confirmed via Ghidra decompile of the real
    // native Misc.cppDie (0x001a30c4) that death itself has no separate
    // native visual effect (it just flips two flags on the entity's native
    // struct and calls a function that decompiles to an empty `return;`,
    // FUN_0016b878 @ 0x0016b878) -- so in real retail, dying simply stops
    // the ball from being drawn at all, same as this script early-out
    // already says. Without this check, our own native reimplementation
    // kept drawing the ball normally even after death.
    {
        gmVariable deadVar = player->Get(machine, "dead");
        if (deadVar.m_type == GM_INT && deadVar.m_value.m_int != 0) return;
    }

    gmVariable mainpbVar = player->Get(machine, "mainpb");
    b2Body* body = GetB2BodyFromVar(*machine, mainpbVar);
    if (!body) return;

    // GE3_TRACE_PLAYER=1 reports the live gameplay state at draw time. This
    // keeps speed/state diagnosis tied to the script-driven player entity,
    // not to an inferred body id from the global movement log.
    if (std::getenv("GE3_TRACE_PLAYER")) {
        static int traceFrame = 0;
        ++traceFrame;
        if (traceFrame <= 80 || traceFrame % 120 == 0) {
            const b2Vec2 p = body->GetPosition();
            const b2Vec2 v = 0.001f * body->GetLinearVelocity();
            gmVariable stateVar = player->Get(machine, "state");
            int state = -1;
            if (stateVar.m_type == GM_TABLE) {
                gmTableObject* stateTable = (gmTableObject*)machine->GetObject(stateVar.m_value.m_ref);
                gmVariable id = stateTable->Get(machine, "id");
                if (id.m_type == GM_INT) state = id.m_value.m_int;
            }
            float dirX = 0.0f, dirY = 0.0f;
            GetVector2D(*machine, player->Get(machine, "logicalDir"), dirX, dirY);
            b2Fixture* fixture = body->GetFixtureList();
            std::fprintf(stderr,
                "[player] frame=%d state=%d type=%d pos=(%.4f,%.4f) "
                "vel=(%.6f,%.6f) dir=(%.2f,%.2f) input=%.2f sticky=%d "
                "mass=%.6f density=%.6f\n",
                traceFrame, state, (int)body->GetType(), p.x, p.y, v.x, v.y,
                dirX, dirY, GetFloatField(*machine, player, "dirPerc", 0.0f),
                (int)GetFloatField(*machine, player, "stickyOn", 0.0f),
                body->GetMass(), fixture ? fixture->GetDensity() : 0.0f);
        }
    }

    // radius -- confirmed real: `local s=(SW*2-8*BACK_CONVERSION);
    // this.radius=s/2-8*BACK_CONVERSION;` (Player.gm ~line 45-47), i.e.
    // SW - 12*BACK_CONVERSION. Read the live field first (this.radius
    // isn't reassigned after construction in the corpus, but reading it
    // live costs nothing and stays correct if that ever changes).
    constexpr float kSW = 0.61f;
    constexpr float kBackConversion = 0.01f;
    float radius = GetFloatField(*machine, player, "radius", kSW - 12.0f * kBackConversion);
    float maxSpeedX = GetFloatField(*machine, player, "maxSpeedX", kSW / 51.0f);
    float squintPerc = GetFloatField(*machine, player, "squintPerc", 0.0f);
    float stickGraphicPerc = GetFloatField(*machine, player, "stickGraphicPerc", 0.0f);
    float drawz = GetFloatField(*machine, player, "drawz", -17.8f); // PLAYER_Z, real value from the
                                                                // retail bootstrap (0xc18e6666).
                                                                // Was 60.0 -- a guess that put the
                                                                // player in FRONT of GROUND_Z.

    // lastJumpNormal -- confirmed real: `!?this.lastJumpNormal` is the
    // reference's own null-check (set to null on spawn/state-reset,
    // Player.gm:36/471/2015/etc, and to a real contact normal on
    // landing/wall-jump elsewhere in its movement state machine), so
    // absence here means "not touching/stuck to anything", not a value
    // to default away.
    bool haveStickDir = false;
    b2Vec2 stickDir(0.0f, 0.0f);
    {
        gmVariable ljn = player->Get(machine, "lastJumpNormal");
        float nx, ny;
        if (ljn.m_type == g_vector2DType && GetVector2D(*machine, ljn, nx, ny)) {
            stickDir = -1.0f * b2Vec2(nx, ny); // `stickDir=this.lastJumpNormal*-1;`
            haveStickDir = true;
        }
    }

    b2Vec2 pos = body->GetPosition();
    // Script-visible velocities are world units per millisecond.
    b2Vec2 vel = 0.001f * body->GetLinearVelocity();
    float speed = vel.Length();

    // dir/idx -- confirmed real: `local s=this.mainpb.getSpeed(); ...
    // if (s>(SW/1311.0)) { dir=this.mainpb.getVel().getNormalized(); }
    // else { dir=this.lastEyeDirDrawn; }` and the idx speed-tier lean
    // (`if (s>=maxSpeedX*0.8) idx=2; else if (s>=maxSpeedX*0.39) idx=1;
    // else idx=0;`), used to squash/lean the aura slightly in the
    // movement direction at higher speeds.
    b2Vec2 dir = g_lastEyeDir;
    if (speed > kSW / 1311.0f) {
        dir = (1.0f / speed) * vel;
        g_lastEyeDir = dir;
    }
    int idx = 0;
    if (speed >= maxSpeedX * 0.8f) idx = 2;
    else if (speed >= maxSpeedX * 0.39f) idx = 1;

    renderer->SetUseCameraView(true);
    // ZINCER, the real retail value (0x3cf5c28f). Used to separate this
    // draw's sub-layers in depth. Once the depth test is on, coplanar
    // passes are NOT harmless: GL_LESS discards a later fragment at the
    // SAME z, so drawing aura/body/eye all at `drawz` made the body and
    // eye vanish entirely behind the aura. Retail spaces its sub-layers
    // by n*ZINCER for exactly this reason.
    constexpr float kZincer = 0.03f;
    // Depth test ON -- see the matching comment in ground_renderer.cpp.
    // With this off, PLAYER_Z (-17.8) vs GROUND_Z (-10) could not order
    // anything and the player drew over the platforms regardless of the
    // depth constants.
    renderer->SetDepthTest(true);

    // Lean offset applied to the aura only (reference pushMatrix/
    // translate/.../popMatrix scope) -- `dir*(-r*0.16*idx)`.
    b2Vec2 leanOfs = (-radius * 0.16f * (float)idx) * dir;
    b2Vec2 auraCenter = pos + leanOfs;

    // ---- aura (outline) ----------------------------------------------
    // RE-ENABLED. Retail DOES draw this; the reasoning that disabled it was
    // wrong, and the note below is kept as a record of the mistake.
    //
    // The old argument was: the retail capture contains zero pixels of
    // player_out (75,59,52), therefore no aura. The flaw is that the aura
    // is not drawn in player_out. The native draw's first pass sets
    // "ground_fill", and in the tutorial's scheme 150 ground_fill is
    // 255,255,255 -- so the aura is WHITE, and the capture has 644 white
    // pixels sitting exactly around the player. Cropping the capture shows
    // it plainly: a white 8-lobe halo around a cream body with a small
    // white eye. See AuraColourName() above and section 7 of the
    // ground-truth doc.
    //
    // Geometry note: these formulas are PlayerOLD.gm's testDraw(), which is
    // an approximation of the native shape -- retail builds a 64-segment
    // circle and deforms it per-frame in the block at 0x1a1cc8..0x1a2bc0
    // that is still being decoded. The halo is definitely present and
    // definitely white; its exact silhouette is not yet 1-1.
    //
    // Historical note, left because the pixel counts are still useful:
    // The formulas below are a faithful port of Platformer/PlayerOLD.gm's
    // testDraw(), and it draws the aura unconditionally -- but PlayerOLD.gm
    // is, as its name says, the OLD player. Retail's shipped player
    // rendering is native (see this file's header), and the real capture
    // says the native version has no aura: sampling the player region of
    //
    //     (210,190,153) 1537px   player_fill, the body
    //     (255,255,255)  627px   the white inner highlight
    //     ( 42, 33, 15)  230px   ground behind it
    //
    // and NOT ONE pixel of player_out (75,59,52). That capture is named
    // "moving" and the aura is speed-driven, so this is not a case of
    // catching it in a state where the aura happens to be suppressed.
    //
    // Drawing it inverted the player: an 8-lobe player_out blob at 1.6x
    // radius under a comparatively tiny fill (measured 1822px dark against
    // 767px cream), where retail shows a cream body and no dark halo at all.
    // Kept, not deleted, because the geometry is a real reference port and
    // is what the aura would need if a state that genuinely uses it turns up.
    // -- and it did: the "no dark halo" observation was right, but the
    // conclusion "therefore no halo" was wrong. The halo is white.
    //
    // Confirmed real formulas, Platformer/PlayerOLD.gm testDraw(): an
    // 8-lobe wobbly ring (qq=6 verts/lobe boundary, n=8 lobes) built as
    // alternating outer/inner arcs so the aura reads as a soft flower/
    // blob shape rather than a plain circle, stretched opposite the
    // movement direction (a speed-scaled "trailing" elongation) and
    // squashed toward lastJumpNormal while wall-stuck (stickDir/
    // stickGraphicPerc, read above).
    {
        constexpr int kQQ = 6, kN = 8;
        float auraR = radius * 1.6f;
        float outCircPerc = (0.39f + 0.13f + 0.06f) / (float)kN;
        float inCircPerc = (1.0f - outCircPerc * (float)kN) / 8.0f;
        float outAngLen = outCircPerc * 2.0f * (float)M_PI;
        float inAngLen = inCircPerc * 2.0f * (float)M_PI;
        float outAngInc = outAngLen / (float)(kQQ - 1);
        float inAngInc = inAngLen / (float)(kQQ - 1);
        float ampAngInc = (float)M_PI / (float)(kQQ - 1);
        float outAmp = auraR * (0.06f - 0.01f);
        float inAmp = auraR * 0.03f;
        // MATHUTIL_GetAngleRad(this.mainpb.getDir()) -- getDir() is the
        // SAME normalized-velocity vector as `dir` above (confirmed:
        // gmf_pb_getDir), not the body's physical rotation.
        float ang = std::atan2(dir.y, dir.x);
        b2Vec2 stretchDir = -1.0f * dir;
        float speedPerc = std::min(1.0f, std::max(0.0f, speed / maxSpeedX));
        float stretchAmt = auraR * 0.3f * speedPerc;
        b2Vec2 stretchTargetPos = (auraR * 2.7f) * stretchDir; // auraR + auraR*1.7
        // `local curve=0.8; ... local downCurve=7.0*this.stickGraphicPerc;
        // local stickRPerc=0.3*this.stickGraphicPerc; local
        // stickAngRange=PIBYTWO*0.9;` -- stickBaseRPerc is always 0 in the
        // reference (a leftover unused param: `r*=(1+0*perc)` is a no-op),
        // so only rp (a wobble-amplitude multiplier) needs reconstructing.
        constexpr float kStickAngRange = (float)M_PI / 2.0f * 0.9f;
        float downCurve = 7.0f * stickGraphicPerc;
        float stickRPerc = 0.3f * stickGraphicPerc;

        std::vector<b2Vec2> verts;
        verts.push_back(b2Vec2(0, 0));
        b2Vec2 first(0, 0);
        bool haveFirst = false;

        auto pushLobe = [&](bool a_outer) {
            float ampAng = 0.0f;
            for (int ii = 0; ii < kQQ; ++ii) {
                b2Vec2 d(std::cos(ang), std::sin(ang));
                float rp = 1.0f;
                if (haveStickDir) {
                    float sdot = std::max(-1.0f, std::min(1.0f, b2Dot(stickDir, d)));
                    float sdelta = std::acos(sdot);
                    if (sdelta < kStickAngRange) {
                        float sperc = sdelta / kStickAngRange;
                        sperc = std::max(0.0f, std::min(1.0f, std::pow(std::max(0.0f, std::min(1.0f, sperc)), downCurve)));
                        rp *= (1.0f + (1.0f - sperc) * stickRPerc);
                    }
                }
                // outer: r + MATH_SIN_POW(ampAng,0.7)*outAmp
                // inner: r - MATH_SIN(ampAng)*inAmp
                float rOfs = a_outer
                    ? std::copysign(std::pow(std::fabs(std::sin(ampAng)), 0.7f), std::sin(ampAng)) * outAmp
                    : -std::sin(ampAng) * inAmp;
                b2Vec2 p = ((auraR + rOfs) * rp) * d;

                float dot = std::max(-1.0f, std::min(1.0f, b2Dot(stretchDir, d)));
                float delta = std::acos(dot);
                if (delta < (float)M_PI / 2.0f) {
                    float perc = 1.0f - std::pow(delta / ((float)M_PI / 2.0f), 0.8f);
                    b2Vec2 toTarget = stretchTargetPos - p;
                    float len = toTarget.Length();
                    if (len > 0.0001f) p += (stretchAmt * perc / len) * toTarget;
                }
                verts.push_back(p);
                if (!haveFirst && a_outer && ii == 1) { first = p; haveFirst = true; }
                ang += a_outer ? outAngInc : inAngInc;
                ampAng += ampAngInc;
            }
            ang -= a_outer ? outAngInc : inAngInc;
        };

        for (int i = 0; i < kN; ++i) {
            pushLobe(true);
            pushLobe(false);
        }
        if (haveFirst) verts.push_back(first);

        // Colour: the native draw's first pass (submit @0x1a2288, colour set
        // @0x1a1c30) selects between three NAMES, and none of them is
        // player_out:
        //     dead            -> "danger_fill"
        //     stickyOn == 1   -> "ground_fill"
        //     otherwise       -> "ground_noClimb_fill"
        SetColourByName(*renderer, AuraColourName(*machine, player));
        std::vector<b2Vec2> world;
        world.reserve(verts.size());
        for (const b2Vec2& v : verts) world.push_back(auraCenter + v);
        // One ZINCER behind the body -- the retail capture shows the
        // cream body drawn over the white halo, so the halo is the
        // farther of the two.
        DrawFilledFan(*renderer, world, drawz - kZincer);
    }

    // ---- body (fill) ---------------------------------------------------
    // Confirmed real: a filled ellipse, `rx=radius, ry=radius*(1-
    // squintPerc*0.3)`, plotted via `MATH_COS(ang)*rx, MATH_SIN_POW(ang,c)
    // *ry` (c=1-0.6*squintPerc) -- squintPerc read live off the entity
    // table above; reduces to a plain circle (rx==ry==radius, c==1) when
    // it's 0 (not currently squinting/blinking).
    {
        // 64, not 16: the native draw caches the body circle's segment
        // count in this+0x1ec and builds it with 0x40 segments
        // (0x1a1998 `li r4, 0x40` / `stw r4, 0x1ec(r25)`), against 0x3d=61
        // for the eye. 16 was a guess and visibly polygonal at this radius.
        constexpr int kShapeQ = 64;
        float rx = radius;
        float ry = radius * (1.0f - squintPerc * 0.3f);
        float c = 1.0f - 0.6f * squintPerc;
        float angInc = 2.0f * (float)M_PI / (float)(kShapeQ - 1);
        std::vector<b2Vec2> verts;
        verts.reserve(kShapeQ);
        for (int i = 0; i < kShapeQ; ++i) {
            float a = (float)i * angInc;
            float s = std::sin(a);
            float sPow = std::copysign(std::pow(std::fabs(s), c), s);
            verts.push_back(pos + b2Vec2(std::cos(a) * rx, sPow * ry));
        }
        SetColourByName(*renderer, "player_fill");
        DrawFilledFan(*renderer, verts, drawz);
    }

    // ---- eye -------------------------------------------------------------
    // Confirmed real: a small filled circle (`r*(11.0/60.0)`), offset
    // along the facing direction by `eyePosR = inr/2` (inr==radius here,
    // this port doesn't reconstruct the unused `*(42.0/60.0)` scale the
    // reference itself leaves commented out) and rotated with
    // lastEyeDirDrawn, drawn one ZINCER above the body.
    {
        float eyeR = radius * (11.0f / 60.0f);
        float eyePosR = radius / 2.0f;
        b2Vec2 eyeCenter = pos + eyePosR * g_lastEyeDir;
        constexpr int kEyeSegs = 61; // VIDEOHELPER_DrawFilledCircle's confirmed default `local q=61;`
        std::vector<b2Vec2> verts;
        verts.reserve(kEyeSegs);
        for (int i = 0; i < kEyeSegs; ++i) {
            float a = (float)i / (float)kEyeSegs * 2.0f * (float)M_PI;
            verts.push_back(eyeCenter + eyeR * b2Vec2(std::cos(a), std::sin(a)));
        }
        // The eye is the LAST of the four native submits (@0x1a2b30), and
        // that submit sets no colour of its own -- it inherits the one set
        // at 0x1a2660, which is the same ground_fill/ground_noClimb_fill/
        // danger_fill selection the aura uses. In scheme 150 ground_fill is
        // white, which matches the small white dot visible inside the body
        // Previously drew player_out (76,59,52) -- a colour the native draw
        // never references and which has zero pixels in that capture.
        SetColourByName(*renderer, AuraColourName(*machine, player));
        DrawFilledFan(*renderer, verts, drawz + kZincer);
    }
}
