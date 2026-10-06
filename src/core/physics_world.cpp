#include <cstdio>
#include <cstdlib>
#include "physics_world.h"
#include "engine_context.h"
#include "../bindings/physics_body_bindings.h"
#include "../bindings/vector2d_bindings.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "gmCall.h"
#include <box2d/box2d.h>
#include <set>

namespace {
b2World* g_world = nullptr;

// ENGINE PATCH (2026-08-16): dispatches Box2D contact events to script
// entities -- previously entirely missing, the same class of gap as
// onKeyPress/onKeyRelease was before this session's DispatchKeyEventToEntities
// fix (entity_bindings.cpp/main.cpp). Confirmed real and load-bearing:
// Player.gm's top-level `onEntityTouch=function(who){ this.state.call(...);
// if (!this.checkDeath(who)) {...} }` / `onEntityCollide=function(who,
// itToMe){ ... }` (~line 4440-4461) is what checkDeath() (~line 4372) hooks
// into to fire the real "KillPlayer" event when touching a DANGER/FIRE-styled
// ground piece -- with NOTHING in this engine ever calling onEntityCollide/
// onEntityTouch on any entity, touching a hazard silently did nothing at
// all, no matter how solid the danger-ground rendering fix made it look.
// BeginContact fires for solid AND sensor fixtures alike (matching "touch"
// semantics for both); PreSolve does not fire for sensors (real Box2D
// behavior) but is still wired up for onEntityCollidePreSolve, whose one
// real use (Player.gm's `this.preCollisionVel=this.mainpb.getVel()`) only
// matters for genuine solid contacts anyway. EndContact dispatches
// onEntitySeparate, used by checkpoints and the
// EntityBase player-touch flag.
// One-step marks for disableRestitutionThisFrame / disableContact. Mirrors
// retail's per-contact flag bytes (see physics_world.h); cleared after each
// world step so a mark lasts exactly one step, as the retail name says.
static std::set<b2Body*> g_restitutionDisabledBodies;
static std::set<b2Contact*> g_disabledContacts;

struct DeferredContactEvent {
    gmVariable self;
    gmVariable other;
    b2Vec2 normalForSelf;
    const char* method;
    bool twoArgs;
};
static std::vector<DeferredContactEvent> g_deferredContactEvents;

void DispatchContactEvent(gmMachine* a_machine, const DeferredContactEvent& a_event) {
    if (!a_machine) return;
    if (a_event.self.m_type != GM_TABLE || a_event.other.m_type != GM_TABLE) return;
    gmTableObject* table = (gmTableObject*)a_machine->GetObject(a_event.self.m_value.m_ref);
    if (!table || table->Get(a_machine, a_event.method).m_type != GM_FUNCTION) return;
    gmCall call;
    gmObject* selfObject = a_machine->GetObject(a_event.self.m_value.m_ref);
    gmObject* otherObject = a_machine->GetObject(a_event.other.m_value.m_ref);
    // Contact events are queued while Box2D is stepping, then dispatched
    // after it unlocks. An earlier callback in the same batch can delete an
    // entity and its table can be collected before a later queued event is
    // reached. Never pass a stale GM reference as the callback argument.
    if (!selfObject || !otherObject) return;
    const bool rootSelf = selfObject &&
        !a_machine->IsCPPOwnedGMObject(selfObject);
    const bool rootOther = otherObject && otherObject != selfObject &&
        !a_machine->IsCPPOwnedGMObject(otherObject);
    if (rootSelf) a_machine->AddCPPOwnedGMObject(selfObject);
    if (rootOther) a_machine->AddCPPOwnedGMObject(otherObject);
    if (call.BeginTableFunction(a_machine, a_event.method, table, a_event.self)) {
        call.AddParam(a_event.other);
        if (a_event.twoArgs) {
            call.AddParam(MakeVector2D(*a_machine, a_event.normalForSelf.x,
                                       a_event.normalForSelf.y));
        }
        call.End();
    }
    // Only release roots this dispatch added. EntityRegistry or PhysicsData
    // may already own these objects; the generic root helper is set based,
    // not reference counted, so unconditional Unroot would erase their
    // long-lived registry root and create delayed GC crashes.
    if (rootSelf && a_machine->IsCPPOwnedGMObject(selfObject))
        a_machine->RemoveCPPOwnedGMObject(selfObject);
    if (rootOther && a_machine->IsCPPOwnedGMObject(otherObject))
        a_machine->RemoveCPPOwnedGMObject(otherObject);
}

void QueueContactEvent(gmMachine* a_machine, const gmVariable& a_self,
                       const gmVariable& a_other, const b2Vec2& a_normal,
                       const char* a_method, bool a_twoArgs) {
    if (!a_machine || a_self.m_type != GM_TABLE || a_other.m_type != GM_TABLE) return;
    g_deferredContactEvents.push_back(
        {a_self, a_other, a_normal, a_method, a_twoArgs});
}

class ScriptContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* a_contact) override {
        QueueDispatch(a_contact, "onEntityCollide", true);
        QueueDispatch(a_contact, "onEntityTouch", false);
    }
    void EndContact(b2Contact* a_contact) override {
        QueueDispatch(a_contact, "onEntitySeparate", false);
    }
    void PreSolve(b2Contact* a_contact, const b2Manifold*) override {
        Dispatch(a_contact, "onEntityCollidePreSolve", false);
        // Apply this step's disableRestitutionThisFrame/disableContact
        // marks. PreSolve is the only place Box2D honours either: its
        // SetEnabled(false) is documented as lasting for the current step
        // only, and restitution set here is used by the solver for this
        // step's impulse. See physics_world.h for the retail bodies.
        if (!g_restitutionDisabledBodies.empty()) {
            b2Body* a = a_contact->GetFixtureA()->GetBody();
            b2Body* b = a_contact->GetFixtureB()->GetBody();
            if (g_restitutionDisabledBodies.count(a) || g_restitutionDisabledBodies.count(b)) {
                a_contact->SetRestitution(0.0f);
            }
        }
        if (g_disabledContacts.count(a_contact)) {
            a_contact->SetEnabled(false);
        }
    }

private:
    void QueueDispatch(b2Contact* a_contact, const char* a_method, bool a_twoArgs) {
        gmMachine* m = GetEngineContext().machine;
        if (!m) return;
        b2Body* bodyA = a_contact->GetFixtureA()->GetBody();
        b2Body* bodyB = a_contact->GetFixtureB()->GetBody();
        gmVariable ownerA = FindOwnerEntityForB2Body(bodyA);
        gmVariable ownerB = FindOwnerEntityForB2Body(bodyB);

        // Box2D's world-manifold normal points from fixture A to fixture B.
        // The script-facing callback names its second argument `itToMe`, so
        // each recipient receives the direction from the other collider to
        // itself. This is the same orientation exposed by
        // PhysicsData.getCollisionInfo(...).getContactNormal(...).
        b2WorldManifold manifold;
        a_contact->GetWorldManifold(&manifold);
        QueueContactEvent(m, ownerA, ownerB,
                          b2Vec2(-manifold.normal.x, -manifold.normal.y),
                          a_method, a_twoArgs);
        QueueContactEvent(m, ownerB, ownerA, manifold.normal, a_method, a_twoArgs);
    }

    void Dispatch(b2Contact* a_contact, const char* a_method, bool a_twoArgs) {
        gmMachine* m = GetEngineContext().machine;
        if (!m) return;
        b2Body* bodyA = a_contact->GetFixtureA()->GetBody();
        b2Body* bodyB = a_contact->GetFixtureB()->GetBody();
        gmVariable ownerA = FindOwnerEntityForB2Body(bodyA);
        gmVariable ownerB = FindOwnerEntityForB2Body(bodyB);
        b2WorldManifold manifold;
        a_contact->GetWorldManifold(&manifold);
        DispatchNow(m, ownerA, ownerB,
                    b2Vec2(-manifold.normal.x, -manifold.normal.y),
                    a_method, a_twoArgs);
        DispatchNow(m, ownerB, ownerA, manifold.normal, a_method, a_twoArgs);
    }

    void DispatchNow(gmMachine* m, const gmVariable& self, const gmVariable& other,
                     const b2Vec2& normal, const char* method, bool twoArgs) {
        if (!m || self.m_type != GM_TABLE || other.m_type != GM_TABLE) return;
        gmTableObject* table = (gmTableObject*)m->GetObject(self.m_value.m_ref);
        if (!table || table->Get(m, method).m_type != GM_FUNCTION) return;
        gmCall call;
        if (!call.BeginTableFunction(m, method, table, self)) return;
        call.AddParam(other);
        if (twoArgs) call.AddParam(MakeVector2D(*m, normal.x, normal.y));
        call.End();
    }

};

ScriptContactListener g_contactListener;
} // namespace

b2World& GetPhysicsWorld() {
    if (!g_world) {
        // Real scripts drive velocity explicitly every frame (setSpeed/
        // setVel/setMaxSpeed/setVelocityDecayRate -- see
        // Platformer/Player.gm's constructor) rather than reading/setting
        // any global gravity value (no such call site exists anywhere in
        // the corpus) -- zero gravity avoids silently adding an unconfirmed
        // force on top of that.
        g_world = new b2World(b2Vec2(0.0f, 0.0f));
        g_world->SetContactListener(&g_contactListener);
    }
    return *g_world;
}

void StepPhysicsWorld(float a_dt) {
    // Scripts use milliseconds and world-units per millisecond. Stock
    // Box2D uses seconds and world-units per second; its solver assumes
    // that time base for contact bias, friction, and position correction.
    // The bindings convert script velocities at that boundary, so only the
    // step duration changes here.
    GetPhysicsWorld().Step(a_dt * 0.001f, 2, 2);
    // Begin/touch/separate handlers may reposition bodies, rebuild fixtures,
    // or remove entities. Dispatch once Box2D unlocks, retaining the event
    // order observed during its contact traversal. PreSolve remains inline
    // above because its return-state/contact flags affect this very step.
    std::vector<DeferredContactEvent> events;
    events.swap(g_deferredContactEvents);
    gmMachine* machine = GetEngineContext().machine;
    for (const DeferredContactEvent& event : events) {
        DispatchContactEvent(machine, event);
    }
    g_restitutionDisabledBodies.clear();
    g_disabledContacts.clear();
    FlushDeferredPhysicsBodyDestructions();
}

void DisableRestitutionThisFrameForBody(b2Body* a_body) {
    if (a_body) g_restitutionDisabledBodies.insert(a_body);
}

void DisableContactThisFrame(b2Contact* a_contact) {
    if (a_contact) g_disabledContacts.insert(a_contact);
}
