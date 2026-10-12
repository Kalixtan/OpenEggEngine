#include "gmThread.h"
#include "shape_bindings.h"
#include "vector2d_bindings.h"
#include "../render/draw_context.h"
#include "../render/draw_list.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "texture_bindings.h"
#include "process_handle_bindings.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {
std::map<std::tuple<int, int, int>, float> g_entityShaderFloats;
}

void SetEntityShaderFloat(int a_entityId, int a_slot, int a_index, float a_value) {
    g_entityShaderFloats[{a_entityId, a_slot, a_index}] = a_value;
}

namespace {

gmType g_shapeType = -1;

// Retail drawShape/drawShapeWithTexture retain the native shape resource
// when recording opcodes 6/0xe (Video.c: FUN_0003f3dc and drawShape wrappers).
// The desktop command closures hold ShapeData pointers, so root the owning
// GameMonkey object until every queued closure that uses it is destroyed.
struct DeferredShapeRootState {
    size_t uses = 0;
    bool addedRoot = false;
};
std::unordered_map<gmObject*, DeferredShapeRootState> g_deferredShapeRoots;

void RetainDeferredShape(gmMachine& machine, gmObject* object) {
    if (!object) return;
    DeferredShapeRootState& state = g_deferredShapeRoots[object];
    if (state.uses++ == 0 && !machine.IsCPPOwnedGMObject(object)) {
        machine.AddCPPOwnedGMObject(object);
        state.addedRoot = true;
    }
}

void ReleaseDeferredShape(gmMachine& machine, gmObject* object) {
    auto it = g_deferredShapeRoots.find(object);
    if (it == g_deferredShapeRoots.end()) return;
    if (it->second.uses > 0 && --it->second.uses == 0) {
        if (it->second.addedRoot && machine.IsCPPOwnedGMObject(object))
            machine.RemoveCPPOwnedGMObject(object);
        g_deferredShapeRoots.erase(it);
    }
}

struct DeferredShapeLease {
    gmMachine* machine;
    gmObject* object;

    DeferredShapeLease(gmMachine& a_machine, gmObject* a_object)
        : machine(&a_machine), object(a_object) {
        RetainDeferredShape(a_machine, a_object);
    }

    ~DeferredShapeLease() {
        if (machine && object) ReleaseDeferredShape(*machine, object);
    }
};

std::shared_ptr<DeferredShapeLease> MakeDeferredShapeLease(
    gmThread* thread, gmUserObject* shapeObject) {
    if (!thread || !shapeObject || !RecordingTarget(thread)) return {};
    gmVariable shape = thread->Param(0);
    if (!shape.IsReference()) return {};
    gmMachine* machine = thread->GetMachine();
    gmObject* object = machine ? machine->GetObject(shape.m_value.m_ref) : nullptr;
    if (!object || (gmUserObject*)object != shapeObject) return {};
    return std::make_shared<DeferredShapeLease>(*machine, object);
}

// Retail VMSHAPE.endShape at 0x7ba38 maps the raw mode byte through the
// EBOOT tables at 0x3a1384/0x3a13a8. Field +4 receives RSX primitive codes
// [1,2,4,3,5,6,7,8,10] for mode bytes 0..8; PSL1GHT's GCM_TYPE_* definitions
// identify those as points, lines, line strip, line loop, triangles,
// triangle strip, triangle fan, quads, polygon. Numeric mode meanings are
// bytecode-backed; mapping SHAPETYPE_* names to those values follows their
// names and use sites because no named-constant assignment table has yet been
// recovered. This replaces the former claim based on FUN_00421810, which does
// not exist in this EBOOT.
enum class ShapeType {
    Points = 0,
    Lines = 1,
    LineStrip = 2,
    LineLoop = 3,
    Triangles = 4,
    TriangleStrip = 5,
    TriangleFan = 6,
    Quads = 7,
    Polygon = 8,
};

// Verification: integer values are bytecode-backed by retail VMSHAPE.endShape
// at 0x7ba38 and its two nine-entry tables; GCM primitive names/codes are
// cross-checked against PSL1GHT ppu/include/rsx/gcm_sys.h. GL enum translation
// is host-renderer adaptation, not a claim of identical RSX rasterization.
GLenum ToGLMode(int a_shapeType) {
    switch ((ShapeType)a_shapeType) {
        case ShapeType::Points: return GL_POINTS;
        case ShapeType::Lines: return GL_LINES;
        case ShapeType::LineStrip: return GL_LINE_STRIP;
        case ShapeType::LineLoop: return GL_LINE_LOOP;
        case ShapeType::Triangles: return GL_TRIANGLES;
        case ShapeType::TriangleStrip: return GL_TRIANGLE_STRIP;
        case ShapeType::TriangleFan: return GL_TRIANGLE_FAN;
        case ShapeType::Quads: return GL_QUADS;
        case ShapeType::Polygon: return GL_POLYGON;
        default: return GL_TRIANGLE_FAN;
    }
}

struct ShapeData {
    GLenum glMode = GL_TRIANGLE_FAN;
    // VMSHAPE.releaseReference drops the wrapper's own native mesh reference
    // after another owner (notably Animation.setShape) has retained it.
    bool hasDirectReference = true;
    // Retail VMSHAPE.bindEntityShader stores a selected entity shader
    // reference on the shape. Keep the association until the renderer
    // consumes per-entity shader programs and parameters.
    int shaderEntityId = -1;
    int shaderSlot = 0;
    std::string shaderEntityType;
    // SafeVine calls setShaderBasedOnTexture before building its texture-
    // extent UV mesh. Retail selects a render config there; the full config
    // mapping remains open, but keep this shape state for UV submission.
    bool hasTextureBasedRenderConfig = false;
    int textureShaderType = 0;
    // xy pairs, LOCAL space (i.e. relative to whatever transform is active
    // at the eventual draw call -- see header comment update below for why
    // this changed from world-space/pre-transformed).
    std::vector<float> verts;
    // The third through fifth plotVertex values are shader-specific inputs.
    // For example lasers use (angle,y,z,type,width), while DSBounceThing
    // uses (angle,width,height,unused,z). Preserve all three unmodified;
    // the shader selected on this shape determines their meaning.
    std::vector<float> vertexData;
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
    // real per-vertex UVs, DrawPolygonGradient derives UVs from the mesh
    // bounds, which is a flat single-texture-copy stretch, not the animated
    // radial sampling that produces bg.gm's kaleidoscope look.
    std::vector<float> uvs;
    // Last appended Z, retained for existing host draw helpers. Retail
    // VMSHAPE records carry Z per vertex; DrawShapeData currently collapses
    // this field across the whole shape (see VIDEO_SHAPE_PRIMITIVE_AUDIT.md).
    float localZ = 0.0f;
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
    // VIDEO.beginShape's third argument initializes native constructor
    // field +0x24. Default 0x14; the 3-argument overload stores arg2 << 2.
    int retailConstructorField24 = 0x14;
    int retailShapeType = 0;
    int retailEndShapeField4 = 0;
    int retailEndShapeField8 = 0;
};

ShapeData* ThisShape(gmThread* a_thread) {
    ShapeData* shape = (ShapeData*)a_thread->ThisUserCheckType(g_shapeType);
    return shape && shape->hasDirectReference ? shape : nullptr;
}

// Compatibility current-shape slot used by the DrawList extension handlers
// and plotVertices helper. Retail VIDEO.beginShape does not write this global;
// its exact handler below only returns the newly allocated VMSHAPE.
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
// Verification: retail textured-shape config selection is bytecode/decomp
// confirmed (`FUN_00055bb4`, selector 8 = TextureDist). Forwarding this shape
// draw through the existing desktop distance shader is a local approximation;
// retail's aColor payload and aSmoothLevel source are still unresolved.
void DrawShapeData(ShapeData* s) {
    int count = (int)(s->verts.size() / 2);
    if (count <= 0) return;
    DrawContext& ctx = GetDrawContext();
    static std::vector<float> transformed;
    transformed.resize(s->verts.size());
    auto param = [s](int index) {
        auto it = g_entityShaderFloats.find({s->shaderEntityId, s->shaderSlot, index});
        return it == g_entityShaderFloats.end() ? 0.0f : it->second;
    };
    auto signedPow = [](float value, float exponent) {
        return std::copysign(std::pow(std::fabs(value), exponent), value);
    };
    const bool laser = s->shaderEntityType == "Platformer_EntityPacks_SnareLaser" ||
                       s->shaderEntityType == "Platformer_EntityPacks_KickLaser";
    const bool bounce = s->shaderEntityType == "Platformer_EntityPacks_DSBounceThing";
    const bool soundPreview = s->shaderEntityType == "Platformer_SoundPreviewShader";
    const bool hasVertexData = s->vertexData.size() == s->verts.size() / 2 * 3;
    for (int i = 0; i < count; ++i) {
        float x = s->verts[i * 2], y = s->verts[i * 2 + 1];
        if (hasVertexData && laser && s->shaderSlot == 0) {
            const float type = s->vertexData[i * 3 + 1];
            const float width = s->vertexData[i * 3 + 2];
            const float rebirth = param(7);
            if (type < 1.5f) {
                constexpr float pi = 3.14159265358979323846f;
                const float portion = std::clamp((x - pi * 0.3f) / (pi * 0.7f), 0.0f, 1.0f);
                const float sc = x >= pi * 0.3f ? param(5) + (1.0f - param(5)) * (1.0f - std::pow(portion, 1.3f)) : 1.0f;
                float halfWidth = (std::fabs(width) * (1.0f + 0.2f * rebirth) +
                                   signedPow(std::sin(x), 0.23f) * param(4)) * sc;
                if (type > 0.5f) halfWidth *= 1.0f + 0.3f * param(6);
                x = std::copysign(halfWidth, width);
            } else {
                x *= (1.0f + 0.2f * rebirth) * (type > 2.5f ? param(5) : 1.0f);
            }
        } else if (hasVertexData && laser && s->shaderSlot == 1) {
            constexpr float pi = 3.14159265358979323846f;
            const float portion = s->vertexData[i * 3 + 1];
            const float hw = param(5) + signedPow(std::sin(param(4) + pi * 0.7f * portion * param(7)), 0.7f) * param(6);
            x *= hw;
            y += portion * param(8);
        } else if (hasVertexData && bounce && s->shaderSlot == 0) {
            const float angle = x;
            x = signedPow(std::cos(angle), param(4)) * y;
            y = signedPow(std::sin(angle), param(4)) * s->vertexData[i * 3];
        } else if (hasVertexData && bounce && s->shaderSlot == 1) {
            const float portion = s->vertexData[i * 3 + 1];
            const float offset = std::sin(param(4) + param(5) * portion) * param(6) + s->vertexData[i * 3 + 2];
            x *= param(7) + offset;
            y *= param(8) + offset;
        } else if (hasVertexData && soundPreview) {
            const float angle = x;
            const float radius = y * param(5) + s->vertexData[i * 3] * param(6);
            x = signedPow(std::cos(angle), param(4)) * radius;
            y = signedPow(std::sin(angle), param(4)) * radius;
        }
        DC_TransformPoint(ctx.Top(), x, y, transformed[i * 2], transformed[i * 2 + 1]);
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
    // a bounding-box quad, which would flatten a large radial backdrop into
    // a visible rectangle AND
    // discard the gradient entirely (a single uColour uniform, no per-vertex
    // varying) -- draw()/setAsFilledCircle()'s own un-gradiented shapes
    // (colors left empty) still fall through to the plain untextured path.
    if (s->shaderEntityId < 0 && (int)s->colors.size() == count * 4) {
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
        const bool distanceTexture = ctx.distanceTextureEnabled ||
            (s->hasTextureBasedRenderConfig && s->textureShaderType == 8);
        GetEngineContext().renderer->DrawPolygonGradient(
            s->glMode, transformed.data(), s->colors.data(), realUv, count, tex,
            ctx.Top().z + s->localZ, realUv && s->hasTextureBasedRenderConfig,
            distanceTexture, ctx.distanceTextureDiscardLevel,
            ctx.distanceTextureSmoothLevel,
            s->hasTextureBasedRenderConfig ? s->textureShaderType : -1);
        return;
    }
    if (tex != 0) {
        // A shape with authored UVs is a textured mesh, not a request to
        // stretch one image over its axis-aligned bounds. SafeVine builds
        // each thorn as two triangles with UVs in getTextureExt() space and
        // then draws it after setTextureByHandle(); the retail shape path
        // submits that vertex/UV data. If this shape has no authored UVs,
        // derive UVs over its bounds while retaining the original mesh.
        if ((int)s->uvs.size() == count * 2) {
            std::vector<float> uv = s->uvs;
            std::vector<float> white((size_t)count * 4, 1.0f);
            const bool distanceTexture = ctx.distanceTextureEnabled ||
                (s->hasTextureBasedRenderConfig && s->textureShaderType == 8);
            GetEngineContext().renderer->DrawPolygonGradient(
                s->glMode, transformed.data(), white.data(), uv.data(), count,
                tex, ctx.Top().z + s->localZ, s->hasTextureBasedRenderConfig,
                distanceTexture, ctx.distanceTextureDiscardLevel,
                ctx.distanceTextureSmoothLevel,
                s->hasTextureBasedRenderConfig ? s->textureShaderType : -1);
            return;
        }
        std::vector<float> white((size_t)count * 4, 1.0f);
        const bool distanceTexture = ctx.distanceTextureEnabled ||
            (s->hasTextureBasedRenderConfig && s->textureShaderType == 8);
        GetEngineContext().renderer->DrawPolygonGradient(
            s->glMode, transformed.data(), white.data(), nullptr, count,
            tex, ctx.Top().z + s->localZ, false, distanceTexture,
            ctx.distanceTextureDiscardLevel, ctx.distanceTextureSmoothLevel,
            s->hasTextureBasedRenderConfig ? s->textureShaderType : -1);
        return;
    }
    GetEngineContext().renderer->DrawPolygon(s->glMode, transformed.data(), count, ctx.Top().z + s->localZ);
}

} // namespace

// Compatibility beginShape handler for DrawList. The Video library uses
// gmf_Video_beginShape below, which models the directly audited VIDEO
// wrapper. This implicit-current-shape behavior remains unverified for
// DrawList and must not be attributed to VIDEO bytecode.
int GM_CDECL gmf_Shape_beginShape(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeType, 0, 0);
    ShapeData* data = new ShapeData();
    data->retailShapeType = shapeType;
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
int RetailFctiwz(float value) {
    if (!std::isfinite(value) || value >= 2147483648.0f ||
        value < -2147483648.0f)
        return INT32_MIN;
    return static_cast<int>(value);
}
int RetailShapeIntegerArgument(const gmVariable& value) {
    return value.m_type == GM_FLOAT
        ? RetailFctiwz(value.m_value.m_float)
        : value.m_value.m_int;
}
}

// Verification: EBOOT Video.beginShape at 0x75c7c and constructor helper
// FUN_00047750. Retail accepts exactly 1/2/3 args, uses raw int payloads or
// fctiwz for float shape/capacity values, defaults capacity to 64 and native
// field +0x24 to 20, and stores raw arg2<<2 there for the 3-arg form. It
// returns a fresh VMSHAPE and does not install an implicit current shape.
int GM_CDECL gmf_Video_beginShape(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > 3) return GM_EXCEPTION;

    ShapeData* data = new ShapeData();
    data->retailShapeType = RetailShapeIntegerArgument(a_thread->Param(0));
    data->glMode = ToGLMode(data->retailShapeType);
    data->capacity = count >= 2
        ? RetailShapeIntegerArgument(a_thread->Param(1)) : 0x40;
    data->retailConstructorField24 = 0x14;
    if (count == 3) {
        const uint32_t raw = static_cast<uint32_t>(a_thread->Param(2).m_value.m_int);
        data->retailConstructorField24 = static_cast<int32_t>(raw << 2);
    }
    a_thread->PushNewUser(data, g_shapeType);
    return GM_OK;
}

// Verification: VIDEO.plotVertex at 0x76fd8. Counts 1 through 5 follow
// valid branches; their eventual helpers 0x524d0/0x524d4 are literal blr
// stubs, so the Video-library method has no geometry side effect. Count 0
// and counts above 5 return -1 in retail (the VM exception result).
int GM_CDECL gmf_Video_plotVertex(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > 5) return GM_EXCEPTION;
    if (count == 2) {
        const int form = a_thread->Param(0).m_value.m_int;
        if (form < 1 || form > 3) return GM_EXCEPTION;
    }
    return GM_OK;
}

// Verification: VIDEO.endShape at 0x762b8 is `li r3,0; blr`.
int GM_CDECL gmf_Video_endShape(gmThread* a_thread) {
    (void)a_thread;
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
    data->retailShapeType = shapeType;
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
        // VMSHAPE.plotVertex(x,y,z,u,v) stores five float slots in retail
        // (FUN_0007b280, five-argument branch). For unbound shapes, the last
        // two values are also treated as UVs here. Shader-bound shapes use
        // those slots as custom vertex inputs instead.
        if (a_thread->GetNumParams() >= 3) { GM_FLOAT_OR_INT_PARAM(zp, 2, 0.0f); localZ = zp; }
    }

    float data2 = localZ, data3 = 0.0f, data4 = 0.0f;
    if (p0.m_type != g_vector2DType) {
        if (a_thread->GetNumParams() >= 4) { GM_FLOAT_OR_INT_PARAM(value, 3, 0.0f); data3 = value; }
        if (a_thread->GetNumParams() >= 5) { GM_FLOAT_OR_INT_PARAM(value, 4, 0.0f); data4 = value; }
    }
    // DSBounceThing's body shader takes (angle, halfWidth, halfHeight,
    // unused, z), whereas the laser and rubber-band shaders take z third.
    if (a_thread->GetNumParams() >= 5 && s->shaderSlot == 0 &&
        s->shaderEntityType == "Platformer_EntityPacks_DSBounceThing")
        localZ = data4;
    if (a_thread->GetNumParams() >= 4 &&
        s->shaderEntityType == "Platformer_SoundPreviewShader")
        localZ = data3;

    if (s->needsClearOnNextPlot) {
        s->verts.clear();
        s->vertexData.clear();
        s->colors.clear();
        s->uvs.clear();
        s->needsClearOnNextPlot = false;
    }
    s->verts.push_back(x);
    s->verts.push_back(y);
    s->vertexData.push_back(data2);
    s->vertexData.push_back(data3);
    s->vertexData.push_back(data4);
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
    const bool textureUV = p0.m_type != g_vector2DType &&
        a_thread->GetNumParams() >= 5 && s->shaderEntityType.empty();
    s->uvs.push_back(textureUV ? data3 : 0.5f);
    s->uvs.push_back(textureUV ? data4 : 0.5f);
    s->localZ = localZ;
    return GM_OK;
}

// RETAIL BYTECODE: VMSHAPE.plotVertex is registered at 0x7b280 and appends
// native vertex records. It is distinct from VIDEO.plotVertex at 0x76fd8,
// whose terminal helpers are no-ops. The legacy script implementation of
// VIDEOHELPER_DrawFilledRect is specifically in the `else` branch of a
// literal `if(1)`; the active method is native at 0x1b25d8 and builds
// VMSHAPE data directly. Other live VideoHelper script functions need their
// own call-site audit before inferring that the bare VIDEO helpers draw.
// VMSHAPE.endShape at 0x7ba38 maps the native mode byte through static tables
// and writes record fields +4/+8. The desktop records those values for valid
// modes, without treating endShape as an implicit reset.
int GM_CDECL gmf_VMSHAPE_plotVertex(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    return PlotVertexOn(a_thread, s);
}
int GM_CDECL gmf_VMSHAPE_endShape(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    // RETAIL BYTECODE: 0x7ba38 masks the constructor's +0x2c mode to a byte,
    // then writes these two table values to the native shape record +4/+8.
    // Table words were read from EBOOT.elf at 0x3a1384/0x3a13a8.
    static constexpr int modeField4[9] = {1, 2, 4, 3, 5, 6, 7, 8, 10};
    static constexpr int modeField8[9] = {
        0x1b00, 0x1b01, 0x1b01, 0x1b01, 0x1b02,
        0x1b02, 0x1b02, 0x1b02, 0x1b02
    };
    const unsigned int mode = static_cast<uint8_t>(s->retailShapeType);
    if (mode < 9) {
        s->retailEndShapeField4 = modeField4[mode];
        s->retailEndShapeField8 = modeField8[mode];
    }
    return GM_OK;
}

// Verification: retail VMSHAPE.getNumVertices at 0x7bad4 reads the current
// native mesh's +0x20 vertex count and pushes it as a GM integer. ShapeData
// stores vertices as xy pairs, so its exact corresponding count is verts/2.
int GM_CDECL gmf_VMSHAPE_getNumVertices(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    a_thread->PushInt((gmint)(s->verts.size() / 2));
    return GM_OK;
}

// Retail VMSHAPE.releaseReference (0x0007bb54) decrements its held native
// shape reference and clears the pointer at wrapper-native offset +0x14.
// The animation that received this shape already owns the mesh separately;
// its draw path must continue to see the stored vertices after this call.
int GM_CDECL gmf_VMSHAPE_releaseReference(gmThread* a_thread) {
    // Keep this operation idempotent: retail clears the held pointer, then
    // a repeated release observes that it is already null.
    ShapeData* s = (ShapeData*)a_thread->ThisUserCheckType(g_shapeType);
    if (!s) return GM_EXCEPTION;
    s->hasDirectReference = false;
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
    s->vertexData.clear();
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
        g_currentShape->vertexData.clear();
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
    bool srcHasVertexData = ((int)s->vertexData.size() == count * 3);
    for (int idx = from; idx <= to; ++idx) {
        if (idx < 0 || idx >= count) continue;
        g_currentShape->verts.push_back(s->verts[idx * 2]);
        g_currentShape->verts.push_back(s->verts[idx * 2 + 1]);
        for (int component = 0; component < 3; ++component)
            g_currentShape->vertexData.push_back(srcHasVertexData ? s->vertexData[idx * 3 + component] : 0.0f);
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
// current transform -- same underlying draw as drawShape(), just
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
// (selects a retail render-config variant from the texture's +0x48 shader
// type). Retail helper FUN_00055bb4 assigns a refcounted GameSystem config
// to shape +0x30. The complete config-to-GL mapping remains open; preserve
// the selected state for the currently recovered textured-mesh behavior.
int GM_CDECL gmf_shapeSetShaderBasedOnTexture(gmThread* a_thread) {
    ShapeData* shape = ThisShape(a_thread);
    if (!shape || a_thread->GetNumParams() != 1) return GM_EXCEPTION;
    shape->hasTextureBasedRenderConfig = true;
    shape->textureShaderType = GetTextureShaderType(
        *a_thread->GetMachine(), a_thread->Param(0));
    return GM_OK;
}

// Retail VMSHAPE.bindEntityShader at 0x7af38 accepts (entity[, slot]);
// helper 0xbfa28 selects that entity's shader and stores it on the shape.
int GM_CDECL gmf_shapeBindEntityShader(gmThread* a_thread) {
    ShapeData* shape = ThisShape(a_thread);
    if (!shape) return GM_EXCEPTION;
    const int numParams = a_thread->GetNumParams();
    if (numParams != 1 && numParams != 2) return GM_EXCEPTION;
    const gmVariable entity = a_thread->Param(0);
    if (entity.m_type != GM_TABLE) return GM_EXCEPTION;
    shape->shaderSlot = numParams == 2 ? a_thread->ParamInt(1, 0) : 0;
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* entityTable = (gmTableObject*)machine.GetObject(entity.m_value.m_ref);
    gmVariable objectId = entityTable->Get(&machine, "__objectID");
    if (objectId.m_type != GM_INT) return GM_EXCEPTION;
    shape->shaderEntityId = objectId.m_value.m_int;
    gmVariable type = entityTable->Get(&machine, "__typeName");
    shape->shaderEntityType = type.m_type == GM_STRING
        ? ((gmStringObject*)machine.GetObject(type.m_value.m_ref))->GetString() : "";
    return GM_OK;
}

// Retail VMSHAPE.bindShader at 0x0007b194 stores a direct SHADER reference.
int GM_CDECL gmf_shapeBindShader(gmThread* a_thread) {
    ShapeData* shape = ThisShape(a_thread);
    if (!shape || a_thread->GetNumParams() != 1) return GM_EXCEPTION;
    int storageId = 0;
    if (!GetShaderStorageId(*a_thread->GetMachine(), a_thread->Param(0), storageId))
        return GM_EXCEPTION;
    shape->shaderEntityId = storageId;
    shape->shaderSlot = 0;
    shape->shaderEntityType = "Platformer_SoundPreviewShader";
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
    if ((int)(s->vertexData.size() / 3) < needed) s->vertexData.resize((size_t)needed * 3, 0.0f);
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
    {"reset", [](gmThread* a_thread) -> int {
        ShapeData* s = ThisShape(a_thread);
        if (!s) return GM_EXCEPTION;
        // Verification: PPC at 0x7ba90 resets native vertex count (+0x20),
        // copies the stored capacity/size word (+0x14) to cursor (+0x0c),
        // and writes -1 to nested record field +4. The allocated buffer stays.
        s->verts.clear();
        s->vertexData.clear();
        s->colors.clear();
        s->uvs.clear();
        s->retailEndShapeField4 = -1;
        s->needsClearOnNextPlot = false;
        return GM_OK;
    }, nullptr},
    {"plotVertex", gmf_VMSHAPE_plotVertex, nullptr},
    {"endShape", gmf_VMSHAPE_endShape, nullptr},
    {"getNumVertices", gmf_VMSHAPE_getNumVertices, nullptr},
    {"releaseReference", gmf_VMSHAPE_releaseReference, nullptr},
    {"setAsFilledCircle", gmf_setAsFilledCircle, nullptr},
    {"getQuality", gmf_getQuality, nullptr},
    {"plotVertices", gmf_plotVertices, nullptr},
    {"draw", gmf_shapeDraw, nullptr},
    {"setShaderBasedOnTexture", gmf_shapeSetShaderBasedOnTexture, nullptr},
    {"bindEntityShader", gmf_shapeBindEntityShader, nullptr},
    {"bindShader", gmf_shapeBindShader, nullptr},
    {"setTexture", gmf_VMSHAPE_setTexture, nullptr},
    {"setIsUsesTexture", gmf_VMSHAPE_setIsUsesTexture, nullptr},
    {"isUsesTexture", gmf_VMSHAPE_isUsesTexture, nullptr},
    {"getVertex", gmf_VMSHAPE_getVertex, nullptr},
};

void GM_CDECL ShapeGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (ShapeData*)a_object->m_user;
}

} // namespace

unsigned int GetShapePrimitiveMode(gmMachine& a_machine,
                                  const gmVariable& a_shape) {
    if (!a_shape.IsReference() || a_shape.m_type < GM_USER) return 0;
    gmObject* object = a_machine.GetObject(a_shape.m_value.m_ref);
    if (!object || object->GetType() != g_shapeType) return 0;
    auto* shapeObject = static_cast<gmUserObject*>(object);
    auto* shape = static_cast<ShapeData*>(shapeObject->m_user);
    return shape && shape->hasDirectReference ? shape->glMode : 0;
}

// See header comment. Builds shape geometry directly, bypassing the
// beginShape/plotVertex/endShape trap thunks -- those are trapped because
// retail's bodies for them aren't recovered, but a NATIVE VideoHelper
// function doesn't call them in the first place: retail's own
// VIDEOHELPER_DrawFilledRect/DrawFilledPolygon are native code
// the shape's vertex buffer directly, exactly as this does.
void PushShapeFromVerts(gmThread* a_thread, int a_shapeType, const float* a_xy,
                        int a_count, float a_z, gmUserObject* a_existing,
                        bool a_preserveExistingMode, bool a_pushExisting) {
    ShapeData* data;
    if (a_existing) {
        data = (ShapeData*)a_existing->m_user;
    } else {
        data = new ShapeData();
    }
    if (!a_existing || !a_preserveExistingMode) {
        data->retailShapeType = a_shapeType;
        data->glMode = ToGLMode(a_shapeType);
    }
    data->capacity = a_count;
    data->localZ = a_z;
    if (a_count > 0) data->verts.assign(a_xy, a_xy + (size_t)a_count * 2);
    else data->verts.clear();
    data->vertexData.clear();
    // Per-vertex colour/uv stay empty: these shapes carry no gradient and no
    // real UVs, so they draw under the current flat draw colour (the same
    // thing setAsFilledCircle's precomputed ring does).
    data->colors.clear();
    data->uvs.clear();
    data->needsClearOnNextPlot = false;
    if (a_existing && a_pushExisting) {
        a_thread->Push(gmVariable(GM_USER, (gmptr)a_existing));
    } else if (!a_existing) {
        a_thread->PushNewUser(data, g_shapeType);
    }
}

gmUserObject* AsShapeObject(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type < GM_USER) return nullptr;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref);
    if (!obj || obj->m_userType != g_shapeType) return nullptr;
    return obj;
}

bool DrawAnimationShapeWithTransform(gmMachine& a_machine, const gmVariable& a_shape,
                                     unsigned int a_texture, float a_alpha,
                                     float a_a, float a_b, float a_c, float a_d,
                                     float a_e, float a_f, float a_z) {
    gmUserObject* obj = AsShapeObject(a_machine, a_shape);
    if (!obj || !a_texture) return false;
    ShapeData* shape = (ShapeData*)obj->m_user;
    const int count = (int)(shape->verts.size() / 2);
    if (count <= 0) return false;
    DrawContext::Mat matrix;
    matrix.a = a_a; matrix.b = a_b; matrix.c = a_c; matrix.d = a_d;
    matrix.e = a_e; matrix.f = a_f; matrix.z = a_z;
    std::vector<float> xy(shape->verts.size());
    std::vector<float> colours((size_t)count * 4);
    for (int i = 0; i < count; ++i) {
        DC_TransformPoint(matrix, shape->verts[i * 2], shape->verts[i * 2 + 1],
                          xy[i * 2], xy[i * 2 + 1]);
        colours[i * 4] = colours[i * 4 + 1] = colours[i * 4 + 2] = 1.0f;
        colours[i * 4 + 3] = a_alpha;
    }
    const float* uv = nullptr;
    std::vector<float> normalizedUV;
    if (shape->uvs.size() == shape->verts.size()) {
        // The script gets physical UV extents from getTextureExt(). The
        // renderer scales logical UVs by those same extents, so divide here
        // once to avoid applying the padding fraction twice.
        float uMax, vMax;
        GetEngineContext().renderer->GetTextureUVExtent(a_texture, uMax, vMax);
        normalizedUV = shape->uvs;
        for (int i = 0; i < count; ++i) {
            normalizedUV[i * 2] /= uMax;
            normalizedUV[i * 2 + 1] /= vMax;
        }
        uv = normalizedUV.data();
    }
    GetEngineContext().renderer->DrawPolygonGradient(shape->glMode, xy.data(),
        colours.data(), uv, count, a_texture, a_z + shape->localZ);
    return true;
}

bool DrawAnimationShapeOverlayWithTransform(gmMachine& a_machine, const gmVariable& a_shape,
                                            unsigned int a_backTexture,
                                            unsigned int a_frontTexture,
                                            float a_frontScaleX, float a_frontScaleY,
                                            float a_alpha,
                                            float a_a, float a_b, float a_c, float a_d,
                                            float a_e, float a_f, float a_z) {
    gmUserObject* obj = AsShapeObject(a_machine, a_shape);
    Renderer* renderer = GetEngineContext().renderer;
    if (!obj || !renderer || !a_backTexture || !a_frontTexture) return false;
    ShapeData* shape = (ShapeData*)obj->m_user;
    const int count = (int)(shape->verts.size() / 2);
    if (count <= 0) return false;

    DrawContext::Mat matrix;
    matrix.a = a_a; matrix.b = a_b; matrix.c = a_c; matrix.d = a_d;
    matrix.e = a_e; matrix.f = a_f; matrix.z = a_z;
    std::vector<float> xy(shape->verts.size());
    for (int i = 0; i < count; ++i)
        DC_TransformPoint(matrix, shape->verts[i * 2], shape->verts[i * 2 + 1],
                          xy[i * 2], xy[i * 2 + 1]);

    std::vector<float> normalizedUV;
    const float* uv = nullptr;
    if (shape->uvs.size() == shape->verts.size()) {
        float backUMax, backVMax;
        renderer->GetTextureUVExtent(a_backTexture, backUMax, backVMax);
        if (backUMax <= 0.0f || backVMax <= 0.0f) return false;
        normalizedUV = shape->uvs;
        for (int i = 0; i < count; ++i) {
            normalizedUV[i * 2] /= backUMax;
            normalizedUV[i * 2 + 1] /= backVMax;
        }
        uv = normalizedUV.data();
    }

    return renderer->DrawPolygonPaletteOverlay(shape->glMode, xy.data(), uv, count,
        a_backTexture, a_frontTexture, a_frontScaleX, a_frontScaleY,
        a_alpha, a_z + shape->localZ);
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
    auto shapeLease = MakeDeferredShapeLease(a_thread, obj);
    // Checked at REPLAY time (inside the lambda), not here -- setTextureByHandle
    // (draw_context.cpp) is itself a recorded op in the SAME command list, so
    // by the time THIS native call runs, any preceding setTextureByHandle in
    // the same script sequence hasn't actually mutated DrawContext yet, only
    // queued a command to do so later. See DC_gmf_setTextureByHandle's own
    // header comment for the live crash (SafeVine.gm:291) this whole texture-
    // binding pair fixes.
    auto draw = [s, shapeLease] {
        DrawShapeData(s);
    };
    // Deferred if called via a real drawList instance -- see draw_list.h's
    // RecordingTarget header comment; matches every other shared drawList
    // primitive (draw_context.cpp).
    if (DrawListData* rec = RecordingTarget(a_thread)) {
        rec->commandBuffer->commands.push_back(draw);
        return GM_OK;
    }
    draw();
    return GM_OK;
}

int GM_CDECL DC_gmf_drawShapeWithTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmUserObject* obj = a_thread->ParamUserObject(0);
    if (!obj || obj->m_userType != g_shapeType) return GM_OK; // same tolerant-null convention as DC_gmf_drawShape
    ShapeData* s = (ShapeData*)obj->m_user;

    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(1));
    const int shaderType = GetTextureShaderType(*a_thread->GetMachine(), a_thread->Param(1));
    auto resourceLease = ResolveTextureResourceLease(*a_thread->GetMachine(), a_thread->Param(1));
    auto shapeLease = MakeDeferredShapeLease(a_thread, obj);

    // Retail FUN_0003f3dc records texture bind (opcode 0xd), selects the
    // texture render config on the supplied shape, queues the shape, then
    // records opcode 0xe to clear the current texture. Keep the selected
    // config on the shape, but scope the texture handle to this draw only.
    // Without the final clear, QUADSHADOWED's following untextured fill quad
    // inherits dropShadowWhite.png and vanishes wherever that image is
    // transparent; this is the missing backing panel in CampaignScreen.
    return DC_RecordOrRun(a_thread, [s, handle, shaderType, resourceLease, shapeLease] {
        s->hasTextureBasedRenderConfig = true;
        s->textureShaderType = shaderType;
        GetDrawContext().currentTextureHandle = handle;
        DrawShapeData(s);
        GetDrawContext().currentTextureHandle = 0;
    });
}

struct TextBubbleAxisSamples {
    float positions[6]{};
    float texels[6]{};
    int count = 4;
    float fullSpan = 0.0f;
};

// Verification: bytecode-translated from FUN_001ad708 (`textbubble_helper.disasm`).
// `vmaddfp` computes the two half-span candidates on the six-sample path;
// `stvewx`/`vsel` branch tables select the four/six position and texel tables.
// Keep `fullSpan` based on the original target: retail computes it before its
// <=0 target fallback changes the temporary interpolation dimension to 1.
TextBubbleAxisSamples BuildTextBubbleAxis(float a_first, float a_middle,
                                          float a_last, float a_target) {
    TextBubbleAxisSamples result;

    // Retail's ordered compare selects the target unless middle > target.
    // This is intentionally written as a branch, not std::max: NaN follows
    // the PPC vcmpgtfp false path.
    const float spanMiddle = a_middle > a_target ? a_middle : a_target;
    result.fullSpan = (a_first + spanMiddle) + a_last;

    float workingTarget = a_target;
    if (!(workingTarget > 0.0f)) workingTarget = 1.0f;

    if (a_middle > 1.0f && a_target > a_middle) {
        result.count = 6;
        const float leftover = a_target - a_middle;
        const float firstMidpoint = std::fma(leftover, 0.5f, a_first);
        const float secondMidpoint = std::fma(leftover, 0.5f, a_middle);
        const float firstTexelBoundary = a_first + a_middle;
        const float finalTexel = firstTexelBoundary + a_last;
        result.positions[0] = 0.0f;
        result.positions[1] = a_first;
        result.positions[2] = firstMidpoint;
        result.positions[3] = firstMidpoint + a_middle;
        result.positions[4] = firstMidpoint + secondMidpoint;
        result.positions[5] = result.fullSpan;
        result.texels[0] = 0.0f;
        result.texels[1] = a_first;
        result.texels[2] = a_first;
        result.texels[3] = firstTexelBoundary;
        result.texels[4] = firstTexelBoundary;
        result.texels[5] = finalTexel;
    } else {
        const float middleExtent = workingTarget > a_middle
            ? workingTarget : a_middle;
        const float secondTexelBoundary = a_first + a_middle;
        result.positions[0] = 0.0f;
        result.positions[1] = a_first;
        result.positions[2] = a_first + middleExtent;
        result.positions[3] = result.fullSpan;
        result.texels[0] = 0.0f;
        result.texels[1] = a_first;
        result.texels[2] = secondTexelBoundary;
        result.texels[3] = secondTexelBoundary + a_last;
    }
    return result;
}

// Verification: bytecode-matched wrapper and helper (`0x001b8240` and
// triangle strip, including 4/6 sample axes, original-target full spans,
// zero-dimension interpolation fallback, logical-image UV conversion, and
// texture padding extents. Runtime pixel parity remains unverified.
int GM_CDECL gmf_VIDEOHELPER_CreateTextBubble(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(9);
    gmMachine& machine = *a_thread->GetMachine();
    Renderer* renderer = GetEngineContext().renderer;
    const unsigned int texture = ResolveTextureHandle(machine, a_thread->Param(0));
    int textureWidth = 0;
    int textureHeight = 0;
    float textureUScale = 1.0f;
    float textureVScale = 1.0f;
    if (renderer && texture) {
        renderer->GetTextureDimensions(texture, textureWidth, textureHeight);
        renderer->GetTextureUVExtent(texture, textureUScale, textureVScale);
    }

    // The PPC wrapper converts each GM float unchanged and each other value
    // through its signed integer payload before vcfsx. Real scripts supply
    // integers or floats; preserving this native union behavior also keeps
    // invalid-type behavior close to the executable's conversion path.
    auto scalar = [a_thread](int index) {
        const gmVariable& value = a_thread->Param(index);
        return value.m_type == GM_FLOAT ? value.m_value.m_float
                                        : (float)value.m_value.m_int;
    };
    const TextBubbleAxisSamples x = BuildTextBubbleAxis(
        scalar(1), scalar(2), scalar(3), scalar(7));
    const TextBubbleAxisSamples y = BuildTextBubbleAxis(
        scalar(4), scalar(5), scalar(6), scalar(8));

    ShapeData* data = new ShapeData();
    data->glMode = GL_TRIANGLE_STRIP;
    data->capacity = 2 * x.count * y.count - 2 * x.count + 2;

    struct GridPoint { float x, y, u, v; };
    std::vector<GridPoint> grid;
    grid.reserve((size_t)x.count * (size_t)y.count);
    for (int row = 0; row < y.count; ++row) {
        for (int column = 0; column < x.count; ++column) {
            const float pixelU = x.texels[column];
            const float pixelV = y.texels[row];
            const float u = textureWidth > 0
                ? (pixelU / (float)textureWidth) * textureUScale : 0.0f;
            const float v = textureHeight > 0
                ? std::fma(-(pixelV / (float)textureHeight), textureVScale,
                           textureVScale)
                : 0.0f;
            // FUN_001ad708 computes reciprocal full-span values with vrefp
            // and multiplies these pixel-space positions by them before
            // storing the mesh (see the normalization loop near 0x1ada84).
            // QuadShadowed then scales this unit-space mesh to its panel
            // dimensions; keeping pixel coordinates here applies that scale
            // twice and pushes the backing panel off-screen.
            const float normalizedX = x.fullSpan != 0.0f
                ? x.positions[column] / x.fullSpan : 0.0f;
            const float normalizedY = y.fullSpan != 0.0f
                ? y.positions[row] / y.fullSpan : 0.0f;
            grid.push_back({normalizedX, 1.0f - normalizedY, u, v});
        }
    }

    const int rowPairCount = x.count * (y.count - 1);
    data->verts.reserve((size_t)rowPairCount * 4);
    data->uvs.reserve((size_t)rowPairCount * 4);
    for (int point = 0; point < rowPairCount; ++point) {
        const GridPoint pair[] = {grid[point], grid[point + x.count]};
        for (const GridPoint& vertex : pair) {
            data->verts.push_back(vertex.x);
            data->verts.push_back(vertex.y);
            data->uvs.push_back(vertex.u);
            data->uvs.push_back(vertex.v);
        }
    }
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
}
