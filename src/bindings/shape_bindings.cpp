#include "gmThread.h"
#include "shape_bindings.h"
#include "vector2d_bindings.h"
#include "../render/draw_context.h"
#include "../render/draw_list.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "texture_bindings.h"
#include <GL/glew.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace { struct ShapeData; }
void DrawShapeBoundingBoxTextured(const ShapeData* s, unsigned int handle);

namespace {

gmType g_shapeType = -1;

// RETAIL MATCH (retail beginShape, FUN_00421810): these integer values and
// their GL primitive mapping are read directly off retail's own switch:
//
//   case 0 -> 7 (GL_QUADS)          case 4 -> 1 (GL_LINES)      + line flag
//   case 1 -> 4 (GL_TRIANGLES)      case 5 -> 3 (GL_LINE_STRIP) + line flag
//   case 2 -> 6 (GL_TRIANGLE_FAN)   case 6 -> 2 (GL_LINE_LOOP)  + line flag
//   case 3 -> 5 (GL_TRIANGLE_STRIP)
//
// TriangleFan and TriangleStrip were previously SWAPPED relative to retail
// here (Strip=2, Fan=3). That was internally self-consistent -- ToGLMode
// mapped this file's own Fan to GL_TRIANGLE_FAN, so scripts referencing
// SHAPETYPE_TRIANGLEFAN by name still rendered a fan -- but the numeric
// values did not match retail's, which would silently render the wrong
// primitive for any content that carries a raw shape-type integer rather
// than the named constant (e.g. original .gmc bytecode with the value
// baked in, or a data file).
enum class ShapeType {
    Quads = 0,
    Triangles = 1,
    TriangleFan = 2,
    TriangleStrip = 3,
    Lines = 4,
    LineStrip = 5,
    LineLoop = 6,
    Circle = 7,
};

GLenum ToGLMode(int a_shapeType) {
    switch ((ShapeType)a_shapeType) {
        // Real GL_QUADS, matching retail. This used to fall back to
        // GL_TRIANGLE_FAN because the old core-profile renderer had no
        // GL_QUADS; the compatibility-profile context does. The two are only
        // equivalent for exactly 4 vertices -- beyond that GL_QUADS draws
        // independent quads per 4 vertices while a fan radiates from vertex
        // 0, so this was a real divergence for any longer quad list.
        case ShapeType::Quads: return GL_QUADS;
        case ShapeType::Triangles: return GL_TRIANGLES;
        case ShapeType::TriangleFan: return GL_TRIANGLE_FAN;
        case ShapeType::TriangleStrip: return GL_TRIANGLE_STRIP;
        case ShapeType::Lines: return GL_LINES;
        case ShapeType::LineStrip: return GL_LINE_STRIP;
        case ShapeType::LineLoop: return GL_LINE_LOOP;
        case ShapeType::Circle: return GL_TRIANGLE_FAN; // no retail case; unused by the corpus
        default: return GL_TRIANGLE_FAN;
    }
}

struct ShapeData {
    GLenum glMode = GL_TRIANGLE_FAN;
    // xy pairs, LOCAL space (i.e. relative to whatever transform is active
    // at the eventual draw call -- see header comment update below for why
    // this changed from world-space/pre-transformed).
    std::vector<float> verts;
    // Per-vertex rgba, aligned 1:1 with verts (4 floats per xy pair) -- the
    // colour Video.setColour() had set at the moment each vertex was pushed
    // (PlotVertexOn/gmf_plotVertices), not a single shape-wide tint. Needed
    // for FoE's radial-gradient beginShape/plotVertex/endShape idiom (see
    // Renderer::DrawPolygonGradient's header comment) -- setAsFilledCircle's
    // precomputed ring never populates this (no colour involved building
    // it), only plotVertex/plotVertices do.
    std::vector<float> colors;
    // Per-vertex uv, aligned 1:1 with verts -- populated by getVertex(i)'s
    // live SHAPEVERTEX handle (PekkoAndJon/bg.gm's update(): `this.
    // shapePoints[i].vertex.u = 0.5 + cos(ang)*this.amt;` etc, animating the
    // UV radius each frame to "zoom" the sampled texture outward). Without
    // real per-vertex UVs, texturing falls back to the bounding-box
    // approximation (DrawShapeBoundingBoxTextured/DrawPolygonGradient's own
    // auto-generated UVs), which is a flat single-texture-copy stretch, not
    // the animated radial sampling that produces bg.gm's kaleidoscope look.
    std::vector<float> uvs;
    float localZ = 0.0f; // real usage draws one flat Z per shape, not per-vertex
    // True right after a completed draw (endShape or drawShape), until the
    // next plotVertex call -- see gmf_plotVertex's comment.
    bool needsClearOnNextPlot = false;

    // SHAPE(shapeType, numVerts, owner)'s numVerts -- confirmed real via
    // game_data/VideoHelper.gm's `SHAPE(SHAPETYPE_TRIANGLEFAN,61+1,GameSystem)`
    // (see gmf_setAsFilledCircle's header comment for the full derivation).
    // Only setAsFilledCircle/getQuality actually consume this; a plain
    // beginShape()-built shape (capacity never set) leaves it at 0 and grows
    // verts freely via plotVertex, same as before this field existed.
    int capacity = 0;
};

ShapeData* ThisShape(gmThread* a_thread) {
    return (ShapeData*)a_thread->ThisUserCheckType(g_shapeType);
}

// The shape most recently opened by Video.beginShape/drawList.beginShape.
// Still referenced by gmf_plotVertices (setAsFilledCircle's family, kept --
// see this file's header) even though beginShape itself is now routed
// through the strict-call trap (gmf_Shape_beginShape, below) and no longer
// actually sets this: until beginShape is reimplemented for real, this
// stays permanently null, and plotVertices() is consequently a silent no-op
// (its own `if (!g_currentShape) return GM_OK;` guard) rather than a crash
// -- draw()/setAsFilledCircle() themselves are unaffected, since neither
// touches g_currentShape at all.
ShapeData* g_currentShape = nullptr;

// FoE's beginShape/plotVertex/endShape triangle-fan background pattern
// (PekkoAndJon/bg.gm's radial backdrop: `Video.beginTexture(path);
// Video.beginShape(SHAPETYPE_TRIANGLEFAN); ...; Video.endShape();
// Video.endTexture();`) wraps its shape draw in a beginTexture/endTexture
// scope expecting the texture to actually paint -- same currentTextureHandle
// DrawContext already threads through DC_gmf_drawShape's drawList.drawShape()
// path (see that function's own header comment), just not previously
// consulted here. Without this the whole textured backdrop silently fell
// back to DrawPolygon's flat-colour fill, which is why the FoE title-screen
// background under this triangle fan was reduced to a near-black wash --
// the texture that was meant to show through the colour gradient never
// bound to anything.
void DrawShapeData(ShapeData* s) {
    int count = (int)(s->verts.size() / 2);
    if (count <= 0) return;
    DrawContext& ctx = GetDrawContext();
    static std::vector<float> transformed;
    transformed.resize(s->verts.size());
    for (int i = 0; i < count; ++i) {
        DC_TransformPoint(ctx.Top(), s->verts[i * 2], s->verts[i * 2 + 1], transformed[i * 2], transformed[i * 2 + 1]);
    }
    unsigned int tex = GetDrawContext().currentTextureHandle;
    if (std::getenv("GE3_SHAPE_TEX_DEBUG")) {
        static int s_t = 0;
        if (s_t < 16) { ++s_t;
            std::fprintf(stderr, "[TEXDBG] DrawShapeData n=%d tex=%u hasUVs=%d hasColours=%d\n",
                         count, tex, (int)(s->uvs.size() == (size_t)count * 2),
                         (int)(s->colors.size() == (size_t)count * 4));
        }
    }
    // Per-vertex colours (set via plotVertex/plotVertices, see ShapeData::
    // colors' header comment) take the real-fan gradient path instead of
    // DrawShapeBoundingBoxTextured's box-collapse approximation, which would
    // both flatten a large radial backdrop into a visible rectangle AND
    // discard the gradient entirely (a single uColour uniform, no per-vertex
    // varying) -- draw()/setAsFilledCircle()'s own un-gradiented shapes
    // (colors left empty) still fall through to the plain untextured path.
    if ((int)s->colors.size() == count * 4) {
        const float* realUv = ((int)s->uvs.size() == count * 2) ? s->uvs.data() : nullptr;
        // GE3_UV_DEBUG=1 dumps per-layer geometry/UV state for bg.gm's fan
        // (vertex count, whether real per-vertex UVs survived, UV radius,
        // transformed centre, ring radius, Z, centre/rim alpha). This is
        // what caught the backdrop-position divergence traced to
        // MATHUTIL_CalculateExponentialGrowth -- kept (env-gated, zero cost
        // when unset) since static reading of this path had repeatedly
        // failed to find that class of bug.
        if (const char* dbgEnv = std::getenv("GE3_UV_DEBUG")) {
            static int s_dbg = 0;
            int atFrame = std::atoi(dbgEnv);
            int curFrame = GetEngineContext().gameSystem.currentTick;
            if ((atFrame <= 0 || curFrame == atFrame) && s_dbg < 8) {
                ++s_dbg;
                float cx = transformed[0], cy = transformed[1];
                float mnu = 0, mxu = 0;
                if (realUv) {
                    mnu = realUv[0]; mxu = realUv[0];
                    for (int i = 0; i < count; ++i) { mnu = std::min(mnu, realUv[i*2]); mxu = std::max(mxu, realUv[i*2]); }
                }
                float mnR = 1e9f, mxR = -1e9f;
                for (int i = 1; i < count; ++i) {
                    float rx = transformed[i*2] - cx, ry = transformed[i*2+1] - cy;
                    float r = std::sqrt(rx*rx + ry*ry);
                    mnR = std::min(mnR, r); mxR = std::max(mxR, r);
                }
                std::fprintf(stderr, "[UVDBG] n=%d uv=%s u=[%.3f,%.3f] centre=(%.0f,%.0f) R=[%.1f,%.1f] wobble=%.1f%% z=%.1f a0=%.2f aN=%.2f\n",
                             count, realUv ? "Y" : "N", mnu, mxu, cx, cy, mnR, mxR,
                             100.0f * (mxR - mnR) / ((mxR + mnR) * 0.5f),
                             ctx.Top().z + s->localZ, s->colors[3], s->colors[count*4 - 1]);
            }
        }
        GetEngineContext().renderer->DrawPolygonGradient(s->glMode, transformed.data(), s->colors.data(),
                                                           realUv, count, tex, ctx.Top().z + s->localZ);
        return;
    }
    if (tex != 0) {
        DrawShapeBoundingBoxTextured(s, tex);
        return;
    }
    GetEngineContext().renderer->DrawPolygon(s->glMode, transformed.data(), count, ctx.Top().z + s->localZ);
}

} // namespace

// beginShape -- DECOMPILED-CONFIRMED via live Ghidra cross-reference
// dump (function_entry_tables.txt, not string-position guessing) resolved
// the real body to FUN_00075c7c @ 0x00075c7c, arg-count dispatch:
//   1 arg  (shapeType):                    capacity defaults to 0x40 (64)
//   2 args (shapeType, capacity):           capacity explicit
//   3 args (shapeType, capacity, extra):    a 3rd int, meaning not traced
// -- all three forms allocate a NEW shape object via a native constructor
// helper (FUN_00047750, itself not further decompiled -- real refcounted
// GC-object allocation boilerplate, not shape-specific logic) and return
// it. That confirms the arg shape and "always allocates fresh" behavior;
// the internal allocator/refcounting isn't replicated (this engine's
// ShapeData is a plain C++ object under the VM's own GC hook, not a
// second parallel refcount scheme) -- consistent with this project's
// stated renderer principle of matching observable behavior, not internal
// implementation.
int GM_CDECL gmf_Shape_beginShape(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeType, 0, 0);
    ShapeData* data = new ShapeData();
    data->glMode = ToGLMode(shapeType);
    data->capacity = 0x40; // confirmed default (1-arg form), see header comment
    // Lenient float-or-int read, not the strict GM_INT_PARAM macro:
    // VideoHelper.gm's VIDEOHELPER_DrawFilledDonut can be called with one
    // extra leading arg (Platformer/MainMenu5/CampaignScreenRecordPlayer.
    // gm:1072's `VIDEOHELPER_DrawFilledDonut(t.grooveShape,0,0,0,t.r,t.r,
    // th,th,q)` against the function's 8 declared params), which shifts
    // every positional param by one -- this capacity arg ends up bound to
    // a FLOAT (a thickness value) rather than the int literal a normal
    // call site passes. GM_INT_PARAM's strict type check throws a blank-
    // message exception the instant that happens; reading it leniently
    // instead just uses that float's rounded value, which is a real
    // (if slightly-off-intent) capacity rather than a dead thread.
    if (a_thread->GetNumParams() >= 2) {
        data->capacity = (int)a_thread->ParamFloatOrInt(1, 0x40);
    }
    g_currentShape = data; // see g_currentShape's header comment
    a_thread->PushNewUser(data, g_shapeType);
    return GM_OK;
}

namespace {

// Global SHAPE(shapeType, numVerts, owner) -- confirmed real, distinct call
// from Video.beginShape despite superficially overlapping args (see
// Shape_gmf_beginShape above): allocates a persistent shape WITHOUT opening
// it for immediate plotVertex/endShape building, i.e. does not touch
// g_currentShape. Every real call site (VideoHelper.gm's
// `VIDEOHELPER_circleShape=SHAPE(SHAPETYPE_TRIANGLEFAN,61+1,GameSystem)`)
// builds it up afterward via setAsFilledCircle(), not plotVertex/endShape.
// `owner` (GameSystem in the one real call site) has no observable effect on
// anything the corpus reads back -- accepted and ignored, same convention as
// this codebase's other unrecoverable trailing-flag arguments (e.g.
// GameSystem.startNewProcessList's 2nd bool).
int GM_CDECL gmf_SHAPE(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeType, 0, 0);
    GM_INT_PARAM(numVerts, 1, 0);
    ShapeData* data = new ShapeData();
    data->glMode = ToGLMode(shapeType);
    data->capacity = numVerts;
    a_thread->PushNewUser(data, g_shapeType);
    return GM_OK;
}

// Shared by shape.plotVertex() (`s` = the receiver) and the bare
// Video.plotVertex()/drawList.plotVertex() form (`s` = g_currentShape) --
// see gmf_Shape_plotVertexCurrent below. Two real call conventions
// confirmed both by corpus usage and by FUN_0007b280's own dispatch (see
// gmf_VMSHAPE_plotVertex's header comment just below for the address):
// plotVertex(x, y[, z]) with separate numbers, and plotVertex(vector2D[, z])
// with a single vector2D and an optional z (VideoHelper.gm's
// VIDEOHELPER_DrawFilledPolygon/DrawPolygon: `retval.plotVertex(cpos, z)`,
// `retval.plotVertex(plist[j], z)`).
int PlotVertexOn(gmThread* a_thread, ShapeData* s) {
    GM_CHECK_NUM_PARAMS(1);
    float x = 0.0f, y = 0.0f;
    float localZ = 0.0f;
    gmVariable p0 = a_thread->Param(0);
    if (p0.m_type == g_vector2DType) {
        GetVector2D(*a_thread->GetMachine(), p0, x, y);
        if (a_thread->GetNumParams() >= 2) { GM_FLOAT_OR_INT_PARAM(zp, 1, 0.0f); localZ = zp; }
    } else {
        GM_CHECK_NUM_PARAMS(2);
        GM_FLOAT_OR_INT_PARAM(px, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(py, 1, 0.0f);
        x = px; y = py;
        // Optional z (stride>2 real usage) -- real shapes draw at one flat Z
        // (for the GL_EQUAL depth-layering system, see Renderer::SetDepthTest),
        // so last-vertex-wins is fine; u/v beyond that aren't modelled by this
        // untextured polygon renderer.
        if (a_thread->GetNumParams() >= 3) { GM_FLOAT_OR_INT_PARAM(zp, 2, 0.0f); localZ = zp; }
    }

    if (s->needsClearOnNextPlot) {
        s->verts.clear();
        s->colors.clear();
        s->uvs.clear();
        s->needsClearOnNextPlot = false;
    }
    s->verts.push_back(x);
    s->verts.push_back(y);
    float cr, cg, cb, ca;
    GetEngineContext().renderer->GetColour(cr, cg, cb, ca);
    s->colors.push_back(cr);
    s->colors.push_back(cg);
    s->colors.push_back(cb);
    s->colors.push_back(ca);
    // No real UV for a bare plotVertex(x,y[,z]) call -- 0.5,0.5 (texture
    // centre) is the one confirmed real value for this exact call shape
    // (PekkoAndJon/bg.gm's `Video.plotVertex(0,0)` centre point, matching
    // its OWN getVertex(0)'s independently-set u=0.5,v=0.5 -- see
    // ShapeVertexData's header comment) -- kept in lockstep with verts/
    // colors either way so DrawPolygonGradient's per-vertex-UV path (sized
    // against verts) stays valid even when this shape mixes a manual
    // plotVertex() centre with plotVertices()-copied ring UVs.
    s->uvs.push_back(0.5f);
    s->uvs.push_back(0.5f);
    s->localZ = localZ;
    return GM_OK;
}

// plotVertex/endShape -- DECOMPILED-CONFIRMED via live Ghidra
// gmFunctionEntry table dump (function_entry_tables.txt) turned up THREE
// separate registrations sharing these names -- a genuine surprise: Video's
// OWN plotVertex (FUN_00076fd8 @ 0x76fd8) real-writes only for exactly 1,
// 2, or 4 args and is a confirmed, deliberate no-op for exactly 3 or 5 --
// which would make VideoHelper.gm's own `Video.plotVertex(x1,y1,z)` (used
// by VIDEOHELPER_DrawFilledRect, one of the most-called helpers in the
// whole corpus) draw nothing at all. Cross-checked against
// solid-colour full-screen background rect DOES render in real retail --
// so whatever "Video" actually resolves to natively, it is NOT dispatching
// through that particular candidate for this call shape. The OTHER
// candidate (FUN_0007b280 @ 0x7b280, sitting in the table alongside
// `reset`/`getNumVertices`/`bindShader` -- i.e. VMDRAWLIST's own
// confirmed-real methods, draw_list.cpp) DOES real-write for 1, 2, and 3
// args (vector2D[,z] / numeric x,y[,z]), matching the retail-confirmed
// behavior. This engine's Video and drawList already share one underlying
// DrawContext by design (see draw_context.h's own header comment,
// "Video.* and drawList.* really are (mostly) the same underlying command
// list"), so this uses that VMDRAWLIST-table candidate's confirmed 1/2/3-
// arg dispatch for both -- the one demonstrably consistent with what the
// real game actually draws.
// endShape's Video-table candidate (FUN_000762b8) is confirmed a genuine
// literal no-op (`return 0;`) -- the real flush must happen elsewhere in
// retail's deferred/buffered pipeline. This engine's renderer is immediate-
// mode by design (draws happen synchronously, not via a replayed command
// stream), so endShape staying the trigger point for the actual GL draw
// call is the direct translation of that design choice, not a guess at
// retail's internal mechanism -- matches this project's stated renderer
// principle (docs/ARCHITECTURE.md: match visible output, not internal
// implementation).
int GM_CDECL gmf_VMSHAPE_plotVertex(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    return PlotVertexOn(a_thread, s);
}
int GM_CDECL gmf_VMSHAPE_endShape(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    DrawShapeData(s);
    s->needsClearOnNextPlot = true;
    return GM_OK;
}

// VMSHAPE.setAsFilledCircle(radius)/getQuality()/plotVertices(from,to)/draw()
// -- confirmed real (the only real call site, game_data/VideoHelper.gm:685-686
// & 751-813, exercises all four together) but with no decompiled body: the
// name isn't in EBOOT.elf's string table at all (checked directly, unlike
// every other function in this file -- confirmed present: beginShape/
// plotVertex/endShape all are), so unlike the rest of this reimplementation
// this one function's exact geometry can't be pinned to a Ghidra address.
// Reconstructed instead from the SAME retail script file's own reference
// implementation: VideoHelper.gm wraps its entire pre-optimization
// implementation in `if(0){ /* ... */ }` (dead but still literally retail
// source, lines 1-683) immediately before the live `}else{` block that
// replaces it with this SHAPE()-based one (line 684+). The dead
// VIDEOHELPER_DrawFilledCircle draws an identical-looking circle via a plain
// per-frame loop:
//   Video.beginShape(SHAPETYPE_TRIANGLEFAN);
//   Video.plotVertex(x,y,z);                         // centre
//   for (i=0;i<=q;i+=1) {                             // q+1 ring points,
//     Video.plotVertex(x+cos(ang)*r,y+sin(ang)*r,z);  // ang=i*(TWOPI/q) --
//     ang+=angInc;                                    // i=q closes back to
//   }                                                 // i=0's position
//   Video.endShape();
// -- i.e. this is a precompute-once optimization of that same loop, not a
// behavior change: SHAPE(SHAPETYPE_TRIANGLEFAN,61+1,GameSystem)'s capacity
// (62 = centre + (q+1) ring points) fixes q=60, matching the live corpus's
// `61+1` literally spelling out "(q+1) ring points, plus 1 for the centre".
// getQuality() must return the RING point count including its own built-in
// closing duplicate (61 = capacity-1), not q itself: plotVertices(1,
// getQuality()-1) = plotVertices(1,60) is used for the LINELOOP outline
// (VIDEOHELPER_DrawCircle) to replay ring indices 1..60 -- every ring point
// EXCEPT the final one (index 61), which exactly duplicates index 1 and
// would draw a redundant zero-length closing segment on a LINELOOP (which
// already auto-closes). draw() (VIDEOHELPER_DrawFilledCircle) instead draws
// every stored vertex including that duplicate, matching the dead code's
// TRIANGLEFAN loop exactly (centre + full closed ring).
int GM_CDECL gmf_setAsFilledCircle(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(radius, 0, 1.0f);

    int quality = s->capacity - 1; // ring point count, incl. closing duplicate
    if (quality < 2) quality = 2;
    int segments = quality - 1;    // q in the dead code's terms
    if (segments < 1) segments = 1;
    float angInc = 6.28318530718f / (float)segments;

    s->verts.clear();
    s->verts.reserve((size_t)(1 + quality) * 2);
    s->verts.push_back(0.0f);
    s->verts.push_back(0.0f); // centre, index 0
    for (int j = 0; j < quality; ++j) {
        float ang = (float)j * angInc;
        s->verts.push_back(std::cos(ang) * radius);
        s->verts.push_back(std::sin(ang) * radius);
    }
    s->localZ = 0.0f;
    s->needsClearOnNextPlot = false;
    return GM_OK;
}

int GM_CDECL gmf_getQuality(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    int quality = s->capacity - 1;
    a_thread->PushInt(quality > 0 ? quality : 0);
    return GM_OK;
}

// Replays this shape's OWN precomputed vertices [a_from, a_to] (1-indexed,
// matching setAsFilledCircle's layout where index 0 is the centre) into
// whatever shape Video.beginShape/drawList.beginShape most recently opened
// (g_currentShape) -- see that variable's header comment. Every real call
// site wraps this in an outer Video.beginShape(...)/Video.endShape() pair
// around the SAME already-built circleShape, feeding its own precomputed
// ring into a fresh shape under a different transform each time, so this
// intentionally does NOT draw `this` shape directly (that's draw(), below).
int GM_CDECL gmf_plotVertices(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(from, 0, 0);
    GM_INT_PARAM(to, 1, 0);
    if (!g_currentShape) return GM_OK; // no active beginShape -- nothing to feed into

    if (g_currentShape->needsClearOnNextPlot) {
        g_currentShape->verts.clear();
        g_currentShape->colors.clear();
        g_currentShape->uvs.clear();
        g_currentShape->needsClearOnNextPlot = false;
    }
    // The source ring (`s`, from setAsFilledCircle) has no per-vertex colour
    // of its own -- every copied vertex gets whatever Video.setColour() had
    // set as of THIS call, same as a plain plotVertex() would (see
    // PlotVertexOn) -- this is what lets bg.gm's `setColour(...,0.97);
    // plotVertex(0,0); setColour(...,0); shape.plotVertices(1,quality);`
    // idiom produce a real two-stop gradient (bright centre, transparent
    // ring) instead of a flat wash.
    float cr, cg, cb, ca;
    GetEngineContext().renderer->GetColour(cr, cg, cb, ca);
    int count = (int)(s->verts.size() / 2);
    bool srcHasUv = ((int)s->uvs.size() == count * 2);
    for (int idx = from; idx <= to; ++idx) {
        if (idx < 0 || idx >= count) continue;
        g_currentShape->verts.push_back(s->verts[idx * 2]);
        g_currentShape->verts.push_back(s->verts[idx * 2 + 1]);
        g_currentShape->colors.push_back(cr);
        g_currentShape->colors.push_back(cg);
        g_currentShape->colors.push_back(cb);
        g_currentShape->colors.push_back(ca);
        // Real per-vertex UV if the source shape has one (getVertex(i).u/v,
        // see ShapeVertexData's header comment -- PekkoAndJon/bg.gm's
        // update() populates exactly this), else the same 0.5,0.5 default
        // PlotVertexOn uses so the array stays in lockstep with verts.
        g_currentShape->uvs.push_back(srcHasUv ? s->uvs[idx * 2] : 0.5f);
        g_currentShape->uvs.push_back(srcHasUv ? s->uvs[idx * 2 + 1] : 0.5f);
    }
    return GM_OK;
}

// Draws this shape's own stored vertices immediately, under the caller's
// current transform -- same underlying draw as endShape()/drawShape(), just
// without rebuilding the vertex list first (this shape's verts were already
// populated once by setAsFilledCircle(), not by plotVertex()).
int GM_CDECL gmf_shapeDraw(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    DrawShapeData(s);
    return GM_OK;
}

// setShaderBasedOnTexture(texture) -- confirmed real, live call site
// SafeVine.gm:158: `t.shape.setShaderBasedOnTexture(this.thornTexture);`
// (picks a shader variant matching a texture's alpha-channel format). Was
// entirely unbound -- hard "getdot failed" aborting setupThorn()'s caller
// every time a vine's thorn shapes were positioned. This renderer has no
// per-shape shader-variant selection (everything already draws through one
// fixed textured-quad path), so genuine no-op -- consumed rather than left
// as a null-call, matching every other unmodeled-render-feature stub in
// this codebase (e.g. DC_gmf_setOrderTransparent).
int GM_CDECL gmf_shapeSetShaderBasedOnTexture(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// FoE texture methods (stubs) -- shapes can call these but they're no-ops
int GM_CDECL gmf_VMSHAPE_setTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    return GM_OK;
}

int GM_CDECL gmf_VMSHAPE_setIsUsesTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    return GM_OK;
}

int GM_CDECL gmf_VMSHAPE_isUsesTexture(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}

// SHAPEVERTEX -- confirmed real native type (Ghidra: FoE PC binary's own
// native-type registration table lists SHAPEVERTEX alongside SHAPE/ALARM/
// REALPOINT/etc, see reveng_pc/FoE_decompiled_functions.txt). getVertex(i)
// returns a LIVE handle into shape s's own vertex/uv arrays at index i --
// not a copy -- so PekkoAndJon/bg.gm's ShapeHelper.gm idiom
// (`this.shapePoints[i].vertex.x = ...`, ShapeHelper.gm's
// SHAPEHELPER_GetVertexTableArray wrapping getVertex(i) results once in the
// constructor) keeps mutating the SAME underlying shape every subsequent
// frame's update(), instead of writing to a disconnected snapshot. Without
// this, bg.gm's constructor throws "getdot failed" on its very first
// getVertex() call (getVertex didn't exist at all before this), so
// this.shapePoints/this.shape's real star-fan geometry never builds --
// plotVertices() then had 0 real vertices to copy, leaving the whole
// kaleidoscope backdrop as a flat colour wash.
gmType g_shapeVertexType = -1;
struct ShapeVertexData {
    ShapeData* shape = nullptr;
    int index = 0;
};

void EnsureVertexCapacity(ShapeData* s, int a_index) {
    int needed = a_index + 1;
    if ((int)(s->verts.size() / 2) < needed) s->verts.resize((size_t)needed * 2, 0.0f);
    if ((int)(s->uvs.size() / 2) < needed) s->uvs.resize((size_t)needed * 2, 0.0f);
}

void GM_CDECL ShapeVertexGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (ShapeVertexData*)a_object->m_user;
}

// x/y/u/v property access -- same O_GETDOT/O_SETDOT convention as
// vector2D's own x/y (vector2d_bindings.cpp's OpGetDot/OpSetDot), just
// four fields instead of two, and writing through to the OWNING shape's
// verts/uvs arrays (by index) rather than a self-contained struct.
void GM_CDECL ShapeVertexGetDot(gmThread* a_thread, gmVariable* a_operands) {
    gmUserObject* obj = (gmUserObject*)a_thread->GetMachine()->GetObject(a_operands[0].m_value.m_ref);
    ShapeVertexData* v = (ShapeVertexData*)obj->m_user;
    gmStringObject* member = (gmStringObject*)a_thread->GetMachine()->GetObject(a_operands[1].m_value.m_ref);
    if (member->GetLength() != 1 || !v->shape) { a_operands[0].Nullify(); return; }
    EnsureVertexCapacity(v->shape, v->index);
    char c = member->GetString()[0];
    if (c == 'x') a_operands[0].SetFloat(v->shape->verts[v->index * 2]);
    else if (c == 'y') a_operands[0].SetFloat(v->shape->verts[v->index * 2 + 1]);
    else if (c == 'u') a_operands[0].SetFloat(v->shape->uvs[v->index * 2]);
    else if (c == 'v') a_operands[0].SetFloat(v->shape->uvs[v->index * 2 + 1]);
    else a_operands[0].Nullify();
}
void GM_CDECL ShapeVertexSetDot(gmThread* a_thread, gmVariable* a_operands) {
    gmUserObject* obj = (gmUserObject*)a_thread->GetMachine()->GetObject(a_operands[0].m_value.m_ref);
    ShapeVertexData* v = (ShapeVertexData*)obj->m_user;
    gmStringObject* member = (gmStringObject*)a_thread->GetMachine()->GetObject(a_operands[2].m_value.m_ref);
    if (member->GetLength() != 1 || !v->shape) return;
    float newVal = (a_operands[1].m_type == GM_FLOAT) ? a_operands[1].m_value.m_float
                 : (a_operands[1].m_type == GM_INT) ? (float)a_operands[1].m_value.m_int : 0.0f;
    EnsureVertexCapacity(v->shape, v->index);
    char c = member->GetString()[0];
    if (const char* dbg = std::getenv("GE3_VERTEX_SET_DEBUG")) {
        (void)dbg;
        // one line per frame: vertex 1's u, where cos(0)==1 so u == 0.5+amt
        static int s_n = 0;
        if (v->index == 1 && c == 'u' && s_n < 30) { ++s_n;
            std::fprintf(stderr, "[VSETDBG] frame-sample vtx1.u = %.6f  (implies amt=%.6f)\n",
                         newVal, newVal - 0.5f);
        }
    }
    if (c == 'x') v->shape->verts[v->index * 2] = newVal;
    else if (c == 'y') v->shape->verts[v->index * 2 + 1] = newVal;
    else if (c == 'u') v->shape->uvs[v->index * 2] = newVal;
    else if (c == 'v') v->shape->uvs[v->index * 2 + 1] = newVal;
}

// SHAPE.getVertex(index) -- see SHAPEVERTEX's header comment above.
int GM_CDECL gmf_VMSHAPE_getVertex(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    if (index < 0) return GM_EXCEPTION;
    EnsureVertexCapacity(s, index);
    ShapeVertexData* data = new ShapeVertexData();
    data->shape = s;
    data->index = index;
    a_thread->PushNewUser(data, g_shapeVertexType);
    return GM_OK;
}

gmFunctionEntry s_shapeLib[] = {
    {"plotVertex", gmf_VMSHAPE_plotVertex, nullptr},
    {"endShape", gmf_VMSHAPE_endShape, nullptr},
    {"setAsFilledCircle", gmf_setAsFilledCircle, nullptr},
    {"getQuality", gmf_getQuality, nullptr},
    {"plotVertices", gmf_plotVertices, nullptr},
    {"draw", gmf_shapeDraw, nullptr},
    {"setShaderBasedOnTexture", gmf_shapeSetShaderBasedOnTexture, nullptr},
    {"setTexture", gmf_VMSHAPE_setTexture, nullptr},
    {"setIsUsesTexture", gmf_VMSHAPE_setIsUsesTexture, nullptr},
    {"isUsesTexture", gmf_VMSHAPE_isUsesTexture, nullptr},
    {"getVertex", gmf_VMSHAPE_getVertex, nullptr},
};

void GM_CDECL ShapeGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (ShapeData*)a_object->m_user;
}

} // namespace

// See header comment. Builds shape geometry directly, bypassing the
// beginShape/plotVertex/endShape trap thunks -- those are trapped because
// retail's bodies for them aren't recovered, but a NATIVE VideoHelper
// function doesn't call them in the first place: retail's own
// VIDEOHELPER_DrawFilledRect/DrawFilledPolygon are native code
// the shape's vertex buffer directly, exactly as this does.
void PushShapeFromVerts(gmThread* a_thread, int a_shapeType, const float* a_xy,
                        int a_count, float a_z, gmUserObject* a_existing) {
    ShapeData* data;
    if (a_existing) {
        data = (ShapeData*)a_existing->m_user;
    } else {
        data = new ShapeData();
    }
    data->glMode = ToGLMode(a_shapeType);
    data->capacity = a_count;
    data->localZ = a_z;
    data->verts.assign(a_xy, a_xy + (size_t)a_count * 2);
    // Per-vertex colour/uv stay empty: these shapes carry no gradient and no
    // real UVs, so they draw under the current flat draw colour (the same
    // thing setAsFilledCircle's precomputed ring does).
    data->colors.clear();
    data->uvs.clear();
    data->needsClearOnNextPlot = false;
    if (a_existing) {
        a_thread->Push(gmVariable(GM_USER, (gmptr)a_existing));
    } else {
        a_thread->PushNewUser(data, g_shapeType);
    }
}

gmUserObject* AsShapeObject(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type < GM_USER) return nullptr;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref);
    if (!obj || obj->m_userType != g_shapeType) return nullptr;
    return obj;
}

// See header comment -- redraws a shape's stored (local-space) geometry
// under the caller's CURRENT transform/colour, without touching its
// plotVertex/endShape rebuild state.
//
// Leniently no-ops on a missing/wrong-type argument rather than throwing --
// confirmed necessary, not just cautious: real retail's own VideoHelper.gm
// helpers (VIDEOHELPER_DrawFilledRect/Centered, used as
// `drawList.drawShape(VIDEOHELPER_DrawFilledRectCentered(...))` in e.g.
// Platformer/Main.gm's postDraw()) never `return` the shape they build
// internally via Video.beginShape/plotVertex/endShape -- that call always
// passes drawShape() a null in real retail too. Since the genuine article
// doesn't crash on this (players complete the game), the native drawShape
// must tolerate it silently; a hard exception here would abort the rest of
// that draw call's own function every time it runs. For the OVERWHELMING
// majority of real call sites this null is harmless: VIDEOHELPER_
// DrawFilledRect's own Video.beginShape/plotVertex/endShape call already
// drew the correct content immediately, every frame (these helpers are
// virtually always rebuilt-and-redrawn fresh, not cached), so the
// redundant drawShape(null) right after changes nothing visible.
//
// ONE confirmed real exception: PopupProcessBase.gm's customVerticalScrolling
// body type CACHES its stencil-clip mask once (`if (!?this.base_stencilShape)
// { this.base_stencilShape=VIDEOHELPER_DrawFilledRect(...); }`) and replays
// it via drawShape() every subsequent frame -- meaning the one-time
// immediate draw happens before setAttrStencil's REPLACE op is even
// configured for this pass, and every later drawShape(null) genuinely never
// writes the mask. The following VIDEOPARAMS_EQUAL clip test then discards
// everything drawn under it, since the stencil buffer was never actually
// set to the reference value anywhere -- confirmed live: MainMenu5/
// CreditsPopupTable.gm's whole credits list rendered as a blank popup body,
// only fixed once this fallback existed. When a REPLACE stencil write is
// currently configured, this fills the whole viewport into the stencil
// buffer instead of no-op'ing (colour writes suppressed, so nothing paints
// visibly) -- both real cache calls (clear-shape and mask-shape) are sized
// to cover the entire scrollable body anyway, so "the whole viewport" is
// the same practical region a correctly-returned shape would have covered.
// (forward-declared above DrawShapeData, which now also calls it)

int GM_CDECL DC_gmf_drawShape(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmUserObject* obj = a_thread->ParamUserObject(0);
    if (!obj || obj->m_userType != g_shapeType) {
        return DC_RecordOrRun(a_thread, [] {
            DrawContext& ctx = GetDrawContext();
            if (ctx.stencilDppass != 101 /* VIDEOPARAMS_REPLACE */) return;
            GetEngineContext().renderer->FillStencilFullscreen(ctx.Top().z);
        });
    }
    ShapeData* s = (ShapeData*)obj->m_user;
    // Checked at REPLAY time (inside the lambda), not here -- setTextureByHandle
    // (draw_context.cpp) is itself a recorded op in the SAME command list, so
    // by the time THIS native call runs, any preceding setTextureByHandle in
    // the same script sequence hasn't actually mutated DrawContext yet, only
    // queued a command to do so later. See DC_gmf_setTextureByHandle's own
    // header comment for the live crash (SafeVine.gm:291) this whole texture-
    // binding pair fixes.
    auto draw = [s] {
        unsigned int tex = GetDrawContext().currentTextureHandle;
        if (tex != 0) DrawShapeBoundingBoxTextured(s, tex);
        else DrawShapeData(s);
    };
    // Deferred if called via a real drawList instance -- see draw_list.h's
    // RecordingTarget header comment; matches every other shared drawList
    // primitive (draw_context.cpp).
    if (DrawListData* rec = RecordingTarget(a_thread)) {
        rec->commands.push_back(draw);
        return GM_OK;
    }
    draw();
    return GM_OK;
}

// Video.c, 0x0007c1a0) but the decompiled body is opaque refcount/dispatch
// boilerplate wrapping a single un-recovered call (FUN_0003f3dc) -- no
// usable geometry logic survives decompilation. Approximated as a single
// textured quad spanning the shape's own local-space bounding box, rather
// than replaying its individual vertices with per-vertex UVs: this
// renderer's textured draw path only knows how to fill a quad
// (Renderer::DrawTexturedQuad), and every real call site's texture is
// pre-baked bitmap art (QuadShadowed.gm's dropShadowWhite.png,
// CommunityScreenDemo.gm's speech-bubble PNG) whose own alpha channel
// already carries the rounded/bubble shape -- a plain bounding-box quad
// reproduces the visible result without a true 9-slice mesh. Neither real
// call site passes the shape as `this` (always an explicit first arg,
// `drawList.drawShapeWithTexture(this.shadowShape, this.shadowTexture)`),
// so this reads Param(0)/Param(1) directly rather than ThisShape().
// Shared bounding-box textured-quad draw, reused by both
// DC_gmf_drawShapeWithTexture (explicit texture arg) and DC_gmf_drawShape
// (implicit texture via DrawContext::currentTextureHandle -- see
// DC_gmf_setTextureByHandle's header comment, draw_context.cpp).
void DrawShapeBoundingBoxTextured(const ShapeData* s, unsigned int handle) {
    if (s->verts.size() < 2) return;
    float minX = s->verts[0], maxX = s->verts[0];
    float minY = s->verts[1], maxY = s->verts[1];
    for (size_t i = 0; i < s->verts.size(); i += 2) {
        minX = std::min(minX, s->verts[i]);
        maxX = std::max(maxX, s->verts[i]);
        minY = std::min(minY, s->verts[i + 1]);
        maxY = std::max(maxY, s->verts[i + 1]);
    }
    const DrawContext::Mat& m = GetDrawContext().Top();
    float tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3;
    DC_TransformPoint(m, minX, minY, tx0, ty0);
    DC_TransformPoint(m, maxX, minY, tx1, ty1);
    DC_TransformPoint(m, maxX, maxY, tx2, ty2);
    DC_TransformPoint(m, minX, maxY, tx3, ty3);
    GetEngineContext().renderer->DrawTexturedQuad(tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3, handle, m.z + s->localZ);
}

int GM_CDECL DC_gmf_drawShapeWithTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmUserObject* obj = a_thread->ParamUserObject(0);
    if (!obj || obj->m_userType != g_shapeType) return GM_OK; // same tolerant-null convention as DC_gmf_drawShape
    ShapeData* s = (ShapeData*)obj->m_user;

    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(1));

    // Deferred if called via a real drawList instance -- see draw_list.h's
    // RecordingTarget header comment. The transform (m/tx.../ty...) is
    // resolved at DRAW time, not here, same reasoning as drawTexture/print.
    return DC_RecordOrRun(a_thread, [s, handle] {
        DrawShapeBoundingBoxTextured(s, handle);
    });
}

// VIDEOHELPER_CreateTextBubble(texture, borderL, borderT, borderR, borderB,
// VideoHelper.c, 0x001b8240) but Ghidra failed to decompile the body at all
// ("bad instruction data", likely a jump table it couldn't follow) -- no
// geometry to port. The one thing every real call site agrees on: the
// result is only ever fed straight into drawShapeWithTexture (QuadShadowed.
// gm:37, CommunityScreenDemo.gm:231, EditScreen2*.gm), never plotVertex'd,
// inspected, or resized afterward. Approximated as the same plain unit quad
// (-0.5..0.5, matching VIDEOHELPER_DrawFilledPolygon's own shapes, e.g.
// QuadShadowed.gm's `quadShape`) that drawShapeWithTexture's bounding-box
// approximation (above) already reduces any shape to -- the border/corner
// arguments would only matter for a true 9-slice mesh, which nothing here
// builds, so they're accepted and ignored rather than guessed at.
int GM_CDECL gmf_VIDEOHELPER_CreateTextBubble(gmThread* a_thread) {
    ShapeData* data = new ShapeData();
    data->glMode = GL_TRIANGLE_FAN;
    data->verts = { -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, 0.5f,  -0.5f, 0.5f };
    a_thread->PushNewUser(data, g_shapeType);
    return GM_OK;
}

// Video.plotVertex(...)/Video.endShape() (and the same on drawList) -- the
// bare-call counterpart to shape.plotVertex()/shape.endShape() above,
// operating on g_currentShape instead of an explicit receiver. Same
// decompiled-confirmed dispatch as gmf_VMSHAPE_plotVertex/endShape (see
// their header comment) -- these are literally the SAME registration
// table entries, just invoked without a captured local (e.g.
// VideoHelper.gm's VIDEOHELPER_DrawFilledRect: `Video.beginShape(...);
// Video.plotVertex(x1,y1,z); ...; Video.endShape();`, no captured shape
// anywhere in that function).
int GM_CDECL gmf_Shape_plotVertexCurrent(gmThread* a_thread) {
    if (!g_currentShape) { GM_EXCEPTION_MSG("plotVertex: no shape currently being built (no matching beginShape)"); return GM_EXCEPTION; }
    return PlotVertexOn(a_thread, g_currentShape);
}
int GM_CDECL gmf_Shape_endShapeCurrent(gmThread* a_thread) {
    if (!g_currentShape) { GM_EXCEPTION_MSG("endShape: no shape currently being built (no matching beginShape)"); return GM_EXCEPTION; }
    DrawShapeData(g_currentShape);
    g_currentShape->needsClearOnNextPlot = true;
    return GM_OK;
}

void RegisterShapeBindings(gmMachine& a_machine) {
    // Confirmed real retail type name (Ghidra, EBOOT.elf's native type
    // here, an invented name; only affects typeName()/exception text.
    g_shapeType = a_machine.CreateUserType("VMSHAPE");
    a_machine.RegisterTypeLibrary(g_shapeType, s_shapeLib, sizeof(s_shapeLib) / sizeof(s_shapeLib[0]));
    a_machine.RegisterUserCallbacks(g_shapeType, nullptr, ShapeGCDestruct, nullptr);

    // SHAPEVERTEX -- see its header comment (above gmf_VMSHAPE_getVertex).
    g_shapeVertexType = a_machine.CreateUserType("SHAPEVERTEX");
    a_machine.RegisterUserCallbacks(g_shapeVertexType, nullptr, ShapeVertexGCDestruct, nullptr);
    a_machine.RegisterTypeOperator(g_shapeVertexType, O_GETDOT, nullptr, ShapeVertexGetDot);
    a_machine.RegisterTypeOperator(g_shapeVertexType, O_SETDOT, nullptr, ShapeVertexSetDot);

    // SHAPE(shapeType, numVerts, owner) -- confirmed real, game_data/
    // VideoHelper.gm -- distinct native constructor from Video.beginShape,
    // see gmf_SHAPE's header comment for why they can't share a function
    // once beginShape has to track g_currentShape.
    a_machine.RegisterLibraryFunction("SHAPE", gmf_SHAPE, nullptr);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_CreateTextBubble", gmf_VIDEOHELPER_CreateTextBubble, nullptr);

    gmTableObject* globals = a_machine.GetGlobals();
    globals->Set(&a_machine, "SHAPETYPE_QUADS", gmVariable((gmint)ShapeType::Quads));
    globals->Set(&a_machine, "SHAPETYPE_TRIANGLES", gmVariable((gmint)ShapeType::Triangles));
    globals->Set(&a_machine, "SHAPETYPE_TRIANGLESTRIP", gmVariable((gmint)ShapeType::TriangleStrip));
    globals->Set(&a_machine, "SHAPETYPE_TRIANGLEFAN", gmVariable((gmint)ShapeType::TriangleFan));
    globals->Set(&a_machine, "SHAPETYPE_LINES", gmVariable((gmint)ShapeType::Lines));
    globals->Set(&a_machine, "SHAPETYPE_LINESTRIP", gmVariable((gmint)ShapeType::LineStrip));
    globals->Set(&a_machine, "SHAPETYPE_LINELOOP", gmVariable((gmint)ShapeType::LineLoop));
    globals->Set(&a_machine, "SHAPETYPE_CIRCLE", gmVariable((gmint)ShapeType::Circle));
}
