#pragma once
#include "gmMachine.h"

class b2Fixture;
class b2Body;

// Shared with player_renderer.cpp: raw b2Body::GetLinearVelocity() is in
// scaled sim-space (see kScriptVelocityToSimScale's own definition/header
// comment in physics_body_bindings.cpp), NOT script-space -- any code that
// reads a body's velocity directly (bypassing getVel()/getSpeed(), which
// already divide this back out for script callers) must divide by this
// too, or speed comparisons against script-space constants (maxSpeedX etc.)
// come out ~1000x too large.
//
// ENGINE PATCH (player-movement-speed session): the whole
// world-half-height-ratio term is GONE, and this constant is now exactly
// 1000.0f -- the real, 1:1-derived value rather than an empirical fudge.
//
// Derivation (all confirmed): retail stores position and velocity in the
// SAME world units (no pixel<->meter conversion exists anywhere in the
// corpus -- see this header's PhysicsBody comment) and integrates on a
// fixed per-tick dt of 16.666666 MILLISECONDS, read live from the retail
// .exe's static data at 0x00522734 (winedbg, this session; it's the dt
// every PhysicsShape decay setter at 0x004513d0 consumes). So a script
// velocity literal is in units-per-MILLISECOND. Box2D's b2World::Step
// takes dt in SECONDS. The conversion between the two is therefore exactly
// 1000, a pure unit conversion with no free parameter -- nothing to
// calibrate, nothing to eyeball.
//
// Sanity check against real content: Player.gm's `setMaxSpeed(0.972)`
// becomes 972 units/sec on an 8000-unit-tall GameBoard, i.e. crossing it
// in ~8 seconds -- a sensible traversal time for this game, and it lands
// safely under the (now rescaled) b2_maxTranslation cap, see
// b2_settings.h's own b2_lengthUnitsPerMeter note.
//
// The old form multiplied this by (worldHalfHeight/10.98), a Sound
// Shapes-era correction that made the effective scale ~150000-364000x --
// which, combined with Box2D's per-step translation clamp, is why script
// maxSpeed values had no observable relationship to actual on-screen speed.
constexpr float kScriptVelocityToSimScale = 1000.0f; // ms -> s; retail dt = 16.666666ms (0x00522734)

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
// Position/velocity are stored 1:1 in Box2D's own units (no evidence of a
// pixel<->meter conversion constant anywhere in the corpus -- scripts pass
// their own BACK_CONVERSION-scaled values straight through) and gravity is
// zero (see core/physics_world.h) since scripts drive movement explicitly.
//
// getCollisionInfo is an honest stub (no per-contact info -- point/normal/
// impulse -- tracked). getCollidingEntities is real: each b2Body created
// via PhysicsData.createBody has its Box2D user-data pointer set back to
// the owning PhysicsDataObj (see gmf_pd_createBody), so
// PhysicsData.getCollidingEntities can walk every owned body's real
// b2Body::GetContactList(), keep only touching contacts, and resolve the
// other side's owning PhysicsDataObj -> its ownerEntity table.
void RegisterPhysicsBodyBindings(gmMachine& a_machine);

// Retail's per-tick movement integrator -- see this function's own header
// comment in physics_body_bindings.cpp. Must run once per tick, BEFORE
// StepPhysicsWorld, so the velocities it computes are what Box2D then
// integrates into positions.
void StepPhysicsBodyMovement();

// For entity_bindings.cpp: creates a fresh, empty PhysicsData (no bodies
// yet) for a newly-created entity handle. a_ownerEntity is the entity
// table this PhysicsData belongs to (for getCollidingEntities to resolve
// contacts back to entity identities) -- pass gmVariable::s_null if none
// (e.g. ground/scenery PhysicsData not owned by a script entity table).
gmVariable MakePhysicsData(gmMachine& a_machine, gmVariable a_ownerEntity = gmVariable::s_null);

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
gmVariable FindPhysicsBodyVarForB2Body(gmMachine& a_machine, b2Body* a_body);
gmVariable FindOwnerEntityForB2Body(b2Body* a_body);
bool IsPhysicsBodyVar(const gmVariable& a_var);
b2Body* GetB2BodyFromVar(gmMachine& a_machine, const gmVariable& a_var);
// For entity_bindings.cpp's native SauceBase draw hook -- see
// physics_body_bindings.cpp's own header comment on this function.
bool GetPhysicsBodyPosAndDir(gmMachine& a_machine, const gmVariable& a_var,
                              float& a_x, float& a_y, float& a_dirX, float& a_dirY);
