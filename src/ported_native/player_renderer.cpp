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

void SetColourByName(Renderer& a_r, const char* a_name) {
    float r, g, b;
    GetNamedColourRGB(a_name, r, g, b);
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
    float drawz = GetFloatField(*machine, player, "drawz", 60.0f); // PLAYER_Z, constants_bindings.cpp

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
    // ENGINE PATCH (2026-08-16): GetLinearVelocity() is raw sim-space (see
    // kScriptVelocityToSimScale's header comment, physics_body_bindings.h)
    // -- ~1000x larger than the script-space maxSpeedX this gets compared
    // against below. Left unscaled, speed/maxSpeedX was permanently >>1
    // (clamped to 1.0) even standing still, so the aura's speed-based
    // stretch ("trail") rendered at full intensity at all times, and the
    // idx speed-tier lean was permanently maxed out too -- looking like
    // holding Square (run) never visibly changed anything, since the
    // graphic was already stuck at its fastest-tier appearance regardless.
    b2Vec2 vel = (1.0f / kScriptVelocityToSimScale) * body->GetLinearVelocity();
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
    renderer->SetDepthTest(false);

    // Lean offset applied to the aura only (reference pushMatrix/
    // translate/.../popMatrix scope) -- `dir*(-r*0.16*idx)`.
    b2Vec2 leanOfs = (-radius * 0.16f * (float)idx) * dir;
    b2Vec2 auraCenter = pos + leanOfs;

    // ---- aura (outline) ----------------------------------------------
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

        SetColourByName(*renderer, "player_out");
        std::vector<b2Vec2> world;
        world.reserve(verts.size());
        for (const b2Vec2& v : verts) world.push_back(auraCenter + v);
        DrawFilledFan(*renderer, world, drawz);
    }

    // ---- body (fill) ---------------------------------------------------
    // Confirmed real: a filled ellipse, `rx=radius, ry=radius*(1-
    // squintPerc*0.3)`, plotted via `MATH_COS(ang)*rx, MATH_SIN_POW(ang,c)
    // *ry` (c=1-0.6*squintPerc) -- squintPerc read live off the entity
    // table above; reduces to a plain circle (rx==ry==radius, c==1) when
    // it's 0 (not currently squinting/blinking).
    {
        constexpr int kShapeQ = 16;
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
        constexpr float kZincer = 0.01f; // constants_bindings.cpp
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
        SetColourByName(*renderer, "player_out");
        DrawFilledFan(*renderer, verts, drawz + kZincer);
    }
}
