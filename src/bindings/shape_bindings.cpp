#include "gmThread.h"
#include "shape_bindings.h"
#include "vector2d_bindings.h"
#include "../render/draw_context.h"
#include "../render/draw_list.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "texture_bindings.h"
#include "process_handle_bindings.h"
#include "colour_bindings.h"
#include "../ported_native/ground_math.h"
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
    // Some retail builders pass internal flag bits alongside the primitive
    // byte (e.g. rounded-rect outline uses raw mode 0x102). VMSHAPE.endShape
    // masks the same value to its low byte before indexing the retail tables.
    switch ((ShapeType)(static_cast<unsigned int>(a_shapeType) & 0xffu)) {
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
    std::vector<float> colors;
    // Per-vertex uv, aligned 1:1 with verts -- populated by getVertex(i)'s
    // live SHAPEVERTEX handle (PekkoAndJon/bg.gm's update(): `this.
    // shapePoints[i].vertex.u = 0.5 + cos(ang)*this.amt;` etc, animating the
    // UV radius each frame to "zoom" the sampled texture outward). Without
    // real per-vertex UVs, DrawPolygonGradient derives UVs from the mesh
    // bounds, which is a flat single-texture-copy stretch, not the animated
    // radial sampling that produces bg.gm's kaleidoscope look.
    std::vector<float> uvs;
    float localZ = 0.0f;
    // True right after a completed draw (endShape or drawShape), until the
    // next plotVertex call -- see gmf_plotVertex's comment.
    bool needsClearOnNextPlot = false;

    int capacity = 0;
    // VIDEO.beginShape's third argument initializes native constructor
    // field +0x24. Default 0x14; the 3-argument overload stores arg2 << 2.
    int retailConstructorField24 = 0x14;
    int retailShapeType = 0;
    int retailEndShapeField4 = 0;
    int retailEndShapeField8 = 0;
    // Native player shapes allocate 0x14 + capacity*initialStride bytes, then
    // their per-frame XYZ appender changes the live vertex stride to 12.
    int retailBufferBytes = 0;
    int retailVertexStride = 0;
};

ShapeData* ThisShape(gmThread* a_thread) {
    ShapeData* shape = (ShapeData*)a_thread->ThisUserCheckType(g_shapeType);
    return shape && shape->hasDirectReference ? shape : nullptr;
}

// Compatibility current-shape slot used by the DrawList extension handlers
// and plotVertices helper. Retail VIDEO.beginShape does not write this global;
// its exact handler below only returns the newly allocated VMSHAPE.
ShapeData* g_currentShape = nullptr;

void DrawShapeData(ShapeData* s, bool a_defaultTransVertexOrder = false) {
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
        if (a_defaultTransVertexOrder) {
            // DefaultTrans_v evaluates its affine position contribution in
            // this order: y*column1, x*column0 + r0, z*column2 + r0,
            // column3 + r0. Player VMSHAPEs store local z=0 and this 2D
            // stack has no x/y contribution from z. Keep the y product as
            // the initial partial, use an explicit fused MAD for x, then add
            // translation separately. The active view/projection is still
            // applied by Renderer::DrawPolygon after this model-space step.
            const DrawContext::Mat& matrix = ctx.Top();
            const float xPartial = std::fma(x, matrix.a, y * matrix.c);
            const float yPartial = std::fma(x, matrix.b, y * matrix.d);
            transformed[i * 2] = matrix.e + xPartial;
            transformed[i * 2 + 1] = matrix.f + yPartial;
        } else {
            DC_TransformPoint(ctx.Top(), x, y,
                              transformed[i * 2], transformed[i * 2 + 1]);
        }
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
    // A VMSHAPE's saved colour samples can all be identical even though the
    // draw-list supplies its actual flat colour later. This occurs for
    // SwayingTree.gm: the stem mesh is built during configure(), then drawn
    // under a deferred COLOURSCHEME_SetColour("bg_out", drawList). Reusing
    // the renderer colour from mesh construction bakes a stale value (often
    // black) into every vertex and multiplying it by the draw-list colour
    // still produces black. A uniform vertex stream carries no gradient, so
    // let the current draw-list colour provide the flat shader input, as it
    // does for shapes built by the native VideoHelper path.
    const bool hasCapturedColours =
        s->colors.size() == static_cast<size_t>(count) * 4;
    bool uniformCapturedColour = hasCapturedColours;
    if (uniformCapturedColour) {
        for (int i = 1; i < count && uniformCapturedColour; ++i) {
            for (int c = 0; c < 4; ++c) {
                if (s->colors[static_cast<size_t>(i) * 4 + c] != s->colors[c]) {
                    uniformCapturedColour = false;
                    break;
                }
            }
        }
    }
    if (count == 34 && s->glMode == GL_TRIANGLE_STRIP &&
        std::getenv("GE3_TRACE_SWAYING_TREE_COLOUR")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 64) {
            float r, g, b, a;
            GetEngineContext().renderer->GetColour(r, g, b, a);
            const bool flatColourPath =
                s->shaderEntityId < 0 && tex == 0 && uniformCapturedColour;
            std::fprintf(stderr,
                "[sway-tree-colour] shape scheme=%d vertices=%d texture=%u flat=%d captured=(%.4f,%.4f,%.4f,%.4f) draw=(%.4f,%.4f,%.4f,%.4f)\n",
                GetCurrentColourSchemeID(), count, tex, flatColourPath,
                s->colors.empty() ? 0.0f : s->colors[0],
                s->colors.size() < 2 ? 0.0f : s->colors[1],
                s->colors.size() < 3 ? 0.0f : s->colors[2],
                s->colors.size() < 4 ? 0.0f : s->colors[3], r, g, b, a);
        }
    }
    if (s->shaderEntityId < 0 && tex == 0 && uniformCapturedColour) {
        GetEngineContext().renderer->DrawPolygon(
            s->glMode, transformed.data(), count, ctx.Top().z + s->localZ);
        return;
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
    if (a_defaultTransVertexOrder) {
        GetEngineContext().renderer->DrawPolygonDefaultTrans(
            s->glMode, transformed.data(), count, ctx.Top().z + s->localZ);
    } else {
        GetEngineContext().renderer->DrawPolygon(
            s->glMode, transformed.data(), count, ctx.Top().z + s->localZ);
    }
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
    data->capacity = 0x40;
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
    const bool textureUV = p0.m_type != g_vector2DType &&
        a_thread->GetNumParams() >= 5 && s->shaderEntityType.empty();
    s->uvs.push_back(textureUV ? data3 : 0.5f);
    s->uvs.push_back(textureUV ? data4 : 0.5f);
    s->localZ = localZ;
    return GM_OK;
}

int GM_CDECL gmf_VMSHAPE_plotVertex(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    return PlotVertexOn(a_thread, s);
}
int GM_CDECL gmf_VMSHAPE_endShape(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
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

int GM_CDECL gmf_VMSHAPE_releaseReference(gmThread* a_thread) {
    // Keep this operation idempotent: retail clears the held pointer, then
    // a repeated release observes that it is already null.
    ShapeData* s = (ShapeData*)a_thread->ThisUserCheckType(g_shapeType);
    if (!s) return GM_EXCEPTION;
    s->hasDirectReference = false;
    return GM_OK;
}

int GM_CDECL gmf_setAsFilledCircle(gmThread* a_thread) {
    ShapeData* s = ThisShape(a_thread);
    if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(radius, 0, 1.0f);

    int quality = s->capacity - 1; // ring point count, incl. closing duplicate
    if (quality < 2) quality = 2;
    int segments = quality - 1;    // q in the dead code's terms
    if (segments < 1) segments = 1;
    const float angInc = 6.28318530718f / (float)segments;

    s->verts.clear();
    s->vertexData.clear();
    s->verts.reserve((size_t)(1 + quality) * 2);
    s->verts.push_back(0.0f);
    s->verts.push_back(0.0f); // centre, index 0
    float ang = 0.0f;
    for (int j = 0; j < quality; ++j) {
        s->verts.push_back(
            GroundRenderMath::RetailGroundSinCos(ang, true) * radius);
        s->verts.push_back(
            GroundRenderMath::RetailGroundSinCos(ang, false) * radius);
        ang += angInc;
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

gmVariable CreateShapeFromVertsAndUVs(gmMachine& a_machine, int a_shapeType,
                                      const float* a_xy, const float* a_uv,
                                      int a_count, float a_z) {
    if (g_shapeType < 0 || a_count < 0 ||
        (a_count > 0 && (!a_xy || !a_uv))) return gmVariable::s_null;
    auto* object = a_machine.AllocUserObject(new ShapeData(), g_shapeType);
    if (!object) return gmVariable::s_null;
    auto* data = static_cast<ShapeData*>(object->m_user);
    if (!data) return gmVariable::s_null;
    data->retailShapeType = a_shapeType;
    data->glMode = ToGLMode(a_shapeType);
    data->capacity = a_count;
    data->localZ = a_z;
    data->retailEndShapeField4 = 7; // RSX triangle-fan primitive
    data->retailEndShapeField8 = 0x1b02; // filled polygon mode
    if (a_count > 0) {
        data->verts.assign(a_xy, a_xy + (size_t)a_count * 2);
        data->uvs.assign(a_uv, a_uv + (size_t)a_count * 2);
    }
    return gmVariable(object);
}

gmVariable UpdateNativeOwnedShape(gmMachine& a_machine, gmTableObject* a_owner,
                                  const char* a_fieldName, int a_shapeType,
                                  const float* a_xy, int a_count, float a_z,
                                  int a_nativeCapacity,
                                  int a_nativeAllocationStride) {
    if (!a_owner || !a_fieldName || g_shapeType < 0) return gmVariable();
    gmVariable value = a_owner->Get(&a_machine, a_fieldName);
    gmUserObject* object = AsShapeObject(a_machine, value);
    if (!object) {
        object = a_machine.AllocUserObject(new ShapeData(), g_shapeType);
        if (!object) return gmVariable();
        value = gmVariable(object);
        a_owner->Set(&a_machine, a_fieldName, value);
    }

    ShapeData* data = static_cast<ShapeData*>(object->m_user);
    if (!data) return gmVariable();
    data->retailShapeType = a_shapeType;
    data->glMode = ToGLMode(a_shapeType);
    const int liveCount = std::max(a_count, 0);
    data->capacity = a_nativeCapacity >= 0 ? a_nativeCapacity : liveCount;
    const int hostReserveCount = std::max(liveCount, data->capacity);
    if (hostReserveCount > 0 &&
        data->verts.capacity() < static_cast<size_t>(hostReserveCount) * 2)
        data->verts.reserve(static_cast<size_t>(hostReserveCount) * 2);
    data->retailBufferBytes = a_nativeCapacity >= 0
        ? 0x14 + a_nativeCapacity * std::max(a_nativeAllocationStride, 0)
        : 0;
    data->retailVertexStride = a_nativeCapacity >= 0 ? 12 : 0;
    data->localZ = a_z;
    if (a_xy && a_count > 0)
        data->verts.assign(a_xy, a_xy + (size_t)a_count * 2);
    else
        data->verts.clear();
    data->vertexData.clear();
    data->colors.clear();
    data->uvs.clear();
    data->needsClearOnNextPlot = false;
    data->shaderEntityId = -1;
    data->shaderSlot = 0;
    data->shaderEntityType.clear();
    data->hasTextureBasedRenderConfig = false;
    data->textureShaderType = 0;
    return value;
}

std::shared_ptr<void> RetainNativeShapeForDeferredDraw(
    gmMachine& a_machine, const gmVariable& a_shape) {
    gmUserObject* object = AsShapeObject(a_machine, a_shape);
    if (!object) return {};
    gmObject* managed = a_machine.GetObject(a_shape.m_value.m_ref);
    if (!managed || managed != static_cast<gmObject*>(object)) return {};
    return std::static_pointer_cast<void>(
        std::make_shared<DeferredShapeLease>(a_machine, managed));
}

gmUserObject* AsShapeObject(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type < GM_USER) return nullptr;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref);
    if (!obj || obj->m_userType != g_shapeType) return nullptr;
    return obj;
}

bool DrawShapeObjectWithTransform(gmMachine& a_machine, const gmVariable& a_shape,
                                  float a_a, float a_b, float a_c, float a_d,
                                  float a_e, float a_f, float a_z,
                                  bool a_retailPpcMatrixMath) {
    gmUserObject* obj = AsShapeObject(a_machine, a_shape);
    if (!obj || !obj->m_user) return false;
    ShapeData* shape = static_cast<ShapeData*>(obj->m_user);
    if (!shape->hasDirectReference || shape->verts.empty()) return false;

    DrawContext& context = GetDrawContext();
    const DrawContext::Mat previousMatrix = context.Top();
    const unsigned int previousTexture = context.currentTextureHandle;
    const bool previousDistanceTexture = context.distanceTextureEnabled;
    const float previousDiscard = context.distanceTextureDiscardLevel;
    const float previousSmooth = context.distanceTextureSmoothLevel;
    struct RestoreDrawContext {
        DrawContext& context;
        DrawContext::Mat matrix;
        unsigned int texture;
        bool distanceTexture;
        float discard;
        float smooth;
        ~RestoreDrawContext() {
            context.Top() = matrix;
            context.currentTextureHandle = texture;
            context.distanceTextureEnabled = distanceTexture;
            context.distanceTextureDiscardLevel = discard;
            context.distanceTextureSmoothLevel = smooth;
        }
    } restore{context, previousMatrix, previousTexture,
              previousDistanceTexture, previousDiscard, previousSmooth};

    DrawContext::Mat operation;
    operation.a = a_a;
    operation.b = a_b;
    operation.c = a_c;
    operation.d = a_d;
    operation.e = a_e;
    operation.f = a_f;
    operation.z = a_z;
    if (a_retailPpcMatrixMath) {
        const auto roundedProduct = [](float lhs, float rhs) {
            return std::fma(lhs, rhs, 0.0f);
        };
        DrawContext::Mat result;
        result.a = roundedProduct(previousMatrix.a, operation.a) +
                   roundedProduct(previousMatrix.c, operation.b);
        result.b = roundedProduct(previousMatrix.b, operation.a) +
                   roundedProduct(previousMatrix.d, operation.b);
        result.c = roundedProduct(previousMatrix.a, operation.c) +
                   roundedProduct(previousMatrix.c, operation.d);
        result.d = roundedProduct(previousMatrix.b, operation.c) +
                   roundedProduct(previousMatrix.d, operation.d);
        result.e = roundedProduct(previousMatrix.a, operation.e) +
                   std::fma(previousMatrix.c, operation.f, previousMatrix.e);
        result.f = roundedProduct(previousMatrix.b, operation.e) +
                   std::fma(previousMatrix.d, operation.f, previousMatrix.f);
        result.z = previousMatrix.z + operation.z;
        context.Top() = result;
    } else {
        context.Top() = DC_Multiply(previousMatrix, operation);
    }
    // Native player particle meshes are untextured and use the current
    // named color. Do not let an unrelated Video texture/render-config state
    // leak into this native shape submission.
    context.currentTextureHandle = 0;
    context.distanceTextureEnabled = false;
    DrawShapeData(shape, a_retailPpcMatrixMath);
    return true;
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
    g_shapeType = a_machine.CreateUserType("VMSHAPE");
    a_machine.RegisterTypeLibrary(g_shapeType, s_shapeLib, sizeof(s_shapeLib) / sizeof(s_shapeLib[0]));
    a_machine.RegisterUserCallbacks(g_shapeType, nullptr, ShapeGCDestruct, nullptr);

    // SHAPEVERTEX -- see its header comment (above gmf_VMSHAPE_getVertex).
    g_shapeVertexType = a_machine.CreateUserType("SHAPEVERTEX");
    a_machine.RegisterUserCallbacks(g_shapeVertexType, nullptr, ShapeVertexGCDestruct, nullptr);
    a_machine.RegisterTypeOperator(g_shapeVertexType, O_GETDOT, nullptr, ShapeVertexGetDot);
    a_machine.RegisterTypeOperator(g_shapeVertexType, O_SETDOT, nullptr, ShapeVertexSetDot);

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
