#include "draw_list.h"
#include "draw_context.h"
#include "../core/engine_context.h"
#include "../core/physics_world.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/shape_bindings.h"
#include "../bindings/animation_bindings.h"
#include "../core/strict_calls.h"
#include <box2d/box2d.h>
#include <GL/glew.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>

namespace {

gmType g_drawListType = -1;

} // namespace

DrawListData* RecordingTarget(gmThread* a_thread) {
    gmUserObject* obj = a_thread->ThisUserObject();
    if (obj && obj->m_userType == g_drawListType) return (DrawListData*)obj->m_user;
    return nullptr;
}

void ReplayDrawList(DrawListData* a_data) {
    if (!a_data) return;
    for (auto& cmd : a_data->commands) cmd();
}

namespace {

// Was a pure no-op -- confirmed a real bug, not just an accuracy gap: the
// matrix/attribute stack in DrawContext (draw_context.h) is a single
// process-wide shared instance, not scoped per drawList object (see that
// file's own header comment on why), so ANY unbalanced pushMatrix/
// pushAttributes left over from one process's preDraw() call carried
// straight into the next thing that drew that same frame -- including a
// DIFFERENT process's postDraw(), which starts with its own
// `drawList.reset()` specifically expecting a clean slate. Real usage
// confirms every real preDraw()/postDraw() calls reset() as its very first
// drawList operation (e.g. Platformer/Main.gm's own preDraw()/postDraw(),
// both of which do exactly this) -- with reset() doing nothing, that
// leftover state (observed: Platformer/Main.gm's postDraw() "press
// {ACCEPT} to continue" screen rendering squeezed into a small sub-region
// of the screen instead of filling it) silently corrupted every later pass
// in the same frame.
int GM_CDECL gmf_reset(gmThread* a_thread) {
    DrawListData* d = (DrawListData*)a_thread->ThisUserCheckType(g_drawListType);
    if (!d) return GM_EXCEPTION;
    // reset() is the real "clear me, I'm about to be rebuilt from scratch
    // this frame" signal -- always executes immediately (never itself
    // deferred/recorded), same as callDrawList's own administrative role.
    d->commands.clear();
    DrawContext& ctx = GetDrawContext();
    ctx.stack.assign(1, DrawContext::Mat{});
    ctx.attrStack.clear();
    ctx.depthTest = false;
    ctx.depthWrite = true;
    ctx.colourMask = true;
    ctx.stencilFunc = 8; // VIDEOPARAMS_DISABLE
    ctx.stencilRef = 0;
    ctx.stencilSfail = ctx.stencilDpfail = ctx.stencilDppass = 100; // VIDEOPARAMS_KEEP
    ctx.fontEdge = 0.5f;
    ctx.fontWidth = 0.5f;
    // ENGINE PATCH (2026-08-17): reset() never restored the actual draw
    // COLOUR (Renderer::m_colour) to white, only DrawContext's matrix/
    // attribute state -- confirmed real, high-impact bug: colour lives on
    // a single process-wide Renderer instance, not scoped per drawList
    // (same "shared, not per-list" architecture as DrawContext itself, see
    // this function's own header comment above), so ANY earlier
    // setColour(0,0,0,...) left over from a fade-to-black transition or a
    // background wash (PauseProcessBase.gm's exitData fade, screen-clear
    // washes, etc.) silently carried into every SUBSEQUENT drawList's
    // replay for the rest of that frame -- including textured draws
    // (drawAnimation/drawTextureByHandle), which multiply the sampled
    // texture by this colour in the shader (`texture(uTexture,vUV) *
    // uColour`) with no per-call reset of their own. A sprite drawn any
    // time after a stray black tint, without an intervening explicit
    // setColour(white) call, rendered as a solid black rectangle --
    // exactly the "all textures fully black" symptom reported live. Every
    // real preDraw()/postDraw() calls reset() as its first drawList
    // operation specifically expecting a clean slate (see this function's
    // own header comment), and colour is as much a part of that clean
    // slate as the matrix stack.
    GetEngineContext().renderer->SetColour(1.0f, 1.0f, 1.0f, 1.0f);
    // Same bug, same fix, for DrawContext::currentTextureHandle (see its
    // own header comment, draw_context.h): setTextureByHandle/
    // setTextureSubImg (SafeVine.gm's thorn-drawing code) bind a texture
    // that DC_gmf_drawShape then applies to EVERY subsequent plain
    // drawShape() call process-wide until clearTexture() runs -- but nothing
    // guarantees clearTexture() actually runs before some OTHER, unrelated
    // entity's own drawShape() call later in the same frame (its own
    // reset() call resets everything else, but not this). Confirmed live:
    // solid black squares appearing where plain flat-colour ground/prop
    // shapes should be -- a stray leftover vine texture handle being
    // applied to their bounding box with UV coordinates that don't match,
    // sampling into transparent/black texture regions. Clearing it here
    // means every drawList starts governing its own texture-bind state
    // fresh, matching the same "reset() means truly clean" contract as
    // colour above.
    ctx.currentTextureHandle = 0;
    return GM_OK;
}

// drawList.drawStaticSurface() -- confirmed real (Platformer/Main.gm's
// preDraw(): `drawList.drawStaticSurface();`, right after clearing the
// screen and before drawing vibe backgrounds) but with no real
// static-surface render-to-texture cache implemented (a genuine
// performance optimization real levels use -- ground doesn't move, so
// retail renders it once to an offscreen buffer and blits that each
// frame instead of redrawing every ground piece every frame; out of
// scope for this pass). This draws every live Box2D body/fixture
// directly and immediately instead, every frame, no caching -- gives
// real, physically-accurate level geometry on screen (matching real
// level data: shapes/positions came from the actual parsed level XML)
// using simple flat colours keyed off collision category, since no real
// per-entity-type sprite/texture rendering exists yet either. Sensors are
// skipped (nothing in the corpus draws a sensor's own shape visibly).
// Colour categories match the bit values physics_bindings.cpp/
// constants_bindings.cpp assigned to PLAYER_COLLISION_CATEGORY (1<<0) and
// GROUND_COLLISION_CATEGORY (1<<1) -- everything else (checkpoints,
// drones, decorations, ...) gets a neutral fallback colour.
//
// Camera: deliberately NOT overridden here -- the real script-side camera
// (Video.setCameraPos, wired through in video_bindings.cpp, plus
// drawList.setupWorldView()/setupOrthoView() toggling
// Renderer::m_useCameraView) is left in full control, matching real
// retail's own architecture (confirmed via Ghidra: Video.setCameraPos's
// native body, FUN_00052908, is a raw unconditional store of the 3 args
// into a VideoState struct -- no native recentre/fit-to-content logic
// exists anywhere in retail to reproduce). If the script's own camera
// setup leaves nothing visible (e.g. because Main.gm's
// `//this.finalizeLoad();` call is commented out for this old-XML-format
// level, so `this.player.mainpb.setPos(0,0);`'s placeholder position is
// never corrected -- confirmed real, Main.gm:1501-1502), that's an
// honest reflection of the real script's own current state, not
// something this render layer should paper over.
void DrawEntityMarkers(Renderer& a_renderer, const std::vector<int>& a_skipUids); // defined below

// opaque full-screen background quad, which -- since this whole pass runs
// AFTER every real draw call for the frame (main.cpp: DrawGroundsNative/
// DrawEntitiesWithNativeDrawHook/DrawPlayerNative/PostDrawAll all run
// first) -- completely hid the real gameplay behind it instead of
// overlaying on top of it; (2) filled every fixture at alpha=1, same
// problem; (3) coloured fixtures by collision category to vaguely resemble
// the real tutorial level's own palette, which is exactly backwards for a
// DEBUG view -- the point of this overlay is to see simulation bounds
// clearly against the real rendered scene, not to imitate it. Now: no
// background quad at all, every live fixture in the sim is drawn (sensors
// included -- this must show literally everything Box2D is simulating,
// not a filtered subset), translucent fill + an opaque outline on top for
// a crisp boundary, dynamic bodies in green (the user's own ask -- lets
// the player body and any moving/pushable object read at a glance),
// static/kinematic bodies in cyan, and sensors in yellow (dashed-effect
// impossible with GL_LINE_LOOP alone, so just lower alpha to distinguish
// "not actually solid" from real collision bounds) all so this reads as an
// overlay, not a replacement view.
void DrawPhysicsBodiesImmediate(const std::vector<int>& a_nativeDrawnUids) {
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer) return;
    // Defensive: whatever the script's own preceding draws left the
    // depth-test/depth-write state at
    // (`drawList.setAttrDepthTest`/`setAttrDepthWrite`, both real, e.g.
    // used for real ground/vibe z-layering) must not affect this. Left
    // enabled from an earlier same-frame draw, GL_DEPTH_TEST's default
    // func (GL_LESS) silently discards every later fragment at the SAME
    // z (this renderer is purely 2D -- every shape is drawn at z=0), so
    // only the very first fixture in the loop below would ever actually
    // reach the framebuffer.
    renderer->SetDepthTest(false);
    // Same reasoning as the old background quad's own comment (now
    // removed): drawStaticSurface() runs after PostDrawAll, which can
    // legitimately leave the renderer in screen-space UI mode
    // (setupAspectRatioOrthoView) -- force world/camera-relative mode back
    // on so every position/radius below lines up with the real gameplay
    // it's overlaid on, regardless of what the last script draw call left
    // active.
    renderer->SetUseCameraView(true);

    auto drawShape = [&](const b2Fixture* fx, const b2Transform& xf, float a_r, float a_g, float a_b, float a_fillA, float a_outlineA) {
        const b2Shape* shape = fx->GetShape();
        static std::vector<float> verts;
        verts.clear();
        if (shape->GetType() == b2Shape::e_circle) {
            const b2CircleShape* c = (const b2CircleShape*)shape;
            b2Vec2 center = b2Mul(xf, c->m_p);
            constexpr int kSegs = 24;
            for (int i = 0; i < kSegs; ++i) {
                float ang = (float)i / (float)kSegs * 2.0f * (float)M_PI;
                verts.push_back(center.x + std::cos(ang) * c->m_radius);
                verts.push_back(center.y + std::sin(ang) * c->m_radius);
            }
        } else if (shape->GetType() == b2Shape::e_polygon) {
            const b2PolygonShape* p = (const b2PolygonShape*)shape;
            for (int i = 0; i < p->m_count; ++i) {
                b2Vec2 v = b2Mul(xf, p->m_vertices[i]);
                verts.push_back(v.x);
                verts.push_back(v.y);
            }
        } else {
            return; // edge/chain -- no real usage creates these, nothing to draw
        }
        int count = (int)verts.size() / 2;
        if (count < 2) return;
        renderer->SetColour(a_r, a_g, a_b, a_fillA);
        renderer->DrawPolygon(GL_TRIANGLE_FAN, verts.data(), count);
        renderer->SetColour(a_r, a_g, a_b, a_outlineA);
        renderer->DrawPolygon(GL_LINE_LOOP, verts.data(), count);
    };

    b2World& world = GetPhysicsWorld();
    for (b2Body* body = world.GetBodyList(); body; body = body->GetNext()) {
        const b2Transform& xf = body->GetTransform();
        bool isDynamic = (body->GetType() == b2_dynamicBody);
        for (b2Fixture* fx = body->GetFixtureList(); fx; fx = fx->GetNext()) {
            // of red/green's usual green -- red-green colour-blindness is
            // the most common form, so avoid relying on a red-vs-green
            // distinction anywhere in this overlay.
            if (fx->IsSensor()) {
                drawShape(fx, xf, 1.0f, 0.95f, 0.15f, 0.12f, 0.55f); // yellow, faint fill -- "here but not solid"
            } else if (isDynamic) {
                drawShape(fx, xf, 1.0f, 0.15f, 0.1f, 0.28f, 0.9f);   // red -- dynamic bodies
            } else if (body->GetType() == b2_staticBody) {
                // Solid/near-opaque on purpose: physics_body_bindings.cpp's
                // SyncBodyTypeFromMass now converts every zero-mass body
                // (setMass(0) -- every "immovable" ground piece) to a real
                // b2_staticBody, so this colour == "actually solid/
                // immovable", not just "currently not dynamic".
                drawShape(fx, xf, 0.15f, 0.55f, 1.0f, 0.75f, 0.95f); // blue, solid -- static (mass 0 -> truly immovable)
            } else {
                drawShape(fx, xf, 0.9f, 0.9f, 0.9f, 0.22f, 0.8f);    // white -- kinematic (none exist yet, but distinct in case)
            }
        }
    }

    // Real, script-authored per-entity draw hooks (EntityPacks/*.gm's
    // `t.draw=function(){...}`) are called for real, unconditionally,
    // every frame from main.cpp now -- NOT from here (see that call site's
    // comment for why: this whole function is opt-in debug-only
    // (GE3_PHYSICS_DEBUG_DRAW), and entity draw hooks calling real script
    // code must not depend on a debug flag). a_nativeDrawnUids is that
    // same frame's already-computed list, passed in just so the debug
    // marker overlay below doesn't double up on entities that already drew
    // themselves for real.
    renderer->SetDepthTest(false);
    renderer->SetUseCameraView(true);
    DrawEntityMarkers(*renderer, a_nativeDrawnUids);
}

// Plots every entity in the native EntityManager registry
// (entity_bindings.cpp's GetAllEntityDebugPositions) at its
// configureData.posX/posY, independent of whether it has a Box2D body at
// all -- most real entity types (Note, BG_Mob, spawnpoints, triggers,
// decorations, ...) never touch physics, so DrawPhysicsBodiesImmediate's
// body/fixture loop above never sees them. Drawn as a small diamond
// marker (deliberately distinct in shape/colour from the physics-body
// fills above) so overlap with a real physics-backed entity's own fill is
// still visible underneath, per the same "flat bounding box, not real
// sprites" scope as the rest of this file. Position convention matches
// physics bodies exactly (no Y-flip) -- both ultimately come from the
// same untouched configureData.posX/posY read (see MakePhysicsBody /
// GAME_CalculateFillVerts), unlike the user's separate JS level-viewer
// reference which flips Y for its own canvas coordinate system.
void DrawEntityMarkers(Renderer& a_renderer, const std::vector<int>& a_skipUids) {
    constexpr float kHalf = 0.45f;
    for (const EntityDebugInfo& e : GetAllEntityDebugPositions()) {
        if (!e.hasPos) continue;
        if (std::find(a_skipUids.begin(), a_skipUids.end(), e.uid) != a_skipUids.end()) continue;
        if (e.isPlayer) a_renderer.SetColour(0.15f, 0.95f, 0.95f, 0.7f);  // cyan -- player entity marker
        else a_renderer.SetColour(0.95f, 0.9f, 0.15f, 0.7f);             // yellow -- every other entity marker
        float verts[8] = {
            e.x,          e.y + kHalf,
            e.x + kHalf,  e.y,
            e.x,          e.y - kHalf,
            e.x - kHalf,  e.y,
        };
        a_renderer.DrawPolygon(GL_TRIANGLE_FAN, verts, 4);
    }
}

// Real retail drawStaticSurface() is a cached render-to-texture of the
// level's static background (ground, vibe, base colour wash) with real
// per-entity sprite/texture rendering -- this engine has no such caching
// pipeline, and the flat bounding-box/diamond debug shapes drawn straight
// from live Box2D fixtures (DrawPhysicsBodiesImmediate/DrawEntityMarkers
// above) are an invented visual, not confirmed retail output, so they stay
// opt-in only (GE3_PHYSICS_DEBUG_DRAW, platform/main.cpp) rather than
// backing this call.
//
// DrawList.c @ 0x000801cc): it takes 0 arguments and does nothing but
// append a single fixed opcode to the draw-command stream -- no colour,
// texture, or geometry parameters of its own at all. Combined with Main.gm's
// real call order (`COLOURSCHEME_SetColour("clear",drawList);` immediately
// followed by `drawList.drawStaticSurface();`, with the `drawList.clear()`
// call between them commented out) that confirms retail relies on
// drawStaticSurface() ITSELF to paint using whatever colour was just set,
// not a separate clear -- i.e. its one confirmed real effect is "fill the
// visible area with the current draw colour". That's implemented for real
// here (a full-screen quad in the renderer's current colour, sized/
// positioned the same way the debug overlay's background quad already
// was); the cached ground/vibe/sprite compositing on top of it is the
// part with no real pipeline to port yet.
int GM_CDECL gmf_drawStaticSurface(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        Renderer* renderer = GetEngineContext().renderer;
        if (!renderer) return;
        float cx = renderer->GetCameraX(), cy = renderer->GetCameraY();
        // Visible world extent now comes from the real perspective
        // projection (it scales with camera Z) rather than a fixed
        // half-height constant -- see Renderer::kTanHalfFovY.
        float halfH = renderer->GetWorldHalfHeight() * 1.05f; // slight margin so edges never peek through
        float halfW = renderer->GetWorldHalfWidth() * 1.05f;
        float quad[8] = {
            cx - halfW, cy - halfH,
            cx + halfW, cy - halfH,
            cx + halfW, cy + halfH,
            cx - halfW, cy + halfH,
        };
        renderer->DrawPolygon(GL_TRIANGLE_FAN, quad, 4);
    });
}

// confirmed a genuine retail no-op: the decompiled body is exactly
// `undefined8 setDebugID(void) { return 0; }`, discarding whatever it's
// called with (real corpus call sites all pass a single label string, e.g.
// `drawList.setDebugID("QUADSHADOWED")`, purely for whatever debug tooling
// retail's dev builds had -- nothing here reads it back).
int GM_CDECL gmf_setDebugID(gmThread* a_thread) {
    if (!a_thread->ThisUserCheckType(g_drawListType)) return GM_EXCEPTION;
    return GM_OK;
}

// 0x00080788) but the decompiled body is opaque refcount boilerplate; the
// NAME and every real call site (QuadShadowed.gm:50's
// `drawList.callDrawList(this.shadowDrawList)`, EntityMenu_*Cell.gm's
// `parentDrawList.callDrawList(this.drawList)`, ...) agree unambiguously on
// the contract: replay another drawList's recorded commands into this one.
// Called through a real drawList receiver itself (never bare `Video.
// callDrawList(...)` anywhere in the corpus), so it follows the same
// record-vs-execute rule as everything else here.
int GM_CDECL gmf_callDrawList(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmUserObject* obj = a_thread->ParamUserObject(0);
    if (!obj || obj->m_userType != g_drawListType) return GM_OK; // tolerant-null, same convention as drawShape
    DrawListData* target = (DrawListData*)obj->m_user;
    if (DrawListData* rec = RecordingTarget(a_thread)) {
        rec->commands.push_back([target] { ReplayDrawList(target); });
    } else {
        ReplayDrawList(target);
    }
    return GM_OK;
}

gmFunctionEntry s_drawListLib[] = {
    {"reset", gmf_reset, nullptr},
    {"setDebugID", gmf_setDebugID, nullptr},
    {"callDrawList", gmf_callDrawList, nullptr},
    {"drawStaticSurface", gmf_drawStaticSurface, nullptr},
    {"drawShape", DC_gmf_drawShape, nullptr},
    {"drawShapeWithTexture", DC_gmf_drawShapeWithTexture, nullptr},
    // Same shared drawList/Video vocabulary as the rest of this table --
    // confirmed real NAMES (see draw_context.cpp/shape_bindings.h for the
    // EBOOT.elf method-table evidence), but no decompiled body reachable
    // without live Ghidra cross-reference access -- routed through the
    // strict-call trap rather than the previous corpus-inferred bodies.
    {"beginShape", gmf_Shape_beginShape, nullptr},
    {"plotVertex", gmf_Shape_plotVertexCurrent, nullptr},
    {"endShape", gmf_Shape_endShapeCurrent, nullptr},
    {"setupWorldView", DC_gmf_setupWorldView, nullptr},
    {"setupOrthoView", DC_gmf_setupOrthoView, nullptr},
    {"setupAspectRatioOrthoView", DC_gmf_setupAspectRatioOrthoView, nullptr},
    {"pushAttributes", DC_gmf_pushAttributes, nullptr},
    {"popAttributes", DC_gmf_popAttributes, nullptr},
    {"setAttrDepthTest", DC_gmf_setAttrDepthTest, nullptr},
    {"setAttrDepthWrite", DC_gmf_setAttrDepthWrite, nullptr},
    {"setColourMask", DC_gmf_setColourMask, nullptr},
    {"setAttrStencil", DC_gmf_setAttrStencil, nullptr},
    {"setAttrDefaultStencil", DC_gmf_setAttrDefaultStencil, nullptr},
    {"setAttrCulling", DC_gmf_setAttrCulling, nullptr},
    {"setOrderTransparent", DC_gmf_setOrderTransparent, nullptr},
    {"setOrderLast", DC_gmf_setOrderLast, nullptr},
    {"setDistanceTextureRenderAttributes", DC_gmf_setDistanceTextureRenderAttributes, nullptr},
    {"setColour", gmf_DC_setColour_drawList, nullptr},
    {"setColourRef", gmf_DC_setColourRef, nullptr},
    {"drawTextureByHandle", DC_gmf_drawTextureByHandle, nullptr},
    {"setTextureByHandle", DC_gmf_setTextureByHandle, nullptr},
    {"clearTexture", DC_gmf_clearTexture, nullptr},
    {"setTextureSubImg", DC_gmf_setTextureSubImg, nullptr},
    {"drawTexture", DC_gmf_drawTexture, nullptr},
    {"drawAnimation", DC_gmf_drawAnimation, nullptr},
    {"setFontRenderAttributes", DC_gmf_setFontRenderAttributes, nullptr},
    {"resetFontRenderAttributes", DC_gmf_resetFontRenderAttributes, nullptr},
    {"print", DC_gmf_print, nullptr},
    {"printUntranslated", DC_gmf_printUntranslated, nullptr},
    {"printArray", DC_gmf_printArray, nullptr},
    {"pushMatrix", gmf_DC_pushMatrix, nullptr},
    {"popMatrix", gmf_DC_popMatrix, nullptr},
    {"translate", gmf_DC_translate, nullptr},
    {"rotate", gmf_DC_rotate, nullptr},
    {"rotateZ", gmf_DC_rotate, nullptr}, // 2D renderer -- rotateZ is the only rotation axis that means anything
    {"scale", gmf_DC_scale, nullptr},
    {"clear", DC_gmf_clear, nullptr},
};

void GM_CDECL DrawListGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (DrawListData*)a_object->m_user;
}

} // namespace

gmType RegisterDrawListType(gmMachine& a_machine) {
    // Confirmed real retail type name (Ghidra, EBOOT.elf's native type
    // here, an invented name; only affects typeName()/exception text.
    g_drawListType = a_machine.CreateUserType("VMDRAWLIST");
    a_machine.RegisterTypeLibrary(g_drawListType, s_drawListLib,
                                   sizeof(s_drawListLib) / sizeof(s_drawListLib[0]));
    a_machine.RegisterUserCallbacks(g_drawListType, nullptr, DrawListGCDestruct, nullptr);
    return g_drawListType;
}

gmType GetDrawListType() { return g_drawListType; }

void PushNewDrawList(gmThread* a_thread, int a_width, int a_height) {
    DrawListData* data = new DrawListData();
    data->width = a_width;
    data->height = a_height;
    a_thread->PushNewUser(data, g_drawListType);
}

bool SetDrawListFontRenderAttributes(gmUserObject* a_drawListObj, float a_edge, float a_width) {
    if (!a_drawListObj || a_drawListObj->m_userType != g_drawListType) return false;
    GetDrawContext().fontEdge = a_edge;
    GetDrawContext().fontWidth = a_width;
    return true;
}

void DrawAllPhysicsBodies(const std::vector<int>& a_nativeDrawnUids) {
    DrawPhysicsBodiesImmediate(a_nativeDrawnUids);
}
