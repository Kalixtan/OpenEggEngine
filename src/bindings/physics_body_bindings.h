#pragma once
#include "gmMachine.h"

class b2Fixture;
class b2Body;

// PhysicsBody ("pb" in real scripts, e.g. Platformer/Player.gm:
// `local pb=this.PhysicsData.getBody(this.PhysicsData.createBody());`) and
// PhysicsData (the per-entity container of bodies, e.g.
// `PhysicsData=e.getPhysicsData()` in Platformer/Player.gm:12) -- confirmed
// real by extensive corpus usage (Player.gm's constructor alone: getBody,
// createBody, appendShapes, setCollisionCategory, setMaxSpeed,
// setVelocityDecayRate, setMaxRotateSpeed, setRotationDecayRate,
// setRotateAcc, setIsUsingContinuousPhysics, getShapeObjectID, getDir).
// container-level (PhysicsData/gmf_pd_) API by explicit index -- an
// earlier pass of this comment claimed otherwise; that was wrong (see its
// 10 functions: createBody, replaceBody, removeBody, getBodyIndex,
// wakeUp, getBodyPos/setBodyPos/getBodyBBox by index, etc). Real Box2D
// 2.4.1 (vendor/box2d) backs both types, per the same "just import the
// real lib" approach as PHYSICSSHAPECOLLECTIONDEF (physics_bindings.h/
// .cpp), which supplies the b2FixtureDef list `appendShapes` consumes.
//
// Position, geometry, and velocity pass through without a unit conversion.
// Gravity is zero (see core/physics_world.h) since scripts drive movement.
//
// getCollisionInfo is an honest stub (no per-contact info -- point/normal/
// impulse -- tracked). getCollidingEntities is real: each b2Body created
// via PhysicsData.createBody has its Box2D user-data pointer set back to
// the owning PhysicsDataObj (see gmf_pd_createBody), so
// PhysicsData.getCollidingEntities can walk every owned body's real
// b2Body::GetContactList(), keep only touching contacts, and resolve the
// other side's owning PhysicsDataObj -> its ownerEntity table.
void RegisterPhysicsBodyBindings(gmMachine& a_machine);

// Replays the verified retail PhysicsShape movement update immediately
// before the physics world step. a_dtSeconds is passed unchanged to both
// operations, as in the retail physics manager.
void StepPhysicsBodyMovement(float a_dt);

// Completes removals requested from Box2D contact callbacks after the world
// releases its step lock. Called by StepPhysicsWorld.
void FlushDeferredPhysicsBodyDestructions();

// For entity_bindings.cpp: creates a fresh, empty PhysicsData (no bodies
// yet) for a newly-created entity handle. a_ownerEntity is the entity
// table this PhysicsData belongs to (for getCollidingEntities to resolve
// contacts back to entity identities) -- pass gmVariable::s_null if none
// (e.g. ground/scenery PhysicsData not owned by a script entity table).
gmVariable MakePhysicsData(gmMachine& a_machine, gmVariable a_ownerEntity = gmVariable::s_null,
                           int a_type = 0);

// For entity_bindings.cpp's getMinCX/getMaxCX/getMinCY/getMaxCY: reads a
// PhysicsData handle's main body AABB (same computation as
// PhysicsData.getAABB()) without needing a gmThread/GM call. Returns false
// (and zeroes the outputs) if a_physicsDataVar isn't a real PhysicsData or
// has no body yet.
bool GetPhysicsDataAABB(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                         float& a_minX, float& a_minY, float& a_maxX, float& a_maxY);

// For entity_bindings.cpp's generic entity-handle getMainBody() -- see this
// function's own header comment in physics_body_bindings.cpp. Returns
// gmVariable::s_null if a_physicsDataVar isn't a real PhysicsData or has no
// bodies yet.
gmVariable GetPhysicsDataMainBodyVar(gmMachine& a_machine, const gmVariable& a_physicsDataVar);

// Used by the entity scheduler to distinguish a disabled entity awaiting
// normal screen registration from one whose screen-change cleanup has
// already removed every body.
bool PhysicsDataHasBodies(gmMachine& a_machine, const gmVariable& a_physicsDataVar);

// Returns the main Box2D body's origin, as PhysicsData.getPos() does.
bool GetPhysicsDataPos(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                       float& a_x, float& a_y);

// For entity_bindings.cpp's CreateEntityFromFactory: real factories build
// their returned entity table via `PhysicsData=_entity.getPhysicsData()`
// inside the object literal, i.e. BEFORE that literal (the final,
// script-visible entity table) exists at all -- `_entity` there is the
// native "handle" table the factory was called with, a DIFFERENT object.
// So the PhysicsData created at that point has its owner captured as the
// handle, not the final table, even though dual-injection
// (InjectEntityHandleMethods on both) makes method calls on either look
// consistent. Confirmed a real bug, not theoretical: Player.gm's
// `local pd=e.PhysicsData; ... pd.isCollidable()` on an entity returned by
// PhysicsData.getCollidingEntities() (which resolves the OWNER from the
// PhysicsData's captured owner) got a table missing every field the real
// entityTable has, since PhysicsData.getCollidingEntities returns the
// captured owner, not necessarily the final table -- "getdot failed"
// calling a method that only really exists on the final table. Called
// once, right after dual-injection, to repoint ownership at the real
// final table.
void SetPhysicsDataOwnerEntity(gmMachine& a_machine, const gmVariable& a_physicsDataVar, const gmVariable& a_ownerEntity);

// For physics_global_bindings.cpp (the "Physics" global class) -- the real
// global shape-id registry (see RegisterShapeGlobalId's comment,
// physics_body_bindings.cpp) and lookups from a raw Box2D body back to the
// script-visible handles built on top of it.
b2Fixture* FindFixtureByGlobalId(int a_id);
gmVariable MakePhysicsShapeByGlobalId(gmMachine& a_machine, int a_id);
gmVariable FindPhysicsBodyVarForB2Body(gmMachine& a_machine, b2Body* a_body);
gmVariable FindOwnerEntityForB2Body(b2Body* a_body);
bool IsPhysicsBodyVar(const gmVariable& a_var);
b2Body* GetB2BodyFromVar(gmMachine& a_machine, const gmVariable& a_var);
// For entity_bindings.cpp's native SauceBase draw hook -- see
// physics_body_bindings.cpp's own header comment on this function.
bool GetPhysicsBodyPosAndDir(gmMachine& a_machine, const gmVariable& a_var,
                              float& a_x, float& a_y, float& a_dirX, float& a_dirY);
