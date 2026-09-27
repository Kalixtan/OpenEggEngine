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
//
// KNOWN GAP, NOT YET IMPLEMENTED: the entire joint subsystem.
// for a full Box2D joint API that this engine has ZERO implementation of:
// world-level Physics.createJoint/deleteJoint/deleteAllJoints/getJoint/
// setupJoint/setupDistanceJoint/setupPinJoint/setupRelativeJoint, plus
// ~18 joint-INSTANCE methods (setIsBodiesCollidable/isBodiesCollidable,
// setMotor, getReactionForce/getReactionTorque, setPrismaticTranslation/
// setPrismaticWorldAxis, setRevoluteAngularConstraint/setRevoluteMotor,
// setPinAngularConstraint/setPinMotor, setAngularSpring/setLinearSpring,
// setRTMotorSpeed/setRTMaxMotorTorque/getRTMotorSpeed, getEntity1/
// getEntity2, getBody1/getBody2, setIsNeedsHighAccuracy, getAnchorPos,
// setRelativeVel). PhysicsData.getJointsOnMe() currently returns a
// permanently-empty iterator (physics_body_bindings.cpp), which is the
// only physically-honest answer while no joint can ever be created --
// but it means EntityBase.gm's `for(it=this.PhysicsData.getJointsOnMe();
// it.isValid();...) Physics.deleteJoint(it.get());` pattern is a silent
// no-op rather than actually tearing down joints, and any real level
// content relying on Box2D joints (distance/revolute/prismatic/pin
// constraints between bodies) has no collision-response counterpart at
// all in this engine.
//
// The decompiled call shapes (a single opaque "world"+joint-handle pair
// passed positionally into helpers like FUN_00036278/FUN_000348fc,
// rather than the vendored Box2D 2.4.1's own b2JointDef-subclass-per-
// joint-type object graph) suggest retail's native layer sat on an older/
// simpler joint API shape than what vendor/box2d exposes today -- porting
// this faithfully means hand-mapping the decompiled per-joint-type setter
// functions (setRevoluteMotor, setPrismaticWorldAxis, etc) onto the
// corresponding b2RevoluteJointDef/b2PrismaticJointDef/b2DistanceJointDef/
// b2PinJoint-or-equivalent fields, not just wiring up b2World::CreateJoint
// generically. Left undone in this pass given the size of the surface;
// setRelativeVel) for the full decompiled ground truth to implement
// against.
void RegisterPhysicsGlobalBindings(gmMachine& a_machine);
