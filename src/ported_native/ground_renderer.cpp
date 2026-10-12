#include <cstdio>
#include <cfloat>
#include <cstdlib>
#include <algorithm>
#include <set>
#include <string>
#include "ground_renderer.h"
#include "ground_math.h"
#include "../core/engine_context.h"
#include "../core/physics_world.h"
#include "../core/process_manager.h"
#include "../render/draw_list.h"
#include "../bindings/physics_body_bindings.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/colour_bindings.h"
#include "gmTableObject.h"
#include <box2d/box2d.h>
#include <GL/glew.h>
#include <vector>
#include <cmath>

// Verification: retail EntityRegistration.drawGrounds calls
// FUN_001ce4a0 (FUN_001d0520); palette strings/cell constants are PPC
// verified and geometry helpers match shipped GroundProcessNEW/VideoHelper
// source. The normal outline/fill split is an experimentally supported
// rendering hypothesis, not yet proven by the native command stream. The
// desktop pass preserves the bytecode-mapped style category order and the
// fire palette's traced 600 ms ping-pong interpolation, but does not reproduce
// per-cell command replay, stencil lifetime, or fire shader displacement.
namespace {

using GroundRenderMath::RetailGroundSinCos;
using GroundRenderMath::RetailGroundPow;

// Z layer constants -- confirmed real values, constants_bindings.cpp.
constexpr float kBackConversion = 0.01f;

// GROUND_Z, real retail value (see SOUND_SHAPES_GROUND_TRUTH.md section 8;
// read out of the bootstrap at 0x19b1c0..0x19b4f0). This was hardcoded
// +10.0 -- the WRONG SIGN -- and, being a local copy, it did not change
// when the GM global was corrected, so the ground kept drawing on the
// wrong side of the player. Every kOutlineZ/kClearZ/... below derives from
// it, so the whole ground sub-layer stack was mirrored around zero too.
constexpr float kGroundZ = -10.0f;
// CLEAR_Z/OUTLINE_Z/NOCLIMB_CLEAR_Z as computed inside GroundProcessNEW's
// constructor (OUTLINE_Z=GROUND_Z, CLEAR_Z=OUTLINE_Z+60*BACK_CONVERSION,
// NOCLIMB_CLEAR_Z=CLEAR_Z-20*BACK_CONVERSION). Keep these derived from the
// retail GROUND_Z/BACK_CONVERSION values above.
constexpr float kOutlineZ = kGroundZ;
constexpr float kClearZ = kOutlineZ + 60.0f * kBackConversion;
constexpr float kNoclimbClearZ = kClearZ - 20.0f * kBackConversion;


constexpr int kOutlineStencilValue = 1;
constexpr int kNoclimbStencilValue = 2;
// The batch passes DAT_001cc6c8 (-1) to the emitter. In mode zero this
// selects geometry[+0x24] - geometry[+0x28]; it is a subtraction selector,
// not a one-unit radius inset. +0x28 is the registration-specific inset.
constexpr float kRetailGroundEmitterInsetSelector = -1.0f;
// Base registration constructors pass these float values to
// BuildRegistrationGeometry: factory IDs 0/2 use +0.02, ID 1 uses -0.01.
// Factory ID 2 additionally builds another stream with its own -0.01 inset.
constexpr float kGroundRegistrationInsetDefault = 0.02f;
constexpr float kGroundRegistrationInsetId1 = -0.01f;

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
    float registrationInset = kGroundRegistrationInsetDefault; // object +0x28
    float th = 2.0f * kBackConversion; // outline thickness, matches t.th
    std::vector<b2Vec2> verts;     // world-space boundary verts (polygon/edge-chain)
    std::vector<b2Vec2> fillVerts; // inset boundary verts (polygon), or unused for circle
};

struct FireGroundGeometry {
    GroundGeometry dangerFill; // registration's primary cached geometry (+0.02)
    GroundGeometry dangerOut; // style-2 secondary geometry (-0.01)
    float amplitudePhase = 0.0f;
    float colourPhase = 0.0f;
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

// The retail PhysicsBody exposes fixtures in creation order. Box2D's linked
// fixture list is newest-first, so BuildGeometry reverses it before applying
// the ground constructor's shape-index rules.
int GroundShapeVertexCount(const b2Shape* a_shape) {
    if (!a_shape) return 0;
    switch (a_shape->GetType()) {
        case b2Shape::e_circle: return 1;
        case b2Shape::e_edge: return 2;
        case b2Shape::e_polygon:
            return static_cast<const b2PolygonShape*>(a_shape)->m_count;
        case b2Shape::e_chain:
            return static_cast<const b2ChainShape*>(a_shape)->m_count;
        default: return 0;
    }
}

bool GroundShapeLocalVertex(const b2Shape* a_shape, int a_index, b2Vec2& a_out) {
    if (!a_shape || a_index < 0 || a_index >= GroundShapeVertexCount(a_shape))
        return false;
    switch (a_shape->GetType()) {
        case b2Shape::e_circle:
            a_out = static_cast<const b2CircleShape*>(a_shape)->m_p;
            return a_index == 0;
        case b2Shape::e_edge: {
            const auto* edge = static_cast<const b2EdgeShape*>(a_shape);
            a_out = a_index == 0 ? edge->m_vertex1 : edge->m_vertex2;
            return true;
        }
        case b2Shape::e_polygon:
            a_out = static_cast<const b2PolygonShape*>(a_shape)->m_vertices[a_index];
            return true;
        case b2Shape::e_chain:
            a_out = static_cast<const b2ChainShape*>(a_shape)->m_vertices[a_index];
            return true;
        default: return false;
    }
}

// Box2D-side bounds for one PhysicsBody shape-list member. Retail's body-wide
// extent helpers union each member's center +/- half extent. Polygon widths
// exclude Box2D skin.
bool GroundShapeLocalBounds(const b2Shape* a_shape, b2Vec2& a_low, b2Vec2& a_high) {
    if (!a_shape) return false;
    if (a_shape->GetType() == b2Shape::e_circle) {
        const auto* circle = static_cast<const b2CircleShape*>(a_shape);
        const b2Vec2 half(circle->m_radius, circle->m_radius);
        a_low = circle->m_p - half;
        a_high = circle->m_p + half;
        return true;
    }
    const int count = GroundShapeVertexCount(a_shape);
    if (count <= 0) return false;
    b2Vec2 low, high;
    if (!GroundShapeLocalVertex(a_shape, 0, low)) return false;
    high = low;
    for (int i = 1; i < count; ++i) {
        b2Vec2 vertex;
        if (!GroundShapeLocalVertex(a_shape, i, vertex)) return false;
        low.x = std::min(low.x, vertex.x);
        low.y = std::min(low.y, vertex.y);
        high.x = std::max(high.x, vertex.x);
        high.y = std::max(high.y, vertex.y);
    }
    b2Vec2 center = 0.5f * (low + high);
    if (a_shape->GetType() == b2Shape::e_polygon)
        center = static_cast<const b2PolygonShape*>(a_shape)->m_centroid;
    const b2Vec2 half = 0.5f * (high - low);
    a_low = center - half;
    a_high = center + half;
    return true;
}

// GroundProcessNEW.gm::calculateFillVerts -- archived-script helper only.
// FUN_00195d28's configured-corner branch is handled below; this remains the
// fallback approximation for its other branches and unconfigured shapes.
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

// Verification: follows FUN_000f193c's GM_INT/GM_FLOAT conversion at the
// FUN_00195d28 field reads. Returns false for other types so callers can
// preserve the native zero-initialized default.
bool ReadRetailNumeric(gmMachine& a_machine, gmTableObject* a_table,
                       const char* a_key, float& a_value) {
    const gmVariable value = a_table->Get(&a_machine, a_key);
    if (value.m_type == GM_FLOAT) {
        a_value = value.m_value.m_float;
        return true;
    }
    if (value.m_type == GM_INT) {
        a_value = static_cast<float>(value.m_value.m_int);
        return true;
    }
    return false;
}

// PPC ground geometry forms x*x first, then uses fmadds(y, y, x*x), fsqrts,
// and component-wise fdivs. Keep this sequence explicit instead of using
// b2Vec2::Length(), whose multiply/add order is Box2D-defined.
float RetailGroundLength(const b2Vec2& a_vector) {
    return GroundRenderMath::RetailGroundLength(a_vector.x, a_vector.y);
}

// Verification: FUN_00195d28 and FUN_00194c40 initialize each numeric
// destination to zero, then overwrite it only for GM_FLOAT/GM_INT. Missing or
// non-numeric corner fields therefore contribute 0.0 to the cached stream.
float ReadRetailNumericOrZero(gmMachine& a_machine, gmTableObject* a_table,
                              const char* a_key) {
    float value = 0.0f;
    ReadRetailNumeric(a_machine, a_table, a_key, value);
    return value;
}

// Verification: FUN_00195560's PPC instructions build 16 inner contour
// points, one outer endpoint, 15 reversed normal-offset points, and a closing
// endpoint (33 total). Field reads, angle progression, transform, and append
// sites are traced; retail float sine and ground-domain power helpers are
// ported here.
bool BuildRetailCurveFill(gmMachine& a_machine, gmTableObject* a_owner,
                          std::vector<b2Vec2>& a_fill) {
    const gmVariable configVar = a_owner->Get(&a_machine, "configureData");
    if (configVar.m_type != GM_TABLE) return false;
    auto* config = static_cast<gmTableObject*>(a_machine.GetObject(configVar.m_value.m_ref));
    if (!config || config->Get(&a_machine, "curve").m_type == GM_NULL) return false;

    // The native callback zero-initializes values whose GM type is neither
    // float nor int. Preserve that behavior for its five configured scalars.
    auto readOrZero = [&](const char* key) {
        float value = 0.0f;
        ReadRetailNumeric(a_machine, config, key, value);
        return value;
    };
    const float centerX = readOrZero("posX");
    const float centerY = readOrZero("posY");
    const float rotation = readOrZero("editRotateValue");
    const float exponent = readOrZero("curve");
    const float stretch = readOrZero("editStretchValue");

    constexpr float kPiOver32Negative = -0.09817477315664291f;
    constexpr float kHalfPiNegative = -1.5707963267948966f;
    constexpr float kReturnOffset = 1.2200000286102295f;
    const float innerRadius = 2.0f * stretch;
    const float outerRadius = innerRadius + kReturnOffset;

    auto signedPower = [&](float angle, bool cosine) {
        const float sample = RetailGroundSinCos(angle, cosine);
        const float magnitude = RetailGroundPow(std::fabs(sample), exponent);
        return sample < 0.0f ? -magnitude : magnitude;
    };
    auto transform = [&](float x, float y) {
        const float sine = RetailGroundSinCos(rotation, false);
        const float cosine = RetailGroundSinCos(rotation, true);
        // PPC helper 0x000c5f64 rounds y*sin / y*cos with fmuls, then uses
        // fmsubs / fmadds for the two rotated coordinates. Add the configured
        // center separately, matching FUN_00195560's caller sequence.
        return b2Vec2(std::fma(x, cosine, -y * sine) + centerX,
                      std::fma(x, sine, y * cosine) + centerY);
    };
    auto contourPoint = [&](float angle, float radius) {
        const float x = std::fma(signedPower(angle, true), radius, -stretch);
        const float y = std::fma(signedPower(angle, false), radius, stretch);
        return transform(x, y);
    };

    a_fill.clear();
    a_fill.reserve(33);
    a_fill.push_back(contourPoint(0.0f, innerRadius));
    float angle = kPiOver32Negative;
    for (int i = 1; i < 16; ++i) {
        a_fill.push_back(contourPoint(angle, innerRadius));
        angle = static_cast<float>(angle + kPiOver32Negative);
    }

    // Retail bridges the inner contour's final point to this enlarged endpoint.
    a_fill.push_back(contourPoint(kHalfPiNegative, outerRadius));

    // Walk inner points 15..1 backwards. Retail offsets each along the
    // left-hand normal of its incoming edge; the start point is closed by the
    // explicit outer-radius point below.
    for (int i = 15; i >= 1; --i) {
        const b2Vec2 current = a_fill[static_cast<size_t>(i)];
        const b2Vec2 previous = a_fill[static_cast<size_t>(i - 1)];
        const b2Vec2 edge = current - previous;
        const float length = RetailGroundLength(edge);
        b2Vec2 offset = current;
        if (length > 0.0f) {
            const float unitX = edge.x / length;
            const float unitY = edge.y / length;
            offset.x = std::fma(-unitY, kReturnOffset, current.x);
            offset.y = std::fma(unitX, kReturnOffset, current.y);
        }
        a_fill.push_back(offset);
    }
    a_fill.push_back(contourPoint(0.0f, outerRadius));
    return true;
}

// Verification: both non-curve branches of retail FUN_00195d28 are traced
// through their PPC record stores and output loops: FUN_00194c40 reads three
// configData corners and emits 18 samples; the inline four-corner branch
// reads owner corners and emits 24. The length/divide/FMA sequence follows
// the disassembly; PPC fsqrts rounding and matched runtime output remain open.
bool BuildRetailConfiguredCornerFill(gmMachine& a_machine,
                                     gmTableObject* a_owner,
                                     float a_inset,
                                     std::vector<b2Vec2>& a_fill) {
    const gmVariable configVar = a_owner->Get(&a_machine, "configureData");
    if (configVar.m_type != GM_TABLE) return false;
    auto* config = static_cast<gmTableObject*>(a_machine.GetObject(configVar.m_value.m_ref));
    if (!config) return false;

    // A present curve selects the separate 33-point superellipse contour.
    if (config->Get(&a_machine, "curve").m_type != GM_NULL) return false;
    // Missing dx dispatches to FUN_00194c40, whose three corner pairs live in
    // configureData. Present dx selects the inline owner-corner branch.
    const bool threeCornerPath =
        a_owner->Get(&a_machine, "dx").m_type == GM_NULL;

    const float centerX = ReadRetailNumericOrZero(a_machine, config, "posX");
    const float centerY = ReadRetailNumericOrZero(a_machine, config, "posY");
    const int cornerCount = threeCornerPath ? 3 : 4;
    const char* cornerKeys[4][2] = {{"ax", "ay"}, {"bx", "by"},
                                    {"cx", "cy"}, {"dx", "dy"}};
    gmTableObject* cornerTable = threeCornerPath ? config : a_owner;
    b2Vec2 corners[4];
    for (int i = 0; i < cornerCount; ++i) {
        corners[i].x = ReadRetailNumericOrZero(a_machine, cornerTable, cornerKeys[i][0]);
        corners[i].y = ReadRetailNumericOrZero(a_machine, cornerTable, cornerKeys[i][1]);
    }

    // PPC uses float sqrt/divide and fma for this inset. Keep intermediate
    // vectors in float and preserve the retail neighbor order (next, previous).
    b2Vec2 inset[4];
    for (int i = 0; i < cornerCount; ++i) {
        const b2Vec2 toNext = corners[(i + 1) % cornerCount] - corners[i];
        const b2Vec2 toPrev = corners[(i + cornerCount - 1) % cornerCount] - corners[i];
        const float nextLen = RetailGroundLength(toNext);
        const float prevLen = RetailGroundLength(toPrev);
        const b2Vec2 nextDir = nextLen > 0.0f
            ? b2Vec2(toNext.x / nextLen, toNext.y / nextLen) : b2Vec2(0, 0);
        const b2Vec2 prevDir = prevLen > 0.0f
            ? b2Vec2(toPrev.x / prevLen, toPrev.y / prevLen) : b2Vec2(0, 0);
        const b2Vec2 directionSum = nextDir + prevDir;
        inset[i].x = std::fma(directionSum.x, a_inset, corners[i].x);
        inset[i].y = std::fma(directionSum.y, a_inset, corners[i].y);
    }

    a_fill.clear();
    a_fill.reserve(static_cast<size_t>(cornerCount * 6));
    constexpr float kAngleStep = 0.6283185482025146f;
    constexpr float kBaseStep = 0.2f;
    // Retail loads 0.388 as a double, multiplies the float distance in double,
    // then rounds the result back to float with frsp.
    constexpr double kBulgeScale = 0.388;
    for (int i = 0; i < cornerCount; ++i) {
        const b2Vec2 previous = inset[(i + cornerCount - 1) % cornerCount];
        const b2Vec2 current = inset[i];
        const b2Vec2 next = inset[(i + 1) % cornerCount];
        const b2Vec2 incoming = current - previous;
        const b2Vec2 outgoing = next - current;
        const float incomingLength = RetailGroundLength(incoming);
        const float outgoingLength = RetailGroundLength(outgoing);
        const float incomingTrim = threeCornerPath || incomingLength > 0.2618f
            ? 0.2318f : incomingLength * 0.19f;
        const float outgoingTrim = threeCornerPath || outgoingLength > 0.2618f
            ? 0.2318f : outgoingLength * 0.19f;
        const b2Vec2 incomingDir = incomingLength > 0.0f
            ? b2Vec2(incoming.x / incomingLength, incoming.y / incomingLength)
            : b2Vec2(0, 0);
        const b2Vec2 outgoingDir = outgoingLength > 0.0f
            ? b2Vec2(outgoing.x / outgoingLength, outgoing.y / outgoingLength)
            : b2Vec2(0, 0);
        const b2Vec2 p0{
            std::fma(-incomingDir.x, incomingTrim, current.x),
            std::fma(-incomingDir.y, incomingTrim, current.y)};
        const b2Vec2 p1{
            std::fma(outgoingDir.x, outgoingTrim, current.x),
            std::fma(outgoingDir.y, outgoingTrim, current.y)};
        const b2Vec2 chord = p1 - p0;
        const float chordLength = RetailGroundLength(chord);
        // PPC forms P0 + chord * 0.5 with fused single-precision fmadds.
        const b2Vec2 midpoint{
            std::fma(chord.x, 0.5f, p0.x),
            std::fma(chord.y, 0.5f, p0.y)};
        const float bulge = static_cast<float>(
            static_cast<double>(RetailGroundLength(current - midpoint)) * kBulgeScale);
        float angle = 0.0f;
        b2Vec2 base = p0;
        for (int sample = 0; sample < 6; ++sample) {
            const float wave = RetailGroundSinCos(angle, false);
            float bulgeX = 0.0f;
            float bulgeY = 0.0f;
            if (chordLength > 0.0f) {
                bulgeX = (chord.x / chordLength) * bulge;
                bulgeY = (chord.y / chordLength) * bulge;
            }
            // Retail PPC uses the normalized chord X component for the X
            // offset and the negated normalized chord Y component for Y.
            // Preserve its component-wise reflection and arithmetic order.
            float sampleX;
            if (threeCornerPath) {
                volatile float xProduct = wave * bulgeX;
                sampleX = xProduct + base.x;
            } else {
                sampleX = std::fma(wave, bulgeX, base.x);
            }
            // PPC 0x196934..0x196944 computes the Y offset with a rounded
            // multiply, fneg, then a separate fadds into the base. Only X
            // uses fmadds at 0x196938. Keep this unfused on both corner
            // paths; the three-corner helper also emits separate multiply/add
            // operations for X.
            volatile float yProduct = wave * bulgeY;
            const float yOffset = -yProduct;
            const float sampleY = yOffset + base.y;
            a_fill.emplace_back(sampleX + centerX,
                                sampleY + centerY);
            angle += kAngleStep;
            base.x = std::fma(chord.x, kBaseStep, base.x);
            base.y = std::fma(chord.y, kBaseStep, base.y);
        }
    }
    return true;
}

bool BuildGeometry(b2Body* a_body, GroundGeometry& a_out) {
    std::vector<b2Fixture*> shapes;
    for (b2Fixture* f = a_body->GetFixtureList(); f; f = f->GetNext())
        shapes.push_back(f);
    if (shapes.empty()) return false;
    std::reverse(shapes.begin(), shapes.end());
    b2Fixture* first = shapes.front(); // retail shape index 0
    const b2Transform& xf = a_body->GetTransform();

    // FUN_00033dc8/FUN_00033ed8 receive the PhysicsBody and walk its full
    // shape list, unioning each member's local center +/- half its extent.
    // FUN_001cb3c8 uses half the larger resulting dimension for circle and
    // polygon registration radius. Edge mode replaces this with its separate
    // world-space endpoint bounds below.
    b2Vec2 bodyLow(FLT_MAX, FLT_MAX), bodyHigh(-FLT_MAX, -FLT_MAX);
    for (b2Fixture* fixture : shapes) {
        b2Vec2 low, high;
        if (!GroundShapeLocalBounds(fixture->GetShape(), low, high)) continue;
        bodyLow.x = std::min(bodyLow.x, low.x);
        bodyLow.y = std::min(bodyLow.y, low.y);
        bodyHigh.x = std::max(bodyHigh.x, high.x);
        bodyHigh.y = std::max(bodyHigh.y, high.y);
    }
    if (bodyLow.x == FLT_MAX) return false;
    a_out.radius = 0.5f * std::max(bodyHigh.x - bodyLow.x,
                                  bodyHigh.y - bodyLow.y);

    if (first->GetShape()->GetType() == b2Shape::e_circle) {
        a_out.kind = kCircle;
        const b2CircleShape* c = (const b2CircleShape*)first->GetShape();
        a_out.pos = b2Mul(xf, c->m_p);
        // Retail's GroundRegistration type-0 constructor derives its cached
        // perimeter count from half the larger registered-body extent instead
        // of using a fixed 61 points (PPC FUN_001cb3c8). Retail's MATH_POW
        // ordinary positive path is ported for this clamped extent and .3f.
        const float normalizedExtent = std::clamp(
            (a_out.radius - 0.61f) / (13.42f - 0.61f), 0.0f, 1.0f);
        const int kSegs = static_cast<int>(
            RetailGroundPow(normalizedExtent, 0.3f) * 45.0f) + 16;
        a_out.verts.reserve(kSegs);
        for (int i = 0; i < kSegs; ++i) {
            float ang = 1.5f * (float)M_PI +
                (float)i / (float)kSegs * 2.0f * (float)M_PI;
            a_out.verts.push_back(a_out.pos + a_out.radius * b2Vec2{RetailGroundSinCos(ang, true), RetailGroundSinCos(ang, false)});
        }
        return true;
    }
    const int firstShapeVertexCount = GroundShapeVertexCount(first->GetShape());
    if (firstShapeVertexCount >= 3) {
        a_out.kind = kPolygon;
        a_out.verts.reserve(firstShapeVertexCount);
        for (int i = 0; i < firstShapeVertexCount; ++i) {
            b2Vec2 local;
            if (!GroundShapeLocalVertex(first->GetShape(), i, local)) return false;
            b2Vec2 v = b2Mul(xf, local);
            a_out.verts.push_back(v);
        }
        const b2Shape* shape = first->GetShape();
        if (shape->GetType() == b2Shape::e_polygon) {
            // FUN_00032a18 calls the shape center virtual and applies the
            // body transform. Box2D polygon centroid is the matching local
            // center field; an AABB midpoint is wrong for irregular polygons.
            const auto* polygon = static_cast<const b2PolygonShape*>(shape);
            a_out.pos = b2Mul(xf, polygon->m_centroid);
        } else {
            b2AABB bounds;
            shape->ComputeAABB(&bounds, xf, 0);
            a_out.pos = 0.5f * (bounds.lowerBound + bounds.upperBound);
        }
        a_out.fillVerts = CalculateFillVerts(a_out.verts, a_out.th);
        return true;
    }
    // PPC FUN_001cb3c8 edge-chain branch uses shape indices
    // [0, shapeCount-2]. It scans both endpoints for the combined bounds,
    // but its cached mode-2 vertex stream stores only vertex 0 of each shape.
    // Preserve those as separate products: deriving the stream from the
    // perimeter endpoints changes both vertex count and strip ordering.
    a_out.kind = kEdgeChain;
    const size_t edgeCount = shapes.size() - 1;
    a_out.verts.reserve(edgeCount);
    b2Vec2 low(FLT_MAX, FLT_MAX), high(-FLT_MAX, -FLT_MAX);
    for (size_t i = 0; i < edgeCount; ++i) {
        const b2Shape* shape = shapes[i]->GetShape();
        b2Vec2 v0, v1;
        if (GroundShapeVertexCount(shape) < 2 ||
            !GroundShapeLocalVertex(shape, 0, v0) ||
            !GroundShapeLocalVertex(shape, 1, v1))
            return false;
        const b2Vec2 worldEndpoints[2] = {b2Mul(xf, v0), b2Mul(xf, v1)};
        for (const b2Vec2& world : worldEndpoints) {
            low.x = std::min(low.x, world.x);
            low.y = std::min(low.y, world.y);
            high.x = std::max(high.x, world.x);
            high.y = std::max(high.y, world.y);
        }
        a_out.verts.push_back(worldEndpoints[0]);
    }
    if (edgeCount == 0) return false;
    a_out.pos = 0.5f * (low + high);
    const b2Vec2 halfExtent = 0.5f * (high - low);
    a_out.radius = std::max(halfExtent.x, halfExtent.y);
    a_out.fillVerts = CalculateFillVerts(a_out.verts, a_out.th);
    return true;
}

// The archived ground renderer emits a closed SHAPETYPE_LINESTRIP for
// polygon outlines. Keep that primitive and the active Video line-width
// state; expanding edges into world-space quads changes joins and overlaps.
void DrawOutline(Renderer& a_r, const GroundGeometry& a_g, float a_z) {
    if (a_g.kind == kEdgeChain || a_g.verts.empty()) return;
    if (a_g.kind == kCircle) {
        // VIDEOHELPER_DrawCircle replays circleShape ring indices 1..60 as
        // SHAPETYPE_LINELOOP. That shared shape has 60 segments, starts at
        // angle zero, and stores a duplicate closing vertex that the outline
        // range deliberately omits. GroundRegistration's own `verts` uses a
        // separate 61 point sequence for style setup and is not this draw.
        constexpr int kCircleSegments = 60;
        std::vector<float> xy;
        xy.reserve(kCircleSegments * 2);
        for (int i = 0; i < kCircleSegments; ++i) {
            const float ang = (float)i / (float)kCircleSegments *
                2.0f * (float)M_PI;
            const b2Vec2 p = a_g.pos + a_g.radius *
                b2Vec2{RetailGroundSinCos(ang, true), RetailGroundSinCos(ang, false)};
            xy.push_back(p.x); xy.push_back(p.y);
        }
        a_r.DrawPolygon(GL_LINE_LOOP, xy.data(), kCircleSegments, a_z);
        return;
    }
    std::vector<float> xy;
    xy.reserve((a_g.verts.size() + 1) * 2);
    for (const b2Vec2& v : a_g.verts) { xy.push_back(v.x); xy.push_back(v.y); }
    xy.push_back(a_g.verts.front().x);
    xy.push_back(a_g.verts.front().y);
    a_r.DrawPolygon(GL_LINE_STRIP, xy.data(), (int)xy.size() / 2, a_z);
}
void DrawFill(Renderer& a_r, const GroundGeometry& a_g, float a_z,
              float a_fireAmp = 0.0f, bool a_fireShader = false) {
    const std::vector<b2Vec2>& verts = (a_g.kind == kCircle) ? a_g.verts : a_g.fillVerts;
    if (verts.empty()) return;
    std::vector<float> xy;
    std::vector<float> data;
    xy.reserve((verts.size() + 4) * 2);
    if (a_fireShader) data.reserve((verts.size() + 4) * 2);
    auto append = [&xy, &data, &a_g, a_fireShader](const b2Vec2& p) {
        xy.push_back(p.x);
        xy.push_back(p.y);
        if (a_fireShader) {
            // Best-effort substitute for the unresolved retail TEXCOORD0
            // (RSX attribute 8) payload: radial unit direction from the
            // registered ground center. The microcode operation is exact,
            // but this data source is explicitly unverified.
            const b2Vec2 direction = Normalized(p - a_g.pos);
            data.push_back(direction.x);
            data.push_back(direction.y);
        }
    };

    if (a_g.kind == kCircle) {
        // PPC FUN_001cb9b0 mode 0 emits an upper/lower pair at every phase
        // from zero through ceil(count/2), then emits the leftmost point
        // twice. Registration +0x24 is the radius, +0x28 is its per-style
        // inset, and +0x2c/+0x30 are the transformed center. The builder's
        // raw -1 argument selects radius - registrationInset.
        // std::fma preserves the retail fused multiply-add shape; the trig
        // helper's final numeric parity remains separately tracked.
        const int count = (int)verts.size();
        const int halfCount = (count + 1) / 2;
        const float radius = a_g.radius +
            kRetailGroundEmitterInsetSelector * a_g.registrationInset;
        const float step = 2.0f * (float)M_PI / (float)count;
        float angle = 0.0f;
        for (int i = 0; i <= halfCount; ++i) {
            const float x = std::fma(RetailGroundSinCos(angle, true), radius, a_g.pos.x);
            const float sine = RetailGroundSinCos(angle, false);
            append({x, std::fma(sine, radius, a_g.pos.y)});
            append({x, std::fma(-sine, radius, a_g.pos.y)});
            angle += step;
        }
        const b2Vec2 leftmost{a_g.pos.x - radius, a_g.pos.y};
        append(leftmost);
        append(leftmost);
    } else {
        // PPC FUN_001cb9b0 modes 1/2 copy cached float2 records in mirrored
        // order for the triangle strip: initial record, forward/reverse pairs,
        // then the center record. This preserves the emitted degenerates at
        // the strip ends instead of converting the boundary to a fan.
        const int count = (int)verts.size();
        const int halfCount = (count + 1) / 2;
        append(verts.front());
        for (int i = 0; i < halfCount; ++i) {
            append(verts[(size_t)i]);
            append(verts[(size_t)(count - 1 - i)]);
        }
        append(verts[(size_t)(count - halfCount)]);
    }
    if (a_fireShader)
        a_r.DrawFireGroundStrip(xy.data(), data.data(), (int)xy.size() / 2, a_fireAmp, a_z);
    else
        a_r.DrawPolygon(GL_TRIANGLE_STRIP, xy.data(), (int)xy.size() / 2, a_z);
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
// The extracted GroundProcessNEW.gm is a reference stub: retail marks the
// ground draw path as ported to native C++, so its dead script callback is
// not authoritative for the native fill colour. The live retail level-1
// capture and scheme-8 palette agree on ground_fill (yellow) for terrain;
// using clear here aliases the sky colour and makes the terrain disappear.
void DrawNormalOutline(Renderer& a_r, const GroundGeometry& a_g) {
    a_r.SetStencilFunc(kGreater, kOutlineStencilValue);
    a_r.SetStencilOp(kKeep, kKeep, kReplace);
    SetColourByName(a_r, "ground_out");
    DrawOutline(a_r, a_g, kOutlineZ);
}

void DrawNormalFill(Renderer& a_r, const GroundGeometry& a_g) {
    a_r.SetStencilFunc(kDisable, 0);
    SetColourByName(a_r, "ground_fill");
    DrawFill(a_r, a_g, kClearZ);
}

void DrawNormal(Renderer& a_r, const GroundGeometry& a_g) {
    DrawNormalOutline(a_r, a_g);
    DrawNormalFill(a_r, a_g);
}

// installNoClimbGround -- confirmed real; note the stencil wrap is on the
// FILL here, not the outline (opposite of installNormalGround) -- matches
// the reference exactly, not a copy-paste of Normal's order.
void DrawNoClimb(Renderer& a_r, const GroundGeometry& a_g) {
    SetColourByName(a_r, "ground_noClimb_out");
    DrawOutline(a_r, a_g, kOutlineZ);

    SetColourByName(a_r, "ground_noClimb_fill");
    a_r.SetStencilFunc(kGreater, kNoclimbStencilValue);
    a_r.SetStencilOp(kKeep, kKeep, kReplace);
    DrawFill(a_r, a_g, kNoclimbClearZ);
    a_r.SetStencilFunc(kDisable, 0);
}

// The style-2 batch emits two distinct triangle strips: the primary cached
// geometry to the animated danger_fill list, then the secondary registration
// geometry to danger_out. The coordinate-hash strip previously drawn here was
// invented and did not match either retail stream. The owner's two phases are
// stepped by ProcessManager from the traced update callback. Drawing consumes
// those values without advancing animation based on camera-visible geometry.
// FireGroundShader2's aData input remains unresolved.
void DrawFire(Renderer& a_r, const FireGroundGeometry& a_g) {
    const float colourPhase = a_g.colourPhase;
    const float amp = a_g.amplitudePhase * 0.061f;
    float r, g, b;
    if (GetInterpolatedNamedColourRGB((1.0f - colourPhase) * 0.61f + 0.39f,
                                      "danger_fill", "danger_out", r, g, b))
        a_r.SetColour(r, g, b, 1.0f);
    else
        SetColourByName(a_r, "danger_fill");
    // Retail FUN_001ce4a0 writes aZ=-10.0 to FireGroundShader2; its paired
    // Default2d parameter is also -10.0. Do not use the script's computed
    // FIRE_CLEAR_Z (-9.7) for these native shader-backed streams.
    DrawFill(a_r, a_g.dangerFill, kGroundZ, amp, true);

    SetColourByName(a_r, "danger_out");
    DrawFill(a_r, a_g.dangerOut, kGroundZ);
}

} // namespace

namespace {
class GroundRenderStateScope {
public:
    explicit GroundRenderStateScope(Renderer& a_renderer)
        : m_renderer(a_renderer), m_viewState(a_renderer.GetViewState()) {
        a_renderer.GetColour(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);
        // Retail's 0x1a/0x1b scope restores depth, stencil, colour-mask,
        // blend, and other fixed-function state after the ground commands.
        glPushAttrib(GL_ALL_ATTRIB_BITS);
    }

    ~GroundRenderStateScope() {
        glPopAttrib();
        m_renderer.SetColour(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);
        m_renderer.RestoreViewState(m_viewState);
    }

    GroundRenderStateScope(const GroundRenderStateScope&) = delete;
    GroundRenderStateScope& operator=(const GroundRenderStateScope&) = delete;

private:
    Renderer& m_renderer;
    Renderer::ViewState m_viewState;
    float m_colour[4]{};
};

void DrawGroundsNativeNow() {
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer) return;
    gmMachine* machine = GetEngineContext().machine;
    ProcessManager* processManager = GetEngineContext().processManager;
    if (!machine || !processManager) return;

    GroundRenderStateScope renderStateScope(*renderer);
    // Retail FUN_001ce4a0 calls FUN_0003e198(drawList, false): use world view
    // bounds while skipping FUN_0004cea0's camera-rotation branch.
    renderer->SetWorldView(false);
    // Visible cell registrations replay through the 0x1cc448 per-cell path,
    // which records DrawList opcode 0x1c with index 1. Dispatcher 0x44d4c
    // selects DAT_00431684[1] = 0x201 (LESS) and enables depth testing. The
    // separate owner child DrawList at +0x190 has two additional scoped
    // replays that are not represented by this registration renderer yet.
    renderer->SetDepthTestCompareIndex(1);
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
    std::vector<std::pair<int, Process::GroundRegistration>> registrations;
    processManager->GetGroundRegistrationsForCamera(
        renderer->GetCameraX(), renderer->GetCameraY(), registrations);
    std::vector<GroundGeometry> normalGrounds;
    std::vector<GroundGeometry> noClimbGrounds;
    std::vector<FireGroundGeometry> fireGrounds;
    normalGrounds.reserve(registrations.size());
    // The native EntityRegistration renderer gathers grounds through its
    // camera-visible cell grid before drawing. Do not use Box2D's body linked
    // list: Box2D prepends bodies, which reverses creation order and changes
    // painter order when ground shapes overlap.
    for (const auto& registration : registrations) {
        gmVariable* ownerEntity = FindEntityByObjectID(registration.first);
        if (!ownerEntity || ownerEntity->m_type != GM_TABLE) continue;
        gmTableObject* et = (gmTableObject*)machine->GetObject(ownerEntity->m_value.m_ref);
        if (!et) continue;
        gmVariable physicsData = et->Get(machine, "PhysicsData");
        if (physicsData.m_type == GM_NULL)
            physicsData = et->Get(machine, "__physicsData");
        b2Body* retailGroundBody = GetB2BodyFromVar(
                *machine, GetPhysicsDataMainBodyVar(*machine, physicsData));
        b2Body* body = retailGroundBody;
        if (!body) continue;
        b2Fixture* fx = body->GetFixtureList();
        // Registration is the retail visibility filter. The archived ground
        // setup reads the registered body's shape directly and does not skip
        // sensors or inspect collision category bits. Keep only the required
        // existence check; extra fixture filtering can silently drop a
        // registered visual ground whose collision flags differ.
        if (!fx) continue;
        const int style = registration.second.style;
        (void)registration.second.selected; // native draw call's selected behavior is still under audit.

        GroundGeometry geo;
        geo.registrationInset = style == 1
            ? kGroundRegistrationInsetId1
            : kGroundRegistrationInsetDefault;
        if (!BuildGeometry(body, geo)) continue;
        if (geo.kind == kPolygon && style >= 0 && style <= 2) {
            const float registrationInset = style == 1
                ? kGroundRegistrationInsetId1
                : kGroundRegistrationInsetDefault;
            if (!BuildRetailCurveFill(*machine, et, geo.fillVerts))
                BuildRetailConfiguredCornerFill(*machine, et, registrationInset,
                                                geo.fillVerts);
        }

        if (std::getenv("GE3_TRACE_GROUND_GEOMETRY")) {
            static std::set<int> traced;
            if (traced.insert(registration.first).second) {
                float minX = geo.pos.x, maxX = geo.pos.x;
                float minY = geo.pos.y, maxY = geo.pos.y;
                for (const b2Vec2& v : geo.verts) {
                    minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
                    minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
                }
                std::fprintf(stderr,
                    "[ground-geometry] id=%d style=%d kind=%d pos=(%.3f,%.3f) radius=%.3f bounds=(%.3f,%.3f)-(%.3f,%.3f) vertices=%zu\n",
                    registration.first, style, (int)geo.kind, geo.pos.x, geo.pos.y,
                    geo.radius, minX, minY, maxX, maxY, geo.verts.size());
            }
        }

        switch (style) {
            // Normal style is split into outline and fill submissions. The
            // grouped pass removes internal circle arcs in the same-camera
            // retail comparison; exact native command ordering remains open.
            case 0: normalGrounds.push_back(std::move(geo)); break; // NORMAL_GROUNDSTYLEID
            case 1: noClimbGrounds.push_back(std::move(geo)); break; // NOCLIMB_GROUNDSTYLEID
            case 2: {
                // Factory ID 2 constructs a second geometry destination at
                // registration +0x64 with inset -0.01. The batch emits the
                // primary cached records to +0x50 and that secondary cache to
                // +0x54; preserve them as distinct child-list submissions.
                FireGroundGeometry fire;
                fire.amplitudePhase = registration.second.fireAmplitudePhase;
                fire.colourPhase = registration.second.fireColourPhase;
                fire.dangerFill = std::move(geo);
                fire.dangerOut = fire.dangerFill;
                fire.dangerOut.registrationInset = kGroundRegistrationInsetId1;
                if (fire.dangerOut.kind == kPolygon) {
                    if (!BuildRetailCurveFill(*machine, et,
                                              fire.dangerOut.fillVerts))
                        BuildRetailConfiguredCornerFill(*machine, et, -0.01f,
                                                        fire.dangerOut.fillVerts);
                }
                fireGrounds.push_back(std::move(fire));
                break;
            } // FIRE_GROUNDSTYLEID
            // Retail's selector -1 rebuild only emits style vectors for IDs
            // 0, 1, and 2. IDs >=3 remain in generic registration bookkeeping
            // and have no stream in this native pass; any separate draw route
            // is unresolved, so don't synthesize a style-3 fill here.
            default: break;
        }
    }

    // Retail's per-cell batch handoff submits style 0 (+0x44), then style 1
    // (+0x4c), then both style-2 streams (+0x50/+0x54). Keep that proven
    // category order across the direct desktop submissions too. Previously
    // no-climb/fire were drawn during collection and normal ground was
    // deferred until afterward, reversing the retail ordering for overlapping
    // styles.
    for (const GroundGeometry& geo : normalGrounds)
        DrawNormalOutline(*renderer, geo);
    for (const GroundGeometry& geo : normalGrounds)
        DrawNormalFill(*renderer, geo);
    for (const GroundGeometry& geo : noClimbGrounds)
        DrawNoClimb(*renderer, geo);
    for (const FireGroundGeometry& fire : fireGrounds)
        DrawFire(*renderer, fire);

    // Do not add the diagonal no-climb hatch from the archived
    // PORTED_TO_CPP______Platformer_GroundProcessNEW script here. The active
    // retail entry is native (0x1ce4a0); this hatch is not emitted by its
    // per-cell geometry handoff. Its trailing +0x190 child-list replays remain
    // under audit, so any equivalent native pattern must come from that path.
}
} // namespace

void RecordGroundsNative(DrawListData* a_drawList) {
    if (!a_drawList || !a_drawList->commandBuffer) return;
    // Main.gm calls EntityRegistration.drawGrounds(drawList) between its
    // graphics preDraw pass and sticky-ground pass. Preserve that exact
    // command position when the owning list is replayed later this frame.
    a_drawList->commandBuffer->commands.emplace_back([] {
        DrawGroundsNativeNow();
    });
}
