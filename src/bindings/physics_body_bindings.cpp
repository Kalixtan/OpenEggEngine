#include "gmThread.h"
#include "physics_body_bindings.h"
#include "physics_joint_bindings.h"
#include <algorithm>
#include "physics_bindings.h"
#include "vector2d_bindings.h"
#include "../core/physics_world.h"
#include "../core/strict_calls.h"
#include "../core/process_manager.h" // RootGmVariable/UnrootGmVariable
#include <box2d/box2d.h>
#include <vector>
#include <cmath>

namespace {

gmType g_physicsBodyType = -1;
gmType g_physicsDataType = -1;
gmType g_physicsShapeType = -1;
gmType g_entityIterType = -1; // stub iterator for getCollidingEntities()

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

// setPos/setVel/setDir all take EITHER (x,y) as two numbers OR a single
// vector2D (confirmed real: Platformer/Main.gm:1563's
// `t.pb.setPos(t.pos);` vs Player.gm's own `pb.setDir(...)`-style 2-float
// calls elsewhere) -- accept both rather than picking one.
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
    float radius = 0.0f; // last circle radius appendShapes saw, for getRadius()
    bool isSensor = false;
    bool isCollidable = true;
    // The gmVariable (PhysicsBody user object) that wraps THIS struct --
    // set right after allocation in MakePhysicsBody. Needed so native code
    // that only has a b2Body* (e.g. a raycast hit, or Physics.
    // getBodyFromShapeID) can hand the ORIGINAL script-visible PhysicsBody
    // handle back to a script, not just act on the raw Box2D body.
    gmVariable selfVar = gmVariable::s_null;
    // Confirmed real (Player.gm: `local newAnchorID=pb.getObjectID();
    // this.anchorBodyID=newAnchorID;` then later
    // `PHYSICSBODY_GetBodyByID(this.anchorBodyID)` to resolve it back) --
    // a GLOBALLY unique per-body id, same "assign at creation, registry
    // both ways" scheme as the shape-id registry above (b2Body's own user
    // data already holds the owning PhysicsDataObj*, so this can't reuse
    // that slot -- see g_bodiesByObjectId instead).
    int objectId = 0;
    // the acceleration setters store their values directly. The two decay
    // setters store exp(-rate*16.666666), clamped to [0,1], at native
    // offsets +0x24/+0x28. Their consumers are recovered in
    // Constructor defaults confirmed from FUN_00031948, cross-checked
    // against the retail get/set wrappers: maxSpeed +0x34 = 9,999,999.
    float maxSpeed = 9999999.0f;
    // Values after the retail exp(-rate * 16.666666), clamped to [0,1]
    // transform.
    float velocityDecayRate = 1.0f;
    // maxRotateSpeed +0x44 = 9,999,999 in the retail constructor.
    float maxRotateSpeed = 9999999.0f;
    float rotationDecayRate = 1.0f;
    // rotateAcc +0x64 = 999 in the retail constructor.
    float rotateAcc = 999.0f;
    // Ghidra-confirmed real (this session, full PHYSICSDATA method table
    // sweep, FoE PC binary, 0x00514a30-0x00514c58, 47 entries): every one
    // of these is a bare vtable dispatch with the exact same "getdot ->
    // read a float field" / "setdot -> store a float field" shape as
    // maxSpeed/velocityDecayRate above (opaque past the vtable call, but
    // structurally identical to the already-confirmed setters). Not used
    // by the current PekkoAndJon corpus (grepped, 0 call sites for all of
    // these), but implemented for real 1:1 API completeness rather than
    // left as silent no-ops, per explicit direction.
    // Direct getter/setter wrappers map thrustAcc to +0x68 (999) and
    // brakeAcc to +0x3c (999) in the retail constructor.
    float thrustAcc = 999.0f;
    float brakeAcc = 999.0f;
    // Retail per-frame excess-speed retention factors used by
    // FUN_00031f20/FUN_00031d50.
    float maxVelocityDecayRate = 0.0f;
    float maxRotationDecayRate = 0.0f;
    // Retail movement accumulators and per-frame guard flags. The portable
    // representation follows the native offsets and reset order documented
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
    // State written by the retail rotateTo/stopRotatingTo wrappers and
    // consumed by FUN_00031f20.
    bool rotateToActive = false;
    float rotateToX = 0.0f;
    float rotateToY = 0.0f;
    int rotateToSign = 0;
    // Retail's setDir/getDir use body angle when rotation is enabled, and
    // this local normalized vector when fixed rotation is enabled.
    float dirX = 1.0f, dirY = 0.0f;
};

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
    return b2Vec2(std::cos(angle), std::sin(angle));
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
// tick. Parallel to g_bodiesByObjectId (which only holds the script-visible
// gmVariable handles) because the integrator needs the raw struct.
std::vector<PhysicsBodyData*> g_allPhysicsBodies;
std::vector<b2Body*> g_deferredBodyDestructions;

// type is ALWAYS b2_dynamicBody -- confirmed by Ghidra disassembly of the
// real createBody (0x00092428) and its callees this session: the only
// script-facing "type"-shaped argument (the single-bool overload, e.g.
// EntityBase.gm's `createBody(false)`) actually controls fixedRotation,
// not body type (see below) -- and there is no other script API anywhere
// in the corpus that creates or queries a body type at all (grepped for
// setBodyType/isStatic/isDynamic/kinematic -- nothing). Retail's own
// "immovable ground" comes from zero fixture density on a dynamic body,
// exactly as represented by the retail scripts and Box2D's mass data.
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
    data->objectId = (int)g_bodiesByObjectId.size();
    g_bodiesByObjectId.push_back(data->selfVar);
    g_allPhysicsBodies.push_back(data);
    return data->selfVar;
}

// PHYSICSBODY_GetBodyByID(id) -- confirmed real, a global (not type-scoped)
// native function -- reverse lookup for PhysicsBody.getObjectID's id.
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

// --- global shape-ID registry --------------------------------------------
// 0x0008dccc): a LOCAL per-body shape index is resolved to a SEPARATE
// integer field already stored on the native shape struct, not just
// echoed back -- and Physics.isShapeSensor/getBodyFromShapeID/
// integer with no body/owner context at all, confirming shape ids are
// globally, not per-body, unique. Assigned sequentially at fixture-
// creation time (appendShapes) and stashed directly on the b2Fixture's own
// user data so id<->fixture is O(1) both ways.
std::vector<b2Fixture*> g_shapesByGlobalId; // index 0 reserved/invalid
int RegisterShapeGlobalId(b2Fixture* a_fixture) {
    if (g_shapesByGlobalId.empty()) g_shapesByGlobalId.push_back(nullptr); // reserve id 0
    int id = (int)g_shapesByGlobalId.size();
    g_shapesByGlobalId.push_back(a_fixture);
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
int GM_CDECL gmf_shape_isSensor(gmThread* a_thread) {
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
gmFunctionEntry s_physicsShapeLib[] = {
    {"isSensor", gmf_shape_isSensor, nullptr},
    {"getIsSensor", gmf_shape_isSensor, nullptr},
    {"setIsSensor", gmf_shape_setIsSensor, nullptr},
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
        RemovePhysicsJointsForBody(body);
        GetPhysicsWorld().DestroyBody(body);
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
    return GM_OK;
}
// getVelRef -- confirmed real (Player.gm's DECISION_STATEID dead-fall
// branch: `local vel=mypb.getVelRef(); vel->y=vel.y-gravity*msElapsed;`,
// with no separate setVel() call afterward in that one call site), entirely
// unregistered before this -- same hard "getdot failed" class of bug as
// applyForce above, aborting the whole calling function every time it was
// The vector2D proxy routes component reads/writes to Box2D while retaining
// the normal vector2D type expected by the rest of the VM.
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
// applyForce(force, offset) -- confirmed real (Player.gm's CHEAT/fly-mode
// state: `parent.getMainBody().applyForce(v, zero);`), entirely unregistered
// before this -- calling it on a real PhysicsBody userobject is a hard
// "getdot failed" GM error (see doesRotationResponse's header comment for
// the same class of bug), aborting whatever function called it. Maps
// directly onto Box2D's own ApplyForce(force, worldPoint, wake) -- offset
// is a body-local point, so it's applied relative to the body's current
// world position, matching every real call site's `(force, zero)`/
// `(force, someLocalOffset)` shape.
int GM_CDECL gmf_pb_applyForce(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    float fx, fy, ox, oy;
    if (!GetVector2D(m, a_thread->Param(0), fx, fy)) return GM_EXCEPTION;
    if (!GetVector2D(m, a_thread->Param(1), ox, oy)) { ox = 0.0f; oy = 0.0f; }
    b2Vec2 worldPoint = b->body->GetPosition() + b2Vec2(ox, oy);
    b->body->ApplyForce((kPhysicsMillisecondsPerSecond * kPhysicsMillisecondsPerSecond) *
                        b2Vec2(fx, fy),
                        worldPoint, true);
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
        // Retail also marks native bytes +0x5c and associated object +0x40
        // in this branch. Their consumers are not mapped yet (see
        // Physics_Movement_Analysis.md), so don't guess at Box2D state.
        b->body->SetTransform(b->body->GetPosition(), std::atan2(dir.y, dir.x));
    }
    return GM_OK;
}
// Retail setter helpers store exp(-float(rate*16.666666)), clamped to
// [0,1]; the per-frame consumers are FUN_00031f20/FUN_00031d50.
constexpr float kDecayRateTickMs = 16.666666f;
float ComputeDecayFactor(float a_rate) {
    float f = std::exp(-a_rate * kDecayRateTickMs);
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return f;
}
// Retail FUN_00031f20 consumes this accumulator once per update. The guard
// ensures repeated thrust calls during one script frame accumulate once.
int GM_CDECL gmf_pb_thrust(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->thrustFlag) { b->thrustFlag = true; b->thrustAccum += b->thrustAcc; }
    return GM_OK;
}
// Retail wrappers dispatch through the PhysicsShape method table. Their
// descriptor targets are audited in Physics_Movement_Analysis.md.
int GM_CDECL gmf_pb_reverseThrust(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->reverseFlag) { b->reverseFlag = true; b->thrustAccum -= b->thrustAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pb_brake(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    // Retail's brake wrapper sets native byte +0x5d; FUN_00031f20 consumes
    // brakeAcc while this flag is set.
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
// Retail PhysicsShape.computeAngularVel reads two numeric arguments and calls
// FUN_00031d50, but the native GM wrapper discards that helper's scalar result
// and pushes no value. Keep the API's observable behavior: consume the args,
// mutate no body state, and return no GM value.
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
int GM_CDECL gmf_pb_getRadius(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->radius);
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
        else
            radius = shape->m_radius;
    }
    a_thread->PushFloat(radius);
    return GM_OK;
}
int GM_CDECL gmf_pb_setAllowSleeping(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(allowed, 0, 0);
    b->body->SetSleepingAllowed(allowed != 0);
    return GM_OK;
}
// setAsCircle(radius) -- confirmed real (Ghidra: retail FUN_0043be80 @
// 0x0043be80, registered in the PhysicsBody table at 0x00514bb0, right next
// to setAsRect/getRadius). Real callers: Player.gm:16, Point.gm:332,
// Thing.gm:232 (all constructor-time), AND Thing.gm:384 which calls it
// EVERY FRAME during EXPLODING_STATEID to grow a blast radius
// (`this.PhysicsData.setAsCircle(3 + ...)`) -- so unlike appendShapes
// (additive), this must replace the body's existing shape outright rather
// than stack a new fixture on each call. Was completely unbound (silent
// gmCallNativeFunction failure, matching this codebase's convention of null
// no-ops on missing script members), so the player/point/blast never had
// ANY collision shape -- the whole PekkoAndJon collision layer.
int GM_CDECL gmf_pb_setAsCircle(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(radius, 0, 0.0f);
    float r = std::max(radius, 0.001f); // same degenerate-shape guard as gmf_addCircle

    // Preserve the existing fixture's filter (setCollisionCategory/
    // setCategoriesToCollideWith) across the replacement if one exists --
    // Thing.gm's per-frame blast-radius growth calls this repeatedly on a
    // body whose category was already set once at construction.
    b2Filter filter;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
    fx->SetSensor(b->isSensor || !b->isCollidable);
    RegisterShapeGlobalId(fx);
    b->radius = r;
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
// isCircle()/getWidth()/getHeight() -- confirmed real (Ghidra: retail
// FUN_0043c150/FUN_0043b8c0/FUN_0043b930, same PhysicsBody table), real
// caller VideoHelper.gm's VIDEOHELPER_DrawBoundingBox (called from
// PekkoAndJon/Main.gm:258, gated behind the in-game `showingBoundingBox`
// debug toggle -- reachable, not dead code). Mapped generically off the
// body's first fixture's AABB rather than a stored shape-specific
// width/height field (retail's internal PhysicsShape representation isn't
// recoverable past the opaque vtable call), which gives the same values
// setAsCircle/addBox/addPolygon shapes would produce for a bounding-box
// debug draw.
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
    b2Fixture* f = b->body->GetFixtureList();
    if (!f) { a_thread->PushFloat(0.0f); return GM_OK; }
    b2AABB aabb;
    f->GetShape()->ComputeAABB(&aabb, b2Transform(b2Vec2(0, 0), b2Rot(0)), 0);
    a_thread->PushFloat(aabb.upperBound.x - aabb.lowerBound.x);
    return GM_OK;
}
int GM_CDECL gmf_pb_getHeight(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    if (!f) { a_thread->PushFloat(0.0f); return GM_OK; }
    b2AABB aabb;
    f->GetShape()->ComputeAABB(&aabb, b2Transform(b2Vec2(0, 0), b2Rot(0)), 0);
    a_thread->PushFloat(aabb.upperBound.y - aabb.lowerBound.y);
    return GM_OK;
}
int GM_CDECL gmf_pb_appendShapes(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_USER_PARAM(ShapeDefCollection*, g_physicsShapeDefType, sd, 0);
    // Retail EBOOT 0x32340 returns body->shapeCount - 1 after appending,
    // including when the collection contains several shapes.
    for (auto& def : sd->fixtureDefs) {
        b2Fixture* fx = b->body->CreateFixture(&def);
        RegisterShapeGlobalId(fx);
        if (def.shape->GetType() == b2Shape::e_circle) {
            b->radius = ((b2CircleShape*)def.shape)->m_radius;
        }
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
        b2Filter filter = f->GetFilterData();
        filter.categoryBits = (uint16)category;
        f->SetFilterData(filter);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_setCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(mask, 0, -1);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = f->GetFilterData();
        filter.maskBits = (uint16)mask;
        f->SetFilterData(filter);
    }
    return GM_OK;
}
// getCollisionCategory/getCategoriesToCollideWith -- Ghidra-confirmed real
// (PHYSICSDATA table sweep, this session), simple int-field getters
// mirroring the setters directly above. Reads the first fixture's filter
// (matches how setCollisionCategory/setCategoriesToCollideWith apply to
// ALL fixtures uniformly -- there's no per-fixture distinction anywhere
// else in this API). 0 real call sites in the current corpus.
int GM_CDECL gmf_pb_getCollisionCategory(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt(f ? (int)f->GetFilterData().categoryBits : 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_getCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt(f ? (int)f->GetFilterData().maskBits : -1);
    return GM_OK;
}
// setCollisionGroup(group)/getCollisionGroup() -- confirmed real and
// (0x00090394, `FUN_00032470(self, 0)` -- the hardcoded literal 0 second
// arg means this always resolves the body's first/only fixture, matching
// the "apply to every fixture uniformly" pattern setCollisionCategory/
// setCategoriesToCollideWith already use). No decompiled setter exists in
// this corpus, but game_data/Platformer/EntityBase.gm:914 -- part of
// EVERY entity's configure() step, live and uncommented -- calls
// `this.PhysicsData.setCollisionGroup((this.configureData.uid +
// RESERVED_UIDS)*-1);`. The value is forced negative, which is exactly
// Box2D's b2Filter::groupIndex semantic (nonzero group index: negative =
// never collide with same group, positive = always collide, independent
// of and orthogonal to categoryBits/maskBits) -- a THIRD filter field,
// distinct from setCollisionCategory/setCategoriesToCollideWith, so this
// must not be aliased onto those. Was entirely missing (grepped, zero
// matches anywhere in the corpus of binding files), meaning every entity
// hit "getdot failed" during configure() before this fix.
int GM_CDECL gmf_pb_setCollisionGroup(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(group, 0, 0);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
        b2Filter filter = f->GetFilterData();
        filter.groupIndex = (int16)group;
        f->SetFilterData(filter);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getCollisionGroup(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Fixture* f = b->body->GetFixtureList();
    a_thread->PushInt(f ? (int)f->GetFilterData().groupIndex : 0);
    return GM_OK;
}
// 0x0008fafc) and confirmed-live: `this.mainpb.isSleeping()`
// (SpiralLightBulb.gm:1005,1013; SpiralStaircaseLightBulb.gm:183) and
// `PHYSICSBODY_GetBodyByID(this.endBody).isSleeping()` (Chain2.gm:402,509,
// 786). Retail's sleep bit (flags bit 2) is set while asleep and cleared on
// wake, the inverse of Box2D's awake bit. Return the inverse of Box2D's
// IsAwake().
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
// PhysicsShape.putToSleep is the single-body counterpart of
// PhysicsData.putToSleep. Retail clears velocity/forces and marks the
// underlying body asleep; Box2D's SetAwake(false) performs that transition.
int GM_CDECL gmf_pb_putToSleep(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b->body->SetAwake(false);
    return GM_OK;
}
int GM_CDECL gmf_pb_setIsSensor(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    b->isSensor = (v != 0);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext())
        f->SetSensor(b->isSensor || !b->isCollidable);
    return GM_OK;
}
int GM_CDECL gmf_pb_isSensor(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(b->isSensor ? 1 : 0);
    return GM_OK;
}
// setIsCollidable(false): no dedicated Box2D concept, and no decompiled
// body found (not in the retail symbol index). Same mapping already used
// (and accepted) for the PhysicsData sibling, gmf_pd_setIsCollidable below:
// "not collidable" -> sensor-mode fixtures, so contacts/overlap are still
// detected (getCollidingEntities etc still work) but Box2D stops applying
// collision response. Composes with setIsSensor via the same OR.
int GM_CDECL gmf_pb_setIsCollidable(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 1);
    b->isCollidable = (v != 0);
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext())
        f->SetSensor(b->isSensor || !b->isCollidable);
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
int GM_CDECL gmf_pb_setRestitution(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(r, 0, 0.0f);
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetRestitution(r);
    return GM_OK;
}
// setShapeDensity(shapeIndex, density) -- confirmed real 2-arg signature
// (every real call site in the corpus, e.g. Player.gm's
// `mypb.setShapeDensity(0,parent.baseDensity)`, passes a LOCAL shape index
// first and the actual density second). Apply density to just that
// fixture and recalculate the still-dynamic body's mass. Player.gm changes
// body density at state transitions, so preserving dynamic body type lets
// positive densities restore motion. Applies to just the one
// fixture at that local index, matching the real per-shape-index API
// shape (falls back to "all fixtures" if no such index exists, so a
// caller passing an out-of-range index still gets a sane no-crash result
// rather than silently doing nothing).
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
// setIsShapeSensor(shapeIndex, bool)/setShapeCategoriesToCollideWith(
// shapeIndex, mask)/getShapeCategoriesToCollideWith(shapeIndex) --
// confirmed real, decompiled per-fixture (index-addressed) siblings of the
// "all fixtures" setIsSensor/setCategoriesToCollideWith/
// 1761-1832, 0x000912d8/0x00091bec/0x00091d8c -- getShapeCategoriesToCollideWith
// reads a ushort at the resolved fixture struct's +0x3e, matching Box2D's
// own b2Filter::maskBits field). Same "falls back to all fixtures on an
// out-of-range index" no-crash convention as setShapeDensity above, for
// the same reason (no confirmed decompiled out-of-range behavior to match
// against, and every real call site the corpus has for setShapeDensity
// uses a valid index).
int GM_CDECL gmf_pb_setIsShapeSensor(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_INT_PARAM(isSensor, 1, 0);
    b2Fixture* fx = FixtureAtShapeIndex(b->body, shapeIndex);
    if (fx) fx->SetSensor(isSensor != 0 || !b->isCollidable);
    return GM_OK;
}
int GM_CDECL gmf_pb_setShapeCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_INT_PARAM(mask, 1, -1);
    b2Fixture* fx = FixtureAtShapeIndex(b->body, shapeIndex);
    if (fx) {
        b2Filter filter = fx->GetFilterData();
        filter.maskBits = (uint16)mask;
        fx->SetFilterData(filter);
    }
    return GM_OK;
}
int GM_CDECL gmf_pb_getShapeCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeIndex, 0, 0);
    b2Fixture* fx = FixtureAtShapeIndex(b->body, shapeIndex);
    if (fx) { a_thread->PushInt((int)fx->GetFilterData().maskBits); return GM_OK; }
    a_thread->PushInt(-1);
    return GM_OK;
}
// 1436-1452, 0x0008ff6c, reads flag bit 8) counterpart to the already-
// implemented setter (setIsUsingContinuousPhysics, confirmed live:
// Player.gm:66). No confirmed-live call site for the getter itself was
// found, but it's a trivial, direct proxy for Box2D's own IsBullet() --
// the exact flag setIsUsingContinuousPhysics's SetBullet call sets.
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
// 1934-1959, 0x000922dc, reads body fields +0x24/+0x28 directly, i.e. a
// bare field read with no formula) -- Box2D's own GetWorldCenter() is the
// direct real equivalent (the body's centroid in world space, matching
// what those two fields would hold in a real Box2D-backed body).
int GM_CDECL gmf_pb_getCenterOfMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 c = b->body->GetWorldCenter();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), c.x, c.y));
    return GM_OK;
}
// Retail PhysicsBody.setMass (EBOOT 0x91fe4 -> 0x33c60) passes a mass
// record to Box2D's SetMassData equivalent at 0x156720. The record keeps
// the current local center and uses mass + center.x^2 + center.y^2 as its
// origin inertia. Fixture density is unchanged.
void SetRetailBodyMass(b2Body* a_body, float a_mass) {
    if (!a_body || a_body->GetMass() == a_mass) return;
    if (a_mass <= 0.0f) {
        // Box2D 2.4 SetMassData clamps a dynamic body's requested zero mass
        // to one. Retail's Box2D keeps it at zero. ResetMassData preserves
        // zero mass; restore fixture densities afterward because the retail
        // body override does not change them.
        std::vector<std::pair<b2Fixture*, float>> densities;
        for (b2Fixture* fixture = a_body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
            densities.emplace_back(fixture, fixture->GetDensity());
            fixture->SetDensity(0.0f);
        }
        a_body->ResetMassData();
        for (const auto& [fixture, density] : densities) fixture->SetDensity(density);
        return;
    }
    b2MassData data;
    a_body->GetMassData(&data);
    data.mass = a_mass;
    data.I = a_mass + b2Dot(data.center, data.center);
    a_body->SetMassData(&data);
}
int GM_CDECL gmf_pb_setMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(mass, 0, 1.0f);
    SetRetailBodyMass(b->body, mass);
    return GM_OK;
}
// getMass -- Ghidra-confirmed real (PHYSICSDATA table sweep, this
// session), a getter counterpart to setMass above. Reads Box2D's own
// computed body mass directly (b2Body::GetMass(), which reflects whatever
// setMass's density writes above resolve to) rather than a separately
// tracked field, so it stays consistent regardless of how mass was set.
// 0 real call sites in the current corpus.
int GM_CDECL gmf_pb_getMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->body->GetMass());
    return GM_OK;
}
// Retail appendShapes returns shapeCount-1 (EBOOT 0x32340), and its index
// lookup walks the body's shape list from the head (0x32470). Box2D 2.4
// prepends fixtures, so walk its list backwards to preserve creation order.
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
int GM_CDECL gmf_pb_getShapeObjectID(gmThread* a_thread) {
    // 0x0008dccc): resolves the LOCAL per-body shape index (the argument)
    // to a GLOBALLY unique id stored on the native shape struct -- NOT an
    // echo of the argument, which was the previous (unconfirmed) guess.
    // Matters beyond just this getter: Physics.isShapeSensor/
    // getBodyFromShapeID/getEntityFromShapeID all take that same bare
    // global id with no body context, so echoing the local index back
    // would make every body's "shape 0" collide under the same id.
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
// getShapeWorldCenterPos(localIndex) -- confirmed real, live call site
// SingleCircleGround.gm:53: `local myPos=this.PhysicsData.getBody(0).
// getShapeWorldCenterPos(0);` (positions an edit button relative to the
// ground shape). Was entirely unbound -- hard "getdot failed" aborting
// removeAllEditButtons()'s caller before any edit buttons could be placed.
// Same local-index fixture walk as getShapeObjectID above; center computed
// via Box2D's own per-fixture AABB midpoint (works uniformly across every
// shape type this engine builds -- circle, polygon, edge chain -- without
// needing a shape-type switch).
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
// disableRestitutionThisFrame() -- REAL, now implemented. Retail body
// FUN_00034084, which decompiles to:
//     contact = body->contactList;            // *(this+0x14)+0x6c
//     while (contact) {
//         *(u8*)(contact + 0x45) = 1;         // per-contact flag byte
//         contact = *(int*)(contact + 0x20);  // next in list
//     }
// i.e. mark EVERY contact on this body, let the solver honour it this
// step. Not a PreSolve-only action on one live contact, as this file
// previously assumed.
//
// Live call site Player.gm:4488-4489 (landing-impact handling):
//     this.mainpb.disableRestitutionThisFrame();
//     sci.disableContact(i);
//     ...setVel(v.x, v.y*(1-MATH_ABS(dn.y)));
// This was stubbed as a "safe no-op" on the reasoning that the setVel on
// the next line does the real work. That reasoning was wrong: zeroing the
// velocity component does not stop Box2D's solver applying a restitution
// impulse for the same contact in the same step, so landings kept a bounce
// retail does not have.
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
// These wrappers access native fields directly; FUN_00031f20 consumes both
// acceleration values during movement updates.
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
// setMaxVelocityDecayRate/setMaxRotationDecayRate -- Ghidra-confirmed real
// (PHYSICSDATA table sweep, this session), bare field stores; see
// PhysicsBodyData's own header comment on these two fields for why they
// aren't wired into ComputeDecayFactor (unconfirmed relationship past the
// opaque vtable call). 0 real call sites in the current corpus.
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
// setAsRect(width, height) -- Ghidra-confirmed real (retail FUN_0043bdb0 @
// 0x0043bdb0, registered right next to setAsCircle in the same table),
// same "replace the body's shape outright" semantics as setAsCircle (see
// that function's own header comment for the full reasoning -- mirrored
// here for the box case). 0 real call sites in the current corpus, but
// implemented for 1:1 completeness alongside setAsCircle rather than left
// as the one asymmetric gap in an otherwise-complete shape-setter pair.
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
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
    fx->SetSensor(b->isSensor || !b->isCollidable);
    RegisterShapeGlobalId(fx);
    b->radius = 0.0f;
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
// getObjectID() -- confirmed real (Player.gm: `local newAnchorID=
// pb.getObjectID(); this.anchorBodyID=newAnchorID;`, later resolved back
// via the global PHYSICSBODY_GetBodyByID) -- see PhysicsBodyData::objectId
// and g_bodiesByObjectId above.
int GM_CDECL gmf_pb_getObjectID(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(b->objectId);
    return GM_OK;
}
int GM_CDECL gmf_pb_getEntity(gmThread* a_thread); // defined below PhysicsDataObj, registered here
int GM_CDECL gmf_pb_setIsUsingContinuousPhysics(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    b->body->SetBullet(v != 0);
    return GM_OK;
}
int GM_CDECL gmf_pb_getAABB(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    b2AABB aabb;
    bool first = true;
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) {
        for (int i = 0; i < fx->GetShape()->GetChildCount(); ++i) {
            b2AABB shapeAabb;
            fx->GetShape()->ComputeAABB(&shapeAabb, b->body->GetTransform(), i);
            if (first) { aabb = shapeAabb; first = false; }
            else aabb.Combine(shapeAabb);
        }
    }
    if (first) { b2Vec2 p = b->body->GetPosition(); aabb.lowerBound = p; aabb.upperBound = p; }
    gmTableObject* t = m.AllocTableObject();
    // Lowercase field names -- confirmed real by exhaustive corpus usage:
    // every one of the ~220 real `.getAABB().minx/.maxx/.miny/.maxy`
    // accesses across game_data uses this exact lowercase form, zero use
    // the camelCase minX/maxX this used to push (e.g. EntityPacks/
    // SwayingTree.gm:290's `aabb.maxx = MATH_MAX(right.x, aabb.maxx)`
    // read back null under the old casing -> blank strict-param exception
    // in MATH_MAX -> that entity's whole configure() step failed every
    // frame, which is a load-bearing step for every other entity's own
    // screen-extent registration in the same level).
    t->Set(&m, "minx", gmVariable(aabb.lowerBound.x));
    t->Set(&m, "miny", gmVariable(aabb.lowerBound.y));
    t->Set(&m, "maxx", gmVariable(aabb.upperBound.x));
    t->Set(&m, "maxy", gmVariable(aabb.upperBound.y));
    a_thread->PushTable(t);
    return GM_OK;
}

// raycast(startPos, endPos, resultTable) -- confirmed real, single call
// site (Player.gm:1696's hang-time anchor teleport): a raycast against
// JUST this body's own fixtures (not the whole world), filling
// resultTable.lambda/nx/ny on a hit -- same result-table shape as
// Physics.raycastOne (physics_global_bindings.cpp), which this mirrors.
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

// --- real ENTITYLISTREFCONTAINER-shaped colliding-entities iterator -----
// Matches the idiom real scripts use
// (`for(it=x.getCollidingEntities();it.isValid();it.goToNext())`). Backed
// by real Box2D contacts -- see gmf_pd_getCollidingEntities below.
struct EntityIterData {
    std::vector<gmVariable> entities;
    size_t index = 0;
};
int GM_CDECL gmf_iter_isValid(gmThread* a_thread) {
    EntityIterData* it = (EntityIterData*)a_thread->ThisUserCheckType(g_entityIterType);
    a_thread->PushInt((it && it->index < it->entities.size()) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_iter_goToNext(gmThread* a_thread) {
    EntityIterData* it = (EntityIterData*)a_thread->ThisUserCheckType(g_entityIterType);
    if (it && it->index < it->entities.size()) ++it->index;
    return GM_OK;
}
int GM_CDECL gmf_iter_getEntity(gmThread* a_thread) {
    EntityIterData* it = (EntityIterData*)a_thread->ThisUserCheckType(g_entityIterType);
    if (it && it->index < it->entities.size()) a_thread->Push(it->entities[it->index]);
    else a_thread->PushNull();
    return GM_OK;
}
// PhysicsBody.getCollisionInfo -- no per-contact info (point/normal/
// impulse) tracked; honest stub.
GE3_REMOVED(gmf_pb_getCollisionInfo, "PhysicsBody.getCollisionInfo")
gmFunctionEntry s_entityIterLib[] = {
    {"isValid", gmf_iter_isValid, nullptr},
    {"goToNext", gmf_iter_goToNext, nullptr},
    {"get", gmf_iter_getEntity, nullptr},
    {"getEntity", gmf_iter_getEntity, nullptr},
};
void GM_CDECL EntityIterGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (EntityIterData*)a_object->m_user;
}

gmFunctionEntry s_physicsBodyLib[] = {
    {"getPos", gmf_pb_getPos, nullptr},
    {"getPosVec", gmf_pb_getPos, nullptr},
    {"setPos", gmf_pb_setPos, nullptr},
    {"getVel", gmf_pb_getVel, nullptr},
    {"setVel", gmf_pb_setVel, nullptr},
    {"getSpeed", gmf_pb_getSpeed, nullptr},
    {"setSpeed", gmf_pb_setSpeed, nullptr},
    {"getDir", gmf_pb_getDir, nullptr},
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
    {"setAllowSleeping", gmf_pb_setAllowSleeping, nullptr},
    {"setAsCircle", gmf_pb_setAsCircle, nullptr},
    {"setAsRect", gmf_pb_setAsRect, nullptr},
    {"isCircle", gmf_pb_isCircle, nullptr},
    {"isRect", gmf_pb_isRect, nullptr},
    {"getWidth", gmf_pb_getWidth, nullptr},
    {"getHeight", gmf_pb_getHeight, nullptr},
    {"appendShapes", gmf_pb_appendShapes, nullptr},
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
    {"getAngularVel", gmf_pb_getAngularVel, nullptr},
    {"setAngularVel", gmf_pb_setAngularVel, nullptr},
    {"setIsCollidable", gmf_pb_setIsCollidable, nullptr},
    {"isCollidable", gmf_pb_isCollidable, nullptr},
    {"setFriction", gmf_pb_setFriction, nullptr},
    {"setRestitution", gmf_pb_setRestitution, nullptr},
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
    {"getNumShapeVerts", gmf_pb_getNumShapeVerts, nullptr},
    {"getShapeVert", gmf_pb_getShapeVert, nullptr},
    {"getShapeWorldVert", gmf_pb_getShapeWorldVert, nullptr},
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
    {"setIsUsingContinuousPhysics", gmf_pb_setIsUsingContinuousPhysics, nullptr},
    {"getAABB", gmf_pb_getAABB, nullptr},
    {"raycast", gmf_pb_raycast, nullptr},
    {"getCollisionInfo", gmf_pb_getCollisionInfo, nullptr}, // no collision-info tracked, see header
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

// getEntity() -- confirmed real (Player.gm: `apb.getEntity().isHangable()`
// on a body resolved via PHYSICSBODY_GetBodyByID) -- the entity table that
// owns this body's PhysicsData, same lookup as PhysicsData.
// getCollidingEntities uses (b2Body's user data = owning PhysicsDataObj*).
int GM_CDECL gmf_pb_getEntity(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    PhysicsDataObj* owner = (PhysicsDataObj*)(uintptr_t)b->body->GetUserData().pointer;
    if (owner && owner->ownerEntity.m_type == GM_TABLE) a_thread->Push(owner->ownerEntity);
    else a_thread->PushNull();
    return GM_OK;
}

// Every "delegate to body 0" method below mirrors real usage seen calling
// the same method name directly on PhysicsData (e.g.
// `this.player.PhysicsData.setSpeed(0)`) as well as via `getBody(0)` --
// Player.gm treats them as equivalent for the main body.
//
// Auto-creates body 0 on first use (this session): confirmed real for FoE
// -- PekkoAndJon/Player.gm/Point.gm/Thing.gm call `this.PhysicsData.
// setAsCircle(...)`/`setPos(...)`/etc directly, NEVER calling createBody()
// first anywhere in the corpus (grepped). Sound Shapes' own Platformer
// corpus (per this file's existing comments) always calls createBody()
// explicitly before touching the body, so for that game bodies is already
// non-empty by the time MainBody is reached and this auto-create path
// never triggers -- safe for both conventions. Uses createBody()'s own
// no-args default (fixedRotation=false, awake=true).
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

// createBody([bodyDef]) -- confirmed real (Platformer/EntityPacks/
// GroundResizeNoRotate.gm: `global StandardBodyDef; StandardBodyDef.x=
// this.configureData.posX; StandardBodyDef.y=this.configureData.posY;
// local pb=this.PhysicsData.getBody(this.PhysicsData.createBody(StandardBodyDef));`)
// -- the optional table's x/y fields set the new body's initial world
// position; this was previously ignored entirely (every body started at
// world origin), which combined with ground shapes being authored as
// vertices RELATIVE TO the entity's own position (`vertices[i]-myPos`
// just before this call) meant every ground piece's fixture, even once
// addPolygon existed, would have appeared shifted to the wrong place
// (anchored at (0,0) instead of its real configured position).
// fixedRotation/sleeping (awake) handling below matches the real
// decompiled body exactly (Ghidra, this session): createBody's native
// (0x00092428) builds a ~40-byte body-def-equivalent struct one of two
// ways depending on the call shape --
//   - createBody({table}): reads table.x/y (already had this) AND
//     table.sleeping/table.fixedRotation via 4 confirmed real
//     FUN_000f1a0c (table-get-with-default) calls -- was dropping the
//     latter two entirely.
//   - createBody() [no args] or createBody(bool): builds the SAME struct
//     inline (FUN_0002ea74, no table involved) with a `cntlzw`+`rlwinm`
//     "value == 0" idiom on the incoming type byte writing straight into
//     the struct's fixedRotation slot -- i.e. the single-bool overload's
//     argument controls fixedRotation, INVERTED (`createBody(false)` ->
//     fixedRotation TRUE; `createBody(true)`/no-arg, whose default type
//     byte is 1 -> fixedRotation FALSE). This is NOT a body-type selector
//     (see MakePhysicsBody's own header comment -- confirmed there's no
//     such thing anywhere in the real API).
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
// 0x0009291c): 0-or-1-arg, index<0 or omitted means "remove all". Real
// call site (EntityBase.gm:691, entity re-load path: `this.PhysicsData.
// removeBody(-1);`) tears down every body this entity owns before
// rebuilding them fresh. Chain2.gm also passes a PhysicsBody object ID here
// while shortening its linked chain, so resolve an owned object ID before
// accepting the legacy positional form. Erasing from d->bodies (after
// UnrootGmVariable).  Destruction cannot wait for collection: scripts keep
// body references in entity state while checkpoints rebuild vehicle bodies,
// and those references can be read before the next collection pass.
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
// getCollidingEntities -- real Box2D contacts, not a stub: walks every
// body this PhysicsData owns, keeps only currently-touching contacts
// (b2Contact::IsTouching(), so overlapping-AABB-but-not-actually-touching
// broad-phase pairs are excluded), resolves the other side's b2Body user
// data back to ITS owning PhysicsDataObj (set in gmf_pd_createBody) and
// then to that PhysicsData's ownerEntity, de-duplicating (two fixtures
// between the same pair of entities would otherwise appear twice).
int GM_CDECL gmf_pd_getCollidingEntities(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    std::vector<gmVariable> found;
    for (gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* b = GetBodyData(m, bodyVar);
        if (!b || !b->body) continue;
        for (b2ContactEdge* edge = b->body->GetContactList(); edge; edge = edge->next) {
            if (!edge->contact->IsTouching()) continue;
            PhysicsDataObj* otherData = (PhysicsDataObj*)(uintptr_t)edge->other->GetUserData().pointer;
            if (!otherData || otherData == d) continue;
            if (otherData->ownerEntity.m_type != GM_TABLE) continue;
            bool already = false;
            for (gmVariable& e : found) {
                if (e.m_value.m_ref == otherData->ownerEntity.m_value.m_ref) { already = true; break; }
            }
            if (!already) found.push_back(otherData->ownerEntity);
        }
    }
    gmUserObject* iterObj = m.AllocUserObject(new EntityIterData{std::move(found), 0}, g_entityIterType);
    a_thread->Push(gmVariable(iterObj));
    return GM_OK;
}

// allocates a list container, populates it with joints attached to this
// body, and returns an iterator over it (matching the real
// ENTITYLISTREFCONTAINER-shaped idiom, e.g. EntityBase.gm:1308's `for(it=
// this.PhysicsData.getJointsOnMe();it.isValid();it.goToNext()){ local
// j=it.get(); Physics.deleteJoint(j); }`). A joint registry supplies the
// returned stable handles while Box2D owns and solves constraints.
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

// getCollisionInfo(otherEntity) -- real Box2D contact/manifold data, not a
// stub. Usage (Platformer/Player.gm) confirms a 3-level real API:
//   ci = this.PhysicsData.getCollisionInfo(e)
//   ci.isBodyColliding(pb)
//   for (it=ci.getCollidingShapeIDList(reusableIntContainer); it.isValid(); it.goToNext())
//       sci = ci.getShapeCollisionInfo(it.get())
//       sci.getNumOfMyShapesColliding() / getMyContactShapeID(i) / getContactNormal(i)
// "shape ID" matches PhysicsBody.getShapeObjectID's existing confirmed
// contract (a fixture's creation-order index within its OWN body's
// fixture list) -- computed here by walking GetFixtureList(). The
// getCollidingShapeIDList() accepts the reusable IntListRefContainer
// created by INTLISTREFCONTAINER() and repopulates it with shape IDs.
gmType g_collisionInfoType = -1;
gmType g_shapeCollisionInfoType = -1;
gmType g_intIterType = -1;

// Up to 2 real Box2D manifold contact points per (my shape, other shape)
// touching pair -- confirmed real (Physics.getContactPos(myShapeIndex,
// pointIndex), e.g. Player.gm:2050's `cp=sci.getContactPos(j,i)` where `i`
// genuinely varies, not always 0).
struct ContactPoints {
    b2Vec2 pts[2] = {b2Vec2(0, 0), b2Vec2(0, 0)};
    int count = 0;
};
struct ShapeContactEntry {
    int otherShapeId = -1;
    std::vector<int> myShapeIds;
    std::vector<b2Vec2> normals; // world-space, pointing from the other shape toward mine ("itToMe")
    std::vector<ContactPoints> points;
    // The live b2Contact each record came from, so disableContact(i) can
    // actually reach it. Retail's Physics.disableContact (0x000982f4)
    // scans its own vector for the record whose index matches and ORs a
    // flag into that contact; this is the same lookup.
    std::vector<b2Contact*> contacts;
};
struct CollisionInfoData {
    std::vector<b2Body*> collidingMyBodies;
    std::vector<ShapeContactEntry> contacts;
    // isAllSensors() backing -- see gmf_ci_isAllSensors's own header comment.
    // Retail FUN_0002e08c walks CollisionInfo's keys (the OTHER shape IDs)
    // and calls FUN_00036eec, which reads that shape's sensor byte. This is
    // true only when every contacted OTHER shape is a sensor. In particular,
    // a note's sensor touching the player's solid body must return false.
    bool allSensors = true;
};
struct ShapeCollisionInfoData {
    std::vector<int> myShapeIds;
    std::vector<b2Vec2> normals;
    std::vector<ContactPoints> points;
    std::vector<b2Contact*> contacts; // parallel to myShapeIds, for disableContact
};
struct IntIterData {
    std::vector<int> values;
    size_t index = 0;
};

int GM_CDECL gmf_INTLISTREFCONTAINER(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* obj = m.AllocUserObject(new IntIterData(), g_intIterType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}

CollisionInfoData* BuildCollisionInfo(gmMachine& a_machine, PhysicsDataObj* a_myData, const gmVariable& a_otherEntityVar) {
    CollisionInfoData* info = new CollisionInfoData();
    if (a_otherEntityVar.m_type != GM_TABLE) return info;
    gmTableObject* otherTable = (gmTableObject*)a_machine.GetObject(a_otherEntityVar.m_value.m_ref);
    if (!otherTable) return info;
    gmVariable pdVar = otherTable->Get(&a_machine, "PhysicsData");
    if (pdVar.m_type != g_physicsDataType) pdVar = otherTable->Get(&a_machine, "__physicsData");
    if (pdVar.m_type != g_physicsDataType) return info;
    gmObject* obj = a_machine.GetObject(pdVar.m_value.m_ref);
    if (!obj) return info;
    PhysicsDataObj* otherData = (PhysicsDataObj*)((gmUserObject*)obj)->m_user;

    for (gmVariable& myBodyVar : a_myData->bodies) {
        PhysicsBodyData* myBody = GetBodyData(a_machine, myBodyVar);
        if (!myBody || !myBody->body) continue;
        for (b2ContactEdge* edge = myBody->body->GetContactList(); edge; edge = edge->next) {
            if (!edge->contact->IsTouching()) continue;
            PhysicsDataObj* otherOwner = (PhysicsDataObj*)(uintptr_t)edge->other->GetUserData().pointer;
            if (otherOwner != otherData) continue;

            b2Contact* contact = edge->contact;
            b2Fixture* fA = contact->GetFixtureA();
            b2Fixture* fB = contact->GetFixtureB();
            bool aIsMine = (fA->GetBody() == myBody->body);
            b2Fixture* myFixture = aIsMine ? fA : fB;
            b2Fixture* otherFixture = aIsMine ? fB : fA;
            if (!otherFixture->IsSensor()) info->allSensors = false;

            b2WorldManifold wm;
            contact->GetWorldManifold(&wm); // normal points from A to B
            b2Vec2 itToMe = aIsMine ? -wm.normal : wm.normal;
            ContactPoints cp;
            cp.count = contact->GetManifold()->pointCount;
            cp.pts[0] = wm.points[0];
            if (cp.count > 1) cp.pts[1] = wm.points[1];

            // Global ids (see RegisterShapeGlobalId), matching the same
            // id-space Physics.isShapeSensor/getBodyFromShapeID/
            // getEntityFromShapeID resolve against.
            int myShapeId = (int)myFixture->GetUserData().pointer;
            int otherShapeId = (int)otherFixture->GetUserData().pointer;

            bool bodyAlready = false;
            for (b2Body* b : info->collidingMyBodies) if (b == myBody->body) { bodyAlready = true; break; }
            if (!bodyAlready) info->collidingMyBodies.push_back(myBody->body);

            ShapeContactEntry* entry = nullptr;
            for (auto& e : info->contacts) if (e.otherShapeId == otherShapeId) { entry = &e; break; }
            if (!entry) { info->contacts.push_back(ShapeContactEntry{otherShapeId, {}, {}, {}, {}}); entry = &info->contacts.back(); }
            entry->myShapeIds.push_back(myShapeId);
            entry->normals.push_back(itToMe);
            entry->points.push_back(cp);
            entry->contacts.push_back(contact);
        }
    }
    // Retail stores per-other-shape entries in an ordered map. Preserve its
    // key iteration order for getCollidingShapeIDList()/getFirstShape... .
    std::sort(info->contacts.begin(), info->contacts.end(),
              [](const ShapeContactEntry& a, const ShapeContactEntry& b) {
                  return a.otherShapeId < b.otherShapeId;
              });
    return info;
}

int GM_CDECL gmf_pd_getCollisionInfo(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    CollisionInfoData* info = BuildCollisionInfo(m, d, a_thread->Param(0));
    // Retail Physics.getCollisionInfo first looks the other PhysicsData up
    // in its active collision map and pushes null on a miss
    // empty CollisionInfo is truthy to GM, so returning one here made every
    // `if (?data.getCollisionInfo(other))` path claim a collision after the
    // bodies had separated.
    if (info->contacts.empty()) {
        delete info;
        a_thread->PushNull();
        return GM_OK;
    }
    gmUserObject* obj = m.AllocUserObject(info, g_collisionInfoType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}
int GM_CDECL gmf_pd_isColliding(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    const int argc = a_thread->GetNumParams();
    if (argc > 1) return GM_EXCEPTION;
    // Retail's no-arg path checks whether its collision collection is
    // nonempty; its one-arg path looks the other PhysicsData up in that
    // collection (FUN_002f6348). The local equivalent scans touching Box2D
    // contacts and compares the stored owner entity. Contact/map update
    // timing still needs a retail tick trace.
    const gmVariable target = argc == 1 ? a_thread->Param(0) : gmVariable::s_null;
    PhysicsDataObj* targetData = nullptr;
    if (target.m_type == g_physicsDataType) {
        gmObject* targetObject = a_thread->GetMachine()->GetObject(target.m_value.m_ref);
        if (targetObject) targetData = (PhysicsDataObj*)((gmUserObject*)targetObject)->m_user;
    }
    bool colliding = false;
    for (const gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* body = GetBodyData(*a_thread->GetMachine(), bodyVar);
        if (!body || !body->body) continue;
        for (b2ContactEdge* edge = body->body->GetContactList(); edge; edge = edge->next) {
            if (!edge->contact || !edge->contact->IsTouching()) continue;
            if (argc == 0) { colliding = true; break; }
            const uintptr_t userData = edge->other->GetUserData().pointer;
            PhysicsDataObj* other = reinterpret_cast<PhysicsDataObj*>(userData);
            if (!other) continue;
            if (target.m_type == GM_TABLE && other->ownerEntity.m_type == GM_TABLE &&
                target.m_value.m_ref == other->ownerEntity.m_value.m_ref) {
                colliding = true;
                break;
            }
            if (targetData && other == targetData) {
                colliding = true;
                break;
            }
        }
        if (colliding) break;
    }
    a_thread->PushInt(colliding ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_ci_isBodyColliding(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    PhysicsBodyData* b = GetBodyData(*a_thread->GetMachine(), a_thread->Param(0));
    bool found = false;
    if (b) for (b2Body* body : info->collidingMyBodies) if (body == b->body) { found = true; break; }
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
// CollisionInfo.isBodyCollidingWithShape(body, globalShapeId): check the
// contact records captured for this entity pair, preserving the distinction
// between a platform's solid and sensor fixtures. Retail dispatches to
// FUN_0002df38 (EBOOT 0x97920) with precisely these two arguments.
int GM_CDECL gmf_ci_isBodyCollidingWithShape(gmThread* a_thread) {
    auto* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    PhysicsBodyData* body = GetBodyData(*a_thread->GetMachine(), a_thread->Param(0));
    GM_INT_PARAM(shapeId, 1, 0);
    bool found = false;
    if (body && body->body) {
        for (const ShapeContactEntry& entry : info->contacts) {
            if (std::find(entry.myShapeIds.begin(), entry.myShapeIds.end(), shapeId) == entry.myShapeIds.end())
                continue;
            for (b2ContactEdge* edge = body->body->GetContactList(); edge; edge = edge->next) {
                b2Contact* contact = edge->contact;
                if (!contact || !contact->IsTouching()) continue;
                b2Fixture* other = contact->GetFixtureA()->GetBody() == body->body
                    ? contact->GetFixtureB() : contact->GetFixtureA();
                if ((int)other->GetUserData().pointer == shapeId) { found = true; break; }
            }
            if (found) break;
        }
    }
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_ci_isShapeColliding(gmThread* a_thread) {
    auto* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(shapeId, 0, 0);
    bool found = false;
    for (const ShapeContactEntry& entry : info->contacts) {
        if (std::find(entry.myShapeIds.begin(), entry.myShapeIds.end(), shapeId) != entry.myShapeIds.end()) {
            found = true;
            break;
        }
    }
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_ci_getCollidingShapeIDList(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
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
int GM_CDECL gmf_ci_getShapeCollisionInfo(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(sid, 0, -1);
    for (auto& e : info->contacts) {
        if (e.otherShapeId == sid) {
            ShapeCollisionInfoData* sci = new ShapeCollisionInfoData{e.myShapeIds, e.normals, e.points, e.contacts};
            gmUserObject* obj = a_thread->GetMachine()->AllocUserObject(sci, g_shapeCollisionInfoType);
            a_thread->Push(gmVariable(obj));
            return GM_OK;
        }
    }
    a_thread->PushNull();
    return GM_OK;
}
// getFirstShapeCollisionInfo() -- no-arg convenience wrapper, confirmed
// real (Player.gm:4398's checkDeath(): `local sci=ci.
// getFirstShapeCollisionInfo(); SYSTEMHELPER_NotifyEvent("KillPlayer",...,
// sci.getContactNormal(0));`) but was entirely unregistered -- calling it
// on a real CollisionInfo userobject is the same hard "getdot failed" class
// of bug as every other missing-binding case this session, aborting
// checkDeath() (and the onEntityCollide/onEntityTouch call chain that leads
// into it) before the real "KillPlayer" event could ever fire: touching a
// DANGER/FIRE ground silently did nothing, no matter how solid/red the
// rendering looked. Just the first entry in the same `info->contacts` list
// getShapeCollisionInfo(id) already searches -- there's no real ordering
// guarantee beyond "whichever contact was recorded first", matching a
// plain "first" accessor rather than a specific shape lookup.
int GM_CDECL gmf_ci_getFirstShapeCollisionInfo(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    if (info->contacts.empty()) { a_thread->PushNull(); return GM_OK; }
    ShapeContactEntry& e = info->contacts.front();
    ShapeCollisionInfoData* sci = new ShapeCollisionInfoData{e.myShapeIds, e.normals, e.points, e.contacts};
    gmUserObject* obj = a_thread->GetMachine()->AllocUserObject(sci, g_shapeCollisionInfoType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}
// disableContact(shapeIndex) -- REAL, now implemented. Retail body
// vector of 16-byte records and, for the record whose index field equals
// the argument, ORs a flag bit into that contact:
//     for each record: if (record[3] == index) *(u32*)(record[0]+4) |= 1;
// Same shape as the lookup here: find the recorded contact at this index
// and mark it for the current step.
//
// The argument indexes this ShapeCollisionInfo's own parallel arrays --
// the same `i` that getContactNormal(i)/getContactPos(i, n) take (see
// Player.gm:4489, where `i` comes from the loop over this sci's shapes).
int GM_CDECL gmf_sci_disableContact(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    if (index >= 0 && index < (int)sci->contacts.size()) {
        DisableContactThisFrame(sci->contacts[(size_t)index]);
    }
    return GM_OK;
}
int GM_CDECL gmf_sci_getNumOfMyShapesColliding(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    a_thread->PushInt((int)sci->myShapeIds.size());
    return GM_OK;
}
int GM_CDECL gmf_sci_getMyContactShapeID(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(idx, 0, 0);
    a_thread->PushInt((idx >= 0 && idx < (int)sci->myShapeIds.size()) ? sci->myShapeIds[idx] : -1);
    return GM_OK;
}
// getNumContacts(myShapeIndex) -- confirmed real (Ghidra decompile,
// (`local nc=sci.getNumContacts(j);`, part of the player's own ground-
// contact/landing detection loop) -- was entirely unbound, a HALTING
// crash (this engine's default GE3_STRICT=halt mode) reached the moment
// the player pressed R1 (JOYSTICK0_R1, run/dash) and grounded again in
// ANY level, not level-specific. Backed by the exact same per-shape
// `points[idx].count` data getContactNormal/getContactPos already expose
// (real Box2D manifold point count for that contact).
int GM_CDECL gmf_sci_getNumContacts(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(idx, 0, 0);
    int count = (idx >= 0 && idx < (int)sci->points.size()) ? sci->points[idx].count : 0;
    a_thread->PushInt(count);
    return GM_OK;
}
int GM_CDECL gmf_sci_getContactNormal(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(idx, 0, 0);
    b2Vec2 n = (idx >= 0 && idx < (int)sci->normals.size()) ? sci->normals[idx] : b2Vec2(0.0f, 0.0f);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), n.x, n.y));
    return GM_OK;
}
// getContactPos(myShapeIndex, pointIndex) -- confirmed real, pointIndex
// genuinely varies (Player.gm:2050's `cp=sci.getContactPos(j,i)`), so this
// is a real per-manifold-point lookup, not always point 0.
int GM_CDECL gmf_sci_getContactPos(gmThread* a_thread) {
    ShapeCollisionInfoData* sci = (ShapeCollisionInfoData*)a_thread->ThisUserCheckType(g_shapeCollisionInfoType);
    if (!sci) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(myIdx, 0, 0);
    GM_INT_PARAM(ptIdx, 1, 0);
    b2Vec2 p(0.0f, 0.0f);
    if (myIdx >= 0 && myIdx < (int)sci->points.size()) {
        const ContactPoints& cp = sci->points[myIdx];
        int clamped = (ptIdx >= 0 && ptIdx < cp.count) ? ptIdx : 0;
        p = cp.pts[clamped];
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), p.x, p.y));
    return GM_OK;
}
int GM_CDECL gmf_intIter_isValid(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    a_thread->PushInt((it && it->index < it->values.size()) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_intIter_goToNext(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    if (it && it->index < it->values.size()) ++it->index;
    return GM_OK;
}
int GM_CDECL gmf_intIter_get(gmThread* a_thread) {
    IntIterData* it = (IntIterData*)a_thread->ThisUserCheckType(g_intIterType);
    if (it && it->index < it->values.size()) a_thread->PushInt(it->values[it->index]);
    else a_thread->PushNull();
    return GM_OK;
}
// isAllSensors() -- confirmed real, live call site Player.gm:4458:
// `if(!ci.isAllSensors()){this.setOverrideGravity(null);}` right after a
// KillPlayer-style collide check -- decides whether the thing the player
// just touched was solid ground (apply override gravity) vs. a pure sensor
// trigger (leave gravity alone). Was entirely unbound, a hard "getdot
// failed" aborting this branch of checkDeath()'s collide handling every
// single call. True (vacuous) when there were no non-sensor contacts.
int GM_CDECL gmf_ci_isAllSensors(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    a_thread->PushInt(info->allSensors ? 1 : 0);
    return GM_OK;
}
gmFunctionEntry s_collisionInfoLib[] = {
    {"isBodyColliding", gmf_ci_isBodyColliding, nullptr},
    {"isBodyCollidingWithShape", gmf_ci_isBodyCollidingWithShape, nullptr},
    {"isShapeColliding", gmf_ci_isShapeColliding, nullptr},
    {"getCollidingShapeIDList", gmf_ci_getCollidingShapeIDList, nullptr},
    {"getShapeCollisionInfo", gmf_ci_getShapeCollisionInfo, nullptr},
    {"getFirstShapeCollisionInfo", gmf_ci_getFirstShapeCollisionInfo, nullptr},
    {"isAllSensors", gmf_ci_isAllSensors, nullptr},
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
    {"isValid", gmf_intIter_isValid, nullptr},
    {"goToNext", gmf_intIter_goToNext, nullptr},
    {"get", gmf_intIter_get, nullptr},
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
// The retail method name is used by Player.gm's obstruction/raycast helpers.
// Those call sites only read .x/.y; this maps them to the same current
// position value as getPos(). Whether retail permits writing through the
// returned value is still unverified, so this is not claimed as reference
// semantics.
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
    a_thread->PushFloat(b ? b->radius : 0.0f);
    return GM_OK;
}
// setAsCircle/isCircle/isRect/getWidth/getHeight/setIsSensor/setMaxSpeed/
// thrust/doesRotationResponse/setVelocityDecayRate -- confirmed real,
// called directly on PhysicsData by FoE's PekkoAndJon corpus (which never
// calls createBody() first, see MainBody's header comment above) and by
// VideoHelper.gm's debug bounding-box drawer. Same "delegate to body 0"
// pattern as every other gmf_pd_* method in this block; body logic itself
// lives on PhysicsBody (gmf_pb_setAsCircle etc, above) so this just
// forwards through MainBody.
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
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
    fx->SetSensor(b->isSensor || !b->isCollidable);
    RegisterShapeGlobalId(fx);
    b->radius = r;
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
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    if (!f) { a_thread->PushFloat(0.0f); return GM_OK; }
    b2AABB aabb;
    f->GetShape()->ComputeAABB(&aabb, b2Transform(b2Vec2(0, 0), b2Rot(0)), 0);
    a_thread->PushFloat(aabb.upperBound.x - aabb.lowerBound.x);
    return GM_OK;
}
int GM_CDECL gmf_pd_getHeight(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    if (!f) { a_thread->PushFloat(0.0f); return GM_OK; }
    b2AABB aabb;
    f->GetShape()->ComputeAABB(&aabb, b2Transform(b2Vec2(0, 0), b2Rot(0)), 0);
    a_thread->PushFloat(aabb.upperBound.y - aabb.lowerBound.y);
    return GM_OK;
}
int GM_CDECL gmf_pd_setIsSensor(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        b->isSensor = (v != 0);
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext())
            f->SetSensor(b->isSensor || !b->isCollidable);
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
// NOTE: this, not gmf_pb_thrust, is the one the real corpus actually calls
// (Player.gm:88 / Thing.gm:333 both go through PhysicsData). It must use
// the same accumulator model as gmf_pb_thrust -- see that function's header
// comment. Leaving the old direct-velocity-write here while the pb_ side
// moved to accumulators made the player completely immobile: this wrote a
// velocity, then StepPhysicsBodyMovement's decay pass (which sees no
// accumulator input) scrubbed it back to zero before it could integrate.
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
// Delegates for the rest of the PHYSICSDATA method table added this
// session (see the matching gmf_pb_* implementations above for the
// Ghidra evidence/reasoning each one is based on) -- same "forward to
// MainBody" pattern as every other pd_ method in this file.
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
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
    fx->SetSensor(b->isSensor || !b->isCollidable);
    RegisterShapeGlobalId(fx);
    b->radius = 0.0f;
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
int GM_CDECL gmf_pd_getCollisionCategory(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt(f ? (int)f->GetFilterData().categoryBits : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_getCategoriesToCollideWith(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt(f ? (int)f->GetFilterData().maskBits : -1);
    return GM_OK;
}
int GM_CDECL gmf_pd_setCollisionCategory(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(category, 0, 1);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
            b2Filter filter = f->GetFilterData();
            filter.categoryBits = (uint16)category;
            f->SetFilterData(filter);
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
            b2Filter filter = f->GetFilterData();
            filter.maskBits = (uint16)mask;
            f->SetFilterData(filter);
        }
    }
    return GM_OK;
}
// setCollisionGroup/getCollisionGroup/isSleeping -- see the gmf_pb_ versions
// above for the full decompiled/live-call-site justification (this is the
// SAME real method, real scripts call it directly on PhysicsData, e.g.
// EntityBase.gm:914's `this.PhysicsData.setCollisionGroup(...)` -- the
// exact "delegate to body 0" pattern every other pd_/pb_ pair here uses).
int GM_CDECL gmf_pd_setCollisionGroup(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(group, 0, 0);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext()) {
            b2Filter filter = f->GetFilterData();
            filter.groupIndex = (int16)group;
            f->SetFilterData(filter);
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_getCollisionGroup(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    b2Fixture* f = b ? b->body->GetFixtureList() : nullptr;
    a_thread->PushInt(f ? (int)f->GetFilterData().groupIndex : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_isSleeping(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushInt((b && !b->body->IsAwake()) ? 1 : 0);
    return GM_OK;
}
// Retail PhysicsData.putToSleep walks all owned bodies, zeros linear/angular
// velocity, forces/torque, and sleep time, then marks the body asleep. Box2D's
// SetAwake(false) performs those same state transitions.
int GM_CDECL gmf_pd_putToSleep(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    for (const gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* b = GetBodyData(m, bodyVar);
        if (b && b->body) b->body->SetAwake(false);
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
// 0x00094e78): walks every body pointer this PhysicsData owns and clears
// its "asleep" bit -- i.e. wakes every body this entity has, not just the
// main one (distinct from the isSleeping()/getPos() etc "just body 0"
// pattern above). Box2D's SetAwake(true) is the direct real equivalent.
int GM_CDECL gmf_pd_wakeUp(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    for (gmVariable& bodyVar : d->bodies) {
        PhysicsBodyData* b = GetBodyData(m, bodyVar);
        if (b && b->body) b->body->SetAwake(true);
    }
    return GM_OK;
}
// appendShapes(index, shapeDefCollection) -- confirmed real 2-arg overload
// on PhysicsData itself (distinct from the single-body, 1-arg gmf_pb_
// appendShapes above): CONFIRMED LIVE, game_data/Platformer/EntityPacks/
// Minimal/SingleDirLaserTurret.gm:445-446 --
//   `this.PhysicsData.replaceBody(0); this.PhysicsData.appendShapes(0,sd);`
// -- both uncommented, genuinely reachable. Was entirely missing (only the
// 1-arg per-body form existed), so any level containing this entity hit
// "getdot failed" here. Resolves the body at `index` the same way getBody/
// removeBody do, then does exactly what gmf_pb_appendShapes does to it.
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
        RegisterShapeGlobalId(fx);
        if (def.shape->GetType() == b2Shape::e_circle) {
            b->radius = ((b2CircleShape*)def.shape)->m_radius;
        }
    }
    SyncBodyTypeFromMass(b->body);
    int lastIndex = -1;
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) ++lastIndex;
    a_thread->PushInt(lastIndex);
    return GM_OK;
}
// 88-114, 0x00092810: `FUN_0002ef8c(world, index, 1)`, returns either null
// or a fresh PhysicsBody wrapper) and CONFIRMED LIVE (see appendShapes
// comment above -- SingleDirLaserTurret.gm:445, immediately followed by
// appendShapes(0,sd) re-populating the fresh body's fixtures). Exact
// semantics of FUN_0002ef8c are AMBIGUOUS -- needs a comment, not a
// guess: the internals aren't in this decompiled corpus, so "destroy the
// body at this slot and replace it with a fresh, empty one at the same
// index/position" is inferred only from the immediately-following
// appendShapes(0,sd) call needing something to attach fixtures to, not
// from decompiled proof. Out-of-range index returns null, matching
// getBody's existing convention.
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
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushInt((b && b->isSensor) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_pd_setIsCollidable(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 1);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        b->isCollidable = (v != 0);
        for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext())
            fx->SetSensor(b->isSensor || !b->isCollidable);
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
        // Lowercase field names -- see gmf_pb_getAABB's comment above for
        // the corpus evidence (this PhysicsData variant feeds the exact
        // same real call sites, e.g. SwayingTree.gm:280's
        // `this.getPhysicsData().getAABB()`).
        t->Set(&m, "minx", gmVariable(aabb.lowerBound.x));
        t->Set(&m, "miny", gmVariable(aabb.lowerBound.y));
        t->Set(&m, "maxx", gmVariable(aabb.upperBound.x));
        t->Set(&m, "maxy", gmVariable(aabb.upperBound.y));
    }
    a_thread->PushTable(t);
    return GM_OK;
}

// getMinPos()/getMaxPos() -- confirmed real by usage only, no decompiled
// and not caught by strict survey either (calls on GM_USER-typed objects
// like PhysicsData don't route through the same OnNullCall hook plain
// GM_TABLE receivers do) -- a real, silent gap found only by tracing a
// live crash: TouchEditProcessBase.gm:2954's getGroupAABB() (`local min=
// e.getPhysicsData().getMinPos(); ...; retval.minx=min.x;`) got `min` as
// null when this method didn't exist, and `null.x` is a hard "getdot
// failed" that aborted the editor's entire entity-selection keypress
// handler (confirmed via live repro placing any entity from the editor
// palette). Same AABB this PhysicsData's own getAABB() already computes
// (see ComputePhysicsDataAABB) -- min/max are just its lower/upper bound
// as vector2Ds instead of a 4-field table, matching the (x,y) contract
// `retval.minx=min.x; retval.miny=min.y;` depends on.
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

void FlushDeferredPhysicsBodyDestructions() {
    // StepPhysicsWorld invokes this only after b2World::Step has released
    // its lock. Keep the guard for shutdown and direct callers.
    if (GetPhysicsWorld().IsLocked()) return;
    for (b2Body* body : g_deferredBodyDestructions) {
        RemovePhysicsJointsForBody(body);
        GetPhysicsWorld().DestroyBody(body);
    }
    g_deferredBodyDestructions.clear();
}

// For entity_bindings.cpp's generic entity-handle getMainBody() -- Player.gm
// itself (line 5153-5155) defines getMainBody() as `return this.mainpb;
// //this.PhysicsData.getBody(0);`, i.e. the script's own author already
// documented these as equivalent; this is the getBody(0) side of that
// equivalence, usable from entity handles that don't carry a `mainpb`
// field of their own. Returns gmVariable::s_null if there's no PhysicsData
// or no bodies yet, matching gmf_pd_getBody's own out-of-range behavior.
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

bool GetPhysicsDataPos(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                       float& a_x, float& a_y) {
    PhysicsBodyData* body = GetBodyData(a_machine, GetPhysicsDataMainBodyVar(a_machine, a_physicsDataVar));
    if (!body || !body->body) return false;
    const b2Vec2 pos = body->body->GetPosition();
    a_x = pos.x;
    a_y = pos.y;
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

// --- exposed for physics_global_bindings.cpp (the "Physics" global class,
// e.g. isShapeSensor/getBodyFromShapeID/getEntityFromShapeID/raycastOne) --

b2Fixture* FindFixtureByGlobalId(int a_id) {
    return GetFixtureByGlobalId(a_id);
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

// For entity_bindings.cpp's native SauceBase draw (see that function's
// header comment): mirrors gmf_pb_getPos + gmf_pb_getDir without needing a
// gmThread, so the native draw hook can read an entity's mainpb position/
// facing the same way its own commented-out real draw() would have via
// `this.mainpb.getPos()`/`getDir()`.
bool GetPhysicsBodyPosAndDir(gmMachine& a_machine, const gmVariable& a_var,
                              float& a_x, float& a_y, float& a_dirX, float& a_dirY) {
    PhysicsBodyData* bd = GetBodyData(a_machine, a_var);
    if (!bd || !bd->body) return false;
    b2Vec2 p = bd->body->GetPosition();
    a_x = p.x; a_y = p.y;
    b2Vec2 v = bd->body->GetLinearVelocity();
    float len = v.Length();
    if (len > 0.0001f) { a_dirX = v.x / len; a_dirY = v.y / len; }
    else { a_dirX = bd->dirX; a_dirY = bd->dirY; }
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
}

gmVariable MakePhysicsData(gmMachine& a_machine, gmVariable a_ownerEntity, int a_type) {
    PhysicsDataObj* data = new PhysicsDataObj();
    data->type = a_type;
    data->ownerEntity = a_ownerEntity;
    RootGmVariable(a_machine, data->ownerEntity);
    gmUserObject* obj = a_machine.AllocUserObject(data, g_physicsDataType);
    return gmVariable(obj);
}

// FUN_00031d50 is the retail angular-velocity integrator used by
// FUN_00031f20. Preserve its single-precision fused operations and order.
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

// Retail FUN_00031f20, called for each PhysicsShape by FUN_0002e860.
// The native manager passes the same timestep to this method and to the
// world step; no separate 16.666 ms movement tick is applied here. Values
// remain in script millisecond units; the stock-Box2D conversion is confined
// to the body/world adapter boundary.
void StepPhysicsBodyMovement(float a_dt) {
    for (PhysicsBodyData* b : g_allPhysicsBodies) {
        if (!b || !b->body) continue;

        const bool rotationResponse = !b->body->IsFixedRotation();
        if (rotationResponse) {
            const float angle = b->body->GetAngle();
            b->dirX = std::cos(angle);
            b->dirY = std::sin(angle);
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
            b->dirX = std::cos(angle);
            b->dirY = std::sin(angle);
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

    g_entityIterType = a_machine.CreateUserType("EntityIterator");
    a_machine.RegisterTypeLibrary(g_entityIterType, s_entityIterLib,
                                   sizeof(s_entityIterLib) / sizeof(s_entityIterLib[0]));
    a_machine.RegisterUserCallbacks(g_entityIterType, nullptr, EntityIterGCDestruct, nullptr);

    g_collisionInfoType = a_machine.CreateUserType("CollisionInfo");
    a_machine.RegisterTypeLibrary(g_collisionInfoType, s_collisionInfoLib,
                                   sizeof(s_collisionInfoLib) / sizeof(s_collisionInfoLib[0]));
    a_machine.RegisterUserCallbacks(g_collisionInfoType, nullptr,
        [](gmMachine*, gmUserObject* a_object) { delete (CollisionInfoData*)a_object->m_user; }, nullptr);

    g_shapeCollisionInfoType = a_machine.CreateUserType("ShapeCollisionInfo");
    a_machine.RegisterTypeLibrary(g_shapeCollisionInfoType, s_shapeCollisionInfoLib,
                                   sizeof(s_shapeCollisionInfoLib) / sizeof(s_shapeCollisionInfoLib[0]));
    a_machine.RegisterUserCallbacks(g_shapeCollisionInfoType, nullptr,
        [](gmMachine*, gmUserObject* a_object) { delete (ShapeCollisionInfoData*)a_object->m_user; }, nullptr);

    g_intIterType = a_machine.CreateUserType("IntListRefContainer");
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
        {"getShapeRadius", gmf_pd_getShapeRadius, nullptr},
        {"isPointWithin", gmf_pd_isPointWithin, nullptr},
    };
    g_physicsDataType = a_machine.CreateUserType("PhysicsData");
    a_machine.RegisterTypeLibrary(g_physicsDataType, physicsDataLib,
                                   sizeof(physicsDataLib) / sizeof(physicsDataLib[0]));
    a_machine.RegisterUserCallbacks(g_physicsDataType, nullptr,
        [](gmMachine* a_m, gmUserObject* a_object) {
            PhysicsDataObj* d = (PhysicsDataObj*)a_object->m_user;
            UnrootGmVariable(*a_m, d->ownerEntity);
            for (gmVariable& bv : d->bodies) {
                // Guard against a body outliving its owning PhysicsData
                // (e.g. an entity dying without its bodies being destroyed
                // in the same GC pass): null the back-pointer so any LATER
                // read of this body's user data sees "no owner" instead of
                // a dangling PhysicsDataObj* into freed memory -- this is
                // exactly what crashed gmf_pb_getEntity (SIGSEGV, confirmed
                // via gdb) before this fix.
                PhysicsBodyData* bd = GetBodyData(*a_m, bv);
                // ...but only if that body is still LIVE. On machine
                // teardown (gmMachine::~gmMachine -> ResetAndFreeMemory ->
                // gmGCColorSet::DestructAll) the collector destructs objects
                // in arbitrary order, so the PhysicsBody this PhysicsData
                // points at may already have been freed -- and then `bd` is
                // freed memory and `bd->body` a dangling b2Body*, which
                // segfaulted on exit (confirmed via gdb: SIGSEGV in this
                // lambda, called from gmUserObject::Destruct).
                //
                // Membership in g_allPhysicsBodies is the liveness test:
                // PhysicsBodyGCDestruct erases from it before freeing, so a
                // pointer still in the list is still alive.
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
