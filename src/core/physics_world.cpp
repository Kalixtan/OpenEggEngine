#include "physics_world.h"
#include "engine_context.h"
#include "../bindings/physics_body_bindings.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "gmCall.h"
#include <box2d/box2d.h>

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
// matters for genuine solid contacts anyway. No EndContact dispatch -- no
// "onEntityUntouch"-style hook was found anywhere in the corpus to call.
class ScriptContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* a_contact) override {
        Dispatch(a_contact, "onEntityCollide", true);
        Dispatch(a_contact, "onEntityTouch", false);
    }
    void PreSolve(b2Contact* a_contact, const b2Manifold*) override {
        Dispatch(a_contact, "onEntityCollidePreSolve", false);
    }

private:
    void Dispatch(b2Contact* a_contact, const char* a_method, bool a_twoArgs) {
        gmMachine* m = GetEngineContext().machine;
        if (!m) return;
        b2Body* bodyA = a_contact->GetFixtureA()->GetBody();
        b2Body* bodyB = a_contact->GetFixtureB()->GetBody();
        gmVariable ownerA = FindOwnerEntityForB2Body(bodyA);
        gmVariable ownerB = FindOwnerEntityForB2Body(bodyB);
        CallIfDefined(m, ownerA, a_method, ownerB, a_twoArgs);
        CallIfDefined(m, ownerB, a_method, ownerA, a_twoArgs);
    }

    void CallIfDefined(gmMachine* m, const gmVariable& a_self, const char* a_method,
                        const gmVariable& a_other, bool a_twoArgs) {
        if (a_self.m_type != GM_TABLE || a_other.m_type != GM_TABLE) return;
        gmTableObject* table = (gmTableObject*)m->GetObject(a_self.m_value.m_ref);
        bool hasMethod = table && table->Get(m, a_method).m_type == GM_FUNCTION;
        if (!hasMethod) return;
        gmCall call;
        if (call.BeginTableFunction(m, a_method, table, a_self)) {
            call.AddParam(a_other);
            // Real 2-arg signature (top-level onEntityCollide=function(who,
            // itToMe)) expects a second identifying-body-ish argument --
            // no faithful reconstruction of "itToMe" attempted here (every
            // real call site that reads it only checks isAllSensors() on a
            // freshly-fetched PhysicsData.getCollisionInfo(who), not on this
            // param directly), so null rather than guessing at a shape.
            if (a_twoArgs) call.AddParamNull();
            call.End();
        }
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

// ENGINE PATCH: fixed-timestep accumulator. Was feeding Box2D the raw,
// variable real-frame delta (SDL_GetTicks difference) straight into
// Step() every frame -- Box2D's own manual is explicit that this is
// wrong ("Don't use a variable time step... it leads to a non-
// deterministic simulation" -- b2World::Step's header comment in
// box2d.h says the same). Variable dt destabilizes exactly the cases
// that were reported as "twitchier than retail on a few objects":
// resting/ramp contact resolution and joint constraints are the parts
// of the solver most sensitive to per-step dt jitter, while free-flight
// motion (unaffected by contacts) looks fine either way -- matching the
// "only some objects" symptom precisely. Not a Box2D bug -- this was
// this engine's own misuse of the API. Real PS3 output is 60Hz-locked
// (no confirmed physics-specific tick-rate constant exists in the
// corpus -- BASE_MS_PER_STEP is a music-beat subdivision, not a physics
// tick), so step in fixed 1/60s increments and carry the remainder,
// capping steps-per-frame so a debugger pause/hitch can't spiral.
constexpr float kFixedStep = 1.0f / 60.0f;
constexpr int kMaxStepsPerFrame = 5;
static float g_accumulator = 0.0f;

void StepPhysicsWorld(float a_dtSeconds) {
    if (a_dtSeconds <= 0.0f) return;
    // Clamp to avoid a huge catch-up burst after a debugger pause/hitch.
    if (a_dtSeconds > 0.25f) a_dtSeconds = 0.25f;
    g_accumulator += a_dtSeconds;
    int steps = 0;
    while (g_accumulator >= kFixedStep && steps < kMaxStepsPerFrame) {
        GetPhysicsWorld().Step(kFixedStep, 8, 3);
        g_accumulator -= kFixedStep;
        ++steps;
    }
    // Steps-per-frame cap reached (sustained hitch) -- drop the rest
    // rather than let the accumulator grow unbounded and burst-catch-up
    // later.
    if (steps >= kMaxStepsPerFrame) g_accumulator = 0.0f;
}
