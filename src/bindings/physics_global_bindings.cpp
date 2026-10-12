#include "gmThread.h"
#include "physics_global_bindings.h"
#include "physics_joint_bindings.h"
#include "physics_body_bindings.h"
#include "vector2d_bindings.h"
#include "../core/physics_world.h"
#include <box2d/box2d.h>
#include <cstdint>
#include <vector>
#include <set>

namespace {

// RETAIL PPC VERIFIED: PHYSICS.flush (0x89d40) forwards to 0x36d00, which
// loads the fixed 0.01 ms delta from 0x36cf8 and invokes PhysicsManagerTick
// (0x35d28). The ordinary manager tick uses the same millisecond delta domain
// as the 16.666666 ms game tick. Keep movement-before-world-step order; the
// local world adapter converts milliseconds to seconds at the Box2D boundary.
int GM_CDECL gmf_Physics_flush(gmThread*) {
    // Retail wrapper never inspects explicit arguments, so preserve that
    // permissive call behavior.
    constexpr float kRetailFlushDeltaMs = 0.01f;
    StepPhysicsBodyMovement(kRetailFlushDeltaMs);
    StepPhysicsWorld(kRetailFlushDeltaMs);
    return GM_OK;
}

// --- isShapeSensor/getBodyFromShapeID/getEntityFromShapeID --------------
// All three confirmed real and heavily used (Player.gm's ledge-hang/
// walk-surface logic, Main.gm's raycast-vs-entity resolution). Decompiled
// the gmFunctionEntry cross-reference for these three specifically even
// though the names ARE real registered natives, unlike Video.
// destroyAllShaders which genuinely has zero occurrences anywhere in the
// binary): isShapeSensor's body is `LZCOUNT(queryFlags(sid)) >> 5 ^ 1`,
// the same "x != 0" boolean-cast idiom used elsewhere in this binary
// (DrawList.setColourMask) -- i.e. `queryFlags(sid) != 0`, which is
// exactly what b2Fixture::IsSensor() already gives us directly against
// this engine's real Box2D fixture.
int GM_CDECL gmf_Physics_isShapeSensor(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(sid, 0, 0);
    b2Fixture* fx = FindFixtureByGlobalId(sid);
    a_thread->PushInt((fx && fx->IsSensor()) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_Physics_getBodyFromShapeID(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(sid, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    b2Fixture* fx = FindFixtureByGlobalId(sid);
    if (!fx) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(FindPhysicsBodyVarForB2Body(m, fx->GetBody()));
    return GM_OK;
}
int GM_CDECL gmf_Physics_getEntityFromShapeID(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(sid, 0, 0);
    b2Fixture* fx = FindFixtureByGlobalId(sid);
    if (!fx) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(FindOwnerEntityForB2Body(fx->GetBody()));
    return GM_OK;
}

// CraigEntityHelper uses this live retail native to change individual
// fixtures between solid and sensor during state transitions.
int GM_CDECL gmf_Physics_getShapeByID(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(sid, 0, 0);
    a_thread->Push(MakePhysicsShapeByGlobalId(*a_thread->GetMachine(), sid));
    return GM_OK;
}

// RETAIL BYTECODE: Physics.setNumFramesToIgnoreCollisions @ 0x0008b860
// forwards the integer argument to FUN_00036f34, which stores it at manager
// +0x5c. FUN_00035d28 decrements positive values and skips post-step
// collision-pair traversal until the countdown reaches zero.
int GM_CDECL gmf_Physics_setNumFramesToIgnoreCollisions(gmThread* a_thread) {
    const gmint frames = a_thread->Param(0).m_value.m_int;
    SetPhysicsNumFramesToIgnoreCollisions(frames);
    return GM_OK;
}

// RETAIL BYTECODE: Physics.getNumFramesToIgnoreCollisions @ 0x0008b8bc
// reads the signed integer from the same manager field through FUN_00036f3c.
int GM_CDECL gmf_Physics_getNumFramesToIgnoreCollisions(gmThread* a_thread) {
    a_thread->PushInt(GetPhysicsNumFramesToIgnoreCollisions());
    return GM_OK;
}

// --- raycasting -----------------------------------------------------------
// Confirmed real: Physics.raycastOne (85 call sites) and Physics.raycast
// (16). Argument contract confirmed by usage, not by fully decompiling the
// ABI-confused double-heavy bodies (FUN_00089d90/FUN_0008a5e0):
//   raycastOne(startPos, endPos, considerAlreadyColliding, considerSensors,
//              resultTable, [excludeList], [categoryMask]) -> bool,
//     filling resultTable.lambda/nx/ny/sid/entity on a hit (field names
//     confirmed: Player.gm reads result.lambda, result.nx/result.ny
//     (PhysicsBody.raycast's own result -- same shape), rayResult.sid,
//     retval.entity).
//   raycast(startPos, endPos, considerAlreadyColliding, considerSensors,
//           [excludeList], [categoryMask]) -> entity iterator (confirmed:
//     TouchEditProcess.gm's `local what=it.get(); ...
//     this.target.getObjectID()==what.getObjectID()` -- what.get() is a
//     real entity table, not a shape id or result table).
// considerSensors=false (the overwhelmingly common case) skips sensor
// fixtures. considerAlreadyColliding=false skips a fixture the ray START
// point is already inside (TestPoint) -- the plain-English reading of the
// name; not decompiled bit-for-bit, but a real, sensible Box2D behavior
// rather than an invented one.
std::vector<b2Body*> ReadExcludeList(gmMachine& a_machine, const gmVariable& a_var) {
    std::vector<b2Body*> out;
    if (a_var.m_type != GM_TABLE) return out;
    gmTableObject* t = (gmTableObject*)a_machine.GetObject(a_var.m_value.m_ref);
    if (!t) return out;
    gmVariable sizeVar = t->Get(&a_machine, "size");
    int size = (sizeVar.m_type == GM_INT) ? sizeVar.m_value.m_int : 0;
    for (int i = 0; i < size; ++i) {
        b2Body* body = GetB2BodyFromVar(a_machine, t->Get(i));
        if (body) out.push_back(body);
    }
    return out;
}

struct ClosestRayCastCallback : b2RayCastCallback {
    b2Vec2 rayStart;
    bool ignoreSensors = true;
    bool ignoreAlreadyColliding = true;
    const std::vector<b2Body*>* excludeBodies = nullptr;
    uint16 categoryMask = 0;
    bool hit = false;
    b2Fixture* fixture = nullptr;
    b2Vec2 point = b2Vec2(0, 0), normal = b2Vec2(0, 0);
    float fraction = 1.0f;

    bool Skip(b2Fixture* a_fx) const {
        if (ignoreSensors && a_fx->IsSensor()) return true;
        if (ignoreAlreadyColliding && a_fx->TestPoint(rayStart)) return true;
        if (categoryMask != 0 && (a_fx->GetFilterData().categoryBits & categoryMask) == 0) return true;
        if (excludeBodies) for (b2Body* b : *excludeBodies) if (b == a_fx->GetBody()) return true;
        return false;
    }
    float ReportFixture(b2Fixture* a_fx, const b2Vec2& a_point, const b2Vec2& a_normal, float a_fraction) override {
        if (Skip(a_fx)) return -1.0f;
        hit = true; fixture = a_fx; point = a_point; normal = a_normal; fraction = a_fraction;
        return a_fraction; // clip to closest hit so far
    }
};

int GM_CDECL gmf_Physics_raycastOne(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(5);
    gmMachine& m = *a_thread->GetMachine();
    float sx, sy, ex, ey;
    if (!GetVector2D(m, a_thread->Param(0), sx, sy)) return GM_EXCEPTION;
    if (!GetVector2D(m, a_thread->Param(1), ex, ey)) return GM_EXCEPTION;
    int considerAlreadyColliding = a_thread->ParamInt(2, 0);
    int considerSensors = a_thread->ParamInt(3, 0);
    gmVariable resultVar = a_thread->Param(4);
    if (resultVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* resultTable = (gmTableObject*)m.GetObject(resultVar.m_value.m_ref);

    std::vector<b2Body*> excludeBodies;
    if (a_thread->GetNumParams() >= 6) excludeBodies = ReadExcludeList(m, a_thread->Param(5));
    // ENGINE PATCH: categoryMask read from the LAST param, not a hardcoded
    // index 6, and leniently (ParamInt's default-on-mismatch, not
    // GM_INT_PARAM's strict throw). Confirmed real, live crash
    // SwarmKiller.gm:313: `Physics.raycastOne(this.mainpb.getPos(),ppos,
    // true,false,this.raycastResult,null,null,GROUND_COLLISION_CATEGORY|
    // STRUCTURE_COLLISION_CATEGORY)` -- an 8-arg call with an extra null
    // between excludeList and categoryMask this binding's documented
    // 7-param signature doesn't account for (no decompiled evidence for
    // what that 7th positional arg is -- possibly a second exclude-style
    // list, always null in every real call site found). Reading a null
    // param 6 via the old hardcoded-index/strict-macro combination threw a
    // blank GM_EXCEPTION every single call, aborting checkAndDoAttack()
    // (and its update() callers, 3 levels up) for every SwarmKiller enemy,
    // every frame. Reading the true last argument works for both the
    // documented 7-arg shape and this real 8-arg one without needing to
    // know what the unexplained middle arg means.
    uint16 categoryMask = 0;
    if (a_thread->GetNumParams() >= 7) {
        int lastIdx = a_thread->GetNumParams() - 1;
        categoryMask = (uint16)a_thread->ParamInt(lastIdx, 0);
    }

    ClosestRayCastCallback cb;
    cb.rayStart = b2Vec2(sx, sy);
    cb.ignoreSensors = (considerSensors == 0);
    cb.ignoreAlreadyColliding = (considerAlreadyColliding == 0);
    cb.excludeBodies = &excludeBodies;
    cb.categoryMask = categoryMask;
    GetPhysicsWorld().RayCast(&cb, b2Vec2(sx, sy), b2Vec2(ex, ey));

    if (!cb.hit) { a_thread->PushInt(0); return GM_OK; }

    resultTable->Set(&m, "lambda", gmVariable(cb.fraction));
    resultTable->Set(&m, "nx", gmVariable(cb.normal.x));
    resultTable->Set(&m, "ny", gmVariable(cb.normal.y));
    resultTable->Set(&m, "sid", gmVariable((gmint)(int)cb.fixture->GetUserData().pointer));
    resultTable->Set(&m, "entity", FindOwnerEntityForB2Body(cb.fixture->GetBody()));
    a_thread->PushInt(1);
    return GM_OK;
}

// RETAIL PPC: Physics.raycastSimple @ 0x0008ac5c decodes exactly two
// vector2D arguments, calls FUN_00036a78 with both filter bytes zero, and
// pushes the helper's hit flag as a GM integer/boolean. The wrapper and
// return contract and the second flag's sensor behavior are bytecode-
// verified: the PPC callback at 0x002ff0c8 rejects sensor fixtures when
// this flag is zero. The first flag's complete filter semantics are still
// being reconciled; the desktop currently maps it to raycastOne's
// start-overlap exclusion, which remains an inference.
int GM_CDECL gmf_Physics_raycastSimple(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    float sx, sy, ex, ey;
    if (!GetVector2D(m, a_thread->Param(0), sx, sy) ||
        !GetVector2D(m, a_thread->Param(1), ex, ey)) return GM_EXCEPTION;

    ClosestRayCastCallback cb;
    cb.rayStart = b2Vec2(sx, sy);
    cb.ignoreSensors = true;
    cb.ignoreAlreadyColliding = true;
    GetPhysicsWorld().RayCast(&cb, b2Vec2(sx, sy), b2Vec2(ex, ey));
    a_thread->PushInt(cb.hit ? 1 : 0);
    return GM_OK;
}

// raycast -- all touching entities along the ray, deduplicated, as a real
// entity iterator (matches the ENTITYLISTREFCONTAINER idiom used
// elsewhere, e.g. PhysicsData.getCollidingEntities).
struct AllHitsRayCastCallback : b2RayCastCallback {
    b2Vec2 rayStart;
    bool ignoreSensors = true;
    bool ignoreAlreadyColliding = true;
    const std::vector<b2Body*>* excludeBodies = nullptr;
    uint16 categoryMask = 0;
    std::vector<gmVariable> entities;

    bool Skip(b2Fixture* a_fx) const {
        if (ignoreSensors && a_fx->IsSensor()) return true;
        if (ignoreAlreadyColliding && a_fx->TestPoint(rayStart)) return true;
        if (categoryMask != 0 && (a_fx->GetFilterData().categoryBits & categoryMask) == 0) return true;
        if (excludeBodies) for (b2Body* b : *excludeBodies) if (b == a_fx->GetBody()) return true;
        return false;
    }
    float ReportFixture(b2Fixture* a_fx, const b2Vec2&, const b2Vec2&, float) override {
        if (!Skip(a_fx)) {
            gmVariable e = FindOwnerEntityForB2Body(a_fx->GetBody());
            if (e.m_type == GM_TABLE) {
                bool already = false;
                for (gmVariable& v : entities) if (v.m_value.m_ref == e.m_value.m_ref) { already = true; break; }
                if (!already) entities.push_back(e);
            }
        }
        return 1.0f; // don't clip -- keep going, collect every hit
    }
};

struct ShapeAABBQueryCallback : b2QueryCallback {
    const b2Shape* exactQueryShape = nullptr;
    b2Transform exactQueryTransform;
    std::vector<int> shapeIds;
    std::set<int> seen;

    bool ReportFixture(b2Fixture* fixture) override {
        const int shapeId = (int)(intptr_t)fixture->GetUserData().pointer;
        if (shapeId <= 0 || seen.count(shapeId)) return true;
        if (exactQueryShape) {
            const b2Shape* fixtureShape = fixture->GetShape();
            bool overlaps = false;
            for (int child = 0; child < fixtureShape->GetChildCount(); ++child) {
                if (b2TestOverlap(exactQueryShape, 0, fixtureShape, child,
                                  exactQueryTransform,
                                  fixture->GetBody()->GetTransform())) {
                    overlaps = true;
                    break;
                }
            }
            if (!overlaps) return true;
        }
        seen.insert(shapeId);
        shapeIds.push_back(shapeId);
        return true;
    }
};

int PushShapeAABBQuery(gmThread* a_thread, const b2AABB& bounds,
                       const b2Shape* exactQueryShape = nullptr,
                       const b2Transform* exactQueryTransform = nullptr) {
    if (!bounds.IsValid()) {
        a_thread->Push(MakeIntListRefContainer(
            *a_thread->GetMachine(), std::vector<int>{}));
        return GM_OK;
    }
    ShapeAABBQueryCallback callback;
    callback.exactQueryShape = exactQueryShape;
    if (exactQueryTransform) callback.exactQueryTransform = *exactQueryTransform;
    GetPhysicsWorld().QueryAABB(&callback, bounds);
    a_thread->Push(MakeIntListRefContainer(
        *a_thread->GetMachine(), std::move(callback.shapeIds)));
    return GM_OK;
}

int PushEmptyShapeQuery(gmThread* a_thread) {
    a_thread->Push(MakeIntListRefContainer(
        *a_thread->GetMachine(), std::vector<int>{}));
    return GM_OK;
}

int GM_CDECL gmf_Physics_getShapesWithin(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(4);
    GM_FLOAT_OR_INT_PARAM(centerX, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(centerY, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(halfWidth, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(halfHeight, 3, 0.0f);

    // The active TouchEditProcess/Main call sites pass an area center plus
    // half extents. FUN_00036b68 constructs a rectangle around that center;
    // its rotation constant DAT_00036b60 is zero in the retail binary.
    if (halfWidth < 0.0f || halfHeight < 0.0f) return PushEmptyShapeQuery(a_thread);
    b2PolygonShape queryShape;
    queryShape.SetAsBox(halfWidth, halfHeight);
    b2Transform queryTransform;
    queryTransform.Set(b2Vec2(centerX, centerY), 0.0f);
    b2AABB bounds;
    queryShape.ComputeAABB(&bounds, queryTransform, 0);
    return PushShapeAABBQuery(a_thread, bounds, &queryShape, &queryTransform);
}

int GM_CDECL gmf_Physics_getShapesNear(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(4);
    GM_FLOAT_OR_INT_PARAM(centerX, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(centerY, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(halfWidth, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(halfHeight, 3, 0.0f);
    if (halfWidth < 0.0f || halfHeight < 0.0f) return PushEmptyShapeQuery(a_thread);
    b2AABB bounds;
    bounds.lowerBound.Set(centerX - halfWidth, centerY - halfHeight);
    bounds.upperBound.Set(centerX + halfWidth, centerY + halfHeight);
    return PushShapeAABBQuery(a_thread, bounds);
}

int GM_CDECL gmf_Physics_getShapesRoughlyWithin(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(4);
    GM_FLOAT_OR_INT_PARAM(minX, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(minY, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(maxX, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(maxY, 3, 0.0f);
    if (minX > maxX || minY > maxY) return GM_EXCEPTION;
    b2AABB bounds;
    bounds.lowerBound.Set(minX, minY);
    bounds.upperBound.Set(maxX, maxY);
    return PushShapeAABBQuery(a_thread, bounds);
}

gmType g_rayIterType = -1;
struct RayEntityIterData {
    std::vector<gmVariable> entities;
    size_t index = 0;
};
int GM_CDECL gmf_rayIter_isValid(gmThread* a_thread) {
    RayEntityIterData* it = (RayEntityIterData*)a_thread->ThisUserCheckType(g_rayIterType);
    a_thread->PushInt((it && it->index < it->entities.size()) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_rayIter_goToNext(gmThread* a_thread) {
    RayEntityIterData* it = (RayEntityIterData*)a_thread->ThisUserCheckType(g_rayIterType);
    if (it && it->index < it->entities.size()) ++it->index;
    return GM_OK;
}
int GM_CDECL gmf_rayIter_get(gmThread* a_thread) {
    RayEntityIterData* it = (RayEntityIterData*)a_thread->ThisUserCheckType(g_rayIterType);
    if (it && it->index < it->entities.size()) a_thread->Push(it->entities[it->index]);
    else a_thread->PushNull();
    return GM_OK;
}
gmFunctionEntry s_rayIterLib[] = {
    {"isValid", gmf_rayIter_isValid, nullptr},
    {"goToNext", gmf_rayIter_goToNext, nullptr},
    {"get", gmf_rayIter_get, nullptr},
};

int GM_CDECL gmf_Physics_raycast(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(4);
    gmMachine& m = *a_thread->GetMachine();
    float sx, sy, ex, ey;
    if (!GetVector2D(m, a_thread->Param(0), sx, sy)) return GM_EXCEPTION;
    if (!GetVector2D(m, a_thread->Param(1), ex, ey)) return GM_EXCEPTION;
    GM_INT_PARAM(considerAlreadyColliding, 2, 0);
    GM_INT_PARAM(considerSensors, 3, 0);

    std::vector<b2Body*> excludeBodies;
    if (a_thread->GetNumParams() >= 5) excludeBodies = ReadExcludeList(m, a_thread->Param(4));
    uint16 categoryMask = 0;
    if (a_thread->GetNumParams() >= 6) { GM_INT_PARAM(cc, 5, 0); categoryMask = (uint16)cc; }

    AllHitsRayCastCallback cb;
    cb.rayStart = b2Vec2(sx, sy);
    cb.ignoreSensors = (considerSensors == 0);
    cb.ignoreAlreadyColliding = (considerAlreadyColliding == 0);
    cb.excludeBodies = &excludeBodies;
    cb.categoryMask = categoryMask;
    GetPhysicsWorld().RayCast(&cb, b2Vec2(sx, sy), b2Vec2(ex, ey));

    RayEntityIterData* it = new RayEntityIterData{std::move(cb.entities), 0};
    gmUserObject* obj = m.AllocUserObject(it, g_rayIterType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}

void GM_CDECL RayIterGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (RayEntityIterData*)a_object->m_user;
}

gmFunctionEntry s_physicsLib[] = {
    {"flush", gmf_Physics_flush, nullptr},
    {"isShapeSensor", gmf_Physics_isShapeSensor, nullptr},
    {"getShapeByID", gmf_Physics_getShapeByID, nullptr},
    {"getBodyFromShapeID", gmf_Physics_getBodyFromShapeID, nullptr},
    {"getEntityFromShapeID", gmf_Physics_getEntityFromShapeID, nullptr},
    {"setNumFramesToIgnoreCollisions", gmf_Physics_setNumFramesToIgnoreCollisions, nullptr},
    {"getNumFramesToIgnoreCollisions", gmf_Physics_getNumFramesToIgnoreCollisions, nullptr},
    {"raycastOne", gmf_Physics_raycastOne, nullptr},
    {"raycastSimple", gmf_Physics_raycastSimple, nullptr},
    {"raycast", gmf_Physics_raycast, nullptr},
    // Retail's native Physics table calls the all-hits iterator function
    // `raycastShapes` (Physics.raycastShapes @ 0x0008a3a4). The game_data
    // scripts in this checkout call it `raycast`; keep that compatibility
    // name while also exposing the retail native entry point.
    {"raycastShapes", gmf_Physics_raycast, nullptr},
    {"getShapesNear", gmf_Physics_getShapesNear, nullptr},
    {"getShapesRoughlyWithin", gmf_Physics_getShapesRoughlyWithin, nullptr},
    {"getShapesWithin", gmf_Physics_getShapesWithin, nullptr},
};

} // namespace

void RegisterPhysicsGlobalBindings(gmMachine& a_machine) {
    RegisterPhysicsJointBindings(a_machine);
    g_rayIterType = a_machine.CreateUserType("RayHitEntityIterator");
    a_machine.RegisterTypeLibrary(g_rayIterType, s_rayIterLib, sizeof(s_rayIterLib) / sizeof(s_rayIterLib[0]));
    a_machine.RegisterUserCallbacks(g_rayIterType, nullptr, RayIterGCDestruct, nullptr);

    // RegisterPhysicsJointBindings created the retail Physics global above.
    // RegisterLibrary defaults to replacing the named table, which discarded
    // createJoint/deleteJoint/getJoint and left live EntityBase.gm calls
    // unbound. Extend that table with the global query helpers instead.
    a_machine.RegisterLibrary(s_physicsLib, sizeof(s_physicsLib) / sizeof(s_physicsLib[0]), "Physics", false);
}
