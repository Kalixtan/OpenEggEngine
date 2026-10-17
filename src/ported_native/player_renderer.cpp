#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstdint>
#include <string>
#include <set>
#include "player_renderer.h"
#include "ground_math.h"
#include "../core/engine_context.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/physics_body_bindings.h"
#include "../bindings/colour_bindings.h"
#include "../bindings/vector2d_bindings.h"
#include "../bindings/shape_bindings.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <box2d/box2d.h>
#include <GL/glew.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

// Player geometry is being ported from retail PPC FUN_001a1660.
// GameMonkey fields are read as native renderer inputs, not as renderer
// behavior. The particle pass is ported from native FUN_001a0be8 using
// headless Ghidra evidence. Every field below (radius,
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

// Retail builds the player-owned draw list during the entity traversal, then
// replays its recorded commands later at the sorted key position. Capture
// the desktop equivalents at that same point; their closures run only during
// sorted replay. The command buffer remains owned by the returned submission.
thread_local DrawListData::CommandBuffer* g_playerCommandRecorder = nullptr;

void RecordPlayerCommand(std::function<void()> a_command) {
    if (g_playerCommandRecorder)
        g_playerCommandRecorder->commands.emplace_back(std::move(a_command));
}

class PlayerCommandRecordingScope {
public:
    explicit PlayerCommandRecordingScope(
        DrawListData::CommandBuffer& a_buffer)
        : m_previous(g_playerCommandRecorder) {
        g_playerCommandRecorder = &a_buffer;
    }
    ~PlayerCommandRecordingScope() {
        g_playerCommandRecorder = m_previous;
    }
    PlayerCommandRecordingScope(const PlayerCommandRecordingScope&) = delete;
    PlayerCommandRecordingScope& operator=(
        const PlayerCommandRecordingScope&) = delete;

private:
    DrawListData::CommandBuffer* m_previous;
};

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
float GetFloatField(gmMachine& a_m, gmTableObject* a_t, const char* a_key, float a_default);
double GetDoubleField(gmMachine& a_m, gmTableObject* a_t, const char* a_key, double a_default);

// Verification: retail helper 0x34864c loads vec2.y first, computes y*y,
// then executes fmadds(x, x, ySquared) and fsqrts. Keep this separate from
// the ground renderer helper, whose PPC call sites use the opposite fused
// operand order. This distinction can change the final binary32 bit pattern.
static float RetailPlayerVectorLength(float a_x, float a_y) {
    const float ySquared = a_y * a_y;
    const float squaredLength = std::fma(a_x, a_x, ySquared);
    return std::sqrt(squaredLength);
}

// Verification: retail helper 0x3486b8 loads x/y, computes the same
// y-squared then fused x-squared length, and divides x and y separately by
// that length (`fdivs x,length`; `fdivs y,length`). It explicitly returns a
// zero vector for zero length. Do not replace these divisions with one
// reciprocal followed by multiplies; those round differently.
static b2Vec2 RetailPlayerNormalize(float a_x, float a_y) {
    const float length = RetailPlayerVectorLength(a_x, a_y);
    if (length == 0.0f) return b2Vec2(0.0f, 0.0f);
    return b2Vec2(a_x / length, a_y / length);
}

// Verification: retail helper 0x348670 computes the median of its three
// arguments using seven binary32 subtract/fsel operations. Match their order
// so boundary ties and signed zero follow PPC instead of std::clamp's rule.
static float RetailPlayerClamp(float a_value, float a_bound0, float a_bound1) {
    const float lowerDelta = a_bound0 - a_value;
    const float upperDelta = a_bound1 - a_value;
    const float boundsDelta0 = a_bound0 - a_bound1;
    const float boundsDelta1 = a_bound0 - a_bound1;
    const float lowerOrValue = lowerDelta >= 0.0f ? a_bound0 : a_value;
    const float upperOrValue = upperDelta >= 0.0f ? a_bound1 : a_value;
    const float upperBound = boundsDelta0 >= 0.0f ? a_bound0 : a_bound1;
    const float outer = boundsDelta1 >= 0.0f ? upperOrValue : lowerOrValue;
    const float separation = upperBound - outer;
    return separation >= 0.0f ? outer : upperBound;
}

static bool IsPlayerDead(gmMachine& a_machine, gmTableObject* a_player) {
    return static_cast<int>(GetFloatField(a_machine, a_player, "dead", 0.0f)) != 0;
}

static const char* AuraColourName(gmMachine& a_machine, gmTableObject* a_player) {
    if (IsPlayerDead(a_machine, a_player)) return "danger_fill";
    const bool sticky = static_cast<int>(GetFloatField(
        a_machine, a_player, "stickyOn", 0.0f)) == 1;
    return sticky ? "ground_fill" : "ground_noClimb_fill";
}

static const char* BodyColourName(gmMachine& a_machine, gmTableObject* a_player) {
    return IsPlayerDead(a_machine, a_player) ? "danger_fill" : "player_fill";
}

bool ResolveColourByName(const char* a_name, float& a_r, float& a_g, float& a_b) {
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
        return false;
    }
    a_r = r;
    a_g = g;
    a_b = b;
    return true;
}

void ApplyColourByName(Renderer& a_renderer, const char* a_name) {
    float r, g, b;
    if (ResolveColourByName(a_name, r, g, b))
        a_renderer.SetColour(r, g, b, 1.0f);
}

void SetColourByName(Renderer& a_renderer, const char* a_name) {
    if (!g_playerCommandRecorder) {
        ApplyColourByName(a_renderer, a_name);
        return;
    }
    // Retail FUN_001d11e8 resolves the colour during native draw-list
    // construction, then FUN_0003da60 copies {r,g,b,alpha} into the list's
    // work buffer under opcode 0x0b. Capture those resolved values now;
    // resolving the name from a deferred closure could observe a later colour
    // scheme change than retail would.
    float r, g, b;
    if (!ResolveColourByName(a_name, r, g, b)) return;
    Renderer* renderer = &a_renderer;
    RecordPlayerCommand([renderer, r, g, b] {
        renderer->SetColour(r, g, b, 1.0f);
    });
}

// The Video frame pass records DefaultTrans as the live fallback render
// config (Video+0x8120). Player-owned VMSHAPEs have no shape-local config, so
// opcode 6 binds this fallback before drawing them. Ghidra's static fragment
// descriptor has blend flag 1; FUN_000548b8 maps that to SRC_ALPHA /
// ONE_MINUS_SRC_ALPHA for RGB and alpha, additive equations, blending enabled,
// and logic/line/polygon smoothing disabled. Keep the state change in the
// deferred stream so it runs at the player's sorted replay position, before
// its particle and retained-shape submissions. The fixed-function path below
// supplies equivalent operations: current colour to the varying, then the
// retail y*c257, x*c256+r0, z*c258+r0, c259+r0 position sequence, and
// interpolated colour to fragment output. This is semantic equivalence only;
// host GL matrix arithmetic/rounding and raster output have not been matched
// to the RSX program.
void ApplyPlayerDefaultTransState() {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glEnable(GL_BLEND);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
}

void RecordPlayerDefaultTransState() {
    if (g_playerCommandRecorder) {
        RecordPlayerCommand([] { ApplyPlayerDefaultTransState(); });
    } else {
        ApplyPlayerDefaultTransState();
    }
}

// Verification: retail FUN_001a1660 draws particles before its scale cutoff,
// then calls FUN_003484f4 at 0x1a18d4 to emit state-save opcode 0x1a and emits
// restore opcode 0x1b at 0x1a2b78 only on the non-cutoff path. FUN_00044d4c
// dispatches these through the native render-state stack. FUN_0030a414
// reapplies the saved stencil function/operations, depth compare/write,
// colour mask, and front-face/culling configuration; its non-null path enables
// stencil and depth testing. The desktop adapter restores the captured
// stencil enable and tuple; this matches the disabled GL state in the observed
// `VIDEOPARAMS_DISABLE` case. Whether that retail path saves a null/default
// tuple or an explicit always-pass tuple remains unverified. It also restores
// the fixed-function matrices changed by immediate rendering. See
// and player_renderer_state_restore_20261010.md. The player entry inherits
// the active view. Construct this scope after particles and the scale cutoff:
// particles are outside retail's saved scope, and the cutoff has no restore.
class NativePlayerRenderStateScope {
public:
    NativePlayerRenderStateScope()
        : m_stencilEnabled(glIsEnabled(GL_STENCIL_TEST)),
          m_cullingEnabled(glIsEnabled(GL_CULL_FACE)) {
        glGetIntegerv(GL_DEPTH_FUNC, &m_depthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &m_depthWrite);
        glGetBooleanv(GL_COLOR_WRITEMASK, m_colourMask);
        CaptureStencilFace(GL_FRONT, m_stencilFront);
        CaptureStencilFace(GL_BACK, m_stencilBack);
        glGetIntegerv(GL_CULL_FACE_MODE, &m_cullFaceMode);
        glGetIntegerv(GL_MATRIX_MODE, &m_matrixMode);
        glGetFloatv(GL_PROJECTION_MATRIX, m_projectionMatrix);
        glMatrixMode(GL_MODELVIEW);
        glGetFloatv(GL_MODELVIEW_MATRIX, m_modelViewMatrix);
        glMatrixMode(static_cast<GLenum>(m_matrixMode));
    }

    ~NativePlayerRenderStateScope() {
        // FUN_0030a414 disables stencil for the null/default state and enables
        // it when restoring an explicit stencil tuple. In this desktop path,
        // the `VIDEOPARAMS_DISABLE` state has GL stencil disabled at scope
        // entry; preserve that bit so stale GL compare values (for example
        // EQUAL/ref 8) cannot become active. Whether retail saves a null/default
        // or explicit always-pass tuple here remains unverified.
        if (m_stencilEnabled) glEnable(GL_STENCIL_TEST);
        else glDisable(GL_STENCIL_TEST);
        RestoreStencilFace(GL_FRONT, m_stencilFront);
        RestoreStencilFace(GL_BACK, m_stencilBack);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(static_cast<GLenum>(m_depthFunc));
        glDepthMask(m_depthWrite);
        glColorMask(m_colourMask[0], m_colourMask[1],
                    m_colourMask[2], m_colourMask[3]);
        // RSX method 0x41834 is FRONT_FACE and the restore helper always
        // writes CCW (0x901). Its tuple selector 0 writes BACK + disabled,
        // 1 writes BACK + enabled, 2 writes FRONT + enabled, and >=3 leaves
        // cull mode/enable untouched. The selector comes from the live
        // DrawContext state at sorted replay, while actual GL state is saved
        // for the >=3 no-op case.
        glFrontFace(GL_CCW);
        if (m_savedCullSelector == 0) {
            glCullFace(GL_BACK);
            glDisable(GL_CULL_FACE);
        } else if (m_savedCullSelector == 1) {
            glCullFace(GL_BACK);
            glEnable(GL_CULL_FACE);
        } else if (m_savedCullSelector == 2) {
            glCullFace(GL_FRONT);
            glEnable(GL_CULL_FACE);
        } else {
            glCullFace(static_cast<GLenum>(m_cullFaceMode));
            if (m_cullingEnabled) glEnable(GL_CULL_FACE);
            else glDisable(GL_CULL_FACE);
        }
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(m_projectionMatrix);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(m_modelViewMatrix);
        glMatrixMode(static_cast<GLenum>(m_matrixMode));
    }

    NativePlayerRenderStateScope(const NativePlayerRenderStateScope&) = delete;
    NativePlayerRenderStateScope& operator=(const NativePlayerRenderStateScope&) = delete;

private:
    struct StencilFaceState {
        GLint function = GL_ALWAYS;
        GLint reference = 0;
        GLint valueMask = 0xffffffff;
        GLint fail = GL_KEEP;
        GLint depthFail = GL_KEEP;
        GLint depthPass = GL_KEEP;
    };

    static void CaptureStencilFace(GLenum a_face, StencilFaceState& a_state) {
        const bool front = a_face == GL_FRONT;
        glGetIntegerv(front ? GL_STENCIL_FUNC : GL_STENCIL_BACK_FUNC,
                      &a_state.function);
        glGetIntegerv(front ? GL_STENCIL_REF : GL_STENCIL_BACK_REF,
                      &a_state.reference);
        glGetIntegerv(front ? GL_STENCIL_VALUE_MASK : GL_STENCIL_BACK_VALUE_MASK,
                      &a_state.valueMask);
        glGetIntegerv(front ? GL_STENCIL_FAIL : GL_STENCIL_BACK_FAIL,
                      &a_state.fail);
        glGetIntegerv(front ? GL_STENCIL_PASS_DEPTH_FAIL : GL_STENCIL_BACK_PASS_DEPTH_FAIL,
                      &a_state.depthFail);
        glGetIntegerv(front ? GL_STENCIL_PASS_DEPTH_PASS : GL_STENCIL_BACK_PASS_DEPTH_PASS,
                      &a_state.depthPass);
    }

    static void RestoreStencilFace(GLenum a_face, const StencilFaceState& a_state) {
        glStencilFuncSeparate(a_face, static_cast<GLenum>(a_state.function),
                              a_state.reference,
                              static_cast<GLuint>(a_state.valueMask));
        glStencilOpSeparate(a_face, static_cast<GLenum>(a_state.fail),
                            static_cast<GLenum>(a_state.depthFail),
                            static_cast<GLenum>(a_state.depthPass));
    }

    GLboolean m_cullingEnabled;
    GLboolean m_stencilEnabled;
    uint32_t m_savedCullSelector = static_cast<uint32_t>(GetDrawContext().culling);
    GLint m_depthFunc = GL_LEQUAL;
    GLboolean m_depthWrite = GL_TRUE;
    GLboolean m_colourMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    StencilFaceState m_stencilFront;
    StencilFaceState m_stencilBack;
    GLint m_cullFaceMode = GL_BACK;
    GLint m_matrixMode = GL_MODELVIEW;
    GLfloat m_projectionMatrix[16] = {};
    GLfloat m_modelViewMatrix[16] = {};
};

struct DeferredPlayerRenderState {
    std::unique_ptr<NativePlayerRenderStateScope> scope;
};

void SubmitPlayerPolygon(Renderer& a_renderer, GLenum a_mode,
                         const float* a_xy, int a_count, float a_z) {
    if (a_count <= 0) return;
    if (!g_playerCommandRecorder) {
        a_renderer.DrawPolygon(a_mode, a_xy, a_count, a_z);
        return;
    }
    Renderer* renderer = &a_renderer;
    std::vector<float> vertices(a_xy, a_xy + (size_t)a_count * 2);
    RecordPlayerCommand([renderer, a_mode, vertices = std::move(vertices),
                         a_count, a_z] {
        renderer->DrawPolygon(a_mode, vertices.data(), a_count, a_z);
    });
}

void DrawFilledFan(Renderer& a_renderer, const std::vector<b2Vec2>& a_verts,
                   float a_z) {
    if (a_verts.size() < 3) return;
    std::vector<float> xy;
    xy.reserve(a_verts.size() * 2);
    for (const b2Vec2& v : a_verts) { xy.push_back(v.x); xy.push_back(v.y); }
    SubmitPlayerPolygon(a_renderer, GL_TRIANGLE_FAN, xy.data(),
                        (int)a_verts.size(), a_z);
}

std::vector<float> FlattenPlayerVertices(const std::vector<b2Vec2>& a_vertices) {
    std::vector<float> xy;
    xy.reserve(a_vertices.size() * 2);
    for (const b2Vec2& vertex : a_vertices) {
        xy.push_back(vertex.x);
        xy.push_back(vertex.y);
    }
    return xy;
}

// Verification: retail FUN_00044d4c interprets player opcodes 0x13/0x14 as
// current-matrix multiplied by translation/rotation, and 0x17/0x18 as a
// 64-byte matrix push/pop. See SOUND_SHAPES_GROUND_TRUTH.md's player
// transform trace and player_drawlist_matrix_ops_capstone_20261010.txt.
// This compact 2D affine stack applies those same operations to the native
// player's planar shapes before handing their vertices to the desktop GL
// renderer; it is intentionally local to the player renderer.
struct PlayerMatrix2D {
    float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f;
    float x = 0.0f, y = 0.0f;

    // Verification: FUN_0030ab94 uses `vmaddfp product, lhs, scalar, zero`
    // for each matrix-product term, then `vaddfp` combines the two partials.
    // An explicit fma with a zero addend rounds each binary32 product before
    // the separate sum, independent of host compiler contraction settings.
    static float RetailRoundedProduct(float a_lhs, float a_rhs) {
        return std::fma(a_lhs, a_rhs, 0.0f);
    }

    PlayerMatrix2D Multiplied(const PlayerMatrix2D& inner) const {
        PlayerMatrix2D out;
        // Keep the PPC VMX product rounding points and final vaddfp distinct.
        out.a = RetailRoundedProduct(a, inner.a) +
                RetailRoundedProduct(c, inner.b);
        out.b = RetailRoundedProduct(b, inner.a) +
                RetailRoundedProduct(d, inner.b);
        out.c = RetailRoundedProduct(a, inner.c) +
                RetailRoundedProduct(c, inner.d);
        out.d = RetailRoundedProduct(b, inner.c) +
                RetailRoundedProduct(d, inner.d);
        // FUN_0030ab94 builds the translation column as two VMX partial sums:
        // col0*tx + (col1*ty + col3*1), then adds those partials. Preserve
        // the fused col1/col3 operation and the final add for retail rounding.
        out.x = (a * inner.x) + std::fma(c, inner.y, x);
        out.y = (b * inner.x) + std::fma(d, inner.y, y);
        return out;
    }

    b2Vec2 Transform(float px, float py) const {
        return b2Vec2(a * px + c * py + x, b * px + d * py + y);
    }
};

class PlayerTransformStack {
public:
    explicit PlayerTransformStack(const b2Vec2& origin) {
        PlayerMatrix2D root;
        root.x = origin.x;
        root.y = origin.y;
        m_stack.push_back(root);
    }

    // Verification: opcode 0x17 copies all 64 bytes of the active matrix to
    // the next slot; the native player balances each run-shape push with 0x18.
    void Push() { m_stack.push_back(m_stack.back()); }

    // Verification: opcode 0x18 decrements the native stack depth when it is
    // nonzero; all player call sites pop a preceding push.
    void Pop() { if (m_stack.size() > 1) m_stack.pop_back(); }

    // Verification: opcode 0x13 forms a translation matrix and multiplies it
    // into the current matrix (retail FUN_0030ab94 current * translation).
    void Translate(float x, float y) {
        PlayerMatrix2D t;
        t.x = x;
        t.y = y;
        m_stack.back() = m_stack.back().Multiplied(t);
    }

    // Verification: opcode 0x14's FUN_0030ad80 writes column-major columns
    // (cos,sin,0,0), (-sin,cos,0,0), (0,0,1,0), (0,0,0,1); final VMX permutes
    // and identity-column loads are traced in player_drawlist_rotate_opcode14
    // and player_rotation_matrix_decomp_headless_20261010.c. The dispatcher
    // multiplies current * rotation. Bitwise trig/output comparison remains.
    void Rotate(float angle) {
        const float cosine = GroundRenderMath::RetailGroundSinCos(angle, true);
        const float sine = GroundRenderMath::RetailGroundSinCos(angle, false);
        PlayerMatrix2D r;
        r.a = cosine;
        r.b = sine;
        r.c = -sine;
        r.d = cosine;
        m_stack.back() = m_stack.back().Multiplied(r);
    }

    b2Vec2 Transform(float x, float y) const {
        return m_stack.back().Transform(x, y);
    }

    const PlayerMatrix2D& Current() const { return m_stack.back(); }

private:
    std::vector<PlayerMatrix2D> m_stack;
};

void SubmitRetainedPlayerShape(gmMachine& a_machine, gmTableObject* a_player,
                               const char* a_fieldName, int a_shapeType,
                               const std::vector<b2Vec2>& a_vertices,
                               const PlayerMatrix2D& a_transform, float a_z,
                               int a_nativeCapacity = -1,
                               int a_nativeAllocationStride = 12) {
    const std::vector<float> xy = FlattenPlayerVertices(a_vertices);
    const gmVariable shape = UpdateNativeOwnedShape(
        a_machine, a_player, a_fieldName, a_shapeType, xy.data(),
        static_cast<int>(a_vertices.size()), 0.0f, a_nativeCapacity,
        a_nativeAllocationStride);
    if (shape.m_type < GM_USER) return;

    const float a = a_transform.a, b = a_transform.b;
    const float c = a_transform.c, d = a_transform.d;
    const float x = a_transform.x, y = a_transform.y;
    gmMachine* machine = &a_machine;
    if (g_playerCommandRecorder) {
        auto lease = RetainNativeShapeForDeferredDraw(a_machine, shape);
        RecordPlayerCommand([machine, shape, lease, a, b, c, d, x, y, a_z] {
            (void)lease;
            DrawShapeObjectWithTransform(*machine, shape, a, b, c, d,
                                         x, y, a_z, true);
        });
    } else {
        DrawShapeObjectWithTransform(a_machine, shape, a, b, c, d,
                                     x, y, a_z, true);
    }
}

// Retail keeps four mutable mesh instances per player pass and uses one
// shared round-robin slot for the current draw. Keep separate rooted GM
// wrappers for those slots so sorted deferred submissions retain the mesh
// instance selected by their draw.
static std::string PlayerShapeRingField(const char* a_prefix, int a_slot) {
    return std::string(a_prefix) + std::to_string(a_slot);
}

// Verification: FUN_001dbcb8 and FUN_001dcb70 were decompiled headlessly
// from the retail EBOOT and checked against PPC instructions at those
// addresses. Coefficients, reduction threshold, and quadrant offsets are
// read from the ELF TOC data referenced by the instructions. Keep binary32
// rounding and fused operations here; this is the vector-angle path used by
// retail player rendering, not a generic libm replacement.
float RetailPlayerAtanReduced(float a_ratio, uint32_t a_quadrant) {
    constexpr float kReductionThreshold = 0.2679491937f;
    constexpr float kSqrtThree = 1.7320507765f;
    constexpr float kOne = 1.0f;
    constexpr float kTinyAngle = 0.000244140625f;
    constexpr float kNumeratorC = 0.8483080864f;
    constexpr float kDenominatorC0 = 0.5149751902f;
    constexpr float kDenominatorC1 = 0.08280213922f;
    constexpr float kAngles[8] = {
        0.0f, 0.5235987902f, 1.570796371f, 1.047197580f,
        1.570796371f, 2.094395161f, 3.141592741f, 2.617993832f
    };

    if (a_ratio > kReductionThreshold) {
        a_quadrant |= 1u;
        const float denominator = a_ratio + kSqrtThree;
        const float numerator = std::fma(a_ratio, kSqrtThree, -kOne);
        a_ratio = numerator / denominator;
    }
    if (a_ratio < -kTinyAngle || a_ratio > kTinyAngle) {
        const float square = a_ratio * a_ratio;
        const float cube = a_ratio * square;
        const float numerator = std::fma(cube, kNumeratorC, a_ratio);
        const float denominatorPolynomial = std::fma(
            square, kDenominatorC0, kDenominatorC1);
        const float denominator = std::fma(
            square, denominatorPolynomial, kOne);
        a_ratio = numerator / denominator;
    }
    if ((a_quadrant & 2u) != 0) a_ratio = -a_ratio;
    float angle = a_ratio + kAngles[a_quadrant & 7u];
    if ((a_quadrant & 8u) != 0) angle = -angle;
    return angle;
}

// Verification: the wrapper classifies both binary32 inputs with retail
// FUN_001dcd24, forms |y/x| in PPC single precision, and passes the ratio and
// quadrant flags to FUN_001dcb70. Signed zero is retained through the same
// sign-bit flags. Retail returns the exceptional operand directly when either
// input class is NaN/Inf, matching FUN_001dbcb8's early returns.
float RetailPlayerAtan2(float a_y, float a_x) {
    const auto isExceptional = [](float a_value) {
        return !std::isfinite(a_value);
    };
    if (isExceptional(a_y)) return a_y;
    if (isExceptional(a_x)) return a_x;

    uint32_t quadrant = 0;
    float absY = a_y;
    float absX = a_x;
    if (std::signbit(absY)) {
        absY = -absY;
        quadrant = 8;
    }
    if (std::signbit(absX)) {
        absX = -absX;
        quadrant ^= 6;
    }

    float ratio = 0.0f;
    if (absY <= absX) {
        if (a_x != 0.0f) ratio = absY / absX;
    } else {
        quadrant ^= 2;
        ratio = absX / absY;
    }
    return RetailPlayerAtanReduced(ratio, quadrant);
}

// Verification: retail FUN_003488c0 subtracts the input points, computes
// sqrt(dx*dx + dy*dy) with the PPC order `dy*dy` then `fmadds(dx,dx,...)`,
// and divides each component by that length when it is positive; a zero
// length yields (0,0). The aura stretch call at FUN_001a1660 0x1a2030 passes
// the current vertex and target point to this helper before scaling the unit
// direction by its falloff and stretch amount. See
static b2Vec2 RetailPlayerUnitDirection(const b2Vec2& a_from,
                                       const b2Vec2& a_to) {
    const float dy = a_to.y - a_from.y;
    const float dx = a_to.x - a_from.x;
    const float lengthSquared = std::fma(dx, dx, dy * dy);
    const float length = std::sqrt(lengthSquared);
    if (!(length > 0.0f)) return b2Vec2(0.0f, 0.0f);
    return b2Vec2(dx / length, dy / length);
}

// Verification: retail FUN_000c5e64 converts a vector to atan2(y,x), and
// FUN_000c6ca0 wraps (second-first) into [-pi,pi] using the PPC fmod helper;
// the call sequence used below is at FUN_001a1660 0x1a2584..0x1a2618.
float RetailWrappedAngleDelta(float a_from, float a_to) {
    constexpr float kTwoPi = 6.2831854820251465f;
    constexpr float kPi = 3.1415927410125732f;
    float delta = GroundRenderMath::RetailGroundRemainder(
        static_cast<float>(a_to - a_from), kTwoPi);
    if (delta < -kPi) delta = static_cast<float>(delta + kTwoPi);
    if (delta > kPi) delta = static_cast<float>(delta - kTwoPi);
    return delta;
}

// FUN_001aa590's native SHAPETYPE_TRIANGLEFAN circle builder. For n=61 it
// writes one center followed by 61 rim vertices; its angle increment is
// 2*pi/(n-1), so the last rim point closes the full circle. Keep the angle
// update as a binary32 addition (`fadds` in retail), and use the retail trig
// approximation and fused radius multiply for each coordinate.
static std::vector<b2Vec2> BuildRetailPlayerEyeFan(float a_radius) {
    constexpr int kRimPointCount = 61;
    constexpr float kTwoPi = 6.2831854820251465f;
    constexpr float kZero = 0.0f;
    const float angleStep = kTwoPi /
                            static_cast<float>(kRimPointCount - 1);

    std::vector<b2Vec2> vertices;
    vertices.reserve(kRimPointCount + 1);
    vertices.emplace_back(0.0f, 0.0f);
    float angle = 0.0f;
    for (int i = 0; i < kRimPointCount; ++i) {
        const float x = GroundRenderMath::RetailGroundSinCos(angle, true);
        const float y = GroundRenderMath::RetailGroundSinCos(angle, false);
        vertices.emplace_back(std::fma(x, a_radius, kZero),
                              std::fma(y, a_radius, kZero));
        angle = angle + angleStep;
    }
    return vertices;
}

// Verification: retail FUN_001a1660 reads `mainpbDirOnRun` at native +0x1b8,
// obtains current mainpb direction, and wraps their angle delta with
// FUN_000c6ca0. It adds that delta to cached native facing (+0x1f0). For the
// sticky branch, PPC wraps the adjusted angle back against cached facing,
// multiplies the result by 2.0 and runPerc, then uses fmadds to add cached
// facing. Constants and stack input are at 0x1a2604..0x1a2654; runPerc is
// loaded at 0x1a17a4 and saved at stack +0x1b0.
float GetRetailPlayerRenderAngle(gmMachine& a_machine,
                                 gmTableObject* a_player,
                                 float a_cachedX, float a_cachedY,
                                 float a_currentPhysicsX,
                                 float a_currentPhysicsY,
                                 bool a_sticky, float a_runPerc) {
    float cachedMainX = 0.0f, cachedMainY = 0.0f;
    if (!GetVector2D(a_machine, a_player->Get(&a_machine, "mainpbDirOnRun"),
                     cachedMainX, cachedMainY))
        return RetailPlayerAtan2(a_cachedY, a_cachedX);

    const float cachedFacingAngle = RetailPlayerAtan2(a_cachedY, a_cachedX);
    const float cachedPhysicsAngle = RetailPlayerAtan2(cachedMainY,
                                                       cachedMainX);
    const float currentPhysicsAngle = RetailPlayerAtan2(a_currentPhysicsY,
                                                        a_currentPhysicsX);
    const float physicsDelta = RetailWrappedAngleDelta(cachedPhysicsAngle,
                                                       currentPhysicsAngle);
    const float adjustedAngle = static_cast<float>(cachedFacingAngle + physicsDelta);
    if (!a_sticky) return adjustedAngle;

    const float stickyDelta = RetailWrappedAngleDelta(cachedFacingAngle,
                                                     adjustedAngle);
    const float weightedDelta = stickyDelta * 2.0f;
    return std::fma(weightedDelta, a_runPerc, cachedFacingAngle);
}

// Verification: particle iteration, active-life condition, size/travel
// curves, particle colour field, mixed-precision ratio and interpolation,
// and command ordering were checked against read-only headless Ghidra
// decompilation/instructions for retail FUN_001a0be8
// (0x001a0be8..0x001a15ab), saved in
// fallback passes n=61 to FUN_001aa590; that helper's geometry and color
// selection are also recorded in
// emitted directly through Renderer instead of replaying native draw-list
// opcodes; matched runtime pixel comparison remains open.
void DrawPlayerParticles(gmMachine& a_machine, gmTableObject* a_player,
                         Renderer& a_renderer) {
    gmVariable particlesVar = a_player->Get(&a_machine, "particles");
    if (particlesVar.m_type != GM_TABLE) return;
    gmTableObject* particles = static_cast<gmTableObject*>(
        a_machine.GetObject(particlesVar.m_value.m_ref));
    if (!particles) return;

    // The retail native loop uses the array's stored entry count. Player.gm
    // currently initializes 33 contiguous entries, but use the live table
    // count so the renderer follows that native contract if the pool changes.
    const int particleCount = particles->Count();
    constexpr int kParticleRimPoints = 61;
    // Native FUN_001a0be8's initial position vector has z=-17.7699986
    // (constant at 0x001a0bd8), equal to PLAYER_Z + ZINCER.
    constexpr float kParticleZ = -17.7699986f;
    constexpr float kTwoPi = 6.2831854820251465f;

    for (int i = 0; i < particleCount; ++i) {
        gmVariable particleVar = particles->Get(static_cast<gmint>(i));
        if (particleVar.m_type != GM_TABLE) continue;
        gmTableObject* particle = static_cast<gmTableObject*>(
            a_machine.GetObject(particleVar.m_value.m_ref));
        if (!particle) continue;

        const double msAlive = GetDoubleField(a_machine, particle, "msAlive", 0.0);
        const double lifeTime = GetDoubleField(a_machine, particle, "lifeTime", 0.0);
        if (!(msAlive < lifeTime)) continue;

        // FUN_001a0be8 divides the double-valued GM numbers, rounds that
        // ratio to binary32, subtracts it from 1.0 in binary64, then rounds
        // the remaining-life fraction to binary32 (dVar20 in its decompile).
        const float elapsedRatio = static_cast<float>(msAlive / lifeTime);
        const float perc = static_cast<float>(1.0 -
                                              static_cast<double>(elapsedRatio));
        // Retail calls MATH_POW for both particle curves. The shipped player
        // uses size exponents 0.1/0.3 and speed exponents 0.7/1.0 (the
        // inactive pool initializes both to 1.0); these stay in the audited
        // RetailGroundPow [0,1] input domain, so use the PPC-backed helper
        // instead of host libm rounding.
        const double baseRadius = GetDoubleField(a_machine, particle, "radius", 0.0);
        const float sizeCurve = static_cast<float>(GetDoubleField(
            a_machine, particle, "sizeCurve", 0.0));
        const float radius = static_cast<float>(baseRadius *
            static_cast<double>(GroundRenderMath::RetailGroundPow(
                perc, sizeCurve)));

        float px = 0.0f, py = 0.0f, tx = 0.0f, ty = 0.0f;
        if (!GetVector2D(a_machine, particle->Get(&a_machine, "pos"), px, py)
            || !GetVector2D(a_machine, particle->Get(&a_machine, "targetPos"), tx, ty))
            continue;

        const float elapsedPerc = static_cast<float>(1.0 -
            static_cast<double>(perc));
        const float speedCurve = static_cast<float>(GetDoubleField(
            a_machine, particle, "speedCurve", 0.0));
        const float travel = GroundRenderMath::RetailGroundPow(
            elapsedPerc, speedCurve);
        // The vector subtraction is stored as float by the retail helper;
        // interpolation then multiplies/adds in double and rounds once when
        // writing the translated point. This differs from float `p + d*t`.
        const float deltaX = tx - px;
        const float deltaY = ty - py;
        const b2Vec2 center(
            static_cast<float>(static_cast<double>(deltaX) * travel + px),
            static_cast<float>(static_cast<double>(deltaY) * travel + py));

        gmVariable colourVar = particle->Get(&a_machine, "colour");
        if (colourVar.m_type == GM_STRING) {
            gmStringObject* colour = static_cast<gmStringObject*>(
                a_machine.GetObject(colourVar.m_value.m_ref));
            if (colour) SetColourByName(a_renderer, colour->GetString());
        }

        // The native lazy-shape fallback calls FUN_001aa590 with n=61
        // (li r8,0x3d at 0x1a10ec), creates and caches a unit VMSHAPE, then
        // submits it with the same T*S transform as the normal cached-shape
        // route. Keep the cache behavior and circle-builder binary32 math.
        auto buildFallbackUnitCircle = [&] {
            std::vector<float> xy;
            xy.reserve((kParticleRimPoints + 1) * 2);
            xy.push_back(0.0f);
            xy.push_back(0.0f);
            const float angleStep = kTwoPi /
                                    static_cast<float>(kParticleRimPoints - 1);
            float angle = 0.0f;
            for (int point = 0; point < kParticleRimPoints; ++point) {
                const float unitX = GroundRenderMath::RetailGroundSinCos(angle, true);
                const float unitY = GroundRenderMath::RetailGroundSinCos(angle, false);
                xy.push_back(unitX);
                xy.push_back(unitY);
                angle += angleStep;
            }
            return xy;
        };

        // The native list retains the live VMSHAPE pointer and submits that
        // same object at replay. Keep it rooted in the desktop command buffer;
        // if absent/invalid, construct the same 62-vertex unit fan once and
        // cache it in particleFillShape, as the native fallback does.
        gmVariable particleShape = a_player->Get(&a_machine,
                                                 "particleFillShape");
        if (GetShapePrimitiveMode(a_machine, particleShape) == 0) {
            const std::vector<float> unitCircle = buildFallbackUnitCircle();
            particleShape = UpdateNativeOwnedShape(
                a_machine, a_player, "particleFillShape", 6,
                unitCircle.data(), kParticleRimPoints + 1, 0.0f,
                kParticleRimPoints + 1, 12);
        }
        if (g_playerCommandRecorder) {
            gmUserObject* shapeObject = AsShapeObject(a_machine, particleShape);
            if (shapeObject)
                g_playerCommandRecorder->retainedObjects.push_back(
                    static_cast<gmObject*>(shapeObject));
            gmMachine* machine = &a_machine;
            const float scale = radius;
            const float centerX = center.x;
            const float centerY = center.y;
            RecordPlayerCommand([machine, particleShape,
                                 scale, centerX, centerY] {
                DrawShapeObjectWithTransform(*machine, particleShape,
                    scale, 0.0f, 0.0f, scale, centerX, centerY,
                    kParticleZ, true);
            });
            continue;
        }
        if (DrawShapeObjectWithTransform(a_machine, particleShape,
                                         radius, 0.0f, 0.0f, radius,
                                         center.x, center.y, kParticleZ, true))
            continue;
    }
}

float GetFloatField(gmMachine& a_m, gmTableObject* a_t, const char* a_key, float a_default) {
    gmVariable v = a_t->Get(&a_m, a_key);
    if (v.m_type == GM_FLOAT) return v.m_value.m_float;
    if (v.m_type == GM_INT) return (float)v.m_value.m_int;
    return a_default;
}

// Verification: FUN_001a0be8 reads numeric fields through
// FUN_000f1b24. Its GM_FLOAT path widens the stored float to double, while
// GM_INT sign-extends the integer before conversion to double. Keep that
// distinction for the particle lifetime and interpolation calculations.
double GetDoubleField(gmMachine& a_m, gmTableObject* a_t, const char* a_key,
                      double a_default) {
    const gmVariable value = a_t->Get(&a_m, a_key);
    if (value.m_type == GM_FLOAT) return static_cast<double>(value.m_value.m_float);
    if (value.m_type == GM_INT) return static_cast<double>(value.m_value.m_int);
    return a_default;
}

// FUN_001a1660 converts runDrawStyle with AsInt semantics before branching:
// integer payloads pass through, floats truncate toward zero, and other
// types become zero. The live Player.gm value is integer 1.
int GetPlayerRunDrawStyle(gmMachine& a_machine, gmTableObject* a_player) {
    const gmVariable value = a_player->Get(&a_machine, "runDrawStyle");
    if (value.m_type == GM_INT) return value.m_value.m_int;
    if (value.m_type == GM_FLOAT) {
        const float x = value.m_value.m_float;
        if (!std::isfinite(x) || x < static_cast<float>(INT_MIN) ||
            x >= static_cast<float>(INT_MAX)) return INT_MIN;
        return static_cast<int>(x);
    }
    return 0;
}

float GetPlayerDrawScale(gmMachine& a_machine, gmTableObject* a_player) {
    float scale = GetFloatField(a_machine, a_player, "__nativePlayerDrawScale", 1.0f);
    if (scale != 1.0f) return scale;
    // Entity and its behavior table share native fields in retail. The local
    // Entity binding mirrors writes, but accept either side during startup.
    gmVariable entityVar = a_player->Get(&a_machine, "Entity");
    if (entityVar.m_type == GM_TABLE) {
        gmTableObject* entity = static_cast<gmTableObject*>(
            a_machine.GetObject(entityVar.m_value.m_ref));
        if (entity) scale = GetFloatField(a_machine, entity,
                                          "__nativePlayerDrawScale", scale);
    }
    return scale;
}

bool IsPlayerDrawScaleTransitionActive(gmMachine& a_machine,
                                       gmTableObject* a_player) {
    int active = static_cast<int>(GetFloatField(
        a_machine, a_player, "__nativePlayerDrawScaleTransition", 0.0f));
    if (active != 0) return true;
    gmVariable entityVar = a_player->Get(&a_machine, "Entity");
    if (entityVar.m_type != GM_TABLE) return false;
    gmTableObject* entity = static_cast<gmTableObject*>(
        a_machine.GetObject(entityVar.m_value.m_ref));
    return entity && static_cast<int>(GetFloatField(
        a_machine, entity, "__nativePlayerDrawScaleTransition", 0.0f)) != 0;
}

// Verification: retail GE3_Internal_MathExpCore at 0x1dbe30 classifies its
// binary32 input, then calls GE3_Internal_MathExpReduceScale at 0x1dce40 with
// multiplier 1 and mode 0 for ordinary finite values. That helper's range
// reduction, rational polynomial, and binary exponent adjustment match the
// exp portion of RetailGroundPowScaled in ground_math.h. The player scale
// callback normally supplies x in [-80,0]. The same PPC reduction also
// handles positive finite inputs; retain it through +80 (normal binary32
// results) for unusual negative deltas. Exceptional and more extreme values
// use std::exp until their wrapper/status behavior is ported.
float RetailPlayerExp(float a_x) {
    if (!std::isfinite(a_x) || a_x < -80.0f || a_x > 80.0f)
        return std::exp(a_x);

    const float kLog2E = GroundRenderMath::FloatFromBits(0x3fb8aa3bu);
    const float kLn2High = GroundRenderMath::FloatFromBits(0x3f317216u);
    const float kLn2Low = GroundRenderMath::FloatFromBits(0xb3fbe8e8u);
    const float scaled = a_x * kLog2E;
    const float bias = scaled < 0.0f ? -0.5f : 0.5f;
    const int scale = static_cast<int>(scaled + bias); // PPC fctiwz
    float reduced = std::fma(static_cast<float>(scale), -kLn2High, a_x);
    reduced = std::fma(static_cast<float>(scale), -kLn2Low, reduced);

    float approximation = 1.0f;
    constexpr float kExpTiny = 2.98023223876953125e-8f; // 2^-25
    if (reduced <= -kExpTiny || reduced >= kExpTiny) {
        const float square = reduced * reduced;
        const float numeratorTerm = reduced * std::fma(
            square, GroundRenderMath::FloatFromBits(0x3c085413u), 0.5f);
        const float denominatorTerm = square *
            GroundRenderMath::FloatFromBits(0x3dccbf9eu);
        const double numerator = static_cast<double>(denominatorTerm) +
                                 static_cast<double>(numeratorTerm) + 1.0;
        const double denominator = static_cast<double>(denominatorTerm) -
                                   static_cast<double>(numeratorTerm) + 1.0;
        approximation = static_cast<float>(numerator / denominator);
    }

    uint32_t bits = GroundRenderMath::FloatBits(approximation);
    const int exponent = static_cast<int>((bits >> 23) & 0xffu) + scale;
    if (exponent >= 255) return GroundRenderMath::FloatFromBits(0x7f800000u);
    if (exponent >= 1) {
        bits = (bits & 0x807fffffu) | (static_cast<uint32_t>(exponent) << 23);
        return GroundRenderMath::FloatFromBits(bits);
    }
    const int shift = 1 - exponent;
    if (shift >= 24) return 0.0f;
    const uint32_t significand = (bits & 0x7fffffu) | 0x800000u;
    return GroundRenderMath::FloatFromBits(significand >> shift);
}

} // namespace

float RetailAtan2(float a_y, float a_x) {
    return RetailPlayerAtan2(a_y, a_x);
}

// Verification: the transition flag, reset value, and update formula were
// traced from retail FUN_001a30c4/FUN_001a3124/FUN_001a0b60 and helper
// and player_scale_update_helpers_20261010.c. The native callback multiplies
// scale by MathExpCore(-(float)(0.0140845068 * deltaMs)), clamped to [0,1].
// Its base FUN_0005d17c runs the cached Player.update callback first. The
// enabled-entity adapter invokes this immediately after that script callback.
void TickPlayerNativeRenderer(float a_deltaMs) {
    EngineContext& ctx = GetEngineContext();
    if (!ctx.machine) return;
    gmVariable playerVar = FindPlayerEntity(*ctx.machine);
    if (playerVar.m_type != GM_TABLE) return;
    gmTableObject* player = static_cast<gmTableObject*>(
        ctx.machine->GetObject(playerVar.m_value.m_ref));
    if (!player || !IsPlayerDrawScaleTransitionActive(*ctx.machine, player)) return;

    constexpr float kScaleDecayRate = 0.0140845068f;
    const float exponent = -static_cast<float>(kScaleDecayRate * a_deltaMs);
    // FUN_000c6988 performs these two ordered strict comparisons in double
    // precision (0 - exp < 0, then 1 - result < 0). Preserve that sequence:
    // in particular, its NaN behavior differs from std::clamp, and the
    // callback multiplies the float scale by the double result before its
    // final binary32 store. See the saved native decompilation.
    double multiplier = static_cast<double>(RetailPlayerExp(exponent));
    if (0.0 - multiplier < 0.0) {
        // Keep the MathExpCore result.
    } else {
        multiplier = 0.0;
    }
    if (1.0 - multiplier < 0.0) multiplier = 1.0;
    const float scale = static_cast<float>(
        static_cast<double>(GetPlayerDrawScale(*ctx.machine, player)) * multiplier);
    player->Set(ctx.machine, "__nativePlayerDrawScale", gmVariable(scale));
    gmVariable entityVar = player->Get(ctx.machine, "Entity");
    if (entityVar.m_type == GM_TABLE) {
        gmTableObject* entity = static_cast<gmTableObject*>(
            ctx.machine->GetObject(entityVar.m_value.m_ref));
        if (entity) entity->Set(ctx.machine, "__nativePlayerDrawScale", gmVariable(scale));
    }
}

// Verification: retail native entry FUN_001a1660 is decoded in
// player_renderer_frame_math_capstone_20261010.txt. Particle ordering,
// scale cutoff, facing fallback, body ring formula, and color-name sites
// are traced there; the particle pass and per-vertex geometry are ported from
// those instructions. Style-0/style-1 transforms now apply their traced
// matrix command order locally. Native retained draw-list replay and output
// comparison remain incomplete, so this is not yet a 1:1 renderer.
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
    float runPerc = GetFloatField(*machine, player, "runPerc", 0.0f);
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
    float speed = RetailPlayerVectorLength(vel.x, vel.y);

    // Retail caches a fallback direction at native this+0x1f0. It refreshes
    // that vector only when stickyOn == 1 and dirPerc > 0, copying the
    // logicalDir field. Otherwise motion direction comes from the physics
    // velocity above when it exceeds the native threshold; at low speed the
    // cached vector is used. Keep the native object state per player instead
    // of sharing a process-global velocity direction.
    float cachedDirX = GetFloatField(*machine, player,
                                     "__nativePlayerDrawFacingX", 1.0f);
    float cachedDirY = GetFloatField(*machine, player,
                                     "__nativePlayerDrawFacingY", 0.0f);
    const int stickyOn = static_cast<int>(GetFloatField(
        *machine, player, "stickyOn", 0.0f));
    const float dirPerc = GetFloatField(*machine, player, "dirPerc", 0.0f);
    if (stickyOn == 1 && dirPerc > 0.0f) {
        float logicalX = cachedDirX, logicalY = cachedDirY;
        if (GetVector2D(*machine, player->Get(machine, "logicalDir"),
                        logicalX, logicalY)) {
            cachedDirX = logicalX;
            cachedDirY = logicalY;
            player->Set(machine, "__nativePlayerDrawFacingX", gmVariable(cachedDirX));
            player->Set(machine, "__nativePlayerDrawFacingY", gmVariable(cachedDirY));
        }
    }
    constexpr float kNativeFacingSpeedThreshold = 0.00046529368f;
    b2Vec2 dir(cachedDirX, cachedDirY);
    if (speed > kNativeFacingSpeedThreshold)
        dir = RetailPlayerNormalize(vel.x, vel.y);

    // The retail player renderer does not select a view; its DrawList shape
    // commands inherit the active stage view. ApplyProjection therefore
    // uses the current Renderer view established by the surrounding pass.
    // Retail pushes `drawz` once into the player DrawList transform; its
    // aura, body, run shapes, and eye all keep local vertex z=0. Do not apply
    // the game's general-purpose ZINCER constant between these passes.
    // The frame pass queues its DefaultTrans clear list before static/native
    // lists. All use the default 0x200000ff key, and retail's equal-key sort
    // is stable, so this fallback is active at the player's replay point.
    // The only opcode-0x25 producers found are the frame pass and static flush;
    // both record DefaultTrans. Mirror its known blend/raster state before
    // even the particle submission. This does not execute the native Cg code.
    RecordPlayerDefaultTransState();

    // Retail submits particles before applying the native object draw-scale
    // cutoff. cppDie/cppRebirth and the fixed-tick scale callback are mapped
    // to per-player native state above; particles remain visible even after
    // the body scale reaches the native 0.011 cutoff.
    DrawPlayerParticles(*machine, player, *renderer);
    const float kNativeDrawScale = GetPlayerDrawScale(*machine, player);
    if (kNativeDrawScale <= 0.011f) return;

    // Retail at 0x1a1cc8 increments this native index and reduces it modulo
    // four before selecting the parallel shape-cache entries. The cutoff
    // above precedes that increment in the PPC path.
    int ringSlot = static_cast<int>(GetFloatField(
        *machine, player, "__nativePlayerShapeRingIndex", 0.0f));
    ringSlot = (ringSlot + 1) & 3;
    player->Set(machine, "__nativePlayerShapeRingIndex", gmVariable(ringSlot));

    // Match the retail 0x1a18d4 state-save point: the particle pass is already
    // recorded, and the <= 0.011 scale path exits without emitting 0x1b.
    std::unique_ptr<NativePlayerRenderStateScope> immediateRenderStateScope;
    std::shared_ptr<DeferredPlayerRenderState> deferredRenderState;
    if (g_playerCommandRecorder) {
        deferredRenderState = std::make_shared<DeferredPlayerRenderState>();
        const auto state = deferredRenderState;
        RecordPlayerCommand([state] {
            state->scope = std::make_unique<NativePlayerRenderStateScope>();
        });
    } else {
        immediateRenderStateScope =
            std::make_unique<NativePlayerRenderStateScope>();
    }

    radius *= kNativeDrawScale;
    const int runDrawStyle = GetPlayerRunDrawStyle(*machine, player);

    // The eye/run angle uses the main physics body direction, as does the
    // aura's initial angle. Keep this data available to all three passes.
    float mainPosX = 0.0f, mainPosY = 0.0f;
    float facingX = 1.0f, facingY = 0.0f;
    GetPhysicsBodyPosAndDir(*machine, mainpbVar, mainPosX, mainPosY,
                            facingX, facingY);

    // Retail computes this sticky-only offset exactly before recording shape
    // pass one. Its push/translation/pop is applied to aura vertices through
    // the local matrix stack; native retained command recording is separate.
    const float nativeStretch = stickyOn == 1
        ? 3.0f * GroundRenderMath::RetailPlayerPow(
              RetailPlayerClamp(speed / maxSpeedX, 0.0f, 1.0f), 1.7f)
        : 0.0f;
    b2Vec2 leanOfs = (radius * -0.057f * nativeStretch) * dir;

    // ---- aura (outline) ----------------------------------------------
    // PARTIAL native port: radius, amplitude, arc increments, and the
    // recovered per-vertex deformation factors below come from PPC. Exact
        // The retained triangle-fan buffer is refreshed on the player and
        // submitted with the recorded player-position/lean transform.
    {
        constexpr int kQQ = 6, kN = 8;
        // Native PPC at 0x1a1d5c..0x1a1da8 derives a runPerc-dependent
        // radius and two run-faded amplitudes from the constant pool:
        // R*1.33 + (R*1.6 - R*1.33)*(1-runPerc), then 0.05/0.03 of
        // that result times (1-runPerc). The native field key is `runPerc`
        // at this+0x188; `squintPerc` is read separately for the body pass.
        // Keep the multiply/subtract order used by the PPC fused instructions.
        const float runBlend = 1.0f - runPerc;
        const float auraBase = radius * 1.33f;
        const float auraDelta = std::fma(radius, 1.6f, -auraBase);
        const float auraR = std::fma(auraDelta, runBlend, auraBase);
        // Preserve the pool's exact f32 value (0x3d4ccccc), which differs
        // from the compiler-rounded literal 0.05f by one ULP.
        const float outAmp = (auraR * 0.0499999976f) * runBlend;
        const float inAmp = (auraR * 0.03f) * runBlend;
        // Native constants at pool offsets +0x28/+0x2c/+0x30, divided
        // over five intervals in each six-vertex arc. They are not the
        // legacy PlayerOLD.gm arc percentages.
        constexpr float outAngInc = 0.0911061913f;
        constexpr float inAngInc = 0.0659734532f;
        constexpr float ampAngInc = 0.628318548f;
        // Retail obtains the aura's initial angle from mainpb.getDirRef
        // (FUN_00031e94): the body's facing vector, distinct from `dir`
        // above, which is the velocity/logical-facing vector used for stretch.
        float ang = RetailPlayerAtan2(facingY, facingX);
        b2Vec2 stretchDir = -1.0f * dir;
        // Retail calls the fsub/fsel median helper 0x348670 at 0x1a1de8
        // after dividing speed by maxSpeedX. Preserve its NaN, signed-zero,
        // and boundary behavior instead of host min/max semantics.
        const float speedPerc = RetailPlayerClamp(speed / maxSpeedX,
                                                  0.0f, 1.0f);
        float stretchAmt = auraR * 0.3f * speedPerc + nativeStretch * 0.061f;
        b2Vec2 stretchTargetPos = (auraR * 2.7f) * stretchDir; // auraR + auraR*1.7
        // Retail scales both angular stick deformation terms by the same
        // non-running blend used by its runPerc state.
        constexpr float kStickAngRange = 1.41371667f;
        float downCurve = 7.0f * stickGraphicPerc * runBlend;
        float stickRPerc = 0.3f * stickGraphicPerc * runBlend;

        std::vector<b2Vec2> verts;
        // Native submit contains its center, 8 pairs of six-point arcs, and
        // the saved first-arc closure vertex (98 vertices total).
        verts.reserve(98);
        verts.push_back(b2Vec2(0, 0));
        b2Vec2 first(0, 0);
        bool haveFirst = false;

        auto pushLobe = [&](bool a_outer) {
            float ampAng = 0.0f;
            for (int ii = 0; ii < kQQ; ++ii) {
                b2Vec2 d(GroundRenderMath::RetailGroundSinCos(ang, true),
                         GroundRenderMath::RetailGroundSinCos(ang, false));
                float rp = 1.0f;
                if (stickyOn == 1 && haveStickDir) {
                    // FUN_000c6030 computes absolute wrapped difference of
                    // the two vector angles. Keep that angular domain here;
                    // acos(dot) loses the signed-angle rounding path.
                    float stickAng = RetailPlayerAtan2(stickDir.y, stickDir.x);
                    float vertexAng = RetailPlayerAtan2(d.y, d.x);
                    float sdelta = std::fabs(vertexAng - stickAng);
                    if (sdelta > (float)M_PI) sdelta = 2.0f * (float)M_PI - sdelta;
                    if (sdelta < kStickAngRange) {
                        float sperc = RetailPlayerClamp(
                            sdelta * 0.70735532f, 0.0f, 1.0f);
                        sperc = GroundRenderMath::RetailPlayerPow(sperc, downCurve);
                        rp *= (1.0f + (1.0f - sperc) * stickRPerc);
                    }
                }
                // outer: r + MATH_SIN_POW(ampAng,0.7)*outAmp
                // inner: r - MATH_SIN(ampAng)*inAmp
                const float wobble = GroundRenderMath::RetailGroundSinCos(
                    ampAng, false);
                const float rOfs = a_outer
                    ? std::copysign(GroundRenderMath::RetailGroundPow(
                          std::fabs(wobble), 0.7f), wobble) * outAmp
                    : -wobble * inAmp;
                b2Vec2 p = ((auraR + rOfs) * rp) * d;

                if (stickyOn == 1) {
                    float stretchAng = RetailPlayerAtan2(stretchDir.y, stretchDir.x);
                    float vertexAng = RetailPlayerAtan2(d.y, d.x);
                    float delta = std::fabs(vertexAng - stretchAng);
                    if (delta > (float)M_PI) delta = 2.0f * (float)M_PI - delta;
                    if (delta < (float)M_PI / 2.0f) {
                        float perc = 1.0f - GroundRenderMath::RetailGroundPow(
                            delta * 0.636619747f, 0.8f);
                        const b2Vec2 stretchDirection =
                            RetailPlayerUnitDirection(p, stretchTargetPos);
                        const float stretchScale = perc * stretchAmt;
                        p.x += stretchDirection.x * stretchScale;
                        p.y += stretchDirection.y * stretchScale;
                    }
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
        PlayerTransformStack auraTransform(pos);
        auraTransform.Push();
        auraTransform.Translate(leanOfs.x, leanOfs.y);
        const std::string shapeField = PlayerShapeRingField(
            "__nativePlayerAuraShape", ringSlot);
        SubmitRetainedPlayerShape(*machine, player, shapeField.c_str(),
            6, verts, auraTransform.Current(), drawz, 64, 20);
        auraTransform.Pop();
    }

    // ---- body (fill) ---------------------------------------------------
    // Retail PPC at 0x1a23a8..0x1a24ec writes the native 64-point ring via
    // 0x348798. With R=radius*this+0x250, S=squintPerc, V=speed:
    // step=2*pi/(64-1), angle=i*step; x=cos(angle)*R
    // y=pow(abs(sin(angle)),1-0.6*S)*R*(1-S*V), z=0.
    // This replaces the old script-derived ellipse approximation. Selector
    // 0 is sine and selector 1 cosine (retail Math wrappers).
    {
        // The constructor initially builds a center plus 64 samples, but the
        // live draw resets the selected mesh's vertex count to zero at
        // 0x1a2440 and overwrites it with exactly 64 ring samples.
        constexpr int kShapeQ = 64;
        float angInc = 2.0f * (float)M_PI / (float)(kShapeQ - 1);
        std::vector<b2Vec2> verts;
        verts.reserve(kShapeQ);
        // The native squash translation is bracketed by matrix push/pop
        // around the aura submit at 0x1a2288/0x1a22bc. The body is submitted
        // after that pop, so its ring remains centered on the physics position.
        const float squintVerticalFactor =
            std::fma(-squintPerc, speed, 1.0f);
        const float bodyVerticalScale = radius * squintVerticalFactor;
        const float bodyPower = std::fma(-squintPerc, 0.6f, 1.0f);
        for (int i = 0; i < kShapeQ; ++i) {
            float a = (float)i * angInc;
            const float x = GroundRenderMath::RetailGroundSinCos(a, true) * radius;
            const float sine = GroundRenderMath::RetailGroundSinCos(a, false);
            // Retail takes abs(sin) for FUN_001dc1c0, then uses fsel at
            // 0x1a24c4..0x1a24cc to restore the original sine sign. Without
            // this sign restoration the lower half of the ring folds upward,
            // making the body fan overlap itself and look cut off.
            const float poweredSine = GroundRenderMath::RetailPlayerPow(
                std::fabs(sine), bodyPower);
            const float signedSinePower = sine < 0.0f
                ? -poweredSine : poweredSine;
            const float y = signedSinePower * bodyVerticalScale;
            verts.push_back(b2Vec2(x, y));
        }
        SetColourByName(*renderer, BodyColourName(*machine, player));
        PlayerMatrix2D bodyTransform;
        bodyTransform.x = pos.x;
        bodyTransform.y = pos.y;
        const std::string shapeField = PlayerShapeRingField(
            "__nativePlayerBodyShape", ringSlot);
        SubmitRetainedPlayerShape(*machine, player, shapeField.c_str(),
            6, verts, bodyTransform, drawz, 65);
    }

    // Retail's third color selection at 0x1a2660 is unconditional and occurs
    // after the body submit but before either run-style branch. Style 1's eye
    // inherits this color, and unsupported style values still record it.
    SetColourByName(*renderer, AuraColourName(*machine, player));

    // Verification: retail FUN_001a1660's style-0 loop at
    // 0x1a2744..0x1a2a3c builds two 17-vertex ShapeType::TriangleStrip meshes
    // (`this+0x20c` and `this+0x21c`). PPC supplies the span/radii from the
    // scalar pool and advances theta with fmadds; each strip is then drawn
    // under the same player-position/angle transform. The loop writes one
    // vertex into each mesh per sample (34 total, 17 per shape).
    if (runDrawStyle == 0) {
        const float runBlend = 1.0f - runPerc;
        const float span = std::fma(runBlend,
            GroundRenderMath::FloatFromBits(0x4079511au),
            GroundRenderMath::FloatFromBits(0x4018ce9cu));
        const float radiusHalf = radius * 0.5f;
        const float outerRunRadius = std::fma(radiusHalf,
            GroundRenderMath::FloatFromBits(0x3f95c28fu),
            GroundRenderMath::FloatFromBits(0x3d851eb8u)) * runPerc;
        const float innerRunRadius = std::fma(radiusHalf,
            GroundRenderMath::FloatFromBits(0x3f95c28fu),
            -GroundRenderMath::FloatFromBits(0x3d851eb8u)) * runPerc;
        const float movingRadius = std::fma(radius,
            GroundRenderMath::FloatFromBits(0x3e3bbbbcu), outerRunRadius);
        float sampleAngle = std::fma(span,
            GroundRenderMath::FloatFromBits(0xbf000000u),
            GroundRenderMath::FloatFromBits(0x40490fdbu));

        const float renderAngle = GetRetailPlayerRenderAngle(*machine, player,
            cachedDirX, cachedDirY, facingX, facingY, stickyOn == 1, runPerc);

        auto drawRunStrip = [&](float a_radius, float a_commandOffsetX) {
            std::vector<b2Vec2> vertices;
            vertices.reserve(17);
            PlayerTransformStack transform(pos);
            transform.Push();
            // Retail emits opcode 0x14 only when fcmpu sees a nonzero angle;
            // unordered NaNs take the rotation path, signed zeros skip it.
            if (renderAngle != 0.0f) transform.Rotate(renderAngle);
            transform.Translate(a_commandOffsetX, 0.0f);
            float angle = sampleAngle;
            for (int i = 0; i < 17; ++i) {
                const float localX =
                    GroundRenderMath::RetailGroundSinCos(angle, true) * a_radius;
                const float localY =
                    GroundRenderMath::RetailGroundSinCos(angle, false) * a_radius;
                vertices.emplace_back(localX, localY);
                angle = std::fma(span,
                    GroundRenderMath::FloatFromBits(0x3d800000u), angle);
            }
            const char* fieldPrefix = a_commandOffsetX == 0.0f
                ? "__nativePlayerRunShape0_" : "__nativePlayerRunShape1_";
            const std::string shapeField = PlayerShapeRingField(
                fieldPrefix, ringSlot);
            SubmitRetainedPlayerShape(*machine, player, shapeField.c_str(), 5,
                vertices, transform.Current(), drawz, 64, 20);
            transform.Pop();
        };

        // opcode 0x13's second vector is {raw loop index,0,0,0}; index 1 is
        // the smallest positive subnormal when the replay treats it as float.
        // Retain that payload for the second strip even though normal world
        // positions round it away when added.
        drawRunStrip(innerRunRadius, GroundRenderMath::FloatFromBits(0));
        drawRunStrip(movingRadius, GroundRenderMath::FloatFromBits(1));
    }

    // ---- style-1 eye shape -----------------------------------------------
    // The constructor's FUN_001aa590 builds a center vertex followed by 61
    // rim points at radius 11/60 R. Its PPC uses RetailGroundSinCos and a
    // float-accumulated 2*pi/60 increment. The center follows the recovered
    // style-1 angle logic; native command-buffer replay remains separate.
    if (runDrawStyle == 1) {
        float eyeR = radius * (11.0f / 60.0f);
        float eyePosR = radius / 2.0f;
        // The render angle controls the cached eye shape's rotation. The
        // preceding retail translation stays on local +X at R/2. Retail
        // corrects cached facing by the mainpb direction change since
        // `mainpbDirOnRun` was recorded, then applies its 2*runPerc weight
        // while sticky.
        const float eyeAngle = GetRetailPlayerRenderAngle(*machine, player,
            cachedDirX, cachedDirY, facingX, facingY, stickyOn == 1, runPerc);
        PlayerTransformStack eyeTransform(pos);
        eyeTransform.Push();
        // Retail emits base translation (0x1a2ab8), optional rotation
        // (0x1a2ac0..0x1a2af8), then FUN_003489e8's local translation
        // (0x1a2b0c). Since opcode replay multiplies current * opcode, the
        // half-radius offset is rotated with the eye. Keep that command order.
        // Retail's fcmpu/beq pair omits opcode 0x14 for either signed zero;
        // unordered (NaN) angles take the rotation path.
        if (eyeAngle != 0.0f) eyeTransform.Rotate(eyeAngle);
        eyeTransform.Translate(eyePosR, 0.0f);
        std::vector<b2Vec2> verts = BuildRetailPlayerEyeFan(eyeR);
        // The eye is the LAST of the four native submits (@0x1a2b30), and
        // that submit sets no colour of its own -- it inherits the one set
        // at 0x1a2660, which is the same ground_fill/ground_noClimb_fill/
        // danger_fill selection the aura uses. In scheme 150 ground_fill is
        // white, matching the small white dot visible inside the body. Do not
        // append a redundant color command here.
        // The style-1 PPC path stores drawz+ZINCER at stack 0x1d8/0x1dc,
        // then passes that sum as the z component of its player-position
        // translation at 0x1a2ab8. The body and aura use drawz; the eye is
        // deliberately one native ZINCER step nearer. This is also why the
        // particle helper's fixed z is PLAYER_Z+ZINCER.
        constexpr float kRetailZincer = 0.03f; // FUN_001a1660 constant pool +0x40
        const std::string shapeField = PlayerShapeRingField(
            "__nativePlayerEyeShape", ringSlot);
        SubmitRetainedPlayerShape(*machine, player, shapeField.c_str(),
            6, verts, eyeTransform.Current(), drawz + kRetailZincer, 62);
        eyeTransform.Pop();
    }

    if (deferredRenderState) {
        const auto state = deferredRenderState;
        RecordPlayerCommand([state] { state->scope.reset(); });
    }
}

// Verification: retail `GE3_Internal_VideoDrawNativeMapAndBuffers` calls the
// native entity vtable's +0x3c draw slot, then queues the entity's DrawList at
// object +0x14 for the frame sort. `FUN_001a1660` resets its list with
// `FUN_0003d6d4`; that helper resets the command count and sets order key
// 0x200000ff. Preserve that placement in the desktop entity-list sort. The
// recorded callback retains and replays the player's VMSHAPE objects through
// the standard shape path; individual matrix opcodes are composed into the
// affine transform and remain an instruction-stream parity gap.
DrawListSubmission CaptureNativePlayerDrawSubmission() {
    DrawListData playerList;
    playerList.orderKey = 0x200000ffu;
    {
        PlayerCommandRecordingScope recording(*playerList.commandBuffer);
        DrawPlayerNative();
    }
    DrawListSubmission submission;
    submission.commandBuffer = playerList.commandBuffer;
    submission.orderKey = playerList.orderKey;
    // The player shares the same replay-time render state as every queued
    // DrawList; opcode 0x1a saves that live tuple when its command executes.
    return submission;
}
