#include "gmThread.h"
#include "gmStringObject.h"
#include "physics_body_bindings.h"
#include "physics_joint_bindings.h"
#include "entity_bindings.h"
#include <algorithm>
#include <cstdio>
#include "physics_bindings.h"
#include "vector2d_bindings.h"
#include "../core/physics_world.h"
#include "../core/engine_context.h"
#include "../core/retail_polygon_geometry.h"
#include "../core/retail_object_id.h"
#include "../core/strict_calls.h"
#include "../core/process_manager.h" // RootGmVariable/UnrootGmVariable
#include "../ported_native/ground_math.h"
#include <box2d/box2d.h>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

gmType g_physicsBodyType = -1;
gmType g_physicsDataType = -1;
gmType g_physicsShapeType = -1;
gmType g_collisionInfoType = -1;
// GM's incremental collector destroys user objects in arbitrary order during
// machine teardown. Keep shared-root bookkeeping outside CollisionInfoData so
// a ShapeCollisionInfo destructor never touches an already-destroyed parent.
std::unordered_map<gmMachine*, std::unordered_map<int, size_t>>
    g_collisionInfoRootLeases;

// Game scripts express velocity and angular velocity per millisecond. Stock
// Box2D expresses both per second, so keep the unit conversion at this
// boundary and leave all script-visible values in their original units.
constexpr float kPhysicsSecondsPerMillisecond = 0.001f;
constexpr float kPhysicsMillisecondsPerSecond = 1000.0f;
inline b2Vec2 ScriptVelocity(const b2Vec2& a_velocity) {
    return kPhysicsSecondsPerMillisecond * a_velocity;
}
inline b2Vec2 Box2DVelocity(const b2Vec2& a_velocity) {
    return kPhysicsMillisecondsPerSecond * a_velocity;
}
inline float ScriptAngularVelocity(float a_velocity) {
    return a_velocity * kPhysicsSecondsPerMillisecond;
}
inline float Box2DAngularVelocity(float a_velocity) {
    return a_velocity * kPhysicsMillisecondsPerSecond;
}

// Opt-in trace for the Sparkle Darkle ground-walking creatures. This lets a
// runtime capture distinguish script velocity writes from the following
// Box2D step for both GoombaCritterGuitar3 and SlimeEyes.
bool GetCreatureDebugEntityId(gmMachine& a_machine, b2Body* a_body, int& a_id) {
    const gmVariable owner = FindOwnerEntityForB2Body(a_body);
    if (owner.m_type != GM_TABLE) return false;
    auto* table = (gmTableObject*)a_machine.GetObject(owner.m_value.m_ref);
    if (!table) return false;
    const gmVariable type = table->Get(&a_machine, "__typeName");
    if (type.m_type != GM_STRING || !type.m_value.m_ref) return false;
    auto* name = (gmStringObject*)a_machine.GetObject(type.m_value.m_ref);
    if (!name || (std::strcmp(name->GetString(),
            "Platformer_EntityPacks_GoombaCritterGuitar3") != 0 &&
        std::strcmp(name->GetString(),
            "Platformer_EntityPacks_SlimeEyes") != 0)) return false;
    const gmVariable objectId = table->Get(&a_machine, "__objectID");
    a_id = objectId.m_type == GM_INT ? objectId.m_value.m_int : -1;
    return true;
}

bool CreatureDebugEnabled() {
    static const bool enabled = std::getenv("GE3_CREATURE_DEBUG") != nullptr;
    return enabled;
}

bool ReadVec2Args(gmThread* a_thread, gmMachine& a_machine, float& a_x, float& a_y) {
    if (a_thread->GetNumParams() >= 2) {
        a_x = a_thread->ParamFloatOrInt(0, 0.0f);
        a_y = a_thread->ParamFloatOrInt(1, 0.0f);
        return true;
    }
    if (a_thread->GetNumParams() == 1) {
        return GetVector2D(a_machine, a_thread->Param(0), a_x, a_y);
    }
    return false;
}

// --- PhysicsBody ("pb") ------------------------------------------------

struct PhysicsBodyData {
    b2Body* body = nullptr;
    bool isSensor = false;
    bool isCollidable = true;
    bool geometryDirty = false;
    // A non-collidable body must not keep generating sensor overlap events.
    // Keep each fixture's authored filter while masking it out, then restore
    // it when the body is claimed/enabled again.
    std::unordered_map<b2Fixture*, b2Filter> savedCollidableFilters;
    // The gmVariable (PhysicsBody user object) that wraps THIS struct --
    // set right after allocation in MakePhysicsBody. Needed so native code
    // that only has a b2Body* (e.g. a raycast hit, or Physics.
    // getBodyFromShapeID) can hand the ORIGINAL script-visible PhysicsBody
    // handle back to a script, not just act on the raw Box2D body.
    gmVariable selfVar = gmVariable::s_null;
    int objectId = 0;
    float maxSpeed = 9999999.0f;
    // Values after the retail exp(-rate * 16.666666), clamped to [0,1]
    // transform.
    float velocityDecayRate = 1.0f;
    // maxRotateSpeed +0x44 = 9,999,999 in the retail constructor.
    float maxRotateSpeed = 9999999.0f;
    float rotationDecayRate = 1.0f;
    // rotateAcc +0x64 = 999 in the retail constructor.
    float rotateAcc = 999.0f;
    float thrustAcc = 999.0f;
    float brakeAcc = 999.0f;
    float maxVelocityDecayRate = 0.0f;
    float maxRotationDecayRate = 0.0f;
    float thrustAccum = 0.0f;
    float rotAccum = 0.0f;
    // Retail keeps a separate rotate velocity on the shape while its
    // underlying body has rotation response disabled.
    float fixedRotationVel = 0.0f;
    bool thrustFlag = false;
    bool reverseFlag = false;
    bool brakeFlag = false;
    bool rotLeftFlag = false;
    bool rotRightFlag = false;
    bool rotateToActive = false;
    float rotateToX = 0.0f;
    float rotateToY = 0.0f;
    int rotateToSign = 0;
    // Retail's setDir/getDir use body angle when rotation is enabled, and
    // this local normalized vector when fixed rotation is enabled.
    float dirX = 1.0f, dirY = 0.0f;
};

void SetBodySleeping(PhysicsBodyData* a_body, bool a_sleeping) {
    if (!a_body || !a_body->body) return;
    if (a_sleeping) a_body->fixedRotationVel = 0.0f;
    a_body->body->SetAwake(!a_sleeping);
}

namespace {

b2Filter DesiredFixtureFilter(const PhysicsBodyData* body, b2Fixture* fixture) {
    auto it = body->savedCollidableFilters.find(fixture);
    return it != body->savedCollidableFilters.end() ? it->second : fixture->GetFilterData();
}

void ApplyDesiredFixtureFilter(PhysicsBodyData* body, b2Fixture* fixture,
                               const b2Filter& desired) {
    if (!body->isCollidable) {
        body->savedCollidableFilters[fixture] = desired;
        b2Filter disabled = desired;
        disabled.maskBits = 0;
        fixture->SetFilterData(disabled);
    } else {
        body->savedCollidableFilters.erase(fixture);
        fixture->SetFilterData(desired);
    }
}

void SetBodyCollidable(PhysicsBodyData* body, bool collidable) {
    if (body->isCollidable == collidable) return;
    if (!collidable) {
        for (b2Fixture* fixture = body->body->GetFixtureList(); fixture;
             fixture = fixture->GetNext()) {
            const b2Filter desired = DesiredFixtureFilter(body, fixture);
            body->savedCollidableFilters[fixture] = desired;
            b2Filter disabled = desired;
            disabled.maskBits = 0;
            fixture->SetFilterData(disabled);
        }
        body->isCollidable = false;
    } else {
        body->isCollidable = true;
        for (b2Fixture* fixture = body->body->GetFixtureList(); fixture;
             fixture = fixture->GetNext()) {
            auto it = body->savedCollidableFilters.find(fixture);
            if (it == body->savedCollidableFilters.end()) continue;
            const b2Filter desired = it->second;
            body->savedCollidableFilters.erase(it);
            fixture->SetFilterData(desired);
        }
    }
}

void TrackNewFixture(PhysicsBodyData* body, b2Fixture* fixture) {
    if (body->isCollidable) return;
    const b2Filter desired = fixture->GetFilterData();
    body->savedCollidableFilters[fixture] = desired;
    b2Filter disabled = desired;
    disabled.maskBits = 0;
    fixture->SetFilterData(disabled);
}

} // namespace

// The raw wrapper can outlive its Box2D body: scripts retain body variables
// while an entity tears down and rebuilds its PhysicsData.  Native entry
// points must use ThisBody(), which refuses such detached wrappers; the one
// lifecycle query that deliberately accepts them (isSleeping) uses Raw.
PhysicsBodyData* ThisBodyRaw(gmThread* a_thread) {
    return (PhysicsBodyData*)a_thread->ThisUserCheckType(g_physicsBodyType);
}
PhysicsBodyData* ThisBody(gmThread* a_thread) {
    PhysicsBodyData* body = ThisBodyRaw(a_thread);
    return body && body->body ? body : nullptr;
}
b2Fixture* FixtureAtShapeIndex(b2Body* a_body, int a_index);

b2Vec2 BodyFacingDirection(const PhysicsBodyData& a_body) {
    if (a_body.body->IsFixedRotation()) return b2Vec2(a_body.dirX, a_body.dirY);
    const float angle = a_body.body->GetAngle();
    return b2Vec2(GroundRenderMath::RetailGroundSinCos(angle, true),
                  GroundRenderMath::RetailGroundSinCos(angle, false));
}

// Retail createBody creates dynamic bodies and has no body-type setter.
// Ground fixtures use zero density, which gives Box2D a dynamic body with
// zero inverse mass/inertia. Keep that body type: converting it to static
// changes the body model exposed by the native physics API and makes runtime
// density changes take a different Box2D path.
void SyncBodyTypeFromMass(b2Body* a_body) {
    if (a_body && a_body->GetType() == b2_dynamicBody) a_body->ResetMassData();
}

std::vector<gmVariable> g_bodiesByObjectId; // index 0 reserved/invalid
// Every live PhysicsBodyData, for StepPhysicsBodyMovement to walk once per
// tick and for liveness checks. Parallel to g_bodiesByObjectId (which only
// holds the script-visible gmVariable handles) because the integrator needs
// the raw struct.
std::vector<PhysicsBodyData*> g_allPhysicsBodies;
struct PhysicsDataObj;
std::vector<PhysicsDataObj*> g_allPhysicsData;
std::vector<PhysicsDataObj*> g_orderedPhysicsMovementData;
bool g_physicsMovementMembershipDirty = true;
std::vector<b2Body*> g_deferredBodyDestructions;
std::vector<std::pair<b2Body*, b2Fixture*>> g_deferredFixtureDestructions;

gmVariable MakePhysicsBody(gmMachine& a_machine, float a_x = 0.0f, float a_y = 0.0f,
                            bool a_fixedRotation = false, bool a_awake = true) {
    PhysicsBodyData* data = new PhysicsBodyData();
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.fixedRotation = a_fixedRotation;
    def.position.Set(a_x, a_y);
    def.awake = a_awake;
    data->body = GetPhysicsWorld().CreateBody(&def);
    gmUserObject* obj = a_machine.AllocUserObject(data, g_physicsBodyType);
    data->selfVar = gmVariable(obj);
    if (g_bodiesByObjectId.empty()) g_bodiesByObjectId.push_back(gmVariable::s_null); // reserve id 0
    data->objectId = static_cast<int>(AllocateRetailObjectId());
    if (g_bodiesByObjectId.size() <= static_cast<size_t>(data->objectId))
        g_bodiesByObjectId.resize(static_cast<size_t>(data->objectId) + 1, gmVariable::s_null);
    g_bodiesByObjectId[static_cast<size_t>(data->objectId)] = data->selfVar;
    g_allPhysicsBodies.push_back(data);
    return data->selfVar;
}

int GM_CDECL gmf_PHYSICSBODY_GetBodyByID(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(id, 0, 0);
    if (id > 0 && id < (int)g_bodiesByObjectId.size() &&
        g_bodiesByObjectId[(size_t)id].m_type != GM_NULL) {
        a_thread->Push(g_bodiesByObjectId[(size_t)id]);
    } else {
        a_thread->PushNull();
    }
    return GM_OK;
}

std::vector<b2Fixture*> g_shapesByGlobalId; // index 0 reserved/invalid
std::vector<float> g_shapePlayerFrictions;
int RegisterShapeGlobalId(b2Fixture* a_fixture, float a_playerFriction = 0.3f) {
    if (g_shapesByGlobalId.empty()) {
        g_shapesByGlobalId.push_back(nullptr); // reserve id 0
        g_shapePlayerFrictions.push_back(0.3f);
    }
    int id = static_cast<int>(AllocateRetailObjectId());
    if (g_shapesByGlobalId.size() <= static_cast<size_t>(id)) {
        g_shapesByGlobalId.resize(static_cast<size_t>(id) + 1, nullptr);
        g_shapePlayerFrictions.resize(static_cast<size_t>(id) + 1, 0.3f);
    }
    g_shapesByGlobalId[static_cast<size_t>(id)] = a_fixture;
    g_shapePlayerFrictions[static_cast<size_t>(id)] = a_playerFriction;
    a_fixture->GetUserData().pointer = (uintptr_t)id;
    return id;
}
b2Fixture* GetFixtureByGlobalId(int a_id) {
    if (a_id <= 0 || a_id >= (int)g_shapesByGlobalId.size()) return nullptr;
    return g_shapesByGlobalId[a_id];
}

// Store the stable ID rather than a Box2D pointer: scripts can retain a
// shape after its body is removed by a checkpoint or state change.
struct PhysicsShapeHandle { int id; };
b2Fixture* ThisShape(gmThread* a_thread) {
    auto* handle = (PhysicsShapeHandle*)a_thread->ThisUserCheckType(g_physicsShapeType);
    return handle ? GetFixtureByGlobalId(handle->id) : nullptr;
}
PhysicsBodyData* PhysicsBodyForShapeFixture(b2Fixture* fixture) {
    if (!fixture) return nullptr;
    b2Body* owner = fixture->GetBody();
    for (PhysicsBodyData* body : g_allPhysicsBodies)
        if (body && body->body == owner) return body;
    return nullptr;
}

int RetailShapeVertexCount(const b2Shape* shape) {
    if (!shape) return 0;
    switch (shape->GetType()) {
        case b2Shape::e_circle: return 1;
        case b2Shape::e_edge: return 2;
        case b2Shape::e_polygon:
            return static_cast<const b2PolygonShape*>(shape)->m_count;
        case b2Shape::e_chain:
            return static_cast<const b2ChainShape*>(shape)->m_count;
        default: return 0;
    }
}

bool RetailShapeLocalVertex(const b2Shape* shape, int index, b2Vec2& vertex) {
    if (!shape || index < 0 || index >= RetailShapeVertexCount(shape)) return false;
    switch (shape->GetType()) {
        case b2Shape::e_circle:
            vertex = static_cast<const b2CircleShape*>(shape)->m_p;
            return index == 0;
        case b2Shape::e_edge: {
            const auto* edge = static_cast<const b2EdgeShape*>(shape);
            vertex = index == 0 ? edge->m_vertex1 : edge->m_vertex2;
            return true;
        }
        case b2Shape::e_polygon:
            vertex = static_cast<const b2PolygonShape*>(shape)->m_vertices[index];
            return true;
        case b2Shape::e_chain:
            vertex = static_cast<const b2ChainShape*>(shape)->m_vertices[index];
            return true;
        default: return false;
    }
}

void SetShapeFixtureFilter(b2Fixture* fixture, const b2Filter& desired) {
    if (!fixture) return;
    PhysicsBodyData* body = PhysicsBodyForShapeFixture(fixture);
    const b2Filter current = DesiredFixtureFilter(body, fixture);
    if (current.categoryBits == desired.categoryBits &&
        current.maskBits == desired.maskBits &&
        current.groupIndex == desired.groupIndex) return;
    ApplyDesiredFixtureFilter(body, fixture, desired);
}

// RETAIL PPC VERIFIED: PHYSICSSHAPE.getType (0x8d854) returns "circle" for
// native kind 0; for kind 1 it returns "edge" when getVertexCount is 2 and
// "polygon" otherwise. Unknown kinds return "unknown". Ownership is from
// the PHYSICSSHAPE method table at 0x421988.
int GM_CDECL gmf_shape_getType(gmThread* a_thread) {
    b2Fixture* fixture = ThisShape(a_thread);
    if (!fixture) { a_thread->PushNewString("unknown"); return GM_OK; }
    const b2Shape* shape = fixture->GetShape();
    const char* type = "unknown";
    if (shape->GetType() == b2Shape::e_circle) type = "circle";
    else if (shape->GetType() == b2Shape::e_edge ||
             (shape->GetType() == b2Shape::e_chain && RetailShapeVertexCount(shape) == 2))
        type = "edge";
    else if (shape->GetType() == b2Shape::e_polygon || shape->GetType() == b2Shape::e_chain)
        type = "polygon";
    a_thread->PushNewString(type);
    return GM_OK;
}

// RETAIL PPC VERIFIED: PHYSICSSHAPE.getVertexCount (0x8d4dc) dispatches the
// selected native shape's virtual vertex-count method and returns its value.
// Desktop counts the same local geometry vertices for each Box2D shape kind.
int GM_CDECL gmf_shape_getVertexCount(gmThread* a_thread) {
    b2Fixture* fixture = ThisShape(a_thread);
    a_thread->PushInt(fixture ? RetailShapeVertexCount(fixture->GetShape()) : 0);
    return GM_OK;
}

// RETAIL PPC VERIFIED: PHYSICSSHAPE.getVertexWorldPos (0x8d6a8) range-checks
// the requested vertex and transforms its local coordinate by the owning
// body's transform; an invalid index returns the native error status.
int GM_CDECL gmf_shape_getVertexWorldPos(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    b2Fixture* fixture = ThisShape(a_thread);
    if (!fixture) return GM_EXCEPTION;
    GM_INT_PARAM(index, 0, 0);
    b2Vec2 local;
    if (!RetailShapeLocalVertex(fixture->GetShape(), index, local)) return GM_EXCEPTION;
    const b2Vec2 world = fixture->GetBody()->GetWorldPoint(local);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), world.x, world.y));
    return GM_OK;
}

// RETAIL PPC VERIFIED: PHYSICSSHAPE.setCategoriesToCollideWith (0x8d984)
// stores a 16-bit mask at native shape +0x0e. Helpers 0x15511c/0x1573d0 copy
// the ShapeDef filter record and flag matching retail contacts with bit 0x40.
// Box2D SetFilterData flags contacts for filtering and touches their proxies;
// exact consumer/timing equivalence to retail bit 0x40 remains open.
int GM_CDECL gmf_shape_setCategoriesToCollideWith(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(mask, 0, 0);
    b2Fixture* fixture = ThisShape(a_thread);
    if (!fixture) return GM_OK;
    PhysicsBodyData* body = PhysicsBodyForShapeFixture(fixture);
    b2Filter filter = DesiredFixtureFilter(body, fixture);
    filter.maskBits = static_cast<uint16>(mask);
    SetShapeFixtureFilter(fixture, filter);
    return GM_OK;
}

// RETAIL PPC VERIFIED: PHYSICSSHAPE.setCollisionCategory (0x8d9e0) stores a
// 16-bit category at native shape +0x0c. Helpers 0x155184/0x1573d0 copy the
// ShapeDef filter record and flag matching retail contacts with bit 0x40.
// Box2D SetFilterData flags contacts for filtering and touches their proxies;
// exact consumer/timing equivalence to retail bit 0x40 remains open.
int GM_CDECL gmf_shape_setCollisionCategory(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(category, 0, 0);
    b2Fixture* fixture = ThisShape(a_thread);
    if (!fixture) return GM_OK;
    PhysicsBodyData* body = PhysicsBodyForShapeFixture(fixture);
    b2Filter filter = DesiredFixtureFilter(body, fixture);
    filter.categoryBits = static_cast<uint16>(category);
    SetShapeFixtureFilter(fixture, filter);
    return GM_OK;
}

int GM_CDECL gmf_shape_isSensor(gmThread* a_thread) {
    // RETAIL PPC VERIFIED: PHYSICSSHAPE.isSensor (0x8d5a0) reads native
    // shape byte +0x44 and returns it as a boolean; fixture state is the local
    // representation of that sensor byte.
    b2Fixture* fixture = ThisShape(a_thread);
    a_thread->PushInt(fixture && fixture->IsSensor() ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_shape_setIsSensor(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(value, 0, 0);
    b2Fixture* fixture = ThisShape(a_thread);
    if (fixture) fixture->SetSensor(value != 0);
    return GM_OK;
}
int GM_CDECL gmf_shape_setFrictionForPlayer(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(value, 0, 0.0f);
    b2Fixture* fixture = ThisShape(a_thread);
    if (!fixture) return GM_OK;
    const int id = (int)fixture->GetUserData().pointer;
    if (id > 0 && id < (int)g_shapePlayerFrictions.size())
        g_shapePlayerFrictions[(size_t)id] = value;
    return GM_OK;
}
// RETAIL PPC VERIFIED: PHYSICSSHAPE.dummy (0x8d184) returns zero without
// reading arguments or mutating state; keep the retail no-op callable.
int GM_CDECL gmf_shape_dummy(gmThread*) { return GM_OK; }

gmFunctionEntry s_physicsShapeLib[] = {
    {"dummy", gmf_shape_dummy, nullptr},
    {"isSensor", gmf_shape_isSensor, nullptr},
    {"getIsSensor", gmf_shape_isSensor, nullptr},
    {"setIsSensor", gmf_shape_setIsSensor, nullptr},
    {"setFrictionForPlayer", gmf_shape_setFrictionForPlayer, nullptr},
    {"getType", gmf_shape_getType, nullptr},
    {"getVertexCount", gmf_shape_getVertexCount, nullptr},
    {"getVertexWorldPos", gmf_shape_getVertexWorldPos, nullptr},
    {"setCategoriesToCollideWith", gmf_shape_setCategoriesToCollideWith, nullptr},
    {"setCollisionCategory", gmf_shape_setCollisionCategory, nullptr},
};

// Fixture ids are global handles exposed to script.  Box2D owns fixtures and
// frees them with their body, so clear the corresponding handle before either
// an individual fixture or its parent body is destroyed.  Leaving the pointer
// in this table lets a later shape-id lookup dereference freed Box2D storage.
void UnregisterShapeGlobalId(b2Fixture* a_fixture) {
    if (!a_fixture) return;
    const int id = (int)a_fixture->GetUserData().pointer;
    if (id > 0 && id < (int)g_shapesByGlobalId.size() &&
        g_shapesByGlobalId[(size_t)id] == a_fixture) {
        g_shapesByGlobalId[(size_t)id] = nullptr;
        g_shapePlayerFrictions[(size_t)id] = 0.3f;
        a_fixture->GetUserData().pointer = 0;
    }
}

void UnregisterBodyShapeGlobalIds(b2Body* a_body) {
    if (!a_body) return;
    for (b2Fixture* fixture = a_body->GetFixtureList(); fixture;
         fixture = fixture->GetNext()) {
        UnregisterShapeGlobalId(fixture);
    }
}

// Removal from PhysicsData is observable immediately: scripts can retain a
// PhysicsBody reference after its owner has rebuilt its shapes.  Detach the
// Box2D object now, rather than waiting for GM collection, and mark the
// wrapper invalid so retained references cannot reach freed Box2D memory.
void InvalidatePhysicsBody(PhysicsBodyData* a_body) {
    if (!a_body || !a_body->body) return;
    b2Body* body = a_body->body;
    // Native PhysicsData body removal erases the contact records immediately,
    // before the manager's same-tick pair-queue recheck. Box2D destruction can
    // be deferred while locked, so remove this body's local pair memberships
    // before clearing its owner pointer or scheduling DestroyBody.
    RemovePhysicsContactPairsForBody(body);
    UnregisterBodyShapeGlobalIds(body);
    // A checkpoint can remove a vehicle from onEntityCollide, i.e. while
    // b2World::Step is still dispatching callbacks. Box2D forbids body or
    // joint destruction while locked. Detach the script-visible wrapper now
    // and defer the Box2D operation until the step has finished.
    body->GetUserData().pointer = 0;
    a_body->body = nullptr;
    g_allPhysicsBodies.erase(
        std::remove(g_allPhysicsBodies.begin(), g_allPhysicsBodies.end(), a_body),
        g_allPhysicsBodies.end());
    if (a_body->objectId > 0 && a_body->objectId < (int)g_bodiesByObjectId.size())
        g_bodiesByObjectId[(size_t)a_body->objectId] = gmVariable::s_null;
    if (GetPhysicsWorld().IsLocked()) {
        if (std::find(g_deferredBodyDestructions.begin(),
                      g_deferredBodyDestructions.end(), body) ==
            g_deferredBodyDestructions.end()) {
            g_deferredBodyDestructions.push_back(body);
        }
    } else {
        // A fixture may have been queued by PreSolve during the step that
        // just ended. Its body can be removed by a deferred script contact
        // event before the fixture queue is flushed.
        g_deferredFixtureDestructions.erase(
            std::remove_if(g_deferredFixtureDestructions.begin(),
                           g_deferredFixtureDestructions.end(),
                           [body](const auto& entry) { return entry.first == body; }),
            g_deferredFixtureDestructions.end());
        RemovePhysicsJointsForBody(body);
        GetPhysicsWorld().DestroyBody(body);
        ClearEntityPhysicsBodyTeardown(body);
    }
}

int GM_CDECL gmf_pb_getPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 p = b->body->GetPosition();
    // GE3_TEXT_DIAG=1 -- only report reads that come back at the origin.
    // Paired with [setpos], this identifies bodies that were positioned
    // once and then read back as (0,0), i.e. destroyed and recreated
    // without configure() re-running.
    if (getenv("GE3_TEXT_DIAG") && p.x == 0.0f && p.y == 0.0f) {
        std::fprintf(stderr, "[getpos0] body=%p\n", (void*)b->body);
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), p.x, p.y));
    return GM_OK;
}
// Retail 0x8e020 returns a vector reference to the body's position at
// native offset +0xc. Keep component writes attached to the Box2D body.
int GM_CDECL gmf_pb_getPosRef(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->Push(MakeVector2DRef(*a_thread->GetMachine(), b->selfVar, b,
                                   [](void* ctx, float& x, float& y) {
                                       auto* body = static_cast<PhysicsBodyData*>(ctx)->body;
                                       if (!body) { x = 0.0f; y = 0.0f; return; }
                                       const b2Vec2 p = body->GetPosition();
                                       x = p.x; y = p.y;
                                   },
                                   [](void* ctx, float x, float y) {
                                       auto* body = static_cast<PhysicsBodyData*>(ctx)->body;
                                       if (body) body->SetTransform(b2Vec2(x, y), body->GetAngle());
                                   }));
    return GM_OK;
}
// Retail 0x8e354 accepts one vector or two numeric coordinates and applies
// the body's full transform. 0x8e5ac accepts the same forms but applies
// rotation only.
int GM_CDECL gmf_pb_getWorldPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (a_thread->GetNumParams() != 1 && a_thread->GetNumParams() != 2) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    const b2Vec2 world = b2Mul(b->body->GetTransform(), b2Vec2(x, y));
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), world.x, world.y));
    return GM_OK;
}
int GM_CDECL gmf_pb_getWorldDir(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (a_thread->GetNumParams() != 1 && a_thread->GetNumParams() != 2) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    const b2Vec2 world = b2Mul(b->body->GetTransform().q, b2Vec2(x, y));
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), world.x, world.y));
    return GM_OK;
}
int GM_CDECL gmf_pb_setPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    b->body->SetTransform(b2Vec2(x, y), b->body->GetAngle());
    // GE3_TEXT_DIAG=1 -- body lifecycle trace. Pair with [mkbody] below: a
    // body that is positioned here and later reads back (0,0) has been
    // destroyed and recreated without configure() re-running.
    if (getenv("GE3_TEXT_DIAG")) {
        std::fprintf(stderr, "[setpos] body=%p (%.2f,%.2f)\n", (void*)b->body, x, y);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 v = ScriptVelocity(b->body->GetLinearVelocity());
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), v.x, v.y));
    return GM_OK;
}
int GM_CDECL gmf_pb_setVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    b->body->SetLinearVelocity(Box2DVelocity(b2Vec2(x, y)));
    if (CreatureDebugEnabled()) {
        int entityId = -1;
        gmMachine& machine = *a_thread->GetMachine();
        if (GetCreatureDebugEntityId(machine, b->body, entityId)) {
            const b2Vec2 pos = b->body->GetPosition();
            const gmVariable owner = FindOwnerEntityForB2Body(b->body);
            auto* ownerTable = owner.m_type == GM_TABLE
                ? (gmTableObject*)machine.GetObject(owner.m_value.m_ref) : nullptr;
            const gmVariable scriptMax = ownerTable
                ? ownerTable->Get(&machine, "maxSpeed") : gmVariable::s_null;
            const gmVariable scriptDirSign = ownerTable
                ? ownerTable->Get(&machine, "dirSign") : gmVariable::s_null;
            const gmVariable gravityDirVar = ownerTable
                ? ownerTable->Get(&machine, "gravityDir") : gmVariable::s_null;
            const gmVariable stateVar = ownerTable
                ? ownerTable->Get(&machine, "state") : gmVariable::s_null;
            auto* stateTable = stateVar.m_type == GM_TABLE
                ? (gmTableObject*)machine.GetObject(stateVar.m_value.m_ref) : nullptr;
            const gmVariable stateIdVar = stateTable
                ? stateTable->Get(&machine, "id") : gmVariable::s_null;
            const gmVariable fallStateVar = ownerTable
                ? ownerTable->Get(&machine, "FALL_STATEID") : gmVariable::s_null;
            const float scriptMaxSpeed = scriptMax.m_type == GM_FLOAT
                ? scriptMax.m_value.m_float
                : (scriptMax.m_type == GM_INT ? (float)scriptMax.m_value.m_int : -1.0f);
            const int dirSign = scriptDirSign.m_type == GM_INT
                ? scriptDirSign.m_value.m_int : 0;
            float gravityX = 0.0f, gravityY = 0.0f;
            const bool haveGravity = GetVector2D(machine, gravityDirVar, gravityX, gravityY);
            const float forwardX = haveGravity ? -gravityY * dirSign : 0.0f;
            const float forwardY = haveGravity ? gravityX * dirSign : 0.0f;
            const float forwardProjection = x * forwardX + y * forwardY;
            const int stateId = stateIdVar.m_type == GM_INT ? stateIdVar.m_value.m_int : -1;
            const int fallStateId = fallStateVar.m_type == GM_INT ? fallStateVar.m_value.m_int : -1;
            std::fprintf(stderr,
                "[creature-setvel] tick=%u entity=%d body=%d pos=(%.5f,%.5f) "
                "requestedMs=(%.7f,%.7f) scriptMax=%.7f dirSign=%d "
                "gravity=(%.5f,%.5f) forward=(%.5f,%.5f) proj=%.7f "
                "state=%d fallState=%d "
                "mass=%.6g awake=%d\n",
                GetEngineContext().gameSystem.currentTick, entityId,
                b->objectId, pos.x, pos.y, x, y, scriptMaxSpeed, dirSign,
                gravityX, gravityY, forwardX, forwardY, forwardProjection,
                stateId, fallStateId,
                b->body->GetMass(),
                b->body->IsAwake() ? 1 : 0);
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getVelRef(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->Push(MakeVector2DRef(*a_thread->GetMachine(), b->selfVar, b,
                                   [](void* ctx, float& x, float& y) {
                                       auto* body = static_cast<PhysicsBodyData*>(ctx);
                                       b2Vec2 v = ScriptVelocity(body->body->GetLinearVelocity());
                                       x = v.x;
                                       y = v.y;
                                   },
                                   [](void* ctx, float x, float y) {
                                       auto* body = static_cast<PhysicsBodyData*>(ctx);
                                       body->body->SetLinearVelocity(Box2DVelocity(b2Vec2(x, y)));
                                   }));
    return GM_OK;
}
int GM_CDECL gmf_pb_applyForce(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    float fx, fy, ox, oy;
    if (!GetVector2D(m, a_thread->Param(0), fx, fy)) return GM_EXCEPTION;
    if (!GetVector2D(m, a_thread->Param(1), ox, oy)) { ox = 0.0f; oy = 0.0f; }
    b->body->ApplyForce((kPhysicsMillisecondsPerSecond * kPhysicsMillisecondsPerSecond) *
                        b2Vec2(fx, fy),
                        b2Vec2(ox, oy), true);
    return GM_OK;
}
int GM_CDECL gmf_pb_getSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(ScriptVelocity(b->body->GetLinearVelocity()).Length());
    return GM_OK;
}
// Retail setSpeed preserves current velocity direction, falling back to
// the body's facing when velocity is zero.
int GM_CDECL gmf_pb_setSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(speed, 0, 0.0f);
    float simSpeed = speed;
    b2Vec2 v = ScriptVelocity(b->body->GetLinearVelocity());
    float len = v.Length();
    if (v.x == 0.0f && v.y == 0.0f) {
        v = simSpeed * BodyFacingDirection(*b);
    } else if (len >= 1.1920928955078125e-7f) {
        v *= (simSpeed / len);
    } else {
        // Retail's normalizer leaves nonzero sub-epsilon vectors unnormalized.
        v *= simSpeed;
    }
    b->body->SetLinearVelocity(Box2DVelocity(v));
    return GM_OK;
}
int GM_CDECL gmf_pb_getDir(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 dir = BodyFacingDirection(*b);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), dir.x, dir.y));
    return GM_OK;
}
int GM_CDECL gmf_pb_getDirRef(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->body->IsFixedRotation()) {
        const float angle = b->body->GetAngle();
        b->dirX = GroundRenderMath::RetailGroundSinCos(angle, true);
        b->dirY = GroundRenderMath::RetailGroundSinCos(angle, false);
    }
    a_thread->Push(MakeVector2DRef(*a_thread->GetMachine(), b->selfVar, b,
                                   [](void* ctx, float& x, float& y) {
                                       const auto* body = static_cast<PhysicsBodyData*>(ctx);
                                       x = body->dirX; y = body->dirY;
                                   },
                                   [](void* ctx, float x, float y) {
                                       auto* body = static_cast<PhysicsBodyData*>(ctx);
                                       body->dirX = x; body->dirY = y;
                                   }));
    return GM_OK;
}
int GM_CDECL gmf_pb_setDir(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    b2Vec2 dir(x, y);
    float dlen = dir.Length();
    if (dlen > 0.0f) dir *= (1.0f / dlen);
    else dir.SetZero();
    if (b->body->IsFixedRotation()) {
        b->dirX = dir.x;
        b->dirY = dir.y;
    } else {
        b->body->SetTransform(b->body->GetPosition(), std::atan2(dir.y, dir.x));
    }
    return GM_OK;
}
constexpr float kDecayRateTickMs = 16.666666f;
float ComputeDecayFactor(float a_rate) {
    float f = std::exp(-a_rate * kDecayRateTickMs);
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return f;
}
int GM_CDECL gmf_pb_thrust(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->thrustFlag) { b->thrustFlag = true; b->thrustAccum += b->thrustAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pb_reverseThrust(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->reverseFlag) { b->reverseFlag = true; b->thrustAccum -= b->thrustAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pb_brake(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b->brakeFlag = true;
    return GM_OK;
}
int GM_CDECL gmf_pb_rotateLeft(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->rotLeftFlag) { b->rotLeftFlag = true; b->rotAccum += b->rotateAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pb_rotateRight(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->rotRightFlag) { b->rotRightFlag = true; b->rotAccum -= b->rotateAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pb_rotate(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    // Retail's rotate(angle) helper only runs if neither directional rotate
    // flag is set, sets both flags, clamps to +/- rotateAcc, then stores the
    // result in the shared rotation accumulator.
    if (!b->rotLeftFlag && !b->rotRightFlag) {
        b->rotLeftFlag = true;
        b->rotRightFlag = true;
        if (v < -b->rotateAcc) v = -b->rotateAcc;
        if (v > b->rotateAcc) v = b->rotateAcc;
        b->rotAccum = v;
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_rotateTo(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    float x, y;
    if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y)) return GM_EXCEPTION;
    b->rotateToActive = true;
    b->rotateToX = x;
    b->rotateToY = y;
    b->rotateToSign = (int)a_thread->ParamFloatOrInt(1, 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_pb_computeAngularVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    (void)a_thread->ParamFloatOrInt(0, 0.0f);
    (void)a_thread->ParamFloatOrInt(1, 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_pb_stopRotatingTo(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b->rotateToActive = false;
    return GM_OK;
}
// Retail polygon vtable +0x1c/+0x20 returns vertex-extrema width/height;
// +0x2c returns its stored centroid. Body bounds use centroid +/- half
// extents. Box2D ComputeAABB includes collision skin and is not this API.
bool GetShapeLocalBounds(const b2Shape* a_shape, b2AABB& a_bounds) {
    if (!a_shape) return false;
    if (a_shape->GetType() == b2Shape::e_circle) {
        const auto* circle = static_cast<const b2CircleShape*>(a_shape);
        const b2Vec2 radius(circle->m_radius, circle->m_radius);
        a_bounds.lowerBound = circle->m_p - radius;
        a_bounds.upperBound = circle->m_p + radius;
        return true;
    }
    const b2Vec2* vertices = nullptr;
    int count = 0;
    b2Vec2 center(0.0f, 0.0f);
    b2Vec2 edgeVertices[2];
    switch (a_shape->GetType()) {
        case b2Shape::e_polygon: {
            const auto* polygon = static_cast<const b2PolygonShape*>(a_shape);
            vertices = polygon->m_vertices;
            count = polygon->m_count;
            center = polygon->m_centroid;
            break;
        }
        case b2Shape::e_edge: {
            const auto* edge = static_cast<const b2EdgeShape*>(a_shape);
            edgeVertices[0] = edge->m_vertex1;
            edgeVertices[1] = edge->m_vertex2;
            vertices = edgeVertices;
            count = 2;
            break;
        }
        case b2Shape::e_chain: {
            const auto* chain = static_cast<const b2ChainShape*>(a_shape);
            vertices = chain->m_vertices;
            count = chain->m_count;
            break;
        }
        default: return false;
    }
    if (!vertices || count <= 0) return false;
    b2Vec2 low = vertices[0], high = vertices[0];
    for (int i = 1; i < count; ++i) {
        low.x = std::min(low.x, vertices[i].x);
        low.y = std::min(low.y, vertices[i].y);
        high.x = std::max(high.x, vertices[i].x);
        high.y = std::max(high.y, vertices[i].y);
    }
    if (a_shape->GetType() != b2Shape::e_polygon) center = 0.5f * (low + high);
    const b2Vec2 half = 0.5f * (high - low);
    a_bounds.lowerBound = center - half;
    a_bounds.upperBound = center + half;
    return true;
}
bool GetBodyLocalBounds(b2Body* a_body, b2AABB& a_bounds) {
    if (!a_body) return false;
    bool found = false;
    for (b2Fixture* fixture = a_body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
        b2AABB part;
        if (!GetShapeLocalBounds(fixture->GetShape(), part)) continue;
        if (!found) a_bounds = part;
        else {
            a_bounds.lowerBound.x = std::min(a_bounds.lowerBound.x, part.lowerBound.x);
            a_bounds.lowerBound.y = std::min(a_bounds.lowerBound.y, part.lowerBound.y);
            a_bounds.upperBound.x = std::max(a_bounds.upperBound.x, part.upperBound.x);
            a_bounds.upperBound.y = std::max(a_bounds.upperBound.y, part.upperBound.y);
        }
        found = true;
    }
    return found;
}
int GM_CDECL gmf_pb_getRadius(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2AABB bounds;
    float radius = 0.0f;
    if (GetBodyLocalBounds(b->body, bounds)) {
        radius = 0.5f * std::max(bounds.upperBound.x - bounds.lowerBound.x,
                                 bounds.upperBound.y - bounds.lowerBound.y);
    }
    a_thread->PushFloat(radius);
    return GM_OK;
}
// Shape-level query/setter pair used by EntityBase's socket attachment and
// sleeping setup. The retail code treats shape indices as linked-list
// positions; FixtureAtShapeIndex mirrors that ordering.
int GM_CDECL gmf_pb_getShapeRadius(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    float radius = 0.0f;
    if (fixture) {
        const b2Shape* shape = fixture->GetShape();
        if (shape->GetType() == b2Shape::e_circle)
            radius = static_cast<const b2CircleShape*>(shape)->m_radius;
        else {
            b2AABB bounds;
            if (GetShapeLocalBounds(shape, bounds))
                radius = std::max(bounds.upperBound.x - bounds.lowerBound.x,
                                  bounds.upperBound.y - bounds.lowerBound.y);
        }
    }
    a_thread->PushFloat(radius);
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeWidth(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    if (!fixture) { a_thread->PushFloat(0.0f); return GM_OK; }
    b2AABB aabb;
    if (!GetShapeLocalBounds(fixture->GetShape(), aabb)) {
        a_thread->PushFloat(0.0f); return GM_OK;
    }
    a_thread->PushFloat(aabb.upperBound.x - aabb.lowerBound.x);
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeHeight(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    if (!fixture) { a_thread->PushFloat(0.0f); return GM_OK; }
    b2AABB aabb;
    if (!GetShapeLocalBounds(fixture->GetShape(), aabb)) {
        a_thread->PushFloat(0.0f); return GM_OK;
    }
    a_thread->PushFloat(aabb.upperBound.y - aabb.lowerBound.y);
    return GM_OK;
}
int GM_CDECL gmf_pb_setAllowSleeping(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(allowed, 0, 0);
    b->body->SetSleepingAllowed(allowed != 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_setAsCircle(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(radius, 0, 0.0f);
    float r = std::max(radius, 0.001f); // same degenerate-shape guard as gmf_addCircle

    b2Filter filter;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = DesiredFixtureFilter(b, f); haveFilter = true; }
        b->savedCollidableFilters.erase(f);
        UnregisterShapeGlobalId(f);
        b->body->DestroyFixture(f);
        f = next;
    }
    b2CircleShape circle;
    circle.m_radius = r;
    b2FixtureDef def;
    def.shape = &circle;
    def.density = 1.0f;
    def.filter = filter;
    b2Fixture* fx = b->body->CreateFixture(&def);
    fx->SetSensor(b->isSensor);
    TrackNewFixture(b, fx);
    RegisterShapeGlobalId(fx);
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
// isCircle()/getWidth()/getHeight() are registered on the body. Retail
// width/height helpers union all local shape extents before subtraction.
int GM_CDECL gmf_pb_isCircle(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt((f && f->GetShape()->GetType() == b2Shape::e_circle) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_isRect(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt((f && f->GetShape()->GetType() == b2Shape::e_polygon) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_getWidth(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2AABB aabb;
    if (!GetBodyLocalBounds(b->body, aabb)) { a_thread->PushFloat(0.0f); return GM_OK; }
    a_thread->PushFloat(aabb.upperBound.x - aabb.lowerBound.x);
    return GM_OK;
}
int GM_CDECL gmf_pb_getHeight(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2AABB aabb;
    if (!GetBodyLocalBounds(b->body, aabb)) { a_thread->PushFloat(0.0f); return GM_OK; }
    a_thread->PushFloat(aabb.upperBound.y - aabb.lowerBound.y);
    return GM_OK;
}
int GM_CDECL gmf_pb_appendShapes(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_USER_PARAM(ShapeDefCollection*, g_physicsShapeDefType, sd, 0);
    for (auto& def : sd->fixtureDefs) {
        b2Fixture* fx = b->body->CreateFixture(&def);
        TrackNewFixture(b, fx);
        RegisterShapeGlobalId(fx, sd->playerFriction);
    }
    SyncBodyTypeFromMass(b->body);
    int lastIndex = -1;
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) ++lastIndex;
    a_thread->PushInt(lastIndex);
    return GM_OK;
}
int GM_CDECL gmf_pb_setCollisionCategory(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(category, 0, 1);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = DesiredFixtureFilter(b, f);
        filter.categoryBits = (uint16)category;
        ApplyDesiredFixtureFilter(b, f, filter);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_setCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(mask, 0, -1);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = DesiredFixtureFilter(b, f);
        filter.maskBits = (uint16)mask;
        ApplyDesiredFixtureFilter(b, f, filter);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getCollisionCategory(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt(f ? (int)DesiredFixtureFilter(b, f).categoryBits : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_getCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt(f ? (int)DesiredFixtureFilter(b, f).maskBits : -1);
    return GM_OK;
}
int GM_CDECL gmf_pb_setCollisionGroup(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(group, 0, 0);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = DesiredFixtureFilter(b, f);
        filter.groupIndex = (int16)group;
        ApplyDesiredFixtureFilter(b, f, filter);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getCollisionGroup(gmThread* a_thread) {
    // RETAIL PPC VERIFIED: PHYSICSBODY.getCollisionGroup (0x90394) resolves
    // shape index 0 and returns its 32-bit field at +0x40. This adapter reads
    // the first fixture's groupIndex; its narrowed width is documented above.
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt(f ? (int)DesiredFixtureFilter(b, f).groupIndex : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_isSleeping(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBodyRaw(a_thread); if (!b) return GM_EXCEPTION;
    // A body removed by PhysicsData.removeBody can still be held by an
    // entity's draw or behaviour state for the rest of the current frame.
    // Retail treats that detached body as inactive; crucially, never follow
    // its former Box2D pointer after removal.
    if (!b->body) { a_thread->PushInt(1); return GM_OK; }
    a_thread->PushInt(b->body->IsAwake() ? 0 : 1);
    return GM_OK;
}
int GM_CDECL gmf_pb_putToSleep(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    SetBodySleeping(b, true);
    return GM_OK;
}
int GM_CDECL gmf_pb_setIsSensor(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    b->isSensor = (v != 0);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext())
        f->SetSensor(b->isSensor);
    return GM_OK;
}
bool AllShapesAreSensors(const b2Body* body) {
    // Retail returns true when there are no fixtures to check.
    if (!body) return true;
    for (const b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()) {
        if (!f->IsSensor()) return false;
    }
    return true;
}
int GM_CDECL gmf_pb_isSensor(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(AllShapesAreSensors(b->body) ? 1 : 0);
    return GM_OK;
}
// A sensor still reports overlap contacts to the listener. Retail callers
// use setIsCollidable(false) to deactivate pooled bullets and hidden fixtures,
// so disable filter matching while preserving their sensor setting and
// authored category/mask/group for a later setIsCollidable(true).
int GM_CDECL gmf_pb_setIsCollidable(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 1);
    SetBodyCollidable(b, v != 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_isCollidable(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(b->isCollidable ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_setFriction(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(f, 0, 0.2f);
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetFriction(f);
    return GM_OK;
}
int GM_CDECL gmf_pb_getFriction(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    b2Fixture* first = b->body->GetFixtureList();
    while (first && first->GetNext()) first = first->GetNext();
    a_thread->PushFloat(first ? first->GetFriction() : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_pb_setRestitution(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(r, 0, 0.0f);
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetRestitution(r);
    return GM_OK;
}
int GM_CDECL gmf_pb_getRestitution(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    b2Fixture* first = b->body->GetFixtureList();
    while (first && first->GetNext()) first = first->GetNext();
    a_thread->PushFloat(first ? first->GetRestitution() : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_pb_setShapeDensity(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_FLOAT_OR_INT_PARAM(d, 1, 1.0f);
    b2Fixture* selected = FixtureAtShapeIndex(b->body, shapeIndex);
    if (selected) selected->SetDensity(d);
    else {
        for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetDensity(d);
    }
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
int GM_CDECL gmf_pb_setIsShapeSensor(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_INT_PARAM(isSensor, 1, 0);
    b2Fixture* fx = FixtureAtShapeIndex(b->body, shapeIndex < 0 ? 0 : shapeIndex);
    if (fx) fx->SetSensor(isSensor != 0);
    else std::fprintf(stderr, "PHYSICSBODY::setIsShapeSensor: unknown shape\n");
    return GM_OK;
}
int GM_CDECL gmf_pb_setShapeCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_INT_PARAM(mask, 1, -1);
    b2Fixture* fx = FixtureAtShapeIndex(b->body, shapeIndex);
    if (fx) {
        b2Filter filter = DesiredFixtureFilter(b, fx);
        filter.maskBits = (uint16)mask;
        ApplyDesiredFixtureFilter(b, fx, filter);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeIndex, 0, 0);
    b2Fixture* fx = FixtureAtShapeIndex(b->body, shapeIndex);
    if (fx) { a_thread->PushInt((int)DesiredFixtureFilter(b, fx).maskBits); return GM_OK; }
    a_thread->PushInt(-1);
    return GM_OK;
}
int GM_CDECL gmf_pb_isBullet(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(b->body->IsBullet() ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_setIsBullet(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    b->body->SetBullet(v != 0);
    return GM_OK;
}
// These names are present in the retail PhysicsShape table. Unlike
// getRotateVel/setRotateVel, they access the underlying body's +0x50 angular
// velocity directly, even when rotation response is disabled.
int GM_CDECL gmf_pb_getAngularVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(ScriptAngularVelocity(b->body->GetAngularVelocity()));
    return GM_OK;
}
int GM_CDECL gmf_pb_setAngularVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    // Retail's bit 0x2 means sleeping (inverse of Box2D's awake bit): a
    // nonzero write clears it and resets sleep time. Box2D's public setter
    // performs the corresponding SetAwake(true) transition.
    b->body->SetAngularVelocity(Box2DAngularVelocity(v));
    return GM_OK;
}
int GM_CDECL gmf_pb_getCenterOfMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 c = b->body->GetWorldCenter();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), c.x, c.y));
    return GM_OK;
}
void SetRetailBodyMass(b2Body* a_body, float a_mass) {
    if (!a_body) return;
    if (a_body->GetMass() != a_mass) {
        b2MassData data;
        a_body->GetMassData(&data);
        data.mass = a_mass;
        data.I = a_mass + b2Dot(data.center, data.center);
        a_body->SetMassData(&data);
    }
    // Retail game scripts use setMass(0) to park dynamic bodies while
    // preserving their contacts (for example ControlledPlatform's STOP
    // state). Box2D still integrates a dynamic body's retained velocity when
    // its inverse mass is zero, so explicitly clear stale motion at this
    // adapter boundary. Otherwise a mover coasts away after setPos()+setMass(0)
    // and its next trip starts from the wrong location.
    if (a_mass <= 0.0f) {
        a_body->SetLinearVelocity(b2Vec2_zero);
        a_body->SetAngularVelocity(0.0f);
    }
}
int GM_CDECL gmf_pb_setMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(mass, 0, 1.0f);
    SetRetailBodyMass(b->body, mass);
    return GM_OK;
}
int GM_CDECL gmf_pb_getMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->body->GetMass());
    return GM_OK;
}
b2Fixture* FixtureAtShapeIndex(b2Body* a_body, int a_index) {
    if (!a_body || a_index < 0) return nullptr;
    int count = 0;
    for (b2Fixture* fx = a_body->GetFixtureList(); fx; fx = fx->GetNext()) ++count;
    if (a_index >= count) return nullptr;
    a_index = count - 1 - a_index;
    b2Fixture* fixture = a_body ? a_body->GetFixtureList() : nullptr;
    while (fixture && a_index > 0) {
        fixture = fixture->GetNext();
        --a_index;
    }
    return fixture;
}
int GM_CDECL gmf_pb_removeShape(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    if (!fixture) return GM_OK;
    UnregisterShapeGlobalId(fixture);
    // The saved filter map is keyed by fixture address. Drop this entry even
    // for deferred destruction so a later Box2D allocation reusing the same
    // address cannot inherit this fixture's old collidability state.
    b->savedCollidableFilters.erase(fixture);
    if (GetPhysicsWorld().IsLocked()) {
        const auto entry = std::make_pair(b->body, fixture);
        if (std::find(g_deferredFixtureDestructions.begin(),
                      g_deferredFixtureDestructions.end(), entry) ==
            g_deferredFixtureDestructions.end()) {
            g_deferredFixtureDestructions.push_back(entry);
        }
    } else {
        b->body->DestroyFixture(fixture);
        SyncBodyTypeFromMass(b->body);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getShape(gmThread*) { return GM_OK; }
int GM_CDECL gmf_pb_getShapeIndex(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeId, 0, 0);
    if (shapeId <= 0) { a_thread->PushInt(-1); return GM_OK; }
    std::vector<b2Fixture*> fixtures;
    for (b2Fixture* fixture = b->body->GetFixtureList(); fixture;
         fixture = fixture->GetNext()) fixtures.push_back(fixture);
    int index = 0;
    for (auto it = fixtures.rbegin(); it != fixtures.rend(); ++it, ++index) {
        if ((int)(*it)->GetUserData().pointer == shapeId) {
            a_thread->PushInt(index);
            return GM_OK;
        }
    }
    a_thread->PushInt(-1);
    return GM_OK;
}
int GM_CDECL gmf_pb_rebuildBodyMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b->body->ResetMassData();
    return GM_OK;
}
int GM_CDECL gmf_pb_getNumShapes(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread);
    if (!b) return GM_EXCEPTION;
    int count = 0;
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) ++count;
    a_thread->PushInt(count);
    return GM_OK;
}
int GM_CDECL gmf_pb_isShapeSensor(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread);
    if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    a_thread->PushInt(fixture && fixture->IsSensor() ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_isPointWithin(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread);
    if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    bool inside = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
        if (f->TestPoint(b2Vec2(x, y))) { inside = true; break; }
    }
    a_thread->PushInt(inside ? 1 : 0);
    return GM_OK;
}
// Retail 0x91ed0 passes a vector and local shape index to helper 0x338e0,
// which selects that shape and tests the point using the body's transform.
int GM_CDECL gmf_pb_isPointWithinShape(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    float x, y;
    if (!GetVector2D(m, a_thread->Param(0), x, y)) return GM_EXCEPTION;
    GM_INT_PARAM(index, 1, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    a_thread->PushInt(fixture && fixture->TestPoint(b2Vec2(x, y)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeType(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    if (!fixture) { a_thread->PushInt(-1); return GM_OK; }
    const b2Shape::Type type = fixture->GetShape()->GetType();
    // Retail's polygon proxy type is 1 (the getNumShapeVerts helper checks
    // that exact value before dispatching to the vertex-count method).
    // Circle is the other explicitly named retail type, zero. Edge and chain
    // are grouped as the remaining non-polygon type for this API.
    const int retailType = type == b2Shape::e_circle ? 0
                         : type == b2Shape::e_polygon ? 1 : 2;
    a_thread->PushInt(retailType);
    return GM_OK;
}
int GM_CDECL gmf_pb_getNumShapeVerts(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    int count = 0;
    if (fixture && fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        count = static_cast<const b2PolygonShape*>(fixture->GetShape())->m_count;
    }
    a_thread->PushInt(count);
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeVert(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_INT_PARAM(vertexIndex, 1, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, shapeIndex);
    b2Vec2 vertex(0.0f, 0.0f);
    if (fixture && fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        const b2PolygonShape* polygon = static_cast<const b2PolygonShape*>(fixture->GetShape());
        if (vertexIndex >= 0 && vertexIndex < polygon->m_count) vertex = polygon->m_vertices[vertexIndex];
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), vertex.x, vertex.y));
    return GM_OK;
}
// Retail 0x8ee48 calls the polygon vertex-count/vertex helpers, inserts
// vector2D values at integer table keys 0..count-1, then sets table.size.
int GM_CDECL gmf_pb_getShapeVerts(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeIndex, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* table = m.AllocTableObject();
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, shapeIndex);
    int count = 0;
    if (fixture && fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        const auto* polygon = static_cast<const b2PolygonShape*>(fixture->GetShape());
        count = polygon->m_count;
        for (int i = 0; i < count; ++i) {
            const b2Vec2& vertex = polygon->m_vertices[i];
            table->Set(&m, gmVariable((gmint)i), MakeVector2D(m, vertex.x, vertex.y));
        }
    }
    table->Set(&m, "size", gmVariable(count));
    a_thread->PushTable(table);
    return GM_OK;
}
void RefreshShapeAfterRetailEdit(PhysicsBodyData* body) {
    body->geometryDirty = true;
}

int GM_CDECL gmf_pb_setPolygonShapeVertices(gmThread* a_thread) {
    PhysicsBodyData* body = ThisBody(a_thread);
    if (!body) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_INT_PARAM(vertexCount, 2, 0);
    if (vertexCount < 0 || vertexCount >= 16) return GM_EXCEPTION;
    const gmVariable verticesVar = a_thread->Param(1);
    if (verticesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* table = (gmTableObject*)machine.GetObject(verticesVar.m_value.m_ref);
    if (!table) return GM_EXCEPTION;

    std::vector<b2Vec2> vertices;
    vertices.reserve((size_t)vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
        float x = 0.0f, y = 0.0f;
        if (!GetVector2D(machine, table->Get(gmVariable((gmint)i)), x, y))
            return GM_EXCEPTION;
        vertices.emplace_back(x, y);
    }

    // Retail's signed bounds check leaves a negative index at the first
    // shape; its traversal advances only for positive indices.
    b2Fixture* fixture = FixtureAtShapeIndex(body->body, std::max(shapeIndex, 0));
    if (!fixture || fixture->GetShape()->GetType() != b2Shape::e_polygon)
        return GM_EXCEPTION;
    auto* polygon = static_cast<b2PolygonShape*>(fixture->GetShape());
    SetRetailPolygonGeometry(polygon, vertices);
    RefreshShapeAfterRetailEdit(body);
    return GM_OK;
}

int GM_CDECL gmf_pb_setEdgeShapeVertex(gmThread* a_thread) {
    PhysicsBodyData* body = ThisBody(a_thread);
    if (!body) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(shapeIndex, 0, 0);
    gmMachine& machine = *a_thread->GetMachine();
    float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
    if (!GetVector2D(machine, a_thread->Param(1), x1, y1) ||
        !GetVector2D(machine, a_thread->Param(2), x2, y2))
        return GM_EXCEPTION;
    // Preserve the same negative-index behavior as retail's shape traversal.
    b2Fixture* fixture = FixtureAtShapeIndex(body->body, std::max(shapeIndex, 0));
    if (!fixture || fixture->GetShape()->GetType() != b2Shape::e_edge)
        return GM_EXCEPTION;
    static_cast<b2EdgeShape*>(fixture->GetShape())->SetTwoSided(
        b2Vec2(x1, y1), b2Vec2(x2, y2));
    RefreshShapeAfterRetailEdit(body);
    return GM_OK;
}

int GM_CDECL gmf_pb_getShapeWorldVert(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_INT_PARAM(vertexIndex, 1, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, shapeIndex);
    b2Vec2 vertex(0.0f, 0.0f);
    if (fixture && fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        const b2PolygonShape* polygon = static_cast<const b2PolygonShape*>(fixture->GetShape());
        if (vertexIndex >= 0 && vertexIndex < polygon->m_count) {
            vertex = b->body->GetWorldPoint(polygon->m_vertices[vertexIndex]);
        }
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), vertex.x, vertex.y));
    return GM_OK;
}
// Retail 0x8f9e4 gets the selected shape's vtable +0x2c offset vector.
// Circle 0x153b30 and polygon 0x154cc8 both return their stored local
// center at native shape offset +0x18. GetShapeLocalBounds uses those same
// centers (circle m_p, polygon m_centroid).
int GM_CDECL gmf_pb_getShapeOffset(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    b2Fixture* fixture = FixtureAtShapeIndex(b->body, index);
    b2AABB bounds;
    b2Vec2 center(0.0f, 0.0f);
    if (fixture && GetShapeLocalBounds(fixture->GetShape(), bounds))
        center = 0.5f * (bounds.lowerBound + bounds.upperBound);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), center.x, center.y));
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeObjectID(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(localIndex, 0, 0);
    b2Fixture* f = FixtureAtShapeIndex(b->body, localIndex);
    if (f) { a_thread->PushInt((int)f->GetUserData().pointer); return GM_OK; }
    a_thread->PushInt(0);
    return GM_OK;
}
// isContainsShape(globalShapeId) is used by GoombaCritterGuitar3's collision
// callback to distinguish contact with the player's main body. CollisionInfo
// and getShapeObjectID already expose the same global fixture IDs, so resolve
// through that registry and compare Box2D body ownership. Missing or removed
// shapes cannot belong to this body.
int GM_CDECL gmf_pb_isContainsShape(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeId, 0, 0);
    b2Fixture* fixture = GetFixtureByGlobalId(shapeId);
    a_thread->PushInt(fixture && fixture->GetBody() == b->body ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeWorldCenterPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(localIndex, 0, 0);
    b2Fixture* f = FixtureAtShapeIndex(b->body, localIndex);
    if (f) {
        b2AABB aabb = f->GetAABB(0);
        b2Vec2 c = 0.5f * (aabb.lowerBound + aabb.upperBound);
        a_thread->Push(MakeVector2D(*a_thread->GetMachine(), c.x, c.y));
        return GM_OK;
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), 0.0f, 0.0f));
    return GM_OK;
}
int GM_CDECL gmf_pb_disableRestitutionThisFrame(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    DisableRestitutionThisFrameForBody(b->body);
    return GM_OK;
}
int GM_CDECL gmf_pb_setMaxSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    b->maxSpeed = v;
    return GM_OK;
}
int GM_CDECL gmf_pb_getMaxSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->maxSpeed);
    return GM_OK;
}
int GM_CDECL gmf_pb_setMaxRotateSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    b->maxRotateSpeed = v;
    return GM_OK;
}
int GM_CDECL gmf_pb_getMaxRotateSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->maxRotateSpeed);
    return GM_OK;
}
int GM_CDECL gmf_pb_setRotateAcc(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    b->rotateAcc = v;
    return GM_OK;
}
int GM_CDECL gmf_pb_getRotateAcc(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->rotateAcc);
    return GM_OK;
}
int GM_CDECL gmf_pb_setBrakeAcc(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    b->brakeAcc = v;
    return GM_OK;
}
int GM_CDECL gmf_pb_getBrakeAcc(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->brakeAcc);
    return GM_OK;
}
int GM_CDECL gmf_pb_setThrustAcc(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    b->thrustAcc = v;
    return GM_OK;
}
int GM_CDECL gmf_pb_getThrustAcc(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->thrustAcc);
    return GM_OK;
}
int GM_CDECL gmf_pb_setMaxVelocityDecayRate(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    b->maxVelocityDecayRate = v;
    return GM_OK;
}
int GM_CDECL gmf_pb_setMaxRotationDecayRate(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    b->maxRotationDecayRate = v;
    return GM_OK;
}
int GM_CDECL gmf_pb_setAsRect(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(width, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(height, 1, 0.0f);
    float hx = std::max(width * 0.5f, 0.001f), hy = std::max(height * 0.5f, 0.001f);
    b2Filter filter;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = DesiredFixtureFilter(b, f); haveFilter = true; }
        b->savedCollidableFilters.erase(f);
        UnregisterShapeGlobalId(f);
        b->body->DestroyFixture(f);
        f = next;
    }
    b2PolygonShape box;
    box.SetAsBox(hx, hy);
    b2FixtureDef def;
    def.shape = &box;
    def.density = 1.0f;
    def.filter = filter;
    b2Fixture* fx = b->body->CreateFixture(&def);
    fx->SetSensor(b->isSensor);
    TrackNewFixture(b, fx);
    RegisterShapeGlobalId(fx);
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
int GM_CDECL gmf_pb_setVelocityDecayRate(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    b->velocityDecayRate = ComputeDecayFactor(v);
    return GM_OK;
}
int GM_CDECL gmf_pb_setRotationDecayRate(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    b->rotationDecayRate = ComputeDecayFactor(v);
    return GM_OK;
}
// setDoesRotationResponse's retail setter has not yet been located. Retail's
// getter reads a custom native flag (bit 0x10); mapping it to Box2D fixed
// rotation is a compatibility approximation until the setter and tick use
// of that flag are recovered.
int GM_CDECL gmf_pb_setDoesRotationResponse(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 1);
    b->body->SetFixedRotation(v == 0);
    return GM_OK;
}
// Retail getter returns one when custom native flag bit 0x10 is clear. The
// constructor initializes this flag clear, so the retail default is enabled.
// Box2D fixed rotation currently stands in for this custom flag; see the
// setter note above for the remaining fidelity gap.
int GM_CDECL gmf_pb_doesRotationResponse(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(b->body->IsFixedRotation() ? 0 : 1);
    return GM_OK;
}
// Retail reads/writes body angular velocity when rotation response is
// enabled, and a shape-local +0x40 value when disabled. Mirror the split so
// setting rotate velocity on a fixed-rotation body doesn't discard it.
int GM_CDECL gmf_pb_getRotateVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->body->IsFixedRotation()
                            ? b->fixedRotationVel
                            : ScriptAngularVelocity(b->body->GetAngularVelocity()));
    return GM_OK;
}
int GM_CDECL gmf_pb_setRotateVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    if (b->body->IsFixedRotation()) b->fixedRotationVel = v;
    else b->body->SetAngularVelocity(Box2DAngularVelocity(v));
    return GM_OK;
}
int GM_CDECL gmf_pb_getObjectID(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(b->objectId);
    return GM_OK;
}
int GM_CDECL gmf_pb_getEntity(gmThread* a_thread); // defined below PhysicsDataObj, registered here
// Retail getAABB/getMinPos/getMaxPos all read the body's cached world-space
// fixture bounds (0x32fe8, 0x33198, 0x331e4). Compute the same union from
// Box2D shapes so all three script entry points share one bounds source.
b2AABB GetBodyWorldAABB(b2Body* body) {
    b2AABB aabb;
    bool first = true;
    for (b2Fixture* fx = body->GetFixtureList(); fx; fx = fx->GetNext()) {
        for (int i = 0; i < fx->GetShape()->GetChildCount(); ++i) {
            b2AABB shapeAabb;
            fx->GetShape()->ComputeAABB(&shapeAabb, body->GetTransform(), i);
            if (first) { aabb = shapeAabb; first = false; }
            else aabb.Combine(shapeAabb);
        }
    }
    if (first) { b2Vec2 p = body->GetPosition(); aabb.lowerBound = p; aabb.upperBound = p; }
    return aabb;
}
// Retail 0x923d0 calls 0x31934 to mark cached body/owner bounds dirty, then
// returns no value. This adapter recomputes those bounds on every query;
// there is no cached result to invalidate.
int GM_CDECL gmf_pb_getDirty(gmThread* a_thread) {
    if (!ThisBody(a_thread)) return GM_EXCEPTION;
    return GM_OK;
}
// Retail 0x8f690 explicitly pushes a null GM value.
int GM_CDECL gmf_pb_getBBox(gmThread* a_thread) {
    if (!ThisBody(a_thread)) return GM_EXCEPTION;
    a_thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_pb_getAABB(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    const b2AABB aabb = GetBodyWorldAABB(b->body);
    gmTableObject* t = m.AllocTableObject();
    t->Set(&m, "minx", gmVariable(aabb.lowerBound.x));
    t->Set(&m, "miny", gmVariable(aabb.lowerBound.y));
    t->Set(&m, "maxx", gmVariable(aabb.upperBound.x));
    t->Set(&m, "maxy", gmVariable(aabb.upperBound.y));
    a_thread->PushTable(t);
    return GM_OK;
}
int GM_CDECL gmf_pb_getMinPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    const b2Vec2 lower = GetBodyWorldAABB(b->body).lowerBound;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), lower.x, lower.y));
    return GM_OK;
}
int GM_CDECL gmf_pb_getMaxPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    const b2Vec2 upper = GetBodyWorldAABB(b->body).upperBound;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), upper.x, upper.y));
    return GM_OK;
}

struct BodyOnlyRayCastCallback : b2RayCastCallback {
    b2Body* onlyBody = nullptr;
    bool hit = false;
    b2Vec2 normal = b2Vec2(0.0f, 0.0f);
    float fraction = 1.0f;
    float ReportFixture(b2Fixture* a_fx, const b2Vec2&, const b2Vec2& a_normal, float a_fraction) override {
        if (a_fx->GetBody() != onlyBody) return -1.0f;
        hit = true; normal = a_normal; fraction = a_fraction;
        return a_fraction;
    }
};
int GM_CDECL gmf_pb_raycast(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    gmMachine& m = *a_thread->GetMachine();
    float sx, sy, ex, ey;
    if (!GetVector2D(m, a_thread->Param(0), sx, sy)) return GM_EXCEPTION;
    if (!GetVector2D(m, a_thread->Param(1), ex, ey)) return GM_EXCEPTION;
    gmVariable resultVar = a_thread->Param(2);
    if (resultVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* resultTable = (gmTableObject*)m.GetObject(resultVar.m_value.m_ref);

    BodyOnlyRayCastCallback cb;
    cb.onlyBody = b->body;
    GetPhysicsWorld().RayCast(&cb, b2Vec2(sx, sy), b2Vec2(ex, ey));

    if (!cb.hit) { a_thread->PushInt(0); return GM_OK; }
    resultTable->Set(&m, "lambda", gmVariable(cb.fraction));
    resultTable->Set(&m, "nx", gmVariable(cb.normal.x));
    resultTable->Set(&m, "ny", gmVariable(cb.normal.y));
    a_thread->PushInt(1);
    return GM_OK;
}

struct RaycastShapesOneResult {
    int shapeId = -1;
    float fraction = 1.0f;
    b2Vec2 normal = b2Vec2(0.0f, 0.0f);
};

bool RaycastPhysicsBodyShapesOne(PhysicsBodyData* a_body, float a_sx, float a_sy,
                                float a_ex, float a_ey,
                                bool a_rejectContainingBodyPoint,
                                bool a_includeSensors,
                                RaycastShapesOneResult& a_result) {
    if (!a_body || !a_body->body) return false;
    const b2Vec2 bodyPoint = a_body->body->GetPosition();
    b2RayCastInput input;
    input.p1.Set(a_sx, a_sy);
    input.p2.Set(a_ex, a_ey);
    input.maxFraction = 1.0f;
    bool hit = false;
    for (b2Fixture* fixture = a_body->body->GetFixtureList(); fixture;
         fixture = fixture->GetNext()) {
        if (!a_includeSensors && fixture->IsSensor()) continue;
        if (a_rejectContainingBodyPoint && fixture->TestPoint(bodyPoint)) continue;
        for (int child = 0; child < fixture->GetShape()->GetChildCount(); ++child) {
            b2RayCastOutput output;
            input.maxFraction = hit ? a_result.fraction : 1.0f;
            if (!fixture->RayCast(&output, input, child)) continue;
            if (hit && output.fraction >= a_result.fraction) continue;
            hit = true;
            a_result.shapeId = (int)(uint32_t)fixture->GetUserData().pointer;
            a_result.fraction = output.fraction;
            a_result.normal = output.normal;
        }
    }
    return hit;
}

// RETAIL PPC VERIFIED: PHYSICSBODY.raycastShapesOne (0x91658) takes two
// vectors, two filter flags, and an output table. It writes lambda/nx/ny/
// shapeID on a hit and returns shapeID, or -1 on a miss.
int GM_CDECL gmf_pb_raycastShapesOne(gmThread* a_thread) {
    PhysicsBodyData* body = ThisBody(a_thread);
    if (!body) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(5);
    gmMachine& machine = *a_thread->GetMachine();
    float sx, sy, ex, ey;
    if (!GetVector2D(machine, a_thread->Param(0), sx, sy) ||
        !GetVector2D(machine, a_thread->Param(1), ex, ey)) return GM_EXCEPTION;
    gmVariable resultVar = a_thread->Param(4);
    if (resultVar.m_type != GM_TABLE) return GM_EXCEPTION;
    auto* result = static_cast<gmTableObject*>(machine.GetObject(resultVar.m_value.m_ref));
    if (!result) return GM_EXCEPTION;
    RaycastShapesOneResult ray;
    const bool hit = RaycastPhysicsBodyShapesOne(body, sx, sy, ex, ey,
        a_thread->ParamInt(2, 0) != 0, a_thread->ParamInt(3, 0) != 0, ray);
    if (!hit) { a_thread->PushInt(-1); return GM_OK; }
    result->Set(&machine, "lambda", gmVariable(ray.fraction));
    result->Set(&machine, "nx", gmVariable(ray.normal.x));
    result->Set(&machine, "ny", gmVariable(ray.normal.y));
    result->Set(&machine, "shapeID", gmVariable((gmint)ray.shapeId));
    a_thread->PushInt(ray.shapeId);
    return GM_OK;
}

// Retail PHYSICSBODY.dummy at 0x8da3c returns immediately without a value.
int GM_CDECL gmf_pb_dummy(gmThread*) { return GM_OK; }
gmFunctionEntry s_physicsBodyLib[] = {
    {"dummy", gmf_pb_dummy, nullptr},
    {"getPos", gmf_pb_getPos, nullptr},
    {"getPosRef", gmf_pb_getPosRef, nullptr},
    {"getPosVec", gmf_pb_getPos, nullptr},
    {"getWorldPos", gmf_pb_getWorldPos, nullptr},
    {"getWorldDir", gmf_pb_getWorldDir, nullptr},
    {"setPos", gmf_pb_setPos, nullptr},
    {"getVel", gmf_pb_getVel, nullptr},
    {"setVel", gmf_pb_setVel, nullptr},
    {"getSpeed", gmf_pb_getSpeed, nullptr},
    {"setSpeed", gmf_pb_setSpeed, nullptr},
    {"getDir", gmf_pb_getDir, nullptr},
    {"getDirRef", gmf_pb_getDirRef, nullptr},
    {"setDir", gmf_pb_setDir, nullptr},
    {"thrust", gmf_pb_thrust, nullptr},
    {"reverseThrust", gmf_pb_reverseThrust, nullptr},
    {"brake", gmf_pb_brake, nullptr},
    {"rotateLeft", gmf_pb_rotateLeft, nullptr},
    {"rotateRight", gmf_pb_rotateRight, nullptr},
    {"rotate", gmf_pb_rotate, nullptr},
    {"rotateTo", gmf_pb_rotateTo, nullptr},
    {"computeAngularVel", gmf_pb_computeAngularVel, nullptr},
    {"stopRotatingTo", gmf_pb_stopRotatingTo, nullptr},
    {"getRadius", gmf_pb_getRadius, nullptr},
    {"getShapeRadius", gmf_pb_getShapeRadius, nullptr},
    {"getShapeWidth", gmf_pb_getShapeWidth, nullptr},
    {"getShapeHeight", gmf_pb_getShapeHeight, nullptr},
    {"setAllowSleeping", gmf_pb_setAllowSleeping, nullptr},
    {"setAsCircle", gmf_pb_setAsCircle, nullptr},
    {"setAsRect", gmf_pb_setAsRect, nullptr},
    {"isCircle", gmf_pb_isCircle, nullptr},
    {"isRect", gmf_pb_isRect, nullptr},
    {"getWidth", gmf_pb_getWidth, nullptr},
    {"getHeight", gmf_pb_getHeight, nullptr},
    {"appendShapes", gmf_pb_appendShapes, nullptr},
    {"removeShape", gmf_pb_removeShape, nullptr},
    {"getShape", gmf_pb_getShape, nullptr},
    {"getShapeIndex", gmf_pb_getShapeIndex, nullptr},
    {"rebuildBodyMass", gmf_pb_rebuildBodyMass, nullptr},
    {"setCollisionCategory", gmf_pb_setCollisionCategory, nullptr},
    {"getCollisionCategory", gmf_pb_getCollisionCategory, nullptr},
    {"setCategoriesToCollideWith", gmf_pb_setCategoriesToCollideWith, nullptr},
    {"getCategoriesToCollideWith", gmf_pb_getCategoriesToCollideWith, nullptr},
    {"setCollisionGroup", gmf_pb_setCollisionGroup, nullptr},
    {"getCollisionGroup", gmf_pb_getCollisionGroup, nullptr},
    {"isSleeping", gmf_pb_isSleeping, nullptr},
    {"putToSleep", gmf_pb_putToSleep, nullptr},
    {"setIsSensor", gmf_pb_setIsSensor, nullptr},
    {"isSensor", gmf_pb_isSensor, nullptr},
    {"setIsBullet", gmf_pb_setIsBullet, nullptr},
    {"setIsUsingContinuousPhysics", gmf_pb_setIsBullet, nullptr},
    {"getAngularVel", gmf_pb_getAngularVel, nullptr},
    {"setAngularVel", gmf_pb_setAngularVel, nullptr},
    {"setIsCollidable", gmf_pb_setIsCollidable, nullptr},
    {"isCollidable", gmf_pb_isCollidable, nullptr},
    {"setFriction", gmf_pb_setFriction, nullptr},
    {"getFriction", gmf_pb_getFriction, nullptr},
    {"setRestitution", gmf_pb_setRestitution, nullptr},
    {"getRestitution", gmf_pb_getRestitution, nullptr},
    {"setShapeDensity", gmf_pb_setShapeDensity, nullptr},
    {"setIsShapeSensor", gmf_pb_setIsShapeSensor, nullptr},
    {"setShapeCategoriesToCollideWith", gmf_pb_setShapeCategoriesToCollideWith, nullptr},
    {"getShapeCategoriesToCollideWith", gmf_pb_getShapeCategoriesToCollideWith, nullptr},
    {"isBullet", gmf_pb_isBullet, nullptr},
    {"getCenterOfMass", gmf_pb_getCenterOfMass, nullptr},
    {"setMass", gmf_pb_setMass, nullptr},
    {"getMass", gmf_pb_getMass, nullptr},
    {"getShapeType", gmf_pb_getShapeType, nullptr},
    {"getNumShapes", gmf_pb_getNumShapes, nullptr},
    {"isShapeSensor", gmf_pb_isShapeSensor, nullptr},
    {"isPointWithin", gmf_pb_isPointWithin, nullptr},
    {"isPointWithinShape", gmf_pb_isPointWithinShape, nullptr},
    {"getNumShapeVerts", gmf_pb_getNumShapeVerts, nullptr},
    {"getShapeVert", gmf_pb_getShapeVert, nullptr},
    {"getShapeVerts", gmf_pb_getShapeVerts, nullptr},
    {"setEdgeShapeVertex", gmf_pb_setEdgeShapeVertex, nullptr},
    {"setPolygonShapeVertices", gmf_pb_setPolygonShapeVertices, nullptr},
    {"getShapeWorldVert", gmf_pb_getShapeWorldVert, nullptr},
    {"getShapeOffset", gmf_pb_getShapeOffset, nullptr},
    {"getShapeObjectID", gmf_pb_getShapeObjectID, nullptr},
    {"isContainsShape", gmf_pb_isContainsShape, nullptr},
    {"getShapeWorldCenterPos", gmf_pb_getShapeWorldCenterPos, nullptr},
    {"setMaxSpeed", gmf_pb_setMaxSpeed, nullptr},
    {"getMaxSpeed", gmf_pb_getMaxSpeed, nullptr},
    {"disableRestitutionThisFrame", gmf_pb_disableRestitutionThisFrame, nullptr},
    {"setVelocityDecayRate", gmf_pb_setVelocityDecayRate, nullptr},
    {"setMaxVelocityDecayRate", gmf_pb_setMaxVelocityDecayRate, nullptr},
    {"setMaxRotateSpeed", gmf_pb_setMaxRotateSpeed, nullptr},
    {"getMaxRotateSpeed", gmf_pb_getMaxRotateSpeed, nullptr},
    {"setRotationDecayRate", gmf_pb_setRotationDecayRate, nullptr},
    {"setMaxRotationDecayRate", gmf_pb_setMaxRotationDecayRate, nullptr},
    {"setRotateAcc", gmf_pb_setRotateAcc, nullptr},
    {"getRotateAcc", gmf_pb_getRotateAcc, nullptr},
    {"setBrakeAcc", gmf_pb_setBrakeAcc, nullptr},
    {"getBrakeAcc", gmf_pb_getBrakeAcc, nullptr},
    {"setThrustAcc", gmf_pb_setThrustAcc, nullptr},
    {"getThrustAcc", gmf_pb_getThrustAcc, nullptr},
    {"setDoesRotationResponse", gmf_pb_setDoesRotationResponse, nullptr},
    {"doesRotationResponse", gmf_pb_doesRotationResponse, nullptr},
    {"getRotateVel", gmf_pb_getRotateVel, nullptr},
    {"setRotateVel", gmf_pb_setRotateVel, nullptr},
    {"getObjectID", gmf_pb_getObjectID, nullptr},
    {"getEntity", gmf_pb_getEntity, nullptr},
    {"getBBox", gmf_pb_getBBox, nullptr},
    {"getDirty", gmf_pb_getDirty, nullptr},
    {"getAABB", gmf_pb_getAABB, nullptr},
    {"getMinPos", gmf_pb_getMinPos, nullptr},
    {"getMaxPos", gmf_pb_getMaxPos, nullptr},
    {"raycast", gmf_pb_raycast, nullptr},
    {"raycastShapesOne", gmf_pb_raycastShapesOne, nullptr},
    {"applyForce", gmf_pb_applyForce, nullptr},
    {"getVelRef", gmf_pb_getVelRef, nullptr},
};

void GM_CDECL PhysicsBodyGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    PhysicsBodyData* b = (PhysicsBodyData*)a_object->m_user;
    if (!b) return;
    // Drop the registry entry BEFORE freeing. g_allPhysicsBodies is the list
    // StepPhysicsBodyMovement() walks every tick, and it was only ever
    // pushed to -- nothing erased from it, here or anywhere else. So every
    // garbage-collected body left a dangling PhysicsBodyData* behind, and
    // the next tick dereferenced freed memory.
    //
    // Not a latent worry: this is the segfault that killed the run the
    // moment the tutorial level started loading (SIGSEGV inside
    // StepPhysicsBodyMovement, straight after Platformer/colours.dat
    // loaded), because level load is the first point where entities -- and
    // therefore bodies -- are created and collected in bulk.
    g_allPhysicsBodies.erase(
        std::remove(g_allPhysicsBodies.begin(), g_allPhysicsBodies.end(), b),
        g_allPhysicsBodies.end());
    // Object IDs are long-lived script references (Chain2 stores them for
    // each link and resolves them later through PHYSICSBODY_GetBodyByID).
    // Once this wrapper is destroyed, leaving its GM user object in the
    // registry turns that lookup into a use-after-free: the next
    // `...GetBodyByID(id).isSleeping()` reaches a freed PhysicsBodyData and
    // crashes. A removed body has no valid object to return.
    if (b->objectId > 0 && b->objectId < (int)g_bodiesByObjectId.size())
        g_bodiesByObjectId[(size_t)b->objectId] = gmVariable::s_null;
    InvalidatePhysicsBody(b);
    delete b;
}

// --- PhysicsData (the per-entity container of bodies) -------------------

struct PhysicsDataObj {
    std::vector<gmVariable> bodies; // each a PhysicsBody user object
    int type = 0; // selected by EntityManager.createEntity's physics-type argument
    // Retail setVisibilityAABBScale stores these at PhysicsData offsets
    // +0x18/+0x1c before asking its spatial index to rebuild cached bounds.
    // The local entity query path does not yet have that retail index/update.
    float visibilityAabbScaleX = 1.0f;
    float visibilityAabbScaleY = 1.0f;
    // rotateTo's retail multi-body wrapper sets its native update flag at
    // PhysicsData offset +0x40. The corresponding consumer/reset is unknown.
    bool rotateToDirty = false;
    // The entity table this PhysicsData belongs to, if any -- see
    // MakePhysicsData/getCollidingEntities.
    gmVariable ownerEntity = gmVariable::s_null;
    // One rooted GM CollisionInfo user object per active directed pair. Retail
    // returns the existing pair-map value pointer on every lookup; this cache
    // preserves that identity until physics_world invalidates the pair.
    std::unordered_map<uint32_t, gmVariable> collisionInfoViews;
};

PhysicsDataObj* ThisData(gmThread* a_thread) {
    return (PhysicsDataObj*)a_thread->ThisUserCheckType(g_physicsDataType);
}

// Entity hit tests must include every body and every fixture. TestPoint uses
// each body's transform and the actual Box2D shape geometry.
int GM_CDECL gmf_pd_isPointWithin(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread);
    if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    float x, y;
    if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y)) return GM_EXCEPTION;
    bool inside = false;
    for (const gmVariable& bodyVar : d->bodies) {
        if (bodyVar.m_type != g_physicsBodyType) continue;
        gmObject* obj = a_thread->GetMachine()->GetObject(bodyVar.m_value.m_ref);
        PhysicsBodyData* b = obj ? (PhysicsBodyData*)((gmUserObject*)obj)->m_user : nullptr;
        if (!b || !b->body) continue;
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
            if (f->TestPoint(b2Vec2(x, y))) { inside = true; break; }
        }
        if (inside) break;
    }
    a_thread->PushInt(inside ? 1 : 0);
    return GM_OK;
}

// Retail PhysicsData.getType() exposes the type selected by
// EntityManager.createEntity(typeName, physicsType).
int GM_CDECL gmf_pd_getType(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread);
    if (!d) return GM_EXCEPTION;
    a_thread->PushInt(d->type);
    return GM_OK;
}

// Retail Physics.setVisibilityAABBScale(float x, float y): converts each
// float-or-int argument to float, stores the pair, then refreshes the native
// visibility-AABB index. The scale storage is represented here; the index
// refresh remains unavailable until the retail spatial-index path is ported.
int GM_CDECL gmf_pd_setVisibilityAABBScale(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread);
    if (!d) return GM_EXCEPTION;
    d->visibilityAabbScaleX = a_thread->ParamFloatOrInt(0, 0.0f);
    d->visibilityAabbScaleY = a_thread->ParamFloatOrInt(1, 0.0f);
    return GM_OK;
}

PhysicsBodyData* GetBodyData(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type != g_physicsBodyType) return nullptr;
    gmObject* obj = a_machine.GetObject(a_var.m_value.m_ref);
    if (!obj) return nullptr;
    return (PhysicsBodyData*)((gmUserObject*)obj)->m_user;
}

int GM_CDECL gmf_pb_getEntity(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    PhysicsDataObj* owner = (PhysicsDataObj*)(uintptr_t)b->body->GetUserData().pointer;
    if (owner && owner->ownerEntity.m_type == GM_TABLE) a_thread->Push(owner->ownerEntity);
    else a_thread->PushNull();
    return GM_OK;
}

PhysicsBodyData* MainBody(gmThread* a_thread, PhysicsDataObj* a_data) {
    gmMachine& m = *a_thread->GetMachine();
    if (a_data->bodies.empty()) {
        gmVariable body = MakePhysicsBody(m, 0.0f, 0.0f, false, true);
        PhysicsBodyData* bd = GetBodyData(m, body);
        if (bd && bd->body) bd->body->GetUserData().pointer = (uintptr_t)a_data;
        RootGmVariable(m, body);
        a_data->bodies.push_back(body);
    }
    return GetBodyData(m, a_data->bodies[0]);
}

int GM_CDECL gmf_pd_createBody(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    float x = 0.0f, y = 0.0f;
    bool fixedRotation = false;
    bool awake = true;
    if (a_thread->GetNumParams() >= 1 && a_thread->Param(0).m_type == GM_TABLE) {
        gmTableObject* def = (gmTableObject*)m.GetObject(a_thread->Param(0).m_value.m_ref);
        gmVariable xv = def->Get(&m, "x");
        gmVariable yv = def->Get(&m, "y");
        if (xv.m_type == GM_FLOAT) x = xv.m_value.m_float; else if (xv.m_type == GM_INT) x = (float)xv.m_value.m_int;
        if (yv.m_type == GM_FLOAT) y = yv.m_value.m_float; else if (yv.m_type == GM_INT) y = (float)yv.m_value.m_int;
        gmVariable frv = def->Get(&m, "fixedRotation");
        if (frv.m_type == GM_INT) fixedRotation = (frv.m_value.m_int != 0);
        gmVariable slv = def->Get(&m, "sleeping");
        if (slv.m_type == GM_INT) awake = (slv.m_value.m_int == 0);
    } else if (a_thread->GetNumParams() >= 1) {
        bool typeArg = (a_thread->ParamInt(0, 1) != 0);
        fixedRotation = !typeArg;
    }
    gmVariable body = MakePhysicsBody(m, x, y, fixedRotation, awake);
    // Link the Box2D body back to its owning PhysicsData so a contact on
    // it can be resolved back to an entity -- see getCollidingEntities.
    PhysicsBodyData* bd = GetBodyData(m, body);
    if (bd && bd->body) bd->body->GetUserData().pointer = (uintptr_t)d;
    // d->bodies is a plain C++ vector<gmVariable> inside a native struct --
    // GM's GC has no way to see into it (see process_manager.h's
    // RootGmVariable header comment), so without this the PhysicsBody
    // object here could be collected while still the only thing keeping
    // its b2Body's back-pointer meaningful, leaving a dangling
    // PhysicsDataObj* the next time something reads that body's user
    // data. Unrooted again in PhysicsDataObj's own GC destructor below.
    RootGmVariable(m, body);
    int index = (int)d->bodies.size();
    d->bodies.push_back(body);
    a_thread->PushInt(index);
    return GM_OK;
}
int GM_CDECL gmf_pd_getBody(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    if (index >= 0 && index < (int)d->bodies.size()) a_thread->Push(d->bodies[(size_t)index]);
    else a_thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_pd_removeBody(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    int bodyIdOrIndex = -1;
    if (a_thread->GetNumParams() >= 1) bodyIdOrIndex = a_thread->ParamInt(0, -1);
    gmMachine& m = *a_thread->GetMachine();
    if (bodyIdOrIndex < 0) {
        for (gmVariable& bv : d->bodies) {
            InvalidatePhysicsBody(GetBodyData(m, bv));
            UnrootGmVariable(m, bv);
        }
        d->bodies.clear();
    } else {
        int index = -1;
        for (size_t i = 0; i < d->bodies.size(); ++i) {
            PhysicsBodyData* body = GetBodyData(m, d->bodies[i]);
            if (body && body->objectId == bodyIdOrIndex) {
                index = (int)i;
                break;
            }
        }
        if (index < 0 && bodyIdOrIndex < (int)d->bodies.size()) {
            index = bodyIdOrIndex;
        }
        if (index < 0) return GM_OK;
        InvalidatePhysicsBody(GetBodyData(m, d->bodies[(size_t)index]));
        UnrootGmVariable(m, d->bodies[(size_t)index]);
        d->bodies.erase(d->bodies.begin() + index);
    }
    return GM_OK;
}

int GM_CDECL gmf_pd_getNumBodies(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    a_thread->PushInt((int)d->bodies.size());
    return GM_OK;
}

int GM_CDECL gmf_pd_raycast(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    gmMachine& m = *a_thread->GetMachine();
    float sx, sy, ex, ey;
    if (!GetVector2D(m, a_thread->Param(0), sx, sy) ||
        !GetVector2D(m, a_thread->Param(1), ex, ey)) return GM_EXCEPTION;
    gmVariable resultVar = a_thread->Param(2);
    if (resultVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* result = (gmTableObject*)m.GetObject(resultVar.m_value.m_ref);
    if (!result) return GM_EXCEPTION;

    b2RayCastInput input;
    input.p1.Set(sx, sy);
    input.p2.Set(ex, ey);
    input.maxFraction = 1.0f;
    bool hit = false;
    float bestFraction = 1.0f;
    b2Vec2 bestNormal(0.0f, 0.0f);
    for (const gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* body = GetBodyData(m, bodyVar);
        if (!body || !body->body) continue;
        for (b2Fixture* fixture = body->body->GetFixtureList(); fixture;
             fixture = fixture->GetNext()) {
            for (int child = 0; child < fixture->GetShape()->GetChildCount(); ++child) {
                b2RayCastOutput output;
                input.maxFraction = bestFraction;
                if (fixture->RayCast(&output, input, child) &&
                    (!hit || output.fraction < bestFraction)) {
                    hit = true;
                    bestFraction = output.fraction;
                    bestNormal = output.normal;
                }
            }
        }
    }
    if (hit) {
        result->Set(&m, "lambda", gmVariable(bestFraction));
        result->Set(&m, "nx", gmVariable(bestNormal.x));
        result->Set(&m, "ny", gmVariable(bestNormal.y));
    }
    a_thread->PushInt(hit ? 1 : 0);
    return GM_OK;
}

// RETAIL PPC VERIFIED: MULTIBODYPHYSICSDATA.raycastShapesOne (0x95100)
// walks the body's vector in index order, calls 0x339a4 for each body, and
// retains only a strictly nearer hit. A tie therefore keeps the earlier
// body/shape. The output keys and integer shapeID/-1 return match wrapper
// 0x91658. Point-field mapping and local fixture traversal order remain open.
int GM_CDECL gmf_pd_raycastShapesOne(gmThread* a_thread) {
    PhysicsDataObj* data = ThisData(a_thread);
    if (!data) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(5);
    gmMachine& machine = *a_thread->GetMachine();
    float sx, sy, ex, ey;
    if (!GetVector2D(machine, a_thread->Param(0), sx, sy) ||
        !GetVector2D(machine, a_thread->Param(1), ex, ey)) return GM_EXCEPTION;
    gmVariable resultVar = a_thread->Param(4);
    if (resultVar.m_type != GM_TABLE) return GM_EXCEPTION;
    auto* result = static_cast<gmTableObject*>(machine.GetObject(resultVar.m_value.m_ref));
    if (!result) return GM_EXCEPTION;

    const bool rejectContainingBodyPoint = a_thread->ParamInt(2, 0) != 0;
    const bool includeSensors = a_thread->ParamInt(3, 0) != 0;
    bool hit = false;
    RaycastShapesOneResult closest;
    for (const gmVariable& bodyVar : data->bodies) {
        PhysicsBodyData* body = GetBodyData(machine, bodyVar);
        RaycastShapesOneResult candidate;
        if (!RaycastPhysicsBodyShapesOne(body, sx, sy, ex, ey,
                rejectContainingBodyPoint, includeSensors, candidate)) continue;
        if (hit && candidate.fraction >= closest.fraction) continue;
        hit = true;
        closest = candidate;
    }
    if (!hit) { a_thread->PushInt(-1); return GM_OK; }
    result->Set(&machine, "lambda", gmVariable(closest.fraction));
    result->Set(&machine, "nx", gmVariable(closest.normal.x));
    result->Set(&machine, "ny", gmVariable(closest.normal.y));
    result->Set(&machine, "shapeID", gmVariable((gmint)closest.shapeId));
    a_thread->PushInt(closest.shapeId);
    return GM_OK;
}

int GM_CDECL gmf_pd_getShapeByID(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeId, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    b2Fixture* fixture = GetFixtureByGlobalId(shapeId);
    if (fixture) {
        for (const gmVariable& bodyVar : d->bodies) {
            PhysicsBodyData* body = GetBodyData(m, bodyVar);
            if (body && body->body == fixture->GetBody()) {
                a_thread->Push(MakePhysicsShapeByGlobalId(m, shapeId));
                return GM_OK;
            }
        }
    }
    a_thread->PushNull();
    return GM_OK;
}
// RETAIL PPC VERIFIED: getCollidingEntities (0x8bc64) fills a linked list
// from the active pair tree via 0x315bc; the outer tree key is the other
// Entity's unsigned ID at Entity+4. Each mapped pair record supplies the
// other PhysicsData pointer; iterator get resolves it to its owner Entity.
// The one-argument path clears/refills the same EntityListRefContainer and
// resets its cursor through 0x31c178/0x31abec. The local retained map uses
// that same other-Entity-ID ordering and keeps the initially mapped
// PhysicsData owner for the lifetime of the pair.
// Desktop getCollidingEntities reads the retained directed pair map, matching
// retail's ordered outer Entity-ID tree and preserving the pair value through
// synchronous onEntitySeparate dispatch.
int GM_CDECL gmf_pd_getCollidingEntities(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    std::vector<gmVariable> found;
    GetPhysicsCollidingEntitiesForPhysicsData(
        reinterpret_cast<uintptr_t>(d), found);
    if (CreatureDebugEnabled() && d->ownerEntity.m_type == GM_TABLE) {
        int creatureId = -1;
        gmMachine& machine = *a_thread->GetMachine();
        auto* ownerTable = (gmTableObject*)machine.GetObject(d->ownerEntity.m_value.m_ref);
        const gmVariable typeVar = ownerTable ? ownerTable->Get(&machine, "__typeName") : gmVariable::s_null;
        auto* typeName = typeVar.m_type == GM_STRING
            ? (gmStringObject*)machine.GetObject(typeVar.m_value.m_ref) : nullptr;
        const gmVariable ownerId = ownerTable ? ownerTable->Get(&machine, "__objectID") : gmVariable::s_null;
        if (typeName && (std::strcmp(typeName->GetString(), "Platformer_EntityPacks_GoombaCritterGuitar3") == 0 ||
                         std::strcmp(typeName->GetString(), "Platformer_EntityPacks_SlimeEyes") == 0)) {
            if (ownerId.m_type == GM_INT) creatureId = ownerId.m_value.m_int;
            std::fprintf(stderr, "[creature-colliding] tick=%u entity=%d results=%zu",
                GetEngineContext().gameSystem.currentTick, creatureId, found.size());
            for (const gmVariable& other : found) {
                int otherId = -1;
                if (other.m_type == GM_TABLE) {
                    auto* t = (gmTableObject*)a_thread->GetMachine()->GetObject(other.m_value.m_ref);
                    const gmVariable id = t ? t->Get(a_thread->GetMachine(), "__objectID") : gmVariable::s_null;
                    if (id.m_type == GM_INT) otherId = id.m_value.m_int;
                    const gmVariable otherType = t ? t->Get(a_thread->GetMachine(), "__typeName") : gmVariable::s_null;
                    auto* otherName = otherType.m_type == GM_STRING
                        ? (gmStringObject*)a_thread->GetMachine()->GetObject(otherType.m_value.m_ref) : nullptr;
                    std::fprintf(stderr, " %d:%s", otherId, otherName ? otherName->GetString() : "?");
                } else {
                    std::fprintf(stderr, " %d:<non-table>", otherId);
                }
            }
            std::fprintf(stderr, "\n");
            const gmVariable mainBodyVar = ownerTable
                ? ownerTable->Get(&machine, "mainpb") : gmVariable::s_null;
            PhysicsBodyData* mainBody = GetBodyData(machine, mainBodyVar);
            b2Fixture* mainShape0 = mainBody && mainBody->body
                ? FixtureAtShapeIndex(mainBody->body, 0) : nullptr;
            const int mainShapeId = mainShape0
                ? (int)mainShape0->GetUserData().pointer : -1;
            std::fprintf(stderr, "[creature-contact-detail] tick=%u entity=%d main0=%d",
                GetEngineContext().gameSystem.currentTick, creatureId, mainShapeId);
            for (const gmVariable& other : found) {
                int otherId = -1;
                uintptr_t otherData = 0;
                if (other.m_type == GM_TABLE) {
                    auto* t = (gmTableObject*)machine.GetObject(other.m_value.m_ref);
                    const gmVariable id = t ? t->Get(&machine, "__objectID") : gmVariable::s_null;
                    if (id.m_type == GM_INT) otherId = id.m_value.m_int;
                    const gmVariable pdVar = t ? t->Get(&machine, "PhysicsData") : gmVariable::s_null;
                    gmObject* pdObject = pdVar.m_type == g_physicsDataType
                        ? machine.GetObject(pdVar.m_value.m_ref) : nullptr;
                    if (pdObject)
                        otherData = (uintptr_t)((gmUserObject*)pdObject)->m_user;
                }
                std::vector<PhysicsContactRecordView> records;
                if (otherId >= 0 && otherData != 0 && GetPhysicsContactRecordsForPhysicsData(
                        reinterpret_cast<uintptr_t>(d), (uint32_t)otherId, records)) {
                    for (const PhysicsContactRecordView& contact : records) {
                        b2Vec2 normal(0.0f, 0.0f);
                        if (contact.contact) {
                            b2WorldManifold manifold{};
                            contact.contact->GetWorldManifold(&manifold);
                            normal = contact.normalSign * manifold.normal;
                        }
                        std::fprintf(stderr,
                            " {%d:%d->%d,n=(%.3f,%.3f)}", otherId,
                            contact.myShapeId, contact.otherShapeId,
                            normal.x, normal.y);
                    }
                }
            }
            std::fprintf(stderr, "\n");
        }
    }
    const gmVariable* reuse = a_thread->GetNumParams() > 0 ? &a_thread->Param(0) : nullptr;
    if (!PushOrRefillEntityListIterator(a_thread, std::move(found), reuse))
        return GM_EXCEPTION;
    return GM_OK;
}

int GM_CDECL gmf_pd_getJointsOnMe(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    PhysicsDataObj* data = ThisData(a_thread);
    std::vector<b2Body*> bodies;
    if (data) {
        bodies.reserve(data->bodies.size());
        for (const gmVariable& bodyVar : data->bodies) {
            PhysicsBodyData* body = GetBodyData(m, bodyVar);
            if (body && body->body) bodies.push_back(body->body);
        }
    }
    a_thread->Push(MakePhysicsJointIterator(m, bodies));
    return GM_OK;
}

gmType g_shapeCollisionInfoType = -1;
gmType g_intIterType = -1;

struct ContactPoints {
    b2Vec2 pts[2] = {b2Vec2(0, 0), b2Vec2(0, 0)};
    int count = 0;
};
struct ShapeContactEntry {
    int otherShapeId = -1;
    std::vector<int> myShapeIds;
    std::vector<b2Vec2> normals; // world-space, pointing from the other shape toward mine ("itToMe")
    std::vector<float> normalSigns; // live-contact orientation from retail record +4
    std::vector<ContactPoints> points;
    // The live b2Contact each record came from, so disableContact can mark
    // contacts in the selected SCI when its otherShapeId matches the raw ID
    // argument passed by the script.
    std::vector<b2Contact*> contacts;
};
struct CollisionInfoData {
    std::vector<ShapeContactEntry> contacts;
    // Retail CollisionInfo.getEntity returns the entity owning the OTHER
    // PhysicsData in this directed pair record (PPC 0x97e00; record created
    // at 0x30f28). Like the retail record, this handle is only valid while its
    // owner PhysicsData remains alive; it is not an independent GC root.
    gmVariable otherEntity = gmVariable::s_null;
    // A native pair entry remains findable during onEntitySeparate until its
    // callback returns, even after its final contact record was erased.
    bool pairExists = false;
    uintptr_t physicsDataKey = 0;
    uint32_t otherEntityId = 0;
    bool invalidated = false;
    gmMachine* machine = nullptr;
    // Retail keeps one ShapeCollisionInfo value per outer shape key inside
    // the CollisionInfo map. Cache the GM views here so repeated lookup of
    // that key returns the same script-visible object while the pair lives.
    std::unordered_map<int, gmVariable> shapeCollisionInfoViews;
};
struct ShapeCollisionInfoData {
    // Retail records are grouped by their outer other-shape ID; record[3]
    // (compared with disableContact's raw argument) stores this same ID.
    int otherShapeId = -1;
    std::vector<int> myShapeIds;
    std::vector<b2Vec2> normals;
    std::vector<float> normalSigns;
    std::vector<ContactPoints> points;
    std::vector<b2Contact*> contacts; // native contact records represented by this SCI
    // Retail returns a pointer into the CollisionInfo's stored shape-group
    // map. Keep the parent object alive and resolve this key again on each
    // getter so the local proxy observes live group contents.
    gmVariable ownerCollisionInfo = gmVariable::s_null;
    gmMachine* machine = nullptr;
};
struct IntIterData {
    std::vector<int> values;
    size_t index = 0;
};

void AcquireCollisionInfoRoot(gmMachine& a_machine, const gmVariable& a_view) {
    if (a_view.m_type != g_collisionInfoType) return;
    auto& leases = g_collisionInfoRootLeases[&a_machine];
    size_t& count = leases[a_view.m_value.m_ref];
    if (count++ == 0) RootGmVariable(a_machine, a_view);
}

void ReleaseCollisionInfoRoot(gmMachine& a_machine, const gmVariable& a_view) {
    if (a_view.m_type != g_collisionInfoType) return;
    auto machineIt = g_collisionInfoRootLeases.find(&a_machine);
    if (machineIt == g_collisionInfoRootLeases.end()) return;
    auto& leases = machineIt->second;
    auto viewIt = leases.find(a_view.m_value.m_ref);
    if (viewIt == leases.end() || viewIt->second == 0) return;
    if (--viewIt->second == 0) {
        UnrootGmVariable(a_machine, a_view);
        leases.erase(viewIt);
        if (leases.empty()) g_collisionInfoRootLeases.erase(machineIt);
    }
}

void ReleaseShapeCollisionInfoViews(CollisionInfoData* a_info) {
    if (!a_info || !a_info->machine) return;
    gmMachine& machine = *a_info->machine;
    if (machine.IsResetting()) {
        // The GM collector destroys payloads in arbitrary order while
        // resetting; its reset pass owns all remaining C++ roots.
        a_info->shapeCollisionInfoViews.clear();
        return;
    }
    for (const auto& view : a_info->shapeCollisionInfoViews) {
        if (view.second.m_type == g_shapeCollisionInfoType) {
            gmObject* object = machine.GetObject(view.second.m_value.m_ref);
            auto* user = object ? static_cast<gmUserObject*>(object) : nullptr;
            auto* shape = user
                ? static_cast<ShapeCollisionInfoData*>(user->m_user) : nullptr;
            if (shape) {
                shape->myShapeIds.clear();
                shape->normals.clear();
                shape->normalSigns.clear();
                shape->points.clear();
                shape->contacts.clear();
            }
        }
        UnrootGmVariable(machine, view.second);
    }
    a_info->shapeCollisionInfoViews.clear();
}

int GM_CDECL gmf_INTLISTREFCONTAINER(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* obj = m.AllocUserObject(new IntIterData(), g_intIterType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}

// RETAIL PPC VERIFIED: outer CollisionInfo groups use a signed-key tree, but
// each group's contact records preserve contact-add insertion order (0x30f28)
// with stable deletion (0x31308). Query this adapter's persistent listener
// records in contact-add order, then sort only outer signed shape-ID groups.
CollisionInfoData* BuildCollisionInfo(gmMachine& a_machine, PhysicsDataObj* a_myData, const gmVariable& a_otherEntityVar) {
    CollisionInfoData* info = new CollisionInfoData();
    info->machine = &a_machine;
    if (a_otherEntityVar.m_type != GM_TABLE) return info;
    gmTableObject* otherTable = (gmTableObject*)a_machine.GetObject(a_otherEntityVar.m_value.m_ref);
    if (!otherTable) return info;
    gmVariable pdVar = otherTable->Get(&a_machine, "PhysicsData");
    if (pdVar.m_type != g_physicsDataType) pdVar = otherTable->Get(&a_machine, "__physicsData");
    if (pdVar.m_type != g_physicsDataType) return info;
    gmObject* obj = a_machine.GetObject(pdVar.m_value.m_ref);
    if (!obj) return info;
    PhysicsDataObj* otherData = (PhysicsDataObj*)((gmUserObject*)obj)->m_user;
    info->otherEntity = otherData->ownerEntity;
    int otherEntityId = -1;
    if (info->otherEntity.m_type == GM_TABLE) {
        gmTableObject* owner = static_cast<gmTableObject*>(
            a_machine.GetObject(info->otherEntity.m_value.m_ref));
        if (owner) {
            const gmVariable id = owner->Get(&a_machine, "__objectID");
            if (id.m_type == GM_INT) otherEntityId = id.m_value.m_int;
        }
    }
    if (otherEntityId < 0) return info;
    info->physicsDataKey = reinterpret_cast<uintptr_t>(a_myData);
    info->otherEntityId = static_cast<uint32_t>(otherEntityId);
    std::vector<PhysicsContactRecordView> records;
    info->pairExists = GetPhysicsContactRecordsForPhysicsData(
        reinterpret_cast<uintptr_t>(a_myData),
        static_cast<uint32_t>(otherEntityId), records);
    for (const PhysicsContactRecordView& record : records) {
        b2Contact* contact = record.contact;
        if (!contact) continue;
        b2Fixture* fA = contact->GetFixtureA();
        b2Fixture* fB = contact->GetFixtureB();
        const bool myIsA = record.normalSign < 0.0f;
        b2Fixture* myFixture = myIsA ? fA : fB;
        b2Fixture* otherFixture = myIsA ? fB : fA;
        // Retained contact records can describe sensor overlaps with no
        // manifold points. Zero initialization matches the native helper's
        // point-count checks without exposing undefined output bytes.
        b2WorldManifold wm{};
        contact->GetWorldManifold(&wm);
        ContactPoints cp;
        cp.count = contact->GetManifold()->pointCount;
        for (int point = 0; point < cp.count && point < 2; ++point)
            cp.pts[point] = wm.points[point];

        ShapeContactEntry* entry = nullptr;
        for (auto& candidate : info->contacts)
            if (candidate.otherShapeId == record.otherShapeId) {
                entry = &candidate;
                break;
            }
        if (!entry) {
            info->contacts.push_back(ShapeContactEntry{
                record.otherShapeId, {}, {}, {}, {}, {}});
            entry = &info->contacts.back();
        }
        entry->myShapeIds.push_back(record.myShapeId);
        entry->normals.push_back(record.normalSign * wm.normal);
        entry->normalSigns.push_back(record.normalSign);
        entry->points.push_back(cp);
        entry->contacts.push_back(contact);
    }
    // RETAIL PPC VERIFIED: getShapeCollisionInfo searches this int-keyed
    // red-black tree with signed comparisons (0x2f52d0); getFirstShape...
    // returns its leftmost value (0x2dbec), and getCollidingShapeIDList walks
    // it in-order via 0x2f5228. Sort outer groups by signed shape ID. Inner
    // records retain the existing traversal order, the closest local analogue
    // to the retail insertion-ordered per-shape vector.
    std::sort(info->contacts.begin(), info->contacts.end(),
              [](const ShapeContactEntry& a, const ShapeContactEntry& b) {
                  return a.otherShapeId < b.otherShapeId;
              });
    return info;
}

gmVariable GetShapeCollisionInfoView(gmMachine& a_machine,
                                     CollisionInfoData* a_info,
                                     ShapeContactEntry& a_entry,
                                     const gmVariable& a_owner) {
    auto cached = a_info->shapeCollisionInfoViews.find(a_entry.otherShapeId);
    if (cached != a_info->shapeCollisionInfoViews.end()) {
        gmObject* object = cached->second.m_type == g_shapeCollisionInfoType
            ? a_machine.GetObject(cached->second.m_value.m_ref) : nullptr;
        auto* user = object ? static_cast<gmUserObject*>(object) : nullptr;
        auto* sci = user ? static_cast<ShapeCollisionInfoData*>(user->m_user) : nullptr;
        if (sci) {
            sci->myShapeIds = a_entry.myShapeIds;
            sci->normals = a_entry.normals;
            sci->normalSigns = a_entry.normalSigns;
            sci->points = a_entry.points;
            sci->contacts = a_entry.contacts;
            return cached->second;
        }
        UnrootGmVariable(a_machine, cached->second);
        a_info->shapeCollisionInfoViews.erase(cached);
    }

    auto* sci = new ShapeCollisionInfoData{
        a_entry.otherShapeId, a_entry.myShapeIds, a_entry.normals,
        a_entry.normalSigns, a_entry.points, a_entry.contacts};
    sci->machine = &a_machine;
    sci->ownerCollisionInfo = a_owner;
    AcquireCollisionInfoRoot(a_machine, a_owner);
    gmVariable view(a_machine.AllocUserObject(sci, g_shapeCollisionInfoType));
    RootGmVariable(a_machine, view); // CollisionInfo owns this mapped value.
    a_info->shapeCollisionInfoViews.emplace(a_entry.otherShapeId, view);
    return view;
}

namespace {
void CopyCollisionInfoContents(CollisionInfoData* a_destination,
                               CollisionInfoData* a_source) {
    if (!a_destination || !a_source) return;
    a_destination->contacts = std::move(a_source->contacts);
    a_destination->otherEntity = a_source->otherEntity;
    a_destination->pairExists = a_source->pairExists;
    a_destination->physicsDataKey = a_source->physicsDataKey;
    a_destination->otherEntityId = a_source->otherEntityId;
    a_destination->invalidated = a_source->invalidated;
    for (auto it = a_destination->shapeCollisionInfoViews.begin();
         it != a_destination->shapeCollisionInfoViews.end();) {
        const bool keyStillPresent = std::any_of(
            a_destination->contacts.begin(), a_destination->contacts.end(),
            [it](const ShapeContactEntry& entry) {
                return entry.otherShapeId == it->first;
            });
        if (!keyStillPresent) {
            if (a_destination->machine &&
                !a_destination->machine->IsResetting())
                UnrootGmVariable(*a_destination->machine, it->second);
            it = a_destination->shapeCollisionInfoViews.erase(it);
        } else {
            ++it;
        }
    }
}

bool RefreshCollisionInfoData(gmMachine& a_machine, CollisionInfoData* a_info) {
    if (!a_info || a_info->invalidated || !a_info->pairExists ||
        a_info->physicsDataKey == 0)
        return false;
    auto ownerIt = std::find_if(g_allPhysicsData.begin(), g_allPhysicsData.end(),
        [a_info](PhysicsDataObj* a_data) {
            return reinterpret_cast<uintptr_t>(a_data) == a_info->physicsDataKey;
        });
    if (ownerIt == g_allPhysicsData.end()) {
        InvalidatePhysicsCollisionInfoView(a_info->physicsDataKey,
                                           a_info->otherEntityId);
        return false;
    }
    CollisionInfoData* fresh = BuildCollisionInfo(a_machine, *ownerIt,
                                                   a_info->otherEntity);
    const bool stillActive = fresh->pairExists;
    if (stillActive) {
        CopyCollisionInfoContents(a_info, fresh);
    } else {
        ReleaseShapeCollisionInfoViews(a_info);
        a_info->contacts.clear();
        a_info->pairExists = false;
        a_info->invalidated = true;
        a_info->physicsDataKey = 0;
    }
    delete fresh;
    return stillActive;
}

void RefreshShapeCollisionInfoData(ShapeCollisionInfoData* a_info) {
    if (!a_info || !a_info->machine ||
        a_info->ownerCollisionInfo.m_type != g_collisionInfoType)
        return;
    gmObject* object = a_info->machine->GetObject(
        a_info->ownerCollisionInfo.m_value.m_ref);
    if (!object) return;
    auto* parent = static_cast<CollisionInfoData*>(
        static_cast<gmUserObject*>(object)->m_user);
    if (!RefreshCollisionInfoData(*a_info->machine, parent)) {
        a_info->myShapeIds.clear();
        a_info->normals.clear();
        a_info->normalSigns.clear();
        a_info->points.clear();
        a_info->contacts.clear();
        return;
    }
    auto group = std::find_if(parent->contacts.begin(), parent->contacts.end(),
        [a_info](const ShapeContactEntry& a_entry) {
            return a_entry.otherShapeId == a_info->otherShapeId;
        });
    if (group == parent->contacts.end()) {
        a_info->myShapeIds.clear();
        a_info->normals.clear();
        a_info->normalSigns.clear();
        a_info->points.clear();
        a_info->contacts.clear();
        return;
    }
    a_info->myShapeIds = group->myShapeIds;
    a_info->normals = group->normals;
    a_info->normalSigns = group->normalSigns;
    a_info->points = group->points;
    a_info->contacts = group->contacts;
}
} // namespace

void InvalidatePhysicsCollisionInfoViewImpl(uintptr_t a_physicsData,
                                            uint32_t a_otherEntityId) {
    auto ownerIt = std::find_if(g_allPhysicsData.begin(), g_allPhysicsData.end(),
        [a_physicsData](PhysicsDataObj* a_data) {
            return reinterpret_cast<uintptr_t>(a_data) == a_physicsData;
        });
    if (ownerIt == g_allPhysicsData.end()) return;
    PhysicsDataObj* owner = *ownerIt;
    auto viewIt = owner->collisionInfoViews.find(a_otherEntityId);
    if (viewIt == owner->collisionInfoViews.end()) return;
    gmVariable view = viewIt->second;
    gmMachine* machine = GetEngineContext().machine;
    if (machine && view.m_type == g_collisionInfoType) {
        // During ResetAndFreeMemory the collector calls native destructors
        // in arbitrary object order. The cached GM object's payload may
        // already have been destroyed, and the machine is discarding every
        // view anyway, so only perform live-view invalidation during normal
        // runtime teardown.
        if (!machine->IsResetting()) {
            gmObject* object = machine->GetObject(view.m_value.m_ref);
            if (object) {
                auto* info = static_cast<CollisionInfoData*>(
                    static_cast<gmUserObject*>(object)->m_user);
                if (info) {
                    ReleaseShapeCollisionInfoViews(info);
                    info->contacts.clear();
                    info->pairExists = false;
                    info->invalidated = true;
                    info->physicsDataKey = 0;
                }
            }
        }
        ReleaseCollisionInfoRoot(*machine, view);
    }
    owner->collisionInfoViews.erase(viewIt);
}

int GM_CDECL gmf_pd_getCollisionInfo(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    CollisionInfoData* info = BuildCollisionInfo(m, d, a_thread->Param(0));
    // RETAIL PPC VERIFIED: Physics.getCollisionInfo (0x8bb5c) calls
    // 0x31544, which confirms an exact active-pair map key with 0x2f6348
    // before retrieving the mapped CollisionInfo value through 0x2f902c.
    // The latter is a get-or-insert helper, but this getter invokes it only
    // after the exact-key check, so a miss returns null without creating a
    // record. Desktop builds the value from persistent contact-add/remove
    // records; native object identity and exact PhysicsData-key ordering are
    // still represented by an adapter view. The view cache preserves the
    // existing mapped-value identity while this directed pair is active.
    // An allocated but
    // empty CollisionInfo is truthy to GM, so returning one here made every
    // `if (?data.getCollisionInfo(other))` path claim a collision after the
    // bodies had separated.
    if (!info->pairExists) {
        delete info;
        a_thread->PushNull();
        return GM_OK;
    }
    auto cached = d->collisionInfoViews.find(info->otherEntityId);
    if (cached != d->collisionInfoViews.end()) {
        gmObject* object = cached->second.m_type == g_collisionInfoType
            ? m.GetObject(cached->second.m_value.m_ref) : nullptr;
        auto* existing = object
            ? static_cast<CollisionInfoData*>(static_cast<gmUserObject*>(object)->m_user)
            : nullptr;
        if (existing && !existing->invalidated) {
            CopyCollisionInfoContents(existing, info);
            delete info;
            a_thread->Push(cached->second);
            return GM_OK;
        }
        ReleaseCollisionInfoRoot(m, cached->second);
        d->collisionInfoViews.erase(cached);
    }
    gmUserObject* obj = m.AllocUserObject(info, g_collisionInfoType);
    gmVariable view(obj);
    AcquireCollisionInfoRoot(m, view);
    d->collisionInfoViews.emplace(info->otherEntityId, view);
    a_thread->Push(view);
    return GM_OK;
}
int GM_CDECL gmf_pd_isColliding(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    const int argc = a_thread->GetNumParams();
    if (argc > 1) return GM_EXCEPTION;
    // RETAIL PPC VERIFIED: Physics.isColliding (0x8b9c4) returns whether the
    // PhysicsData pair map count at +8 is nonzero with no argument; with one
    // Entity argument it reads the Entity's +4 ID and calls 0x2f6348 for that
    // exact unsigned outer-map key. Desktop uses the retained directed pair
    // map. Empty pair records remain visible while onEntitySeparate runs,
    // matching native erase-helper order.
    const gmVariable target = argc == 1 ? a_thread->Param(0) : gmVariable::s_null;
    bool hasTargetEntityId = false;
    uint32_t targetEntityId = 0;
    if (target.m_type == GM_TABLE) {
        gmTableObject* targetTable = static_cast<gmTableObject*>(
            a_thread->GetMachine()->GetObject(target.m_value.m_ref));
        if (targetTable) {
            const gmVariable id = targetTable->Get(a_thread->GetMachine(), "__objectID");
            if (id.m_type == GM_INT) {
                targetEntityId = static_cast<uint32_t>(id.m_value.m_int);
                hasTargetEntityId = true;
            }
        }
    }
    const uintptr_t myData = reinterpret_cast<uintptr_t>(d);
    const bool colliding = argc == 0
        ? PhysicsDataHasAnyCollisionPairs(myData)
        : hasTargetEntityId && PhysicsDataPairIsColliding(myData, targetEntityId);
    a_thread->PushInt(colliding ? 1 : 0);
    return GM_OK;
}
// RETAIL PPC VERIFIED: CollisionInfo.isBodyColliding (0x9778c) calls
// 0x2dd04. Retail walks the supplied PhysicsBody's shape IDs and checks each
// against both the CollisionInfo outer shape key and every record's my-shape
// ID. Mirror that ID comparison directly instead of caching Box2D body
// pointers at CollisionInfo construction time.
int GM_CDECL gmf_ci_isBodyColliding(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    GM_CHECK_NUM_PARAMS(1);
    PhysicsBodyData* b = GetBodyData(*a_thread->GetMachine(), a_thread->Param(0));
    bool found = false;
    if (b && b->body) {
        for (b2Fixture* fixture = b->body->GetFixtureList(); fixture && !found;
             fixture = fixture->GetNext()) {
            const int shapeId = static_cast<int>(fixture->GetUserData().pointer);
            for (const ShapeContactEntry& entry : info->contacts) {
                if (entry.otherShapeId == shapeId ||
                    std::find(entry.myShapeIds.begin(), entry.myShapeIds.end(), shapeId) !=
                        entry.myShapeIds.end()) {
                    found = true;
                    break;
                }
            }
        }
    }
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
// RETAIL PPC VERIFIED: CollisionInfo.isBodyCollidingWithShape
// (0x97920 -> 0x2df38) tests each outer shape ID and each contact's +8 shape
// ID. PPC sign-extends the raw second argument before the helper call. It
// returns true when either ID matches the explicit shape and the opposite ID
// belongs to the supplied body, including either orientation.
// 0x33b5c checks body ownership by walking its shape-ID list. It does not
// recheck whether the native contact is still touching.
int GM_CDECL gmf_ci_isBodyCollidingWithShape(gmThread* a_thread) {
    auto* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    GM_CHECK_NUM_PARAMS(2);
    PhysicsBodyData* body = GetBodyData(*a_thread->GetMachine(), a_thread->Param(0));
    const int shapeId = a_thread->Param(1).m_value.m_int;
    bool found = false;
    auto bodyContains = [body](int id) {
        if (!body || !body->body) return false;
        b2Fixture* fixture = GetFixtureByGlobalId(id);
        return fixture && fixture->GetBody() == body->body;
    };
    for (const ShapeContactEntry& entry : info->contacts) {
        const bool outerMatches = bodyContains(entry.otherShapeId) ||
            entry.otherShapeId == shapeId;
        for (int myShapeId : entry.myShapeIds) {
            const bool innerMatches = myShapeId == shapeId || bodyContains(myShapeId);
            if (outerMatches && innerMatches) {
                found = true;
                break;
            }
        }
        if (found) break;
    }
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
// RETAIL PPC VERIFIED: CollisionInfo.isBodyCollidingWithBody (0x97850 ->
// 0x2ddb8) checks each pair record if either side's shape ID belongs to
// either supplied PhysicsBody. Shape ownership is established by global
// fixture IDs, matching the retail PhysicsBody shape-ID walk in 0x33b5c.
int GM_CDECL gmf_ci_isBodyCollidingWithBody(gmThread* a_thread) {
    auto* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& machine = *a_thread->GetMachine();
    PhysicsBodyData* bodyA = GetBodyData(machine, a_thread->Param(0));
    PhysicsBodyData* bodyB = GetBodyData(machine, a_thread->Param(1));
    auto bodyContains = [](PhysicsBodyData* body, int id) {
        if (!body || !body->body) return false;
        b2Fixture* fixture = GetFixtureByGlobalId(id);
        return fixture && fixture->GetBody() == body->body;
    };
    bool found = false;
    for (const ShapeContactEntry& entry : info->contacts) {
        const bool outerMatches = bodyContains(bodyA, entry.otherShapeId) ||
            bodyContains(bodyB, entry.otherShapeId);
        if (!outerMatches) continue;
        for (int myShapeId : entry.myShapeIds) {
            if (bodyContains(bodyA, myShapeId) || bodyContains(bodyB, myShapeId)) {
                found = true;
                break;
            }
        }
        if (found) break;
    }
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
// RETAIL PPC VERIFIED: CollisionInfo.isShapeColliding (0x976cc -> 0x2dc14)
// sign-extends the raw shape-ID payload, then scans ordered outer keys and
// 16-byte contact records, returning true if the ID occurs on either side.
int GM_CDECL gmf_ci_isShapeColliding(gmThread* a_thread) {
    auto* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    GM_CHECK_NUM_PARAMS(1);
    const int shapeId = a_thread->Param(0).m_value.m_int;
    bool found = false;
    for (const ShapeContactEntry& entry : info->contacts) {
        if (entry.otherShapeId == shapeId ||
            std::find(entry.myShapeIds.begin(), entry.myShapeIds.end(), shapeId) != entry.myShapeIds.end()) {
            found = true;
            break;
        }
    }
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
// Retail PPC verified: wrapper 0x979f0 allocates or reuses an
// INTLISTREFCONTAINER, clears/reinitializes it via 0x31c3f8 when reused, then
// fills it with signed-key in-order tree traversal through 0x2d86c. The local
// vector is that copied ID list; INTLISTREFCONTAINER's five exposed methods
// match their registered retail method-table cursor/value/count behavior.
int GM_CDECL gmf_ci_getCollidingShapeIDList(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    gmMachine& m = *a_thread->GetMachine();
    IntIterData* it = nullptr;
    gmVariable result;
    if (a_thread->GetNumParams() == 0) {
        it = new IntIterData();
        gmUserObject* obj = m.AllocUserObject(it, g_intIterType);
        result = gmVariable(obj);
    } else {
        // Retail reuses the provided IntListRefContainer, clearing and
        // repopulating it with ordered collision-map keys.
        result = a_thread->Param(0);
        if (result.m_type != g_intIterType) return GM_EXCEPTION;
        gmObject* obj = m.GetObject(result.m_value.m_ref);
        if (!obj) return GM_EXCEPTION;
        it = (IntIterData*)((gmUserObject*)obj)->m_user;
        if (!it) return GM_EXCEPTION;
        it->values.clear();
        it->index = 0;
    }
    for (auto& e : info->contacts) it->values.push_back(e.otherShapeId);
    a_thread->Push(result);
    return GM_OK;
}
// Retail PPC verified: Physics.getShapeCollisionInfo (0x974cc) sign-extends
// the raw argument payload, searches by signed key through 0x2db88/0x2f52d0,
// and returns null on a miss. A hit is the stored ShapeCollisionInfo pointer
// at tree-node +0x10. Cache one rooted proxy per parent/key so repeated lookup
// returns the same GM object and its fields refresh from the live contact group.
int GM_CDECL gmf_ci_getShapeCollisionInfo(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    GM_CHECK_NUM_PARAMS(1);
    const int sid = a_thread->Param(0).m_value.m_int;
    for (auto& e : info->contacts) {
        if (e.otherShapeId == sid) {
            const gmVariable owner = a_thread->GetThis()
                ? *a_thread->GetThis() : gmVariable::s_null;
            gmVariable view = GetShapeCollisionInfoView(
                *a_thread->GetMachine(), info, e, owner);
            a_thread->Push(view);
            return GM_OK;
        }
    }
    a_thread->PushNull();
    return GM_OK;
}
// Retail wrapper 0x975d4 delegates to 0x2dbec, which returns null for an
// empty map or the stored ShapeCollisionInfo pointer from the leftmost entry.
// Desktop sorts outer groups to preserve signed-key order; use the same cached
// view as getShapeCollisionInfo so both retail accessors alias one map value.
int GM_CDECL gmf_ci_getFirstShapeCollisionInfo(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    if (info->contacts.empty()) { a_thread->PushNull(); return GM_OK; }
    ShapeContactEntry& e = info->contacts.front();
    const gmVariable owner = a_thread->GetThis()
        ? *a_thread->GetThis() : gmVariable::s_null;
    gmVariable view = GetShapeCollisionInfoView(
        *a_thread->GetMachine(), info, e, owner);
    a_thread->Push(view);
    return GM_OK;
}
int GM_CDECL gmf_sci_disableContact(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    RefreshShapeCollisionInfoData(sci);
    GM_CHECK_NUM_PARAMS(1);
    const int32_t otherShapeId = a_thread->Param(0).m_value.m_int;
    if (sci->otherShapeId == otherShapeId) {
        for (b2Contact* contact : sci->contacts)
            MarkContactDisabled(contact);
    }
    return GM_OK;
}
// RETAIL PPC VERIFIED: ShapeCollisionInfo.getNumOfMyShapesColliding
// (0x97f34 -> 0x2d544) returns the record vector's 16-byte element count.
// Desktop's parallel myShapeIds vector has one element per record.
int GM_CDECL gmf_sci_getNumOfMyShapesColliding(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    RefreshShapeCollisionInfoData(sci);
    a_thread->PushInt((int)sci->myShapeIds.size());
    return GM_OK;
}
// RETAIL PPC VERIFIED: ShapeCollisionInfo.getMyContactShapeID
// (0x97fdc -> 0x2d56c) sign-extends the raw first argument and returns record
// +8 for a nonnegative in-range index, or -1 otherwise. The wrapper does not
// type-convert its argument; read the same raw GM payload here.
int GM_CDECL gmf_sci_getMyContactShapeID(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    RefreshShapeCollisionInfoData(sci);
    GM_CHECK_NUM_PARAMS(1);
    const int idx = a_thread->Param(0).m_value.m_int;
    a_thread->PushInt((idx >= 0 && idx < (int)sci->myShapeIds.size()) ? sci->myShapeIds[idx] : -1);
    return GM_OK;
}
int GM_CDECL gmf_sci_getNumContacts(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    RefreshShapeCollisionInfoData(sci);
    GM_CHECK_NUM_PARAMS(1);
    // Retail sign-extends the raw argument payload before calling 0x2d5bc.
    const int idx = a_thread->Param(0).m_value.m_int;
    int count = (idx >= 0 && idx < (int)sci->points.size()) ? sci->points[idx].count : 0;
    if (idx >= 0 && idx < (int)sci->contacts.size()) {
        int liveCount = 0;
        if (GetPhysicsContactManifoldPointCount(sci->contacts[idx], liveCount))
            count = liveCount;
    }
    a_thread->PushInt(count);
    return GM_OK;
}
// RETAIL PPC VERIFIED: getContactNormal (0x98144 -> 0x2d71c -> 0x151038)
// reads the live contact and record orientation sign, rebuilds its world
// manifold, and returns the signed normal. Preserve the sign and query an
// active contact at call time; cached normal is only a stale-view fallback.
int GM_CDECL gmf_sci_getContactNormal(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    RefreshShapeCollisionInfoData(sci);
    GM_CHECK_NUM_PARAMS(1);
    const int idx = a_thread->Param(0).m_value.m_int;
    b2Vec2 n = (idx >= 0 && idx < (int)sci->normals.size()) ? sci->normals[idx] : b2Vec2(0.0f, 0.0f);
    if (idx >= 0 && idx < (int)sci->contacts.size() &&
        idx < (int)sci->normalSigns.size()) {
        float x = 0.0f, y = 0.0f;
        if (GetPhysicsContactWorldNormal(sci->contacts[idx], sci->normalSigns[idx], x, y))
            n.Set(x, y);
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), n.x, n.y));
    return GM_OK;
}
int GM_CDECL gmf_sci_getContactPos(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    RefreshShapeCollisionInfoData(sci);
    GM_CHECK_NUM_PARAMS(2);
    // Retail sign-extends both raw GM argument payloads before the helper call.
    const int myIdx = a_thread->Param(0).m_value.m_int;
    const int ptIdx = a_thread->Param(1).m_value.m_int;
    b2Vec2 p(0.0f, 0.0f);
    if (myIdx >= 0 && myIdx < (int)sci->points.size() && ptIdx >= 0) {
        if (myIdx < (int)sci->contacts.size()) {
            int liveCount = 0;
            if (GetPhysicsContactManifoldPointCount(sci->contacts[myIdx], liveCount)) {
                if (ptIdx < liveCount && ptIdx < b2_maxManifoldPoints) {
                    float x = 0.0f, y = 0.0f;
                    if (GetPhysicsContactWorldPoint(sci->contacts[myIdx], ptIdx, x, y))
                        p.Set(x, y);
                }
            } else {
                // The returned ShapeCollisionInfo may outlive its contact.
                // Preserve the copied value without touching a stale pointer.
                const ContactPoints& cp = sci->points[myIdx];
                if (ptIdx < cp.count && ptIdx < 2) p = cp.pts[ptIdx];
            }
        } else {
            const ContactPoints& cp = sci->points[myIdx];
            if (ptIdx < cp.count && ptIdx < 2) p = cp.pts[ptIdx];
        }
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), p.x, p.y));
    return GM_OK;
}
// Retail method-table entry INTLISTREFCONTAINER.isValid resolves to PPC
// 0x31e3d0. It tests the initialized flag and compares the current node with
// the backing list sentinel; index<size has the same result for this copied list.
int GM_CDECL gmf_intIter_isValid(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    a_thread->PushInt((it && it->index < it->values.size()) ? 1 : 0);
    return GM_OK;
}
// Retail INTLISTREFCONTAINER.goToNext at 0x31e47c follows current.next on a
// circular sentinel list. The vector cursor's modulo size+1 models the same
// first-to-end-to-first cycle, including the empty list.
int GM_CDECL gmf_intIter_goToNext(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    if (it) it->index = (it->index + 1) % (it->values.size() + 1);
    return GM_OK;
}
// Retail INTLISTREFCONTAINER.get at 0x31e5a0 returns the current node value,
// or GM null when the cursor is at the sentinel. Local copied integer nodes
// expose the same value and end result.
int GM_CDECL gmf_intIter_get(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    if (it && it->index < it->values.size()) a_thread->PushInt(it->values[it->index]);
    else a_thread->PushNull();
    return GM_OK;
}
// Retail INTLISTREFCONTAINER.reset at 0x31e4c4 resets the cursor to the
// backing list's first node; local snapshot index zero is the same position.
int GM_CDECL gmf_intIter_reset(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    if (it) it->index = 0; // retail reset follows the sentinel's first link
    return GM_OK;
}
// Retail INTLISTREFCONTAINER.size at 0x31e50c returns the backing list count;
// this container owns a copied ID list, represented locally by vector length.
int GM_CDECL gmf_intIter_size(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    a_thread->PushInt(it ? (gmint)it->values.size() : 0);
    return GM_OK;
}
// RETAIL PPC VERIFIED: `Physics.isAllSensors` at 0x97c8c calls 0x2e08c,
// which walks CollisionInfo's outer other-shape IDs and queries each current
// sensor byte through 0x36eec; an empty map returns true. 0x36eec defaults to
// true if the shape ID no longer resolves. Resolve IDs at query time, matching
// retail's live lookup rather than caching sensor state at snapshot creation.
int GM_CDECL gmf_ci_isAllSensors(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    bool allSensors = true;
    for (const ShapeContactEntry& entry : info->contacts) {
        b2Fixture* fixture = GetFixtureByGlobalId(entry.otherShapeId);
        if (fixture && !fixture->IsSensor()) {
            allSensors = false;
            break;
        }
    }
    a_thread->PushInt(allSensors ? 1 : 0);
    return GM_OK;
}
// RETAIL PPC VERIFIED: Physics.isMyShapeCollidingWithSensorsOnly (0x97d40)
// passes the requested shape ID and CollisionInfo receiver to 0x2e138. The
// helper checks only contact records whose +8 field is this shape ID, then
// tests each record's other shape sensor byte through 0x36eec. No matching
// records returns true. Keep this per-shape query distinct from isAllSensors.
int GM_CDECL gmf_ci_isMyShapeCollidingWithSensorsOnly(gmThread* a_thread) {
    auto* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    if (a_thread->GetNumParams() < 1) return GM_EXCEPTION;

    // Retail forwards the raw 32-bit GM value to the helper without type
    // conversion; extra arguments are ignored by the wrapper.
    const int myShapeId = a_thread->Param(0).m_value.m_int;
    bool allSensors = true;
    for (const ShapeContactEntry& entry : info->contacts) {
        for (int contactShapeId : entry.myShapeIds) {
            if (contactShapeId != myShapeId) continue;
            // Retail queries the current other-shape sensor flag by outer
            // shape ID; it does not dereference the retained contact record.
            b2Fixture* otherFixture = GetFixtureByGlobalId(entry.otherShapeId);
            if (otherFixture && !otherFixture->IsSensor()) {
                allSensors = false;
                break;
            }
        }
        if (!allSensors) break;
    }
    a_thread->PushInt(allSensors ? 1 : 0);
    return GM_OK;
}
// Retail Physics.getNumCollidingShapes (0x97420) returns the CollisionInfo
// shape-group count stored at receiver +0x14. BuildCollisionInfo keeps one
// contacts entry per other-shape ID, the same groups exposed by
// getCollidingShapeIDList() and getShapeCollisionInfo(shapeId).
int GM_CDECL gmf_ci_getNumCollidingShapes(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    a_thread->PushInt((int)info->contacts.size());
    return GM_OK;
}
// RETAIL PPC VERIFIED: CollisionInfo.getEntity (0x97e00) reads the first
// PhysicsData pointer in its pair record and converts PhysicsData+0x98 to the
// owning Entity. The contact callback inserts a directed record in A's
// CollisionInfo map with B's PhysicsData first and A's second (0x374e0 ->
// 0x30f28 -> 0x2e3c4), so this returns the other/colliding entity.
int GM_CDECL gmf_ci_getEntity(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    a_thread->Push(info->otherEntity);
    return GM_OK;
}
// RETAIL PPC VERIFIED: CollisionInfo.getContactNormalOfMyCollidingShape
// (0x97e64) sign-extends the raw first payload and calls helper 0x2e278 with
// that my-shape ID. The helper walks signed-key ordered other-shape groups and
// returns the first matching record normal, defaulting to (0,1). Desktop
// preserves the same group and listener order; runtime normal direction and
// equal-ID contact sequence still need a matched capture.
int GM_CDECL gmf_ci_getContactNormalOfMyCollidingShape(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    RefreshCollisionInfoData(*a_thread->GetMachine(), info);
    GM_CHECK_NUM_PARAMS(1);
    const int shapeId = a_thread->Param(0).m_value.m_int;
    b2Vec2 normal(0.0f, 1.0f);
    bool foundContact = false;
    for (const ShapeContactEntry& entry : info->contacts) {
        for (size_t i = 0; i < entry.myShapeIds.size() && i < entry.normals.size(); ++i) {
            if (entry.myShapeIds[i] == shapeId) {
                normal = entry.normals[i];
                foundContact = true;
                break;
            }
        }
        if (foundContact) break;
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), normal.x, normal.y));
    return GM_OK;
}
gmFunctionEntry s_collisionInfoLib[] = {
    {"getEntity", gmf_ci_getEntity, nullptr},
    {"isBodyColliding", gmf_ci_isBodyColliding, nullptr},
    {"isBodyCollidingWithBody", gmf_ci_isBodyCollidingWithBody, nullptr},
    {"isBodyCollidingWithShape", gmf_ci_isBodyCollidingWithShape, nullptr},
    {"isShapeColliding", gmf_ci_isShapeColliding, nullptr},
    {"getNumCollidingShapes", gmf_ci_getNumCollidingShapes, nullptr},
    {"getCollidingShapeIDList", gmf_ci_getCollidingShapeIDList, nullptr},
    {"getShapeCollisionInfo", gmf_ci_getShapeCollisionInfo, nullptr},
    {"getFirstShapeCollisionInfo", gmf_ci_getFirstShapeCollisionInfo, nullptr},
    {"isAllSensors", gmf_ci_isAllSensors, nullptr},
    {"isMyShapeCollidingWithSensorsOnly", gmf_ci_isMyShapeCollidingWithSensorsOnly, nullptr},
    {"getContactNormalOfMyCollidingShape", gmf_ci_getContactNormalOfMyCollidingShape, nullptr},
};
gmFunctionEntry s_shapeCollisionInfoLib[] = {
    {"getNumOfMyShapesColliding", gmf_sci_getNumOfMyShapesColliding, nullptr},
    {"getMyContactShapeID", gmf_sci_getMyContactShapeID, nullptr},
    {"getContactNormal", gmf_sci_getContactNormal, nullptr},
    {"getNumContacts", gmf_sci_getNumContacts, nullptr},
    {"getContactPos", gmf_sci_getContactPos, nullptr},
    {"disableContact", gmf_sci_disableContact, nullptr},
};
gmFunctionEntry s_intIterLib[] = {
    {"get", gmf_intIter_get, nullptr},
    {"isValid", gmf_intIter_isValid, nullptr},
    {"goToNext", gmf_intIter_goToNext, nullptr},
    {"reset", gmf_intIter_reset, nullptr},
    {"size", gmf_intIter_size, nullptr},
};

int GM_CDECL gmf_pd_getPos(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    gmMachine& m = *a_thread->GetMachine();
    if (!b) { a_thread->Push(MakeVector2D(m, 0.0f, 0.0f)); return GM_OK; }
    b2Vec2 p = b->body->GetPosition();
    a_thread->Push(MakeVector2D(m, p.x, p.y));
    return GM_OK;
}
int GM_CDECL gmf_pd_getPosRef(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (!b) return gmf_pd_getPos(a_thread);
    a_thread->Push(MakeVector2DRef(*a_thread->GetMachine(), b->selfVar, b,
                                   [](void* ctx, float& x, float& y) {
                                       b2Vec2 p = static_cast<PhysicsBodyData*>(ctx)->body->GetPosition();
                                       x = p.x; y = p.y;
                                   },
                                   [](void* ctx, float x, float y) {
                                       auto* body = static_cast<PhysicsBodyData*>(ctx)->body;
                                       body->SetTransform(b2Vec2(x, y), body->GetAngle());
                                   }));
    return GM_OK;
}
int GM_CDECL gmf_pd_setPos(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->body->SetTransform(b2Vec2(x, y), b->body->GetAngle());
    return GM_OK;
}
int GM_CDECL gmf_pd_getVel(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    gmMachine& m = *a_thread->GetMachine();
    if (!b) { a_thread->Push(MakeVector2D(m, 0.0f, 0.0f)); return GM_OK; }
    b2Vec2 v = ScriptVelocity(b->body->GetLinearVelocity());
    a_thread->Push(MakeVector2D(m, v.x, v.y));
    return GM_OK;
}
int GM_CDECL gmf_pd_setVel(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->body->SetLinearVelocity(Box2DVelocity(b2Vec2(x, y)));
    return GM_OK;
}
int GM_CDECL gmf_pd_getSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? ScriptVelocity(b->body->GetLinearVelocity()).Length() : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_pd_setSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(speed, 0, 0.0f);
    float simSpeed = speed;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        b2Vec2 v = ScriptVelocity(b->body->GetLinearVelocity());
        float len = v.Length();
        if (v.x == 0.0f && v.y == 0.0f) {
            v = simSpeed * BodyFacingDirection(*b);
        } else if (len >= 1.1920928955078125e-7f) {
            v *= (simSpeed / len);
        } else {
            v *= simSpeed;
        }
        b->body->SetLinearVelocity(Box2DVelocity(v));
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_getDir(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    gmMachine& m = *a_thread->GetMachine();
    if (!b) { a_thread->Push(MakeVector2D(m, 1.0f, 0.0f)); return GM_OK; }
    b2Vec2 v = BodyFacingDirection(*b);
    a_thread->Push(MakeVector2D(m, v.x, v.y));
    return GM_OK;
}
int GM_CDECL gmf_pd_setDir(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        b2Vec2 dir(x, y);
        float dlen = dir.Length();
        if (dlen > 0.0f) dir *= (1.0f / dlen);
        else dir.SetZero();
        if (b->body->IsFixedRotation()) {
            b->dirX = dir.x;
            b->dirY = dir.y;
        } else {
            b->body->SetTransform(b->body->GetPosition(), std::atan2(dir.y, dir.x));
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_getRadius(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2AABB bounds;
    float radius = 0.0f;
    if (b && GetBodyLocalBounds(b->body, bounds)) {
        radius = 0.5f * std::max(bounds.upperBound.x - bounds.lowerBound.x,
                                 bounds.upperBound.y - bounds.lowerBound.y);
    }
    a_thread->PushFloat(radius);
    return GM_OK;
}
int GM_CDECL gmf_pd_setAsCircle(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(radius, 0, 0.0f);
    float r = std::max(radius, 0.001f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (!b) return GM_OK;
    b2Filter filter;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = DesiredFixtureFilter(b, f); haveFilter = true; }
        b->savedCollidableFilters.erase(f);
        UnregisterShapeGlobalId(f);
        b->body->DestroyFixture(f);
        f = next;
    }
    b2CircleShape circle;
    circle.m_radius = r;
    b2FixtureDef def;
    def.shape = &circle;
    def.density = 1.0f;
    def.filter = filter;
    b2Fixture* fx = b->body->CreateFixture(&def);
    fx->SetSensor(b->isSensor);
    TrackNewFixture(b, fx);
    RegisterShapeGlobalId(fx);
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
int GM_CDECL gmf_pd_isCircle(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt((f && f->GetShape()->GetType() == b2Shape::e_circle) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_isRect(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt((f && f->GetShape()->GetType() == b2Shape::e_polygon) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_getWidth(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2AABB aabb;
    if (!b || !GetBodyLocalBounds(b->body, aabb)) { a_thread->PushFloat(0.0f); return GM_OK; }
    a_thread->PushFloat(aabb.upperBound.x - aabb.lowerBound.x);
    return GM_OK;
}
int GM_CDECL gmf_pd_getHeight(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2AABB aabb;
    if (!b || !GetBodyLocalBounds(b->body, aabb)) { a_thread->PushFloat(0.0f); return GM_OK; }
    a_thread->PushFloat(aabb.upperBound.y - aabb.lowerBound.y);
    return GM_OK;
}
int GM_CDECL gmf_pd_setIsSensor(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    gmMachine& machine = *a_thread->GetMachine();
    for (const gmVariable& body : d->bodies) {
        PhysicsBodyData* b = GetBodyData(machine, body);
        if (!b || !b->body) continue;
        b->isSensor = (v != 0);
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext())
            f->SetSensor(b->isSensor);
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_setMaxSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->maxSpeed = v;
    return GM_OK;
}
int GM_CDECL gmf_pd_thrust(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b && !b->thrustFlag) { b->thrustFlag = true; b->thrustAccum += b->thrustAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pd_doesRotationResponse(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushInt((b && !b->body->IsFixedRotation()) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_setVelocityDecayRate(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->velocityDecayRate = ComputeDecayFactor(v);
    return GM_OK;
}
int GM_CDECL gmf_pd_reverseThrust(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b && !b->reverseFlag) { b->reverseFlag = true; b->thrustAccum -= b->thrustAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pd_brake(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->brakeFlag = true;
    return GM_OK;
}
int GM_CDECL gmf_pd_rotateLeft(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b && !b->rotLeftFlag) { b->rotLeftFlag = true; b->rotAccum += b->rotateAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pd_rotateRight(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b && !b->rotRightFlag) { b->rotRightFlag = true; b->rotAccum -= b->rotateAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pd_rotateTo(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    float x, y;
    if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y)) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    PhysicsBodyData* b = d->bodies.empty() ? nullptr : GetBodyData(m, d->bodies[0]);
    if (b) {
        b->rotateToActive = true;
        b->rotateToX = x;
        b->rotateToY = y;
        b->rotateToSign = (int)a_thread->ParamFloatOrInt(1, 0.0f);
    }
    d->rotateToDirty = true;
    return GM_OK;
}
int GM_CDECL gmf_pd_stopRotatingTo(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->rotateToActive = false;
    return GM_OK;
}
int GM_CDECL gmf_pd_setAsRect(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(width, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(height, 1, 0.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (!b) return GM_OK;
    float hx = std::max(width * 0.5f, 0.001f), hy = std::max(height * 0.5f, 0.001f);
    b2Filter filter;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = DesiredFixtureFilter(b, f); haveFilter = true; }
        b->savedCollidableFilters.erase(f);
        UnregisterShapeGlobalId(f);
        b->body->DestroyFixture(f);
        f = next;
    }
    b2PolygonShape box;
    box.SetAsBox(hx, hy);
    b2FixtureDef def;
    def.shape = &box;
    def.density = 1.0f;
    def.filter = filter;
    b2Fixture* fx = b->body->CreateFixture(&def);
    fx->SetSensor(b->isSensor);
    TrackNewFixture(b, fx);
    RegisterShapeGlobalId(fx);
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
int GM_CDECL gmf_pd_getCollisionCategory(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt(f ? (int)DesiredFixtureFilter(b, f).categoryBits : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_getCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt(f ? (int)DesiredFixtureFilter(b, f).maskBits : -1);
    return GM_OK;
}
int GM_CDECL gmf_pd_setCollisionCategory(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(category, 0, 1);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
            b2Filter filter = DesiredFixtureFilter(b, f);
            filter.categoryBits = (uint16)category;
            ApplyDesiredFixtureFilter(b, f, filter);
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_setCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(mask, 0, -1);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
            b2Filter filter = DesiredFixtureFilter(b, f);
            filter.maskBits = (uint16)mask;
            ApplyDesiredFixtureFilter(b, f, filter);
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_setCollisionGroup(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(group, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    for (const gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* b = GetBodyData(m, bodyVar);
        if (!b || !b->body) continue;
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
            b2Filter filter = DesiredFixtureFilter(b, f);
            filter.groupIndex = (int16)group;
            ApplyDesiredFixtureFilter(b, f, filter);
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_getCollisionGroup(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt(f ? (int)DesiredFixtureFilter(b, f).groupIndex : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_isSleeping(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushInt((b && !b->body->IsAwake()) ? 1 : 0);
    return GM_OK;
}
// Retail PhysicsData.putToSleep walks all owned bodies, zeros linear/angular
// velocity, forces/torque, and sleep time, then marks the body asleep. The
// fixed-rotation adapter also needs the shape-local angular velocity cleared.
int GM_CDECL gmf_pd_putToSleep(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    for (const gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* b = GetBodyData(m, bodyVar);
        SetBodySleeping(b, true);
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_setAllowSleeping(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(allowed, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    for (const gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* b = GetBodyData(m, bodyVar);
        if (b && b->body) b->body->SetSleepingAllowed(allowed != 0);
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_wakeUp(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    for (gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* b = GetBodyData(m, bodyVar);
        if (b && b->body) b->body->SetAwake(true);
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_appendShapes(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(index, 0, 0);
    GM_CHECK_USER_PARAM(ShapeDefCollection*, g_physicsShapeDefType, sd, 1);
    if (index < 0 || index >= (int)d->bodies.size()) return GM_OK;
    PhysicsBodyData* b = GetBodyData(*a_thread->GetMachine(), d->bodies[(size_t)index]);
    if (!b || !b->body) return GM_OK;
    for (auto& def : sd->fixtureDefs) {
        b2Fixture* fx = b->body->CreateFixture(&def);
        TrackNewFixture(b, fx);
        RegisterShapeGlobalId(fx, sd->playerFriction);
    }
    SyncBodyTypeFromMass(b->body);
    int lastIndex = -1;
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) ++lastIndex;
    a_thread->PushInt(lastIndex);
    return GM_OK;
}
int GM_CDECL gmf_pd_replaceBody(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    if (index < 0 || index >= (int)d->bodies.size()) { a_thread->PushNull(); return GM_OK; }
    PhysicsBodyData* old = GetBodyData(m, d->bodies[(size_t)index]);
    float x = 0.0f, y = 0.0f;
    if (old && old->body) { b2Vec2 p = old->body->GetPosition(); x = p.x; y = p.y; }
    InvalidatePhysicsBody(old);
    UnrootGmVariable(m, d->bodies[(size_t)index]);
    gmVariable fresh = MakePhysicsBody(m, x, y, false, true);
    PhysicsBodyData* bd = GetBodyData(m, fresh);
    if (bd && bd->body) bd->body->GetUserData().pointer = (uintptr_t)d;
    RootGmVariable(m, fresh);
    d->bodies[(size_t)index] = fresh;
    a_thread->Push(fresh);
    return GM_OK;
}
int GM_CDECL gmf_pd_getMass(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? b->body->GetMass() : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_pd_getMaxSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? b->maxSpeed : 1e9f);
    return GM_OK;
}
int GM_CDECL gmf_pd_setMaxRotateSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->maxRotateSpeed = v;
    return GM_OK;
}
int GM_CDECL gmf_pd_getMaxRotateSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? b->maxRotateSpeed : 1e9f);
    return GM_OK;
}
int GM_CDECL gmf_pd_setRotationDecayRate(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->rotationDecayRate = ComputeDecayFactor(v);
    return GM_OK;
}
int GM_CDECL gmf_pd_setMaxVelocityDecayRate(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->maxVelocityDecayRate = v;
    return GM_OK;
}
int GM_CDECL gmf_pd_setMaxRotationDecayRate(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->maxRotationDecayRate = v;
    return GM_OK;
}
int GM_CDECL gmf_pd_setRotateAcc(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->rotateAcc = v;
    return GM_OK;
}
int GM_CDECL gmf_pd_getRotateAcc(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? b->rotateAcc : 1e9f);
    return GM_OK;
}
int GM_CDECL gmf_pd_setBrakeAcc(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->brakeAcc = v;
    return GM_OK;
}
int GM_CDECL gmf_pd_getBrakeAcc(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? b->brakeAcc : 1e9f);
    return GM_OK;
}
int GM_CDECL gmf_pd_setThrustAcc(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 1e9f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->thrustAcc = v;
    return GM_OK;
}
int GM_CDECL gmf_pd_getThrustAcc(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? b->thrustAcc : 1e9f);
    return GM_OK;
}
int GM_CDECL gmf_pd_setDoesRotationResponse(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 1);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->body->SetFixedRotation(v == 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_isSensor(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    for (const gmVariable& body : d->bodies) {
        PhysicsBodyData* b = GetBodyData(machine, body);
        if (b && !AllShapesAreSensors(b->body)) {
            a_thread->PushInt(0);
            return GM_OK;
        }
    }
    a_thread->PushInt(1);
    return GM_OK;
}
int GM_CDECL gmf_pd_setIsCollidable(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 1);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        SetBodyCollidable(b, v != 0);
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_isCollidable(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushInt((b && b->isCollidable) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_setRestitution(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(r, 0, 0.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetRestitution(r);
    return GM_OK;
}
// PhysicsData forwards setMass to its primary body.
int GM_CDECL gmf_pd_setMass(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(mass, 0, 1.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) SetRetailBodyMass(b->body, mass);
    return GM_OK;
}
// Shared by getAABB/getMinPos/getMaxPos -- same fixture-union AABB
// computation, single-point fallback (bare position, no fixtures) when
// the body has none.
bool ComputePhysicsDataAABB(gmThread* a_thread, PhysicsDataObj* d, b2AABB& a_out) {
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (!b) return false;
    bool first = true;
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) {
        for (int i = 0; i < fx->GetShape()->GetChildCount(); ++i) {
            b2AABB shapeAabb;
            fx->GetShape()->ComputeAABB(&shapeAabb, b->body->GetTransform(), i);
            if (first) { a_out = shapeAabb; first = false; }
            else a_out.Combine(shapeAabb);
        }
    }
    if (first) { b2Vec2 p = b->body->GetPosition(); a_out.lowerBound = p; a_out.upperBound = p; }
    return true;
}

int GM_CDECL gmf_pd_getAABB(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* t = m.AllocTableObject();
    b2AABB aabb;
    if (ComputePhysicsDataAABB(a_thread, d, aabb)) {
        t->Set(&m, "minx", gmVariable(aabb.lowerBound.x));
        t->Set(&m, "miny", gmVariable(aabb.lowerBound.y));
        t->Set(&m, "maxx", gmVariable(aabb.upperBound.x));
        t->Set(&m, "maxy", gmVariable(aabb.upperBound.y));
    }
    a_thread->PushTable(t);
    return GM_OK;
}

int GM_CDECL gmf_pd_getMinPos(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    b2AABB aabb;
    if (!ComputePhysicsDataAABB(a_thread, d, aabb)) { a_thread->Push(MakeVector2D(m, 0.0f, 0.0f)); return GM_OK; }
    a_thread->Push(MakeVector2D(m, aabb.lowerBound.x, aabb.lowerBound.y));
    return GM_OK;
}
int GM_CDECL gmf_pd_getMaxPos(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    b2AABB aabb;
    if (!ComputePhysicsDataAABB(a_thread, d, aabb)) { a_thread->Push(MakeVector2D(m, 0.0f, 0.0f)); return GM_OK; }
    a_thread->Push(MakeVector2D(m, aabb.upperBound.x, aabb.upperBound.y));
    return GM_OK;
}
int GM_CDECL gmf_pd_getShapeRadius(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(bodyIndex, 0, 0);
    GM_INT_PARAM(shapeIndex, 1, 0);
    gmMachine& m = *a_thread->GetMachine();
    float radius = 0.0f;
    if (bodyIndex >= 0 && bodyIndex < (int)d->bodies.size()) {
        PhysicsBodyData* b = GetBodyData(m, d->bodies[(size_t)bodyIndex]);
        b2Fixture* fixture = b ? FixtureAtShapeIndex(b->body, shapeIndex) : nullptr;
        if (fixture) radius = fixture->GetShape()->m_radius;
    }
    a_thread->PushFloat(radius);
    return GM_OK;
}

} // namespace

void InvalidatePhysicsMovementMembership() {
    g_physicsMovementMembershipDirty = true;
}

void InvalidatePhysicsCollisionInfoView(uintptr_t a_physicsData,
                                        uint32_t a_otherEntityId) {
    InvalidatePhysicsCollisionInfoViewImpl(a_physicsData, a_otherEntityId);
}

gmType GetShapeCollisionInfoType() {
    return g_shapeCollisionInfoType;
}

gmVariable MakeIntListRefContainer(gmMachine& a_machine, std::vector<int> a_values) {
    auto* data = new IntIterData();
    data->values = std::move(a_values);
    gmUserObject* object = a_machine.AllocUserObject(data, g_intIterType);
    return gmVariable(object);
}

void FlushDeferredPhysicsBodyDestructions() {
    // StepPhysicsWorld invokes this only after b2World::Step has released
    // its lock. Keep the guard for shutdown and direct callers.
    if (GetPhysicsWorld().IsLocked()) return;
    for (const auto& [body, fixture] : g_deferredFixtureDestructions) {
        if (std::find(g_deferredBodyDestructions.begin(),
                      g_deferredBodyDestructions.end(), body) !=
            g_deferredBodyDestructions.end()) continue;
        body->DestroyFixture(fixture);
        SyncBodyTypeFromMass(body);
    }
    g_deferredFixtureDestructions.clear();
    for (b2Body* body : g_deferredBodyDestructions) {
        RemovePhysicsJointsForBody(body);
        GetPhysicsWorld().DestroyBody(body);
        ClearEntityPhysicsBodyTeardown(body);
    }
    g_deferredBodyDestructions.clear();
}

gmVariable GetPhysicsDataMainBodyVar(gmMachine& a_machine, const gmVariable& a_physicsDataVar) {
    if (a_physicsDataVar.m_type != g_physicsDataType) return gmVariable::s_null;
    gmObject* obj = a_machine.GetObject(a_physicsDataVar.m_value.m_ref);
    if (!obj) return gmVariable::s_null;
    PhysicsDataObj* d = (PhysicsDataObj*)((gmUserObject*)obj)->m_user;
    if (d->bodies.empty()) return gmVariable::s_null;
    return d->bodies[0];
}

bool PhysicsDataHasBodies(gmMachine& a_machine, const gmVariable& a_physicsDataVar) {
    if (a_physicsDataVar.m_type != g_physicsDataType) return false;
    gmObject* obj = a_machine.GetObject(a_physicsDataVar.m_value.m_ref);
    if (!obj) return false;
    PhysicsDataObj* d = (PhysicsDataObj*)((gmUserObject*)obj)->m_user;
    return d && !d->bodies.empty();
}

void SetPhysicsDataAwake(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                         bool a_awake) {
    if (a_physicsDataVar.m_type != g_physicsDataType) return;
    gmObject* obj = a_machine.GetObject(a_physicsDataVar.m_value.m_ref);
    if (!obj) return;
    PhysicsDataObj* data = (PhysicsDataObj*)((gmUserObject*)obj)->m_user;
    if (!data) return;
    for (const gmVariable& bodyVar : data->bodies) {
        PhysicsBodyData* body = GetBodyData(a_machine, bodyVar);
        SetBodySleeping(body, !a_awake);
    }
}

bool GetPhysicsDataPos(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                       float& a_x, float& a_y) {
    PhysicsBodyData* body = GetBodyData(a_machine, GetPhysicsDataMainBodyVar(a_machine, a_physicsDataVar));
    if (!body || !body->body) return false;
    const b2Vec2 pos = body->body->GetPosition();
    a_x = pos.x;
    a_y = pos.y;
    return true;
}

bool GetPhysicsDataFacingDirection(gmMachine& a_machine,
                                   const gmVariable& a_physicsDataVar,
                                   float& a_x, float& a_y) {
    PhysicsBodyData* body = GetBodyData(a_machine,
        GetPhysicsDataMainBodyVar(a_machine, a_physicsDataVar));
    if (!body || !body->body) return false;
    const b2Vec2 direction = BodyFacingDirection(*body);
    a_x = direction.x;
    a_y = direction.y;
    return true;
}

bool GetPhysicsDataAABB(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                         float& a_minX, float& a_minY, float& a_maxX, float& a_maxY) {
    a_minX = a_minY = a_maxX = a_maxY = 0.0f;
    if (a_physicsDataVar.m_type != g_physicsDataType) return false;
    gmObject* obj = a_machine.GetObject(a_physicsDataVar.m_value.m_ref);
    if (!obj) return false;
    PhysicsDataObj* d = (PhysicsDataObj*)((gmUserObject*)obj)->m_user;
    PhysicsBodyData* b = d->bodies.empty() ? nullptr : GetBodyData(a_machine, d->bodies[0]);
    if (!b || !b->body) return false;
    b2AABB aabb; bool first = true;
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) {
        for (int i = 0; i < fx->GetShape()->GetChildCount(); ++i) {
            b2AABB shapeAabb;
            fx->GetShape()->ComputeAABB(&shapeAabb, b->body->GetTransform(), i);
            if (first) { aabb = shapeAabb; first = false; }
            else aabb.Combine(shapeAabb);
        }
    }
    if (first) { b2Vec2 p = b->body->GetPosition(); aabb.lowerBound = p; aabb.upperBound = p; }
    a_minX = aabb.lowerBound.x; a_minY = aabb.lowerBound.y;
    a_maxX = aabb.upperBound.x; a_maxY = aabb.upperBound.y;
    return true;
}

bool GetPhysicsDataAllBodiesAABB(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                                 float& a_minX, float& a_minY,
                                 float& a_maxX, float& a_maxY) {
    a_minX = a_minY = std::numeric_limits<float>::infinity();
    a_maxX = a_maxY = -std::numeric_limits<float>::infinity();
    if (a_physicsDataVar.m_type != g_physicsDataType) return false;
    gmObject* obj = a_machine.GetObject(a_physicsDataVar.m_value.m_ref);
    if (!obj) return false;
    auto* data = static_cast<PhysicsDataObj*>(
        static_cast<gmUserObject*>(obj)->m_user);
    if (!data) return false;

    bool haveBounds = false;
    for (const gmVariable& bodyVar : data->bodies) {
        PhysicsBodyData* bodyData = GetBodyData(a_machine, bodyVar);
        if (!bodyData || !bodyData->body) continue;
        b2Body* body = bodyData->body;
        bool haveBodyFixture = false;
        for (b2Fixture* fixture = body->GetFixtureList(); fixture;
             fixture = fixture->GetNext()) {
            const b2Shape* shape = fixture->GetShape();
            for (int child = 0; child < shape->GetChildCount(); ++child) {
                b2AABB bounds;
                shape->ComputeAABB(&bounds, body->GetTransform(), child);
                a_minX = std::min(a_minX, bounds.lowerBound.x);
                a_minY = std::min(a_minY, bounds.lowerBound.y);
                a_maxX = std::max(a_maxX, bounds.upperBound.x);
                a_maxY = std::max(a_maxY, bounds.upperBound.y);
                haveBounds = haveBodyFixture = true;
            }
        }
        if (!haveBodyFixture) {
            const b2Vec2 position = body->GetPosition();
            a_minX = std::min(a_minX, position.x);
            a_minY = std::min(a_minY, position.y);
            a_maxX = std::max(a_maxX, position.x);
            a_maxY = std::max(a_maxY, position.y);
            haveBounds = true;
        }
    }
    return haveBounds;
}

// --- exposed for physics_global_bindings.cpp (the "Physics" global class,
// e.g. isShapeSensor/getBodyFromShapeID/getEntityFromShapeID/raycastOne) --

b2Fixture* FindFixtureByGlobalId(int a_id) {
    return GetFixtureByGlobalId(a_id);
}
float GetFixturePlayerFriction(b2Fixture* a_fixture) {
    if (!a_fixture) return 0.3f;
    const int id = (int)a_fixture->GetUserData().pointer;
    if (id <= 0 || id >= (int)g_shapePlayerFrictions.size()) return 0.3f;
    return g_shapePlayerFrictions[(size_t)id];
}
gmVariable MakePhysicsShapeByGlobalId(gmMachine& a_machine, int a_id) {
    if (!GetFixtureByGlobalId(a_id)) return gmVariable::s_null;
    return gmVariable(a_machine.AllocUserObject(new PhysicsShapeHandle{a_id}, g_physicsShapeType));
}
// Resolve the original script-visible handle from the live body registry.
// The owner's bodies list is not reliable for every body creation path;
// collision callbacks can refer to a body while that list is empty.
gmVariable FindPhysicsBodyVarForB2Body(gmMachine& a_machine, b2Body* a_body) {
    (void)a_machine;
    if (!a_body) return gmVariable::s_null;
    for (PhysicsBodyData* bd : g_allPhysicsBodies)
        if (bd && bd->body == a_body) return bd->selfVar;
    return gmVariable::s_null;
}
// The entity table that owns a_body's PhysicsData, if any.
gmVariable FindOwnerEntityForB2Body(b2Body* a_body) {
    PhysicsDataObj* owner = (PhysicsDataObj*)(uintptr_t)a_body->GetUserData().pointer;
    return owner ? owner->ownerEntity : gmVariable::s_null;
}
bool IsPhysicsBodyVar(const gmVariable& a_var) { return a_var.m_type == g_physicsBodyType; }
b2Body* GetB2BodyFromVar(gmMachine& a_machine, const gmVariable& a_var) {
    PhysicsBodyData* bd = GetBodyData(a_machine, a_var);
    return bd ? bd->body : nullptr;
}

bool GetPhysicsBodyPosAndDir(gmMachine& a_machine, const gmVariable& a_var,
                              float& a_x, float& a_y, float& a_dirX, float& a_dirY) {
    PhysicsBodyData* bd = GetBodyData(a_machine, a_var);
    if (!bd || !bd->body) return false;
    b2Vec2 p = bd->body->GetPosition();
    a_x = p.x; a_y = p.y;
    if (!bd->body->IsFixedRotation()) {
        const float angle = bd->body->GetAngle();
        bd->dirX = GroundRenderMath::RetailGroundSinCos(angle, true);
        bd->dirY = GroundRenderMath::RetailGroundSinCos(angle, false);
    }
    a_dirX = bd->dirX;
    a_dirY = bd->dirY;
    return true;
}

void SetPhysicsDataOwnerEntity(gmMachine& a_machine, const gmVariable& a_physicsDataVar, const gmVariable& a_ownerEntity) {
    if (a_physicsDataVar.m_type != g_physicsDataType) return;
    gmObject* obj = a_machine.GetObject(a_physicsDataVar.m_value.m_ref);
    if (!obj) return;
    PhysicsDataObj* d = (PhysicsDataObj*)((gmUserObject*)obj)->m_user;
    UnrootGmVariable(a_machine, d->ownerEntity);
    d->ownerEntity = a_ownerEntity;
    RootGmVariable(a_machine, d->ownerEntity);
    InvalidatePhysicsMovementMembership();
}

void DestroyPhysicsDataForEntity(gmMachine& a_machine, const gmVariable& a_entity) {
    if (a_entity.m_type != GM_TABLE) return;
    gmTableObject* entity = (gmTableObject*)a_machine.GetObject(a_entity.m_value.m_ref);
    if (!entity) return;

    gmVariable dataVar = entity->Get(&a_machine, "PhysicsData");
    if (dataVar.m_type == GM_NULL)
        dataVar = entity->Get(&a_machine, "__physicsData");
    if (dataVar.m_type != g_physicsDataType) return;
    gmObject* object = a_machine.GetObject(dataVar.m_value.m_ref);
    if (!object) return;
    PhysicsDataObj* data = (PhysicsDataObj*)((gmUserObject*)object)->m_user;
    if (!data) return;

    // Mirror native entity teardown at the deletion boundary instead of
    // waiting for GM collection: script code may retain the PhysicsData or
    // PhysicsBody wrappers after the entity is gone. InvalidatePhysicsBody
    // clears native IDs/owner links immediately and defers only the Box2D
    // call itself when the world is locked.
    for (gmVariable& bodyVar : data->bodies) {
        PhysicsBodyData* body = GetBodyData(a_machine, bodyVar);
        if (body && body->body) MarkEntityPhysicsBodyForTeardown(body->body);
        InvalidatePhysicsBody(body);
        UnrootGmVariable(a_machine, bodyVar);
    }
    data->bodies.clear();

    // PhysicsData's owner is explicitly rooted for contact-to-entity lookup.
    // Break that root now; otherwise an entity owning this PhysicsData and
    // PhysicsData rooting the entity form a cycle that survives manager
    // deletion and postpones its native resource cleanup indefinitely.
    UnrootGmVariable(a_machine, data->ownerEntity);
    data->ownerEntity = gmVariable::s_null;
    InvalidatePhysicsMovementMembership();
}

gmVariable MakePhysicsData(gmMachine& a_machine, gmVariable a_ownerEntity, int a_type) {
    PhysicsDataObj* data = new PhysicsDataObj();
    data->type = a_type;
    data->ownerEntity = a_ownerEntity;
    RootGmVariable(a_machine, data->ownerEntity);
    gmUserObject* obj = a_machine.AllocUserObject(data, g_physicsDataType);
    g_allPhysicsData.push_back(data);
    InvalidatePhysicsMovementMembership();
    return gmVariable(obj);
}

float RetailStepAngularVelocity(const PhysicsBodyData& a_body, float a_velocity,
                                float a_dt) {
    const float decayed = a_velocity * a_body.rotationDecayRate;
    const float excess = std::max(decayed - a_body.maxRotateSpeed, 0.0f);
    const float excessFactor = 1.0f - a_body.maxRotationDecayRate;
    float result = std::fma(-excess, excessFactor, decayed);
    result = std::fma(a_body.rotAccum, a_dt, result);
    if (std::fabs(result) > a_body.maxRotateSpeed)
        result = std::copysign(a_body.maxRotateSpeed, result);
    return result;
}

float RetailDirectionAngle(float a_x, float a_y) {
    return std::atan2(a_y, a_x);
}

void StepPhysicsBodyMovement(float a_dt) {
    gmMachine* machine = GetEngineContext().machine;
    if (!machine) return;

    if (g_physicsMovementMembershipDirty) {
        struct TickEntry { PhysicsDataObj* data; int objectId; size_t insertionOrder; };
        std::vector<TickEntry> tickEntries;
        tickEntries.reserve(g_allPhysicsData.size());
        size_t insertionOrder = 0;
        for (PhysicsDataObj* data : g_allPhysicsData) {
            const size_t thisOrder = insertionOrder++;
            if (!data) continue;
            if (data->ownerEntity.m_type != GM_TABLE) continue;
            gmTableObject* owner = (gmTableObject*)machine->GetObject(
                data->ownerEntity.m_value.m_ref);
            if (!owner) continue;
            gmVariable enabled = owner->Get(machine, "__enabled");
            if (enabled.m_type != GM_INT || enabled.m_value.m_int == 0) continue;
            gmVariable noPhysicsUpdate = owner->Get(machine, "__noPhysicsUpdate");
            if (noPhysicsUpdate.m_type == GM_INT && noPhysicsUpdate.m_value.m_int != 0)
                continue;
            gmVariable id = owner->Get(machine, "__objectID");
            if (id.m_type != GM_INT) continue;
            tickEntries.push_back({data, id.m_value.m_int, thisOrder});
        }
        std::stable_sort(tickEntries.begin(), tickEntries.end(),
            [](const TickEntry& a, const TickEntry& b) {
                if (a.objectId != b.objectId) return a.objectId < b.objectId;
                return a.insertionOrder < b.insertionOrder;
            });
        g_orderedPhysicsMovementData.clear();
        g_orderedPhysicsMovementData.reserve(tickEntries.size());
        for (const TickEntry& entry : tickEntries)
            g_orderedPhysicsMovementData.push_back(entry.data);
        g_physicsMovementMembershipDirty = false;
    }
    for (PhysicsDataObj* data : g_orderedPhysicsMovementData) {
        for (const gmVariable& bodyVar : data->bodies) {
            if (bodyVar.m_type != g_physicsBodyType) continue;
            PhysicsBodyData* b = GetBodyData(*machine, bodyVar);
            if (!b || !b->body) continue;

            if (b->geometryDirty) {
                b->body->RefreshFixtureProxies();
                b->geometryDirty = false;
            }

            const bool rotationResponse = !b->body->IsFixedRotation();
            if (rotationResponse) {
                const float angle = b->body->GetAngle();
                b->dirX = GroundRenderMath::RetailGroundSinCos(angle, true);
                b->dirY = GroundRenderMath::RetailGroundSinCos(angle, false);
            }

            // Retain PPC fmadds' single-rounding behavior for the velocity
            // integration: velocity*decay + facing*thrust*dt.
            const b2Vec2 oldVelocity = ScriptVelocity(b->body->GetLinearVelocity());
            const float thrustX = (b->dirX * b->thrustAccum) * a_dt;
            const float thrustY = (b->dirY * b->thrustAccum) * a_dt;
            float velocityX = std::fma(oldVelocity.x, b->velocityDecayRate, thrustX);
            float velocityY = std::fma(oldVelocity.y, b->velocityDecayRate, thrustY);

            const float speed = std::sqrt(std::fma(velocityX, velocityX,
                                                    velocityY * velocityY));
            const float excess = std::max(speed - b->maxSpeed, 0.0f);
            const float excessFactor = 1.0f - b->maxVelocityDecayRate;
            float adjustedSpeed = std::fma(-excess, excessFactor, speed);
            if (b->brakeFlag)
                adjustedSpeed = std::fma(-b->brakeAcc, a_dt, adjustedSpeed);
            adjustedSpeed = std::max(adjustedSpeed, 0.0f);
            if (speed == 0.0f) {
                velocityX = 0.0f;
                velocityY = 0.0f;
            } else {
                velocityX = (velocityX / speed) * adjustedSpeed;
                velocityY = (velocityY / speed) * adjustedSpeed;
            }
            b->body->SetLinearVelocity(Box2DVelocity(b2Vec2(velocityX, velocityY)));
            if (CreatureDebugEnabled()) {
                int entityId = -1;
                if (GetCreatureDebugEntityId(*machine, b->body, entityId)) {
                    const b2Vec2 pos = b->body->GetPosition();
                    std::fprintf(stderr,
                        "[creature-integrate] tick=%u dtMs=%.6f entity=%d body=%d "
                        "pos=(%.5f,%.5f) beforeMs=(%.7f,%.7f) "
                        "afterMs=(%.7f,%.7f) dir=(%.5f,%.5f) "
                        "maxSpeed=%.7f mass=%.6g\n",
                        GetEngineContext().gameSystem.currentTick, a_dt,
                        entityId, b->objectId, pos.x, pos.y,
                        oldVelocity.x, oldVelocity.y, velocityX, velocityY,
                        b->dirX, b->dirY, b->maxSpeed, b->body->GetMass());
                }
            }

            if (b->rotateToActive) {
                const float currentAngle = RetailDirectionAngle(b->dirX, b->dirY);
                const float targetAngle = RetailDirectionAngle(b->rotateToX,
                                                                b->rotateToY);
                const float rawDifference = std::fabs(targetAngle - currentAngle);
                const float angleDifference = std::min(rawDifference,
                                                        6.28318548203f - rawDifference);
                const float sign = static_cast<float>(b->rotateToSign);
                const float signedDt = sign * a_dt;
                const float rotationInput = b->rotateAcc * signedDt;
                const float turnThreshold = rotationInput;
                if (angleDifference <= turnThreshold || angleDifference <= 0.005f) {
                    if (rotationResponse) {
                        b->body->SetAngularVelocity(0.0f);
                        b2Vec2 direction(b->rotateToX, b->rotateToY);
                        if (direction.LengthSquared() > 0.0f) direction.Normalize();
                        b->body->SetTransform(b->body->GetPosition(),
                                              RetailDirectionAngle(direction.x,
                                                                   direction.y));
                    } else {
                        b->fixedRotationVel = 0.0f;
                        b->dirX = b->rotateToX;
                        b->dirY = b->rotateToY;
                    }
                    b->rotateToActive = false;
                } else {
                    if (rotationResponse) {
                        b->body->SetAngularVelocity(Box2DAngularVelocity(
                            ScriptAngularVelocity(b->body->GetAngularVelocity()) + rotationInput));
                    } else {
                        b->fixedRotationVel += rotationInput;
                    }
                }
            }

            if (rotationResponse) {
                b->body->SetAngularVelocity(Box2DAngularVelocity(RetailStepAngularVelocity(
                    *b, ScriptAngularVelocity(b->body->GetAngularVelocity()), a_dt)));
            } else {
                b->fixedRotationVel = RetailStepAngularVelocity(
                    *b, b->fixedRotationVel, a_dt);
                const float angle = RetailDirectionAngle(b->dirX, b->dirY) +
                                    b->fixedRotationVel * a_dt;
                b->dirX = GroundRenderMath::RetailGroundSinCos(angle, true);
                b->dirY = GroundRenderMath::RetailGroundSinCos(angle, false);
            }

            // GE3_MOVE_DEBUG=1: dump the real per-tick movement state for any
            // body that is actually being driven, so on-screen speed can be
            // checked against real numbers instead of inferred. Zero cost when
            // unset, same pattern as GE3_UV_DEBUG in shape_bindings.cpp.
            static const char* s_moveDebug = std::getenv("GE3_MOVE_DEBUG");
            if (s_moveDebug && (b->thrustAccum != 0.0f ||
                                ScriptVelocity(b->body->GetLinearVelocity()).LengthSquared() > 0.0f)) {
                b2Vec2 p = b->body->GetPosition();
                const b2Vec2 vel = ScriptVelocity(b->body->GetLinearVelocity());
                std::fprintf(stderr,
                    "[move] id=%d pos=(%.1f,%.1f) vel=(%.1f,%.1f) |v|=%.1f "
                    "dir=(%.2f,%.2f) accum=%.4g maxSpeed=%.4g decay=%.4g\n",
                    b->objectId, p.x, p.y, vel.x, vel.y, vel.Length(),
                    b->dirX, b->dirY, b->thrustAccum, b->maxSpeed,
                    b->velocityDecayRate);
            }

            b->thrustAccum = 0.0f;
            b->rotAccum = 0.0f;
            b->thrustFlag = false;
            b->reverseFlag = false;
            b->brakeFlag = false;
            b->rotLeftFlag = false;
            b->rotRightFlag = false;
        }
    }
}

void RegisterPhysicsBodyBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("PHYSICSBODY_GetBodyByID", gmf_PHYSICSBODY_GetBodyByID);

    g_physicsShapeType = a_machine.CreateUserType("PhysicsShape");
    a_machine.RegisterTypeLibrary(g_physicsShapeType, s_physicsShapeLib,
                                  sizeof(s_physicsShapeLib) / sizeof(s_physicsShapeLib[0]));
    a_machine.RegisterUserCallbacks(g_physicsShapeType, nullptr,
        [](gmMachine*, gmUserObject* object) { delete (PhysicsShapeHandle*)object->m_user; }, nullptr);

    g_physicsBodyType = a_machine.CreateUserType("PhysicsBody");
    a_machine.RegisterTypeLibrary(g_physicsBodyType, s_physicsBodyLib,
                                   sizeof(s_physicsBodyLib) / sizeof(s_physicsBodyLib[0]));
    a_machine.RegisterUserCallbacks(g_physicsBodyType, nullptr, PhysicsBodyGCDestruct, nullptr);

    g_collisionInfoType = a_machine.CreateUserType("CollisionInfo");
    a_machine.RegisterTypeLibrary(g_collisionInfoType, s_collisionInfoLib,
                                   sizeof(s_collisionInfoLib) / sizeof(s_collisionInfoLib[0]));
    a_machine.RegisterUserCallbacks(g_collisionInfoType, nullptr,
        [](gmMachine*, gmUserObject* a_object) {
            auto* info = static_cast<CollisionInfoData*>(a_object->m_user);
            ReleaseShapeCollisionInfoViews(info);
            delete info;
        }, nullptr);

    g_shapeCollisionInfoType = a_machine.CreateUserType("ShapeCollisionInfo");
    a_machine.RegisterTypeLibrary(g_shapeCollisionInfoType, s_shapeCollisionInfoLib,
                                   sizeof(s_shapeCollisionInfoLib) / sizeof(s_shapeCollisionInfoLib[0]));
    a_machine.RegisterUserCallbacks(g_shapeCollisionInfoType, nullptr,
        [](gmMachine*, gmUserObject* a_object) {
            auto* sci = static_cast<ShapeCollisionInfoData*>(a_object->m_user);
            if (sci && sci->machine)
                ReleaseCollisionInfoRoot(*sci->machine, sci->ownerCollisionInfo);
            delete sci;
        }, nullptr);

    g_intIterType = a_machine.CreateUserType("INTLISTREFCONTAINER");
    a_machine.RegisterLibraryFunction("INTLISTREFCONTAINER", gmf_INTLISTREFCONTAINER);
    a_machine.RegisterTypeLibrary(g_intIterType, s_intIterLib,
                                   sizeof(s_intIterLib) / sizeof(s_intIterLib[0]));
    a_machine.RegisterUserCallbacks(g_intIterType, nullptr,
        [](gmMachine*, gmUserObject* a_object) { delete (IntIterData*)a_object->m_user; }, nullptr);

    gmFunctionEntry physicsDataLib[] = {
        {"getType", gmf_pd_getType, nullptr},
        {"setVisibilityAABBScale", gmf_pd_setVisibilityAABBScale, nullptr},
        {"createBody", gmf_pd_createBody, nullptr},
        {"getBody", gmf_pd_getBody, nullptr},
        {"getNumBodies", gmf_pd_getNumBodies, nullptr},
        {"removeBody", gmf_pd_removeBody, nullptr},
        {"replaceBody", gmf_pd_replaceBody, nullptr},
        {"appendShapes", gmf_pd_appendShapes, nullptr},
        {"wakeUp", gmf_pd_wakeUp, nullptr},
        {"putToSleep", gmf_pd_putToSleep, nullptr},
        {"setAllowSleeping", gmf_pd_setAllowSleeping, nullptr},
        {"isSleeping", gmf_pd_isSleeping, nullptr},
        {"getCollidingEntities", gmf_pd_getCollidingEntities, nullptr},
        {"getJointsOnMe", gmf_pd_getJointsOnMe, nullptr},
        {"getCollisionInfo", gmf_pd_getCollisionInfo, nullptr},
        {"isColliding", gmf_pd_isColliding, nullptr},
        {"getPos", gmf_pd_getPos, nullptr},
        {"getPosRef", gmf_pd_getPosRef, nullptr},
        {"setPos", gmf_pd_setPos, nullptr},
        {"getVel", gmf_pd_getVel, nullptr},
        {"setVel", gmf_pd_setVel, nullptr},
        {"getSpeed", gmf_pd_getSpeed, nullptr},
        {"setSpeed", gmf_pd_setSpeed, nullptr},
        {"getDir", gmf_pd_getDir, nullptr},
        {"setDir", gmf_pd_setDir, nullptr},
        {"getRadius", gmf_pd_getRadius, nullptr},
        {"setAsCircle", gmf_pd_setAsCircle, nullptr},
        {"setAsRect", gmf_pd_setAsRect, nullptr},
        {"isCircle", gmf_pd_isCircle, nullptr},
        {"isRect", gmf_pd_isRect, nullptr},
        {"getWidth", gmf_pd_getWidth, nullptr},
        {"getHeight", gmf_pd_getHeight, nullptr},
        {"setIsSensor", gmf_pd_setIsSensor, nullptr},
        {"setMaxSpeed", gmf_pd_setMaxSpeed, nullptr},
        {"getMaxSpeed", gmf_pd_getMaxSpeed, nullptr},
        {"thrust", gmf_pd_thrust, nullptr},
        {"reverseThrust", gmf_pd_reverseThrust, nullptr},
        {"brake", gmf_pd_brake, nullptr},
        {"rotateLeft", gmf_pd_rotateLeft, nullptr},
        {"rotateRight", gmf_pd_rotateRight, nullptr},
        {"rotateTo", gmf_pd_rotateTo, nullptr},
        {"stopRotatingTo", gmf_pd_stopRotatingTo, nullptr},
        {"doesRotationResponse", gmf_pd_doesRotationResponse, nullptr},
        {"setDoesRotationResponse", gmf_pd_setDoesRotationResponse, nullptr},
        {"setVelocityDecayRate", gmf_pd_setVelocityDecayRate, nullptr},
        {"setMaxVelocityDecayRate", gmf_pd_setMaxVelocityDecayRate, nullptr},
        {"setRotationDecayRate", gmf_pd_setRotationDecayRate, nullptr},
        {"setMaxRotationDecayRate", gmf_pd_setMaxRotationDecayRate, nullptr},
        {"setRotateAcc", gmf_pd_setRotateAcc, nullptr},
        {"getRotateAcc", gmf_pd_getRotateAcc, nullptr},
        {"setMaxRotateSpeed", gmf_pd_setMaxRotateSpeed, nullptr},
        {"getMaxRotateSpeed", gmf_pd_getMaxRotateSpeed, nullptr},
        {"setBrakeAcc", gmf_pd_setBrakeAcc, nullptr},
        {"getBrakeAcc", gmf_pd_getBrakeAcc, nullptr},
        {"setThrustAcc", gmf_pd_setThrustAcc, nullptr},
        {"getThrustAcc", gmf_pd_getThrustAcc, nullptr},
        {"setCollisionCategory", gmf_pd_setCollisionCategory, nullptr},
        {"getCollisionCategory", gmf_pd_getCollisionCategory, nullptr},
        {"setCategoriesToCollideWith", gmf_pd_setCategoriesToCollideWith, nullptr},
        {"getCategoriesToCollideWith", gmf_pd_getCategoriesToCollideWith, nullptr},
        {"setCollisionGroup", gmf_pd_setCollisionGroup, nullptr},
        {"getCollisionGroup", gmf_pd_getCollisionGroup, nullptr},
        {"getMass", gmf_pd_getMass, nullptr},
        {"isSensor", gmf_pd_isSensor, nullptr},
        {"setIsCollidable", gmf_pd_setIsCollidable, nullptr},
        {"isCollidable", gmf_pd_isCollidable, nullptr},
        {"setRestitution", gmf_pd_setRestitution, nullptr},
        {"setMass", gmf_pd_setMass, nullptr},
        {"getAABB", gmf_pd_getAABB, nullptr},
        {"getMinPos", gmf_pd_getMinPos, nullptr},
        {"getMaxPos", gmf_pd_getMaxPos, nullptr},
        {"raycast", gmf_pd_raycast, nullptr},
        {"raycastShapesOne", gmf_pd_raycastShapesOne, nullptr},
        {"getShapeByID", gmf_pd_getShapeByID, nullptr},
        {"getShapeRadius", gmf_pd_getShapeRadius, nullptr},
        {"isPointWithin", gmf_pd_isPointWithin, nullptr},
    };
    // Retail registers this user type as MULTIBODYPHYSICSDATA. Preserve the
    // native type name for GM type inspection as well as its method library.
    g_physicsDataType = a_machine.CreateUserType("MULTIBODYPHYSICSDATA");
    a_machine.RegisterTypeLibrary(g_physicsDataType, physicsDataLib,
                                   sizeof(physicsDataLib) / sizeof(physicsDataLib[0]));
    a_machine.RegisterUserCallbacks(g_physicsDataType, nullptr,
        [](gmMachine* a_m, gmUserObject* a_object) {
            PhysicsDataObj* d = (PhysicsDataObj*)a_object->m_user;
            g_allPhysicsData.erase(
                std::remove(g_allPhysicsData.begin(), g_allPhysicsData.end(), d),
                g_allPhysicsData.end());
            InvalidatePhysicsMovementMembership();
            UnrootGmVariable(*a_m, d->ownerEntity);
            // CollisionInfo views are explicitly rooted because the native
            // PhysicsData map owns their identity in retail. The owner is
            // going away now, so make any script-held view inert before
            // dropping our roots; otherwise its next accessor could resolve
            // a recycled PhysicsData address.
            for (const auto& cached : d->collisionInfoViews) {
                const gmVariable& view = cached.second;
                if (view.m_type == g_collisionInfoType) {
                    gmObject* object = a_m->GetObject(view.m_value.m_ref);
                    auto* user = object ? static_cast<gmUserObject*>(object) : nullptr;
                    auto* info = user
                        ? static_cast<CollisionInfoData*>(user->m_user) : nullptr;
                    if (info) {
                        ReleaseShapeCollisionInfoViews(info);
                        info->contacts.clear();
                        info->pairExists = false;
                        info->invalidated = true;
                        info->physicsDataKey = 0;
                    }
                }
                ReleaseCollisionInfoRoot(*a_m, view);
            }
            d->collisionInfoViews.clear();
            for (gmVariable& bv : d->bodies) {
                PhysicsBodyData* bd = GetBodyData(*a_m, bv);
                const bool bodyStillLive =
                    bd && std::find(g_allPhysicsBodies.begin(),
                                    g_allPhysicsBodies.end(), bd) != g_allPhysicsBodies.end();
                if (bodyStillLive && bd->body) bd->body->GetUserData().pointer = 0;
                UnrootGmVariable(*a_m, bv);
            }
            delete d;
        },
        nullptr);
}
