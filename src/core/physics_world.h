#pragma once

class b2World;

// Single shared Box2D world for every PhysicsBody (see
// src/bindings/physics_body_bindings.h) -- real scripts create one body per
// entity (sometimes several, via PhysicsData.createBody()) but there's no
// evidence of more than one simulation space (no call site ever selects a
// "world" to create a body in), so one process-wide b2World matches real
// usage.
b2World& GetPhysicsWorld();

// Steps the shared world. Called once per frame from main.cpp.
void StepPhysicsWorld(float a_dtSeconds);
