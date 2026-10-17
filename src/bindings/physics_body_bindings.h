#pragma once
#include "gmMachine.h"
#include <cstdint>
#include <vector>

class b2Fixture;
class b2Body;

// Runtime user type created by RegisterPhysicsBodyBindings. Video's
// setInterpolatedColour wrapper accepts this type as one of its two native
// colour-source forms (retail type tag 37).
gmType GetShapeCollisionInfoType();

// PhysicsBody ("pb" in real scripts, e.g. Platformer/Player.gm:
// `local pb=this.PhysicsData.getBody(this.PhysicsData.createBody());`) and
// PhysicsData (the per-entity container of bodies, e.g.
// `PhysicsData=e.getPhysicsData()` in Platformer/Player.gm:12) -- confirmed
// real by extensive corpus usage (Player.gm's constructor alone: getBody,
// createBody, appendShapes, setCollisionCategory, setMaxSpeed,
// setVelocityDecayRate, setMaxRotateSpeed, setRotationDecayRate,
// setRotateAcc, setIsBullet, getShapeObjectID, getDir).
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
// getCollisionInfo returns a cached GM view per active directed pair and
// refreshes its contact data from retained contact-add/remove records. Pair
// removal and PhysicsData collection invalidate/unroot that view. This models
// the retail mapped-value identity/lifetime at the script-object level, while
// the local pair registry and record layout remain adapters (see
// via PhysicsData.createBody has its Box2D user-data pointer set back to the
// owning PhysicsDataObj (see gmf_pd_createBody), so collision queries resolve
// the other PhysicsData and then its ownerEntity. getCollidingEntities sorts
// distinct other PhysicsData handles by unsigned local key, preserving retail's
// ordered-map traversal shape without deduplicating by owner Entity.
void RegisterPhysicsBodyBindings(gmMachine& a_machine);

// Creates the retail IntListRefContainer user object used by native Physics
// queries that return shape IDs.
gmVariable MakeIntListRefContainer(gmMachine& a_machine, std::vector<int> a_values);

// Replays retail's PhysicsShape movement update immediately before the world
// step. a_dt is the raw millisecond delta passed to both phases;
// StepPhysicsWorld converts it to seconds only at the Box2D call boundary.
void StepPhysicsBodyMovement(float a_dt);

// Call when entity enabled/no-physics membership or a PhysicsData owner
// changes. The movement manager rebuilds its ordered candidate snapshot only
// after those native membership transitions.
void InvalidatePhysicsMovementMembership();

// Completes removals requested from Box2D contact callbacks after the world
// releases its step lock. Called by StepPhysicsWorld.
void FlushDeferredPhysicsBodyDestructions();

// Retail CollisionInfo values are stable pair-map values while the directed
// PhysicsData pair exists. Physics world calls this when that pair is erased
// so the cached script view can be safely unrooted and invalidated.
void InvalidatePhysicsCollisionInfoView(uintptr_t a_physicsData,
                                        uint32_t a_otherEntityId);

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
// Union of every live body/fixture in PhysicsData. Used where retail's entity
// extent helper walks the complete entity-owned shape list (FUN_000302f8).
bool GetPhysicsDataAllBodiesAABB(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                                 float& a_minX, float& a_minY,
                                 float& a_maxX, float& a_maxY);

// For entity_bindings.cpp's generic entity-handle getMainBody() -- see this
// function's own header comment in physics_body_bindings.cpp. Returns
// gmVariable::s_null if a_physicsDataVar isn't a real PhysicsData or has no
// bodies yet.
gmVariable GetPhysicsDataMainBodyVar(gmMachine& a_machine, const gmVariable& a_physicsDataVar);

// Used by the entity scheduler to distinguish a disabled entity awaiting
// normal screen registration from one whose screen-change cleanup has
// already removed every body.
bool PhysicsDataHasBodies(gmMachine& a_machine, const gmVariable& a_physicsDataVar);

// Entity.setIsEnabled mirrors the retail wake/sleep transition across every
// body owned by that entity's PhysicsData (EntityBase.gm calls it after
// rebuilding bodies specifically to restore their enabled/sleep state).
void SetPhysicsDataAwake(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                         bool a_awake);

// Returns the main Box2D body's origin, as PhysicsData.getPos() does.
bool GetPhysicsDataPos(gmMachine& a_machine, const gmVariable& a_physicsDataVar,
                       float& a_x, float& a_y);

// Reads the first PhysicsBody's native direction vector, including its cached
// direction when rotation is locked. Used by Entity.getRadiansToFace.
bool GetPhysicsDataFacingDirection(gmMachine& a_machine,
                                   const gmVariable& a_physicsDataVar,
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

// EntityManager calls this after the entity's script destructor and before
// dropping its final manager root. It releases every Box2D body owned by the
// entity's PhysicsData, including the PhysicsData -> entity root, so a retained
// script handle cannot keep a deleted entity's collision bodies alive.
void DestroyPhysicsDataForEntity(gmMachine& a_machine, const gmVariable& a_entity);

// For physics_global_bindings.cpp (the "Physics" global class) -- the real
// global shape-id registry (see RegisterShapeGlobalId's comment,
// physics_body_bindings.cpp) and lookups from a raw Box2D body back to the
// script-visible handles built on top of it.
b2Fixture* FindFixtureByGlobalId(int a_id);
float GetFixturePlayerFriction(b2Fixture* a_fixture);
gmVariable MakePhysicsShapeByGlobalId(gmMachine& a_machine, int a_id);
gmVariable FindPhysicsBodyVarForB2Body(gmMachine& a_machine, b2Body* a_body);
gmVariable FindOwnerEntityForB2Body(b2Body* a_body);
bool IsPhysicsBodyVar(const gmVariable& a_var);
b2Body* GetB2BodyFromVar(gmMachine& a_machine, const gmVariable& a_var);
// For entity_bindings.cpp's native SauceBase draw hook -- see
// physics_body_bindings.cpp's own header comment on this function.
bool GetPhysicsBodyPosAndDir(gmMachine& a_machine, const gmVariable& a_var,
                              float& a_x, float& a_y, float& a_dirX, float& a_dirY);
