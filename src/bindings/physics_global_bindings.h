#pragma once
#include "gmMachine.h"

// ~66-function native API, confirmed via a full raw-byte scan of the
// retail EBOOT.elf plus corpus usage frequency: Physics.isShapeSensor
// (47 call sites), Physics.raycastOne (85), Physics.getShapeByID (25, but
// EVERY real call site is commented out -- see physics_global_bindings.cpp),
// Physics.getBodyFromShapeID (9), Physics.getEntityFromShapeID (8). This
// engine had no "Physics" global registered at all before this pass --
// every call silently failed as a GM "getdot failed" runtime error
// (dotting a member off an undefined global), not a routed strict-call
// trap, since the identifier itself didn't exist as anything.
//
// Backed by this engine's real global shape-id registry
// (physics_body_bindings.h's RegisterShapeGlobalId/FindFixtureByGlobalId)
// and real Box2D queries (b2World::RayCast) -- not fabricated stand-ins.
void RegisterPhysicsGlobalBindings(gmMachine& a_machine);
