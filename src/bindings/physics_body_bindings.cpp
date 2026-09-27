#include "gmThread.h"
#include "physics_body_bindings.h"
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
gmType g_entityIterType = -1; // stub iterator for getCollidingEntities()

// ENGINE PATCH (2026-08-16): empirically-calibrated velocity scale between
// script-space units and Box2D's real-time integration. Player.gm's own
// real velocity/acceleration constants (walkSpeed, maxHangSpeed, etc.),
// taken literally, imply ~45 minutes to cross one confirmed screen-width --
// Box2D itself is confirmed byte-identical to upstream v2.4.1, and the
// msElapsed value passed into script update() calls (main.cpp) is confirmed
// genuine milliseconds, so neither explains the discrepancy. Retail's real
// native setVel/setPos/setSpeed/setDir could not be located in the binary
// a scale factor we're missing, so this is a stopgap, not a confirmed fix.
//
// Applied ONLY at the setVel/setSpeed (script->sim) and getVel/getSpeed
// (sim->script) boundaries, so every script-visible value (comparisons
// against maxHangSpeed etc.) is unaffected -- only the body's actual
// real-time motion through the (unscaled) Box2D/ground coordinate space
// speeds up by this factor. Positions, shape geometry, and ground bodies
// are untouched, so collision alignment is unaffected. TUNABLE.
// (kScriptVelocityToSimScale itself now declared in physics_body_bindings.h
// so player_renderer.cpp -- which reads body->GetLinearVelocity() directly
// for its own speed-dependent visuals -- can divide it back out too.)

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
    // setThrustAcc/setBrakeAcc all decompile to a bare
    // `*(float*)(nativeObj+OFS) = arg` store with no other side effects,
    // and setVelocityDecayRate/setRotationDecayRate decompile to a call
    // into a shared helper (FUN_00032b94/FUN_00032b3c) that computes
    // `clamp(exp(-arg * K), lo, hi)` before storing -- i.e. retail
    // pre-converts a "decay rate" into a per-tick multiplicative velocity-
    // retention factor at set time, not at consume time. No decompiled
    // evidence exists for setMaxRotateSpeed/setRotateAcc specifically, but
    // every other same-class "Acc"/"Max*" setter (setThrustAcc,
    // setMaxSpeed) is a bare store, so these follow the same pattern by
    // strong structural analogy. None of these are consumed by our own
    // physics step yet (see StepPhysicsWorld, physics_world.cpp) --
    // scripts set them and otherwise leave the sim to Box2D, so storing
    // them (without a confirmed effect on the sim) is the honest
    // stand-in, same as before, just now backed by real setter evidence
    // instead of a guess.
    float maxSpeed = 1e9f;
    // These two hold the PRE-CONVERTED per-tick retention factor (what
    // ComputeDecayFactor returns), not the raw script-supplied rate --
    // matching retail, which converts at set time. Default 1.0 = "retain
    // everything", i.e. no decay until a script actually calls
    // setVelocityDecayRate/setRotationDecayRate. (Was 0.0f, which as a
    // retention factor means "zero out velocity every tick" -- harmless
    // while nothing consumed these fields, but actively wrong now that
    // StepPhysicsBodyMovement does.)
    float velocityDecayRate = 1.0f;
    float maxRotateSpeed = 1e9f;
    float rotationDecayRate = 1.0f;
    float rotateAcc = 1e9f;
    // Ghidra-confirmed real (this session, full PHYSICSDATA method table
    // sweep, FoE PC binary, 0x00514a30-0x00514c58, 47 entries): every one
    // of these is a bare vtable dispatch with the exact same "getdot ->
    // read a float field" / "setdot -> store a float field" shape as
    // maxSpeed/velocityDecayRate above (opaque past the vtable call, but
    // structurally identical to the already-confirmed setters). Not used
    // by the current PekkoAndJon corpus (grepped, 0 call sites for all of
    // these), but implemented for real 1:1 API completeness rather than
    // left as silent no-ops, per explicit direction.
    float thrustAcc = 1e9f;
    float brakeAcc = 1e9f;
    // setMaxVelocityDecayRate/setMaxRotationDecayRate (vtable +0xa4/+0xa8,
    // fields +0x74/+0x78). Their role IS now confirmed (see
    // StepPhysicsBodyMovement): retail's integrator uses them as
    // `speed -= (1 - thisFactor) * excessOverMax`, i.e. they control how
    // much of the OVER-maxSpeed excess is shed per tick. Default 0.0 =
    // shed all of it = a hard clamp to maxSpeed, which is what gives the
    // retail-sharp "instantly at top speed, never above it" feel and is
    // what a zero-initialised float field yields in retail for the
    // (common) case where no script ever calls these setters.
    float maxVelocityDecayRate = 0.0f;
    float maxRotationDecayRate = 0.0f;
    // Retail's real per-tick movement accumulators + once-per-tick guard
    // flags (fields +0x40/+0x41 thrust/reverse flags, +0x44 thrust accum,
    // +0x48 brake flag, +0x4c brake accum, +0x50/+0x51 rotate flags,
    // +0x54 rotate accum). thrust()/brake()/rotateLeft() etc. only ADD to
    // these (guarded so repeated calls in one tick count once); the actual
    // velocity change happens in the per-tick integrator, which then clears
    // all of them (retail FUN_004159c0).
    float thrustAccum = 0.0f;
    float brakeAccum = 0.0f;
    float rotAccum = 0.0f;
    // Retail's persistent angular velocity (field +0x9c): the rotate
    // integrator carries it across ticks and integrates it into the facing
    // angle, rather than rotateLeft/Right snapping the facing directly.
    float rotVel = 0.0f;
    bool thrustFlag = false;
    bool reverseFlag = false;
    bool brakeFlag = false;
    bool rotLeftFlag = false;
    bool rotRightFlag = false;
    // ENGINE PATCH (2026-08-17): persistent facing direction, separate from
    // live velocity. getDir()/setDir() used to be purely velocity-derived
    // (getDir() normalizes GetLinearVelocity(), setDir() re-applies the
    // current speed along a new heading) -- fine for a moving body, but a
    // STATIONARY one (0 velocity) can never durably store a direction that
    // way: setDir(x,y) on a zero-speed body computes speed*dir = zero
    // velocity, so the call is a complete no-op, and getDir() always fell
    // back to a hardcoded (1,0). Confirmed real, load-bearing case:
    // EntityPacks/SpawnPoint.gm:186's `pb.setDir(vector2D(this.
    // configureData.editRotateValue));` -- the player-spawn marker's own
    // placed rotation, set once at configure time on a body that never
    // moves -- which is exactly why the spawn marker's drawList.rotateZ(pb.
    // getDir()) (SpawnPoint.gm:270) never visibly rotated. Stored
    // separately here and consulted by getDir() only when the body's
    // current velocity is too small to derive a heading from, so moving
    // bodies (Player.gm et al) keep their existing velocity-based behaviour
    // unchanged.
    float dirX = 1.0f, dirY = 0.0f;
};

PhysicsBodyData* ThisBody(gmThread* a_thread) {
    return (PhysicsBodyData*)a_thread->ThisUserCheckType(g_physicsBodyType);
}

// Box2D's own debug renderer (vendor/box2d/src/dynamics/b2_world.cpp,
// DebugDraw()) draws a b2_dynamicBody with zero computed mass in RED with
// the comment "Bad body" -- Box2D's authors consider this an unsupported
// configuration, not a documented "immovable dynamic body" trick. Real
// scripts create every ground piece as a dynamic body with density 0
// (`sd.setMass(0)`, e.g. GroundResizeNoRotate.gm) with no script-facing
// way to ask for a real static body at all (see MakePhysicsBody's own
// header comment -- confirmed no such API exists). The zero-mass-dynamic
// body still "works" for simple resting contact (invMass=0 means impulses
// can't move it), but Box2D's continuous-collision eligibility
// (b2World::SolveTOI, `collideA = bA->IsBullet() || typeA != b2_dynamicBody`)
// and its sleep/wake island logic both treat "dynamic" specially in ways
// "static" is exempt from -- observed in practice as the player partially
// sinking into/clipping through these shapes instead of resting cleanly on
// top, exactly what "Bad body" is warning about. Fix: whenever a body's
// fixtures make its computed mass hit zero, actually convert it to a real
// b2_staticBody (and back to dynamic if density later becomes nonzero
// again, e.g. a script that starts an object at mass 0 then activates it
// later) -- same physical intent the script already expresses
// (setMass(0) == "should not move"), just backed by the Box2D body type
// that's actually built and tested for that, instead of an edge case its
// own debug tooling flags as broken.
void SyncBodyTypeFromMass(b2Body* a_body) {
    if (!a_body || a_body->GetType() == b2_kinematicBody) return; // never auto-managed
    a_body->ResetMassData();
    bool hasFixtures = a_body->GetFixtureList() != nullptr;
    bool shouldBeStatic = hasFixtures && a_body->GetMass() <= 0.0f;
    b2BodyType wantType = shouldBeStatic ? b2_staticBody : b2_dynamicBody;
    if (a_body->GetType() != wantType) {
        a_body->SetType(wantType);
        a_body->ResetMassData();
    }
}

std::vector<gmVariable> g_bodiesByObjectId; // index 0 reserved/invalid
// Every live PhysicsBodyData, for StepPhysicsBodyMovement to walk once per
// tick. Parallel to g_bodiesByObjectId (which only holds the script-visible
// gmVariable handles) because the integrator needs the raw struct.
std::vector<PhysicsBodyData*> g_allPhysicsBodies;

// type is ALWAYS b2_dynamicBody -- confirmed by Ghidra disassembly of the
// real createBody (0x00092428) and its callees this session: the only
// script-facing "type"-shaped argument (the single-bool overload, e.g.
// EntityBase.gm's `createBody(false)`) actually controls fixedRotation,
// not body type (see below) -- and there is no other script API anywhere
// in the corpus that creates or queries a body type at all (grepped for
// setBodyType/isStatic/isDynamic/kinematic -- nothing). Retail's own
// "immovable ground" comes entirely from setMass(0) on a dynamic body
// (zero fixture density -> Box2D computes zero mass AND zero rotational
// inertia for a dynamic body, see b2Body::ResetMassData in the vendored
// 2.4.1 source -- this makes it exactly as immovable as a real static
// body, no b2_staticBody needed), same as this engine already does. So
// this was NOT a bug -- flagged in an earlier pass as a suspicious
// architecture choice, now confirmed retail-faithful by decompile.
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
    if (id > 0 && id < (int)g_bodiesByObjectId.size()) a_thread->Push(g_bodiesByObjectId[id]);
    else a_thread->PushNull();
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

int GM_CDECL gmf_pb_getPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 p = b->body->GetPosition();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), p.x, p.y));
    return GM_OK;
}
int GM_CDECL gmf_pb_setPos(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    b->body->SetTransform(b2Vec2(x, y), b->body->GetAngle());
    return GM_OK;
}
int GM_CDECL gmf_pb_getVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 v = b->body->GetLinearVelocity();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), v.x / kScriptVelocityToSimScale, v.y / kScriptVelocityToSimScale));
    return GM_OK;
}
int GM_CDECL gmf_pb_setVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    b->body->SetLinearVelocity(b2Vec2(x * kScriptVelocityToSimScale, y * kScriptVelocityToSimScale));
    return GM_OK;
}
// getVelRef -- confirmed real (Player.gm's DECISION_STATEID dead-fall
// branch: `local vel=mypb.getVelRef(); vel->y=vel.y-gravity*msElapsed;`,
// with no separate setVel() call afterward in that one call site), entirely
// unregistered before this -- same hard "getdot failed" class of bug as
// applyForce above, aborting the whole calling function every time it was
// hit. TODO/known gap: this returns a plain by-value vector2D copy (same
// type every other vector2D-returning method uses, so it stays compatible
// with every method/param-check elsewhere in the corpus) rather than a
// true live reference, so mutating the result here does NOT write back to
// the body the way retail's single known real use site implies -- that one
// site's `vel->y-=...` mutation is currently a no-op on the actual
// simulation. Filed as a follow-up (needs a real "live" vector2D variant,
// or resolving this call site's field offset from more of the binary) --
// registering it at all at least stops it from throwing and aborting
// DECISION_STATEID's entire update() on death, which every other branch of
// that function depends on running.
int GM_CDECL gmf_pb_getVelRef(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 v = b->body->GetLinearVelocity();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), v.x / kScriptVelocityToSimScale, v.y / kScriptVelocityToSimScale));
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
    b->body->ApplyForce(b2Vec2(fx, fy), worldPoint, true);
    return GM_OK;
}
int GM_CDECL gmf_pb_getSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->body->GetLinearVelocity().Length() / kScriptVelocityToSimScale);
    return GM_OK;
}
// setSpeed -- confirmed real: sets magnitude only, preserving current
// direction (or defaulting to +x if the body has no velocity yet, matching
// getDir's own real fallback).
int GM_CDECL gmf_pb_setSpeed(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(speed, 0, 0.0f);
    float simSpeed = speed * kScriptVelocityToSimScale;
    b2Vec2 v = b->body->GetLinearVelocity();
    float len = v.Length();
    if (len > 0.0001f) {
        v *= (simSpeed / len);
    } else {
        v = b2Vec2(simSpeed, 0.0f); // no direction yet -- pick +x, matches getDir's fallback
    }
    b->body->SetLinearVelocity(v);
    return GM_OK;
}
int GM_CDECL gmf_pb_getDir(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    b2Vec2 v = b->body->GetLinearVelocity();
    float len = v.Length();
    float x, y;
    // See PhysicsBodyData::dirX/dirY's header comment -- a stationary body
    // (len ~0) has no velocity to derive a heading from, so fall back to
    // the persistent direction setDir() actually stored instead of a
    // hardcoded (1,0).
    if (len > 0.0001f) { x = v.x / len; y = v.y / len; }
    else { x = b->dirX; y = b->dirY; }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), x, y));
    return GM_OK;
}
int GM_CDECL gmf_pb_setDir(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    float speed = b->body->GetLinearVelocity().Length();
    b2Vec2 dir(x, y);
    float dlen = dir.Length();
    if (dlen > 0.0001f) dir *= (1.0f / dlen);
    b->dirX = dir.x; b->dirY = dir.y;
    b->body->SetLinearVelocity(speed * dir);
    return GM_OK;
}
// thrust() -- confirmed real (Ghidra: retail FUN_00435a50 @ 0x00435a50, a
// bare no-arg vtable dispatch, +0x40). Real callers: Player.gm:88 and
// Thing.gm:333, BOTH in the exact same `setDir(x,y); thrust();` pattern,
// called every frame a movement key/moveType is active -- i.e. this is the
// whole "move at max speed in current facing direction" mechanic (Player.gm
// itself sets maxSpeed once at construction, Thing.gm's ground-spawned
// entities via setMaxSpeed at Thing.gm:110/175). Retail's internal
// thrustAcc-based acceleration curve isn't recoverable past the opaque
// vtable call, so this snaps directly to maxSpeed in the persisted facing
// direction (b->dirX/dirY, already tracked by setDir) rather than guess an
// accel-integration curve -- reuses the already-verified maxSpeed field
// (setMaxSpeed) and kScriptVelocityToSimScale boundary, same scale applied
// at every other script->sim velocity write in this file.
// setVelocityDecayRate/setRotationDecayRate -- the SET-time transform is
// solidly confirmed this session (live winedbg traced PhysicsShape's real
// vtable via thrust's wrapper at 0x00435a50 -> vtable+0x40 -> resolved live
// to 0x00415600; full vtable dumped via Ghidra from base 0x004ed7c8): every
// setXDecayRate setter (e.g. setMaxVelocityDecayRate @ 0x00415960,
// field+0x74) calls a shared helper at 0x004513d0, `clamp(exp(-rate*dt),
// 0, 1)`, with dt read live from static data at 0x00522734: 16.666666 (a
// fixed per-tick constant, 1000/60).
// The CONSUMING side is now confirmed too, which is what makes the
// millisecond reading safe to apply. The velocity integrator itself
// (vtable+0xd0 @ 0x00415c30) is dense esp-relative asm, but its ROTATION
// twin -- FUN_00415b70, called from that same integrator -- decompiles
// completely cleanly, and the two are structurally identical (same fields,
// one axis vs two):
//     retained = current * decayFactor;          // field +0x70
//     excess   = max(retained - maxRotSpeed, 0); // field +0x64
//     result   = dt*rotAccum + (retained - (1 - maxRotDecay)*excess);
//     clamp(result, +-maxRotSpeed);
// i.e. the factor multiplies the WHOLE quantity every tick (a drag/retain
// term), the accumulator supplies the per-tick input, and the max*DecayRate
// field controls how much over-max excess is shed. With retail's real
// dt=16.666666 and a script literal like Player.gm's
// setVelocityDecayRate(1), the retain factor is ~5.8e-8 -- i.e. velocity is
// essentially zeroed each tick and rebuilt from the accumulator, which is
// precisely the sharp "instant start, instant stop" arcade feel retail has
// and this engine was missing.
// (Moved earlier in the file this session so gmf_pb_brake, right below, can
// use it too -- was previously defined only just before its own original
// setter call sites, further down.)
constexpr float kDecayRateTickMs = 16.666666f;
float ComputeDecayFactor(float a_rate) {
    float f = std::exp(-a_rate * kDecayRateTickMs);
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    return f;
}
// ENGINE PATCH (movement-speed session): now the real retail
// implementation, decompiled from the actual vtable target rather than
// approximated. Retail's thrust (0x00415600, vtable+0x40) is:
//     if (!thrustFlag) { thrustFlag = true; thrustAccum += thrustAcc; }
// It does NOT touch velocity at all -- it only arms an accumulator that the
// per-tick integrator (StepPhysicsBodyMovement) consumes and then clears.
// The flag makes repeated calls within one tick count exactly once.
// This replaces the old "snap velocity straight to maxSpeed" stand-in,
// which had two visible consequences: nothing ever removed that velocity
// again (no integrator existed), so a body kept gliding at full speed after
// the key was released, and speed could never build or vary the way retail's
// accumulate-then-clamp model produces.
int GM_CDECL gmf_pb_thrust(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->thrustFlag) { b->thrustFlag = true; b->thrustAccum += b->thrustAcc; }
    return GM_OK;
}
// reverseThrust/brake/rotateLeft/rotateRight -- like thrust above, these
// are now the REAL retail implementations, decompiled from their concrete
// vtable targets (resolved once thrust's live-traced vtable base 0x004ed7c8
// made the whole table dumpable). All four follow the identical
// "guard flag, then accumulate" shape, and all four touch only accumulator
// state -- never velocity directly:
//   reverseThrust (0x00415620, +0x44): !reverseFlag -> thrustAccum -= thrustAcc
//   brake         (0x00415640, +0x48): !brakeFlag   -> brakeAccum  += brakeAcc
//   rotateLeft    (0x00415510, +0x20): !rotLeftFlag -> rotAccum    += rotateAcc
//   rotateRight   (0x00415530, +0x24): !rotRightFlag-> rotAccum    -= rotateAcc
// Note rotateLeft/rotateRight share one accumulator with opposing signs, so
// holding both cancels out exactly -- previously these were implemented as
// direct trig rotations of the stored facing with an invented step size.
int GM_CDECL gmf_pb_reverseThrust(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->reverseFlag) { b->reverseFlag = true; b->thrustAccum -= b->thrustAcc; }
    return GM_OK;
}
int GM_CDECL gmf_pb_brake(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    if (!b->brakeFlag) { b->brakeFlag = true; b->brakeAccum += b->brakeAcc; }
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
int GM_CDECL gmf_pb_getRadius(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->radius);
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
    filter.categoryBits = 0; // matches ShapeDefCollection::AddFixture's "no category set" default
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
    for (auto& def : sd->fixtureDefs) {
        b2Fixture* fx = b->body->CreateFixture(&def);
        RegisterShapeGlobalId(fx);
        if (def.shape->GetType() == b2Shape::e_circle) {
            b->radius = ((b2CircleShape*)def.shape)->m_radius;
        }
    }
    SyncBodyTypeFromMass(b->body);
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
// first and the actual density second). This was reading density from
// param 0 -- the shape INDEX, always literally 0 in every real call site
// -- and applying it to every fixture, so every single
// `setShapeDensity(0, someRealDensity)` call silently zeroed the body's
// density instead of setting it to someRealDensity. This directly broke
// the PLAYER's own body: Player.gm's BIRTH_STATEID/DECISION_STATEID
// init() (and several other states) call
// `parent.mainpb.setShapeDensity(0,parent.baseDensity)` on every state
// entry, so the player's density was being forced to 0 every time --
// combined with SyncBodyTypeFromMass (this session, above), that made the
// player's own body a real b2_staticBody, which silently no-ops
// SetLinearVelocity entirely -- the actual root cause of "the player
// barely moves," not a units/gravity issue. Applies to just the one
// fixture at that local index, matching the real per-shape-index API
// shape (falls back to "all fixtures" if no such index exists, so a
// caller passing an out-of-range index still gets a sane no-crash result
// rather than silently doing nothing).
int GM_CDECL gmf_pb_setShapeDensity(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(shapeIndex, 0, 0);
    GM_FLOAT_OR_INT_PARAM(d, 1, 1.0f);
    int idx = 0;
    bool applied = false;
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext(), ++idx) {
        if (idx == shapeIndex) { fx->SetDensity(d); applied = true; break; }
    }
    if (!applied) {
        for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetDensity(d);
    }
    SyncBodyTypeFromMass(b->body);
    return GM_OK;
}
// setMass(density) -- confirmed real, single-arg (e.g. `this.mainpb.
// setMass(999);`, `pd.setMass(this.baseDensity);`), a DIFFERENT real
// signature from setShapeDensity's 2-arg (shapeIndex, density) above --
// they used to incorrectly share one implementation, which broke once
// setShapeDensity was fixed to actually require 2 args. Same density
// mapping as PHYSICSSHAPECOLLECTIONDEF.setMass (physics_bindings.cpp) --
// Box2D has no direct per-body mass setter that also updates fixtures --
// applied to every fixture on the body, matching every real call site's
// "set the whole body's mass" intent (none of them target one shape).
int GM_CDECL gmf_pb_setMass(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(d, 0, 1.0f);
    for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetDensity(d);
    SyncBodyTypeFromMass(b->body);
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
    int idx = 0;
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext(), ++idx) {
        if (idx == localIndex) { a_thread->PushInt((int)f->GetUserData().pointer); return GM_OK; }
    }
    a_thread->PushInt(0);
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
    int idx = 0;
    for (b2Fixture* f = b->body->GetFixtureList(); f; f = f->GetNext(), ++idx) {
        if (idx == localIndex) {
            b2AABB aabb = f->GetAABB(0);
            b2Vec2 c = 0.5f * (aabb.lowerBound + aabb.upperBound);
            a_thread->Push(MakeVector2D(*a_thread->GetMachine(), c.x, c.y));
            return GM_OK;
        }
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), 0.0f, 0.0f));
    return GM_OK;
}
// disableRestitutionThisFrame() -- confirmed real (Ghidra decompile,
// action (real semantics: b2Contact::SetRestitution(0) on the LIVE contact
// currently being resolved, auto-reset by Box2D's own contact-restitution
// recompute on the NEXT step -- not a persistent fixture property change).
// Live call site Player.gm:4488, `this.mainpb.disableRestitutionThisFrame();
// sci.disableContact(i); ...setVel(v.x,v.y*(1-MATH_ABS(dn.y)));` -- landing-
// impact handling that suppresses a physical bounce. This engine has no
// live PreSolve contact-listener hook (its collision-info objects are
// built AFTER the fact via b2Body::GetContactList(), see BuildCollisionInfo
// in this file), so there's no real b2Contact to call SetRestitution on
// here -- was entirely unbound, a HALTING crash (this engine's default
// GE3_STRICT=halt mode) reached the moment the player first landed on
// anything, in EVERY level. Safe no-op: the very next line in the same
// real call site manually zeroes the vertical velocity component anyway
// (`v.y*(1-MATH_ABS(dn.y))`), which is the dominant, already-correct part
// of "don't bounce on landing" -- this call alone being a no-op only
// leaves a secondary Box2D-restitution-formula edge case unaddressed.
int GM_CDECL gmf_pb_disableRestitutionThisFrame(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// disableContact(shapeIndex) -- same real-but-unbacked situation as
// disableRestitutionThisFrame directly above (confirmed real Box2D
// PreSolve action, b2Contact::SetEnabled(false) on the live contact this
// engine has no listener hook to expose) -- same live call site
// (Player.gm:4489, `sci.disableContact(i);`, the very next statement after
// disableRestitutionThisFrame). Was entirely unbound on ShapeCollisionInfo
// objects -- would have been the NEXT halting crash immediately after
// fixing disableRestitutionThisFrame above. Safe no-op, same reasoning.
int GM_CDECL gmf_sci_disableContact(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// setMaxSpeed/setMaxRotateSpeed/setRotateAcc -- confirmed (setMaxSpeed) or
// strongly-analogous (see PhysicsBodyData's comment) bare field stores.
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
// setBrakeAcc/getBrakeAcc/setThrustAcc/getThrustAcc -- Ghidra-confirmed
// real (PHYSICSDATA table sweep, this session), bare field get/set exactly
// like setMaxSpeed/setRotateAcc above. brakeAcc now backs gmf_pb_brake's
// own decay-per-call formula; thrustAcc is stored for completeness (this
// engine's thrust() snaps directly to maxSpeed, see thrust's own header
// comment -- no accel-integration curve to apply thrustAcc to). 0 real
// call sites in the current corpus for either.
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
    b2Filter filter; filter.categoryBits = 0;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
// setDoesRotationResponse -- no decompiled evidence located this session
// (string not found by the retail-symbol indexer, no gmFunctionEntry xref
// resolved either under -noanalysis). Best-effort semantic mapping onto
// the one Box2D primitive that matches the name: whether the body
// responds to rotational forces at all.
int GM_CDECL gmf_pb_setDoesRotationResponse(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 1);
    b->body->SetFixedRotation(v == 0);
    return GM_OK;
}
// doesRotationResponse() getter -- same evidence situation as the setter
// (no decompiled body), but confirmed real by usage (Player.gm's hang-time
// code branches on it every frame: `if(pb.doesRotationResponse()){...}`,
// and calling it on a real PhysicsBody userobject with no such method
// registered is a hard "getdot failed" GM error here, not a silent
// null-call -- this was crashing that whole code path). Mirrors the
// setter's mapping exactly.
int GM_CDECL gmf_pb_doesRotationResponse(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushInt(b->body->IsFixedRotation() ? 0 : 1);
    return GM_OK;
}
// getRotateVel/setRotateVel -- confirmed real (Player.gm:
// `pb.getRotateVel()`, used both as a magnitude and sign-tested with
// `>= 0`) -- Box2D's own angular velocity is exactly this, no fabricated
// state needed.
int GM_CDECL gmf_pb_getRotateVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    a_thread->PushFloat(b->body->GetAngularVelocity());
    return GM_OK;
}
int GM_CDECL gmf_pb_setRotateVel(gmThread* a_thread) {
    PhysicsBodyData* b = ThisBody(a_thread); if (!b) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    b->body->SetAngularVelocity(v);
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
    {"getRadius", gmf_pb_getRadius, nullptr},
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
    {"setIsSensor", gmf_pb_setIsSensor, nullptr},
    {"isSensor", gmf_pb_isSensor, nullptr},
    {"setIsCollidable", gmf_pb_setIsCollidable, nullptr},
    {"isCollidable", gmf_pb_isCollidable, nullptr},
    {"setFriction", gmf_pb_setFriction, nullptr},
    {"setRestitution", gmf_pb_setRestitution, nullptr},
    {"setShapeDensity", gmf_pb_setShapeDensity, nullptr},
    {"setMass", gmf_pb_setMass, nullptr},
    {"getMass", gmf_pb_getMass, nullptr},
    {"getShapeObjectID", gmf_pb_getShapeObjectID, nullptr},
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
    if (b->body) GetPhysicsWorld().DestroyBody(b->body);
    delete b;
}

// --- PhysicsData (the per-entity container of bodies) -------------------

struct PhysicsDataObj {
    std::vector<gmVariable> bodies; // each a PhysicsBody user object
    // The entity table this PhysicsData belongs to, if any -- see
    // MakePhysicsData/getCollidingEntities.
    gmVariable ownerEntity = gmVariable::s_null;
};

PhysicsDataObj* ThisData(gmThread* a_thread) {
    return (PhysicsDataObj*)a_thread->ThisUserCheckType(g_physicsDataType);
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
// rebuilding them fresh. Erasing from d->bodies (after UnrootGmVariable)
// makes each PhysicsBody user object collectible; its own GC destructor
// (PhysicsBodyGCDestruct above) already calls GetPhysicsWorld().
// DestroyBody() on collection, so the actual Box2D teardown reuses that
// exact same path rather than duplicating it here.
int GM_CDECL gmf_pd_removeBody(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    int index = -1;
    if (a_thread->GetNumParams() >= 1) index = a_thread->ParamInt(0, -1);
    gmMachine& m = *a_thread->GetMachine();
    if (index < 0) {
        for (gmVariable& bv : d->bodies) UnrootGmVariable(m, bv);
        d->bodies.clear();
    } else if (index < (int)d->bodies.size()) {
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
// j=it.get(); Physics.deleteJoint(j); }`). This engine has no Box2D joint
// system at all (no createJoint/deleteJoint modeled anywhere in this
// codebase, and grepping the corpus turns up no OTHER real joint-creation
// call site either) -- so the only physically-correct answer is "zero
// joints exist", matching an always-empty iterator (same EntityIterData
// type getCollidingEntities uses above) rather than null (which would
// hard-crash the very next `it.isValid()` call with "getdot failed").
int GM_CDECL gmf_pd_getJointsOnMe(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* iterObj = m.AllocUserObject(new EntityIterData{}, g_entityIterType);
    a_thread->Push(gmVariable(iterObj));
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
// getCollidingShapeIDList() argument (real retail: a reusable
// IntListRefContainer, `this.tmpIntIterator=INTLISTREFCONTAINER()`) is
// accepted but not required to be a real object -- IntListRefContainer
// itself is otherwise never used standalone in the corpus, so a fresh
// iterator is returned each call instead of populating a passed-in one;
// functionally identical for every real call site, all of which just
// immediately iterate isValid()/goToNext()/get().
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
};
struct CollisionInfoData {
    std::vector<b2Body*> collidingMyBodies;
    std::vector<ShapeContactEntry> contacts;
    // isAllSensors() backing -- see gmf_ci_isAllSensors's own header comment.
    // Vacuously true (matches "no contact disqualified this" default) until
    // BuildCollisionInfo finds a touching fixture pair where NEITHER side is
    // a sensor.
    bool allSensors = true;
};
struct ShapeCollisionInfoData {
    std::vector<int> myShapeIds;
    std::vector<b2Vec2> normals;
    std::vector<ContactPoints> points;
};
struct IntIterData {
    std::vector<int> values;
    size_t index = 0;
};

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
            if (!myFixture->IsSensor() && !otherFixture->IsSensor()) info->allSensors = false;

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
            if (!entry) { info->contacts.push_back(ShapeContactEntry{otherShapeId, {}, {}, {}}); entry = &info->contacts.back(); }
            entry->myShapeIds.push_back(myShapeId);
            entry->normals.push_back(itToMe);
            entry->points.push_back(cp);
        }
    }
    return info;
}

int GM_CDECL gmf_pd_getCollisionInfo(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    CollisionInfoData* info = BuildCollisionInfo(m, d, a_thread->Param(0));
    gmUserObject* obj = m.AllocUserObject(info, g_collisionInfoType);
    a_thread->Push(gmVariable(obj));
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
int GM_CDECL gmf_ci_getCollidingShapeIDList(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    IntIterData* it = new IntIterData();
    for (auto& e : info->contacts) it->values.push_back(e.otherShapeId);
    gmUserObject* obj = a_thread->GetMachine()->AllocUserObject(it, g_intIterType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}
int GM_CDECL gmf_ci_getShapeCollisionInfo(gmThread* a_thread) {
    CollisionInfoData* info = (CollisionInfoData*)a_thread->ThisUserCheckType(g_collisionInfoType);
    if (!info) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(sid, 0, -1);
    for (auto& e : info->contacts) {
        if (e.otherShapeId == sid) {
            ShapeCollisionInfoData* sci = new ShapeCollisionInfoData{e.myShapeIds, e.normals, e.points};
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
    ShapeCollisionInfoData* sci = new ShapeCollisionInfoData{e.myShapeIds, e.normals, e.points};
    gmUserObject* obj = a_thread->GetMachine()->AllocUserObject(sci, g_shapeCollisionInfoType);
    a_thread->Push(gmVariable(obj));
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
    b2Vec2 v = b->body->GetLinearVelocity();
    a_thread->Push(MakeVector2D(m, v.x / kScriptVelocityToSimScale, v.y / kScriptVelocityToSimScale));
    return GM_OK;
}
int GM_CDECL gmf_pd_setVel(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) b->body->SetLinearVelocity(b2Vec2(x * kScriptVelocityToSimScale, y * kScriptVelocityToSimScale));
    return GM_OK;
}
int GM_CDECL gmf_pd_getSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    a_thread->PushFloat(b ? (b->body->GetLinearVelocity().Length() / kScriptVelocityToSimScale) : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_pd_setSpeed(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(speed, 0, 0.0f);
    float simSpeed = speed * kScriptVelocityToSimScale;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        b2Vec2 v = b->body->GetLinearVelocity();
        float len = v.Length();
        v = (len > 0.0001f) ? (simSpeed / len) * v : b2Vec2(simSpeed, 0.0f);
        b->body->SetLinearVelocity(v);
    }
    return GM_OK;
}
int GM_CDECL gmf_pd_getDir(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    gmMachine& m = *a_thread->GetMachine();
    if (!b) { a_thread->Push(MakeVector2D(m, 1.0f, 0.0f)); return GM_OK; }
    b2Vec2 v = b->body->GetLinearVelocity();
    float len = v.Length();
    v = (len > 0.0001f) ? (1.0f / len) * v : b2Vec2(1.0f, 0.0f);
    a_thread->Push(MakeVector2D(m, v.x, v.y));
    return GM_OK;
}
int GM_CDECL gmf_pd_setDir(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    float x, y;
    if (!ReadVec2Args(a_thread, *a_thread->GetMachine(), x, y)) return GM_EXCEPTION;
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        float speed = b->body->GetLinearVelocity().Length();
        b2Vec2 dir(x, y);
        float dlen = dir.Length();
        if (dlen > 0.0001f) dir *= (1.0f / dlen);
        // Must also update dirX/dirY (mirrors gmf_pb_setDir exactly) --
        // gmf_pd_thrust/gmf_pb_thrust read THESE fields, not the body's
        // current velocity direction. Without this, Player.gm's real
        // movement sequence (`this.PhysicsData.setDir(x,y);
        // this.PhysicsData.thrust();`) always thrust toward (0,0) --
        // confirmed the actual cause of "player/enemy movement doesn't
        // work at all": setDir moved the body's existing (zero, at rest)
        // velocity to face the new direction at its current (zero) speed,
        // then thrust() read the never-updated dirX/dirY=(0,0) instead.
        b->dirX = dir.x; b->dirY = dir.y;
        b->body->SetLinearVelocity(speed * dir);
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
    b2Filter filter; filter.categoryBits = 0;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
    if (b && !b->brakeFlag) { b->brakeFlag = true; b->brakeAccum += b->brakeAcc; }
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
int GM_CDECL gmf_pd_setAsRect(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(width, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(height, 1, 0.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (!b) return GM_OK;
    float hx = std::max(width * 0.5f, 0.001f), hy = std::max(height * 0.5f, 0.001f);
    b2Filter filter; filter.categoryBits = 0;
    bool haveFilter = false;
    for (b2Fixture* f = b->body->GetFixtureList(); f;) {
        b2Fixture* next = f->GetNext();
        if (!haveFilter) { filter = f->GetFilterData(); haveFilter = true; }
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
// setMass(density) -- confirmed real (Player.gm: `pd.setMass(this.
// baseDensity);`), delegating to body 0 same as every other PhysicsData
// pass-through here -- was entirely unregistered on PhysicsData (only
// existed on PhysicsBody), a hard "getdot failed" GM error every time hit.
int GM_CDECL gmf_pd_setMass(gmThread* a_thread) {
    PhysicsDataObj* d = ThisData(a_thread); if (!d) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(mass, 0, 1.0f);
    PhysicsBodyData* b = MainBody(a_thread, d);
    if (b) {
        for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetDensity(mass);
        SyncBodyTypeFromMass(b->body);
    }
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

} // namespace

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
// The gmVariable (PhysicsBody handle) that wraps a_body, if any -- searches
// the owning PhysicsData's body list for the matching b2Body* (bodies
// don't store their own PhysicsBodyData* directly; only their owning
// PhysicsDataObj*, via SetUserData in gmf_pd_createBody).
gmVariable FindPhysicsBodyVarForB2Body(gmMachine& a_machine, b2Body* a_body) {
    PhysicsDataObj* owner = (PhysicsDataObj*)(uintptr_t)a_body->GetUserData().pointer;
    if (!owner) return gmVariable::s_null;
    for (gmVariable& v : owner->bodies) {
        PhysicsBodyData* bd = GetBodyData(a_machine, v);
        if (bd && bd->body == a_body) return v;
    }
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

gmVariable MakePhysicsData(gmMachine& a_machine, gmVariable a_ownerEntity) {
    PhysicsDataObj* data = new PhysicsDataObj();
    data->ownerEntity = a_ownerEntity;
    RootGmVariable(a_machine, data->ownerEntity);
    gmUserObject* obj = a_machine.AllocUserObject(data, g_physicsDataType);
    return gmVariable(obj);
}

// ENGINE PATCH (movement-speed session): retail's per-tick movement
// integrator, the single biggest missing piece of this engine's physics.
// Retail runs this on every PhysicsShape every tick (vtable+0xd0 @
// 0x00415c30, with the rotation half factored out into FUN_00415b70);
// this engine had NO equivalent at all. Every movement field scripts set
// -- velocityDecayRate, maxSpeed, thrustAcc, brakeAcc, maxVelocityDecayRate
// -- was stored and then never read by anything, and thrust() faked the
// whole mechanic by snapping velocity straight to maxSpeed. That is why
// velocity was never removed once applied (a body kept gliding after the
// key was released) and why no amount of retuning the decay constant
// changed anything: nothing consumed it.
//
// The model, straight from the decompile (see ComputeDecayFactor's header
// comment for the clean rotation-twin listing this is derived from):
//     vel   *= velocityDecayFactor              // whole-velocity drag
//     vel   += thrustAccum * dir * dt           // this tick's thrust input
//     speed  = |vel|
//     excess = max(speed - maxSpeed, 0)
//     speed -= (1 - maxVelocityDecayFactor)*excess + dt*brakeAccum
//     vel    = normalize(vel) * max(speed, 0)
// then every accumulator and guard flag is cleared for the next tick
// (retail FUN_004159c0).
//
// dt is retail's own fixed per-tick constant (16.666666 ms, read from
// 0x00522734), not wall-clock delta -- this engine's 60fps limiter already
// matches that cadence, and using the fixed value keeps the sim
// deterministic the way retail's is.
void StepPhysicsBodyMovement() {
    const float dt = kDecayRateTickMs;
    for (PhysicsBodyData* b : g_allPhysicsBodies) {
        if (!b || !b->body) continue;
        // A body with neither maxSpeed nor thrustAcc ever set has no
        // movement configuration at all -- leave it entirely to Box2D
        // rather than invent motion for it.
        const bool hasMaxSpeed = (b->maxSpeed < 1e9f);
        float thrustAcc = b->thrustAcc;
        if (thrustAcc >= 1e9f) {
            // thrustAcc unset: retail's constructor default isn't
            // recoverable, but with the excess fully shed each tick (the
            // maxVelocityDecayRate=0 default) any sufficiently large value
            // produces the same observable result -- reaching maxSpeed in
            // one tick. Use exactly that rather than a 1e9 sentinel, which
            // would work but push absurd intermediate magnitudes through
            // the solver.
            thrustAcc = hasMaxSpeed ? (b->maxSpeed / dt) : 0.0f;
        }
        float brakeAcc = (b->brakeAccum != 0.0f && b->brakeAcc >= 1e9f)
                             ? (hasMaxSpeed ? (b->maxSpeed / dt) : 0.0f)
                             : b->brakeAccum;

        // --- linear ---
        b2Vec2 vel = b->body->GetLinearVelocity();
        vel *= b->velocityDecayRate;
        if (b->thrustAccum != 0.0f) {
            float acc = (b->thrustAcc >= 1e9f)
                            ? ((b->thrustAccum > 0.0f) ? thrustAcc : -thrustAcc)
                            : b->thrustAccum;
            float step = acc * kScriptVelocityToSimScale * dt;
            vel += b2Vec2(b->dirX * step, b->dirY * step);
        }
        float speed = vel.Length();
        if (speed > 0.0001f) {
            float simMax = hasMaxSpeed ? (b->maxSpeed * kScriptVelocityToSimScale) : speed;
            float excess = speed - simMax;
            if (excess < 0.0f) excess = 0.0f;
            speed -= (1.0f - b->maxVelocityDecayRate) * excess;
            speed -= dt * brakeAcc * kScriptVelocityToSimScale;
            if (speed < 0.0f) speed = 0.0f;
            b2Vec2 dir = vel;
            dir *= (1.0f / vel.Length());
            vel = speed * dir;
        } else {
            vel.SetZero();
        }
        b->body->SetLinearVelocity(vel);

        // --- angular (facing direction) ---
        if (b->rotAccum != 0.0f || b->rotVel != 0.0f) {
            float rotAcc = b->rotAccum;
            if (b->rotateAcc >= 1e9f && rotAcc != 0.0f) {
                rotAcc = (rotAcc > 0.0f) ? 1.0f : -1.0f;
            }
            b->rotVel *= b->rotationDecayRate;
            b->rotVel += dt * rotAcc;
            if (b->maxRotateSpeed < 1e9f) {
                float mag = std::fabs(b->rotVel);
                float excess = mag - b->maxRotateSpeed;
                if (excess > 0.0f) {
                    mag -= (1.0f - b->maxRotationDecayRate) * excess;
                    b->rotVel = (b->rotVel < 0.0f) ? -mag : mag;
                }
            }
            if (b->rotVel != 0.0f) {
                float ang = std::atan2(b->dirY, b->dirX) + b->rotVel * dt;
                b->dirX = std::cos(ang);
                b->dirY = std::sin(ang);
            }
        }

        // GE3_MOVE_DEBUG=1: dump the real per-tick movement state for any
        // body that is actually being driven, so on-screen speed can be
        // checked against real numbers instead of inferred. Zero cost when
        // unset, same pattern as GE3_UV_DEBUG in shape_bindings.cpp.
        static const char* s_moveDebug = std::getenv("GE3_MOVE_DEBUG");
        if (s_moveDebug && (b->thrustAccum != 0.0f || vel.LengthSquared() > 0.0f)) {
            b2Vec2 p = b->body->GetPosition();
            std::fprintf(stderr,
                "[move] id=%d pos=(%.1f,%.1f) vel=(%.1f,%.1f) |v|=%.1f "
                "dir=(%.2f,%.2f) accum=%.4g maxSpeed=%.4g decay=%.4g scale=%.1f\n",
                b->objectId, p.x, p.y, vel.x, vel.y, vel.Length(),
                b->dirX, b->dirY, b->thrustAccum, b->maxSpeed,
                b->velocityDecayRate, kScriptVelocityToSimScale);
        }

        b->thrustAccum = 0.0f;
        b->brakeAccum = 0.0f;
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
    a_machine.RegisterTypeLibrary(g_intIterType, s_intIterLib,
                                   sizeof(s_intIterLib) / sizeof(s_intIterLib[0]));
    a_machine.RegisterUserCallbacks(g_intIterType, nullptr,
        [](gmMachine*, gmUserObject* a_object) { delete (IntIterData*)a_object->m_user; }, nullptr);

    gmFunctionEntry physicsDataLib[] = {
        {"createBody", gmf_pd_createBody, nullptr},
        {"getBody", gmf_pd_getBody, nullptr},
        {"getNumBodies", gmf_pd_getNumBodies, nullptr},
        {"removeBody", gmf_pd_removeBody, nullptr},
        {"getCollidingEntities", gmf_pd_getCollidingEntities, nullptr},
        {"getJointsOnMe", gmf_pd_getJointsOnMe, nullptr},
        {"getCollisionInfo", gmf_pd_getCollisionInfo, nullptr},
        {"getPos", gmf_pd_getPos, nullptr},
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
        {"getMass", gmf_pd_getMass, nullptr},
        {"isSensor", gmf_pd_isSensor, nullptr},
        {"setIsCollidable", gmf_pd_setIsCollidable, nullptr},
        {"isCollidable", gmf_pd_isCollidable, nullptr},
        {"setRestitution", gmf_pd_setRestitution, nullptr},
        {"setMass", gmf_pd_setMass, nullptr},
        {"getAABB", gmf_pd_getAABB, nullptr},
        {"getMinPos", gmf_pd_getMinPos, nullptr},
        {"getMaxPos", gmf_pd_getMaxPos, nullptr},
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
                if (bd && bd->body) bd->body->GetUserData().pointer = 0;
                UnrootGmVariable(*a_m, bv);
            }
            delete d;
        },
        nullptr);
}
