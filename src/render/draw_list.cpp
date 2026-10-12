#include "draw_list.h"
#include "draw_context.h"
#include "renderer.h"
#include "../core/engine_context.h"
#include "../core/physics_world.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/shape_bindings.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/animation_bindings.h"
#include "../bindings/colour_bindings.h"
#include "../core/strict_calls.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <box2d/box2d.h>
#include <GL/glew.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <cstdint>

namespace {

gmType g_drawListType = -1;

} // namespace

DrawListData* RecordingTarget(gmThread* a_thread) {
    gmUserObject* obj = a_thread->ThisUserObject();
    if (obj && obj->m_userType == g_drawListType) {
        auto* data = static_cast<DrawListData*>(obj->m_user);
        return data && data->commandBuffer ? data : nullptr;
    }
    return nullptr;
}

void ReplayDrawList(DrawListData* a_data) {
    if (a_data) ReplayDrawList(a_data->commandBuffer);
}

void ReplayDrawList(const std::shared_ptr<DrawListData::CommandBuffer>& a_buffer) {
    if (!a_buffer) return;
    for (auto& cmd : a_buffer->commands) cmd();
}

DrawListSubmission CaptureDrawListSubmission(DrawListData* a_list,
                                             bool a_disableDepthTest) {
    DrawListSubmission submission;
    if (a_list) {
        submission.commandBuffer = a_list->commandBuffer;
        submission.orderKey = a_list->orderKey;
    }
    submission.disableDepthTest = a_disableDepthTest;
    submission.context = GetDrawContext();
    if (Renderer* renderer = GetEngineContext().renderer)
        renderer->GetColour(submission.colour[0], submission.colour[1],
                            submission.colour[2], submission.colour[3]);
    return submission;
}

void ReplayDrawListsInOrder(std::vector<DrawListSubmission>& a_lists) {
    // Retail FUN_00046a34 is the comparator passed to the native
    // FUN_0030d488 sort: unsigned(listA+0x30) < unsigned(listB+0x30).
    std::sort(a_lists.begin(), a_lists.end(), [](const DrawListSubmission& a,
                                                  const DrawListSubmission& b) {
        if (!a.commandBuffer) return static_cast<bool>(b.commandBuffer);
        if (!b.commandBuffer) return false;
        return a.orderKey < b.orderKey;
    });
    Renderer* renderer = GetEngineContext().renderer;
    for (const DrawListSubmission& submission : a_lists) {
        if (!submission.commandBuffer) continue;
        DrawContext& context = GetDrawContext();
        context = submission.context;
        if (renderer) {
            renderer->SetColour(submission.colour[0], submission.colour[1],
                                submission.colour[2], submission.colour[3]);
            renderer->SetDepthTest(context.depthTest);
            renderer->SetDepthWrite(context.depthWrite);
            renderer->SetColourMask(context.colourMask);
            renderer->SetStencilFunc(context.stencilFunc, context.stencilRef);
            renderer->SetStencilOp(context.stencilSfail, context.stencilDpfail,
                                   context.stencilDppass);
            renderer->SetCulling(context.culling);
        }
        if (submission.disableDepthTest && renderer) renderer->SetDepthTest(false);
        ReplayDrawList(submission.commandBuffer);
        if (submission.disableDepthTest && renderer) renderer->SetDepthTest(true);
    }
    a_lists.clear();
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
    if (!d || !d->commandBuffer) return GM_EXCEPTION;
    // reset() is the real "clear me, I'm about to be rebuilt from scratch
    // this frame" signal -- always executes immediately (never itself
    // deferred/recorded), same as callDrawList's own administrative role.
    d->commandBuffer->commands.clear();
    d->commandBuffer->retainedObjects.clear();
    d->commandBuffer->retainedBuffers.clear();
    DrawContext& ctx = GetDrawContext();
    ctx.stack.assign(1, DrawContext::Mat{});
    ctx.attrStack.clear();
    // A cleared command stream starts from the device's real baseline:
    // depth comparison enabled (LEQUAL) and writes enabled. Resetting this
    // shadow state to false made a later balanced pushAttributes/popAttributes
    // restore GL_ALWAYS, silently disabling Z occlusion for the rest of the
    // entity pass. That let later background sprites cover nearer geometry
    // (for example clouds over MarshmallowTree stems).
    ctx.depthTest = true;
    ctx.depthWrite = true;
    // SetColourMask(false) means colour writes are enabled in this renderer.
    ctx.colourMask = false;
    ctx.stencilFunc = 8; // VIDEOPARAMS_DISABLE
    ctx.stencilRef = 0;
    ctx.stencilSfail = ctx.stencilDpfail = ctx.stencilDppass = 100; // VIDEOPARAMS_KEEP
    ctx.fontEdge = 0.5f;
    ctx.fontWidth = 0.45f;
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

// Bytecode verified against VMDRAWLIST.releaseReference at EBOOT 0x7bd54.
// It decrements the refcounted native command buffer at wrapper offset +0x14,
// destroys/frees the buffer at zero, then clears that pointer. callDrawList
// holds an independent reference to the same buffer, so a recorded nested
// list remains replayable after its original wrapper releases ownership.
int GM_CDECL gmf_releaseReference(gmThread* a_thread) {
    auto* data = static_cast<DrawListData*>(
        a_thread->ThisUserCheckType(g_drawListType));
    if (!data) return GM_EXCEPTION;
    data->commandBuffer.reset();
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
// Colour categories match the retail bit values in constants_bindings.cpp:
// PLAYER_COLLISION_CATEGORY (1<<2) and GROUND_COLLISION_CATEGORY (1<<0).
// Everything else (checkpoints, drones, decorations, ...) gets a neutral
// fallback colour.
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
                // Solid/near-opaque means the body was explicitly made
                // static. Retail-created zero-density bodies remain dynamic.
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
        // half-height extent -- see Renderer::GetWorldHalfHeight().
        float halfH = renderer->GetWorldHalfHeight() * 1.05f; // slight margin so edges never peek through
        float halfW = renderer->GetWorldHalfWidth() * 1.05f;
        float quad[8] = {
            cx - halfW, cy - halfH,
            cx + halfW, cy - halfH,
            cx + halfW, cy + halfH,
            cx - halfW, cy + halfH,
        };
        // The base colour wash must not participate in depth. It is drawn
        // first, and its job is "fill the visible area with the current
        // colour" -- everything else in the frame belongs in front of it,
        // whatever z that content carries.
        //
        // This quad has no z of its own, so it lands at z=0. Once depth
        // testing was enabled (see ground_renderer.cpp / draw_context.h),
        // that silently hid every piece of world content BEHIND z=0 --
        // most visibly all tutorial text, which TutorialMessage.gm draws at
        // PLAYER_MODE_NOTE_Z + ZINCER*3 = -19.91. The text was being
        // recorded and drawn correctly the whole time (12307 print calls
        // per run, at correct world positions and correct z); this quad was
        // painting over it.
        //
        // Disabling both test and write, rather than picking some far z,
        // avoids inventing a depth value retail does not have: the
        // drawStaticSurface opcode (0x000801cc) takes no arguments at all.
        renderer->SetDepthTest(false);
        renderer->SetDepthWrite(false);
        renderer->DrawPolygon(GL_TRIANGLE_FAN, quad, 4);
        renderer->SetDepthWrite(true);
        renderer->SetDepthTest(true);
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

// callDrawList(other) -- bytecode verified at EBOOT 0x80788. Retail increments
// the called DrawList's native command-buffer reference, then stores that
// reference in the caller's opcode stream. The shared command buffer below
// preserves that ownership after the source wrapper calls releaseReference.
// Every real call site (QuadShadowed.gm:50's
// `drawList.callDrawList(this.shadowDrawList)`, EntityMenu_*Cell.gm's
// `parentDrawList.callDrawList(this.drawList)`, ...) agree unambiguously on
// the contract: replay another drawList's recorded commands into this one.
// Called through a real drawList receiver itself (never bare `Video.
// callDrawList(...)` anywhere in the corpus), so it follows the same
// record-vs-execute rule as everything else here.
int GM_CDECL gmf_callDrawList(gmThread* a_thread) {
    gmUserObject* obj = a_thread->ParamUserObject(0);
    if (!obj || obj->m_userType != g_drawListType) return GM_OK; // tolerant-null, same convention as drawShape
    DrawListData* target = (DrawListData*)obj->m_user;
    if (DrawListData* rec = RecordingTarget(a_thread)) {
        auto sourceBuffer = target ? target->commandBuffer : nullptr;
        if (sourceBuffer) {
            rec->commandBuffer->retainedBuffers.push_back(sourceBuffer);
            rec->commandBuffer->commands.push_back(
                [sourceBuffer] { ReplayDrawList(sourceBuffer); });
        }
    } else {
        ReplayDrawList(target);
    }
    return GM_OK;
}

// Retail 0x000809d8 accepts (shaderOrEntity, index, value) and
// (entity, slot, index, value). Its last argument is converted to a float
// and a four-byte parameter command is appended to this draw list. Apply
// that command during replay, before the following drawShape commands.
int GM_CDECL gmf_setVertexShaderParam(gmThread* a_thread) {
    const int argc = a_thread->GetNumParams();
    if (argc != 3 && argc != 4) return GM_EXCEPTION;
    const gmVariable target = a_thread->Param(0);
    gmMachine& machine = *a_thread->GetMachine();
    int entityId = 0;
    if (target.m_type == GM_TABLE) {
        gmTableObject* entity = (gmTableObject*)machine.GetObject(target.m_value.m_ref);
        const gmVariable objectId = entity->Get(&machine, "__objectID");
        if (objectId.m_type != GM_INT) return GM_EXCEPTION;
        entityId = objectId.m_value.m_int;
    } else if (argc != 3 || !GetShaderStorageId(machine, target, entityId)) {
        return GM_EXCEPTION;
    }
    const gmVariable slotArg = argc == 4 ? a_thread->Param(1) : gmVariable((gmint)0);
    const gmVariable indexArg = a_thread->Param(argc - 2);
    const gmVariable valueArg = a_thread->Param(argc - 1);
    if (slotArg.m_type != GM_INT || indexArg.m_type != GM_INT ||
        (valueArg.m_type != GM_FLOAT && valueArg.m_type != GM_INT)) return GM_EXCEPTION;
    // The retail four-argument wrapper routes negative slots to slot zero.
    const int slot = std::max(0, (int)slotArg.m_value.m_int);
    const int index = indexArg.m_value.m_int;
    const float value = valueArg.m_type == GM_FLOAT ? valueArg.m_value.m_float : (float)valueArg.m_value.m_int;
    return DC_RecordOrRun(a_thread, [entityId, slot, index, value] {
        SetEntityShaderFloat(entityId, slot, index, value);
    });
}

// Bytecode verified: retail VMDRAWLIST.setAttrAlphaBlend at EBOOT 0x7e644
// is `li r3,-1; blr`, so every call returns the native invalid-result value.
int GM_CDECL gmf_setAttrAlphaBlend(gmThread*) { return -1; }
// Bytecode verified: retail VMDRAWLIST.setStencil/setDefaultStencil at
// EBOOT 0x804c8/0x80710 are each `li r3,-1; blr`.
int GM_CDECL gmf_invalidStencilSetter(gmThread*) { return -1; }
// Bytecode verified: retail VMDRAWLIST.nop appends {opcode=0,arg=0}; the
// retail draw-list dispatcher skips opcodes 0 and 1 without side effects.
int GM_CDECL gmf_nop(gmThread* a_thread) {
    DrawListData* data = RecordingTarget(a_thread);
    if (!data) return GM_EXCEPTION;
    data->commandBuffer->commands.emplace_back([] {});
    return GM_OK;
}

struct RetailSplitGlyph {
    uint32_t codepoint;
    size_t byteStart;
    size_t byteEnd;
    float advance;
};

// `FUN_00044074` derives UTF-8 length from the leading byte, accepts legacy
// five/six-byte forms, and does not validate continuation bytes. Bound reads
// to the desktop string allocation while preserving its decoded value rules.
uint32_t DecodeSplitGlyph(const std::string& a_text, size_t& a_offset) {
    const uint8_t lead = static_cast<uint8_t>(a_text[a_offset++]);
    const size_t length = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 :
                          lead < 0xf8 ? 4 : lead < 0xfc ? 5 : 6;
    const uint8_t masks[] = {0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
    uint32_t codepoint = lead & static_cast<uint8_t>(~masks[length - 1]);
    for (size_t i = 1; i < length; ++i) {
        const uint8_t next = a_offset < a_text.size()
            ? static_cast<uint8_t>(a_text[a_offset]) : 0;
        if (a_offset < a_text.size()) ++a_offset;
        codepoint = (codepoint << 6) | (next & 0x3f);
    }
    return codepoint;
}

bool IsRetailSplitOpportunity(uint32_t a_cp) {
    switch (a_cp) {
    case 0x09: case 0x20:
    case 0x3001: case 0x3002: case 0x30fb: case 0x30fc:
    case 0xff01: case 0xff09: case 0xff0c: case 0xff1a: case 0xff1f:
    case 0x29:
        return true;
    default:
        return false;
    }
}

struct RetailTextRange { size_t start; size_t length; };

// Translates the line-boundary state in retail FUN_00040e90. Its output
// ranges index UTF-8 bytes (not Unicode scalar values), as the caller uses
// them in STRING.Mid(). Glyph advances are the retail font records' +8 value
// multiplied by the supplied scale. The comparison deliberately retains the
// retail strict '<' boundary and its empty range for an over-wide first glyph.
std::vector<RetailTextRange> SplitTextLikeRetail(const BitmapFont* a_font,
                                                  const std::string& a_text,
                                                  float a_width, float a_scale) {
    std::vector<RetailTextRange> ranges;
    std::vector<RetailSplitGlyph> glyphs;
    size_t offset = 0;
    float fullWidth = 0.0f;
    while (offset < a_text.size()) {
        const size_t start = offset;
        const uint32_t cp = DecodeSplitGlyph(a_text, offset);
        const float advance = a_font ? a_font->GetRetailGlyphAdvance(cp) : 0.0f;
        const float scaled = static_cast<float>(static_cast<double>(advance) * a_scale);
        glyphs.push_back({cp, start, offset, scaled});
        fullWidth = static_cast<float>(static_cast<double>(fullWidth) + scaled);
    }
    if (glyphs.empty()) return ranges;

    // Retail helper's initial width and threshold constants are both 0.0f.
    if (a_width == 0.0f || fullWidth < a_width) {
        ranges.push_back({0, a_text.size()});
        return ranges;
    }

    size_t lineStartByte = 0;
    size_t lastBreakByte = 0;
    size_t lastBreakGlyph = 0;
    size_t lastEmittedGlyphEnd = 0;
    float committedWidth = 0.0f;
    float wordWidth = 0.0f;

    for (size_t i = 0; i < glyphs.size(); ++i) {
        const RetailSplitGlyph& glyph = glyphs[i];
        if (glyph.codepoint == 0x0a || glyph.codepoint == 0x0d || glyph.codepoint == 0x5e) {
            ranges.push_back({lineStartByte, glyph.byteStart - lineStartByte});
            lineStartByte = glyph.byteEnd;
            lastBreakByte = 0;
            lastBreakGlyph = 0;
            committedWidth = 0.0f;
            wordWidth = 0.0f;
            lastEmittedGlyphEnd = i + 1;
            if (glyph.codepoint == 0x0d && i + 1 < glyphs.size() &&
                glyphs[i + 1].codepoint == 0x0a) {
                lineStartByte = glyphs[i + 1].byteEnd;
                ++i;
                lastEmittedGlyphEnd = i + 1;
            }
            continue;
        }

        const float candidateWidth = static_cast<float>(
            static_cast<double>(committedWidth) + wordWidth + glyph.advance);
        if (a_width > 0.0f && a_width < candidateWidth) {
            if (committedWidth != 0.0f && lastBreakByte >= lineStartByte) {
                ranges.push_back({lineStartByte, lastBreakByte - lineStartByte});
                lineStartByte = lastBreakByte;
                lastEmittedGlyphEnd = lastBreakGlyph;
            } else {
                ranges.push_back({lineStartByte, glyph.byteStart - lineStartByte});
                lineStartByte = glyph.byteStart;
                lastEmittedGlyphEnd = i;
                committedWidth = 0.0f;
                wordWidth = 0.0f;
            }
            lastBreakByte = 0;
            lastBreakGlyph = 0;
            committedWidth = 0.0f;
        }

        if (IsRetailSplitOpportunity(glyph.codepoint)) {
            committedWidth = static_cast<float>(
                static_cast<double>(committedWidth) + wordWidth + glyph.advance);
            wordWidth = 0.0f;
            lastBreakByte = glyph.byteEnd;
            lastBreakGlyph = i + 1;
        } else {
            wordWidth = static_cast<float>(static_cast<double>(wordWidth) + glyph.advance);
        }
    }

    // Retail compares the last emitted codepoint position to UTF-8 byte
    // length here; preserve that quirk for multibyte strings and trailing
    // explicit separators.
    if (lastEmittedGlyphEnd < a_text.size()) {
        ranges.push_back({lineStartByte, a_text.size() - lineStartByte});
    }
    return ranges;
}

// Retail bytecode: splitString@0x81e38 and splitStringMidWord@0x821e0 have
// identical argument dispatch/return construction, including zero float
// defaults. Both call FUN_00040e90; the helper's line state is translated
// below from its PPC-derived Ghidra body, including its byte-offset quirks.
int GM_CDECL gmf_splitStringRetail(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    const gmVariable& leftArg = count > 0 ? a_thread->Param(0) : gmVariable();
    const gmVariable& rightArg = count > 1 ? a_thread->Param(1) : gmVariable();
    const gmVariable& textArg = count > 2 ? a_thread->Param(2) : gmVariable();
    const gmVariable& scaleArg = count > 3 ? a_thread->Param(3) : gmVariable();

    // Retail only consumes float tags for bounds/scale and only a string tag
    // for text; other provided types leave the corresponding zero/empty
    // default intact rather than coercing or rejecting the call.
    const float left = leftArg.m_type == GM_FLOAT ? leftArg.m_value.m_float : 0.0f;
    const float right = rightArg.m_type == GM_FLOAT ? rightArg.m_value.m_float : 0.0f;
    const float scale = scaleArg.m_type == GM_FLOAT ? scaleArg.m_value.m_float : 0.0f;
    std::string text;
    if (textArg.m_type == GM_STRING) {
        gmStringObject* object = static_cast<gmStringObject*>(
            a_thread->GetMachine()->GetObject(textArg.m_value.m_ref));
        if (object) text = object->GetString();
    }

    const auto ranges = SplitTextLikeRetail(GetEngineContext().currentFont,
                                             text, right - left, scale);
    gmMachine* machine = a_thread->GetMachine();
    gmTableObject* result = machine->AllocTableObject();
    for (size_t i = 0; i < ranges.size(); ++i) {
        gmTableObject* range = machine->AllocTableObject();
        range->Set(machine, "startIndex", gmVariable(static_cast<gmint>(ranges[i].start)));
        range->Set(machine, "length", gmVariable(static_cast<gmint>(ranges[i].length)));
        result->Set(machine, static_cast<int>(i), gmVariable(range));
    }
    a_thread->PushTable(result);
    return GM_OK;
}

// Retail exposes these shader methods, but the portable renderer has no
// equivalent program/parameter backend yet. Keep them bound so strict survey
// reports real call sites instead of failing later as an unresolved native.
GE3_REMOVED(gmf_setFragmentShaderParam, "VMDRAWLIST.setFragmentShaderParam")
GE3_REMOVED(gmf_hackSetFragmentShaderParam, "VMDRAWLIST.hack_setFragmentShaderParam")

gmFunctionEntry s_drawListLib[] = {
    {"reset", gmf_reset, nullptr},
    {"releaseReference", gmf_releaseReference, nullptr},
    {"nop", gmf_nop, nullptr},
    {"setDebugID", gmf_setDebugID, nullptr},
    {"callDrawList", gmf_callDrawList, nullptr},
    {"drawStaticSurface", gmf_drawStaticSurface, nullptr},
    {"drawShape", DC_gmf_drawShape, nullptr},
    {"setVertexShaderParam", gmf_setVertexShaderParam, nullptr},
    {"setFragmentShaderParam", gmf_setFragmentShaderParam, nullptr},
    {"hack_setFragmentShaderParam", gmf_hackSetFragmentShaderParam, nullptr},
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
    // Bytecode verified: retail PPC at EBOOT 0x7e644 is `li r3,-1; blr`.
    {"setAttrAlphaBlend", gmf_setAttrAlphaBlend, nullptr},
    {"setStencil", gmf_invalidStencilSetter, nullptr},
    {"setDefaultStencil", gmf_invalidStencilSetter, nullptr},
    {"setColourMask", DC_gmf_setColourMask, nullptr},
    {"setAttrStencil", DC_gmf_setAttrStencil, nullptr},
    {"setAttrDefaultStencil", DC_gmf_setAttrDefaultStencil, nullptr},
    {"setAttrCulling", DC_gmf_setAttrCulling, nullptr},
    {"setOrderTransparent", DC_gmf_setOrderTransparent, nullptr},
    {"setOrderAlphaTest", DC_gmf_setOrderAlphaTest, nullptr},
    {"setOrderLast", DC_gmf_setOrderLast, nullptr},
    {"setDistanceTextureRenderAttributes", DC_gmf_setDistanceTextureRenderAttributes, nullptr},
    {"setColour", gmf_DC_setColour_drawList, nullptr},
    {"setColourHSL", DrawList_SetColourHSL, nullptr},
    {"setColourWithLightnessShift", DrawList_SetColourWithLightnessShift, nullptr},
    {"setInterpolatedColour", DrawList_SetInterpolatedColour, nullptr},
    {"setColourRef", gmf_DC_setColourRef, nullptr},
    {"drawModTexture", DC_gmf_drawModTexture, nullptr},
    {"drawTextureByHandle", DC_gmf_drawTextureByHandle, nullptr},
    {"setTexture", DC_gmf_setTexture, nullptr},
    {"setTextureByHandle", DC_gmf_setTextureByHandle, nullptr},
    {"clearTexture", DC_gmf_clearTexture, nullptr},
    {"setTextureSubImg", DC_gmf_setTextureSubImg, nullptr},
    {"drawTexture", DC_gmf_drawTexture, nullptr},
    {"drawAnimation", DC_gmf_drawAnimation, nullptr},
    {"setFontRenderAttributes", DC_gmf_setFontRenderAttributes, nullptr},
    {"resetFontRenderAttributes", DC_gmf_resetFontRenderAttributes, nullptr},
    {"print", DC_gmf_print, nullptr},
    {"printTranslated", DC_gmf_printTranslated, nullptr},
    {"printUntranslated", DC_gmf_printUntranslated, nullptr},
    {"printArray", DC_gmf_printArray, nullptr},
    {"splitString", gmf_splitStringRetail, nullptr},
    {"splitStringMidWord", gmf_splitStringRetail, nullptr},
    {"pushMatrix", gmf_DC_pushMatrix, nullptr},
    {"popMatrix", gmf_DC_popMatrix, nullptr},
    {"translate", gmf_DC_translate, nullptr},
    {"rotate", gmf_DC_rotate, nullptr},
    {"rotateZ", gmf_DC_rotate, nullptr}, // 2D renderer -- rotateZ is the only rotation axis that means anything
    {"scale", gmf_DC_scale, nullptr},
    {"multMatrix", gmf_DC_multMatrix, nullptr},
    {"clear", DC_gmf_clear, nullptr},
};

void GM_CDECL DrawListGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (DrawListData*)a_object->m_user;
}

#if GM_USE_INCGC
bool GM_CDECL DrawListGCTrace(gmMachine* a_machine, gmUserObject* a_object,
                              gmGarbageCollector* a_gc, const int a_workLeftToGo,
                              int& a_workDone) {
    auto* data = static_cast<DrawListData*>(a_object->m_user);
    if (data && data->commandBuffer) {
        using Buffer = DrawListData::CommandBuffer;
        std::vector<std::shared_ptr<Buffer>> pending{data->commandBuffer};
        std::unordered_set<Buffer*> visited;
        while (!pending.empty()) {
            auto buffer = std::move(pending.back());
            pending.pop_back();
            if (!buffer || !visited.insert(buffer.get()).second) continue;
            for (gmObject* object : buffer->retainedObjects) {
                if (!object) continue;
                a_gc->GetNextObject(object);
                ++a_workDone;
            }
            pending.insert(pending.end(), buffer->retainedBuffers.begin(),
                           buffer->retainedBuffers.end());
        }
    }
    ++a_workDone;
    return true;
}
#else
void GM_CDECL DrawListGCMark(gmMachine* a_machine, gmUserObject* a_object, gmuint32 a_mark) {
    auto* data = static_cast<DrawListData*>(a_object->m_user);
    if (!data || !data->commandBuffer) return;
    using Buffer = DrawListData::CommandBuffer;
    std::vector<std::shared_ptr<Buffer>> pending{data->commandBuffer};
    std::unordered_set<Buffer*> visited;
    while (!pending.empty()) {
        auto buffer = std::move(pending.back());
        pending.pop_back();
        if (!buffer || !visited.insert(buffer.get()).second) continue;
        for (gmObject* object : buffer->retainedObjects) {
            if (object && object->NeedsMark(a_mark)) object->Mark(a_machine, a_mark);
        }
        pending.insert(pending.end(), buffer->retainedBuffers.begin(),
                       buffer->retainedBuffers.end());
    }
}
#endif

} // namespace

gmType RegisterDrawListType(gmMachine& a_machine) {
    // Confirmed real retail type name (Ghidra, EBOOT.elf's native type
    // here, an invented name; only affects typeName()/exception text.
    g_drawListType = a_machine.CreateUserType("VMDRAWLIST");
    a_machine.RegisterTypeLibrary(g_drawListType, s_drawListLib,
                                   sizeof(s_drawListLib) / sizeof(s_drawListLib[0]));
#if GM_USE_INCGC
    a_machine.RegisterUserCallbacks(g_drawListType, DrawListGCTrace, DrawListGCDestruct, nullptr);
#else
    a_machine.RegisterUserCallbacks(g_drawListType, DrawListGCMark, DrawListGCDestruct, nullptr);
#endif
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
    GetDrawContext().fontEdge = a_edge < 0.0f ? 0.25f : 0.5f;
    // The native helper adds widened doubles, rounds the sum to float, then
    // multiplies by its single-precision 0.5 constant.
    GetDrawContext().fontWidth = static_cast<float>(
        static_cast<double>(a_width) + std::fabs(static_cast<double>(a_edge))) * 0.5f;
    return true;
}

void DrawAllPhysicsBodies(const std::vector<int>& a_nativeDrawnUids) {
    DrawPhysicsBodiesImmediate(a_nativeDrawnUids);
}
