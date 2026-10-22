#pragma once
#include "gmMachine.h"
#include <cstdint>
#include <vector>

class b2Fixture;
class b2Body;

// Collision group is 32-bit in the retail shape record; Box2D stores only
// 16 bits in b2Filter, so its contact filter asks this binding-side state.
int32_t GetPhysicsFixtureCollisionGroup(const b2Fixture* a_fixture);

// Runtime user type created by RegisterPhysicsBodyBindings. Video's
// setInterpolatedColour wrapper accepts this type as one of its two native
// colour-source forms (retail type tag 37).
gmType GetShapeCollisionInfoType();

void RegisterPhysicsBodyBindings(gmMachine& a_machine);

// Creates the retail IntListRefContainer user object used by native Physics
// queries that return shape IDs.
gmVariable MakeIntListRefContainer(gmMachine& a_machine, std::vector<int> a_values);

// Replays retail's PhysicsShape movement update immediately before the world
// step. a_dt is the millisecond delta passed to both phases.
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
