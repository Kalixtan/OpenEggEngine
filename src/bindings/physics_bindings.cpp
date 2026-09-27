#include "gmThread.h"
#include "physics_bindings.h"
#include "vector2d_bindings.h"
#include "gmTableObject.h"
#include <box2d/box2d.h>
#include <vector>
#include <cstdio>
#include <memory>
#include <algorithm>
#include <cmath>

gmType g_physicsShapeDefType = -1;

namespace {

// alias kept short for the rest of this file; struct itself now lives in
// physics_bindings.h so physics_body_bindings.cpp's appendShapes(sd) can
// consume it too.
gmType& g_shapeDefType = g_physicsShapeDefType;

ShapeDefCollection* ThisCollection(gmThread* a_thread) {
    return (ShapeDefCollection*)a_thread->ThisUserCheckType(g_shapeDefType);
}

int GM_CDECL gmf_PHYSICSSHAPECOLLECTIONDEF(gmThread* a_thread) {
    a_thread->PushNewUser(new ShapeDefCollection(), g_shapeDefType);
    return GM_OK;
}

int GM_CDECL gmf_reset(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    c->Reset();
    return GM_OK;
}

// addBox(width, height, offsetX, offsetY, angle) -- confirmed real method,
// game_data/Platformer/EntityPacks/GoAroundGuy.gm (`sd.addBox(w,h,0,0,0)`).
int GM_CDECL gmf_addBox(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(width, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(height, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(offsetX, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(offsetY, 3, 0.0f);
    GM_FLOAT_OR_INT_PARAM(angle, 4, 0.0f);

    // Guard against a degenerate (near-zero-area) box: Box2D's own
    // b2PolygonShape::ComputeMass hard-asserts (`area > FLT_EPSILON`,
    // aborting the whole process, not a catchable GM exception) rather
    // than tolerating one. Real level data can produce these (confirmed
    // crash: Platformer/EntityPacks/BrianMover.gm's constructor chain
    // eventually creates one) -- clamping to a tiny minimum extent is a
    // safe stand-in since a hairline-thin collision box is functionally
    // equivalent to "no real collision" anyway, which is a much better
    // failure mode than crashing the engine.
    constexpr float kMinBoxExtent = 0.001f;
    float hx = std::max(width * 0.5f, kMinBoxExtent);
    float hy = std::max(height * 0.5f, kMinBoxExtent);
    b2PolygonShape* box = new b2PolygonShape();
    box->SetAsBox(hx, hy, b2Vec2(offsetX, offsetY), angle);
    c->AddFixture(box);
    return GM_OK;
}

// addPolygon(vertices, count) -- confirmed real (real usage:
// Platformer/EntityPacks/GroundResizeNoRotate.gm/TriangleGround.gm --
// together 183 of this corpus's confirmed 225-entity tutorial level --
// both build an arbitrary vector2D point list and call
// `sd.addPolygon(vertices,tableCount(vertices))`). Was entirely missing,
// so every ground entity's `pb.appendShapes(sd)` silently added ZERO
// fixtures (confirmed: 230 live bodies but only 47 real fixtures before
// this fix) -- ground pieces existed as empty, invisible physics bodies.
// `vertices` is a plain GM table of vector2D (0-indexed, matching every
// other array-shaped table in this corpus); b2PolygonShape::Set computes
// the convex hull itself and caps at b2_maxPolygonVertices (8) -- real
// ground pieces are simple quads/triangles well under that limit.
int GM_CDECL gmf_addPolygon(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmVariable verticesVar = a_thread->Param(0);
    if (verticesVar.m_type != GM_TABLE) return GM_OK; // honest no-op, matches null-call leniency elsewhere
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* table = (gmTableObject*)m.GetObject(verticesVar.m_value.m_ref);
    int count = (a_thread->GetNumParams() >= 2) ? a_thread->ParamInt(1, 0) : 0;

    std::vector<b2Vec2> pts;
    for (int i = 0; i < count && (int)pts.size() < b2_maxPolygonVertices; ++i) {
        gmVariable v = table->Get(gmVariable((gmint)i));
        float x, y;
        if (GetVector2D(m, v, x, y)) pts.push_back(b2Vec2(x, y));
    }
    // b2PolygonShape::Set hard-asserts (aborts the whole process, not a
    // catchable GM exception) on a degenerate point set -- confirmed
    // crash: real level data can produce near-duplicate or collinear
    // vertices (e.g. a hairline-thin ground sliver) that Box2D's own
    // convex-hull computation collapses to fewer than 3 usable points.
    // De-duplicate near-identical consecutive points and check the
    // signed area (shoelace formula) is comfortably above zero before
    // ever handing this to Box2D, same "fail safe rather than crash"
    // reasoning as addBox/addCircle's degenerate-shape guard above.
    std::vector<b2Vec2> dedup;
    for (const b2Vec2& p : pts) {
        if (dedup.empty() || (p - dedup.back()).LengthSquared() > 1e-8f) dedup.push_back(p);
    }
    if (dedup.size() >= 2 && (dedup.front() - dedup.back()).LengthSquared() <= 1e-8f) dedup.pop_back();
    if (dedup.size() < 3) return GM_OK;

    float area2 = 0.0f;
    for (size_t i = 0; i < dedup.size(); ++i) {
        const b2Vec2& p0 = dedup[i];
        const b2Vec2& p1 = dedup[(i + 1) % dedup.size()];
        area2 += p0.x * p1.y - p1.x * p0.y;
    }
    if (std::fabs(area2) < 1e-6f) return GM_OK;

    b2PolygonShape* poly = new b2PolygonShape();
    poly->Set(dedup.data(), (int32)dedup.size());
    c->AddFixture(poly);
    return GM_OK;
}

// addCircle(radius, offsetX, offsetY) -- confirmed real method, real usage
// `sd.addCircle(this.radius,0,0)` in GoAroundGuy.gm.
int GM_CDECL gmf_addCircle(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(radius, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(offsetX, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(offsetY, 2, 0.0f);

    // Same degenerate-shape guard as addBox above -- a zero/negative
    // radius circle hits the identical Box2D ComputeMass hard-assert.
    b2CircleShape* circle = new b2CircleShape();
    circle->m_radius = std::max(radius, 0.001f);
    circle->m_p = b2Vec2(offsetX, offsetY);
    c->AddFixture(circle);
    return GM_OK;
}

// addEdges(vertices, count, loop) -- confirmed real, live call site
// SafeVine.gm:90: `sd.addEdges(this.verts,this.verts.size,false);` (builds
// the vine's thin collision line from its own procedurally-generated point
// list). Was entirely unbound -- hard "getdot failed" aborting the vine's
// constructor before `pb.appendShapes(sd)` ever ran, leaving it with zero
// collision. Same 0-indexed vertex-table convention as addPolygon above;
// maps directly to Box2D's own open/closed edge-chain primitive
// (b2ChainShape::CreateChain for an open strip, CreateLoop when the 3rd
// arg is true) rather than approximating with addPolygon, since a vine is
// a thin LINE, not a filled area -- addPolygon's convex-hull/area-based
// degenerate guards would collapse a near-zero-width shape like this to
// nothing.
int GM_CDECL gmf_addEdges(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    gmVariable verticesVar = a_thread->Param(0);
    if (verticesVar.m_type != GM_TABLE) return GM_OK; // honest no-op, matches null-call leniency elsewhere
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* table = (gmTableObject*)m.GetObject(verticesVar.m_value.m_ref);
    int count = a_thread->ParamInt(1, 0);
    bool loop = a_thread->GetNumParams() >= 3 ? (a_thread->ParamInt(2, 0) != 0) : false;

    std::vector<b2Vec2> pts;
    for (int i = 0; i < count; ++i) {
        gmVariable v = table->Get(gmVariable((gmint)i));
        float x, y;
        if (GetVector2D(m, v, x, y)) pts.push_back(b2Vec2(x, y));
    }
    if (pts.size() < 2) return GM_OK; // degenerate, matches addPolygon's "fail safe rather than crash"

    b2ChainShape* chain = new b2ChainShape();
    if (loop) {
        chain->CreateLoop(pts.data(), (int32)pts.size());
    } else {
        // This box2d version requires explicit ghost vertices for an open
        // chain; reusing the first/last real points (zero-length ghost
        // edges) is the standard degenerate-but-safe stand-in when the
        // caller has no adjacent-chain data to hand it real ones.
        chain->CreateChain(pts.data(), (int32)pts.size(), pts.front(), pts.back());
    }
    c->AddFixture(chain);
    return GM_OK;
}

// setIsSensor/setMass/setCollisionCategory/setCategoriesToCollideWith/
// setFrictionForPlayer: confirmed real call shapes from GoAroundGuy.gm
// Applied to every fixture currently in the collection -- real scripts
// always call these immediately after a single addBox/addCircle, so
// "all" and "most recent" are equivalent in every real usage seen so far.
int GM_CDECL gmf_setIsSensor(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(isSensor, 0, 0);
    for (auto& def : c->fixtureDefs) def.isSensor = (isSensor != 0);
    return GM_OK;
}

int GM_CDECL gmf_setMass(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(mass, 0, 0.0f);
    // Box2D fixtures carry density, not mass directly; density is the
    // natural mapping (Box2D derives actual body mass = density * area).
    for (auto& def : c->fixtureDefs) def.density = mass;
    return GM_OK;
}

int GM_CDECL gmf_setCollisionCategory(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(category, 0, 1);
    for (auto& def : c->fixtureDefs) def.filter.categoryBits = (uint16)category;
    return GM_OK;
}

int GM_CDECL gmf_setCategoriesToCollideWith(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(mask, 0, -1);
    for (auto& def : c->fixtureDefs) def.filter.maskBits = (uint16)mask;
    return GM_OK;
}

int GM_CDECL gmf_setFrictionForPlayer(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(friction, 0, 0.2f);
    for (auto& def : c->fixtureDefs) def.friction = friction;
    return GM_OK;
}

gmFunctionEntry s_shapeDefLib[] = {
    {"reset", gmf_reset, nullptr},
    {"addBox", gmf_addBox, nullptr},
    {"addPolygon", gmf_addPolygon, nullptr},
    {"addCircle", gmf_addCircle, nullptr},
    {"addEdges", gmf_addEdges, nullptr},
    {"setIsSensor", gmf_setIsSensor, nullptr},
    {"setMass", gmf_setMass, nullptr},
    {"setCollisionCategory", gmf_setCollisionCategory, nullptr},
    {"setCategoriesToCollideWith", gmf_setCategoriesToCollideWith, nullptr},
    {"setFrictionForPlayer", gmf_setFrictionForPlayer, nullptr},
};

void GM_CDECL ShapeDefGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (ShapeDefCollection*)a_object->m_user;
}

} // namespace

void RegisterPhysicsBindings(gmMachine& a_machine) {
    g_shapeDefType = a_machine.CreateUserType("PHYSICSSHAPECOLLECTIONDEF");
    a_machine.RegisterTypeLibrary(g_shapeDefType, s_shapeDefLib,
                                   sizeof(s_shapeDefLib) / sizeof(s_shapeDefLib[0]));
    a_machine.RegisterUserCallbacks(g_shapeDefType, nullptr, ShapeDefGCDestruct, nullptr);

    // The type's own bare name is its constructor (confirmed pattern, see
    a_machine.RegisterLibraryFunction("PHYSICSSHAPECOLLECTIONDEF", gmf_PHYSICSSHAPECOLLECTIONDEF);
}
