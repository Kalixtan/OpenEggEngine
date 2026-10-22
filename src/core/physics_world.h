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

void SetPhysicsNumFramesToIgnoreCollisions(int a_frames);
int GetPhysicsNumFramesToIgnoreCollisions();
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

void DisableRestitutionThisFrameForBody(b2Body* a_body);
void MarkContactDisabled(b2Contact* a_contact);
void MarkPhysicsBodyExplicitlySlept(b2Body* a_body);

void MarkEntityPhysicsBodyForTeardown(b2Body* a_body);
void ClearEntityPhysicsBodyTeardown(b2Body* a_body);

void StepPhysicsWorld(float a_dt);
