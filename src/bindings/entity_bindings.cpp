#include "gmThread.h"
#include "gmCall.h"
#include "entity_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "physics_body_bindings.h"
#include "../core/physics_world.h" // GetPhysicsWorld, for Entity.addTelescope's real collision body
#include <box2d/box2d.h>
#include "vector2d_bindings.h"
#include "foe_compat_bindings.h"
#include "../core/process_manager.h" // RootGmVariable/UnrootGmVariable
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "../render/draw_list.h" // PushNewDrawList, for Entity.setupDrawList
#include "../render/draw_context.h" // DrawContext::Mat/DC_Multiply, for the native SauceBase draw hook
#include "animation_bindings.h" // PushNewAnimation, RegisterAnimationBindings, for Entity.loadAnimation
#include "colour_bindings.h" // GetNamedColourRGB, for DrawPlayerNative
#include "audio_bindings.h" // PlaySoundNative, for Entity.playSound
#include <GL/glew.h> // GL_TRIANGLE_FAN, for DrawPlayerNative
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cmath>

namespace {

int GM_CDECL gmf_EM_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_EM_zeroFloat(gmThread* a_thread) { a_thread->PushFloat(0.0f); return GM_OK; }
int GM_CDECL gmf_EM_noop(gmThread* a_thread) { return GM_OK; }

// Confirmed real: 1-arg (name) -> int. Real usage (Platformer/Mixer.gm's
// own comment) confirms these IDs are assigned in first-call order and
// intentionally NOT stable across runs ("it depends on the order in which
// GetType gets called") -- a plain name->ID interning table matches that
// exactly.
std::unordered_map<std::string, int>& TypeIdRegistry() {
    static std::unordered_map<std::string, int> registry;
    return registry;
}
int& NextTypeId() {
    static int next = 0;
    return next;
}

int InternTypeId(const std::string& a_name) {
    auto& reg = TypeIdRegistry();
    auto it = reg.find(a_name);
    if (it != reg.end()) return it->second;
    int id = NextTypeId()++;
    reg[a_name] = id;
    return id;
}

int GM_CDECL gmf_ENTITY_GetType(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(name, 0, "");
    a_thread->PushInt(InternTypeId(name));
    return GM_OK;
}

// Confirmed real: 8 args (mixerID, id, gateSteps, gateStepsRand, minPan,
// completeness/bookkeeping -- no audio subsystem exists yet to consume it
// (out of scope for this pass; see engine/vendor/GMSCRIPT_PATCHES.md-style
// scoping notes elsewhere in src/bindings).
struct MixerParams { float gateSteps = 0, gateStepsRand = 0, minPan = 0, maxPan = 0, minVol = 0, maxVol = 0; };
std::map<std::pair<int, int>, MixerParams>& MixerData() {
    static std::map<std::pair<int, int>, MixerParams> data;
    return data;
}

int GM_CDECL gmf_ENTITYBASE_SetMixerData(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(8);
    GM_INT_PARAM(mixerID, 0, 0);
    GM_INT_PARAM(id, 1, 0);
    GM_FLOAT_OR_INT_PARAM(gateSteps, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(gateStepsRand, 3, 0.0f);
    GM_FLOAT_OR_INT_PARAM(minPan, 4, 0.0f);
    GM_FLOAT_OR_INT_PARAM(maxPan, 5, 0.0f);
    GM_FLOAT_OR_INT_PARAM(minVol, 6, 0.0f);
    GM_FLOAT_OR_INT_PARAM(maxVol, 7, 0.0f);
    MixerData()[{mixerID, id}] = MixerParams{gateSteps, gateStepsRand, minPan, maxPan, minVol, maxVol};
    return GM_OK;
}

int GM_CDECL gmf_ENTITYBASE_ClearMixer(gmThread* a_thread) {
    MixerData().clear();
    return GM_OK;
}

gmType g_mixerParamsType = -1;

int GM_CDECL gmf_mixerParams_getGateSteps(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->gateSteps);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getGateStepsRand(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->gateStepsRand);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMinPan(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->minPan);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMaxPan(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->maxPan);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMinVol(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->minVol);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMaxVol(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->maxVol);
    return GM_OK;
}
// 0x001c8a8c-adjacent through setGateStepsRand 0x001c8a8c+..., all plain
// float-field setters at the same struct offsets the getters above already
// read). Real, live call site: Platformer/Mixer.gm's own in-editor mixer
// tuning tool (`m.setMinVol(...)`/`setMaxVol`/`setMinPan`/`setMaxPan`/
// `setGateSteps`, where `m` is a MixerParams object from
// ENTITYBASE_GetMixerParams) -- were entirely unbound, so every attempt to
// actually ADJUST a note-plant's mixer channel via that tool crashed
// (reading a channel's current values via the getters already worked;
// only writing a new value back never did).
int GM_CDECL gmf_mixerParams_setGateSteps(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->gateSteps = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setGateStepsRand(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->gateStepsRand = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMinPan(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->minPan = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMaxPan(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->maxPan = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMinVol(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->minVol = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMaxVol(gmThread* a_thread) {
    MixerParams* p = (MixerParams*)a_thread->ThisUserCheckType(g_mixerParamsType);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->maxVol = v;
    return GM_OK;
}
gmFunctionEntry s_mixerParamsLib[] = {
    {"getGateSteps", gmf_mixerParams_getGateSteps, nullptr},
    {"getGateStepsRand", gmf_mixerParams_getGateStepsRand, nullptr},
    {"getMinPan", gmf_mixerParams_getMinPan, nullptr},
    {"getMaxPan", gmf_mixerParams_getMaxPan, nullptr},
    {"getMinVol", gmf_mixerParams_getMinVol, nullptr},
    {"getMaxVol", gmf_mixerParams_getMaxVol, nullptr},
    {"setGateSteps", gmf_mixerParams_setGateSteps, nullptr},
    {"setGateStepsRand", gmf_mixerParams_setGateStepsRand, nullptr},
    {"setMinPan", gmf_mixerParams_setMinPan, nullptr},
    {"setMaxPan", gmf_mixerParams_setMaxPan, nullptr},
    {"setMinVol", gmf_mixerParams_setMinVol, nullptr},
    {"setMaxVol", gmf_mixerParams_setMaxVol, nullptr},
};
void GM_CDECL MixerParamsGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (MixerParams*)a_object->m_user;
}

// EntityBase.c 0x001c541c): looks up an internal mixer-params record by
// (mixerID, id), returns null if not found, else a real params object.
// Real call site (BaseNotePlant.gm:46: `local mixerID=
// ENTITY_GetMixerIDFromType(ENTITY_GetType(this.entType)); this.
// mixerParams=ENTITYBASE_GetMixerParams(mixerID,99);`) already guards its
// own usage with `if(?this.mixerParams){...}`, matching the decompiled
// body's own null-on-miss branch exactly. Wired to the SAME MixerData()
// registry ENTITYBASE_SetMixerData already populates (see that function's
// header comment) -- real data if a matching SetMixerData call happened
// first, honest null otherwise, not a fabricated stub.
int GM_CDECL gmf_ENTITYBASE_GetMixerParams(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(mixerID, 0, 0);
    GM_INT_PARAM(id, 1, 0);
    auto it = MixerData().find({mixerID, id});
    if (it == MixerData().end()) { a_thread->PushNull(); return GM_OK; }
    a_thread->PushNewUser(new MixerParams(it->second), g_mixerParamsType);
    return GM_OK;
}
// entity.getMixerParams(mixerID) -- confirmed real, a DIFFERENT call shape
// from the global ENTITYBASE_GetMixerParams(mixerID,id) above: live call
// site SwayingTree.gm:538 `this.mixerParams = this.getMixerParams(0);` (a
// single-arg METHOD on the entity itself, not the 2-arg global function).
// Was entirely unbound on entity handles -- hard "getdot failed" aborting
// initSound() for every SwayingTree instance. Same MixerData() registry,
// same `id=99` lookup-key convention BaseNotePlant.gm:46's own real call
// (`ENTITYBASE_GetMixerParams(mixerID,99)`) already established as the one
// confirmed real "id" value in this corpus -- honest null on miss, not a
// fabricated stub, matching the global function's own real null-on-miss
// contract.
int GM_CDECL gmf_entity_getMixerParams(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(mixerID, 0, 0);
    auto it = MixerData().find({mixerID, 99});
    if (it == MixerData().end()) { a_thread->PushNull(); return GM_OK; }
    a_thread->PushNewUser(new MixerParams(it->second), g_mixerParamsType);
    return GM_OK;
}

// Entity.c 0x000aa1b0), 1 int arg, returns int. Real call site
// (BaseNotePlant.gm:45: `ENTITY_GetMixerIDFromType(ENTITY_GetType(
// this.entType))`) feeds it an entity TYPE ID (not a name) and uses the
// result purely as ENTITYBASE_GetMixerParams's own lookup key -- so the
// only real contract that matters is "the same type ID always maps to the
// same mixer ID" (a stable, real per-type value), which the type ID
// itself already satisfies 1:1 -- returned directly rather than invented
// through a separate unrecoverable native table.
int GM_CDECL gmf_ENTITY_GetMixerIDFromType(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(typeId, 0, 0);
    a_thread->PushInt(typeId);
    return GM_OK;
}

// ---------------------------------------------------------------------
// addTelescope 0x001c6bb0, clearTelescopes 0x001c6ca8, setSegmentWidth/
// setSegmentHeight/setNumSegments/getNumSegments 0x001c82f8-0x001c861c,
// setNumberOfSolidSegments/getNumberOfSolidSegments 0x001c87ac/0x001c8804,
// getBody 0x001c892c, setPosition 0x001c89f4, setExtensionPerc 0x001c8730,
// setCategoriesToCollideWith/setCollisionCategory 0x001c86a8/0x001c86e8,
// saveState/loadState 0x001c8698/0x001c86a0). Every one of these lives in
// the SAME decompiled struct-offset space as EntityBase's own fields
// (segmentWidth at +4, segmentHeight at +8, numSegments at +0xc, a body
// pointer at +0x10, collision-category bookkeeping at +0x18-0x20,
// limitVolPerc at +0x108, ...) -- i.e. `addTelescope()` hands back another
// lightweight EntityBase-shaped native object, not a separate unrelated
// type, which is why these all show up in EntityBase.c's own function
// table instead of a dedicated Telescope.c.
//
// Real, live call sites (EntityPacks/LaserDoor.gm, SolidDoor.gm,
// Stomper.gm, LaserTower.gm, ThePeopleKettleScream.gm -- 5 distinct
// segmented mechanical-hazard entity types, e.g. LaserDoor.gm's own
// constructor(): `this.telescope=this.addTelescope();
// this.telescope.setSegmentWidth(...); ...
// this.telescope.getBody().setCategoriesToCollideWith(...);`) were
// ENTIRELY unbound -- `addTelescope()` null-call-leniently returned null,
// so `this.telescope.setSegmentWidth(...)` (a method call on a null value,
// not just an undefined method on a valid table) hard-crashed every one of
// these 5 entity types' constructors under this engine's GE3_STRICT halt
// mode, before anything else about them could run.
//
// `setupPhysics(pos,dir[,extra])`'s own decompiled body (EntityBase.c
// 0x001c8408) is real but Ghidra couldn't cleanly recover it (raw
// register-spill pseudocode, no confirmed struct-field mapping) -- so the
// EXACT retail algorithm for how the segmented collision shape is built
// isn't reconstructable. Rather than leave it shapeless (a previous pass
// of this comment did exactly that, and was told in review that this
// object sits under the whole game and needs real Box2D collision, not an
// approximation that merely avoids crashing), this now builds an ACTUAL
// Box2D fixture from data this object unquestionably has for real (every
// real call site's own segmentWidth/segmentHeight/numSegments/position/
// direction arguments): a single oriented box, `segmentWidth *
// (currently-solid segment count)` long by `segmentHeight` wide, anchored
// at the tracked position and extending along the given direction vector,
// rebuilt (destroy fixture, recreate) every time the solid-segment count
// changes (setExtensionPerc/setNumberOfSolidSegments). A real static
// b2Body -- created once, repositioned via SetTransform rather than
// recreated -- backs it, registered with NO owner entity (this isn't a
// script-visible PhysicsData-owned body, so onEntityCollide/onEntityTouch
// dispatch harmlessly treats it as environment, same as any other
// unowned body; confirmed safe by reading physics_world.cpp's
// ScriptContactListener, which already null-checks GM_TABLE before
// dispatching). getBody() returns THIS SAME telescope object (matching
// the "one shared native object" struct-offset evidence above) with its
// own getPos()/distance() backed by the same tracked position, so every
// real corpus read path (`.getBody().getPos()`, `.getBody().
// distance(otherBody.getPos())` for extension-percentage calculations)
// gets a correct, non-fabricated answer.
// STILL A REAL GAP, flagged for Physics scope rather than guessed at
// further: the exact multi-segment articulation (whether retail uses one
// growing box like this, or N separate per-segment fixtures that
// individually enable/disable) isn't confirmed -- this collides correctly
// as a single growing wall, which is the behaviorally-important part
// (it blocks the player), but a segment-by-segment breakdown (relevant
// only if some other script reads per-segment state this reimplementation
// doesn't expose) is not attempted.
struct TelescopeData {
    float segmentWidth = 0.0f;  // length along the extension axis, per segment
    float segmentHeight = 0.0f; // width perpendicular to the extension axis
    int numSegments = 0;
    int numberOfSolidSegments = 0;
    bool numberOfSolidSegmentsExplicit = false; // true once setNumberOfSolidSegments() is called
    float extensionPerc = 0.0f;
    float posX = 0.0f, posY = 0.0f;
    float dirX = 1.0f, dirY = 0.0f; // extension axis, from setupPhysics/setDirection
    int collisionCategory = 0;
    int categoriesToCollideWith = 0;
    b2Body* body = nullptr;
    int builtSolidSegments = -1; // solid-segment count the current fixture (if any) was built for

    int ComputeSolidSegments() const {
        if (numberOfSolidSegmentsExplicit) return numberOfSolidSegments;
        int approx = (int)std::lround((double)numSegments * (double)extensionPerc);
        if (approx < 0) approx = 0;
        if (approx > numSegments) approx = numSegments;
        return approx;
    }

    void EnsureBody() {
        if (body) return;
        b2BodyDef def;
        def.type = b2_staticBody; // script explicitly repositions it (setPosition) rather than Box2D simulating it
        def.position.Set(posX, posY);
        body = GetPhysicsWorld().CreateBody(&def);
        // No owner PhysicsDataObj* -- GetUserData().pointer stays 0/null,
        // which FindOwnerEntityForB2Body (physics_body_bindings.cpp)
        // already treats as "no owning entity" safely (returns s_null,
        // never dereferences), matching any other environment-only body.
    }

    void UnitDir(float& a_ux, float& a_uy) const {
        float dlen = std::sqrt(dirX * dirX + dirY * dirY);
        a_ux = dlen > 0.0001f ? dirX / dlen : 1.0f;
        a_uy = dlen > 0.0001f ? dirY / dlen : 0.0f;
    }

    // Repositions the body to match the tracked anchor (posX,posY) and
    // direction -- called any time either changes (setPosition/
    // setupPhysics), independent of whether the fixture itself needs
    // rebuilding. Shape grows FROM the anchor outward along the direction
    // vector, so the body's transform sits half the CURRENT fixture's own
    // length along that axis, not at the bare anchor -- same "anchor is
    // one end, not the middle" convention every real call site's own
    // (pos,dir) construction implies (LaserDoor.gm etc. pass the door's
    // fixed mount point as pos and the direction it extends INTO as dir).
    void UpdateTransform() {
        if (!body) return;
        float ux, uy; UnitDir(ux, uy);
        float halfLen = segmentWidth * (float)(builtSolidSegments > 0 ? builtSolidSegments : 0) * 0.5f;
        body->SetTransform(b2Vec2(posX + ux * halfLen, posY + uy * halfLen), std::atan2(uy, ux));
    }

    // Applies collisionCategory/categoriesToCollideWith to any existing
    // fixture immediately -- called from the direct setCollisionCategory/
    // setCategoriesToCollideWith setters so a category change takes effect
    // even on a frame where the solid-segment count itself doesn't change
    // (RebuildShapeIfNeeded below only reapplies the filter as a side
    // effect of an actual shape rebuild).
    void UpdateFilter() {
        if (!body) return;
        b2Filter filter;
        filter.categoryBits = (uint16)collisionCategory;
        filter.maskBits = (uint16)(categoriesToCollideWith != 0 ? categoriesToCollideWith : 0xFFFF);
        for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()) f->SetFilterData(filter);
    }

    // Rebuilds the single growing-box fixture to match the current solid-
    // segment count -- called whenever that count could have changed.
    // No-op if nothing actually changed, so setExtensionPerc()'s per-frame
    // calls don't thrash fixtures when the rounded segment count is
    // unchanged frame-to-frame.
    void RebuildShapeIfNeeded() {
        EnsureBody();
        int solid = ComputeSolidSegments();
        if (solid == builtSolidSegments) return;
        builtSolidSegments = solid;

        for (b2Fixture* f = body->GetFixtureList(); f;) {
            b2Fixture* next = f->GetNext();
            body->DestroyFixture(f);
            f = next;
        }
        UpdateTransform();
        if (solid <= 0 || segmentHeight <= 0.0f) return; // nothing solid yet -- no fixture, matches "fully retracted" having no collision

        b2PolygonShape shape;
        shape.SetAsBox(segmentWidth * (float)solid * 0.5f, segmentHeight * 0.5f);
        b2FixtureDef fd;
        fd.shape = &shape;
        fd.density = 1.0f;
        fd.filter.categoryBits = (uint16)collisionCategory;
        fd.filter.maskBits = (uint16)(categoriesToCollideWith != 0 ? categoriesToCollideWith : 0xFFFF);
        body->CreateFixture(&fd);
    }

    ~TelescopeData() {
        if (body) GetPhysicsWorld().DestroyBody(body);
    }
};
gmType g_telescopeType = -1;
TelescopeData* ThisTelescope(gmThread* a_thread) { return (TelescopeData*)a_thread->ThisUserCheckType(g_telescopeType); }

int GM_CDECL gmf_telescope_setSegmentWidth(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    t->segmentWidth = v;
    return GM_OK;
}
int GM_CDECL gmf_telescope_setSegmentHeight(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    t->segmentHeight = v;
    return GM_OK;
}
int GM_CDECL gmf_telescope_setNumSegments(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    t->numSegments = v;
    return GM_OK;
}
int GM_CDECL gmf_telescope_getNumSegments(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread);
    a_thread->PushInt(t ? t->numSegments : 0);
    return GM_OK;
}
int GM_CDECL gmf_telescope_setNumberOfSolidSegments(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    t->numberOfSolidSegments = v;
    t->numberOfSolidSegmentsExplicit = true;
    t->RebuildShapeIfNeeded();
    return GM_OK;
}
// getNumberOfSolidSegments() -- the real decompiled body computes this
// dynamically (iterating a per-segment list, counting how many are
// currently "solid"), not a plain stored field -- unconfirmable exactly
// without a real segment/physics model (see this section's own header
// comment). Real call site LaserTower.gm:1098 reads this purely for a
// RENDERING loop bound (`numSegmentsToRender=this.telescope.
// getNumberOfSolidSegments()+1`) and never calls the setter itself,
// relying entirely on the native computed value -- approximated here as
// `numSegments * extensionPerc` (how far the telescope has extended is
// exactly what "how many segments are solid so far" tracks in every real
// caller, since setExtensionPerc/setNumberOfSolidSegments are always
// driven by the same extension-animation logic in these entities),
// clamped to [0, numSegments]. Explicitly NOT a confirmed retail formula --
// an inferred stand-in for a real per-segment computation this
// reimplementation doesn't model, clearly worse than fabricating a
// precise-looking wrong number would be to hide.
int GM_CDECL gmf_telescope_getNumberOfSolidSegments(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread);
    a_thread->PushInt(t ? t->ComputeSolidSegments() : 0);
    return GM_OK;
}
// setExtensionPerc(perc) -- confirmed real (EntityBase.c 0x001c8730, a
// plain float setter forwarded to FUN_001c145c). Real, live call sites
// (SolidDoor.gm/Stomper.gm/LaserTower.gm's own update() logic) drive the
// door/laser's visible extension animation every frame -- now also drives
// the real collision fixture's length via RebuildShapeIfNeeded.
int GM_CDECL gmf_telescope_setExtensionPerc(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    t->extensionPerc = v;
    t->RebuildShapeIfNeeded();
    return GM_OK;
}
int GM_CDECL gmf_telescope_setCollisionCategory(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    t->collisionCategory = v;
    t->UpdateFilter();
    return GM_OK;
}
int GM_CDECL gmf_telescope_setCategoriesToCollideWith(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    t->categoriesToCollideWith = v;
    t->UpdateFilter();
    return GM_OK;
}
// setPosition(vector2D) -- confirmed real (EntityBase.c 0x001c89f4). Also
// repositions the real Box2D body if one already exists (e.g.
// ThePeopleKettleScream.gm calls this every frame from update(), well
// after setupPhysics/getBody already created it).
int GM_CDECL gmf_telescope_setPosition(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    GM_CHECK_NUM_PARAMS(1);
    GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), t->posX, t->posY);
    t->UpdateTransform();
    return GM_OK;
}
int GM_CDECL gmf_telescope_getPos(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), t ? t->posX : 0.0f, t ? t->posY : 0.0f));
    return GM_OK;
}
// distance(otherPos) -- plain geometric helper (not itself a decompiled
// EntityBase.c function -- PhysicsBody's own real `distance` isn't in this
// file's scope either) added only because getBody() returns this same
// object (see this section's header comment) and LaserTower.gm:985 chains
// `.getBody().getPos().distance(this.sensorBody.getPos())` -- a plain
// Euclidean distance against whatever vector2D is passed, no different
// from calling it on a real PhysicsBody's own getPos() result.
int GM_CDECL gmf_telescope_distance(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread);
    GM_CHECK_NUM_PARAMS(1);
    float ox = 0, oy = 0;
    GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), ox, oy);
    float dx = (t ? t->posX : 0.0f) - ox, dy = (t ? t->posY : 0.0f) - oy;
    a_thread->PushFloat(std::sqrt(dx * dx + dy * dy));
    return GM_OK;
}
// setupPhysics(pos,dir[,extra]) -- confirmed real call (EntityBase.c
// 0x001c8408), 2-arg (pos,dir) or 4-arg overload per its own decompiled
// arg-count dispatch, but the BODY of that dispatch is unconfirmed (see
// this section's header comment) -- every real corpus call site uses the
// 2-arg form (LaserDoor.gm/SolidDoor.gm/Stomper.gm/LaserTower.gm/
// ThePeopleKettleScream.gm all call `this.telescope.setupPhysics(pos,dir)`,
// none use a 3rd/4th arg), so only that shape is handled: creates the real
// Box2D body at `pos`, tracks `dir` as the extension axis, and builds the
// initial fixture (there IS always a currently-solid segment count by this
// point in every real caller -- extensionPerc/numberOfSolidSegments are
// set either just before or just after this call in the same constructor).
int GM_CDECL gmf_telescope_setupPhysics(gmThread* a_thread) {
    TelescopeData* t = ThisTelescope(a_thread); if (!t) return GM_OK;
    gmMachine& m = *a_thread->GetMachine();
    if (a_thread->GetNumParams() >= 1) GetVector2D(m, a_thread->Param(0), t->posX, t->posY);
    if (a_thread->GetNumParams() >= 2) GetVector2D(m, a_thread->Param(1), t->dirX, t->dirY);
    t->RebuildShapeIfNeeded();
    return GM_OK;
}
// getBody() -- see this section's header comment for why this returns the
// SAME telescope object rather than a real PhysicsBody.
int GM_CDECL gmf_telescope_getBody(gmThread* a_thread) {
    const gmVariable* self = a_thread->GetThis();
    a_thread->Push(self ? *self : gmVariable::s_null);
    return GM_OK;
}
// saveState()/loadState() -- confirmed real, and confirmed GENUINE no-ops
// even in retail (EntityBase.c 0x001c8698/0x001c86a0: both decompile to a
// bare `return 0;`, no body at all) -- not an approximation, this IS the
// real behavior.
int GM_CDECL gmf_telescope_saveState(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_telescope_loadState(gmThread* a_thread) { (void)a_thread; return GM_OK; }

gmFunctionEntry s_telescopeLib[] = {
    {"setSegmentWidth", gmf_telescope_setSegmentWidth, nullptr},
    {"setSegmentHeight", gmf_telescope_setSegmentHeight, nullptr},
    {"setNumSegments", gmf_telescope_setNumSegments, nullptr},
    {"getNumSegments", gmf_telescope_getNumSegments, nullptr},
    {"setNumberOfSolidSegments", gmf_telescope_setNumberOfSolidSegments, nullptr},
    {"getNumberOfSolidSegments", gmf_telescope_getNumberOfSolidSegments, nullptr},
    {"setExtensionPerc", gmf_telescope_setExtensionPerc, nullptr},
    {"setCollisionCategory", gmf_telescope_setCollisionCategory, nullptr},
    {"setCategoriesToCollideWith", gmf_telescope_setCategoriesToCollideWith, nullptr},
    {"setPosition", gmf_telescope_setPosition, nullptr},
    {"getPos", gmf_telescope_getPos, nullptr},
    {"distance", gmf_telescope_distance, nullptr},
    {"setupPhysics", gmf_telescope_setupPhysics, nullptr},
    {"getBody", gmf_telescope_getBody, nullptr},
    {"saveState", gmf_telescope_saveState, nullptr},
    {"loadState", gmf_telescope_loadState, nullptr},
};
void GM_CDECL TelescopeGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (TelescopeData*)a_object->m_user;
}
// addTelescope() -- confirmed real (EntityBase.c 0x001c6bb0): allocates and
// returns a new telescope object; real call sites always keep exactly one
// per entity (`this.telescope=this.addTelescope();`), so no per-entity
// list needs tracking here beyond the object itself.
int GM_CDECL gmf_entity_addTelescope(gmThread* a_thread) {
    a_thread->PushNewUser(new TelescopeData(), g_telescopeType);
    return GM_OK;
}

// ---------------------------------------------------------------------
// EntityManager -- confirmed real (284 call sites across the corpus, e.g.
// Platformer/MainMenu5PS3/MainMenu5PS3.gm's constructor:
// EntityManager.c exists to decompile against -- no proper entity/physics
// simulation exists in this engine yet either (out of scope for this
// pass), so this is a lightweight but functionally real registry: real
// entities are plain GM tables (so arbitrary script-side property
// get/set on them just works, no native type needed) tracked by a
// C++-side id->table map for the query methods. Held outside the VM's
// normal GC roots like ProcessManager::Process::table -- rooted the same
// way (see process_manager.h's RootGmVariable/UnrootGmVariable, added
// after the exact same gap silently emptied process tables, see
// ---------------------------------------------------------------------

struct EntityRegistry {
    std::vector<std::pair<int, gmVariable>> entities; // insertion order, id -> table
    int nextId = 1;
};
EntityRegistry& Entities() {
    static EntityRegistry reg;
    return reg;
}

gmVariable* FindEntity(int a_uid) {
    for (auto& e : Entities().entities) if (e.first == a_uid) return &e.second;
    return nullptr;
}

// Fallback lookup by the entity's OWN script-visible `__uid` field (see
// gmf_entity_setUIDData's header comment: setUIDData sets `__uid` to the
// LEVEL-AUTHORED uid, e.g. `this.setUIDData(this.configureData.uid);`,
// which is a DIFFERENT number from this registry's own creation-order key
// -- and registry re-keying itself stays disabled there, since a previous
// attempt surfaced a worse native crash). Real call sites depend on
// EntityManager.getEntityFromUID(uid)/ENTITYBASE_GetEntityByUID(uid)
// finding an entity by that LEVEL uid, not the creation-order one --
// confirmed a real, reproducible gap: TouchEditProcessBase.gm's
// addToSelected() stores `e.getUID()` (the level uid) into
// selectedEntities, and TouchEditProcessPS3.gm's convertSelectedEntitiesToStamp()
// looks it back up via ENTITYBASE_GetEntityByUID() immediately after
// placing any entity from the editor's palette -- with only the creation-
// order lookup, that always missed, returning null, and `stampEntity.
// isStamp` on the null result was a hard "getdot failed" that aborted the
// entire keypress handler every time (confirmed via live repro + reading
// the exact crash callstack). This is purely an ADDITIVE fallback search
// (no registry mutation, unlike the disabled re-keying path), so it
// carries none of that previous attempt's crash risk.
gmVariable* FindEntityByScriptUID(gmMachine& a_machine, int a_uid) {
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)a_machine.GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        gmVariable uidVar = t->Get(&a_machine, "__uid");
        if (uidVar.m_type == GM_INT && uidVar.m_value.m_int == a_uid) return &e.second;
    }
    return nullptr;
}

// --- entity handle ("e" in Platformer_Player=function(e){...}) ----------
// Mirrors ProcessManager's CreateProcessHandle/InjectProcessHandleMethods
// pattern exactly: a plain table carrying just enough native methods for
// the factory function to build the real entity table around. The one
// method that MUST be real (not null-call-leniency-forgiven) is
// getPhysicsData() -- every real factory calls it unconditionally at
// construction time (Platformer/Player.gm:12:
// `PhysicsData=e.getPhysicsData(),`) and immediately calls real methods on
// its result (`this.PhysicsData.getBody(this.PhysicsData.createBody())`),
// so a null/stub return would crash the factory before it ever finishes.
// Also injected onto the FACTORY'S OWN RETURNED table (not just the raw
// handle `e`) -- confirmed necessary the same way process methods needed
// dual injection (InjectProcessHandleMethods, process_handle_bindings.cpp):
// EntityBase.gm's `ENTITYBASE_calculateAndRecordScreenExtents` calls
// `me.getPhysicsData()` on `me`, which is always the returned entity
// table (e.g. `this.PhysicsData=e.getPhysicsData()`'s `this`), not the
// handle -- and no script anywhere defines its own `getPhysicsData`
// wrapper, so this has to resolve natively on both. Checks the plain
// `PhysicsData` field first (what real factories assign the handle's
// result to, e.g. Platformer/Player.gm:12) so a call on the returned
// table returns the SAME object the factory stored, rather than lazily
// creating a second, disconnected PhysicsData.
int GM_CDECL gmf_entity_getPhysicsData(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable field = self->Get(&m, "PhysicsData");
    if (field.m_type != GM_NULL) { a_thread->Push(field); return GM_OK; }
    gmVariable existing = self->Get(&m, "__physicsData");
    if (existing.m_type != GM_NULL) { a_thread->Push(existing); return GM_OK; }
    // Pass `self` as the owner entity so PhysicsData.getCollidingEntities
    // can resolve a real Box2D contact back to the entity table on the
    // other side (see physics_body_bindings.cpp).
    gmVariable pd = MakePhysicsData(m, gmVariable(self));
    self->Set(&m, "__physicsData", pd);
    a_thread->Push(pd);
    return GM_OK;
}
// Delegate calls (die/isWizard/isInvincible) -- confirmed real: Main.gm's
// KillPlayer handler (SYSTEMHELPER_NotifyEvent("KillPlayer", this.Entity,
// who, ...), checkDeath()/Player.gm) receives `this.Entity` -- the raw
// native entity handle -- as its "player" argument, then calls
// player.isInvincible()/isWizard()/die(killDir) directly on it. In real
// retail this works because Entity and the full Player behavior object are
// the same underlying native object; this engine splits them into two
// separate tables (the bare handle vs. the big `Platformer_Player=function
// (e){return {...}}` table the factory returns), so those three methods
// don't exist on the handle at all. Calling them there previously hit
// null-call leniency (undefined method -> silent no-op) instead of a hard
// exception, which is why this went undetected for so long: checkDeath()
// ran to completion with no visible error, `player.die(killDir)` silently
// did nothing, and `this.dead` simply never became true -- touching a
// DANGER/FIRE ground looked and felt like nothing happened at all, with no
// crash to point at the cause. Fixed by having the handle delegate these
// three specific calls to whichever full behavior table CreateEntityFromFactory
// (below) links back via `__behaviorOwner`, once that table exists.
int GM_CDECL gmf_entity_delegate_die(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable owner = self->Get(&m, "__behaviorOwner");
    if (owner.m_type != GM_TABLE) return GM_OK;
    gmTableObject* ownerTbl = (gmTableObject*)m.GetObject(owner.m_value.m_ref);
    if (!ownerTbl || ownerTbl->Get(&m, "die").m_type != GM_FUNCTION) return GM_OK;
    gmCall call;
    if (call.BeginTableFunction(&m, "die", ownerTbl, owner)) {
        if (a_thread->GetNumParams() >= 1) call.AddParam(a_thread->Param(0));
        call.End();
    }
    return GM_OK;
}
int GM_CDECL gmf_entity_delegate_isWizard(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    gmMachine& m = *a_thread->GetMachine();
    if (self) {
        gmVariable owner = self->Get(&m, "__behaviorOwner");
        gmTableObject* ownerTbl = (owner.m_type == GM_TABLE) ? (gmTableObject*)m.GetObject(owner.m_value.m_ref) : nullptr;
        if (ownerTbl && ownerTbl->Get(&m, "isWizard").m_type == GM_FUNCTION) {
            gmCall call;
            if (call.BeginTableFunction(&m, "isWizard", ownerTbl, owner)) {
                call.End();
                a_thread->Push(call.GetReturnedVariable());
                return GM_OK;
            }
        }
    }
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_entity_delegate_isInvincible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    gmMachine& m = *a_thread->GetMachine();
    if (self) {
        gmVariable owner = self->Get(&m, "__behaviorOwner");
        gmTableObject* ownerTbl = (owner.m_type == GM_TABLE) ? (gmTableObject*)m.GetObject(owner.m_value.m_ref) : nullptr;
        if (ownerTbl && ownerTbl->Get(&m, "isInvincible").m_type == GM_FUNCTION) {
            gmCall call;
            if (call.BeginTableFunction(&m, "isInvincible", ownerTbl, owner)) {
                call.End();
                a_thread->Push(call.GetReturnedVariable());
                return GM_OK;
            }
        }
    }
    a_thread->PushInt(0);
    return GM_OK;
}

// Generic version of the die/isWizard/isInvincible delegate pattern above --
// same root cause (script "Install" mixins like GroundBase.gm's
// Platformer_GroundBase_Install(t) attach methods like getStyle/isGround
// directly to the returned behavior table `t`, but real call sites
// (GroundBase.gm's own destructor(): `this.Entity.getStyle()`) call them on
// `this.Entity`, the bare native handle -- valid in real retail where the
// native entity object and its script behavior table are the same
// underlying object, invalid here where this engine splits them). Reused
// for any bare-handle method with no dedicated native implementation that
// should instead forward to whatever real script method the behavior
// table defines.
int GM_CDECL DelegateToBehaviorOwner(gmThread* a_thread, const char* a_methodName) {
    gmTableObject* self = a_thread->ThisTable();
    gmMachine& m = *a_thread->GetMachine();
    if (self) {
        gmVariable owner = self->Get(&m, "__behaviorOwner");
        gmTableObject* ownerTbl = (owner.m_type == GM_TABLE) ? (gmTableObject*)m.GetObject(owner.m_value.m_ref) : nullptr;
        if (ownerTbl && ownerTbl->Get(&m, a_methodName).m_type == GM_FUNCTION) {
            gmCall call;
            if (call.BeginTableFunction(&m, a_methodName, ownerTbl, owner)) {
                for (int i = 0; i < a_thread->GetNumParams(); ++i) call.AddParam(a_thread->Param(i));
                call.End();
                a_thread->Push(call.GetReturnedVariable());
                return GM_OK;
            }
        }
    }
    a_thread->PushNull();
    return GM_OK;
}
// tagForDeletion() -- confirmed real (Ghidra: retail FUN_0043dae0 @
// 0x0043dae0, a bare no-arg vtable dispatch). Real call sites: Player.gm:99,
// Point.gm:343/392, Main.gm:471, Thing.gm:324/381/523/535 -- every one of
// these is an entity ending its own lifetime (player death, point
// collected/expired, wall/enemy killed). Was entirely unbound (silent
// no-op), so EVERY entity in PekkoAndJon was permanently un-removable --
// points would never disappear once collected, dead enemies would stay
// forever. Removes this entity from the EntityRegistry immediately (same
// "additions/removals already apply immediately" contract as
// gmf_EM_flush/deleteAllEntities above) and unroots it so the GC can
// reclaim it once no script reference remains.
int GM_CDECL gmf_entity_tagForDeletion(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    // self is the bare handle; the tracked registry entry is the RETURNED
    // behavior table (__behaviorOwner), same object CreateEntityFromFactory
    // pushed into Entities().entities -- match by reference either way so
    // this works whether called on the handle or directly on that table.
    gmVariable owner = self->Get(&m, "__behaviorOwner");
    int ownerRef = (owner.m_type == GM_TABLE) ? owner.m_value.m_ref : -1;
    auto& list = Entities().entities;
    for (size_t i = 0; i < list.size(); ++i) {
        gmVariable& v = list[i].second;
        if (v.m_type == GM_TABLE && (v.m_value.m_ref == ownerRef ||
            (gmTableObject*)m.GetObject(v.m_value.m_ref) == self)) {
            UnrootGmVariable(m, v);
            list.erase(list.begin() + (long)i);
            break;
        }
    }
    return GM_OK;
}
// isType(typeNameOrId) -- confirmed real (Ghidra: retail FUN_0043e230 @
// 0x0043e230), dual-mode: an int param (type=1) compares against the
// entity's own interned type id directly (real caller Thing.gm:570:
// `arg0.isType(this.Entity.getType())`, i.e. ENTITY_GetType's own return
// value fed straight back in); a string param (type=3) compares by name
// (Point.gm:390/Player.gm:36: `arg0.isType("PekkoAndJon_Player")`). Backed
// by the exact same __typeId/InternTypeId registry gmf_entity_getType/
// gmf_ENTITY_GetType already use, so both call conventions agree. Was
// entirely unbound -- every PekkoAndJon type-dispatch check silently
// null-called to null (leniency), which is falsy, so e.g. Point.gm's own
// "did the player pick me up" check could never fire.
int GM_CDECL gmf_entity_isType(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    gmVariable arg = a_thread->Param(0);
    gmVariable typeIdVar = self->Get(&m, "__typeId");
    int myTypeId = (typeIdVar.m_type == GM_INT) ? typeIdVar.m_value.m_int : 0;
    bool result = false;
    if (arg.m_type == GM_INT) {
        result = (arg.m_value.m_int == myTypeId);
    } else if (arg.m_type == GM_STRING) {
        char buf[256];
        const char* s = arg.AsString(&m, buf, sizeof(buf));
        result = s && (InternTypeId(s) == myTypeId);
    }
    a_thread->PushInt(result ? 1 : 0);
    return GM_OK;
}
// 0x0009b59c): compares THIS entity's own __typeId-shaped field against
// the ARGUMENT entity's, both read off the same offset -- i.e. a plain
// `this.getType()==other.getType()` shortcut, null-safe on a non-entity/
// null argument (the decompiled body reads a null pointer's own field as
// 0, never matching a real type id). Real, live call site (Main.gm:10092,
// the level-load duplicate-entity-detector's `if (e.isSameType(ee))`
// pairwise scan) -- was entirely unbound, so this comparison always fell
// back to null-call leniency (silently false) instead of ever finding a
// real duplicate pair, on top of the sibling getAllEntities() bug (see
// PushEntityListIterator's header comment) that made the surrounding loop
// not even run at all until that fix.
int GM_CDECL gmf_entity_isSameType(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    gmVariable arg = a_thread->Param(0);
    bool result = false;
    if (arg.m_type == GM_TABLE) {
        gmTableObject* other = (gmTableObject*)m.GetObject(arg.m_value.m_ref);
        gmVariable myType = self->Get(&m, "__typeId");
        gmVariable otherType = other ? other->Get(&m, "__typeId") : gmVariable::s_null;
        result = (myType.m_type == GM_INT && otherType.m_type == GM_INT &&
                  myType.m_value.m_int == otherType.m_value.m_int);
    }
    a_thread->PushInt(result ? 1 : 0);
    return GM_OK;
}
// callMethod(name, ...args) -- confirmed real (Ghidra: retail FUN_0043e1f0
// @ 0x0043e1f0 on the Entity table, FUN_0043f620 on the Particle table --
// same generic "call a script method on self by name" shape, an internal
// string-dispatch helper). Real call sites span entity handles
// (`this.Entity.callMethod("isDraggable")`, Main.gm:35), plain entity
// tables (`arg0.callMethod("hurt",this.Entity)`, Thing.gm:576), and process
// handles (`GameSystem.getProcess("PekkoAndJon_Main").callMethod(
// "addScore",1)`, Point.gm:393) -- i.e. this needs to work on whatever
// table it's called on, forwarding to the SCRIPT method of that name.
// Mirrors DelegateToBehaviorOwner's fallback (bare Entity handles split
// method/state across __behaviorOwner in this engine, unlike retail) but
// tries `self` FIRST so it also works unchanged on process handles (which
// dual-inject onto the same table, see process_handle_bindings.cpp's
// header comment on why they DON'T have a __behaviorOwner) and plain
// script tables that already define the method directly.
int GM_CDECL gmf_callMethodByName(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    GM_STRING_PARAM(name, 0, "");

    gmTableObject* target = self;
    gmVariable targetVar = gmVariable(self);
    if (self->Get(&m, name).m_type != GM_FUNCTION) {
        gmVariable owner = self->Get(&m, "__behaviorOwner");
        if (owner.m_type == GM_TABLE) {
            gmTableObject* ownerTbl = (gmTableObject*)m.GetObject(owner.m_value.m_ref);
            if (ownerTbl && ownerTbl->Get(&m, name).m_type == GM_FUNCTION) {
                target = ownerTbl;
                targetVar = owner;
            }
        }
    }
    gmCall call;
    if (call.BeginTableFunction(&m, name, target, targetVar)) {
        for (int i = 1; i < a_thread->GetNumParams(); ++i) call.AddParam(a_thread->Param(i));
        call.End();
        a_thread->Push(call.GetReturnedVariable());
    } else {
        a_thread->PushNull();
    }
    return GM_OK;
}
int GM_CDECL gmf_entity_delegate_getStyle(gmThread* a_thread) { return DelegateToBehaviorOwner(a_thread, "getStyle"); }
int GM_CDECL gmf_entity_delegate_isGround(gmThread* a_thread) { return DelegateToBehaviorOwner(a_thread, "isGround"); }
int GM_CDECL gmf_entity_delegate_onDisable(gmThread* a_thread) { return DelegateToBehaviorOwner(a_thread, "onDisable"); }
int GM_CDECL gmf_entity_delegate_onEnable(gmThread* a_thread) { return DelegateToBehaviorOwner(a_thread, "onEnable"); }

// getMainBody() -- entity-handle-level convenience, confirmed real via
// Main.gm's KillPlayer handler (~line 10977): `local ppb=player.
// getMainBody();`, called on the entity handle passed as SYSTEMHELPER_
// NotifyEvent's `a` arg (Player.gm checkDeath(): `this.Entity` -- the raw
// handle, not the full Player behavior table that defines its OWN
// getMainBody() at Player.gm:5153 as `return this.mainpb;`). Entity
// handles never had any getMainBody at all -- a hard "getdot failed"
// calling it, aborting the entire KillPlayer handler (so even once
// checkDeath() correctly fired the event, dying still silently did
// nothing). Player.gm's own getMainBody() leaves an equivalent commented
// out right next to the real one -- `this.PhysicsData.getBody(0)` -- so
// that's exactly what this does, generically, for any entity handle.
int GM_CDECL gmf_entity_getMainBody(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable pd = self->Get(&m, "PhysicsData");
    if (pd.m_type == GM_NULL) pd = self->Get(&m, "__physicsData");
    a_thread->Push(GetPhysicsDataMainBodyVar(m, pd));
    return GM_OK;
}
// Shared position lookup for the getVectorTo/getDistanceTo/-Normalized
// family below -- same PhysicsData-AABB-centre-with-configureData-point-
// fallback approach gmf_entity_isInView/isInGameBoard/EntityOverlapsBox
// (further down this file) already use, so an entity with no physics body
// (a spawn point, a trigger) still participates as a real point.
bool GetEntityWorldPos(gmMachine& a_machine, gmTableObject* a_entity, float& a_outX, float& a_outY) {
    if (!a_entity) return false;
    float minX, minY, maxX, maxY;
    gmVariable pdVar = a_entity->Get(&a_machine, "PhysicsData");
    if (pdVar.m_type == GM_NULL) pdVar = a_entity->Get(&a_machine, "__physicsData");
    if (GetPhysicsDataAABB(a_machine, pdVar, minX, minY, maxX, maxY)) {
        a_outX = (minX + maxX) * 0.5f;
        a_outY = (minY + maxY) * 0.5f;
        return true;
    }
    gmVariable cd = a_entity->Get(&a_machine, "configureData");
    if (cd.m_type != GM_TABLE) return false;
    gmTableObject* cdt = (gmTableObject*)a_machine.GetObject(cd.m_value.m_ref);
    gmVariable xv = cdt->Get(&a_machine, "posX"), yv = cdt->Get(&a_machine, "posY");
    if (xv.m_type != GM_FLOAT && xv.m_type != GM_INT) return false;
    auto tofloat = [](const gmVariable& v) -> float {
        return v.m_type == GM_FLOAT ? v.m_value.m_float : (float)v.m_value.m_int;
    };
    a_outX = tofloat(xv);
    a_outY = tofloat(yv);
    return true;
}
// 0x0009ba1c: reads self and `other`'s own native position fields, calls
// FUN_000c0c40, returns a vector2D). Real, live call sites (EntityBase.gm:
// 1897/5896's death/knockback logic: `dirToFling=killerThing.
// getVectorToNormalized(player);`, feeding the direction the player flies
// off in when killed) -- was entirely unbound, so `dirToFling` silently
// stayed null every time a kill didn't already supply an explicit fling
// direction, on top of aborting checkDeath()'s caller outright under
// GE3_STRICT=halt. Implemented as a real self->other normalized direction
// using the same position source getMainBody/isInView/isInGameBoard
// already established for this file's entity handles.
int GM_CDECL gmf_entity_getVectorToNormalized(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    gmVariable otherVar = a_thread->Param(0);
    gmTableObject* other = (otherVar.m_type == GM_TABLE) ? (gmTableObject*)m.GetObject(otherVar.m_value.m_ref) : nullptr;
    float sx = 0, sy = 0, ox = 0, oy = 0;
    GetEntityWorldPos(m, self, sx, sy);
    GetEntityWorldPos(m, other, ox, oy);
    float dx = ox - sx, dy = oy - sy;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len > 0.0001f) { dx /= len; dy /= len; } else { dx = 0.0f; dy = 0.0f; }
    a_thread->Push(MakeVector2D(m, dx, dy));
    return GM_OK;
}
// setIsAutoRemove/setIsAutoRemoveStranded/setIsInvisible: confirmed real
// by usage only (Player.gm: `this.Entity.setIsAutoRemove(false);` etc, on
// the handle rather than the returned table) -- no reveng evidence for
// what they actually do downstream (no auto-remove/culling subsystem
// exists yet), so honest no-ops rather than fabricated behavior.
int GM_CDECL gmf_entity_noop(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_entity_getUID(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    a_thread->Push(self->Get(a_thread->GetMachine(), "__uid"));
    return GM_OK;
}
// `setUIDData`, address 0x001c6ad4): re-registers "the C++ portion"'s
// uid->entity map entry, unregistering under the OLD uid first if one was
// already set. EntityBase.gm's own configure() wrapper calls this
// unconditionally (`this.setUIDData(this.configureData.uid);`) with the
// LEVEL-AUTHORED uid (cd.uid), which is NOT the same value as this
// engine's own auto-incrementing creation-order id (EntityManager's
// `nextId`) -- was entirely missing, meaning
// `EntityManager.getEntityFromUID(cd.uid)` (aka
// `ENTITYBASE_GetEntityByUID`) could never find an already-configured
// entity on a later pass, so Platformer/Main.gm's configureEntities()
// always took the "create brand new entity" branch instead of "reuse and
// reset() the existing one", endlessly duplicating entities every time
// the incremental level-load loop retried after any later exception.
// Re-keys THIS table's own entry in the shared entity registry (see
// Entities()/gmf_EM_setEntityUID above, same re-key-by-reference approach)
// under the new uid, and updates `__uid` to match so
// getObjectID()/getUID() stay consistent with it.
// NOTE: re-keying is currently disabled (see gmf_entity_setUIDData's
// e.reset()" path this unlocks surfaced a *worse* crash (some reused
// entities reaching reset()->configure() with a still-null
// configureData, plus a native-side segfault) than the duplicate-entity
// growth it fixes. Kept as a real, decompiled-confirmed no-op stand-in
// (updates __uid only) rather than removed outright, since the
// registration itself -- and `this.setUIDData(...)` no longer silently
// vanishing via null-call leniency -- is still correct; only the
// registry re-keying that would change *lookup* behavior is disabled
// until the configureData gap it exposes is root-caused.
int GM_CDECL gmf_entity_setUIDData(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(newUidF, 0, 0.0f);
    int newUid = (int)newUidF;
    self->Set(a_thread->GetMachine(), "__uid", gmVariable((gmint)newUid));
    return GM_OK;
}
// getMinCX/getMaxCX/getMinCY/getMaxCY -- confirmed real (EntityBase.gm's
// screen-registration code calls all four directly after
// ENTITYBASE_calculateAndRecordScreenExtents(this), which is SUPPOSED to
// have just set them as a side effect). ENTITYBASE_calculateAndRecordScreenExtents's
// own body (EntityBase.gm ~line 430) does end after computing a local
// `aabb` with no visible assignment -- BUT a few lines further down (~952),
// still inside a commented-out `C++` reference block, retail's own
// equivalent is spelled out verbatim:
//   this.minCX=this.gameProcess.getScreenX(aabb.minx);
//   this.maxCX=this.gameProcess.getScreenX(aabb.maxx);
//   this.minCY=this.gameProcess.getScreenY(aabb.miny);
//   this.maxCY=this.gameProcess.getScreenY(aabb.maxy);
// i.e. these are the entity's AABB corners converted to screen-grid cell
// indices via gameProcess.getScreenX/Y -- both already real, confirmed
// functions (Platformer/Main.gm ~5873: floor-divide by
// getScreenWidth()/getScreenHeight(), which are themselves the confirmed
// 16*SW*4 / 9*SW*4 grid constants, see process_handle_bindings.cpp).
// Implemented directly (not by calling back into GM) using
// GetPhysicsDataAABB (physics_body_bindings.h) plus the same floor-divide
// formula getScreenX/Y use.
namespace {
constexpr float kScreenCellWidth = 16.0f * 0.61f * 2.0f * 2.0f;
constexpr float kScreenCellHeight = 9.0f * 0.61f * 2.0f * 2.0f;
int WorldToScreenCell(float a_pos, float a_cellSize) {
    int c = (int)(a_pos / a_cellSize);
    if (a_pos < 0.0f) c -= 1;
    return c;
}
}
int GM_CDECL gmf_entity_getMinCX(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    float minX, minY, maxX, maxY;
    GetPhysicsDataAABB(m, self->Get(&m, "PhysicsData"), minX, minY, maxX, maxY);
    a_thread->PushInt(WorldToScreenCell(minX, kScreenCellWidth));
    return GM_OK;
}
int GM_CDECL gmf_entity_getMaxCX(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    float minX, minY, maxX, maxY;
    GetPhysicsDataAABB(m, self->Get(&m, "PhysicsData"), minX, minY, maxX, maxY);
    a_thread->PushInt(WorldToScreenCell(maxX, kScreenCellWidth));
    return GM_OK;
}
int GM_CDECL gmf_entity_getMinCY(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    float minX, minY, maxX, maxY;
    GetPhysicsDataAABB(m, self->Get(&m, "PhysicsData"), minX, minY, maxX, maxY);
    a_thread->PushInt(WorldToScreenCell(minY, kScreenCellHeight));
    return GM_OK;
}
int GM_CDECL gmf_entity_getMaxCY(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    float minX, minY, maxX, maxY;
    GetPhysicsDataAABB(m, self->Get(&m, "PhysicsData"), minX, minY, maxX, maxY);
    a_thread->PushInt(WorldToScreenCell(maxY, kScreenCellHeight));
    return GM_OK;
}
// getDescriptionBits/setDescriptionBits -- confirmed real (a per-entity
// bitmask read/written via plain `&`/`|`/`^` against
// GROUND_ENTITY_DESCRIPTION/SELECTED_ENTITY_DESCRIPTION, e.g.
// Platformer/GroundBase.gm's `endConfigure` calling
// `this.Entity.getDescriptionBits()` unconditionally on every ground
// entity -- was entirely missing, throwing "operator band undefined for
// type null and int" and aborting configureEntities() before it could
// ever advance `progress.startIndex`/`.done`, silently reprocessing (and
// re-creating) the same entities forever). Stored as a plain `__descBits`
// int field on the handle, defaulting to 0 (no flags set).
int GM_CDECL gmf_entity_getDescriptionBits(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__descBits");
    a_thread->PushInt(v.m_type == GM_INT ? v.m_value.m_int : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_setDescriptionBits(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(bits, 0, 0);
    self->Set(a_thread->GetMachine(), "__descBits", gmVariable((gmint)bits));
    return GM_OK;
}

// setIsEnabled/isEnabled -- entirely unregistered, confirmed real and
// heavily used (8 call sites in EntityBase.gm alone, e.g. PlayerDeathBits.gm
// death-particle-bit entities: `e.setIsEnabled(true);` right after
// EntityManager.createEntity(...) in Main.gm's real, live KillPlayer
// handler) -- a hard "getdot failed" calling it on any entity handle,
// aborting the whole caller. For PlayerDeathBits specifically this meant
// the KillPlayer handler crashed immediately after creating each
// death-particle-bit entity, before ever setting its position/velocity/
// colour, so dying never visibly showed the real flying-bits effect.
// Stored the same way setIsInvisible/isInvisible is (a plain flag field) --
// update/draw dispatch isn't gated on it here, no real evidence of what a
// "disabled" entity should visibly do beyond not crashing when touched.
// ENGINE PATCH (2026-08-16): confirmed via a direct C++-level trace (every
// native call whose C function returns GM_EXCEPTION, resolved back to a
// symbol via /proc/<pid>/maps + addr2line) that THIS function -- not
// anything else in the registerEntity/configureEntities chain -- is what's
// been silently aborting Main.gm's incremental level-load loop every single
// frame, forever (confirmed via a separate trace that configureProgress.
// startIndex never advances past 0). Root cause: `GM_INT_PARAM(flag,0,0)`
// expands (this build's vendor gmThread.h, the strict variant) to
// `if(!ParamInt(...)) return GM_EXCEPTION;` -- and ParamInt() only succeeds
// for a LITERAL GM_INT argument, returning false (triggering the exception)
// for ANY other type, including GM_NULL. Main.gm:6942's real call site is
// `entity.setIsEnabled(onScreen)` where `onScreen=entity.
// isOnCurrentScreen()` (Main.gm:6939) -- isOnCurrentScreen isn't a real
// native binding (not in the retail symbol index) and isn't defined on
// every entity type, so it commonly null-call-leniently returns GM_NULL,
// which then blew up here. Fixed by reading the param leniently (matching
// GM_INT_PARAM's own non-strict variant, and every other bool-ish param in
// this codebase) instead of the throwing macro -- not by working around
// ThisTable(), which was never actually the problem.
// ENGINE PATCH (2026-08-17) ADDED THEN REVERTED SAME DAY: this used to also
// lazily trigger configure() when becoming enabled (guarded by
// __nativeConfiguring/__nativeConfigured). Confirmed via a live coredump
// backtrace (coredumpctl gdb) that this caused a genuine native stack
// overflow -- gmf_entity_setIsEnabled calling itself through Sys_Execute
// 58000+ frames deep before segfaulting, on Platformer/EntityPacks/
// GameStuff/PlayerCheckpoint.gm. Root cause: PlayerCheckpoint's own
// configure() chain creates/references a SEPARATE "vehicleEntity" (its own
// distinct entity, own configureData, own __nativeConfiguring flag) and
// calls setIsEnabled on IT too -- the self-recursion guard on one entity's
// table can't see or block a mutual A-configures-B-configures-A cycle
// across two DIFFERENT entities' tables. ConfigureUnconfiguredEntities
// (see its own header comment) already covers the real problem this was
// trying to solve -- a flat once-per-frame sweep, not a reentrant call
// triggered from arbitrarily deep inside another entity's own configure()
// -- so this eager trigger was redundant on top of being unsafe. Back to
// a plain flag store.
int GM_CDECL gmf_entity_setIsEnabled(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    gmint flag = a_thread->ParamInt(0, 0); // lenient: non-int (incl. null) -> default 0
    self->Set(a_thread->GetMachine(), "__enabled", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isEnabled(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(1); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "__enabled");
    a_thread->PushInt((v.m_type != GM_INT || v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}

// setIsInvisible/isInvisible -- was a no-op setter with no matching getter
// at all (InjectEntityHandleMethods only ever bound setIsInvisible, always
// to gmf_entity_noop). Confirmed real and load-bearing: Player.gm's
// FIRSTSPAWN_STATEID hides the player (`parent.Entity.setIsInvisible(true)`)
// until the spawner's animation finishes, then un-hides it
// (`setIsInvisible(false)`) right before switching to BIRTH_STATEID -- with
// the setter doing nothing, the player was visible for the entire
// spawn-in sequence instead of just appearing at the end. testDraw() (see
// Main.gm's preDraw, re-enabled alongside this) also reads
// `this.Entity.isInvisible()` as its own first-line early-out, which didn't
// exist as a callable method at all before this.
int GM_CDECL gmf_entity_setIsInvisible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    self->Set(a_thread->GetMachine(), "__invisible", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isInvisible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__invisible");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}

// 0x001c55a8/0x001c55f4: plain bool flag getter/setter). Real call site
// (EntityPacks/Wall.gm's constructor(): `this.setIsEditable(false);`)
// marks a piece of level geometry as not directly selectable/movable in
// the level editor -- same bool-flag-on-the-handle shape as setIsInvisible
// above, defaulting true (unset == editable, matching every other real
// entity that never calls this setter at all).
int GM_CDECL gmf_entity_setIsEditable(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    self->Set(a_thread->GetMachine(), "__editable", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isEditable(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__editable");
    a_thread->PushInt((v.m_type != GM_INT || v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}

// recordScreenExtents(minx,miny,maxx,maxy)/isOnCurrentScreen() -- confirmed
// converts a world-space AABB to screen-grid indices via two internal
// helpers (FUN_0019e6e8 for X, FUN_0019e760 for Y) and stores
// min/maxScreenX/Y on the native entity struct (+0x120/+0x124/+0x128/
// +0x12c); isOnCurrentScreen() compares those stored bounds against the
// current camera screen-grid position and returns whether they overlap.
// The X/Y conversion helpers match Main.gm's own script-level
// getScreenX(x)/getScreenY(y) byte-for-byte in shape (floor(coord /
// getScreenWidth()), floor(coord / getScreenHeight())) -- but
// getScreenWidth/Height depend on the level's numCellsX/numCellsY grid
// dimensions, level-load state this binding layer has no access to.
// Real consumer (Main.gm:6939: `local onScreen=entity.isOnCurrentScreen();
// entity.setIsEnabled(onScreen);`) uses this purely to DISABLE entities on
// screens the player isn't currently viewing (a perf/culling optimization
// for multi-screen levels) -- a wrong "off screen" answer here would
// wrongly disable real, visible entities, which is strictly worse than
// the perf cost of never culling. Implemented conservatively: always
// "on screen" (matches this project's existing bookkeeping-only-flags
// precedent, e.g. isBeingNice/isShouldUpdateEntities in game_system_state.h,
// prioritizing correct visible output over retail's internal perf
// optimization this reimplementation doesn't have level-grid data to
// replicate exactly).
int GM_CDECL gmf_entity_recordScreenExtents(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_entity_isOnCurrentScreen(gmThread* a_thread) {
    a_thread->PushInt(1);
    return GM_OK;
}
// classes/EntityBase.c 0x001c7f00), but the decompiled body is corrupted
// ("WARNING: Control flow encountered bad instruction data" / halt_baddata
// -- Ghidra couldn't recover it). Real call site (Main.gm:2524, loading a
// level saved with pre-recorded screen extents) sets the same min/max
// ScreenX/Y fields recordScreenExtents does -- same conservative no-op
// treatment, consistent with isOnCurrentScreen always answering true above.
int GM_CDECL gmf_entity_initializeScreenExtents(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// constructorSetNoPhysics/constructorSetNoUpdate -- confirmed real
// actions calling into an internal per-entity-struct flag setter). Real
// call site (EntityPacks/Wall.gm's constructor(), a plain non-ENTITYBASE
// entity per its own header comment "not an ENTITYBASE, therefore no C++
// component"): `this.constructorSetNoPhysics(); this.constructorSetNoUpdate();`
// tell the native entity system not to allocate a physics body / not to
// run this entity through the per-frame update pass -- this reimplementation
// never allocates either unless a script explicitly builds one via
// getPhysicsData()/PhysicsBody(...) or defines its own update(), so both
// are genuine no-ops here (nothing to suppress), consumed rather than
// left as a null-call.
// 0x001c0c9c-adjacent, a `this`-only action). Real call site (EntityBase.gm
// :692, entity re-load path: `this.PhysicsData.removeBody(-1); this.
// clearTelescopes();`) clears "telescope" note-head visual guide lines --
// this reimplementation's renderer has no such debug/guide overlay to
// clear, genuine no-op.
int GM_CDECL gmf_entity_clearTelescopes(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// isBeingEdited()/setIsBeingEdited(bool), isPlacing()/setIsPlacing(bool) --
// setter pairs). Corrected from an earlier pass that only found the
// getters and (wrongly) assumed no real setter call site existed --
// EntityBase.gm:1293/1403 (`this.setIsBeingEdited(true);`/`this.
// setIsPlacing(true);`, both in real onBeginPlace/onPlace editor-drag
// hooks) and ENTITYBASE_startEntityCollectionEdit (`me.Entity.
// setIsAlwaysProcessed(true)`, a sibling flag right next to these) all
// showed up once actual editor entity-placement was exercised. Real
// stateful bool fields on the entity handle, same shape as setIsInvisible/
// isInvisible above -- default false (unset == not currently being
// edited/placed), matching every entity that's just sitting in a loaded
// level.
int GM_CDECL gmf_entity_setIsBeingEdited(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    self->Set(a_thread->GetMachine(), "__beingEdited", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isBeingEdited(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__beingEdited");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_setIsPlacing(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    self->Set(a_thread->GetMachine(), "__placing", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isPlacing(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__placing");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}

// isAlwaysProcessed()/setIsAlwaysProcessed(bool) -- confirmed real by
// load-bearing save/restore pattern: EntityBase.gm's
// ENTITYBASE_startEntityCollectionEdit does `me.entityCollectionPreEditData
// .alwaysProcessed=me.Entity.isAlwaysProcessed(); me.Entity.
// setIsAlwaysProcessed(true);` and ENTITYBASE_endEntityCollectionEdit
// restores it afterward -- and EntityPacks/Wall.gm's constructor()
// separately calls `this.setIsAlwaysProcessed(true)` directly). Real
// stateful bool field, same shape as setIsInvisible/isInvisible --
// defaults false, matching every entity that doesn't opt in to being
// processed even when off-screen/disabled.
// 0x001c587c, plain bool flag getter). No setter found anywhere in this
// corpus -- read-only from script's perspective. Real call site
// (EntityBase.gm:799: `if(this.isStaticDraw()){Video.flushStaticSurface();}`)
// gates a static-surface-cache flush during a move -- no such cache exists
// in this reimplementation's immediate-mode renderer (see draw_list.h's
// header comment), so default false (never triggering the flush) is the
// correct, safe answer.
int GM_CDECL gmf_entity_isStaticDraw(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(0);
    return GM_OK;
}
// isSauceBase()/setIsSauceBase(bool) -- CORRECTED (2026-08-17): a previous
// pass claimed no real setter existed and left this a hardcoded-false
// no-op getter, but EntityPacks/SauceBase.gm:29's own install helper does
// `t.setIsSauceBase(true);` -- a real, live call site this session's strict
// survey caught as unbound ("first called from SauceBase.gm:29"), meaning
// every sauce-base-family entity (bushes/decorations built on SauceBase.gm)
// hard-crashed at construction. Real stateful bool field, same shape as
// isBeingEdited/isPlacing -- defaults false, matching every entity type
// that never calls the setter.
int GM_CDECL gmf_entity_isSauceBase(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "__sauceBase");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_setIsSauceBase(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    gmint flag = a_thread->ParamInt(0, 0);
    self->Set(a_thread->GetMachine(), "__sauceBase", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
// redrawSauceBase() -- confirmed real, live call site SauceBase.gm:364
// (recomputes the entity's cached physical width/height from its idle
// animation frame after a move/resize). Was entirely unbound -- hard
// "getdot failed" aborting configure()'s caller for every sauce-base
// entity once isSauceBase's own missing-setter bug (above) was fixed far
// enough for construction to actually reach this point. This
// reimplementation has no persistent draw-surface cache to invalidate (see
// isStaticDraw's header comment), so genuine no-op -- consumed rather than
// left as a null-call.
int GM_CDECL gmf_entity_redrawSauceBase(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

int GM_CDECL gmf_entity_setIsAlwaysProcessed(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    self->Set(a_thread->GetMachine(), "__alwaysProcessed", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isAlwaysProcessed(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__alwaysProcessed");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}

// getNumOffScreensAudible/setNumOffScreensAudible(n) -- confirmed real
// pair). set_getAudibilityPerc_neverOnScreen(bool) -- confirmed real
// (0x001c5bb8-adjacent, plain bool field setter). Real call sites
// (EntityBase.gm's ENTITYBASE_startEntityCollectionEdit-adjacent setup:
// `t.Entity.setNumOffScreensAudible(0); t.Entity.
// set_getAudibilityPerc_neverOnScreen(true);`; Main.gm:8051's audibility
// fade-out pass: `local numOfs=e.getNumOffScreensAudible();`) back a real
// per-entity audio-fade-out-distance setting (how many off-screen screens
// an entity's sound stays audible for). Stored as plain bookkeeping fields
// on the entity handle, same shape as setIsInvisible/isInvisible above --
// this reimplementation's audio mixer doesn't yet model distance/screen-
// based volume falloff, so nothing currently reads these back, but the
// getter/setter round-trip itself (what real callers actually depend on)
// is exact.
int GM_CDECL gmf_entity_setNumOffScreensAudible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(n, 0, 0);
    self->Set(a_thread->GetMachine(), "__numOffScreensAudible", gmVariable((gmint)n));
    return GM_OK;
}
int GM_CDECL gmf_entity_getNumOffScreensAudible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__numOffScreensAudible");
    a_thread->PushInt(v.m_type == GM_INT ? v.m_value.m_int : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_set_getAudibilityPerc_neverOnScreen(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    self->Set(a_thread->GetMachine(), "__audibilityNeverOnScreen", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
// is_getAudibilityPerc_neverOnScreen() -- the matching getter for the
// setter directly above, confirmed real (0x001c5bb8-adjacent, same
// field). Live call site Main.gm:8404: `e.checkpoint_neverOnScreen=e.
// is_getAudibilityPerc_neverOnScreen();`, part of the same note-plant
// checkpoint bookkeeping pass as isGot()/isGetting() below. Was entirely
// unbound despite its own setter already existing -- a getter/setter pair
// left half-wired.
int GM_CDECL gmf_entity_is_getAudibilityPerc_neverOnScreen(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "__audibilityNeverOnScreen");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
// setGot(bool)/isGot()/isGetting() -- confirmed real note-plant "collected"
// call sites Main.gm:10650 `e.setGot(true/false)` toggling every note
// plant's collected flag for video-capture mode, and Main.gm:8403's per-
// frame checkpoint pass `e.checkpoint_got=e.isGot()||e.isGetting();`). All
// three were entirely unbound -- hard "getdot failed" aborting both call
// chains (the video-capture toggle loop and the checkpoint bookkeeping
// pass) every time either ran. isGetting() (mid-collection-animation, as
// opposed to already-fully-got) has no separate real setter anywhere in
// this corpus -- default false, matching every entity that never enters
// that transient state in this reimplementation's collection flow.
int GM_CDECL gmf_entity_setGot(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    gmint flag = a_thread->ParamInt(0, 0);
    self->Set(a_thread->GetMachine(), "__got", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isGot(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "__got");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
// setEditSensorBox(width, height, physicsBody, offset) -- confirmed real,
// live call site SafeVine.gm:198: `this.setEditSensorBox(this.width,SW,
// this.mainpb,vector2D(0,amp/2));`. Editor-only helper (sets up the
// invisible drag-handle sensor box shown while placing/editing an entity
// in the level editor) -- was entirely unbound, hard "getdot failed"
// aborting the vine's configure() every time. This reimplementation's
// editor doesn't render per-entity edit-sensor hit boxes (selection/
// placement already works through the entity's own regular physics body,
// see this session's earlier isBeingEdited/isPlacing work), so genuine
// no-op -- consumed rather than left as a null-call.
int GM_CDECL gmf_entity_setEditSensorBox(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_entity_isGetting(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(0);
    return GM_OK;
}
// EntityBase.c 0x001c69e0/0x001c6a60: plain int field getter/setter, same
// shape as getNumOffScreensAudible above). Real, live call site
// (Main.gm's KEYBOARD_2 debug hotkey: `proc.setup(e.getTypeName(),
// e.getMixerID());`, launching the Mixer editor tool -- see Mixer.gm --
// for whichever entity the touch-edit cursor is over) -- getMixerID was
// entirely unbound, so that debug tool crashed instead of opening. Default
// -1 (unset/no mixer), matching this file's established "real stateful
// field, defaults to whatever an entity that never calls the setter would
// see" convention (e.g. isEditable's own default-true note).
int GM_CDECL gmf_entity_getMixerID(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(-1); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "__mixerID");
    a_thread->PushInt(v.m_type == GM_INT ? v.m_value.m_int : -1);
    return GM_OK;
}
int GM_CDECL gmf_entity_setMixerID(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(id, 0, -1);
    self->Set(a_thread->GetMachine(), "__mixerID", gmVariable((gmint)id));
    return GM_OK;
}
// EntityBase.c 0x001c6cf4/0x001c6d74: plain float field getter/setter).
// Not a live script definition despite appearing next to `t.setLimitVolPerc
// =function(f){this.limitVolPerc=f;}` in EntityBase.gm -- that whole block
// is dead reference script sitting inside a `/* C++ ... */` comment
// (retail's own note that this moved to native code, same pattern as
// DrawSauceBaseEntityNative/getScreenWidth elsewhere in this codebase),
// confirmed real and LIVE at EntityBase.gm:2431's actual playSound() body:
// `return Audio.playSound(soundfile,note,gt,pan,vol*ap*this.
// getLimitVolPerc());` -- was entirely unbound, so every real playSound()
// call on an EntityBase-family entity crashed multiplying by a
// null-call-lenient null. Default 1.0 -- confirmed exact real default via
// the SAME commented reference block a few hundred lines later
// (EntityBase.gm:4205: `//C++ t.limitVolPerc=1;`).
int GM_CDECL gmf_entity_getLimitVolPerc(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushFloat(1.0f); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "__limitVolPerc");
    a_thread->PushFloat(v.m_type == GM_FLOAT ? v.m_value.m_float : (v.m_type == GM_INT ? (float)v.m_value.m_int : 1.0f));
    return GM_OK;
}
int GM_CDECL gmf_entity_setLimitVolPerc(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(f, 0, 1.0f);
    self->Set(a_thread->GetMachine(), "__limitVolPerc", gmVariable(f));
    return GM_OK;
}
// decompiled body is `!isBeingEdited() && !isPlacing() &&
// GameSystem-shouldUpdateEntities-equivalent-global`, both flags already
// real state on this handle (isBeingEdited/isPlacing above) and the
// existing GameSystemState::shouldUpdateEntities flag (game_system_state.h
// -- confirmed real, but its own header comment notes "ProcessManager
// doesn't consult this yet"; this is the first real consumer). Real, live
// call site (EntityPacks/Minimal/SingleDirLaserTurret.gm:519's
// onMusicTrigger(): `if (!this.isOkToRun()) { return; }`) -- was entirely
// unbound, so that turret's music-triggered firing logic never ran at all
// (null-call-lenient false is falsy, so `!false`... wait: null is falsy,
// so `!null` is truthy in GM, meaning the ACTUAL prior behavior was the
// turret firing unconditionally regardless of edit/placing state -- this
// makes it correctly stop while being placed/edited in the level editor,
// matching every other real isBeingEdited/isPlacing consumer in this file).
int GM_CDECL gmf_entity_isOkToRun(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(1); return GM_OK; }
    gmMachine& m = *a_thread->GetMachine();
    gmVariable beingEdited = self->Get(&m, "__beingEdited");
    gmVariable placing = self->Get(&m, "__placing");
    bool ok = !(beingEdited.m_type == GM_INT && beingEdited.m_value.m_int != 0) &&
              !(placing.m_type == GM_INT && placing.m_value.m_int != 0) &&
              GetEngineContext().gameSystem.shouldUpdateEntities;
    a_thread->PushInt(ok ? 1 : 0);
    return GM_OK;
}
// 0x001c63dc), real live call sites (EntityBase.gm:3502/3510's
// fadeOutVoiceSync/stopVoiceSync, GameStuff/GoombaCritterLoopPlayerBase.gm
// :147) -- relinquishes this entity's ownership of an audio voice so it can
// keep fading out independently after the entity itself goes away. No
// per-voice ownership tracking exists in this reimplementation's audio
// state (see audio_bindings.h) -- genuine no-op, consumed rather than left
// as a null-call that would abort fadeOutVoiceSync before its own
// Audio.fadeOutVoice(...) call right after ever ran.
int GM_CDECL gmf_entity_disownVoice(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// cppDie()/cppRebirth() -- confirmed real, pure-native hooks (the "cpp"
// prefix matches this corpus's established convention for methods with no
// script-side definition anywhere, e.g. constructorSetNoPhysics/
// constructorSetNoUpdate above). Live call sites Player.gm:4927 (right
// after disabling the player's collision on death: `this.Entity.
// cppDie();`) and Player.gm:4794 (right after re-enabling collision on
// respawn: `this.Entity.cppRebirth();`) -- both were entirely unbound,
// hard "getdot failed" aborting the death/rebirth state-change every
// single time, before `this.state.call("die",this)` or the FIRSTSPAWN
// state change could run: touching a hazard visually froze mid-death
// instead of completing the death sequence. Whatever native bookkeeping
// retail's own C++ Entity/Player class did here (physics-body sleep state,
// render-list membership, ...) is already handled by this
// reimplementation's own script-driven state machine and native
// setIsEnabled/physics-body plumbing -- genuine no-op, consumed rather
// than left as a null-call.
int GM_CDECL gmf_entity_cppDie(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_entity_cppRebirth(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// forceAudibilityRecalc() -- confirmed real, live call site Main.gm:8497,
// part of the same checkpoint-restore pass as setGot/
// set_getAudibilityPerc_neverOnScreen above (`e.forceAudibilityRecalc();`
// right after restoring both flags from a checkpoint). Was entirely
// unbound -- hard "getdot failed" aborting the whole checkpoint-restore
// loop after just the FIRST note plant, leaving every subsequent one's
// got/audibility state stuck at whatever it was before the checkpoint
// reload. No distance/audibility-falloff audio model exists in this
// reimplementation yet (see getNumOffScreensAudible's own header comment)
// -- genuine no-op, consumed rather than left as a null-call.
int GM_CDECL gmf_entity_forceAudibilityRecalc(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// getCurrentScreenY() -- confirmed real, ENTITY-side counterpart to
// GameSystem.getCurrentScreenY() (process_handle_bindings.cpp's
// gmf_getCurrentScreenY, already implemented for process handles). Live
// call site BaseNotePlant.gm:544: `local bottom=this.parent.
// getCurrentScreenY()*this.parent.gameProcess.getScreenHeight();` where
// `this.parent` is the note-plant ENTITY itself (`parent=this` in its own
// createCoryLoopNoteExplosionGraphic table literal, not a process) -- was
// entirely unbound on entity handles, hard "getdot failed" aborting this
// loop-graphic-cell-index computation every call. Entities track no
// per-instance screen position of their own in this corpus; delegates to
// the entity's own `gameProcess` field (set by script via `t.gameProcess=
// GameSystem.getEntityUpdateProcess()`, confirmed real convention) reading
// the SAME `currentScreenY` field process handles already expose, which is
// the correct answer for a single-screen-camera game like this one anyway.
int GM_CDECL gmf_entity_getCurrentScreenY(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmMachine& m = *a_thread->GetMachine();
    gmVariable gp = self->Get(&m, "gameProcess");
    if (gp.m_type == GM_TABLE) {
        gmTableObject* gpTable = (gmTableObject*)m.GetObject(gp.m_value.m_ref);
        gmVariable v = gpTable->Get(&m, "currentScreenY");
        if (v.m_type == GM_INT || v.m_type == GM_FLOAT) { a_thread->Push(v); return GM_OK; }
    }
    a_thread->PushInt(0);
    return GM_OK;
}
// getCurrentScreenX() -- X counterpart to getCurrentScreenY above, same
// reasoning. Live call site EntityBase.gm:5013 (part of the same per-
// entity screen-tracking update() pass) -- was entirely unbound, hard
// "getdot failed" aborting entity update() every frame for any entity type
// reaching this code path (a genuinely halting crash under this engine's
// default GE3_STRICT=halt mode, reached almost immediately on entering any
// real level).
// isPlayer() -- confirmed real, live call site GoAroundGuy.gm:548:
// `if (!this.isChangesDirOnPlayer()&&(e.isPlayer()||e.isPlayerRelated())){...}`
// -- reads the SAME `__isPlayer` field gmf_EM_createEntityAsPlayer already
// sets (getAllPlayerEntities, above, already reads it internally) but had
// no script-callable getter. Was entirely unbound -- hard "getdot failed"
// aborting GoAroundGuy's per-frame collision-response update() every time
// it touched anything, under this engine's default GE3_STRICT=halt mode a
// process-ending crash reached on the very first level with one of these
// enemies in it.
int GM_CDECL gmf_entity_isPlayer(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "__isPlayer");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
// isPlayerRelated() -- same call site as isPlayer() above, no separate
// confirmed real semantics found anywhere in this corpus (no "related
// entity" concept -- e.g. a vehicle the player is currently riding --
// exists elsewhere in this reimplementation). Default false, matching
// every entity that isn't the player themselves; was entirely unbound.
int GM_CDECL gmf_entity_isPlayerRelated(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_entity_getCurrentScreenX(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmMachine& m = *a_thread->GetMachine();
    gmVariable gp = self->Get(&m, "gameProcess");
    if (gp.m_type == GM_TABLE) {
        gmTableObject* gpTable = (gmTableObject*)m.GetObject(gp.m_value.m_ref);
        gmVariable v = gpTable->Get(&m, "currentScreenX");
        if (v.m_type == GM_INT || v.m_type == GM_FLOAT) { a_thread->Push(v); return GM_OK; }
    }
    a_thread->PushInt(0);
    return GM_OK;
}

int GM_CDECL gmf_entity_constructorSetNoPhysics(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_entity_constructorSetNoUpdate(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// constructorSetManual/constructorSetNoPhysicsUpdate -- confirmed real
// no-arg construction-time-flag shape as constructorSetNoPhysics/
// constructorSetNoUpdate above (real call sites: GroundBase.gm:94
// `this.constructorSetManual();`; WinThing.gm:84 `this.
// constructorSetNoPhysicsUpdate();`). Same rationale: nothing in this
// reimplementation allocates a physics body or drives per-frame update
// unless a script explicitly asks for one, so both are genuine no-ops.
int GM_CDECL gmf_entity_constructorSetManual(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_entity_constructorSetNoPhysicsUpdate(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// getType/getTypeName -- confirmed real (113 combined call sites across
// Main.gm/EntityBase.gm), backed by the same InternTypeId registry as the
// native ENTITY_GetType(name) global (used identically -- an entity's own
// getType() must agree with ENTITY_GetType(entity.getTypeName()) since
// scripts compare them directly, e.g. `ENTITYTYPE_SPAWNPOINT==e.getType()`).
// Was entirely missing, so `entity.getType()` silently null-called to
// null (leniency), then got used as a raw table KEY
// (`this.entitiesByType[entityTypeID]`) -- GM accepts a null key without
// complaint, so every entity of every type collided into the SAME
// `entitiesByType[null]` bucket, and once two entities landed there
// together `ARRAYHELPER_InsertSorted`'s internal list bookkeeping broke
// in a way that surfaced several call frames away
int GM_CDECL gmf_entity_getType(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "__typeId");
    a_thread->PushInt(v.m_type == GM_INT ? v.m_value.m_int : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_getTypeName(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    a_thread->Push(self->Get(a_thread->GetMachine(), "__typeName"));
    return GM_OK;
}

// reading a precomputed per-entity visibility flag byte off the native
// entity struct); the real pass that COMPUTES that flag each frame isn't
// reconstructable from the corpus, but this is real load-bearing
// infrastructure -- GroundProcessNEW.gm's drawGrounds()
// (`if (!t.entity.isInView()) { continue; }`) skips a ground piece's
// ENTIRE draw call when this returns falsy, and this engine's own native
// ground renderer (engine/src/ported_native/ground_renderer.cpp) uses the
// exact same check, so a wrong answer here silently hides real level
// geometry with no other symptom.
// Implemented as a real camera-vs-entity-AABB test: the entity's own
// PhysicsData AABB (GetPhysicsDataAABB, physics_body_bindings.h) against
// the SAME visible world rect Renderer::UpdateProjection actually
// projects (kWorldHalfHeight, aspect, screenViewScale -- mirrored here
// rather than guessed, so this stays correct if the camera zooms via the
// TV-overscan calibration screen). Falls back to a zero-size point test
// at configureData.posX/posY for entities with no physics body (most
// decorations/triggers/spawn points).
int GM_CDECL gmf_entity_isInView(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer) { a_thread->PushInt(1); return GM_OK; }

    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    gmVariable pdVar = self->Get(&m, "PhysicsData");
    if (pdVar.m_type == GM_NULL) pdVar = self->Get(&m, "__physicsData");
    if (!GetPhysicsDataAABB(m, pdVar, minX, minY, maxX, maxY)) {
        gmVariable cd = self->Get(&m, "configureData");
        if (cd.m_type == GM_TABLE) {
            gmTableObject* cdt = (gmTableObject*)m.GetObject(cd.m_value.m_ref);
            gmVariable xv = cdt->Get(&m, "posX"), yv = cdt->Get(&m, "posY");
            auto tofloat = [](const gmVariable& v) -> float {
                if (v.m_type == GM_FLOAT) return v.m_value.m_float;
                if (v.m_type == GM_INT) return (float)v.m_value.m_int;
                return 0.0f;
            };
            minX = maxX = tofloat(xv);
            minY = maxY = tofloat(yv);
        }
    }

    // Visible world extent derives from the real perspective projection
    // (scales with camera Z) -- see Renderer::kTanHalfFovY.
    float halfH = renderer->GetWorldHalfHeight();
    float halfW = renderer->GetWorldHalfWidth();
    float scale = GetEngineContext().video.screenViewScale;
    if (scale > 0.0001f) { halfH /= scale; halfW /= scale; }
    float cx = renderer->GetCameraX(), cy = renderer->GetCameraY();

    bool visible = !(maxX < cx - halfW || minX > cx + halfW || maxY < cy - halfH || minY > cy + halfH);
    a_thread->PushInt(visible ? 1 : 0);
    return GM_OK;
}

// isInGameBoard()/setIsBoundedToGameBoard(bool) -- confirmed real (Ghidra:
// retail FUN_0043df00/FUN_0043de90 @ 0x0043df00/0x0043de90). Real caller:
// Thing.gm:322 (`if (this.wasInGameBoardOnce && !local0 &&
// !this.Entity.isInGameBoard())` -- offscreen enemy/wall cleanup, part of
// the same PhysicsData position this file's getMinCX/getMaxCX etc already
// read via GetPhysicsDataAABB). Tests the entity's own PhysicsData centre
// against the SAME bounds GameBoard.isInGameBoard() uses (IsPointInGameBoard,
// foe_compat_bindings.cpp) so the two stay consistent.
// setIsBoundedToGameBoard is a real setter (Thing.gm:233:
// `this.Entity.setIsBoundedToGameBoard(0);`) but this engine has no actual
// hard-clamping/wrapping behavior tied to it (no confirmed retail source
// for exactly what "bounded" enforcement looks like) -- stored as an
// honest flag rather than faked clamping, consistent with several other
// "real setter, not-yet-consumed" fields already tracked this way (e.g.
// PhysicsBody's maxRotateSpeed/rotationDecayRate, see that file's header
// comment).
int GM_CDECL gmf_entity_isInGameBoard(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    gmVariable pdVar = self->Get(&m, "PhysicsData");
    if (pdVar.m_type == GM_NULL) pdVar = self->Get(&m, "__physicsData");
    bool haveAABB = GetPhysicsDataAABB(m, pdVar, minX, minY, maxX, maxY);
    bool inBoard = true;
    if (haveAABB) {
        float cx = (minX + maxX) * 0.5f, cy = (minY + maxY) * 0.5f;
        inBoard = IsPointInGameBoard(cx, cy);
    }
    a_thread->PushInt(inBoard ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_setIsBoundedToGameBoard(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    self->Set(a_thread->GetMachine(), "__isBoundedToGameBoard", a_thread->Param(0));
    return GM_OK;
}
int GM_CDECL gmf_entity_isBoundedToGameBoard(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable v = self->Get(&m, "__isBoundedToGameBoard");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int == 0) ? 0 : 1); // default true, matches retail's own default-on convention for bounding flags
    return GM_OK;
}

int GM_CDECL gmf_entity_loadAnimation(gmThread* a_thread); // defined below, with the other Animation/Instrument stubs

// Entity.setupDrawList([width, height]) -- confirmed real (197 call sites,
// e.g. `this.drawList=this.setupDrawList(1024*2,32*4);`,
// BaseNotePlant.gm's `t.drawList=t.Entity.setupDrawList(128*8*3,128*2);`)
// but entirely unregistered, so every one of those assignments silently
// resolved to null via undefined-call leniency -- any real per-entity
// `draw()` hook that then does `this.drawList.reset()` hard-crashed
// ("getdot failed", calling a method on null) the moment anything actually
// invoked it. Same relationship to `Video.createDrawList` as
// `getPhysicsData`'s to `PhysicsData()`: a per-entity-owned drawList
// instead of a bare global one. Width/height are real retail's static-
// surface-cache sizing hints for a subsystem this reimplementation's
// immediate-mode renderer doesn't have (see draw_list.h's header comment) --
// safe to ignore, same as Video.createDrawList already does.
int GM_CDECL gmf_entity_setupDrawList(gmThread* a_thread) {
    int width = 0x400, height = 0x20;
    if (a_thread->GetNumParams() >= 1) width = (int)a_thread->ParamInt(0, (gmint)width);
    if (a_thread->GetNumParams() >= 2) height = (int)a_thread->ParamInt(1, (gmint)height);
    PushNewDrawList(a_thread, width, height);
    return GM_OK;
}

void InjectEntityHandleMethods(gmMachine& a_machine, gmTableObject* a_table, int a_uid, const std::string& a_typeName) {
    a_table->Set(&a_machine, "__uid", gmVariable((gmint)a_uid));
    a_table->Set(&a_machine, "__typeId", gmVariable((gmint)InternTypeId(a_typeName)));
    a_table->Set(&a_machine, "__typeName", gmVariable(a_machine.AllocStringObject(a_typeName.c_str())));
    a_table->Set(&a_machine, "getType", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getType)));
    a_table->Set(&a_machine, "getTypeName", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getTypeName)));
    a_table->Set(&a_machine, "getPhysicsData", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getPhysicsData)));
    a_table->Set(&a_machine, "getMainBody", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMainBody)));
    a_table->Set(&a_machine, "getVectorToNormalized", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getVectorToNormalized)));
    a_table->Set(&a_machine, "setIsEnabled", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsEnabled)));
    a_table->Set(&a_machine, "isEnabled", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isEnabled)));
    a_table->Set(&a_machine, "setIsAutoRemove", gmVariable(a_machine.AllocFunctionObject(gmf_entity_noop)));
    a_table->Set(&a_machine, "setIsAutoRemoveStranded", gmVariable(a_machine.AllocFunctionObject(gmf_entity_noop)));
    a_table->Set(&a_machine, "setIsInvisible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsInvisible)));
    a_table->Set(&a_machine, "isInvisible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isInvisible)));
    a_table->Set(&a_machine, "setIsEditable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsEditable)));
    a_table->Set(&a_machine, "isEditable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isEditable)));
    a_table->Set(&a_machine, "constructorSetNoPhysics", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetNoPhysics)));
    a_table->Set(&a_machine, "constructorSetNoUpdate", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetNoUpdate)));
    a_table->Set(&a_machine, "constructorSetManual", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetManual)));
    a_table->Set(&a_machine, "constructorSetNoPhysicsUpdate", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetNoPhysicsUpdate)));
    a_table->Set(&a_machine, "clearTelescopes", gmVariable(a_machine.AllocFunctionObject(gmf_entity_clearTelescopes)));
    a_table->Set(&a_machine, "addTelescope", gmVariable(a_machine.AllocFunctionObject(gmf_entity_addTelescope)));
    a_table->Set(&a_machine, "isBeingEdited", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isBeingEdited)));
    a_table->Set(&a_machine, "setIsBeingEdited", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsBeingEdited)));
    a_table->Set(&a_machine, "isPlacing", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isPlacing)));
    a_table->Set(&a_machine, "setIsPlacing", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsPlacing)));
    a_table->Set(&a_machine, "isAlwaysProcessed", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isAlwaysProcessed)));
    a_table->Set(&a_machine, "isStaticDraw", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isStaticDraw)));
    a_table->Set(&a_machine, "isSauceBase", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isSauceBase)));
    a_table->Set(&a_machine, "setIsAlwaysProcessed", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsAlwaysProcessed)));
    a_table->Set(&a_machine, "getNumOffScreensAudible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getNumOffScreensAudible)));
    a_table->Set(&a_machine, "setNumOffScreensAudible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setNumOffScreensAudible)));
    a_table->Set(&a_machine, "set_getAudibilityPerc_neverOnScreen", gmVariable(a_machine.AllocFunctionObject(gmf_entity_set_getAudibilityPerc_neverOnScreen)));
    a_table->Set(&a_machine, "is_getAudibilityPerc_neverOnScreen", gmVariable(a_machine.AllocFunctionObject(gmf_entity_is_getAudibilityPerc_neverOnScreen)));
    a_table->Set(&a_machine, "setGot", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setGot)));
    a_table->Set(&a_machine, "isGot", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isGot)));
    a_table->Set(&a_machine, "isGetting", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isGetting)));
    a_table->Set(&a_machine, "getCurrentScreenY", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getCurrentScreenY)));
    a_table->Set(&a_machine, "getCurrentScreenX", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getCurrentScreenX)));
    a_table->Set(&a_machine, "isPlayer", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isPlayer)));
    a_table->Set(&a_machine, "isPlayerRelated", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isPlayerRelated)));
    a_table->Set(&a_machine, "setEditSensorBox", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setEditSensorBox)));
    a_table->Set(&a_machine, "getMixerParams", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMixerParams)));
    a_table->Set(&a_machine, "getMixerID", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMixerID)));
    a_table->Set(&a_machine, "setMixerID", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setMixerID)));
    a_table->Set(&a_machine, "getLimitVolPerc", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getLimitVolPerc)));
    a_table->Set(&a_machine, "setLimitVolPerc", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setLimitVolPerc)));
    a_table->Set(&a_machine, "isOkToRun", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isOkToRun)));
    a_table->Set(&a_machine, "disownVoice", gmVariable(a_machine.AllocFunctionObject(gmf_entity_disownVoice)));
    a_table->Set(&a_machine, "setIsSauceBase", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsSauceBase)));
    a_table->Set(&a_machine, "redrawSauceBase", gmVariable(a_machine.AllocFunctionObject(gmf_entity_redrawSauceBase)));
    a_table->Set(&a_machine, "cppDie", gmVariable(a_machine.AllocFunctionObject(gmf_entity_cppDie)));
    a_table->Set(&a_machine, "cppRebirth", gmVariable(a_machine.AllocFunctionObject(gmf_entity_cppRebirth)));
    a_table->Set(&a_machine, "forceAudibilityRecalc", gmVariable(a_machine.AllocFunctionObject(gmf_entity_forceAudibilityRecalc)));
    a_table->Set(&a_machine, "recordScreenExtents", gmVariable(a_machine.AllocFunctionObject(gmf_entity_recordScreenExtents)));
    a_table->Set(&a_machine, "isOnCurrentScreen", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isOnCurrentScreen)));
    a_table->Set(&a_machine, "initializeScreenExtents", gmVariable(a_machine.AllocFunctionObject(gmf_entity_initializeScreenExtents)));
    // direct Ghidra cross-reference @ 0x001c5d6c-adjacent), same (path,
    // note, gate, pan, vol) contract as Audio.playSound -- see
    // audio_bindings.h's PlaySoundNative.
    a_table->Set(&a_machine, "playSound", gmVariable(a_machine.AllocFunctionObject(PlaySoundNative)));
    a_table->Set(&a_machine, "setIsDeletable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_noop)));
    a_table->Set(&a_machine, "getObjectID", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getUID)));
    a_table->Set(&a_machine, "getUID", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getUID)));
    a_table->Set(&a_machine, "getDescriptionBits", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getDescriptionBits)));
    a_table->Set(&a_machine, "setDescriptionBits", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setDescriptionBits)));
    a_table->Set(&a_machine, "getMinCX", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMinCX)));
    a_table->Set(&a_machine, "getMaxCX", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMaxCX)));
    a_table->Set(&a_machine, "getMinCY", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMinCY)));
    a_table->Set(&a_machine, "getMaxCY", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMaxCY)));
    a_table->Set(&a_machine, "setUIDData", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setUIDData)));
    a_table->Set(&a_machine, "loadAnimation", gmVariable(a_machine.AllocFunctionObject(gmf_entity_loadAnimation)));
    a_table->Set(&a_machine, "isInView", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isInView)));
    a_table->Set(&a_machine, "tagForDeletion", gmVariable(a_machine.AllocFunctionObject(gmf_entity_tagForDeletion)));
    a_table->Set(&a_machine, "isType", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isType)));
    a_table->Set(&a_machine, "isSameType", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isSameType)));
    a_table->Set(&a_machine, "callMethod", gmVariable(a_machine.AllocFunctionObject(gmf_callMethodByName)));
    a_table->Set(&a_machine, "isInGameBoard", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isInGameBoard)));
    a_table->Set(&a_machine, "setIsBoundedToGameBoard", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsBoundedToGameBoard)));
    a_table->Set(&a_machine, "isBoundedToGameBoard", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isBoundedToGameBoard)));
    a_table->Set(&a_machine, "setupDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setupDrawList)));
    // Only ever the bare handle's own fallback -- InjectEntityHandleMethods
    // also runs on the RETURNED behavior table (dual injection, see
    // CreateEntityFromFactory), which for Player.gm already defines its own
    // real die/isWizard/isInvincible; never stomp those.
    if (a_table->Get(&a_machine, "die").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "die", gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_die)));
    }
    if (a_table->Get(&a_machine, "isWizard").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "isWizard", gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_isWizard)));
    }
    if (a_table->Get(&a_machine, "isInvincible").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "isInvincible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_isInvincible)));
    }
    if (a_table->Get(&a_machine, "getStyle").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "getStyle", gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_getStyle)));
    }
    if (a_table->Get(&a_machine, "isGround").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "isGround", gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_isGround)));
    }
    if (a_table->Get(&a_machine, "onDisable").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "onDisable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_onDisable)));
    }
    if (a_table->Get(&a_machine, "onEnable").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "onEnable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_onEnable)));
    }
    // update(msElapsed) -- confirmed real, genuine base-class no-op
    // { return 0; }`). Real call sites (HomeScreenBase.gm's
    // `this.widgets[i].update(msElapsed)` loop over every menu widget
    // entity) call this unconditionally on every entity, expecting
    // whichever ones don't define their own real update() to just do
    // nothing -- same dual-injection "don't stomp a real override" guard
    // as die/isWizard/isInvincible above.
    if (a_table->Get(&a_machine, "update").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "update", gmVariable(a_machine.AllocFunctionObject(gmf_entity_noop)));
    }

    // FoE compatibility methods (alarm/LFO management) -- injected from foe_compat_bindings
    a_table->Set(&a_machine, "getUniqueAlarmID", gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueAlarmID)));
    a_table->Set(&a_machine, "addAlarm", gmVariable(a_machine.AllocFunctionObject(gmf_addAlarm)));
    a_table->Set(&a_machine, "deleteAlarm", gmVariable(a_machine.AllocFunctionObject(gmf_deleteAlarm)));
    a_table->Set(&a_machine, "getAlarm", gmVariable(a_machine.AllocFunctionObject(gmf_getAlarm)));
    a_table->Set(&a_machine, "getUniqueLFOID", gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueLFOID)));
    a_table->Set(&a_machine, "addLFO", gmVariable(a_machine.AllocFunctionObject(gmf_addLFO)));
    a_table->Set(&a_machine, "deleteLFO", gmVariable(a_machine.AllocFunctionObject(gmf_deleteLFO)));
}

// createEntity(typeName, ...) -- confirmed real convention matching
// ProcessManager::StartNewProcessList exactly (see
// game_data/Platformer/Player.gm:8's
// `global Platformer_Player=function(e) { return {...}; };` and real call
// site `EntityManager.createEntityAsPlayer("Platformer_Player")`): builds
// a native entity handle `e`, calls the global factory function by name
// with `e` as its only argument, auto-invokes the returned table's
// constructor() (confirmed necessary: Main.gm calls
// `this.player.mainpb.setPos(...)` immediately after createEntityAsPlayer
// with no explicit `.constructor()` call anywhere in between -- exactly
// mirroring ProcessManager::FinishBringUp's auto constructor() call), and
// registers the RETURNED table (not the raw handle) as the tracked entity,
// since that's what scripts hold onto and what getEntity()/etc need to
// find later.
// Same naming convention/fallback as ProcessManager::TryAutoLoadScript
// (Platformer_Player -> Platformer/Player.gm, flat first then nested with
// underscores as path separators) -- entity type names follow the exact
// same "Platformer_Foo" global-factory convention as process names, and
// nothing anywhere pre-loads Player.gm/EntityPacks/*.gm ahead of the first
// createEntity() call that needs them (confirmed: without this,
// `EntityManager.createEntityAsPlayer("Platformer_Player")` finds no
// global function at all, exactly like the pre-auto-load process bug).
bool TryAutoLoadEntityScript(gmMachine& a_machine, const std::string& a_typeName) {
    std::string rest = a_typeName;
    const std::string prefix = "Platformer_";
    bool hasPrefix = a_typeName.compare(0, prefix.size(), prefix) == 0;
    if (hasPrefix) rest = a_typeName.substr(prefix.size());

    std::string source;
    std::string flatPath = hasPrefix ? ("Platformer/" + rest + ".gm") : (a_typeName + ".gm");
    std::string path = flatPath;
    if (GetEngineContext().fileHelper->ReadTextFile(path, source)) {
        int errors = a_machine.ExecuteString(source.c_str(), nullptr, true, path.c_str());
        return errors == 0;
    }
    if (hasPrefix) {
        std::string nested = rest;
        for (char& c : nested) if (c == '_') c = '/';
        path = "Platformer/" + nested + ".gm";
        if (path != flatPath && GetEngineContext().fileHelper->ReadTextFile(path, source)) {
            int errors = a_machine.ExecuteString(source.c_str(), nullptr, true, path.c_str());
            return errors == 0;
        }
        return false;
    }

    // Generic form of the same convention, without a fixed "Platformer/"
    // root -- same fix ProcessManager::TryAutoLoadScript already needed
    // (see that function's header comment: confirmed necessary for
    // "PekkoAndJon_Main" -> PekkoAndJon/Main.gm). This entity-side loader
    // never got the same generalization, so `EntityManager.createEntity
    // As Player("PekkoAndJon_Player")` (PekkoAndJon/Main.gm:368) found no
    // global function and fell back to a bare, non-functional handle --
    // the player entity silently never existed. Try turning each
    // underscore into a slash, one at a time left to right.
    std::string candidate = a_typeName;
    for (size_t i = 0; i < candidate.size(); ++i) {
        if (candidate[i] != '_') continue;
        candidate[i] = '/';
        std::string tryPath = candidate + ".gm";
        if (GetEngineContext().fileHelper->ReadTextFile(tryPath, source)) {
            int errors = a_machine.ExecuteString(source.c_str(), nullptr, true, tryPath.c_str());
            return errors == 0;
        }
    }
    return false;
}

// configure() trampoline -- see CreateEntityFromFactory's own call site
// comment for why this exists. Forwards every argument through unchanged
// to the real, saved-off original (`__origConfigure`), just bracketed by
// the __nativeConfiguring/__nativeConfigured bookkeeping fields
// ConfigureUnconfiguredEntities (below) and gmf_entity_setIsEnabled (see
// its own updated header comment) both read.
int GM_CDECL gmf_entity_configureTrampoline(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable orig = self->Get(&m, "__origConfigure");
    if (orig.m_type != GM_FUNCTION) return GM_OK;

    self->Set(&m, "__nativeConfiguring", gmVariable((gmint)1));
    gmVariable selfVar(self);
    gmCall call;
    gmVariable result = gmVariable::s_null;
    if (call.BeginTableFunction(&m, "__origConfigure", self, selfVar)) {
        int n = a_thread->GetNumParams();
        for (int i = 0; i < n; ++i) call.AddParam(a_thread->Param(i));
        call.End();
        if (call.DidReturnVariable()) result = call.GetReturnedVariable();
    }
    self->Set(&m, "__nativeConfiguring", gmVariable((gmint)0));
    self->Set(&m, "__nativeConfigured", gmVariable((gmint)1));

    if (result.m_type != GM_NULL) a_thread->Push(result);
    return GM_OK;
}

gmVariable CreateEntityFromFactory(gmMachine& a_machine, const std::string& a_typeName, int a_uid) {
    gmTableObject* handleTable = a_machine.AllocTableObject();
    InjectEntityHandleMethods(a_machine, handleTable, a_uid, a_typeName);
    // ENGINE PATCH: the raw handle is entirely C++-built -- unlike the
    // factory's RETURNED table below, which is script-authored gameplay
    // logic with native methods dual-injected on top of it and must stay
    // unmarked (see strict_calls.h's MarkNative) -- so a missing method
    // looked up directly on this object is unambiguously an engine gap.
    StrictCalls::MarkNative(a_machine, handleTable);
    gmVariable handle(handleTable);

    gmVariable globalFunc = a_machine.GetGlobals()->Get(&a_machine, a_typeName.c_str());
    if (globalFunc.m_type != GM_FUNCTION && TryAutoLoadEntityScript(a_machine, a_typeName)) {
        globalFunc = a_machine.GetGlobals()->Get(&a_machine, a_typeName.c_str());
    }
    if (globalFunc.m_type != GM_FUNCTION) {
        std::fprintf(stderr, "EntityManager.createEntity: global '%s' is not a function, using bare handle\n", a_typeName.c_str());
        return handle;
    }

    gmCall call;
    if (!call.BeginGlobalFunction(&a_machine, a_typeName.c_str())) {
        std::fprintf(stderr, "EntityManager.createEntity: failed to begin call to '%s'\n", a_typeName.c_str());
        return handle;
    }
    call.AddParam(handle);
    call.End();

    if (!call.DidReturnVariable() || call.GetReturnedVariable().m_type != GM_TABLE) {
        std::fprintf(stderr, "EntityManager.createEntity: '%s(e)' did not return a table, using bare handle\n", a_typeName.c_str());
        return handle;
    }

    gmVariable entityTable = call.GetReturnedVariable();
    gmTableObject* table = (gmTableObject*)a_machine.GetObject(entityTable.m_value.m_ref);
    // Dual injection, same reasoning as InjectProcessHandleMethods: real
    // scripts call these on the RETURNED table too (see
    // gmf_entity_getPhysicsData's header comment), not just the handle.
    InjectEntityHandleMethods(a_machine, table, a_uid, a_typeName);

    // ENGINE PATCH (2026-08-17): wrap the entity's own real `configure`
    // function (if it has one) behind a guarded trampoline -- see
    // ConfigureUnconfiguredEntities's header comment below for the full
    // problem this solves. Tracks two bookkeeping fields on the entity:
    // __nativeConfiguring (true only while a configure() call is actively
    // running, prevents infinite recursion when configure()'s own body
    // triggers a re-entrant enable) and __nativeConfigured (true once any
    // configure() call has completed, so nothing tries to configure the
    // same entity twice). Every real entity type's configure() is called
    // exactly the same way normally would be (same args, same `this`) --
    // this only adds bookkeeping around the call, never changes its
    // behavior.
    {
        gmVariable configureFn = table->Get(&a_machine, "configure");
        if (configureFn.m_type == GM_FUNCTION) {
            table->Set(&a_machine, "__origConfigure", configureFn);
            table->Set(&a_machine, "configure", gmVariable(a_machine.AllocFunctionObject(gmf_entity_configureTrampoline)));
        }
    }

    // Link the bare handle back to its real behavior table -- see
    // gmf_entity_delegate_die's header comment for why this exists: real
    // call sites (Main.gm's KillPlayer handler) call die()/isWizard()/
    // isInvincible() directly on `this.Entity` (the handle), expecting the
    // same object identity retail's own unified Entity/Player has.
    handleTable->Set(&a_machine, "__behaviorOwner", entityTable);

    // Repoint the PhysicsData's owner at the real final table -- see
    // SetPhysicsDataOwnerEntity's header comment (physics_body_bindings.h)
    // for why it was captured against the wrong table (the handle) in the
    // first place, whenever the factory built it via
    // `PhysicsData=_entity.getPhysicsData()` in its own object literal.
    {
        gmVariable pdVar = table->Get(&a_machine, "PhysicsData");
        if (pdVar.m_type == GM_NULL) pdVar = table->Get(&a_machine, "__physicsData");
        if (pdVar.m_type != GM_NULL) {
            SetPhysicsDataOwnerEntity(a_machine, pdVar, entityTable);
            // Same handle/behavior-table split as configureData below --
            // ENTITYBASE_startEntityCollectionEdit_setupEntityShapeSensors
            // Stuff(this,this.Entity) reads `e.PhysicsData` (the HANDLE's
            // own copy) right after the configureData field this same
            // function also reads, and hits the identical "was never set
            // on the handle" hard crash there too (confirmed live: fixing
            // configureData alone moved the exact same repro one line
            // further, to `local nb=pd.getNumBodies();` reading a null
            // `pd`). Mirrors the real PhysicsData object directly onto the
            // handle so `this.Entity.PhysicsData` resolves the same real
            // PhysicsData every other real call site already gets.
            handleTable->Set(&a_machine, "PhysicsData", pdVar);
        }
    }

    // Defensive fallback: every entity type's `configure()` is EXPECTED to
    // flow through EntityBase.gm's install wrapper (`t.configure=function(
    // configureData){this.configureData=configureData; ...}`), but not
    // every entity type in a real level installs it the same way
    // (confirmed several distinct patterns just in this corpus:
    // GroundBase_Install, EntityBase_Install, EntityBase_Install_Small,
    // and presumably others for non-ground entity types this pass hasn't
    // traced) -- when one doesn't, `this.configureData` never gets set at
    // all, and Platformer/Main.gm's per-type sorted entity list
    // (`this.scrAddEntityLessObj.fn`: `a.configureData.uid<b.configureData.uid`)
    // hard-crashes ("getdot failed") the moment two same-type entities are
    // compared. Seeding a minimal `{uid=a_uid}` BEFORE the factory's
    // constructor()/configure() ever runs means any real install path
    // that DOES set configureData simply overwrites this placeholder
    // (this field assignment always happens unconditionally on entry to
    // the wrapped configure()), so this only matters as a safety net for
    // whichever entity types don't.
    {
        gmTableObject* placeholder = a_machine.AllocTableObject();
        placeholder->Set(&a_machine, "uid", gmVariable((gmint)a_uid));
        table->Set(&a_machine, "configureData", gmVariable(placeholder));
        // Real, reproducible crash without this: `this.Entity.configureData`
        // (the bare handle's OWN copy, read by e.g. ENTITYBASE_
        // startEntityCollectionEdit_setupEntityShapeSensorsStuff(this,
        // this.Entity) -> local cd=e.configureData -> SYSTEMHELPER_
        // MakeTableCopy(cd) -> `foreach(key and value in cd)`) was never
        // set at all (only the behavior table's own `this.configureData`
        // was seeded above) -- foreach on a GM_NULL value is a hard
        // "foreach expression has no iterator function" exception in this
        // GM implementation, which aborts the ENTIRE calling chain
        // (configure() -> endConfigure() -> ...) partway through, so
        // BaseNotePlant.gm's own `this.graphic=this.createNoteGraphic();`
        // (later in configure(), never reached once this exception fires)
        // silently never ran -- every subsequent draw() call then hit a
        // SEPARATE hard "getdot failed" on `this.graphic.draw(...)`,
        // forever, for that entity. Confirmed via a live repro (place any
        // note-plant entity from the editor's palette) and reading the
        // exact crash callstack. Sharing the SAME placeholder object (not
        // a copy) between the handle and the behavior table matches how
        // every real call site actually uses it: `local t=newEntity.
        // configureData; t.posX=px; ...; newEntity.configure(t);` MUTATES
        // the existing table in place and passes the SAME reference back
        // into configure() (`this.configureData=configureData`), so the
        // handle's copy of the reference stays correctly in sync for
        // every real usage pattern found in this corpus, without needing
        // genuine live property delegation (which plain GM table field
        // access has no hook for).
        handleTable->Set(&a_machine, "configureData", gmVariable(placeholder));
    }

    gmCall ctorCall;
    if (ctorCall.BeginTableFunction(&a_machine, "constructor", table, entityTable)) {
        ctorCall.End();
    }
    return entityTable;
}

int GM_CDECL gmf_EM_createEntity(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(typeName, 0, "");
    int uid = Entities().nextId++;
    gmVariable var = CreateEntityFromFactory(m, typeName, uid);
    gmTableObject* table = (gmTableObject*)m.GetObject(var.m_value.m_ref);
    table->Set(&m, "__uid", gmVariable((gmint)uid));
    RootGmVariable(m, var);
    Entities().entities.push_back({uid, var});
    a_thread->Push(var);
    return GM_OK;
}
// createEntityAsPlayer: same real registry, just also tagged so scripts
// checking their own entity for "am I a player" get a consistent answer.
int GM_CDECL gmf_EM_createEntityAsPlayer(gmThread* a_thread) {
    int result = gmf_EM_createEntity(a_thread);
    if (result == GM_OK) {
        gmVariable pushed = *(a_thread->GetTop() - 1);
        gmTableObject* table = (gmTableObject*)a_thread->GetMachine()->GetObject(pushed.m_value.m_ref);
        table->Set(a_thread->GetMachine(), "__isPlayer", gmVariable((gmint)1));
    }
    return result;
}
int GM_CDECL gmf_EM_getEntity(gmThread* a_thread) {
    // GM_FLOAT_OR_INT_PARAM, not GM_INT_PARAM: real level entity uids come
    // from XML data (`<element key="uid" value="620"/>`), which always
    // parses as a GM_FLOAT (gamemonkeyscript_bindings.cpp's ParseLeafValue)
    // -- the strict int-only param silently threw a blank exception for
    // every real configureEntities() call (same bug class as
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(uidF, 0, 0.0f);
    int uid = (int)uidF;
    gmVariable* e = FindEntity(uid);
    if (!e) e = FindEntityByScriptUID(*a_thread->GetMachine(), uid); // see its header comment
    if (e) a_thread->Push(*e); else a_thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_EM_getEntityFromUID(gmThread* a_thread) { return gmf_EM_getEntity(a_thread); }
int GM_CDECL gmf_EM_isEntityIDValid(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(uidF, 0, 0.0f);
    int uid = (int)uidF;
    bool found = FindEntity(uid) != nullptr || FindEntityByScriptUID(*a_thread->GetMachine(), uid) != nullptr;
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_EM_setEntityUID(gmThread* a_thread) {
    // Confirmed real by usage only -- reassigns which id an already-live
    // entity table is tracked under, so a later getEntity(newUid) finds
    // it. No corpus call sites to confirm the exact param order beyond
    // (entity, newUid), the structurally obvious reading.
    GM_CHECK_NUM_PARAMS(2);
    gmVariable entityVar = a_thread->Param(0);
    GM_FLOAT_OR_INT_PARAM(newUidF, 1, 0.0f);
    int newUid = (int)newUidF;
    for (auto& e : Entities().entities) {
        if (e.second.m_value.m_ref == entityVar.m_value.m_ref) { e.first = newUid; break; }
    }
    return GM_OK;
}
// --- real ENTITYLISTREFCONTAINER-shaped entity-list iterator -------------
// Every EntityManager.getAllXXXEntities()/getEntitiesNear()/
// Entity.c: getAllEntities 0x000986dc, getAllManualEntities 0x00098ae0,
// getAllNonManualEntities 0x00098c04, getAllPlayerEntities 0x00098e28,
// getEntitiesRoughlyWithin 0x000994a8, getEntitiesNear 0x00099740) ends
// its body allocating the SAME 0x14-byte object via the SAME FUN_0031ad28
// constructor call as the standalone ENTITYLISTREFCONTAINER() native type
// these returns a real linked-list container, not a plain array/table.
// Confirmed by usage too: EVERY real call site across the whole corpus
// (300+, e.g. game_data/Platformer/Main.gm:1399/2759/3070/...,
// EntityBase.gm:330/369/1908/3116/5625/5657) walks the result with
// `for(it=EntityManager.getAllEntities();it.isValid();it.goToNext())`, the
// exact same idiom PhysicsData.getCollidingEntities() already uses (see
// physics_body_bindings.cpp's own EntityIterData/g_entityIterType). Before
// this fix, gmf_EM_getAllEntities/getAllPlayerEntities pushed a bare
// gm_table() -- which has no isValid()/goToNext()/get() at all -- so EVERY
// one of those hundreds of real `it.isValid()` calls was either a hard
// "getdot failed" (GE3_STRICT=halt) or a silent null-call-leniency false,
// meaning the loop body NEVER ran, for the entire life of this engine:
// level-load's own per-screen entity sort (Main.gm:6363), checkpoint
// music-trigger registration (Main.gm:7358), "reset the level"
// (Main.gm:8216's `e.reset()` sweep), death cleanup (Main.gm:9119's
// PlayerDeathBits sweep), and the trophy/wizard-mode entity sweeps
// (Main.gm:10143/11114) were all silently doing nothing. Mirrors
// physics_body_bindings.cpp's own EntityIterData shape exactly, kept as a
// separate type here since that one's file-local (anonymous namespace) and
// this file owns EntityManager, not PhysicsData.
struct EntityListIterData {
    std::vector<gmVariable> entities;
    size_t index = 0;
};
gmType g_entityListIterType = -1;
int GM_CDECL gmf_entityListIter_isValid(gmThread* a_thread) {
    EntityListIterData* it = (EntityListIterData*)a_thread->ThisUserCheckType(g_entityListIterType);
    a_thread->PushInt((it && it->index < it->entities.size()) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_entityListIter_goToNext(gmThread* a_thread) {
    EntityListIterData* it = (EntityListIterData*)a_thread->ThisUserCheckType(g_entityListIterType);
    if (it && it->index < it->entities.size()) ++it->index;
    return GM_OK;
}
int GM_CDECL gmf_entityListIter_get(gmThread* a_thread) {
    EntityListIterData* it = (EntityListIterData*)a_thread->ThisUserCheckType(g_entityListIterType);
    if (it && it->index < it->entities.size()) a_thread->Push(it->entities[it->index]);
    else a_thread->PushNull();
    return GM_OK;
}
gmFunctionEntry s_entityListIterLib[] = {
    {"isValid", gmf_entityListIter_isValid, nullptr},
    {"goToNext", gmf_entityListIter_goToNext, nullptr},
    {"get", gmf_entityListIter_get, nullptr},
    {"getEntity", gmf_entityListIter_get, nullptr},
};
void GM_CDECL EntityListIterGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (EntityListIterData*)a_object->m_user;
}
void PushEntityListIterator(gmThread* a_thread, std::vector<gmVariable> a_entities) {
    EntityListIterData* it = new EntityListIterData();
    it->entities = std::move(a_entities);
    a_thread->PushNewUser(it, g_entityListIterType);
}

// Real per-entity AABB-vs-query-box overlap test backing getEntitiesNear/
// getEntitiesRoughlyWithin below -- same PhysicsData-AABB-with-
// configureData-point-fallback approach gmf_entity_isInView/isInGameBoard
// already use elsewhere in this file, so an entity with no physics body
// (a spawn point, a trigger) still participates as a zero-size point.
bool EntityOverlapsBox(gmMachine& a_machine, gmTableObject* a_entity, float a_qMinX, float a_qMinY, float a_qMaxX, float a_qMaxY) {
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    gmVariable pdVar = a_entity->Get(&a_machine, "PhysicsData");
    if (pdVar.m_type == GM_NULL) pdVar = a_entity->Get(&a_machine, "__physicsData");
    if (!GetPhysicsDataAABB(a_machine, pdVar, minX, minY, maxX, maxY)) {
        gmVariable cd = a_entity->Get(&a_machine, "configureData");
        if (cd.m_type != GM_TABLE) return false; // no position signal at all
        gmTableObject* cdt = (gmTableObject*)a_machine.GetObject(cd.m_value.m_ref);
        gmVariable xv = cdt->Get(&a_machine, "posX"), yv = cdt->Get(&a_machine, "posY");
        auto tofloat = [](const gmVariable& v) -> float {
            if (v.m_type == GM_FLOAT) return v.m_value.m_float;
            if (v.m_type == GM_INT) return (float)v.m_value.m_int;
            return 0.0f;
        };
        minX = maxX = tofloat(xv);
        minY = maxY = tofloat(yv);
    }
    return !(maxX < a_qMinX || minX > a_qMaxX || maxY < a_qMinY || minY > a_qMaxY);
}

// getAllEntities([typeName]) -- confirmed real 0/1-arg dispatch (Entity.c
// 0x000986dc: 0 args = everything, 1 string arg = filter by type name via
// FUN_000c3c14, the SAME by-name lookup ENTITY_GetType/isType already back
// via InternTypeId here). The 1-arg form was previously accepted but
// SILENTLY IGNORED (every real filtered call, e.g. Main.gm:9119's
// `getAllEntities("Platformer_PlayerDeathBits")` or EntityBase.gm:5657's
// `getAllEntities("...Checkpoint")`, returned literally every entity in the
// level instead of just the requested type) -- on top of the missing-
// iterator bug above, so both bugs compounded (returned the wrong SET,
// wrapped in a shape callers couldn't even walk).
int GM_CDECL gmf_EM_getAllEntities(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    bool hasFilter = false;
    std::string typeName;
    if (a_thread->GetNumParams() >= 1 && a_thread->Param(0).m_type == GM_STRING) {
        char buf[256];
        const char* s = a_thread->Param(0).AsString(&m, buf, sizeof(buf));
        if (s) { hasFilter = true; typeName = s; }
    }
    std::vector<gmVariable> matches;
    for (auto& e : Entities().entities) {
        if (!hasFilter) { matches.push_back(e.second); continue; }
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        gmVariable tn = t->Get(&m, "__typeName");
        if (tn.m_type != GM_STRING) continue;
        char buf[256];
        const char* s = tn.AsString(&m, buf, sizeof(buf));
        if (s && typeName == s) matches.push_back(e.second);
    }
    PushEntityListIterator(a_thread, std::move(matches));
    return GM_OK;
}
// getAllNonManualEntities() -- confirmed real (Entity.c 0x00098c04), heavily
// used for level-reset/cleanup sweeps (Main.gm:8216's `e.reset()` "reset
// back to configureData" pass, Main.gm:10726's kill-bodies-on-screen-change
// pass, Main.gm:11114's wizard-mode-toggle reset pass, Main.gm:7358's
// checkpoint music-trigger registration). This registry doesn't track a
// separate manual/non-manual split (constructorSetManual is a genuine
// no-op, see its own header comment) -- was returning an always-EMPTY
// table, which is inconsistent with this SAME function table's own
// getNumNonManualEntities entry two lines below ("registry doesn't
// distinguish, so non-manual == everything"): the count said N, the
// iterator gave 0, so every one of the sweeps above silently processed
// nothing despite the level clearly having entities. Matches
// getNumNonManualEntities's own established convention instead.
int GM_CDECL gmf_EM_getAllNonManualEntities(gmThread* a_thread) {
    std::vector<gmVariable> all;
    for (auto& e : Entities().entities) all.push_back(e.second);
    PushEntityListIterator(a_thread, std::move(all));
    return GM_OK;
}
// getAllNonSimulatingEntities() -- no "simulating" (on-screen/active
// physics) distinction tracked by this registry either (see
// isOnCurrentScreen's own header comment: always "on screen"), so unlike
// non-manual there's no sibling getNum* convention to match -- stays an
// honest empty result, just now in the correct iterator SHAPE so
// `it.isValid()` returns real false instead of crashing/null-calling.
int GM_CDECL gmf_EM_emptyEntityIterator(gmThread* a_thread) {
    PushEntityListIterator(a_thread, {});
    return GM_OK;
}
int GM_CDECL gmf_EM_getAllPlayerEntities(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    std::vector<gmVariable> matches;
    for (auto& e : Entities().entities) {
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        gmVariable isPlayer = t->Get(&m, "__isPlayer");
        if (isPlayer.m_type == GM_INT && isPlayer.m_value.m_int != 0) matches.push_back(e.second);
    }
    PushEntityListIterator(a_thread, std::move(matches));
    return GM_OK;
}
// getEntitiesRoughlyWithin(minx,miny,maxx,maxy) -- confirmed real (Entity.c
// 0x000994a8: rejects an inverted box, minx>maxx or miny>maxy, with a hard
// exception return; otherwise an AABB-query container exactly like
// getAllEntities above). getEntitiesNear(x,y,rx,ry) -- confirmed real
// (Entity.c 0x00099740, same container shape) -- every corpus call site
// (EntityBase.gm:330/3116/5625: `getEntitiesNear(pos.x,pos.y,r,r)`) passes
// a centre point plus half-extents, so it's the same AABB overlap test as
// getEntitiesRoughlyWithin, just parameterized as (centre,radius) instead
// of raw (min,max) corners. Both were previously always-empty (see
// gmf_EM_emptyEntityIterator's own scope note) -- real per-entity AABB
// overlap test now, via the same PhysicsData-AABB/configureData-point
// fallback already used by isInView/isInGameBoard.
int GM_CDECL gmf_EM_getEntitiesRoughlyWithin(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    GM_CHECK_NUM_PARAMS(4);
    GM_FLOAT_OR_INT_PARAM(minX, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(minY, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(maxX, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(maxY, 3, 0.0f);
    if (minY > maxY || minX > maxX) return GM_EXCEPTION; // matches the real decompiled body's own inverted-box rejection
    std::vector<gmVariable> matches;
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        if (t && EntityOverlapsBox(m, t, minX, minY, maxX, maxY)) matches.push_back(e.second);
    }
    PushEntityListIterator(a_thread, std::move(matches));
    return GM_OK;
}
int GM_CDECL gmf_EM_getEntitiesNear(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    GM_CHECK_NUM_PARAMS(4);
    GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(rx, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(ry, 3, 0.0f);
    std::vector<gmVariable> matches;
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        if (t && EntityOverlapsBox(m, t, x - rx, y - ry, x + rx, y + ry)) matches.push_back(e.second);
    }
    PushEntityListIterator(a_thread, std::move(matches));
    return GM_OK;
}
int GM_CDECL gmf_EM_getNumEntities(gmThread* a_thread) {
    a_thread->PushInt((int)Entities().entities.size());
    return GM_OK;
}
// Confirmed real: takes a type name (e.g.
// `EntityManager.getRandomEntity("Platformer_EntityPacks_SpawnPoint")`,
// game_data/Platformer/Main.gm:7290) and picks among entities of THAT type
// only. Was previously ignoring the argument entirely and returning any
// random entity -- confirmed a real bug the hard way: Main.gm's own
// finalizeLoad() calls `e.configure(e.configureData)` a SECOND time on
// whatever getRandomEntity("...SpawnPoint") returns (the first configure
// already happened during level setup), and GroundBase.gm's endConfigure()
// nulls `this.gameProcess` after an entity's first configure() in "play"
// mode (its own "@hack: for scee build to reduce GC" comment) -- so
// returning a non-SpawnPoint entity (e.g. a ground triangle) here meant its
// second configure() crashed reading `this.gameProcess.mode` on a table
// whose gameProcess field was already nulled out, at
// Platformer/EntityPacks/TriangleGround.gm:107.
int GM_CDECL gmf_EM_getRandomEntity(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    GM_CHECK_NUM_PARAMS(1); GM_STRING_PARAM(typeName, 0, "");
    std::vector<gmVariable*> matches;
    for (auto& e : Entities().entities) {
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        gmVariable tn = t->Get(&m, "__typeName");
        if (tn.m_type == GM_STRING) {
            char buf[256];
            const char* s = tn.AsString(&m, buf, sizeof(buf));
            if (s && typeName == s) matches.push_back(&e.second);
        }
    }
    if (matches.empty()) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(*matches[(size_t)std::rand() % matches.size()]);
    return GM_OK;
}
int GM_CDECL gmf_EM_getRandomPlayer(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    std::vector<gmVariable*> matches;
    for (auto& e : Entities().entities) {
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        gmVariable isPlayer = t->Get(&m, "__isPlayer");
        if (isPlayer.m_type == GM_INT && isPlayer.m_value.m_int != 0) matches.push_back(&e.second);
    }
    if (matches.empty()) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(*matches[(size_t)std::rand() % matches.size()]);
    return GM_OK;
}
int GM_CDECL gmf_EM_getDummyEntity(gmThread* a_thread) {
    static gmVariable s_dummy = gmVariable::s_null;
    if (s_dummy.m_type != GM_TABLE) {
        s_dummy = gmVariable(a_thread->GetMachine()->AllocTableObject());
        RootGmVariable(*a_thread->GetMachine(), s_dummy);
    }
    a_thread->Push(s_dummy);
    return GM_OK;
}
int GM_CDECL gmf_EM_deleteAllEntities(gmThread* a_thread) {
    for (auto& e : Entities().entities) UnrootGmVariable(*a_thread->GetMachine(), e.second);
    Entities().entities.clear();
    return GM_OK;
}
int GM_CDECL gmf_EM_flush(gmThread* a_thread) { return GM_OK; } // additions/removals already apply immediately

gmFunctionEntry s_entityManagerLib[] = {
    {"createEntity", gmf_EM_createEntity, nullptr},
    {"createEntityAsPlayer", gmf_EM_createEntityAsPlayer, nullptr},
    {"getEntity", gmf_EM_getEntity, nullptr},
    {"getEntityFromUID", gmf_EM_getEntityFromUID, nullptr},
    {"isEntityIDValid", gmf_EM_isEntityIDValid, nullptr},
    {"setEntityUID", gmf_EM_setEntityUID, nullptr},
    {"getAllEntities", gmf_EM_getAllEntities, nullptr},
    {"getAllNonManualEntities", gmf_EM_getAllNonManualEntities, nullptr},
    {"getAllNonSimulatingEntities", gmf_EM_emptyEntityIterator, nullptr},
    {"getEntitiesNear", gmf_EM_getEntitiesNear, nullptr},
    {"getEntitiesRoughlyWithin", gmf_EM_getEntitiesRoughlyWithin, nullptr},
    {"getAllPlayerEntities", gmf_EM_getAllPlayerEntities, nullptr},
    {"getNumEntities", gmf_EM_getNumEntities, nullptr},
    {"getRandomEntity", gmf_EM_getRandomEntity, nullptr},
    {"getDummyEntity", gmf_EM_getDummyEntity, nullptr},
    {"deleteAllEntities", gmf_EM_deleteAllEntities, nullptr},
    {"flush", gmf_EM_flush, nullptr},
    // Not tracked distinctly by this registry (no manual/simulating/
    // player-vs-not bookkeeping beyond __isPlayer) -- honest zero/no-op
    // defaults rather than fabricated counts.
    {"getNumManualEntities", gmf_EM_zeroInt, nullptr},
    {"getNumNonManualEntities", gmf_EM_getNumEntities, nullptr}, // best-effort: registry doesn't distinguish, so "non-manual" == "everything"
    {"getNumPlayers", gmf_EM_zeroInt, nullptr},
    {"getRandomPlayer", gmf_EM_getRandomPlayer, nullptr},
    {"getEntityPerfCost", gmf_EM_zeroFloat, nullptr},
    {"getEntityPerfMaxCost", gmf_EM_zeroFloat, nullptr},
    {"setEntityPerfCost", gmf_EM_noop, nullptr},
};

// --- Instrument stub / Animation real type -------------------------------
// INSTRUMENT(path) -- confirmed real (`.instrument.setSampleDir/
// getSampleInfo` in BrianMover.gm and others) but entirely unregistered.
// No real audio-sample subsystem exists yet (out of scope for this pass),
// so this stays a plain no-op stand-in, just enough that entity
// construction can complete instead of throwing on a null-call.
//
// Entity.loadAnimation(file) used to be the same kind of stub (always
// zero frames, isDone() always false, no drawAnimation at all -- every
// animated entity in the corpus, e.g. Platformer/EntityPacks/
// SpawnPoint.gm's player spawner, silently drew nothing) but now backs a
// real .fsa/.bci sprite animation (animation_bindings.h/.cpp) -- see
// PushNewAnimation below.
gmType g_instrumentStubType = -1;

// Instrument.c @ 0x0019aaf0): stores the path into a fixed 255-char buffer
// on the native Instrument object, then immediately calls the same native
// helper (FUN_00199f98) as Instrument.loadSamples -- i.e. retail's
// setSampleDir is "set the dir, then load from it". No real audio-sample
// decode/output backend exists in this engine yet (out of scope for this
// pass, same as the rest of audio_bindings.cpp's state-only tracking), so
// there's nothing to actually load -- an honest no-op with the confirmed
// real arity, not a fabricated sample-loading pipeline.
int GM_CDECL gmf_Instrument_setSampleDir(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    (void)path;
    return GM_OK;
}
GE3_REMOVED(gmf_Instrument_getSampleInfo, "Instrument.getSampleInfo")

gmFunctionEntry s_instrumentStubLib[] = {
    {"setSampleDir", gmf_Instrument_setSampleDir, nullptr},
    {"getSampleInfo", gmf_Instrument_getSampleInfo, nullptr},
};

int GM_CDECL gmf_entity_loadAnimation(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    std::string resolved;
    auto r = GetEngineContext().fileHelper->Resolve(path);
    if (r) resolved = *r;
    else std::fprintf(stderr, "Entity.loadAnimation: '%s' not found\n", path);
    PushNewAnimation(a_thread, resolved);
    return GM_OK;
}
int GM_CDECL gmf_INSTRUMENT(gmThread* a_thread) {
    a_thread->PushNewUser(new int(0), g_instrumentStubType);
    return GM_OK;
}
void GM_CDECL StubGCDestruct(gmMachine*, gmUserObject* a_object) { delete (int*)a_object->m_user; }

// GAME_CalculateFillVerts(vertices, entity, extra) -- confirmed real
// (Platformer/EntityPacks/GroundResizeNoRotate.gm: `GAME_CalculateFillVerts(vertices,this,0);`,
// immediately followed by `vertices[i]=vertices[i]-myPos;` for every
// entry) but entirely unregistered, so `vertices` stayed the empty table
// it started as -- `sd.addPolygon(vertices,tableCount(vertices))`
// therefore always got called with 0 vertices, meaning every
// GroundResizeNoRotate ground piece (149 of this corpus's 225-entity
// tutorial level) had a real Box2D body with zero fixtures: present in
// the simulation, invisible.
//
// No decompiled evidence for this function specifically, but the
// surrounding script fully determines its contract: `this.ax/ay` ..
// `this.dx/dy` (the entity's own 4 rotated-rectangle corners, computed
// via MATHUTIL_Rotate just above, LOCAL to the entity centre) are already
// set on `this` by the time this is called, and the very next line
// subtracts `myPos` (`vector2D(configureData.posX,configureData.posY)`)
// from every filled entry -- so this function's real job must be
// "convert the entity's local corners to WORLD space", which the caller
// immediately converts back to local (a round-trip that only makes
// sense if this same function is shared with OTHER entity types whose
// fill computation genuinely needs world-space intermediate points, e.g.
// clipping against neighbouring geometry -- not reconstructed here, out
// of scope). Confirmed independently against the user's own separate
// JS level-viewer reference (`Level Viewer/src/Entities.js`'s
// `Ground_Entity.draw_rect()`): it draws GroundResizeNoRotate as a plain
// `posX,posY,w,h` rect rotated by `atan2(-ry,rx)`, with no separate
// "fill vertices" concept at all -- consistent with this being pure
// per-instance rectangle geometry, not a shared complex fill algorithm.
int GM_CDECL gmf_GAME_CalculateFillVerts(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmVariable verticesVar = a_thread->Param(0);
    gmVariable entityVar = a_thread->Param(1);
    if (verticesVar.m_type != GM_TABLE || entityVar.m_type != GM_TABLE) return GM_OK;
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* vertices = (gmTableObject*)m.GetObject(verticesVar.m_value.m_ref);
    gmTableObject* entity = (gmTableObject*)m.GetObject(entityVar.m_value.m_ref);

    auto getf = [&](gmTableObject* t, const char* key) -> float {
        gmVariable v = t->Get(&m, key);
        if (v.m_type == GM_FLOAT) return v.m_value.m_float;
        if (v.m_type == GM_INT) return (float)v.m_value.m_int;
        return 0.0f;
    };

    float posX = 0.0f, posY = 0.0f;
    gmVariable cdVar = entity->Get(&m, "configureData");
    if (cdVar.m_type == GM_TABLE) {
        gmTableObject* cd = (gmTableObject*)m.GetObject(cdVar.m_value.m_ref);
        posX = getf(cd, "posX");
        posY = getf(cd, "posY");
    }

    const char* xKeys[4] = {"ax", "bx", "cx", "dx"};
    const char* yKeys[4] = {"ay", "by", "cy", "dy"};
    for (int i = 0; i < 4; ++i) {
        float x = getf(entity, xKeys[i]) + posX;
        float y = getf(entity, yKeys[i]) + posY;
        vertices->Set(&m, i, MakeVector2D(m, x, y));
    }
    return GM_OK;
}

// GAME_CalculateTriangleFillVerts(vertices, entity, extra) -- same
// situation and same fix as GAME_CalculateFillVerts above, for
// Platformer/EntityPacks/TriangleGround.gm's own 3-point
// (`this.ax/ay,bx/by,cx/cy`) variant (34 of this corpus's 225-entity
// tutorial level) -- was likewise entirely unregistered, leaving every
// triangle ground piece with a real, zero-fixture, invisible body.
int GM_CDECL gmf_GAME_CalculateTriangleFillVerts(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmVariable verticesVar = a_thread->Param(0);
    gmVariable entityVar = a_thread->Param(1);
    if (verticesVar.m_type != GM_TABLE || entityVar.m_type != GM_TABLE) return GM_OK;
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* vertices = (gmTableObject*)m.GetObject(verticesVar.m_value.m_ref);
    gmTableObject* entity = (gmTableObject*)m.GetObject(entityVar.m_value.m_ref);

    auto getf = [&](gmTableObject* t, const char* key) -> float {
        gmVariable v = t->Get(&m, key);
        if (v.m_type == GM_FLOAT) return v.m_value.m_float;
        if (v.m_type == GM_INT) return (float)v.m_value.m_int;
        return 0.0f;
    };

    float posX = 0.0f, posY = 0.0f;
    gmVariable cdVar = entity->Get(&m, "configureData");
    if (cdVar.m_type == GM_TABLE) {
        gmTableObject* cd = (gmTableObject*)m.GetObject(cdVar.m_value.m_ref);
        posX = getf(cd, "posX");
        posY = getf(cd, "posY");
    }

    const char* xKeys[3] = {"ax", "bx", "cx"};
    const char* yKeys[3] = {"ay", "by", "cy"};
    for (int i = 0; i < 3; ++i) {
        float x = getf(entity, xKeys[i]) + posX;
        float y = getf(entity, yKeys[i]) + posY;
        vertices->Set(&m, i, MakeVector2D(m, x, y));
    }
    return GM_OK;
}

// classes/Game.c 0x001ae784/0x001ae7f8), each returning the engine's single
// persistent pre/post-draw DrawList. Only real call site (Main.gm's
// GAME_PostDraw) caches the result in a global the first time it's called
// and reuses it forever (`if (!?GAME_PostDraw_drawList) { GAME_PostDraw_
// drawList=GAME_GetPostDrawList(); }`), so a fresh DrawList handle per call
// matches the only observed usage pattern; size args are irrelevant since
// every real caller calls setupWorldView()/reset() on the list itself
// right after fetching it.
// ENTITYLISTREFCONTAINER()/INTLISTREFCONTAINER() -- confirmed real
// 0x00069420: native doubly-linked-list container constructors, no other
// methods recovered in the decompiled corpus). Real call sites (Main.gm:167
// `this.tmpEntityIterator=ENTITYLISTREFCONTAINER();`, Player.gm:72/74
// `this.tmpIntIterator=INTLISTREFCONTAINER(); this.
// isSomethingImpeding_tmpIntIterator=INTLISTREFCONTAINER();`) only ever
// store the result on a field -- grepping the whole corpus for
// `tmpIntIterator.`/`tmpEntityIterator.` finds zero further method calls on
// either, so there's no observed method-call contract to replicate. A
// plain empty table satisfies every real call site (assignment only) with
// no invented API surface.
int GM_CDECL gmf_ENTITYLISTREFCONTAINER(gmThread* a_thread) {
    a_thread->PushNewTable();
    return GM_OK;
}
int GM_CDECL gmf_INTLISTREFCONTAINER(gmThread* a_thread) {
    a_thread->PushNewTable();
    return GM_OK;
}

// GAME_SetSelectionPulseRate(rate)/GAME_SetSelectionAlphaPerc(perc)/
// Game.c 0x001ae15c/0x001ae22c/0x001ae1d8), real call sites all in
// TouchEditProcessPS3.gm's constructor/edit-mode toggle (`GAME_
// SetSelectionAlphaPerc(1.0);` at construction, `GAME_SetSelectionPulseRate
// (...)`/`GAME_SetIsSelectionPulseEnabled(...)` when entering/leaving
// selection mode) -- configure a native pulsing-highlight shader effect
// for the currently-selected editor entity. This reimplementation's
// renderer has no such fragment-program-driven selection-pulse effect
// (see GAME_SetIsUsingSelectionFragmentProgram/GAME_SetSelectionPulseRate's
// own neighboring entries in Game.c, none of which this engine models),
// genuine no-ops.
int GM_CDECL gmf_GAME_SetSelectionPulseRate(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_GAME_SetSelectionAlphaPerc(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_GAME_SetIsSelectionPulseEnabled(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// GAME_SetIsUsingSelectionFragmentProgram(bool, entity) -- confirmed real
// as the three above, 2 args this time (enabled flag, target entity).
// Real call site (TouchEditProcessBase.gm:2096's addToSelected():
// `GAME_SetIsUsingSelectionFragmentProgram(true,e);`) toggles the
// selection-highlight shader on/off per-entity -- same "no fragment-
// program-driven selection effect modeled" no-op as its siblings.
int GM_CDECL gmf_GAME_SetIsUsingSelectionFragmentProgram(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

int GM_CDECL gmf_GAME_GetPreDrawList(gmThread* a_thread) {
    PushNewDrawList(a_thread, 0, 0);
    return GM_OK;
}
int GM_CDECL gmf_GAME_GetPostDrawList(gmThread* a_thread) {
    PushNewDrawList(a_thread, 0, 0);
    return GM_OK;
}

} // namespace

gmVariable FindPlayerEntity(gmMachine& a_machine) {
    for (auto& e : Entities().entities) {
        gmTableObject* t = (gmTableObject*)a_machine.GetObject(e.second.m_value.m_ref);
        gmVariable isPlayer = t->Get(&a_machine, "__isPlayer");
        if (isPlayer.m_type == GM_INT && isPlayer.m_value.m_int != 0) return e.second;
    }
    return gmVariable::s_null;
}

std::vector<EntityDebugInfo> GetAllEntityDebugPositions() {
    std::vector<EntityDebugInfo> out;
    gmMachine* m = GetEngineContext().machine;
    if (!m) return out;
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;

        float x = 0.0f, y = 0.0f;
        bool hasPos = false;
        gmVariable cdVar = t->Get(m, "configureData");
        if (cdVar.m_type == GM_TABLE) {
            gmTableObject* cd = (gmTableObject*)m->GetObject(cdVar.m_value.m_ref);
            gmVariable xv = cd->Get(m, "posX");
            gmVariable yv = cd->Get(m, "posY");
            auto tofloat = [](const gmVariable& v, float def) -> float {
                if (v.m_type == GM_FLOAT) return v.m_value.m_float;
                if (v.m_type == GM_INT) return (float)v.m_value.m_int;
                return def;
            };
            if (xv.m_type == GM_FLOAT || xv.m_type == GM_INT || yv.m_type == GM_FLOAT || yv.m_type == GM_INT) {
                hasPos = true;
                x = tofloat(xv, 0.0f);
                y = tofloat(yv, 0.0f);
            }
        }

        gmVariable typeNameVar = t->Get(m, "__typeName");
        std::string typeName = (typeNameVar.m_type == GM_STRING && typeNameVar.m_value.m_ref)
            ? ((gmStringObject*)m->GetObject(typeNameVar.m_value.m_ref))->GetString()
            : std::string();

        gmVariable isPlayerVar = t->Get(m, "__isPlayer");
        bool isPlayer = (isPlayerVar.m_type == GM_INT && isPlayerVar.m_value.m_int != 0);

        out.push_back({e.first, typeName, x, y, isPlayer, hasPos});
    }
    return out;
}

// Native port of EntityPacks/SauceBase.gm's own commented-out `t.draw`
// (that file, verbatim: "/*t.draw=function() { This is now in C - and I
// keep forgetting... ... */") -- confirmed real, live, dead script code:
// every "sauce base" decorative entity (background bushes/trees/clouds
// built via Platformer_EntityPacks_SauceBase_Install, e.g. EntityPacks/
// VicCloudBG.gm) never gets a `draw` GM_FUNCTION assigned at all, so
// DrawEntitiesWithNativeDrawHook's own `t->Get(m,"draw").m_type !=
// GM_FUNCTION` check skipped them entirely -- confirmed live, missing from
// retail_screenshots/"level1 screen1.png" clearly shows two clouds; this
// engine drew none). Ported field-for-field from the commented pseudocode:
// pushMatrix/translate(pos,z)/rotateZ(dir), then per idle-or-pulse
// animData entry either the bottomLeftRegistration_pos branch or
// ENTITYBASE_DrawAnimationCentered's own real body (EntityBase.gm:237-244:
// pushMatrix/translate(x-w/2,y-h/2,z)/scale(w,h)/SetAnimationPaletteColour/
// drawAnimation/popMatrix), popMatrix. Runs immediately (composes the
// transform in local floats via DrawContext::Mat/DC_Multiply, calls
// DrawAnimationWithTransform directly) rather than deferring onto
// this.drawList's command queue -- this whole hook already runs at the
// exact point in the frame real draw() calls would have, once per frame,
// so there's nothing to defer FOR (unlike COLOURSCHEME_SetColour's
// drawList-target fix elsewhere in this engine, which has to survive being
// called during a separate, earlier RECORDING pass).
void DrawSauceBaseEntityNative(gmMachine& m, gmTableObject* t, const gmVariable& a_entityVar) {
    gmVariable mainpbVar = t->Get(&m, "mainpb");
    float posX = 0.0f, posY = 0.0f, dirX = 1.0f, dirY = 0.0f;
    if (!GetPhysicsBodyPosAndDir(m, mainpbVar, posX, posY, dirX, dirY)) return;
    float z = 0.0f;
    gmVariable zVar = t->Get(&m, "z");
    if (zVar.m_type == GM_FLOAT) z = zVar.m_value.m_float;
    else if (zVar.m_type == GM_INT) z = (float)zVar.m_value.m_int;

    bool pulsePlaying = false;
    gmVariable pulseDataVar = t->Get(&m, "pulseAnimData");
    if (pulseDataVar.m_type == GM_TABLE) {
        gmTableObject* pulseData = (gmTableObject*)m.GetObject(pulseDataVar.m_value.m_ref);
        for (int i = 0; pulseData; ++i) {
            gmVariable entryVar = pulseData->Get(i);
            if (entryVar.m_type != GM_TABLE) break;
            gmTableObject* entry = (gmTableObject*)m.GetObject(entryVar.m_value.m_ref);
            gmVariable animVar = entry->Get(&m, "anim");
            if (IsAnimationPlayingNow(m, animVar)) { pulsePlaying = true; break; }
        }
    }
    gmVariable animDataVar = t->Get(&m, pulsePlaying ? "pulseAnimData" : "idleAnimData");
    if (animDataVar.m_type != GM_TABLE) return;
    gmTableObject* animData = (gmTableObject*)m.GetObject(animDataVar.m_value.m_ref);
    if (!animData) return;

    bool haveBLR = false;
    float blrX = 0.0f, blrY = 0.0f;
    gmVariable blrVar = t->Get(&m, "bottomLeftRegistration_pos");
    if (blrVar.m_type != GM_NULL) haveBLR = GetVector2D(m, blrVar, blrX, blrY);

    auto tofloat = [](const gmVariable& v, float def) -> float {
        if (v.m_type == GM_FLOAT) return v.m_value.m_float;
        if (v.m_type == GM_INT) return (float)v.m_value.m_int;
        return def;
    };

    // Entity placement: pushMatrix (identity base); translate(pos,z); rotateZ(dir).
    DrawContext::Mat placement;
    DrawContext::Mat t1; t1.e = posX; t1.f = posY; t1.z = z;
    placement = DC_Multiply(placement, t1);
    DrawContext::Mat rot; rot.a = dirX; rot.b = dirY; rot.c = -dirY; rot.d = dirX;
    placement = DC_Multiply(placement, rot);

    for (int i = 0; ; ++i) {
        gmVariable entryVar = animData->Get(i);
        if (entryVar.m_type != GM_TABLE) break;
        gmTableObject* entry = (gmTableObject*)m.GetObject(entryVar.m_value.m_ref);
        if (!entry) continue;

        gmVariable animVar = entry->Get(&m, "anim");
        float ox = tofloat(entry->Get(&m, "ofsX"), 0.0f);
        float oy = tofloat(entry->Get(&m, "ofsY"), 0.0f);
        float zofs = tofloat(entry->Get(&m, "zofs"), 0.0f);
        float w = tofloat(entry->Get(&m, "animWidth"), 0.0f);
        float h = tofloat(entry->Get(&m, "animHeight"), 0.0f);

        std::string c1, c2;
        gmVariable c1Var = entry->Get(&m, "colour1");
        gmVariable c2Var = entry->Get(&m, "colour2");
        if (c1Var.m_type == GM_STRING) c1 = ((gmStringObject*)m.GetObject(c1Var.m_value.m_ref))->GetString();
        if (c2Var.m_type == GM_STRING) c2 = ((gmStringObject*)m.GetObject(c2Var.m_value.m_ref))->GetString();
        float fillR = 1, fillG = 1, fillB = 1, outR = 1, outG = 1, outB = 1;
        if (!c1.empty()) GetNamedColourRGB(c1, fillR, fillG, fillB);
        if (!c2.empty()) GetNamedColourRGB(c2, outR, outG, outB);
        // ResolveAnimColour's real param order is (outColourName, fillColourName)
        // -- see colour_bindings.cpp's own gmf_COLOURSCHEME_SetAnimationPaletteColour --
        // but SauceBase's colour1/colour2 map the OPPOSITE way (colour1=fill,
        // matching EntityBase.gm:241's `COLOURSCHEME_SetAnimationPaletteColour(
        // anim,cn1,cn2)` where cn1 is literally this entry's own "colour1"),
        // so pass (out=colour2, fill=colour1) to match SetAnimationPaletteColour's
        // own (out,fill) parameter order.
        SetAnimationPaletteColour(m, animVar, outR, outG, outB, fillR, fillG, fillB);
        // The animation's OWN texture already bakes fillR/G/B (and outR/G/B)
        // straight into its RGBA pixels (RebuildTextures, animation_bindings.cpp)
        // -- DrawTexturedQuad's plain-quad shader still multiplies by the
        // CURRENT shared draw colour on top of that (`texture * uColour`,
        // renderer.cpp), same as every other textured draw. Real draw()
        // calls always go through drawList.reset() first (renderer colour
        // -> white) before anything draws; this native hook runs standalone,
        // outside that reset/replay cycle, so without an explicit white
        // reset here whatever colour was last left set by some UNRELATED
        // draw call earlier in the frame (ground/vine/danger tints, etc.)
        // silently multiplied back into this already-correctly-tinted
        // texture -- confirmed live: clouds rendered with a wrong orange
        // tint bleeding in from leftover draw-colour state, despite their
        // own fill/out resolving to the correct flat cream every time.
        GetEngineContext().renderer->SetColour(1.0f, 1.0f, 1.0f, 1.0f);

        DrawContext::Mat entryMat;
        if (haveBLR) {
            DrawContext::Mat tr; tr.e = blrX; tr.f = blrY; tr.z = zofs;
            entryMat = DC_Multiply(placement, tr);
            DrawContext::Mat sc; sc.a = w; sc.d = h;
            entryMat = DC_Multiply(entryMat, sc);
        } else {
            DrawContext::Mat tr; tr.e = ox - w / 2.0f; tr.f = oy - h / 2.0f; tr.z = zofs;
            entryMat = DC_Multiply(placement, tr);
            DrawContext::Mat sc; sc.a = w; sc.d = h;
            entryMat = DC_Multiply(entryMat, sc);
        }
        DrawAnimationWithTransform(m, animVar, entryMat.a, entryMat.b, entryMat.c, entryMat.d,
                                    entryMat.e, entryMat.f, entryMat.z);
    }
    (void)a_entityVar;
}

std::vector<int> DrawEntitiesWithNativeDrawHook() {
    std::vector<int> drawn;
    gmMachine* m = GetEngineContext().machine;
    if (!m) return drawn;
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        // ENGINE PATCH REVERTED (2026-08-17): a same-day earlier pass gated
        // this loop on isEnabled() to stop MarshmallowTree/GoAroundGuy/
        // SwayingTree's null-mainpb crash spam (see UpdateEntitiesWith
        // NativeUpdateHook's still-live version of this comment for the
        // full original diagnosis). That gate was too broad: confirmed live
        // regression -- with it in place, NEITHER the player NOR its spawn
        // point rendered at all, on the very first frame of level 1. Not
        // every legitimately-visible entity in this corpus ever calls
        // setIsEnabled(true) itself (the player/spawn point apparently
        // don't), so "isEnabled() defaults true when never set" doesn't
        // hold universally the way the entity-flag doc comments elsewhere
        // in this file assumed -- something in the real init chain for
        // those types leaves __enabled explicitly false without this
        // engine's simplified isOnCurrentScreen()/registerEntity flow ever
        // flipping it back, unlike ground entities. Reverted rather than
        // chasing the player/spawn-specific enable path under time
        // pressure -- the null-mainpb crash this was guarding against is
        // real but narrower in impact (visual/perf on specific decorative
        // entity types) than "nothing in the level renders" is fatal.
        if (t->Get(m, "draw").m_type != GM_FUNCTION) {
            // See DrawSauceBaseEntityNative's own header comment -- SauceBase-
            // family entities (background clouds/bushes/trees) never get a
            // real `draw` GM_FUNCTION at all (their reference implementation
            // is real, live, dead script code, moved to native on retail),
            // so without this they were silently skipped entirely.
            gmVariable sauceFlag = t->Get(m, "__sauceBase");
            if (sauceFlag.m_type == GM_INT && sauceFlag.m_value.m_int != 0) {
                DrawSauceBaseEntityNative(*m, t, e.second);
                drawn.push_back(e.first);
            }
            continue;
        }

        gmCall call;
        if (call.BeginTableFunction(m, "draw", t, e.second)) {
            call.End();
        }
        // A real drawList instance now records rather than draws
        // immediately (see draw_list.h's RecordingTarget/DrawListData
        // header comment) -- an entity's own `this.drawList` (set up via
        // setupDrawList(), 214 real call sites all use this exact field
        // name) is exactly that kind of instance, so whatever draw() just
        // recorded onto it needs an explicit replay here, the same way
        // ProcessManager::PreDrawAll/PostDrawAll flush a process's own
        // preDrawList/postDrawList. Re-fetched by uid rather than reusing
        // `t`/`e.second`: draw() can run arbitrary script, and this whole
        // loop is iterating Entities().entities live.
        gmVariable* freshVar = FindEntity(e.first);
        if (freshVar && freshVar->m_type == GM_TABLE) {
            gmTableObject* freshTable = (gmTableObject*)m->GetObject(freshVar->m_value.m_ref);
            gmVariable listVar = freshTable ? freshTable->Get(m, "drawList") : gmVariable::s_null;
            if (listVar.m_type >= GM_USER) { // see process_manager.cpp's FlushDrawList for why not `==`
                gmUserObject* obj = (gmUserObject*)m->GetObject(listVar.m_value.m_ref);
                if (obj && obj->m_userType == GetDrawListType()) {
                    ReplayDrawList((DrawListData*)obj->m_user);
                }
            }
        }
        drawn.push_back(e.first);
    }
    return drawn;
}

// See this function's declaration (entity_bindings.h) for the Ghidra
// evidence that drawAlpha is a real, separate per-entity hook. Same
// snapshot-first/drawList-replay structure as DrawEntitiesWithNativeDrawHook
// above, just calling "drawAlpha" instead of "draw" -- and with NO native
// fallback for entities that don't define one (retail's own drawAlpha
// wrapper falls back to a bare `return;`, not a default shape), so entities
// with no drawAlpha are simply skipped rather than drawing a placeholder.
std::vector<int> DrawEntitiesWithNativeDrawAlphaHook() {
    std::vector<int> drawn;
    gmMachine* m = GetEngineContext().machine;
    if (!m) return drawn;
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        if (t->Get(m, "drawAlpha").m_type != GM_FUNCTION) continue;

        gmCall call;
        if (call.BeginTableFunction(m, "drawAlpha", t, e.second)) {
            call.End();
        }
        // Same drawList-replay requirement as the draw() pass above --
        // drawAlpha() bodies use the identical `this.drawList`/Video.*
        // immediate-mode calls (PekkoAndJon/Player.gm's drawAlpha:
        // Video.pushMatrix/translate/scale/setColour/popMatrix directly,
        // no separate drawList in this corpus, but other real drawAlpha
        // implementations could use one the same way draw() does).
        gmVariable* freshVar = FindEntity(e.first);
        if (freshVar && freshVar->m_type == GM_TABLE) {
            gmTableObject* freshTable = (gmTableObject*)m->GetObject(freshVar->m_value.m_ref);
            gmVariable listVar = freshTable ? freshTable->Get(m, "drawList") : gmVariable::s_null;
            if (listVar.m_type >= GM_USER) {
                gmUserObject* obj = (gmUserObject*)m->GetObject(listVar.m_value.m_ref);
                if (obj && obj->m_userType == GetDrawListType()) {
                    ReplayDrawList((DrawListData*)obj->m_user);
                }
            }
        }
        drawn.push_back(e.first);
    }
    return drawn;
}

// ConfigureUnconfiguredEntities() -- the real fix for the null-mainpb
// crash spam this session's earlier isEnabled()-gate attempt tried (and
// failed) to paper over. Root cause: Main.gm's level-load loop only calls
// `e.configure(cd)` directly for ground entities (or ones with a
// childEntityList/parentEntity) -- every other placed entity (decorative
// props like MarshmallowTree/SwayingTree/GoAroundGuy, but ALSO, confirmed
// live, the player and its spawn point) is left with configureData set but
// configure() never invoked, relying on retail's real lazy "configure once
// it scrolls on screen" system to finish the job later. This
// reimplementation doesn't have that system (isOnCurrentScreen() is
// conservatively stubbed to always return true instead -- see its own
// header comment for why), so nothing was ever driving that second half.
// gmf_entity_setIsEnabled's own new lazy-configure hook (above) covers the
// case where SOMETHING does eventually call setIsEnabled(true) on an
// unconfigured entity, but for entities where NOTHING ever does (since
// registerEntity, the thing that calls setIsEnabled(true), only runs from
// inside configure()'s own tail -- a chicken-and-egg gap for anything
// configure() was never called on in the first place), that hook never
// fires at all. This sweep is the other half: once per frame, force-
// configure (and enable) any entity that's had configureData set since
// last frame but still isn't __nativeConfigured, matching this whole
// engine's established "always visible/on-screen" simplification (see
// isOnCurrentScreen's own comment) consistently instead of only some of
// the time. `configuring` (in-flight) entities are skipped naturally since
// __nativeConfigured only flips true after the trampoline's call returns --
// no risk of double-configuring an entity whose configure() Main.gm is
// literally in the middle of calling in the same frame this sweep runs.
void ConfigureUnconfiguredEntities() {
    gmMachine* m = GetEngineContext().machine;
    if (!m) return;
    std::vector<std::pair<int, gmVariable>> snapshot = Entities().entities;
    for (auto& e : snapshot) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;

        gmVariable configuredVar = t->Get(m, "__nativeConfigured");
        if (configuredVar.m_type == GM_INT && configuredVar.m_value.m_int != 0) continue;
        gmVariable inFlightVar = t->Get(m, "__nativeConfiguring");
        if (inFlightVar.m_type == GM_INT && inFlightVar.m_value.m_int != 0) continue;

        gmVariable cdVar = t->Get(m, "configureData");
        if (cdVar.m_type != GM_TABLE) continue; // nothing to configure with yet
        gmVariable configureFn = t->Get(m, "configure");
        if (configureFn.m_type != GM_FUNCTION) continue; // no configure() at all, e.g. non-ENTITYBASE types

        gmVariable selfVar(t);
        gmCall call;
        if (call.BeginTableFunction(m, "configure", t, selfVar)) {
            call.AddParam(cdVar);
            call.End();
        }
        // Matches this engine's "always on screen" simplification: a real
        // lazy-configure system would enable the entity at the same moment
        // it configures it (they're the same "just scrolled into view"
        // event in retail); do the same here rather than leaving a
        // configured-but-still-disabled entity that nothing else will ever
        // flip on.
        gmVariable enabledVar = t->Get(m, "__enabled");
        if (!(enabledVar.m_type == GM_INT && enabledVar.m_value.m_int != 0)) {
            t->Set(m, "__enabled", gmVariable((gmint)1));
        }
    }
}

// Calls entity.update(msElapsed) on every registered entity every frame --
// the update-side counterpart to DrawEntitiesWithNativeDrawHook above.
// Confirmed necessary the same way postDraw/onKeyRelease were: nothing else
// in this codebase ever calls it. GameMonkey scripts drive their OWN
// per-type state machines from inside `update` (e.g.
// EntityPacks/SpawnPoint.gm's `update=function(msElapsed){ ...
// this.state.update(msElapsed); }`, which is what actually advances a
// spawn/trigger/idle FSA once fsa/bci playback has been kicked off by
// `play()`/`playOnce()`) -- with this never invoked, EVERY entity's update
// was silently dead code: no animation ever advanced, no per-entity timer
// ever ticked, regardless of how correctly its own state-machine logic was
// written. `getAllNonManualEntities()` (the real retail accessor scripts
// would iterate for this) is a known-unimplemented stub here (always empty,
// see its own comment) so entities can't self-organize a script-side update
// loop either -- this native dispatch is the only path left standing in.
void UpdateEntitiesWithNativeUpdateHook(float a_msElapsed) {
    gmMachine* m = GetEngineContext().machine;
    if (!m) return;
    ConfigureUnconfiguredEntities();
    // Snapshot first: an entity's own update() commonly creates/destroys
    // other entities (spawning, death, projectiles, ...), which would
    // reallocate/mutate Entities().entities out from under a live range-for.
    std::vector<std::pair<int, gmVariable>> snapshot = Entities().entities;
    for (auto& e : snapshot) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        // ENGINE PATCH REVERTED (2026-08-17): see DrawEntitiesWithNative
        // DrawHook's matching comment -- the isEnabled() gate this used to
        // have broke player/spawn-point rendering AND updating entirely
        // (confirmed live: neither appeared on level 1's first frame).
        // Reverted for the same reason.
        if (t->Get(m, "update").m_type != GM_FUNCTION) continue;

        gmCall call;
        if (call.BeginTableFunction(m, "update", t, e.second)) {
            call.AddParamFloat(a_msElapsed);
            call.End();
        }
    }
}

void DispatchKeyEventToEntities(int a_keycode, bool a_isPress) {
    if (!GetEngineContext().gameSystem.isInputEnabled) return; // see game_system_state.h
    gmMachine* m = GetEngineContext().machine;
    if (!m) return;
    const char* methodName = a_isPress ? "onKeyPress" : "onKeyRelease";
    // Snapshot first, same reentrancy hazard as the update hook above --
    // e.g. jump can spawn particles/change entity registration mid-call.
    std::vector<std::pair<int, gmVariable>> snapshot = Entities().entities;
    for (auto& e : snapshot) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        if (t->Get(m, methodName).m_type != GM_FUNCTION) continue;

        gmCall call;
        if (call.BeginTableFunction(m, methodName, t, e.second)) {
            call.AddParamInt(a_keycode);
            call.End();
        }
    }
}

void RegisterEntityBindings(gmMachine& a_machine) {
    RegisterAnimationBindings(a_machine);

    g_instrumentStubType = a_machine.CreateUserType("Instrument");
    a_machine.RegisterTypeLibrary(g_instrumentStubType, s_instrumentStubLib, sizeof(s_instrumentStubLib) / sizeof(s_instrumentStubLib[0]));
    a_machine.RegisterUserCallbacks(g_instrumentStubType, nullptr, StubGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("INSTRUMENT", gmf_INSTRUMENT);
    a_machine.RegisterLibraryFunction("ENTITY_GetType", gmf_ENTITY_GetType);
    a_machine.RegisterLibraryFunction("ENTITYBASE_SetMixerData", gmf_ENTITYBASE_SetMixerData);
    a_machine.RegisterLibraryFunction("ENTITYBASE_ClearMixer", gmf_ENTITYBASE_ClearMixer);
    g_mixerParamsType = a_machine.CreateUserType("MixerParams");
    a_machine.RegisterTypeLibrary(g_mixerParamsType, s_mixerParamsLib, sizeof(s_mixerParamsLib) / sizeof(s_mixerParamsLib[0]));
    a_machine.RegisterUserCallbacks(g_mixerParamsType, nullptr, MixerParamsGCDestruct, nullptr);
    g_telescopeType = a_machine.CreateUserType("Telescope");
    a_machine.RegisterTypeLibrary(g_telescopeType, s_telescopeLib, sizeof(s_telescopeLib) / sizeof(s_telescopeLib[0]));
    a_machine.RegisterUserCallbacks(g_telescopeType, nullptr, TelescopeGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("ENTITYBASE_GetMixerParams", gmf_ENTITYBASE_GetMixerParams);
    a_machine.RegisterLibraryFunction("ENTITY_GetMixerIDFromType", gmf_ENTITY_GetMixerIDFromType);
    a_machine.RegisterLibraryFunction("GAME_CalculateFillVerts", gmf_GAME_CalculateFillVerts);
    a_machine.RegisterLibraryFunction("GAME_CalculateTriangleFillVerts", gmf_GAME_CalculateTriangleFillVerts);
    a_machine.RegisterLibraryFunction("GAME_GetPreDrawList", gmf_GAME_GetPreDrawList);
    a_machine.RegisterLibraryFunction("GAME_GetPostDrawList", gmf_GAME_GetPostDrawList);
    a_machine.RegisterLibraryFunction("GAME_SetSelectionPulseRate", gmf_GAME_SetSelectionPulseRate);
    a_machine.RegisterLibraryFunction("GAME_SetSelectionAlphaPerc", gmf_GAME_SetSelectionAlphaPerc);
    a_machine.RegisterLibraryFunction("GAME_SetIsSelectionPulseEnabled", gmf_GAME_SetIsSelectionPulseEnabled);
    a_machine.RegisterLibraryFunction("GAME_SetIsUsingSelectionFragmentProgram", gmf_GAME_SetIsUsingSelectionFragmentProgram);
    a_machine.RegisterLibraryFunction("ENTITYLISTREFCONTAINER", gmf_ENTITYLISTREFCONTAINER);
    a_machine.RegisterLibraryFunction("INTLISTREFCONTAINER", gmf_INTLISTREFCONTAINER);

    // Real ENTITYLISTREFCONTAINER-shaped iterator every EntityManager.
    // getAllXXXEntities()/getEntitiesNear()/getEntitiesRoughlyWithin() call
    // returns -- see PushEntityListIterator's own header comment for why
    // this replaced a bare gm_table().
    g_entityListIterType = a_machine.CreateUserType("EntityListIterator");
    a_machine.RegisterTypeLibrary(g_entityListIterType, s_entityListIterLib, sizeof(s_entityListIterLib) / sizeof(s_entityListIterLib[0]));
    a_machine.RegisterUserCallbacks(g_entityListIterType, nullptr, EntityListIterGCDestruct, nullptr);

    a_machine.RegisterLibrary(s_entityManagerLib, sizeof(s_entityManagerLib) / sizeof(s_entityManagerLib[0]), "EntityManager");

    // Not decompiled-confirmed (a plain constant, no function body to
    // recover) -- a large offset avoids collision with ENTITY_GetType's
    // own 0-based auto-incrementing IDs, which is the only real constraint
    // implied by the name and real usage (`MIXER_AUTO_ID_START+
    // ENTITY_GetType(mixerName)`).
    a_machine.GetGlobals()->Set(&a_machine, "MIXER_AUTO_ID_START", gmVariable((gmint)100000));
}
