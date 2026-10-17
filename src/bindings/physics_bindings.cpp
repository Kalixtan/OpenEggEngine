#include "gmThread.h"
#include "physics_bindings.h"
#include "vector2d_bindings.h"
#include "gmTableObject.h"
#include "../core/retail_polygon_geometry.h"
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

int GM_CDECL gmf_addBox(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(width, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(height, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(offsetX, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(offsetY, 3, 0.0f);
    GM_FLOAT_OR_INT_PARAM(angle, 4, 0.0f);

    constexpr float kMinBoxExtent = 0.001f;
    float hx = std::max(width * 0.5f, kMinBoxExtent);
    float hy = std::max(height * 0.5f, kMinBoxExtent);
    b2PolygonShape* box = new b2PolygonShape();
    box->SetAsBox(hx, hy, b2Vec2(offsetX, offsetY), angle);
    c->AddFixture(box);
    return GM_OK;
}

int GM_CDECL gmf_addPolygon(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmVariable verticesVar = a_thread->Param(0);
    if (verticesVar.m_type != GM_TABLE) return GM_OK; // honest no-op, matches null-call leniency elsewhere
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* table = (gmTableObject*)m.GetObject(verticesVar.m_value.m_ref);
    if (a_thread->GetNumParams() < 2) return GM_EXCEPTION;
    const int count = a_thread->ParamInt(1, 0);
    if (count < 1 || count > b2_maxPolygonVertices) return GM_EXCEPTION;

    std::vector<b2Vec2> pts;
    pts.reserve((size_t)count);
    for (int i = 0; i < count; ++i) {
        gmVariable v = table->Get(gmVariable((gmint)i));
        float x, y;
        if (!GetVector2D(m, v, x, y)) return GM_EXCEPTION;
        pts.push_back(b2Vec2(x, y));
    }
    b2PolygonShape* poly = new b2PolygonShape();
    SetRetailPolygonGeometry(poly, pts);
    c->AddFixture(poly);
    return GM_OK;
}

int GM_CDECL gmf_addCircle(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    if (a_thread->GetNumParams() == 2) return GM_EXCEPTION;
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
    if (pts.size() < (loop ? 3u : 2u)) return GM_OK;

    if (loop) {
        float area2 = 0.0f;
        for (size_t i = 0; i < pts.size(); ++i) {
            const b2Vec2& p0 = pts[i];
            const b2Vec2& p1 = pts[(i + 1) % pts.size()];
            area2 += p0.x * p1.y - p1.x * p0.y;
        }
        if (area2 < 0.0f) std::reverse(pts.begin(), pts.end());
        for (size_t i = 0; i < pts.size(); ++i) {
            const b2Vec2& first = pts[i];
            const b2Vec2& second = pts[(i + 1) % pts.size()];
            b2EdgeShape* edge = new b2EdgeShape();
            edge->SetOneSided(first, first, second, second);
            c->AddFixture(edge);
        }
    } else {
        std::vector<b2Vec2> retailPath = pts;
        for (int i = (int)pts.size() - 1; i >= 1; --i)
            retailPath.push_back(pts[(size_t)i]);
        for (size_t i = 0; i + 1 < retailPath.size(); ++i) {
            const b2Vec2& first = retailPath[i];
            const b2Vec2& second = retailPath[i + 1];
            // Keep the zero-length turn edge too: retail constructs one
            // shape for every adjacent pair, and omitting it would shift all
            // later global shape IDs. Box2D accepts its zero-area edge shape.
            b2EdgeShape* edge = new b2EdgeShape();
            edge->SetOneSided(first, first, second, second);
            c->AddFixture(edge);
        }
    }
    return GM_OK;
}

// These setters are registered in the retail shape-definition method table.
// Retail wrappers dereference the collection object at this GM user
// value's +0x14; they do not select an individual geometry entry.
int GM_CDECL gmf_setIsSensor(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(isSensor, 0, 0);
    c->isSensor = (isSensor != 0);
    for (auto& def : c->fixtureDefs) def.isSensor = c->isSensor;
    return GM_OK;
}

int GM_CDECL gmf_isSensor(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushInt(c->isSensor);
    return GM_OK;
}

int GM_CDECL gmf_setMass(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(mass, 0, 0.0f);
    // Box2D fixtures carry density, not mass directly; density is the
    // natural mapping (Box2D derives actual body mass = density * area).
    c->density = mass;
    for (auto& def : c->fixtureDefs) def.density = mass;
    return GM_OK;
}

int GM_CDECL gmf_getMass(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushFloat(0.0f);
    return GM_OK;
}

int GM_CDECL gmf_setCollisionCategory(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(category, 0, 1);
    c->category = (uint16)category;
    for (auto& def : c->fixtureDefs) def.filter.categoryBits = c->category;
    return GM_OK;
}

int GM_CDECL gmf_setCategoriesToCollideWith(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(mask, 0, -1);
    c->mask = (uint16)mask;
    for (auto& def : c->fixtureDefs) def.filter.maskBits = c->mask;
    return GM_OK;
}

int GM_CDECL gmf_getCollisionCategory(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushInt(c->category);
    return GM_OK;
}

int GM_CDECL gmf_getCategoriesToCollideWith(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushInt(c->mask);
    return GM_OK;
}

int GM_CDECL gmf_setFrictionForPlayer(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(friction, 0, 0.2f);
    c->playerFriction = friction;
    return GM_OK;
}

int GM_CDECL gmf_getFriction(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushFloat(c->friction);
    return GM_OK;
}

// Fixture friction is part of the shape definition before appendShapes()
// creates the Box2D fixtures. The shipped SingleCircleGround and
// PlayerDeathBits scripts set it on this collection, not on the body.
int GM_CDECL gmf_setFriction(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(friction, 0, 0.2f);
    c->friction = friction;
    for (auto& def : c->fixtureDefs) def.friction = friction;
    return GM_OK;
}

int GM_CDECL gmf_getRestitution(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushFloat(c->restitution);
    return GM_OK;
}

int GM_CDECL gmf_setRestitution(gmThread* a_thread) {
    ShapeDefCollection* c = ThisCollection(a_thread);
    if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(restitution, 0, 0.0f);
    c->restitution = restitution;
    for (auto& def : c->fixtureDefs) def.restitution = restitution;
    return GM_OK;
}

gmFunctionEntry s_shapeDefLib[] = {
    {"reset", gmf_reset, nullptr},
    {"addBox", gmf_addBox, nullptr},
    {"addPolygon", gmf_addPolygon, nullptr},
    {"addCircle", gmf_addCircle, nullptr},
    {"addEdges", gmf_addEdges, nullptr},
    {"setIsSensor", gmf_setIsSensor, nullptr},
    {"isSensor", gmf_isSensor, nullptr},
    {"setMass", gmf_setMass, nullptr},
    {"getMass", gmf_getMass, nullptr},
    {"setCollisionCategory", gmf_setCollisionCategory, nullptr},
    {"setCategoriesToCollideWith", gmf_setCategoriesToCollideWith, nullptr},
    {"getCollisionCategory", gmf_getCollisionCategory, nullptr},
    {"getCategoriesToCollideWith", gmf_getCategoriesToCollideWith, nullptr},
    {"setFrictionForPlayer", gmf_setFrictionForPlayer, nullptr},
    {"setFriction", gmf_setFriction, nullptr},
    {"getFriction", gmf_getFriction, nullptr},
    {"setRestitution", gmf_setRestitution, nullptr},
    {"getRestitution", gmf_getRestitution, nullptr},
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

    a_machine.RegisterLibraryFunction("PHYSICSSHAPECOLLECTIONDEF", gmf_PHYSICSSHAPECOLLECTIONDEF);
}
