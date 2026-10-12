#pragma once

#include <cstdint>
#include <vector>
#include "gmVariable.h"

class b2World;
class b2Contact;
class b2Body;

struct PhysicsContactRecordView {
    b2Contact* contact = nullptr;
    int myShapeId = -1;
    int otherShapeId = -1;
    float normalSign = 1.0f;
};

// Single shared Box2D world for every PhysicsBody (see
// src/bindings/physics_body_bindings.h) -- real scripts create one body per
// entity (sometimes several, via PhysicsData.createBody()) but there's no
// evidence of more than one simulation space (no call site ever selects a
// "world" to create a body in), so one process-wide b2World matches real
// usage.
b2World& GetPhysicsWorld();

// Bytecode-verified PhysicsManager +0x5c. FUN_00035d28 decrements a positive
// value after the world step and skips the post-step contact-pair traversal.
void SetPhysicsNumFramesToIgnoreCollisions(int a_frames);
int GetPhysicsNumFramesToIgnoreCollisions();
// Retail FUN_00036f48 removes contact records involving this Entity from all
// PhysicsData maps. Mirror its observable pair-membership cleanup by ID.
void RemovePhysicsContactPairsForEntity(int a_entityId);
// Remove only contacts owned by a body (a PhysicsData can own multiple bodies).
void RemovePhysicsContactPairsForBody(b2Body* a_body);
// Copies the persistent per-side records created at contact-add and retained
// until contact-end, preserving their insertion order for CollisionInfo views.
bool GetPhysicsContactRecordsForPhysicsData(uintptr_t a_myPhysicsData,
                                            uint32_t a_otherEntityId,
                                            std::vector<PhysicsContactRecordView>& a_out);
// Returns the current manifold point count only for a contact still retained
// by the listener registry; false prevents callers from dereferencing stale
// Box2D contact pointers held by copied CollisionInfo views.
bool GetPhysicsContactManifoldPointCount(b2Contact* a_contact, int& a_count);
bool GetPhysicsContactWorldNormal(b2Contact* a_contact, float a_orientationSign,
                                  float& a_x, float& a_y);
bool GetPhysicsContactWorldPoint(b2Contact* a_contact, int a_pointIndex,
                                 float& a_x, float& a_y);
// Projects the retained directed pair map in its other-Entity-ID order.
// Empty pairs remain visible during synchronous onEntitySeparate dispatch,
// matching retail's callback-before-outer-map-erase window.
void GetPhysicsCollidingEntitiesForPhysicsData(uintptr_t a_myPhysicsData,
                                              std::vector<gmVariable>& a_out);
bool PhysicsDataHasAnyCollisionPairs(uintptr_t a_myPhysicsData);
bool PhysicsDataPairIsColliding(uintptr_t a_myPhysicsData,
                                uint32_t a_otherEntityId);

// PhysicsShape.disableRestitutionThisFrame (retail 0x0008fd14) and
// Physics.disableContact (retail 0x000982f4). Both are real -- the engine
// previously stubbed them as "safe no-ops".
//
// Retail's bodies, decompiled:
//   FUN_00034084(body):   walks body->contactList setting a per-contact
//                         flag byte on EVERY contact of that body
//   FUN_0002d7fc(sci, i): finds the record whose index == i and ORs a bit
//                         into that contact's flag word
//
// So both are "mark a contact, let the solver honour it this step", which
// is exactly what Box2D's PreSolve hook is for. Marks are applied in
// ScriptContactListener::PreSolve and cleared after every world step, so
// they last one step like retail's own per-frame flags.
//
// Real call site, Player.gm:4488-4489 (landing-impact handling):
//     this.mainpb.disableRestitutionThisFrame();
//     sci.disableContact(i);
// With both stubbed, landings kept their restitution and the contact was
// never suppressed, so the player bounced where retail does not.
void DisableRestitutionThisFrameForBody(b2Body* a_body);
void DisableContactThisFrame(b2Contact* a_contact);

// Retail Entity teardown (FUN_000bfdb0 -> FUN_00037120 -> FUN_00036f48)
// removes contact records referencing the entity without dispatching a script
// separate callback. Suppress Box2D's synthetic EndContact during that body's
// destruction window; these hooks bracket immediate or deferred body removal.
void MarkEntityPhysicsBodyForTeardown(b2Body* a_body);
void ClearEntityPhysicsBodyTeardown(b2Body* a_body);

// Steps the shared world once with the incoming frame delta and retail's
// (2, 2) solver iteration counts recovered from FUN_0035d28 -> FUN_0015a140.
void StepPhysicsWorld(float a_dt);
