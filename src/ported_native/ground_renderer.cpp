#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include "ground_renderer.h"
#include "../core/engine_context.h"
#include "../core/physics_world.h"
#include "../bindings/physics_body_bindings.h"
#include "../bindings/colour_bindings.h"
#include "gmTableObject.h"
#include <box2d/box2d.h>
#include <GL/glew.h>
#include <vector>
#include <cmath>

// Ported 1:1 (structure and call order) from Platformer/GroundProcessNEW.gm's
// `PORTED_TO_CPP______Platformer_GroundProcessNEW` -- see ground_renderer.h
// for why that's the best available source. Two real simplifications from
// that reference, both documented at the point they matter below:
//   1. Geometry (verts/fillVerts) is recomputed fresh every frame straight
//      from live Box2D fixtures instead of cached at entity-registration
//      time -- grounds are static (StandardBodyDef: sleeping=true,
//      fixedRotation=true) so this is cheap and avoids needing to hook
//      entity register/unregister events at all.
//   2. The multi-ground-piece stencil coordination (so two adjacent
//      grounds sharing an edge don't double up their outline where they
//      touch) is applied per-ground exactly as the reference does the
//      GL state calls, but without a persistent stencil-value-versus-
//      buffer scheme spanning the whole grounds list in a single pass --
//      each ground's own outline/fill stencil test is self-contained.
//      Visually this can leave a slightly doubled outline exactly on a
//      shared seam between two touching ground pieces; everywhere else
//      (the overwhelming majority of any real level) it's identical.
namespace {

constexpr uint16 kGroundCategory = 1 << 0; // retail GROUND_COLLISION_CATEGORY

// Z layer constants -- confirmed real values, constants_bindings.cpp.
constexpr float kBackConversion = 0.01f;

// GROUND_Z, real retail value (see SOUND_SHAPES_GROUND_TRUTH.md section 8;
// read out of the bootstrap at 0x19b1c0..0x19b4f0). This was hardcoded
// +10.0 -- the WRONG SIGN -- and, being a local copy, it did not change
// when the GM global was corrected, so the ground kept drawing on the
// wrong side of the player. Every kOutlineZ/kClearZ/... below derives from
// it, so the whole ground sub-layer stack was mirrored around zero too.
constexpr float kGroundZ = -10.0f;
// CLEAR_Z/OUTLINE_Z/NOCLIMB_CLEAR_Z/FIRE_CLEAR_Z as computed inside
// GroundProcessNEW's own constructor (OUTLINE_Z=GROUND_Z,
// CLEAR_Z=OUTLINE_Z+60*BACK_CONVERSION, NOCLIMB_CLEAR_Z=CLEAR_Z-20*BACK_CONVERSION,
// FIRE_CLEAR_Z=CLEAR_Z-30*BACK_CONVERSION) -- computed the same way here
// rather than re-declaring the *_Z globals above, so a future change to
// GROUND_Z/BACK_CONVERSION stays consistent automatically.
constexpr float kOutlineZ = kGroundZ;
constexpr float kClearZ = kOutlineZ + 60.0f * kBackConversion;
constexpr float kNoclimbClearZ = kClearZ - 20.0f * kBackConversion;
constexpr float kFireClearZ = kClearZ - 30.0f * kBackConversion;


constexpr int kOutlineStencilValue = 1;
constexpr int kNoclimbStencilValue = 2;

// VIDEOPARAMS_* -- confirmed real integer values, constants_bindings.cpp
// (Renderer::SetStencilFunc/SetStencilOp already consume these directly).
constexpr int kGreater = 4;
constexpr int kDisable = 8;
constexpr int kKeep = 100;
constexpr int kReplace = 101;

enum ShapeKind { kCircle, kPolygon, kEdgeChain };

struct GroundGeometry {
    ShapeKind kind = kPolygon;
    b2Vec2 pos = b2Vec2(0, 0); // circle center / polygon centroid (world)
    float radius = 0.0f;       // circle only
    float th = 2.0f * kBackConversion; // outline thickness, matches t.th
    std::vector<b2Vec2> verts;     // world-space boundary verts (polygon/edge-chain)
    std::vector<b2Vec2> fillVerts; // inset boundary verts (polygon), or unused for circle
};

b2Vec2 Normalized(const b2Vec2& a_v) {
    float len = a_v.Length();
    if (len < 1e-6f) return b2Vec2(0, 0);
    return b2Vec2(a_v.x / len, a_v.y / len);
}
b2Vec2 VecToPointNormalized(const b2Vec2& a_from, const b2Vec2& a_to) {
    return Normalized(a_to - a_from);
}
// GM vector2D.getRightNormal() -- confirmed real, (x,y) -> (y,-x) (a
// clockwise/right-hand perpendicular in this engine's Y-up world space,
// matching every other confirmed vector2D op ported this session).
b2Vec2 RightNormal(const b2Vec2& a_v) { return b2Vec2(a_v.y, -a_v.x); }

// t.calculateFillVerts(distIn) -- confirmed real per-vertex inset via the
// sum of unit vectors toward the next/prev vertex (an approximate angle
// bisector, exact for regular polygons, a reasonable approximation
// otherwise -- this is retail's own algorithm, not an invented one).
std::vector<b2Vec2> CalculateFillVerts(const std::vector<b2Vec2>& a_verts, float a_distIn) {
    std::vector<b2Vec2> out;
    size_t n = a_verts.size();
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const b2Vec2& cur = a_verts[i];
        const b2Vec2& prev = a_verts[(i + n - 1) % n];
        const b2Vec2& next = a_verts[(i + 1) % n];
        b2Vec2 bisector = VecToPointNormalized(cur, next) + VecToPointNormalized(cur, prev);
        out.push_back(cur + a_distIn * bisector);
    }
    return out;
}

bool BuildGeometry(b2Body* a_body, GroundGeometry& a_out) {
    b2Fixture* first = a_body->GetFixtureList();
    if (!first) return false;
    int numFixtures = 0;
    for (b2Fixture* f = first; f; f = f->GetNext()) ++numFixtures;
    const b2Transform& xf = a_body->GetTransform();

    if (first->GetShape()->GetType() == b2Shape::e_circle) {
        a_out.kind = kCircle;
        const b2CircleShape* c = (const b2CircleShape*)first->GetShape();
        a_out.pos = b2Mul(xf, c->m_p);
        a_out.radius = c->m_radius;
        constexpr int kSegs = 61; // matches the reference's own `local q=61;`
        a_out.verts.reserve(kSegs);
        for (int i = 0; i < kSegs; ++i) {
            float ang = (float)i / (float)kSegs * 2.0f * (float)M_PI;
            a_out.verts.push_back(a_out.pos + a_out.radius * b2Vec2{std::cos(ang), std::sin(ang)});
        }
        return true;
    }
    if (numFixtures == 1 && first->GetShape()->GetType() == b2Shape::e_polygon) {
        a_out.kind = kPolygon;
        const b2PolygonShape* p = (const b2PolygonShape*)first->GetShape();
        a_out.verts.reserve(p->m_count);
        b2Vec2 centroid(0, 0);
        for (int i = 0; i < p->m_count; ++i) {
            b2Vec2 v = b2Mul(xf, p->m_vertices[i]);
            a_out.verts.push_back(v);
            centroid += v;
        }
        a_out.pos = (1.0f / (float)p->m_count) * centroid;
        a_out.fillVerts = CalculateFillVerts(a_out.verts, a_out.th);
        return true;
    }
    // Edge-chain ground (multiple 2-vertex edge fixtures) -- confirmed real
    // ("assuming it's a closed shape" in the reference) but no real corpus
    // evidence of one actually reachable via appendShapes/addEdges in this
    // pass, and the reference's own drawOutline/drawFill are both no-ops
    // for this shape kind (`} else if (t.shapeType==this.EDGE_SHAPETYPE) {
    // }` -- empty, same as ICE/MUD/BOUNCY below), so there's nothing to
    // draw either way.
    a_out.kind = kEdgeChain;
    return true;
}

// ENGINE PATCH (2026-08-17): quad-per-segment thick line instead of a raw
// GL_LINE_LOOP. A plain GL line rasterizes at exactly 1px with no coverage-
// based antialiasing in this engine's core GL profile (glLineWidth/
// GL_LINE_SMOOTH are both legacy features core profile doesn't reliably
// honour -- confirmed live: enabling GL_LINE_SMOOTH here made no visible
// difference), so a long, shallow-diagonal ground outline (a real, common
// case -- a mostly-vertical cliff edge with only a slight slope) can leave
// visible sub-pixel gaps between consecutive 1px segments, rendering as a
// dotted/dashed line instead of retail's real solid one (confirmed via
// continuous edge, no dashing). Each edge becomes its own small filled
// quad (perpendicular offset by half the line width), reusing the exact
// same DrawPolygon(GL_TRIANGLE_FAN,...) fill path every other opaque draw
// in this engine already goes through -- guaranteed solid coverage
// regardless of angle, at the cost of unmitred joints (a visually
// negligible gap/overlap at each vertex for a line this thin, not a hole
// in the outline). Width is a small constant in world units, not read from
// Main.gm's confirmed-real `Video.setLineWidth(0.3)` (that value's real
// meaning/unit was never established beyond "it's called once, early" --
// see this project's own notes -- so picking a plausible, visually-tuned
// thin width here is honest about the gap rather than pretending 0.3 is a
// confirmed match).
void DrawOutline(Renderer& a_r, const GroundGeometry& a_g, float a_z) {
	//return; // are these even used?
	
    if (a_g.kind == kEdgeChain || a_g.verts.empty()) return;
    size_t n = a_g.verts.size();
    if (n < 2) return;
    
    float kOutlineHalfWidth = GetEngineContext().video.lineWidth * 0.25;
    for (size_t i = 0; i < n; ++i) {
        const b2Vec2& p0 = a_g.verts[i];
        const b2Vec2& p1 = a_g.verts[(i + 1) % n];
        b2Vec2 dir = p1 - p0;
        float len = dir.Length();
        if (len < 0.0001f) continue;
        b2Vec2 perp(-dir.y / len, dir.x / len);
        b2Vec2 off = kOutlineHalfWidth * perp;
        float quad[8] = {
            p0.x + off.x, p0.y + off.y,
            p1.x + off.x, p1.y + off.y,
            p1.x - off.x, p1.y - off.y,
            p0.x - off.x, p0.y - off.y,
        };
        a_r.DrawPolygon(GL_TRIANGLE_FAN, quad, 4, a_z);
    }
}
void DrawFill(Renderer& a_r, const GroundGeometry& a_g, float a_z) {
    if (a_g.kind == kEdgeChain) return;
    const std::vector<b2Vec2>& verts = (a_g.kind == kCircle) ? a_g.verts : a_g.fillVerts;
    if (verts.empty()) return;
    std::vector<float> xy;
    xy.reserve(verts.size() * 2);
    if (a_g.kind == kCircle) {
        // drawFill uses radius-th, a separate (slightly smaller) ring from
        // the outline's own verts -- build it directly rather than reusing
        // a_g.verts.
        for (size_t i = 0; i < verts.size(); ++i) {
            b2Vec2 dir = Normalized(verts[i] - a_g.pos);
            b2Vec2 p = a_g.pos + (a_g.radius - a_g.th) * dir;
            xy.push_back(p.x); xy.push_back(p.y);
        }
    } else {
        for (const b2Vec2& v : verts) { xy.push_back(v.x); xy.push_back(v.y); }
    }
    a_r.DrawPolygon(GL_TRIANGLE_FAN, xy.data(), (int)xy.size() / 2, a_z);
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

// installNormalGround -- confirmed real (the overwhelming majority style,
// see NORMAL_GROUNDSTYLEID's comment). The grass-tuft decoration block
// above this in the reference is entirely `if(0){...}`-disabled dead code
// in the reference itself, so it's not ported here either.
// ENGINE PATCH (2026-08-16): fill colour changed from the literal script
// name "clear" to "ground_fill". installNormalGround's real script
// (game_data/Platformer/GroundProcessNEW.gm) does genuinely call
// `Video.setColour("clear")` for the fill -- that part of the port was
// tutorial_gameplay_moving.png (not just the one tutorial_welcome_screen.png
// sample the colour table's own header comment describes) confirms the real
// rendered platform is solid dark (42,33,15), matching "ground_fill"
// exactly, NOT the background tan (205,178,66) that "clear" resolves to in
// our own default scheme table -- 98082 dark ground-coloured pixels vs
// 460089 background-coloured pixels, both clearly present and clearly
// different in the same frame. Since Main.gm's preDraw() also confirmedly
// uses "clear" for the actual full-screen background wash every frame, "clear"
// can't be both tan (the wash) and dark (ground fill) as a single static
// value the way our ColourManager currently models it -- something in
// retail's real scheme/theme system must reassign "clear" between those two
// draws within the same frame, which this reimplementation has no evidence
// for and doesn't replicate. Rather than leave ground pieces rendering
// hollow (fill == background, only the outline visible -- "just lines"),
// this uses "ground_fill" directly: it's the one colour we have real sampled
// ground truth for, and it's what's actually only visible on screen either way.
void DrawNormal(Renderer& a_r, const GroundGeometry& a_g) {
    a_r.SetStencilFunc(kGreater, kOutlineStencilValue);
    a_r.SetStencilOp(kKeep, kKeep, kReplace);
    SetColourByName(a_r, "ground_out");
    DrawOutline(a_r, a_g, kOutlineZ);
    a_r.SetStencilFunc(kDisable, 0);

    SetColourByName(a_r, "ground_fill");
    DrawFill(a_r, a_g, kClearZ);
}

// installNoClimbGround -- confirmed real; note the stencil wrap is on the
// FILL here, not the outline (opposite of installNormalGround) -- matches
// the reference exactly, not a copy-paste of Normal's order.
void DrawNoClimb(Renderer& a_r, const GroundGeometry& a_g) {
    SetColourByName(a_r, "ground_outline");
    DrawOutline(a_r, a_g, kOutlineZ);

    // FILL COLOUR: "ground_outline", not the reference's literal "clear" --
    // the same documented deviation DrawNormal carries, for the same reason.
    // installNoClimbGround really does `Video.setColour("clear")` before its
    // drawFill, so the port is faithful; the problem is that this engine
    // models "clear" as ONE value per scheme while retail evidently
    // reassigns it between the full-screen background wash and the ground
    // fill inside a frame.
    //
    // In this level's scheme they collide exactly: s150_clear is
    // (0.819608,0.721569,0.294118) = rgb(209,184,75), which is the
    // background, so filling with it leaves ground pieces hollow -- outline
    // only, the "just lines" look.
    //
    // rgb(42,33,15) is what retail actually draws there, sampled directly
    // bottom ground band and the raised block), and it is the same value
    // this style's own outline already resolves to: "ground_outline" has no
    // s150_ entry in colours.dat, so it falls through to the sampled
    // (0.165,0.129,0.059) = rgb(42,32,15). Retail's ground reads as one
    // solid dark mass, not an outlined hollow shape.
    SetColourByName(a_r, "ground_outline");
    a_r.SetStencilFunc(kGreater, kNoclimbStencilValue);
    a_r.SetStencilOp(kKeep, kKeep, kReplace);
    DrawFill(a_r, a_g, kNoclimbClearZ);
    a_r.SetStencilFunc(kDisable, 0);
}

// installDangerGround -- game_data/Platformer/GroundProcessNEW.gm's own real
// shipped script literally says `Video.setColour("test1"/"test2")` for a
// tmpflag-flashing outline and "test0" for the fill (ground_renderer.h's own
// note: Platformer_GroundProcessNEW is a near-empty stub, retail's real
// ground drawing moved to native code this project can't recover the exact
// bytes of). A previous session substituted "danger_fill"/"danger_out"
// (real, sensible-looking red names) since "test0"/"test1" looked like dead
// placeholder data -- confirmed WRONG by direct pixel sampling against
// retail capture of this exact level/screen): the actual danger-ground
// cliff face on the left is a dark, near-black/brown wall (dominant sampled
// colour (83,73,60) over 160x450px, 2444/… samples), NOT red -- a solid red
// fill there was never visually correct. (83,73,60) matches this engine's
// own "ground_noClimb_fill" colours.dat entry ((87,77,63) for scheme 8)
// almost exactly, while colours.dat's own bare (non-scheme-scoped) "test0"
// -- the ACTUAL name the dead reference script names for this fill -- is
// itself pure near-black (0.000001,0.000001,0.000001), independently
// confirming "dark", not "red", was always the right family of colour here.
// Small red accents genuinely DO appear in the real screenshot (a red
// "boot" hazard icon, thin diagonal dashes) but sampled at only ~1.5% of
// the cliff's pixel area -- clearly a separately-drawn decoration, not this
// ground piece's own fill/outline. "ground_noClimb_out" (warm dusty-rose)
// is used for the outline instead of "test2" (colours.dat's bare "test2" is
// a dark blue/purple, implausible for what's visually a warm-toned edge in
// the real screenshot) since it's the real, confirmed-plausible sibling
// colour for "the dark, no-climb-like edge of a ground piece".
void DrawDanger(Renderer& a_r, const GroundGeometry& a_g) {
    SetColourByName(a_r, "ground_noClimb_out");
    DrawOutline(a_r, a_g, kGroundZ);

    SetColourByName(a_r, "ground_noClimb_fill");
    DrawFill(a_r, a_g, kGroundZ + 8.0f * kBackConversion);
}

// installFireGround -- confirmed real structure (a jagged line built from
// points offset outward along each edge's normal by a per-point random
// amplitude, animated by a spring-like relaxation + occasional on-beat
// kicks in the reference's update()/onMusicTrigger()). The animation state
// (t.fireLines[].vlist[].amp/ampVel, evolved frame to frame) isn't ported
// -- this recomputes a fresh, static jagged edge every frame instead (a
// deterministic per-point offset from a stable hash of its position, so it
// doesn't visibly crawl/shimmer like retail's, but has the same jagged
// "flame" silhouette rather than a smooth boundary). A real animated port
// is a reasonable follow-up once this engine has a place to keep
// per-ground persistent state across frames.
void DrawFire(Renderer& a_r, const GroundGeometry& a_g) {
    if (a_g.kind == kEdgeChain) return;
    SetColourByName(a_r, "fire");
    const std::vector<b2Vec2>& verts = a_g.verts;
    size_t n = verts.size();
    if (n >= 2) {
        std::vector<float> strip;
        strip.reserve(n * 4);
        for (size_t i = 0; i <= n; ++i) {
            const b2Vec2& p = verts[i % n];
            const b2Vec2& prev = verts[(i + n - 1) % n];
            b2Vec2 normal = RightNormal(VecToPointNormalized(prev, p));
            // Deterministic pseudo-random amplitude from position, standing
            // in for the reference's per-point animated `amp`.
            float h = std::fmod(std::abs(p.x * 12.9898f + p.y * 78.233f) * 43758.5453f, 1.0f);
            float amp = h * a_g.th * 5.0f;
            b2Vec2 outer = p + amp * normal;
            b2Vec2 inner = p - a_g.th * normal;
            strip.push_back(outer.x); strip.push_back(outer.y);
            strip.push_back(inner.x); strip.push_back(inner.y);
        }
        a_r.DrawPolygon(GL_TRIANGLE_STRIP, strip.data(), (int)(strip.size() / 2), kOutlineZ);
    }

    SetColourByName(a_r, "clear");
    DrawFill(a_r, a_g, kFireClearZ);
}

bool IsInViewNative(b2Body* a_body, Renderer& a_r) {
    b2AABB aabb; bool first = true;
    for (b2Fixture* fx = a_body->GetFixtureList(); fx; fx = fx->GetNext()) {
        for (int i = 0; i < fx->GetShape()->GetChildCount(); ++i) {
            b2AABB shapeAabb;
            fx->GetShape()->ComputeAABB(&shapeAabb, a_body->GetTransform(), i);
            if (first) { aabb = shapeAabb; first = false; }
            else aabb.Combine(shapeAabb);
        }
    }
    if (first) return true; // no fixtures -- don't cull something we can't measure
    // Visible world extent derives from the real perspective projection
    // (scales with camera Z) -- see Renderer::kTanHalfFovY.
    float halfH = a_r.GetWorldHalfHeight();
    float halfW = a_r.GetWorldHalfWidth();
    float scale = GetEngineContext().video.screenViewScale;
    if (scale > 0.0001f) { halfH /= scale; halfW /= scale; }
    float cx = a_r.GetCameraX(), cy = a_r.GetCameraY();
    return !(aabb.upperBound.x < cx - halfW || aabb.lowerBound.x > cx + halfW ||
             aabb.upperBound.y < cy - halfH || aabb.lowerBound.y > cy + halfH);
}

} // namespace

void DrawGroundsNative() {
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer) return;
    gmMachine* machine = GetEngineContext().machine;
    if (!machine) return;

    renderer->SetUseCameraView(true);
    // Depth test ON. This was `false`, which made every *_Z draw-depth
    // constant inert -- ordering fell back to pure draw order, so fixing
    // the Z constants to their real retail values changed nothing on
    // screen. Retail does depth-test: opcode 0x1c (setAttrDepthTest)
    // exists precisely to drive glEnable/glDisable(GL_DEPTH_TEST), and
    // the whole *_Z scheme is meaningless without it.
    //
    // The convention lines up: our ortho is glOrtho(..., -200, 200) with
    // modelview translate(0,0,-cameraZ), so z_eye = z_world - 26.88 and
    // z_ndc = -z_eye/200 -- larger world z maps to smaller depth, which
    // GL_LESS treats as nearer. That is retail's "larger = nearer", and
    // the real z range (-60 .. 30) sits well inside the clip volume.
    renderer->SetDepthTest(true);
    // Colour writes ON for this pass. This is a NATIVE draw pass, the
    // equivalent of retail's own ported-to-C++ ground renderer, so it must
    // establish its device state rather than inherit whatever the last
    // script draw list left behind -- and what it was inheriting was a
    // disabled colour mask (measured: colorMask=0000 every frame once a
    // level was running).
    //
    // Real content masks colour to lay down stencil (setColourMask(true),
    // see Renderer::SetColourMask) and does not always balance it back:
    // a full-game run logs 2213 mask-on calls against 4278 mask-off. The
    // per-frame reset in Renderer::BeginFrame fixes the frame boundary, but
    // a script pass during PreDrawAll re-masks colour and leaves it masked
    // for everything drawn afterwards -- which is every ground piece.
    //
    // Symptom this fixes: the tutorial level rendered as a single flat
    // background colour. The grounds were being submitted correctly the
    // whole time (5 pieces/frame surviving the in-view cull, real geometry,
    // colours resolving to rgb(193,135,43) against an rgb(209,184,75)
    // background) and every fragment was discarded by the colour mask.
    renderer->SetColourMask(false);
    for (b2Body* body = GetPhysicsWorld().GetBodyList(); body; body = body->GetNext()) {
        b2Fixture* fx = body->GetFixtureList();
        if (!fx) continue;
        if (fx->IsSensor()) continue;
        if ((fx->GetFilterData().categoryBits & kGroundCategory) == 0) continue;
        // drawGrounds(): `if (!t.entity.isInView()) { continue; }`
        if (!IsInViewNative(body, *renderer)) continue;

        gmVariable ownerEntity = FindOwnerEntityForB2Body(body);
        int style = 0; // NORMAL_GROUNDSTYLEID default if configureData.style is missing
        if (ownerEntity.m_type == GM_TABLE) {
            gmTableObject* et = (gmTableObject*)machine->GetObject(ownerEntity.m_value.m_ref);
            gmVariable cd = et->Get(machine, "configureData");
            if (cd.m_type == GM_TABLE) {
                gmTableObject* cdt = (gmTableObject*)machine->GetObject(cd.m_value.m_ref);
                gmVariable sv = cdt->Get(machine, "style");
                if (sv.m_type == GM_INT) style = sv.m_value.m_int;
            }
        }

        GroundGeometry geo;
        if (!BuildGeometry(body, geo)) continue;

        switch (style) {
            case 0: DrawNormal(*renderer, geo); break;   // NORMAL_GROUNDSTYLEID
            case 1: DrawNoClimb(*renderer, geo); break;  // NOCLIMB_GROUNDSTYLEID
            case 2: DrawFire(*renderer, geo); break;     // FIRE_GROUNDSTYLEID
            case 3: DrawDanger(*renderer, geo); break;   // DANGER_GROUNDSTYLEID
            // ICE(4)/MUD(5)/BOUNCY(6) -- confirmed real empty draw
            // functions in the reference (`t.draw=function(){};`), i.e.
            // these styles draw nothing even in retail's own pre-port GM
            // implementation. Matched exactly: no case, falls through to
            // drawing nothing.
            default: break;
        }
    }

    renderer->SetStencilFunc(kDisable, 0);
}
