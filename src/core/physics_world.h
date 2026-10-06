#pragma once

class b2World;

// Single shared Box2D world for every PhysicsBody (see
// src/bindings/physics_body_bindings.h) -- real scripts create one body per
// entity (sometimes several, via PhysicsData.createBody()) but there's no
// evidence of more than one simulation space (no call site ever selects a
// "world" to create a body in), so one process-wide b2World matches real
// usage.
b2World& GetPhysicsWorld();

class b2Contact;
class b2Body;

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

// Steps the shared world once with the incoming frame delta and retail's
// (2, 2) solver iteration counts recovered from FUN_0035d28 -> FUN_0015a140.
void StepPhysicsWorld(float a_dt);
