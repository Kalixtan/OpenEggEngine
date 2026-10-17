#include <cstring>
#include <cstdint>
#include "gmThread.h"
#include "gmCall.h"
#include "entity_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "gmUserObject.h"
#include "physics_body_bindings.h"
#include "../core/physics_world.h" // GetPhysicsWorld, for Entity.addTelescope's real collision body
#include <box2d/box2d.h>
#include "vector2d_bindings.h"
#include "foe_compat_bindings.h"
#include "../core/process_manager.h" // RootGmVariable/UnrootGmVariable
#include "../core/engine_context.h"
#include "../ported_native/ground_math.h"
#include "../core/strict_calls.h"
#include "../render/draw_list.h" // PushNewDrawList, for Entity.setupDrawList
#include "../render/draw_context.h" // DrawContext::Mat/DC_Multiply, for the native SauceBase draw hook
#include "animation_bindings.h" // PushNewAnimation, RegisterAnimationBindings, for Entity.loadAnimation
#include "colour_bindings.h" // GetNamedColourRGB, for DrawPlayerNative
#include "audio_bindings.h" // PlaySoundNative, for Entity.playSound
#include "../ported_native/player_renderer.h" // native player draw lifecycle
#include <GL/glew.h> // GL_TRIANGLE_FAN, for DrawPlayerNative
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {

// GAME_GetPreDrawList/GAME_GetPostDrawList return the two retained native
// GameSystem DrawLists (Game.c 0x1ae784/0x1ae7f8 load GameSystem+0x8310 and
// +0x8314, then return that same pointer on every call). Keep one GM wrapper
// for each singleton and root it while the machine lives, so repeated native
// calls preserve object identity and cannot be collected between calls.
gmMachine* s_gameDrawListsMachine = nullptr;
gmVariable s_gamePreDrawList = gmVariable::s_null;
gmVariable s_gamePostDrawList = gmVariable::s_null;

void PushPersistentGameDrawList(gmThread* a_thread, gmVariable& a_list) {
    gmMachine& machine = *a_thread->GetMachine();
    if (s_gameDrawListsMachine != &machine) {
        s_gameDrawListsMachine = &machine;
        s_gamePreDrawList = gmVariable::s_null;
        s_gamePostDrawList = gmVariable::s_null;
    }
    if (a_list.m_type == GM_NULL) {
        PushNewDrawList(a_thread, 0, 0);
        a_list = *(a_thread->GetTop() - 1);
        RootGmVariable(machine, a_list);
    } else {
        a_thread->Push(a_list);
    }
}

int GM_CDECL gmf_EM_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_EM_zeroFloat(gmThread* a_thread) { a_thread->PushFloat(0.0f); return GM_OK; }
int GM_CDECL gmf_EM_noop(gmThread* a_thread) { return GM_OK; }

struct EntityPerfCost { gmint firstInstance = 0; gmint additionalInstances = 0; };
std::unordered_map<std::string, EntityPerfCost>& EntityPerfCosts() {
    static std::unordered_map<std::string, EntityPerfCost> costs;
    return costs;
}

// FUN_00186af4 enables the retail teardown guard while bulk process-reset
// destructors run. FUN_000bfdb0 records native objects as their native
// destructors begin; Entity.tagForDeletion (FUN_0009b0a0) suppresses its
// virtual deletion call for an object already in that set. The portable
// entity table reference is the stable identity available across the script
// destructor and native PhysicsData teardown phases.
std::set<int>& NativeEntityTeardownRefs() {
    static std::set<int> refs;
    return refs;
}

class ScopedNativeEntityTeardown {
public:
    ScopedNativeEntityTeardown(gmVariable a_entity, ProcessManager* a_processes) {
        if (!a_processes || !a_processes->IsResettingProcessList() ||
            a_entity.m_type != GM_TABLE) return;
        m_ref = a_entity.m_value.m_ref;
        m_inserted = NativeEntityTeardownRefs().insert(m_ref).second;
    }

    ~ScopedNativeEntityTeardown() {
        if (m_inserted) NativeEntityTeardownRefs().erase(m_ref);
    }

private:
    int m_ref = -1;
    bool m_inserted = false;
};

// Retail FUN_000bfdb0 releases the entity's animation pointer vector after
// its native teardown. Each entry is first detached by FUN_000b9dcc; clearing
// this GM-side vector drops the entity-owned references without forcing the
// still-referenced Animation object's resources to unload.
void DetachEntityOwnedAnimations(gmMachine& a_machine, const gmVariable& a_entity) {
    if (a_entity.m_type != GM_TABLE) return;
    gmTableObject* entity = (gmTableObject*)a_machine.GetObject(a_entity.m_value.m_ref);
    if (!entity) return;
    gmVariable listVar = entity->Get(&a_machine, "__nativeOwnedAnimations");
    if (listVar.m_type == GM_TABLE) {
        gmTableObject* list = (gmTableObject*)a_machine.GetObject(listVar.m_value.m_ref);
        if (list) {
            std::vector<gmVariable> animations;
            gmTableIterator it;
            for (gmTableNode* node = list->GetFirst(it); !list->IsNull(it);
                 node = list->GetNext(it)) {
                if (node && node->m_value.m_type >= GM_USER)
                    animations.push_back(node->m_value);
            }
            for (const gmVariable& animation : animations)
                DetachAnimationFromManager(a_machine, animation);
        }
    }
    entity->Set(&a_machine, "__nativeOwnedAnimations", gmVariable::s_null);
}

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

// Retail ENTITY_GetTypeName (0x000aa0ec) maps a native entity type ID back
// through the same intern table used by ENTITY_GetType. FUN_000bf8ec falls
// back to entry zero for an out-of-range ID; keep that behavior when the
// local registry has at least one entry.
int GM_CDECL gmf_ENTITY_GetTypeName(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    const uint32_t typeId = static_cast<uint32_t>(a_thread->Param(0).m_value.m_int);
    std::string name;
    bool found = false;
    for (const auto& entry : TypeIdRegistry()) {
        if (static_cast<uint32_t>(entry.second) == typeId) {
            name = entry.first;
            found = true;
            break;
        }
    }
    if (!found && !TypeIdRegistry().empty()) {
        for (const auto& entry : TypeIdRegistry()) {
            if (entry.second == 0) {
                name = entry.first;
                break;
            }
        }
    }
    a_thread->PushString(a_thread->GetMachine()->AllocStringObject(name.c_str()));
    return GM_OK;
}

// FUN_001c0d38 creates a missing entity channel with min/max volume 1,
// min/max pan 0 and both gate fields 0. The native getters return pointers
// into that registry, so edits to one wrapper must reach later lookups.
struct MixerParams { float gateSteps = 0, gateStepsRand = 0, minPan = 0, maxPan = 0, minVol = 1, maxVol = 1; };
using MixerParamsPtr = std::shared_ptr<MixerParams>;
std::map<std::pair<int, int>, MixerParamsPtr>& MixerData() {
    static std::map<std::pair<int, int>, MixerParamsPtr> data;
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
    MixerParamsPtr& record = MixerData()[{mixerID, id}];
    if (!record) record = std::make_shared<MixerParams>();
    *record = MixerParams{gateSteps, gateStepsRand, minPan, maxPan, minVol, maxVol};
    return GM_OK;
}

int GM_CDECL gmf_ENTITYBASE_ClearMixer(gmThread* a_thread) {
    MixerData().clear();
    return GM_OK;
}

gmType g_mixerParamsType = -1;
MixerParams* ThisMixerParams(gmThread* a_thread) {
    auto* ref = (MixerParamsPtr*)a_thread->ThisUserCheckType(g_mixerParamsType);
    return ref ? ref->get() : nullptr;
}

int GM_CDECL gmf_mixerParams_getGateSteps(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->gateSteps);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getGateStepsRand(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->gateStepsRand);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMinPan(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->minPan);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMaxPan(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->maxPan);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMinVol(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->minVol);
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_getMaxVol(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
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
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->gateSteps = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setGateStepsRand(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->gateStepsRand = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMinPan(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->minPan = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMaxPan(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->maxPan = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMinVol(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
    if (!p) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->minVol = v;
    return GM_OK;
}
int GM_CDECL gmf_mixerParams_setMaxVol(gmThread* a_thread) {
    MixerParams* p = ThisMixerParams(a_thread);
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
    delete (MixerParamsPtr*)a_object->m_user;
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
    a_thread->PushNewUser(new MixerParamsPtr(it->second), g_mixerParamsType);
    return GM_OK;
}
// Retail 0x001c6df0 passes the requested channel ID and receiver mixer to
// FUN_001c0d38, which inserts the default record when the channel is absent.
// The global ENTITYBASE_GetMixerParams uses a different lookup and returns
// null on a miss. This method's argument is the channel, not the mixer ID.
int GM_CDECL gmf_entity_getMixerParams(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(channelID, 0, 0);
    gmVariable mixer = self->Get(a_thread->GetMachine(), "__mixerID");
    const int mixerID = mixer.m_type == GM_INT ? mixer.m_value.m_int : -1;
    MixerParamsPtr& record = MixerData()[{mixerID, channelID}];
    if (!record) record = std::make_shared<MixerParams>();
    a_thread->PushNewUser(new MixerParamsPtr(record), g_mixerParamsType);
    return GM_OK;
}

// ENTITY_GetMixerIDFromType(typeId) -- retail 0x000aa1b0 calls
// FUN_000bf9a0, which adds the fixed signed 32-bit constant 1,000,000.
int GM_CDECL gmf_ENTITY_GetMixerIDFromType(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    const uint32_t payload = static_cast<uint32_t>(a_thread->Param(0).m_value.m_int);
    const uint32_t sum = payload + 1000000u;
    int32_t result;
    std::memcpy(&result, &sum, sizeof(result)); // retail addc followed by extsw
    a_thread->PushInt(result);
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
    std::vector<std::pair<int, gmVariable>> pendingCreation;
    // Retail EntityManager has an object-ID lookup alongside its ordered
    // active list. FindEntity used to linearly scan that list; draw hooks
    // call it once per candidate, turning a full-level draw pass quadratic.
    std::unordered_map<int, gmVariable> entitiesByObjectID;
    std::unordered_map<int, gmVariable> entitiesByScriptUID;
    std::vector<gmVariable> pendingDeletion;
    gmVariable dummyEntity = gmVariable::s_null;
    // EntityManager stores this as an untyped 32-bit word; its retail getter
    // always pushes the word as GM_INT (FUN_000c3abc / Entity.c:1045).
    // Main.gm's createNewScreenData contains a 100-point fallback beside
    // getEntityPerfMaxCost(). Use that fallback for the desktop manager's
    // initial state: otherwise ground-only screens divide a positive ground
    // cost by zero and are immediately classified as over budget. The exact
    // retail constructor default remains under Ghidra audit.
    gmint perfMaxCost = 100;
    bool deleteAllInProgress = false;
    bool deleteAllPending = false;
    // Visibility snapshots are rebuilt only when an entity is added/removed,
    // its screen extents change, or the owning process changes screen cell.
    // The underlying retail predicate remains EntityIsOnCurrentScreen();
    // this revision only avoids repeating the full registry walk every draw.
    std::uint64_t visibilityRevision = 1;
    int nextId = 1;
};
EntityRegistry& Entities() {
    static EntityRegistry reg;
    return reg;
}

gmVariable* FindEntity(int a_uid) {
    // Retail clear-all resets object-ID lookup before its observer/destructor
    // callbacks, but keeps the active iteration list until teardown ends.
    if (!Entities().deleteAllInProgress) {
        auto found = Entities().entitiesByObjectID.find(a_uid);
        if (found != Entities().entitiesByObjectID.end()) return &found->second;
    }
    // Retail getEntity() first checks the committed object-ID map, then
    // scans EntityManager's pending-create array (FUN_000c3acc).
    for (auto& e : Entities().pendingCreation) if (e.first == a_uid) return &e.second;
    return nullptr;
}

// Retail's setUIDData() registers the script UID in a separate UID map;
// getEntityFromUID() queries that map, while getEntity() remains keyed by
// stable object ID. Do not fall back between these namespaces: object IDs
// and level-authored UIDs can overlap numerically.
gmVariable* FindEntityByScriptUID(gmMachine&, int a_uid) {
    auto& byUid = Entities().entitiesByScriptUID;
    auto found = byUid.find(a_uid);
    return found == byUid.end() ? nullptr : &found->second;
}

void RemoveScriptUIDRegistration(gmMachine& a_machine, int a_uid,
                                 const gmVariable& a_entity,
                                 const gmVariable& a_handle = gmVariable::s_null) {
    auto found = Entities().entitiesByScriptUID.find(a_uid);
    if (found == Entities().entitiesByScriptUID.end()) return;
    if (found->second.m_type != GM_TABLE) {
        Entities().entitiesByScriptUID.erase(found);
        return;
    }
    if (found->second.m_value.m_ref == a_entity.m_value.m_ref ||
        (a_handle.m_type == GM_TABLE &&
         found->second.m_value.m_ref == a_handle.m_value.m_ref)) {
        Entities().entitiesByScriptUID.erase(found);
        return;
    }
    // Before the behavior table is returned, setUIDData may register the
    // factory handle itself. Recognize the handle's __behaviorOwner link
    // when replacing or removing that native identity.
    gmTableObject* mapped = (gmTableObject*)a_machine.GetObject(found->second.m_value.m_ref);
    gmVariable mappedOwner = mapped ? mapped->Get(&a_machine, "__behaviorOwner") : gmVariable::s_null;
    if (mappedOwner.m_type == GM_TABLE && a_entity.m_type == GM_TABLE &&
        mappedOwner.m_value.m_ref == a_entity.m_value.m_ref) {
        Entities().entitiesByScriptUID.erase(found);
    }
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
    int physicsType = 0;
    gmVariable typeVar = self->Get(&m, "__physicsType");
    if (typeVar.m_type == GM_INT) physicsType = typeVar.m_value.m_int;
    else if (typeVar.m_type == GM_FLOAT) physicsType = (int)typeVar.m_value.m_float;
    gmVariable pd = MakePhysicsData(m, gmVariable(self), physicsType);
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
// Retail exposes one native entity object to both the script's Entity field
// and its behavior methods. This port has two GM tables for that object;
// native fields must be read and written through the pair as one state.
gmTableObject* PairedEntityTable(gmMachine& a_machine, gmTableObject* a_self) {
    if (!a_self) return nullptr;
    gmVariable other = a_self->Get(&a_machine, "__behaviorOwner");
    if (other.m_type != GM_TABLE) other = a_self->Get(&a_machine, "Entity");
    if (other.m_type != GM_TABLE) return nullptr;
    gmTableObject* table = (gmTableObject*)a_machine.GetObject(other.m_value.m_ref);
    return table != a_self ? table : nullptr;
}
void SetEntityNativeField(gmMachine& a_machine, gmTableObject* a_self,
                          const char* a_key, const gmVariable& a_value) {
    if (!a_self) return;
    a_self->Set(&a_machine, a_key, a_value);
    if (gmTableObject* pair = PairedEntityTable(a_machine, a_self))
        pair->Set(&a_machine, a_key, a_value);
}
gmVariable GetEntityNativeField(gmMachine& a_machine, gmTableObject* a_self,
                                const char* a_key) {
    if (!a_self) return gmVariable::s_null;
    gmVariable value = a_self->Get(&a_machine, a_key);
    if (value.m_type == GM_NULL) {
        if (gmTableObject* pair = PairedEntityTable(a_machine, a_self))
            value = pair->Get(&a_machine, a_key);
    }
    return value;
}
void SyncEntityNativeFields(gmMachine& a_machine, gmTableObject* a_handle,
                            gmTableObject* a_behavior) {
    // A factory can set native flags on its handle before returning the
    // behavior table, when the two objects cannot yet mirror one another.
    static const char* keys[] = {
        "__enabled", "__invisible", "__staticDraw", "__editable",
        "__beingEdited", "__placing", "__alwaysProcessed", "__sauceBase",
        "__noUpdate", "__noPhysicsUpdate", "__noPhysicsCollide", "__isManual", "__isPlayer",
        "__numOffScreensAudible", "__audibilityNeverOnScreen",
        "__lastAudibilityTick", "__audibilityPercCached", "__nativeOwnedAnimations"
    };
    for (const char* key : keys) {
        gmVariable value = a_handle->Get(&a_machine, key);
        if (value.m_type != GM_NULL) a_behavior->Set(&a_machine, key, value);
    }
}
void RemoveEntityFromVideoDrawMap(gmMachine& a_machine, const gmVariable& a_entity) {
    if (a_entity.m_type != GM_TABLE) return;
    gmObject* object = a_machine.GetObject(a_entity.m_value.m_ref);
    if (!object) return;
    // Retail's native ENTITY teardown calls FUN_0004e7cc before releasing
    // the object. The desktop table adapter uses the table allocation as its
    // ordered-map key, so remove it at the corresponding destruction edge.
    auto* table = static_cast<gmTableObject*>(object);
    GetEngineContext().video.drawMapEntities.erase(
        reinterpret_cast<uintptr_t>(table));
}
// Entity.tagForDeletion / isTaggedForDeletion are registered on the native
// ENTITY type (EBOOT method table at 0x004225e4; FUN_0009b0a0 and
// FUN_0009a52c). tagForDeletion enters the manager's deferred deletion path;
// EntityManager.flush later removes manager references and calls the native
// destructor. The split handle/behavior representation stores the retail
// tag state on both tables and queues the behavior table for that flush.
int GM_CDECL gmf_entity_tagForDeletion(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable owner = self->Get(&m, "__behaviorOwner");
    gmTableObject* behavior = owner.m_type == GM_TABLE
        ? (gmTableObject*)m.GetObject(owner.m_value.m_ref) : self;
    const gmVariable entity = behavior ? owner : gmVariable(self);
    const int entityRef = entity.m_type == GM_TABLE ? entity.m_value.m_ref : -1;

    // Retail FUN_0009b0a0 skips its native vtable call if this exact object
    // is already in FUN_00186af4's teardown set. Do so before setting the
    // tagged bit: callers must observe the same unchanged state on this
    // reentrant destruction path.
    if (entityRef >= 0 && NativeEntityTeardownRefs().count(entityRef) != 0)
        return GM_OK;

    // FUN_0009a52c reads this state from native Entity offset +8.
    // EntityManager.flush (FUN_000c3e70) owns the actual deletion boundary.
    if (behavior) behavior->Set(&m, "__taggedForDeletion", gmVariable((gmint)1));
    self->Set(&m, "__taggedForDeletion", gmVariable((gmint)1));
    if (entityRef >= 0) {
        auto& pending = Entities().pendingDeletion;
        const bool queued = std::any_of(pending.begin(), pending.end(),
            [entityRef](const gmVariable& value) {
                return value.m_type == GM_TABLE && value.m_value.m_ref == entityRef;
            });
        if (!queued) pending.push_back(entity);
    }
    return GM_OK;
}
int GM_CDECL gmf_entity_isTaggedForDeletion(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable tagged = self->Get(a_thread->GetMachine(), "__taggedForDeletion");
    a_thread->PushInt(tagged.m_type == GM_INT && tagged.m_value.m_int != 0 ? 1 : 0);
    return GM_OK;
}

void FlushEntityDeletions(gmMachine& a_machine, bool a_deleteAll) {
    auto& registry = Entities();
    auto notifyEntityDeleted = [&](const gmVariable& entity) {
        // The retail flush dispatches entity-deletion observers before the
        // entity's manager links are removed and its destructor is called.
        // Entity scripts can also subscribe with onEntityDelete (for example,
        // GoAroundGuy clears its retained gravityEntity reference there).
        std::vector<int> observerRefs;
        observerRefs.reserve(registry.entities.size());
        for (const auto& candidate : registry.entities) {
            if (candidate.second.m_type == GM_TABLE)
                observerRefs.push_back(candidate.second.m_value.m_ref);
        }
        for (int ref : observerRefs) {
            auto current = std::find_if(registry.entities.begin(), registry.entities.end(),
                [ref](const auto& candidate) {
                    return candidate.second.m_type == GM_TABLE && candidate.second.m_value.m_ref == ref;
                });
            if (current == registry.entities.end()) continue;
            gmVariable observer = current->second;
            gmTableObject* table = (gmTableObject*)a_machine.GetObject(ref);
            if (!table || table->Get(&a_machine, "onEntityDelete").m_type != GM_FUNCTION) continue;
            gmCall call;
            if (call.BeginTableFunction(&a_machine, "onEntityDelete", table, observer)) {
                call.AddParam(entity);
                call.End();
            }
        }
        if (ProcessManager* processes = GetEngineContext().processManager)
            processes->NotifyEntityDeleted(entity);
    };
    if (a_deleteAll) {
        // Retail takes FUN_000c25c8 instead of the ordinary commit/delete
        // path: committed entities are torn down from the active list and
        // pending creations are destroyed separately, without promoting
        // them first. Consume the flag before callbacks so nested requests
        // remain queued for the following flush.
        registry.deleteAllPending = false;
        registry.deleteAllInProgress = true;
        registry.entitiesByObjectID.clear();
        const std::vector<std::pair<int, gmVariable>> active = registry.entities;

        auto removeScriptUID = [&](const gmVariable& entity) {
            if (entity.m_type != GM_TABLE) return;
            gmTableObject* table = (gmTableObject*)a_machine.GetObject(entity.m_value.m_ref);
            if (!table) return;
            gmVariable uid = table->Get(&a_machine, "__uid");
            gmVariable handle = table->Get(&a_machine, "Entity");
            if (uid.m_type == GM_INT && uid.m_value.m_int != -1)
                RemoveScriptUIDRegistration(a_machine, uid.m_value.m_int, entity, handle);
        };
        auto runDestructor = [&](const gmVariable& entity) {
            if (entity.m_type != GM_TABLE) return;
            gmTableObject* table = (gmTableObject*)a_machine.GetObject(entity.m_value.m_ref);
            if (table) {
                gmVariable destructor = table->Get(&a_machine, "destructor");
                if (destructor.m_type == GM_FUNCTION) {
                    gmCall call;
                    if (call.BeginTableFunction(&a_machine, "destructor", table, entity))
                        call.End();
                }
            }
            {
                ScopedNativeEntityTeardown teardown(
                    entity, GetEngineContext().processManager);
                RemoveEntityFromVideoDrawMap(a_machine, entity);
                DetachEntityOwnedAnimations(a_machine, entity);
                DestroyPhysicsDataForEntity(a_machine, entity);
                UnrootGmVariable(a_machine, entity);
            }
        };

        // Remove active lookup registrations before any destructor runs,
        // matching delete-all's manager-index reset boundary.
        for (const auto& entry : active) removeScriptUID(entry.second);
        for (const auto& entry : active) {
            notifyEntityDeleted(entry.second);
            runDestructor(entry.second);
        }

        // Retail then drains the pending-creation array separately. Drain
        // the live vector so entities appended by a destructor are handled
        // by this same clear-all operation too.
        while (!registry.pendingCreation.empty()) {
            gmVariable entity = registry.pendingCreation.front().second;
            registry.pendingCreation.erase(registry.pendingCreation.begin());
            removeScriptUID(entity);
            runDestructor(entity);
        }
        registry.pendingDeletion.clear();
        registry.entities.clear();
        registry.entitiesByObjectID.clear();
        ++registry.visibilityRevision;
        registry.deleteAllInProgress = false;
        return;
    }

    // FUN_000c3e70 commits all pending creations before it drains ordinary
    // deletions. getEntity() can see these objects before this point, but
    // the manager's active iteration list cannot.
    if (!registry.pendingCreation.empty()) {
        ++registry.visibilityRevision;
        for (const auto& entry : registry.pendingCreation)
            registry.entitiesByObjectID[entry.first] = entry.second;
    }
    registry.entities.insert(registry.entities.end(),
        registry.pendingCreation.begin(), registry.pendingCreation.end());
    registry.pendingCreation.clear();
    // The retail flush walks its deletion array and observes entries appended
    // by destructors during that same drain. Keep the index over the live
    // queue rather than swapping it out up front.
    size_t cursor = 0;
    while (cursor < registry.pendingDeletion.size()) {
        gmVariable entity = registry.pendingDeletion[cursor++];
        if (entity.m_type != GM_TABLE) continue;
        auto& live = registry.entities;
        auto found = std::find_if(live.begin(), live.end(), [&entity](const auto& entry) {
            return entry.second.m_type == GM_TABLE &&
                   entry.second.m_value.m_ref == entity.m_value.m_ref;
        });
        if (found == live.end()) continue;

        // Observers can call back into EntityManager. Reacquire the vector
        // iterator and table after dispatch in case a nested flush removed
        // this entity or invalidated the registry's storage.
        notifyEntityDeleted(entity);
        found = std::find_if(live.begin(), live.end(), [&entity](const auto& entry) {
            return entry.second.m_type == GM_TABLE &&
                   entry.second.m_value.m_ref == entity.m_value.m_ref;
        });
        if (found == live.end()) continue;
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(entity.m_value.m_ref);
        if (!table) continue;

        // Retail removes the UID and object-ID lookups before invoking the
        // entity destructor (EntityManager.flush -> FUN_000c2388 -> vtable
        // slot 0x30). Keep the table rooted until the script destructor
        // returns, since the portable registry is its current owning root.
        const int objectId = found->first;
        gmVariable uid = table->Get(&a_machine, "__uid");
        gmVariable handle = table->Get(&a_machine, "Entity");
        if (uid.m_type == GM_INT && uid.m_value.m_int != -1)
            RemoveScriptUIDRegistration(a_machine, uid.m_value.m_int, entity, handle);
        registry.entitiesByObjectID.erase(objectId);
        live.erase(found);
        ++registry.visibilityRevision;

        gmVariable destructor = table->Get(&a_machine, "destructor");
        if (destructor.m_type == GM_FUNCTION) {
            gmCall call;
            if (call.BeginTableFunction(&a_machine, "destructor", table, entity))
                call.End();
        }
        {
            ScopedNativeEntityTeardown teardown(
                entity, GetEngineContext().processManager);
            RemoveEntityFromVideoDrawMap(a_machine, entity);
            DetachEntityOwnedAnimations(a_machine, entity);
            DestroyPhysicsDataForEntity(a_machine, entity);
            UnrootGmVariable(a_machine, entity);
        }
    }
    registry.pendingDeletion.clear();
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
int GM_CDECL gmf_entity_delegate_drawExplosionIfGot(gmThread* a_thread) {
    return DelegateToBehaviorOwner(a_thread, "drawExplosionIfGot");
}
int GM_CDECL gmf_entity_delegate_updateExplosionIfGot(gmThread* a_thread) {
    return DelegateToBehaviorOwner(a_thread, "updateExplosionIfGot");
}
int GM_CDECL gmf_entity_delegate_giveBackToBulletCache(gmThread* a_thread) {
    return DelegateToBehaviorOwner(a_thread, "giveBackToBulletCache");
}
int GM_CDECL gmf_entity_delegate_onMusicTrigger(gmThread* a_thread) {
    // Optional trace for the Craig room recovery audit: these scripts count
    // music-trigger callbacks before standing up after a knockdown. This is
    // diagnostic only and leaves the retail delegate path untouched.
    static const bool s_traceCraigMusic =
        std::getenv("GE3_TRACE_CRAIG_MUSIC") != nullptr;
    if (s_traceCraigMusic) {
        gmTableObject* self = a_thread->ThisTable();
        gmMachine* machine = a_thread->GetMachine();
        if (self && machine) {
            gmVariable type = self->Get(machine, "__typeName");
            gmVariable id = self->Get(machine, "__objectID");
            const char* typeName = type.m_type == GM_STRING && type.m_value.m_ref
                ? static_cast<gmStringObject*>(machine->GetObject(type.m_value.m_ref))->GetString()
                : "?";
            if (std::strstr(typeName, "CraigBoardroomExec") ||
                std::strstr(typeName, "CraigCoffeeDrinker")) {
                const int step = a_thread->GetNumParams() > 0 &&
                                 a_thread->Param(0).m_type == GM_INT
                    ? a_thread->Param(0).m_value.m_int : -1;
                std::fprintf(stderr,
                    "[craig-music] id=%d type=%s step=%d\n",
                    id.m_type == GM_INT ? id.m_value.m_int : -1,
                    typeName, step);
            }
        }
    }
    return DelegateToBehaviorOwner(a_thread, "onMusicTrigger");
}
int GM_CDECL gmf_entity_delegate_isAlwaysMusicTriggered(gmThread* a_thread) {
    return DelegateToBehaviorOwner(a_thread, "isAlwaysMusicTriggered");
}
int GM_CDECL gmf_entity_delegate_setIsAlwaysMusicTriggered(gmThread* a_thread) {
    return DelegateToBehaviorOwner(a_thread, "setIsAlwaysMusicTriggered");
}

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
// Script-space position lookup used by visibility/overlap helpers below.
// Entity vector and distance getters use the primary PhysicsBody position
// directly, as their retail helpers do.
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
// getVectorToNormalized(other) -- retail helper 0xc0c40 normalizes the raw
// difference returned by 0xc0be4, which uses each entity's first PhysicsBody
// position. The PPC fsel sequence reduces to dx/length and dy/length for every
// nonzero length; a zero length preserves the native 0/0 NaN result.
// Retail Entity.getDistanceTo (0x9b964 / FUN_000c0dd8) measures the
// Euclidean distance between the two entities' primary PhysicsBody position
// fields. It does not use fixture/AABB centers or configureData positions.
int GM_CDECL gmf_entity_getDistanceTo(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& machine = *a_thread->GetMachine();
    gmVariable otherVar = a_thread->Param(0);
    gmTableObject* other = otherVar.m_type == GM_TABLE
        ? (gmTableObject*)machine.GetObject(otherVar.m_value.m_ref) : nullptr;
    if (!other) return GM_EXCEPTION;

    gmVariable selfPhysics = self->Get(&machine, "PhysicsData");
    if (selfPhysics.m_type == GM_NULL) selfPhysics = self->Get(&machine, "__physicsData");
    gmVariable otherPhysics = other->Get(&machine, "PhysicsData");
    if (otherPhysics.m_type == GM_NULL) otherPhysics = other->Get(&machine, "__physicsData");
    float sx = 0.0f, sy = 0.0f, ox = 0.0f, oy = 0.0f;
    if (!GetPhysicsDataPos(machine, selfPhysics, sx, sy) ||
        !GetPhysicsDataPos(machine, otherPhysics, ox, oy))
        return GM_EXCEPTION;

    const float dx = ox - sx;
    const float dy = oy - sy;
    a_thread->PushFloat(std::sqrt(dx * dx + dy * dy));
    return GM_OK;
}

// Retail Entity.getVectorTo (0x9b890 -> FUN_000c0be4) returns the raw
// self-to-other difference between each entity's first PhysicsBody position.
// This intentionally does not normalize the result or substitute bounds / script
// configureData when either entity has no main body.
int GM_CDECL gmf_entity_getVectorTo(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable otherVar = a_thread->Param(0);
    gmTableObject* other = otherVar.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine.GetObject(otherVar.m_value.m_ref))
        : nullptr;
    if (!other) return GM_EXCEPTION;

    gmVariable selfPhysics = self->Get(&machine, "PhysicsData");
    if (selfPhysics.m_type == GM_NULL)
        selfPhysics = self->Get(&machine, "__physicsData");
    gmVariable otherPhysics = other->Get(&machine, "PhysicsData");
    if (otherPhysics.m_type == GM_NULL)
        otherPhysics = other->Get(&machine, "__physicsData");

    float selfX = 0.0f, selfY = 0.0f, otherX = 0.0f, otherY = 0.0f;
    if (!GetPhysicsDataPos(machine, selfPhysics, selfX, selfY) ||
        !GetPhysicsDataPos(machine, otherPhysics, otherX, otherY))
        return GM_EXCEPTION;

    a_thread->Push(MakeVector2D(machine, otherX - selfX, otherY - selfY));
    return GM_OK;
}

int GM_CDECL gmf_entity_getRadiansToFace(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable otherVar = a_thread->Param(0);
    gmTableObject* other = otherVar.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine.GetObject(otherVar.m_value.m_ref))
        : nullptr;
    if (!other) return GM_EXCEPTION;

    gmVariable selfPhysics = self->Get(&machine, "PhysicsData");
    if (selfPhysics.m_type == GM_NULL)
        selfPhysics = self->Get(&machine, "__physicsData");
    gmVariable otherPhysics = other->Get(&machine, "PhysicsData");
    if (otherPhysics.m_type == GM_NULL)
        otherPhysics = other->Get(&machine, "__physicsData");

    float selfX = 0.0f, selfY = 0.0f, otherX = 0.0f, otherY = 0.0f;
    float facingX = 0.0f, facingY = 0.0f;
    if (!GetPhysicsDataPos(machine, selfPhysics, selfX, selfY) ||
        !GetPhysicsDataPos(machine, otherPhysics, otherX, otherY) ||
        !GetPhysicsDataFacingDirection(machine, selfPhysics, facingX, facingY))
        return GM_EXCEPTION;

    const float targetAngle = RetailAtan2(otherY - selfY, otherX - selfX);
    const float facingAngle = RetailAtan2(facingY, facingX);
    float difference = std::fabs(targetAngle - facingAngle);
    constexpr float kPi = 3.141592741f;
    constexpr float kTwoPi = 6.283185482f;
    if (kPi - difference < 0.0f)
        difference = kTwoPi - difference;
    a_thread->PushFloat(difference);
    return GM_OK;
}

int GM_CDECL gmf_entity_getVectorToNormalized(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable otherVar = a_thread->Param(0);
    gmTableObject* other = otherVar.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine.GetObject(otherVar.m_value.m_ref))
        : nullptr;
    if (!other) return GM_EXCEPTION;

    gmVariable selfPhysics = self->Get(&machine, "PhysicsData");
    if (selfPhysics.m_type == GM_NULL)
        selfPhysics = self->Get(&machine, "__physicsData");
    gmVariable otherPhysics = other->Get(&machine, "PhysicsData");
    if (otherPhysics.m_type == GM_NULL)
        otherPhysics = other->Get(&machine, "__physicsData");

    float selfX = 0.0f, selfY = 0.0f, otherX = 0.0f, otherY = 0.0f;
    if (!GetPhysicsDataPos(machine, selfPhysics, selfX, selfY) ||
        !GetPhysicsDataPos(machine, otherPhysics, otherX, otherY))
        return GM_EXCEPTION;

    const float dx = otherX - selfX;
    const float dy = otherY - selfY;
    const float length = std::sqrt(dx * dx + dy * dy);
    a_thread->Push(MakeVector2D(machine, dx / length, dy / length));
    return GM_OK;
}

// Retail getWhichSide compares argument 1's facing direction with the vector
// from argument 1 to argument 2. Positive wrapped angle means the second
// entity is on the positive side; the native receiver itself is unused.
int GM_CDECL gmf_entity_getWhichSide(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 2) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable sourceVar = a_thread->Param(0);
    const gmVariable targetVar = a_thread->Param(1);
    gmTableObject* source = sourceVar.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine.GetObject(sourceVar.m_value.m_ref))
        : nullptr;
    gmTableObject* target = targetVar.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine.GetObject(targetVar.m_value.m_ref))
        : nullptr;
    if (!source || !target) return GM_EXCEPTION;

    auto physicsData = [&machine](gmTableObject* entity) {
        gmVariable value = entity->Get(&machine, "PhysicsData");
        if (value.m_type == GM_NULL)
            value = entity->Get(&machine, "__physicsData");
        return value;
    };
    const gmVariable sourcePhysics = physicsData(source);
    const gmVariable targetPhysics = physicsData(target);
    float sourceX = 0.0f, sourceY = 0.0f, targetX = 0.0f, targetY = 0.0f;
    float facingX = 0.0f, facingY = 0.0f;
    if (!GetPhysicsDataPos(machine, sourcePhysics, sourceX, sourceY) ||
        !GetPhysicsDataPos(machine, targetPhysics, targetX, targetY) ||
        !GetPhysicsDataFacingDirection(machine, sourcePhysics, facingX, facingY))
        return GM_EXCEPTION;

    const float dx = targetX - sourceX;
    const float dy = targetY - sourceY;
    const float length = std::sqrt(dx * dx + dy * dy);
    const float directionX = length > 0.0f ? dx / length : 0.0f;
    const float directionY = length > 0.0f ? dy / length : 0.0f;
    float angle = RetailAtan2(directionY, directionX) -
                  RetailAtan2(facingY, facingX);
    constexpr float kPi = 3.141592741f;
    constexpr float kTwoPi = 6.283185482f;
    if (angle > kPi)
        angle -= kTwoPi;
    else if (angle < -kPi)
        angle += kTwoPi;
    a_thread->PushInt(angle > 0.0f ? 1 : 0);
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
    gmMachine* machine = a_thread->GetMachine();
    // Retail has one native Entity object, while the desktop bridge has a
    // short-lived factory handle and a script behavior table. setUIDData()
    // is often called on the latter; make the handle observe that same UID.
    gmVariable behavior = self->Get(machine, "__behaviorOwner");
    if (behavior.m_type == GM_TABLE) {
        gmTableObject* behaviorTable = (gmTableObject*)machine->GetObject(behavior.m_value.m_ref);
        if (behaviorTable) {
            a_thread->Push(behaviorTable->Get(machine, "__uid"));
            return GM_OK;
        }
    }
    a_thread->Push(self->Get(machine, "__uid"));
    return GM_OK;
}
// getObjectID is the stable identity assigned when the native entity is
// created. Retail setUIDData() updates a separate UID registry slot and does
// not change the object identity used by attachment/contact/entity references.
int GM_CDECL gmf_entity_getObjectID(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable id = self->Get(a_thread->GetMachine(), "__objectID");
    if (id.m_type == GM_NULL) id = self->Get(a_thread->GetMachine(), "__uid");
    a_thread->Push(id);
    return GM_OK;
}
// `setUIDData`, address 0x001c6ad4): re-registers "the C++ portion"'s
// uid->entity map entry, unregistering under the OLD uid first if one was
// already set. EntityBase.gm's own configure() wrapper calls this
// unconditionally (`this.setUIDData(this.configureData.uid);`) with the
// LEVEL-AUTHORED uid (cd.uid), distinct from this engine's stable object
// ID. EntityBase.setUIDData's retail body removes the prior UID mapping
// unless it was -1, writes the new UID, then registers the entity pointer
// in the UID map. The portable registry models that as a separate UID->table
// map, leaving EntityManager.getEntity(objectID) unchanged.
int GM_CDECL gmf_entity_setUIDData(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(newUidF, 0, 0.0f);
    int newUid = (int)newUidF;
    gmMachine* machine = a_thread->GetMachine();
    gmVariable oldUidVar = self->Get(machine, "__uid");
    const int oldUid = oldUidVar.m_type == GM_INT ? oldUidVar.m_value.m_int : -1;
    // The handle and returned behavior table represent one retail Entity.
    // Keep their registered UID field and UID-map target aligned without
    // changing __objectID.
    gmVariable behavior = self->Get(machine, "__behaviorOwner");
    gmVariable entity = (behavior.m_type == GM_TABLE) ? behavior : gmVariable(self);
    gmVariable handle = (behavior.m_type == GM_TABLE) ? gmVariable(self) : self->Get(machine, "Entity");
    if (oldUid != -1) RemoveScriptUIDRegistration(*machine, oldUid, entity, handle);
    self->Set(machine, "__uid", gmVariable((gmint)newUid));
    if (behavior.m_type == GM_TABLE) {
        gmTableObject* behaviorTable = (gmTableObject*)machine->GetObject(behavior.m_value.m_ref);
        if (behaviorTable) behaviorTable->Set(machine, "__uid", gmVariable((gmint)newUid));
    }
    if (handle.m_type == GM_TABLE) {
        gmTableObject* handleTable = (gmTableObject*)machine->GetObject(handle.m_value.m_ref);
        if (handleTable) handleTable->Set(machine, "__uid", gmVariable((gmint)newUid));
    }
    Entities().entitiesByScriptUID[newUid] = entity;
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
void StoreScreenExtents(gmMachine& a_machine, gmTableObject* a_self,
                        int a_minX, int a_maxX, int a_minY, int a_maxY) {
    if (!a_self) return;
    auto write = [&](gmTableObject* table) {
        table->Set(&a_machine, "__screenMinCX", gmVariable((gmint)a_minX));
        table->Set(&a_machine, "__screenMaxCX", gmVariable((gmint)a_maxX));
        table->Set(&a_machine, "__screenMinCY", gmVariable((gmint)a_minY));
        table->Set(&a_machine, "__screenMaxCY", gmVariable((gmint)a_maxY));
    };
    write(a_self);
    // The portable engine has separate native-handle and behavior tables;
    // retail stores these four fields on their shared entity object.
    gmVariable other = a_self->Get(&a_machine, "__behaviorOwner");
    if (other.m_type != GM_TABLE) other = a_self->Get(&a_machine, "Entity");
    if (other.m_type == GM_TABLE) {
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(other.m_value.m_ref);
        if (table && table != a_self) write(table);
    }
    ++Entities().visibilityRevision;
}
bool PushRecordedScreenExtent(gmThread* a_thread, gmTableObject* a_self, const char* a_key) {
    gmVariable value = a_self->Get(a_thread->GetMachine(), a_key);
    if (value.m_type != GM_INT) return false;
    a_thread->PushInt(value.m_value.m_int);
    return true;
}
}
int GM_CDECL gmf_entity_getMinCX(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    if (PushRecordedScreenExtent(a_thread, self, "__screenMinCX")) return GM_OK;
    gmMachine& m = *a_thread->GetMachine();
    float minX, minY, maxX, maxY;
    GetPhysicsDataAABB(m, self->Get(&m, "PhysicsData"), minX, minY, maxX, maxY);
    a_thread->PushInt(WorldToScreenCell(minX, kScreenCellWidth));
    return GM_OK;
}
int GM_CDECL gmf_entity_getMaxCX(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    if (PushRecordedScreenExtent(a_thread, self, "__screenMaxCX")) return GM_OK;
    gmMachine& m = *a_thread->GetMachine();
    float minX, minY, maxX, maxY;
    GetPhysicsDataAABB(m, self->Get(&m, "PhysicsData"), minX, minY, maxX, maxY);
    a_thread->PushInt(WorldToScreenCell(maxX, kScreenCellWidth));
    return GM_OK;
}
int GM_CDECL gmf_entity_getMinCY(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    if (PushRecordedScreenExtent(a_thread, self, "__screenMinCY")) return GM_OK;
    gmMachine& m = *a_thread->GetMachine();
    float minX, minY, maxX, maxY;
    GetPhysicsDataAABB(m, self->Get(&m, "PhysicsData"), minX, minY, maxX, maxY);
    a_thread->PushInt(WorldToScreenCell(minY, kScreenCellHeight));
    return GM_OK;
}
int GM_CDECL gmf_entity_getMaxCY(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    if (PushRecordedScreenExtent(a_thread, self, "__screenMaxCY")) return GM_OK;
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
// Retail stores entity creation position as two floats at native offsets
// +0xd0/+0xd4. The script bridge stores the same pair as a copied vector2D.
int GM_CDECL gmf_entity_setEntityCreationPos(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    float x = 0.0f, y = 0.0f;
    if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y))
        return GM_EXCEPTION;
    self->Set(a_thread->GetMachine(), "__entityCreationPos",
              MakeVector2D(*a_thread->GetMachine(), x, y));
    return GM_OK;
}
int GM_CDECL gmf_entity_getEntityCreationPos(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    float x = 0.0f, y = 0.0f;
    gmVariable pos = self->Get(&machine, "__entityCreationPos");
    if (pos.m_type != GM_NULL && !GetVector2D(machine, pos, x, y))
        return GM_EXCEPTION;
    // Retail allocates a fresh Vector2D for each getter call.
    a_thread->Push(MakeVector2D(machine, x, y));
    return GM_OK;
}
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

// setIsEnabled/isEnabled are native entity state. Store through the paired
// handle/behavior tables so callers using this.Entity and callers using the
// factory result observe the same flag. Retail FUN_000c02f8 changes the
// entity's body sleep state and animation-manager membership as well.
// Configuration remains scheduled by ConfigureUnconfiguredEntities;
// starting it recursively from this setter caused a real
// PlayerCheckpoint/vehicleEntity configure cycle.
int GM_CDECL gmf_entity_setIsEnabled(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    gmint flag = a_thread->ParamInt(0, 0); // lenient: non-int (incl. null) -> default 0
    gmMachine& machine = *a_thread->GetMachine();
    gmVariable physicsData = GetEntityNativeField(machine, self, "PhysicsData");
    if (physicsData.m_type == GM_NULL) {
        physicsData = GetEntityNativeField(machine, self, "__physicsData");
    }
    // RETAIL PPC: FUN_000c02f8 accepts disable with no bodies, but the enable
    // path requires a nonempty PhysicsData body array. Otherwise it reports
    // "ERROR: enabling when we have no bodies! that's ludicrious" and returns
    // without changing the enabled byte, manager memberships, or animations.
    // EntityBase.configure() normally reaches this setter after rebuilding
    // bodies; keeping the guard here also matches unusual direct callers.
    if (flag && !PhysicsDataHasBodies(machine, physicsData)) {
        gmVariable objectId = GetEntityNativeField(machine, self, "__objectID");
        gmVariable typeName = GetEntityNativeField(machine, self, "__typeName");
        const char* typeText = "<unknown>";
        if (typeName.m_type == GM_STRING && typeName.m_value.m_ref) {
            gmStringObject* stringObject = (gmStringObject*)machine.GetObject(typeName.m_value.m_ref);
            if (stringObject) typeText = stringObject->GetString();
        }
        std::fprintf(stderr, "[entity-enable] retail rejected bodyless enable: id=%d type=%s physicsDataType=%d\n",
                     objectId.m_type == GM_INT ? objectId.m_value.m_int : -1,
                     typeText, physicsData.m_type);
        // DESKTOP LAZY-CONFIG BRIDGE (not a change to retail's native
        // setter): Main.gm can activate a newly entered screen before this
        // port's ConfigureUnconfiguredEntities fallback has built its bodies.
        // Retail has already constructed the entity before the native screen
        // membership update. Preserve the activation request only for a
        // populated, not-yet-configured level entity; the sweep replays it
        // after configure() creates the native PhysicsData body array.
        gmVariable configured = GetEntityNativeField(machine, self, "__nativeConfigured");
        // The handle can carry an empty configureData placeholder while its
        // paired behavior table owns the level's populated record. Inspect
        // the behavior first so the lazy-config bridge is limited to real
        // placed entities rather than ordinary bodyless handles.
        gmTableObject* behavior = PairedEntityTable(machine, self);
        gmVariable configureData = behavior
            ? behavior->Get(&machine, "configureData") : gmVariable::s_null;
        if (configureData.m_type != GM_TABLE)
            configureData = GetEntityNativeField(machine, self, "configureData");
        gmTableObject* config = configureData.m_type == GM_TABLE
            ? (gmTableObject*)machine.GetObject(configureData.m_value.m_ref) : nullptr;
        if ((configured.m_type != GM_INT || configured.m_value.m_int == 0) &&
            config && config->Get(&machine, "entType").m_type != GM_NULL) {
            SetEntityNativeField(machine, self, "__pendingEnableAfterConfigure",
                                 gmVariable((gmint)1));
            std::fprintf(stderr, "[entity-enable] deferred until lazy configure: id=%d type=%s\n",
                         objectId.m_type == GM_INT ? objectId.m_value.m_int : -1,
                         typeText);
        }
        std::fprintf(stderr,
                     "ERROR: enabling when we have no bodies! that's ludicrious\n");
        return GM_OK;
    }
    if (!flag) {
        gmVariable configuring = GetEntityNativeField(machine, self, "__nativeConfiguring");
        // EntityBase.endConfigure() replays the current enabled value while
        // configure() is still on the stack. Keep the pending screen-entry
        // request through that false-valued synchronization call; an actual
        // disable outside configure must cancel it.
        if (configuring.m_type != GM_INT || configuring.m_value.m_int == 0)
            SetEntityNativeField(machine, self, "__pendingEnableAfterConfigure",
                                 gmVariable((gmint)0));
    }
    SetEntityNativeField(*a_thread->GetMachine(), self, "__enabled", gmVariable((gmint)(flag ? 1 : 0)));
    InvalidatePhysicsMovementMembership();
    // EntityBase.configure() calls setIsEnabled() after rebuilding bodies.
    // Box2D's SetAwake transition clears motion/forces on sleep, matching the
    // retail body sleep helper; it does not deactivate fixtures or contacts.
    SetPhysicsDataAwake(machine, physicsData, flag != 0);
    // Retail FUN_000c02f8 also removes/re-adds every entity-owned animation
    // through FUN_000b9c0c/FUN_000b9f40. Keep the animation object and its
    // cursor alive; only stop manager ticks while the entity is disabled.
    gmVariable animationsVar = GetEntityNativeField(machine, self, "__nativeOwnedAnimations");
    if (animationsVar.m_type == GM_TABLE) {
        gmTableObject* animations = (gmTableObject*)machine.GetObject(animationsVar.m_value.m_ref);
        if (animations) {
            std::vector<gmVariable> owned;
            gmTableIterator it;
            for (gmTableNode* node = animations->GetFirst(it); !animations->IsNull(it);
                 node = animations->GetNext(it)) {
                if (node && node->m_value.m_type >= GM_USER)
                    owned.push_back(node->m_value);
            }
            for (const gmVariable& animation : owned)
                SetAnimationManagerAttached(machine, animation, flag != 0);
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_entity_isEnabled(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(1); return GM_OK; }
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__enabled");
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
    SetEntityNativeField(*a_thread->GetMachine(), self, "__invisible", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isInvisible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__invisible");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
// Retail EntityBase stores the static-draw flag at native offset +0xdb
// (EBOOT 0x1c57a4) and exposes a matching getter (0x1c587c).
int GM_CDECL gmf_entity_setIsStaticDraw(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(flag, 0, 0);
    SetEntityNativeField(*a_thread->GetMachine(), self, "__staticDraw", gmVariable((gmint)(flag != 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isStaticDraw(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__staticDraw");
    a_thread->PushInt(v.m_type == GM_INT && v.m_value.m_int != 0 ? 1 : 0);
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
    SetEntityNativeField(*a_thread->GetMachine(), self, "__editable", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isEditable(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__editable");
    a_thread->PushInt((v.m_type != GM_INT || v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}

// Retail recordScreenExtents stores inclusive screen-grid bounds at entity
// offsets +0x120/+0x124/+0x128/+0x12c (EBOOT 0x1c7d20). The saved-level
// initializeScreenExtents path supplies those same four cell coordinates.
int GM_CDECL gmf_entity_recordScreenExtents(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(4);
    const float minX = a_thread->ParamFloatOrInt(0, 0.0f);
    const float minY = a_thread->ParamFloatOrInt(1, 0.0f);
    const float maxX = a_thread->ParamFloatOrInt(2, 0.0f);
    const float maxY = a_thread->ParamFloatOrInt(3, 0.0f);
    StoreScreenExtents(*a_thread->GetMachine(), self,
        WorldToScreenCell(minX, kScreenCellWidth), WorldToScreenCell(maxX, kScreenCellWidth),
        WorldToScreenCell(minY, kScreenCellHeight), WorldToScreenCell(maxY, kScreenCellHeight));
    return GM_OK;
}
namespace {
bool CurrentVisibilityProcess(gmMachine& machine, gmVariable& processVar,
                              int& screenX, int& screenY);
}

bool EntityIsOnCurrentScreen(gmMachine& machine, gmTableObject* self,
                             bool unboundedVisible = false) {
    if (!self) return true;
    gmVariable owner = self->Get(&machine, "__behaviorOwner");
    gmTableObject* behavior = owner.m_type == GM_TABLE
        ? (gmTableObject*)machine.GetObject(owner.m_value.m_ref) : self;
    gmTableObject* bounds = self;
    if (self->Get(&machine, "__screenMinCX").m_type == GM_NULL) {
        if (behavior && behavior->Get(&machine, "__screenMinCX").m_type != GM_NULL) {
            bounds = behavior;
        } else {
            // EntityBase stores the same screen extent fields on both the
            // native handle and its script behavior table. When this query
            // receives the behavior table, the paired handle is available as
            // `Entity` (not `__behaviorOwner`); failing to follow it treated
            // every such bounded entity as unbounded/visible and defeated
            // screen culling on the render passes.
            gmVariable handleVar = behavior ? behavior->Get(&machine, "Entity")
                                            : gmVariable::s_null;
            gmTableObject* handle = handleVar.m_type == GM_TABLE
                ? (gmTableObject*)machine.GetObject(handleVar.m_value.m_ref) : nullptr;
            if (handle && handle->Get(&machine, "__screenMinCX").m_type != GM_NULL)
                bounds = handle;
        }
    }
    // Entities without recorded bounds (such as global/menu entities) do
    // not participate in the level's screen registration. The native
    // script-visible query still uses its original zero-valued bounds.
    const bool hasBounds = bounds->Get(&machine, "__screenMinCX").m_type != GM_NULL;
    if (!hasBounds && unboundedVisible) return true;
    auto cell = [&](const char* key) {
        gmVariable value = bounds->Get(&machine, key);
        return value.m_type == GM_INT ? value.m_value.m_int
            : value.m_type == GM_FLOAT ? (int)value.m_value.m_float : 0;
    };
    // Retail EntityBase.isOnCurrentScreen reads +0xcc/+0xd0 from the active
    // Platformer_Main process singleton (DAT_00424e70), not VideoState and
    // not an entity-specific gameProcess pointer. GameSystem's
    // getEntityUpdateProcess selects that same process in the desktop port.
    // The entity's gameProcess is only a fallback for startup/teardown when
    // no update process has been installed yet.
    int currentX = GetEngineContext().video.currentScreenX;
    int currentY = GetEngineContext().video.currentScreenY;
    gmVariable processVar = gmVariable::s_null;
    if (!CurrentVisibilityProcess(machine, processVar, currentX, currentY)) {
        gmVariable entityProcess = behavior
            ? behavior->Get(&machine, "gameProcess") : gmVariable::s_null;
        if (entityProcess.m_type == GM_TABLE) {
            gmTableObject* process = (gmTableObject*)machine.GetObject(
                entityProcess.m_value.m_ref);
            if (process) {
                gmVariable x = process->Get(&machine, "currentScreenX");
                gmVariable y = process->Get(&machine, "currentScreenY");
                if (x.m_type == GM_INT) currentX = x.m_value.m_int;
                if (y.m_type == GM_INT) currentY = y.m_value.m_int;
            }
        }
    }
    // EBOOT 0x1c7af4 uses inclusive comparisons on both axes.
    return cell("__screenMinCX") <= currentX && currentX <= cell("__screenMaxCX") &&
           cell("__screenMinCY") <= currentY && currentY <= cell("__screenMaxCY");
}

namespace {
struct EntityVisibilitySnapshot {
    bool initialized = false;
    bool compatible = false;
    std::uint64_t revision = 0;
    gmptr processRef = static_cast<gmptr>(-1);
    int processX = 0;
    int processY = 0;
    int videoX = 0;
    int videoY = 0;
    std::vector<int> unboundedVisible;
    std::vector<int> boundedVisible;
};

EntityVisibilitySnapshot& VisibilitySnapshot() {
    static EntityVisibilitySnapshot snapshot;
    return snapshot;
}

bool CurrentVisibilityProcess(gmMachine& machine, gmVariable& processVar,
                              int& screenX, int& screenY) {
    ProcessManager* manager = GetEngineContext().processManager;
    Process* process = manager ? manager->GetEntityUpdateProcess() : nullptr;
    if (!process || process->table.m_type != GM_TABLE) return false;
    processVar = process->table;
    gmTableObject* table = (gmTableObject*)machine.GetObject(processVar.m_value.m_ref);
    if (!table) return false;
    gmVariable x = table->Get(&machine, "currentScreenX");
    gmVariable y = table->Get(&machine, "currentScreenY");
    if (x.m_type == GM_INT) screenX = x.m_value.m_int;
    if (y.m_type == GM_INT) screenY = y.m_value.m_int;
    return true;
}

bool EntityVisibilityProcessMatches(gmMachine& machine, gmTableObject* entity,
                                    gmptr expectedProcessRef) {
    gmVariable owner = entity->Get(&machine, "__behaviorOwner");
    gmTableObject* behavior = owner.m_type == GM_TABLE
        ? (gmTableObject*)machine.GetObject(owner.m_value.m_ref) : entity;
    gmVariable process = behavior ? behavior->Get(&machine, "gameProcess") : gmVariable::s_null;
    if (process.m_type != GM_TABLE) return true;
    return expectedProcessRef != static_cast<gmptr>(-1) &&
           process.m_value.m_ref == expectedProcessRef;
}

// Reuse the exact inclusive screen predicate, but cache its result by active
// screen and entity-registry revision. A process with a second, independent
// screen coordinate system makes this shortcut unsafe, so retain the old
// full-walk path in that case.
bool GetCachedVisibleEntityIds(gmMachine& machine, bool a_unboundedVisible,
                               std::vector<int>& a_ids) {
    EntityVisibilitySnapshot& snapshot = VisibilitySnapshot();
    const auto& video = GetEngineContext().video;
    gmVariable processVar = gmVariable::s_null;
    int processX = video.currentScreenX;
    int processY = video.currentScreenY;
    const bool hasProcess = CurrentVisibilityProcess(machine, processVar, processX, processY);
    const gmptr processRef = hasProcess ? processVar.m_value.m_ref
                                        : static_cast<gmptr>(-1);
    const bool keyMatches = snapshot.initialized &&
        snapshot.revision == Entities().visibilityRevision &&
        snapshot.processRef == processRef && snapshot.processX == processX &&
        snapshot.processY == processY && snapshot.videoX == video.currentScreenX &&
        snapshot.videoY == video.currentScreenY;
    if (!keyMatches) {
        const bool traceCull = std::getenv("GE3_TRACE_ENTITY_CULL") != nullptr;
        const bool traceScreenChange = traceCull && snapshot.initialized &&
            (snapshot.processX != processX ||
            snapshot.processY != processY || snapshot.videoX != video.currentScreenX ||
            snapshot.videoY != video.currentScreenY);
        const bool traceVisibilityChanges = traceCull && snapshot.initialized;
        const int previousX = snapshot.processX;
        const int previousY = snapshot.processY;
        const std::vector<int> previousBoundedVisible = traceVisibilityChanges
            ? snapshot.boundedVisible : std::vector<int>{};
        snapshot.unboundedVisible.clear();
        snapshot.boundedVisible.clear();
        snapshot.revision = Entities().visibilityRevision;
        snapshot.processRef = processRef;
        snapshot.processX = processX;
        snapshot.processY = processY;
        snapshot.videoX = video.currentScreenX;
        snapshot.videoY = video.currentScreenY;
        snapshot.compatible = true;
        int processlessBounded = 0;
        int processlessVisible = 0;

        for (const auto& entry : Entities().entities) {
            if (entry.second.m_type != GM_TABLE) continue;
            gmTableObject* table = (gmTableObject*)machine.GetObject(entry.second.m_value.m_ref);
            if (!table) continue;
            if (!EntityVisibilityProcessMatches(machine, table, processRef)) {
                snapshot.compatible = false;
                break;
            }
            if (traceScreenChange) {
                gmVariable owner = table->Get(&machine, "__behaviorOwner");
                gmTableObject* behavior = owner.m_type == GM_TABLE
                    ? (gmTableObject*)machine.GetObject(owner.m_value.m_ref) : table;
                const gmVariable process = behavior
                    ? behavior->Get(&machine, "gameProcess") : gmVariable::s_null;
                if (process.m_type != GM_TABLE) {
                    gmVariable bounds = table->Get(&machine, "__screenMinCX");
                    if (bounds.m_type == GM_NULL && behavior)
                        bounds = behavior->Get(&machine, "__screenMinCX");
                    if (bounds.m_type == GM_NULL && behavior) {
                        gmVariable handle = behavior->Get(&machine, "Entity");
                        if (handle.m_type == GM_TABLE) {
                            gmTableObject* handleTable = (gmTableObject*)machine.GetObject(
                                handle.m_value.m_ref);
                            if (handleTable)
                                bounds = handleTable->Get(&machine, "__screenMinCX");
                        }
                    }
                    if (bounds.m_type != GM_NULL) {
                        ++processlessBounded;
                        if (EntityIsOnCurrentScreen(machine, table, true))
                            ++processlessVisible;
                    }
                }
            }
            if (EntityIsOnCurrentScreen(machine, table, true))
                snapshot.unboundedVisible.push_back(entry.first);
            if (EntityIsOnCurrentScreen(machine, table, false))
                snapshot.boundedVisible.push_back(entry.first);
        }
        snapshot.initialized = true;
        if (traceScreenChange) {
            Renderer* renderer = GetEngineContext().renderer;
            std::fprintf(stderr,
                "[entity-cull] processCell=(%d,%d)->(%d,%d) videoCell=(%d,%d) "
                "camera=(%.3f,%.3f) visible=%zu/%zu\n",
                previousX, previousY, processX, processY,
                video.currentScreenX, video.currentScreenY,
                renderer ? renderer->GetCameraX() : 0.0f,
                renderer ? renderer->GetCameraY() : 0.0f,
                snapshot.unboundedVisible.size(), Entities().entities.size());
            std::fprintf(stderr,
                "[entity-cull] processlessBounded=%d selectedByVideoFallback=%d\n",
                processlessBounded, processlessVisible);
        }
        if (traceVisibilityChanges && snapshot.compatible) {
            std::set<int> previousIds(previousBoundedVisible.begin(),
                                      previousBoundedVisible.end());
            std::set<int> currentIds(snapshot.boundedVisible.begin(),
                                     snapshot.boundedVisible.end());
            auto printTransitions = [&](const char* a_label,
                                        const std::set<int>& a_from,
                                        const std::set<int>& a_to) {
                size_t count = 0;
                for (int id : a_from) {
                    if (a_to.find(id) != a_to.end()) continue;
                    ++count;
                    const auto entityIt = Entities().entitiesByObjectID.find(id);
                    gmTableObject* table = nullptr;
                    if (entityIt != Entities().entitiesByObjectID.end() &&
                        entityIt->second.m_type == GM_TABLE) {
                        table = (gmTableObject*)machine.GetObject(
                            entityIt->second.m_value.m_ref);
                    }
                    const char* typeName = "<removed>";
                    int minX = 0, maxX = 0, minY = 0, maxY = 0;
                    int enabled = -1, invisible = 0;
                    bool hasScriptDraw = false, hasDrawList = false;
                    const char* ownerName = "<none>";
                    if (table) {
                        gmVariable type = table->Get(&machine, "__typeName");
                        if (type.m_type == GM_STRING && type.m_value.m_ref) {
                            auto* string = (gmStringObject*)machine.GetObject(
                                type.m_value.m_ref);
                            if (string) typeName = string->GetString();
                        }
                        auto readCell = [&](const char* a_key) {
                            gmVariable cell = table->Get(&machine, a_key);
                            return cell.m_type == GM_INT
                                ? static_cast<int>(cell.m_value.m_int) : 0;
                        };
                        minX = readCell("__screenMinCX");
                        maxX = readCell("__screenMaxCX");
                        minY = readCell("__screenMinCY");
                        maxY = readCell("__screenMaxCY");
                        gmVariable enabledValue = table->Get(&machine, "__enabled");
                        if (enabledValue.m_type == GM_INT)
                            enabled = enabledValue.m_value.m_int;
                        gmVariable invisibleValue = table->Get(&machine, "__invisible");
                        if (invisibleValue.m_type == GM_INT)
                            invisible = invisibleValue.m_value.m_int;
                        hasScriptDraw = table->Get(&machine, "draw").m_type == GM_FUNCTION;
                        hasDrawList = table->Get(&machine, "drawList").m_type >= GM_USER;
                        gmVariable process = table->Get(&machine, "gameProcess");
                        if (process.m_type != GM_TABLE) {
                            gmVariable owner = table->Get(&machine, "__behaviorOwner");
                            gmTableObject* behavior = owner.m_type == GM_TABLE
                                ? (gmTableObject*)machine.GetObject(owner.m_value.m_ref)
                                : nullptr;
                            if (behavior)
                                process = behavior->Get(&machine, "gameProcess");
                        }
                        if (process.m_type == GM_TABLE) {
                            gmTableObject* processTable = (gmTableObject*)machine.GetObject(
                                process.m_value.m_ref);
                            if (processTable) {
                                gmVariable processType = processTable->Get(
                                    &machine, "__typeName");
                                if (processType.m_type == GM_STRING &&
                                    processType.m_value.m_ref) {
                                    auto* string = (gmStringObject*)machine.GetObject(
                                        processType.m_value.m_ref);
                                    if (string) ownerName = string->GetString();
                                }
                            }
                        }
                    }
                    std::fprintf(stderr,
                        "[entity-cull-%s] id=%d type=%s bounds=(%d..%d,%d..%d) "
                        "owner=%s enabled=%d invisible=%d draw=%d drawList=%d\n",
                        a_label, id, typeName, minX, maxX, minY, maxY, ownerName,
                        enabled, invisible, hasScriptDraw ? 1 : 0, hasDrawList ? 1 : 0);
                }
                if (count != 0)
                    std::fprintf(stderr, "[entity-cull-%s] count=%zu\n", a_label, count);
            };
            printTransitions("left", previousIds, currentIds);
            printTransitions("entered", currentIds, previousIds);
        }
    }
    if (!snapshot.compatible) return false;
    a_ids = a_unboundedVisible ? snapshot.unboundedVisible : snapshot.boundedVisible;
    return true;
}
} // namespace

int GM_CDECL gmf_entity_isOnCurrentScreen(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    a_thread->PushInt(EntityIsOnCurrentScreen(*a_thread->GetMachine(), self));
    return GM_OK;
}
int GM_CDECL gmf_entity_isOnScreen(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& machine = *a_thread->GetMachine();
    // Retail EBOOT 0x1c7c38 converts the first argument's raw value word to
    // float, then compares it against both axes, even if a second was passed.
    const float cell = (float)a_thread->Param(0).m_value.m_int;
    auto bound = [&](const char* key) {
        gmVariable value = self->Get(&machine, key);
        return value.m_type == GM_FLOAT ? value.m_value.m_float
            : value.m_type == GM_INT ? (float)value.m_value.m_int : 0.0f;
    };
    a_thread->PushInt(bound("__screenMinCX") <= cell && cell <= bound("__screenMaxCX") &&
                      bound("__screenMinCY") <= cell && cell <= bound("__screenMaxCY"));
    return GM_OK;
}
// initializeScreenExtents(minx,maxx,miny,maxy) takes screen-cell indices
// already serialized by the level editor. The Ghidra body at 0x1c7f00
// fails to decode, but its call site and the four recordScreenExtents fields
// establish the observable contract.
int GM_CDECL gmf_entity_initializeScreenExtents(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(4);
    GM_INT_PARAM(minX, 0, 0);
    GM_INT_PARAM(maxX, 1, 0);
    GM_INT_PARAM(minY, 2, 0);
    GM_INT_PARAM(maxY, 3, 0);
    StoreScreenExtents(*a_thread->GetMachine(), self, minX, maxX, minY, maxY);
    return GM_OK;
}

// 0x0009bef4). This marker suppresses native update membership. The no-physics
// counterpart is implemented below using its verified manager memberships.
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
    SetEntityNativeField(*a_thread->GetMachine(), self, "__beingEdited", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isBeingEdited(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__beingEdited");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_setIsPlacing(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    SetEntityNativeField(*a_thread->GetMachine(), self, "__placing", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isPlacing(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__placing");
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
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__sauceBase");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_setIsSauceBase(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    gmint flag = a_thread->ParamInt(0, 0);
    SetEntityNativeField(*a_thread->GetMachine(), self, "__sauceBase", gmVariable((gmint)(flag ? 1 : 0)));
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
    SetEntityNativeField(*a_thread->GetMachine(), self, "__alwaysProcessed", gmVariable((gmint)(flag ? 1 : 0)));
    return GM_OK;
}
int GM_CDECL gmf_entity_isAlwaysProcessed(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__alwaysProcessed");
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
// on the entity handle, same shape as setIsInvisible/isInvisible above.
// getAudibilityPerc consumes this field for script-visible music-trigger
// decisions; audio output itself still lacks the retail mixer.
int GM_CDECL gmf_entity_setNumOffScreensAudible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(n, 0, 0);
    SetEntityNativeField(*a_thread->GetMachine(), self, "__numOffScreensAudible", gmVariable((gmint)n));
    return GM_OK;
}
int GM_CDECL gmf_entity_getNumOffScreensAudible(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__numOffScreensAudible");
    a_thread->PushInt(v.m_type == GM_INT ? v.m_value.m_int : 0);
    return GM_OK;
}
int GM_CDECL gmf_entity_set_getAudibilityPerc_neverOnScreen(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(flag, 0, 0);
    SetEntityNativeField(*a_thread->GetMachine(), self, "__audibilityNeverOnScreen", gmVariable((gmint)(flag ? 1 : 0)));
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
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__audibilityNeverOnScreen");
    a_thread->PushInt((v.m_type == GM_INT && v.m_value.m_int != 0) ? 1 : 0);
    return GM_OK;
}
// Compatibility bridge for note-state calls made on raw EntityManager
// handles. BaseNotePlant.gm defines setGot()/isGot() on its behavior table;
// the native handle and behavior table are separate desktop objects, so these
// calls must read/write the behavior's `got` field as well. The shipped
// Main.gm also calls isGetting(), but no matching native method appears in the
// recovered retail EntityBase method inventory, and BaseNotePlant has no
// `getting` field or transition setter. Keep this false-valued shim for the
// local note-list workaround, without claiming it is a retail native method.
int GM_CDECL gmf_entity_setGot(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    gmMachine& machine = *a_thread->GetMachine();
    // BaseNotePlant.gm's script setter calls onGot() when the requested state
    // is true. The native Entity facade is a separate table in this port, so
    // preserve that retail script path when a raw Entity handle receives the
    // call; the behavior method is saved before native methods are injected.
    gmVariable owner = self->Get(&machine, "__behaviorOwner");
    gmTableObject* ownerTable = owner.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine.GetObject(owner.m_value.m_ref))
        : nullptr;
    gmVariable scriptSetter = ownerTable
        ? ownerTable->Get(&machine, "__retailScriptSetGot") : gmVariable::s_null;
    if (ownerTable && scriptSetter.m_type == GM_FUNCTION) {
        gmCall call;
        if (call.BeginTableFunction(&machine, "__retailScriptSetGot",
                                    ownerTable, owner)) {
            if (a_thread->GetNumParams() > 0)
                call.AddParam(a_thread->Param(0));
            call.End();
        }
        gmVariable value = ownerTable->Get(&machine, "got");
        if (value.m_type == GM_NULL) value = gmVariable((gmint)0);
        SetEntityNativeField(machine, self, "got", value);
        SetEntityNativeField(machine, self, "__got", value);
        return GM_OK;
    }

    gmint flag = a_thread->ParamInt(0, 0);
    gmVariable value((gmint)(flag ? 1 : 0));
    // BaseNotePlant.gm owns its public collected state in `this.got`, while
    // the native Entity facade also exposes setGot/isGot. Keep both views
    // synchronized so calls routed through the native method cannot leave the
    // script renderer seeing a stale `got=true` after level retry.
    SetEntityNativeField(machine, self, "got", value);
    SetEntityNativeField(machine, self, "__got", value);
    return GM_OK;
}
int GM_CDECL gmf_entity_isGot(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmMachine& machine = *a_thread->GetMachine();
    // A note's collision callback sets `got=true` directly on its behavior
    // table, while the native Entity handle may still hold the `got=false`
    // value written earlier by setGot(false). Prefer the behavior's live
    // script field when called through the raw handle; otherwise the stale
    // handle value masks a collected note during checkpoint snapshots.
    gmTableObject* paired = PairedEntityTable(machine, self);
    const bool selfIsHandle = self->Get(&machine, "__behaviorOwner").m_type == GM_TABLE;
    gmTableObject* primary = selfIsHandle && paired ? paired : self;
    gmTableObject* secondary = primary == self ? paired : self;
    gmVariable scriptValue = primary->Get(&machine, "got");
    if (scriptValue.m_type == GM_NULL && secondary)
        scriptValue = secondary->Get(&machine, "got");
    if (scriptValue.m_type == GM_INT) {
        a_thread->PushInt(scriptValue.m_value.m_int != 0 ? 1 : 0);
        return GM_OK;
    }
    gmVariable v = primary->Get(&machine, "__got");
    if (v.m_type == GM_NULL && secondary)
        v = secondary->Get(&machine, "__got");
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
    gmVariable beingEdited = GetEntityNativeField(m, self, "__beingEdited");
    gmVariable placing = GetEntityNativeField(m, self, "__placing");
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
// retail's native callbacks also notify the player renderer: cppDie sets
// its scale transition flag and resets scale to one; cppRebirth clears the
// flag and resets scale. The player renderer advances that native scale on
// each fixed simulation tick.
int GM_CDECL gmf_entity_cppDie(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* self = a_thread->ThisTable();
    SetEntityNativeField(machine, self, "__nativePlayerDrawScale", gmVariable(1.0f));
    SetEntityNativeField(machine, self, "__nativePlayerDrawScaleTransition", gmVariable((gmint)1));
    return GM_OK;
}
int GM_CDECL gmf_entity_cppRebirth(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* self = a_thread->ThisTable();
    SetEntityNativeField(machine, self, "__nativePlayerDrawScale", gmVariable(1.0f));
    SetEntityNativeField(machine, self, "__nativePlayerDrawScaleTransition", gmVariable((gmint)0));
    return GM_OK;
}

// PlayerGet registers these on the player native object only (retail method
// table at 0x398d14..0x398d6c). Entity handles for ordinary entities do not
// have cppDie/cppRebirth; install the adapters only when the distinct
// createEntityAsPlayer factory returns. The desktop represents retail's
// unified object with a behavior table and an Entity handle, so both views
// receive the same native callbacks.
void InjectPlayerRendererLifecycleMethods(gmMachine& a_machine,
                                          gmTableObject* a_table) {
    if (!a_table) return;
    if (a_table->Get(&a_machine, "cppDie").m_type != GM_FUNCTION)
        a_table->Set(&a_machine, "cppDie",
            gmVariable(a_machine.AllocFunctionObject(gmf_entity_cppDie)));
    if (a_table->Get(&a_machine, "cppRebirth").m_type != GM_FUNCTION)
        a_table->Set(&a_machine, "cppRebirth",
            gmVariable(a_machine.AllocFunctionObject(gmf_entity_cppRebirth)));
}
// Verification: retail PlayerGet.testfn at FUN_001a2f28 reads/writes the
// process-wide DAT_00424f48 boolean. Zero args returns it as an integer-tagged
// GM boolean; one arg stores the raw 32-bit value word's truth value; more
// than one arg does nothing. The raw word matters here (e.g. negative zero's
// float bits are nonzero), so do not use ParamInt()/ParamFloat() conversion.
int GM_CDECL gmf_player_testfn(gmThread* a_thread) {
    static bool enabled = false; // BSS DAT_00424f48 starts at zero in retail.
    const int argc = a_thread->GetNumParams();
    if (argc == 0) {
        a_thread->PushInt(enabled ? 1 : 0);
    } else if (argc == 1) {
        enabled = a_thread->Param(0).m_value.m_int != 0;
    }
    return GM_OK;
}
// Retail 0x001c5c74 clears the cached audibility tick. This is used by
// checkpoint restore to force a fresh value even within the same frame.
int GM_CDECL gmf_entity_forceAudibilityRecalc(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    SetEntityNativeField(*a_thread->GetMachine(), self, "__lastAudibilityTick", gmVariable::s_null);
    return GM_OK;
}
// EntityBase.gm's C++ reference computes an entity's current screen from
// PhysicsData.getBody(0).getPos(), then gameProcess.getScreenX/Y(). Retail
// native getCurrentScreenX/Y call separate entity-position helpers. The
// previous implementation read the CAMERA'S current screen, so every entity
// appeared to be on screen for audibility and screen-specific callbacks.
int EntityScreenCell(gmMachine& m, gmTableObject* self, bool xAxis) {
    gmVariable pd = self->Get(&m, "PhysicsData");
    if (pd.m_type == GM_NULL) pd = self->Get(&m, "__physicsData");
    float x = 0.0f, y = 0.0f;
    if (!GetPhysicsDataPos(m, pd, x, y)) {
        gmVariable cd = self->Get(&m, "configureData");
        if (cd.m_type == GM_TABLE) {
            gmTableObject* config = (gmTableObject*)m.GetObject(cd.m_value.m_ref);
            auto read = [&](const char* key) {
                gmVariable v = config->Get(&m, key);
                return v.m_type == GM_FLOAT ? v.m_value.m_float
                    : v.m_type == GM_INT ? (float)v.m_value.m_int : 0.0f;
            };
            x = read("posX");
            y = read("posY");
        }
    }
    return WorldToScreenCell(xAxis ? x : y, xAxis ? kScreenCellWidth : kScreenCellHeight);
}

// EntityBase.gm:2300-2341 preserves the retired script implementation of
// this C++ method. Retail's binding at 0x001c5ca8 delegates to the entity's
// virtual getAudibilityPerc. Use that reference for the visible behavior:
// one calculation per tick, Manhattan screen distance, and the
// never-on-screen latch that clears only when the entity reaches the camera.
int GM_CDECL gmf_entity_getAudibilityPerc(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    const int tick = GetEngineContext().gameSystem.currentTick;
    gmVariable lastTick = GetEntityNativeField(m, self, "__lastAudibilityTick");
    if (lastTick.m_type == GM_INT && lastTick.m_value.m_int == tick) {
        gmVariable cached = GetEntityNativeField(m, self, "__audibilityPercCached");
        if (cached.m_type == GM_FLOAT || cached.m_type == GM_INT) {
            a_thread->Push(cached);
            return GM_OK;
        }
    }

    int cameraX = 0, cameraY = 0;
    gmVariable gp = self->Get(&m, "gameProcess");
    // Main.gm's music-trigger lists store the raw Entity handle. Retail has
    // one entity object, but this port keeps gameProcess on its behavior
    // table; use that owner when the handle has no direct field. Otherwise
    // every off-screen audibility query compares against screen (0,0).
    if (gp.m_type == GM_NULL) {
        gmVariable owner = self->Get(&m, "__behaviorOwner");
        if (owner.m_type == GM_TABLE) {
            gmTableObject* behavior = (gmTableObject*)m.GetObject(owner.m_value.m_ref);
            if (behavior) gp = behavior->Get(&m, "gameProcess");
        }
    }
    if (gp.m_type == GM_TABLE) {
        gmTableObject* process = (gmTableObject*)m.GetObject(gp.m_value.m_ref);
        if (process) {
            auto readCell = [&](const char* key) {
                gmVariable v = process->Get(&m, key);
                return v.m_type == GM_FLOAT ? (int)v.m_value.m_float
                    : v.m_type == GM_INT ? (int)v.m_value.m_int : 0;
            };
            cameraX = readCell("currentScreenX");
            cameraY = readCell("currentScreenY");
        }
    }

    // Audio registration is based on the entity's recorded screen extent.
    // A SwayingTree can have its root in row 0 while its long head and stem
    // reach row 1. Using only body 0's cell here returned zero audibility
    // on row 1, so its onMusicTrigger never resynchronized swayPerc and the
    // head travelled indefinitely away from the fixed-length stem.
    // Measure the distance to the nearest registered cell. For entities
    // without recorded bounds, retain the body's position as the source.
    auto bound = [&](const char* key, int& out) {
        gmVariable value = GetEntityNativeField(m, self, key);
        if (value.m_type == GM_INT) { out = value.m_value.m_int; return true; }
        if (value.m_type == GM_FLOAT) { out = (int)value.m_value.m_float; return true; }
        return false;
    };
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    const bool hasBounds = bound("__screenMinCX", minX) && bound("__screenMaxCX", maxX) &&
                           bound("__screenMinCY", minY) && bound("__screenMaxCY", maxY) &&
                           minX <= maxX && minY <= maxY;
    const int distance = hasBounds
        ? std::max({minX - cameraX, 0, cameraX - maxX}) +
          std::max({minY - cameraY, 0, cameraY - maxY})
        : std::abs(EntityScreenCell(m, self, true) - cameraX) +
          std::abs(EntityScreenCell(m, self, false) - cameraY);
    gmVariable neverVar = GetEntityNativeField(m, self, "__audibilityNeverOnScreen");
    bool neverOnScreen = neverVar.m_type == GM_INT && neverVar.m_value.m_int != 0;
    if (neverOnScreen && distance == 0) {
        SetEntityNativeField(m, self, "__audibilityNeverOnScreen", gmVariable((gmint)0));
        neverOnScreen = false;
    }

    gmVariable maxVar = GetEntityNativeField(m, self, "__numOffScreensAudible");
    const int maxScreens = maxVar.m_type == GM_INT ? maxVar.m_value.m_int : 0;
    const float value = neverOnScreen ? 0.0f : distance == 0 ? 1.0f
        : maxScreens > 0 && distance <= maxScreens
            ? 1.0f - (float)distance / (float)(maxScreens + 1) : 0.0f;
    static const bool s_traceCraigMusic =
        std::getenv("GE3_TRACE_CRAIG_MUSIC") != nullptr;
    if (s_traceCraigMusic) {
        gmVariable type = self->Get(&m, "__typeName");
        const char* typeName = type.m_type == GM_STRING && type.m_value.m_ref
            ? static_cast<gmStringObject*>(m.GetObject(type.m_value.m_ref))->GetString()
            : "?";
        if (std::strstr(typeName, "CraigBoardroomExec") ||
            std::strstr(typeName, "CraigCoffeeDrinker")) {
            gmVariable id = self->Get(&m, "__objectID");
            std::fprintf(stderr,
                "[craig-audibility] id=%d type=%s camera=(%d,%d) distance=%d "
                "range=%d never=%d value=%.3f\n",
                id.m_type == GM_INT ? id.m_value.m_int : -1, typeName,
                cameraX, cameraY, distance, maxScreens, neverOnScreen ? 1 : 0,
                value);
        }
    }
    SetEntityNativeField(m, self, "__lastAudibilityTick", gmVariable((gmint)tick));
    SetEntityNativeField(m, self, "__audibilityPercCached", gmVariable(value));
    a_thread->PushFloat(value);
    return GM_OK;
}

int GM_CDECL gmf_entity_getCurrentScreenY(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    a_thread->PushInt(EntityScreenCell(*a_thread->GetMachine(), self, false));
    return GM_OK;
}
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
    gmVariable v = GetEntityNativeField(*a_thread->GetMachine(), self, "__isPlayer");
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
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    a_thread->PushInt(EntityScreenCell(m, self, true));
    return GM_OK;
}

void SetEntityFlag(gmThread* a_thread, const char* a_flag) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return;
    SetEntityNativeField(*a_thread->GetMachine(), self, a_flag, gmVariable((gmint)1));
    if (std::strcmp(a_flag, "__noPhysicsUpdate") == 0)
        InvalidatePhysicsMovementMembership();
}

void RemoveEntityPhysicsContactPairs(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return;
    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable id = GetEntityNativeField(machine, self, "__objectID");
    if (id.m_type == GM_INT) RemovePhysicsContactPairsForEntity(id.m_value.m_int);
}

// Retail FUN_000bfc04 erases the entity from the PhysicsManager update map,
// removes contact records that reference it, then clears PhysicsData flags
// +0x48/+0x49. The desktop removes its corresponding active pair memberships
// and retains marker fields for update-map/contact-record eligibility.
int GM_CDECL gmf_entity_constructorSetNoPhysics(gmThread* a_thread) {
    SetEntityFlag(a_thread, "__noPhysicsUpdate");
    SetEntityFlag(a_thread, "__noPhysicsCollide");
    RemoveEntityPhysicsContactPairs(a_thread);
    return GM_OK;
}
// Retail FUN_000bfbbc removes records referencing this entity from registered
// PhysicsData contact maps through FUN_00036f48, then clears +0x49. The local
// pair registry now mirrors the observable removal; future contact records
// are still admitted per side using __isManual/__noPhysicsCollide.
int GM_CDECL gmf_entity_constructorSetNoPhysicsCollide(gmThread* a_thread) {
    SetEntityFlag(a_thread, "__noPhysicsCollide");
    RemoveEntityPhysicsContactPairs(a_thread);
    return GM_OK;
}
int GM_CDECL gmf_entity_constructorSetNoUpdate(gmThread* a_thread) {
    // Retail FUN_000bfa90 removes this entity from its per-frame update
    // system and clears the native update marker at entity+0x30.
    SetEntityFlag(a_thread, "__noUpdate");
    return GM_OK;
}
// Retail constructorSetIsManual at 0x0009bf44 is an unconditional `li r3,-1; blr`.
int GM_CDECL gmf_entity_constructorSetIsManual(gmThread*) {
    return GM_EXCEPTION;
}
// constructorSetManual/constructorSetNoPhysicsUpdate are real retail calls
// (Entity.c 0x0009bdb4/0x0009be04). Manual membership affects the distinct
// EntityManager list/count APIs. The no-physics-update marker controls the
// PhysicsManager map traversed by FUN_0035d28.
int GM_CDECL gmf_entity_constructorSetManual(gmThread* a_thread) {
    // Retail FUN_000bfac8 marks the native entity as manual before linking
    // it into EntityManager's manual list (Entity.c:constructorSetManual).
    // It also clears PhysicsData +0x48/+0x49 and unregisters the entity from
    // PhysicsManager. In this table-backed adapter those PhysicsData flags
    // are represented by the same opt-out markers used by the dedicated
    // no-physics setters, so the movement tick and per-side contact creation
    // both stop for a manual entity.
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    SetEntityNativeField(*a_thread->GetMachine(), self, "__isManual", gmVariable((gmint)1));
    SetEntityFlag(a_thread, "__noPhysicsUpdate");
    SetEntityFlag(a_thread, "__noPhysicsCollide");
    RemoveEntityPhysicsContactPairs(a_thread);
    return GM_OK;
}
int GM_CDECL gmf_entity_constructorSetNoPhysicsUpdate(gmThread* a_thread) {
    SetEntityFlag(a_thread, "__noPhysicsUpdate");
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

// Retail Entity.createParticle accepts no argument or one particle type name,
// but its factory at 0x000388b8 is an unconditional zero return. The following
// manager insertion is a null guard, then the wrapper returns GM_NULL. Preserve
// that shipped behavior rather than inventing a desktop particle instance.
int GM_CDECL gmf_entity_createParticle(gmThread* a_thread) {
    const int argc = a_thread->GetNumParams();
    if (argc > 1) return GM_EXCEPTION;
    if (argc == 1 && a_thread->Param(0).m_type != GM_STRING)
        return GM_EXCEPTION;
    a_thread->PushNull();
    return GM_OK;
}

// Retail's deleteAllParticles clears the two native ParticleManager lists at
// GameSystem+0xe5c. The current desktop particle factory always returns null,
// so there are no locally owned native particles to destroy; keep the method
// callable while the broader particle-manager API is audited.
int GM_CDECL gmf_entity_deleteAllParticles(gmThread*) {
    return GM_OK;
}

void PushEntityListIterator(gmThread* a_thread, std::vector<gmVariable> a_entities);
int GM_CDECL gmf_entity_getAllParticles(gmThread* a_thread) {
    // No local ParticleManager lists exist yet; return the same empty iterator
    // shape used by EntityManager queries until its native list writers land.
    PushEntityListIterator(a_thread, {});
    return GM_OK;
}
int GM_CDECL gmf_entity_getNumParticles(gmThread* a_thread) {
    // Retail reads the first ParticleManager list's count. The local manager
    // currently has no particle-list writers, so its represented count is 0.
    a_thread->PushInt(0);
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
// projects (camera-dependent world-view extent, aspect, screenViewScale -- mirrored here
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
    // (scales with camera Z) -- see Renderer::GetWorldHalfHeight().
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
int GM_CDECL gmf_entity_unloadAnimation(gmThread* a_thread);
int GM_CDECL gmf_entity_unloadAnimationAll(gmThread* a_thread);

int GM_CDECL gmf_entity_playSoundWithMixer(gmThread* a_thread);
int GM_CDECL gmf_entity_playInstrumentWithMixer(gmThread* a_thread);

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
// Retail Entity.setupStaticDrawList (EBOOT 0x9ae14) uses the same
// 0/1/2-argument dimensions and defaults (0x400, 0x20), but allocates
// a static draw list. The portable draw-list wrapper records/replays the
// commands directly, so both entry points share its allocation.
int GM_CDECL gmf_entity_setupStaticDrawList(gmThread* a_thread) {
    return gmf_entity_setupDrawList(a_thread);
}

// Retail EBOOT exports refreshFunctionCache on Entity. Most callbacks here
// resolve from the behavior table at dispatch time, but Main.gm tests the raw
// handle's onMusicTrigger member when registering music-trigger entities.
// Keep that cached handle member in step after a script changes the callback.
int GM_CDECL gmf_entity_refreshFunctionCache(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable owner = self->Get(&m, "__behaviorOwner");
    gmTableObject* behavior = owner.m_type == GM_TABLE
        ? (gmTableObject*)m.GetObject(owner.m_value.m_ref) : self;
    gmTableObject* handle = owner.m_type == GM_TABLE
        ? self : PairedEntityTable(m, self);
    if (handle && handle != behavior) {
        gmVariable callback = behavior->Get(&m, "onMusicTrigger");
        handle->Set(&m, "onMusicTrigger", callback.m_type == GM_FUNCTION
            ? gmVariable(m.AllocFunctionObject(gmf_entity_delegate_onMusicTrigger))
            : gmVariable::s_null);
    }
    return GM_OK;
}

// The retail Entity.getVertexShaderParamIndex wrapper (EBOOT 0x0009a728)
// selects the entity shader by slot and looks up the name in that shader's
// ordered parameter records. These orders come from the shader descriptors
// embedded at EBOOT 0x00424f98 and 0x00425f94..0x00426128.
int GM_CDECL gmf_entity_getVertexShaderParamIndex(gmThread* a_thread) {
    const int argc = a_thread->GetNumParams();
    if (argc != 1 && argc != 2) return GM_EXCEPTION;
    const int slot = argc == 2 ? a_thread->Param(0).GetInt() : 0;
    const gmVariable nameVar = a_thread->Param(argc - 1);
    if (nameVar.m_type != GM_STRING || (argc == 2 && a_thread->Param(0).m_type != GM_INT))
        return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    const gmVariable typeVar = self->Get(&machine, "__typeName");
    if (typeVar.m_type != GM_STRING) return GM_EXCEPTION;
    const char* type = ((gmStringObject*)machine.GetObject(typeVar.m_value.m_ref))->GetString();
    const char* name = ((gmStringObject*)machine.GetObject(nameVar.m_value.m_ref))->GetString();

    static const char* player[] = {"aPosition", "aDetail", "aViewWorldProjection", "aColor", "pulsePerc", "pulseDir"};
    static const char* bounceBody[] = {"aPosition", "aData", "aViewWorldProjection", "aColour", "aCurve"};
    static const char* bounceBand[] = {"aPosition", "aData", "aViewWorldProjection", "aColour", "aAmpAngStart", "aAmpAngLen", "aMaxAmp", "aRadiusX", "aRadiusY"};
    static const char* laserBody[] = {"aPosition", "aData", "aViewWorldProjection", "aColour", "aMaxAmp", "aTopMinA", "aHitPulsePerc", "aRebirthPulsePerc"};
    static const char* laserBeam[] = {"aPosition", "aData", "aViewWorldProjection", "aColour", "aStartAng", "aMinHTH", "aMaxAmp", "aPercLen", "aLaserLen"};
    const char* const* params = nullptr;
    size_t count = 0;
    if (std::strcmp(type, "Platformer_Player") == 0 && slot == 0) {
        params = player; count = std::size(player);
    } else if (std::strcmp(type, "Platformer_EntityPacks_DSBounceThing") == 0) {
        if (slot == 0) { params = bounceBody; count = std::size(bounceBody); }
        if (slot == 1) { params = bounceBand; count = std::size(bounceBand); }
    } else if (std::strcmp(type, "Platformer_EntityPacks_KickLaser") == 0 ||
               std::strcmp(type, "Platformer_EntityPacks_SnareLaser") == 0) {
        if (slot == 0) { params = laserBody; count = std::size(laserBody); }
        if (slot == 1) { params = laserBeam; count = std::size(laserBeam); }
    }
    for (size_t i = 0; i < count; ++i) {
        if (std::strcmp(params[i], name) == 0) {
            a_thread->PushInt((gmint)i);
            return GM_OK;
        }
    }
    // GE3_Internal_ShaderParameterNameToIndex logs a miss and returns index 0;
    // it does not raise a GameMonkey exception. Preserve that numeric result
    // for known reflected shader layouts.
    std::fprintf(stderr,
        "Error: Param %s not found in shader (GetShaderParamID); slot %d on %s\n",
        name, slot, type);
    a_thread->PushInt(0);
    return GM_OK;
}

void InjectEntityHandleMethods(gmMachine& a_machine, gmTableObject* a_table, int a_uid,
                               const std::string& a_typeName, int a_physicsType) {
    // Retail entities start with an unregistered script UID (-1). Their
    // stable native object ID is assigned independently at construction.
    a_table->Set(&a_machine, "__uid", gmVariable((gmint)-1));
    a_table->Set(&a_machine, "__objectID", gmVariable((gmint)a_uid));
    a_table->Set(&a_machine, "__physicsType", gmVariable((gmint)a_physicsType));
    a_table->Set(&a_machine, "__typeId", gmVariable((gmint)InternTypeId(a_typeName)));
    a_table->Set(&a_machine, "__typeName", gmVariable(a_machine.AllocStringObject(a_typeName.c_str())));
    a_table->Set(&a_machine, "getType", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getType)));
    a_table->Set(&a_machine, "getTypeName", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getTypeName)));
    a_table->Set(&a_machine, "createParticle", gmVariable(a_machine.AllocFunctionObject(gmf_entity_createParticle)));
    a_table->Set(&a_machine, "deleteAllParticles", gmVariable(a_machine.AllocFunctionObject(gmf_entity_deleteAllParticles)));
    a_table->Set(&a_machine, "getAllParticles", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getAllParticles)));
    a_table->Set(&a_machine, "getNumParticles", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getNumParticles)));
    a_table->Set(&a_machine, "getPhysicsData", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getPhysicsData)));
    a_table->Set(&a_machine, "refreshFunctionCache", gmVariable(a_machine.AllocFunctionObject(gmf_entity_refreshFunctionCache)));
    // EntityBase installs these script methods on the returned behavior
    // table. Main.gm's globalEntitiesMap stores raw Entity handles and calls
    // isAlwaysMusicTriggered on them during the music tick. Retail has one
    // object for both views; delegate through our split handle/owner pair.
    if (a_table->Get(&a_machine, "isAlwaysMusicTriggered").m_type != GM_FUNCTION)
        a_table->Set(&a_machine, "isAlwaysMusicTriggered",
            gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_isAlwaysMusicTriggered)));
    if (a_table->Get(&a_machine, "setIsAlwaysMusicTriggered").m_type != GM_FUNCTION)
        a_table->Set(&a_machine, "setIsAlwaysMusicTriggered",
            gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_setIsAlwaysMusicTriggered)));
    a_table->Set(&a_machine, "getMainBody", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMainBody)));
    a_table->Set(&a_machine, "getEntityCreationPos", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getEntityCreationPos)));
    a_table->Set(&a_machine, "setEntityCreationPos", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setEntityCreationPos)));
    a_table->Set(&a_machine, "getDistanceTo", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getDistanceTo)));
    a_table->Set(&a_machine, "getVectorTo", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getVectorTo)));
    a_table->Set(&a_machine, "getRadiansToFace", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getRadiansToFace)));
    a_table->Set(&a_machine, "getVectorToNormalized", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getVectorToNormalized)));
    a_table->Set(&a_machine, "getWhichSide", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getWhichSide)));
    a_table->Set(&a_machine, "setIsEnabled", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsEnabled)));
    a_table->Set(&a_machine, "isEnabled", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isEnabled)));
    a_table->Set(&a_machine, "setIsAutoRemove", gmVariable(a_machine.AllocFunctionObject(gmf_entity_noop)));
    a_table->Set(&a_machine, "setIsAutoRemoveStranded", gmVariable(a_machine.AllocFunctionObject(gmf_entity_noop)));
    a_table->Set(&a_machine, "setIsInvisible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsInvisible)));
    a_table->Set(&a_machine, "setIsStaticDraw", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsStaticDraw)));
    a_table->Set(&a_machine, "isStaticDraw", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isStaticDraw)));
    a_table->Set(&a_machine, "isInvisible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isInvisible)));
    a_table->Set(&a_machine, "setIsEditable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsEditable)));
    a_table->Set(&a_machine, "isEditable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isEditable)));
    a_table->Set(&a_machine, "constructorSetNoPhysics", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetNoPhysics)));
    a_table->Set(&a_machine, "constructorSetNoPhysicsCollide", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetNoPhysicsCollide)));
    a_table->Set(&a_machine, "constructorSetNoUpdate", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetNoUpdate)));
    a_table->Set(&a_machine, "constructorSetIsManual", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetIsManual)));
    a_table->Set(&a_machine, "constructorSetManual", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetManual)));
    a_table->Set(&a_machine, "constructorSetNoPhysicsUpdate", gmVariable(a_machine.AllocFunctionObject(gmf_entity_constructorSetNoPhysicsUpdate)));
    a_table->Set(&a_machine, "clearTelescopes", gmVariable(a_machine.AllocFunctionObject(gmf_entity_clearTelescopes)));
    a_table->Set(&a_machine, "addTelescope", gmVariable(a_machine.AllocFunctionObject(gmf_entity_addTelescope)));
    a_table->Set(&a_machine, "isBeingEdited", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isBeingEdited)));
    a_table->Set(&a_machine, "setIsBeingEdited", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsBeingEdited)));
    a_table->Set(&a_machine, "isPlacing", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isPlacing)));
    a_table->Set(&a_machine, "setIsPlacing", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsPlacing)));
    a_table->Set(&a_machine, "isAlwaysProcessed", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isAlwaysProcessed)));
    a_table->Set(&a_machine, "isSauceBase", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isSauceBase)));
    a_table->Set(&a_machine, "setIsAlwaysProcessed", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsAlwaysProcessed)));
    a_table->Set(&a_machine, "getNumOffScreensAudible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getNumOffScreensAudible)));
    a_table->Set(&a_machine, "getAudibilityPerc", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getAudibilityPerc)));
    a_table->Set(&a_machine, "setNumOffScreensAudible", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setNumOffScreensAudible)));
    a_table->Set(&a_machine, "set_getAudibilityPerc_neverOnScreen", gmVariable(a_machine.AllocFunctionObject(gmf_entity_set_getAudibilityPerc_neverOnScreen)));
    a_table->Set(&a_machine, "is_getAudibilityPerc_neverOnScreen", gmVariable(a_machine.AllocFunctionObject(gmf_entity_is_getAudibilityPerc_neverOnScreen)));
    gmVariable scriptSetGot = a_table->Get(&a_machine, "setGot");
    if (scriptSetGot.m_type == GM_FUNCTION) {
        // Native method injection runs after the entity factory returns. Keep
        // the shipped BaseNotePlant.setGot implementation (including its
        // onGot callback) available to calls through the raw native handle.
        a_table->Set(&a_machine, "__retailScriptSetGot", scriptSetGot);
    } else {
        a_table->Set(&a_machine, "setGot",
            gmVariable(a_machine.AllocFunctionObject(gmf_entity_setGot)));
    }
    if (a_table->Get(&a_machine, "isGot").m_type != GM_FUNCTION)
        a_table->Set(&a_machine, "isGot",
            gmVariable(a_machine.AllocFunctionObject(gmf_entity_isGot)));
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
    a_table->Set(&a_machine, "forceAudibilityRecalc", gmVariable(a_machine.AllocFunctionObject(gmf_entity_forceAudibilityRecalc)));
    a_table->Set(&a_machine, "recordScreenExtents", gmVariable(a_machine.AllocFunctionObject(gmf_entity_recordScreenExtents)));
    a_table->Set(&a_machine, "isOnCurrentScreen", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isOnCurrentScreen)));
    a_table->Set(&a_machine, "isOnScreen", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isOnScreen)));
    a_table->Set(&a_machine, "initializeScreenExtents", gmVariable(a_machine.AllocFunctionObject(gmf_entity_initializeScreenExtents)));
    // direct Ghidra cross-reference @ 0x001c5d6c-adjacent), same (path,
    // note, gate, pan, vol) contract as Audio.playSound -- see
    // audio_bindings.h's PlaySoundNative.
    a_table->Set(&a_machine, "playSound", gmVariable(a_machine.AllocFunctionObject(PlaySoundNative)));
    a_table->Set(&a_machine, "playSoundWithMixer", gmVariable(a_machine.AllocFunctionObject(gmf_entity_playSoundWithMixer)));
    a_table->Set(&a_machine, "playInstrumentWithMixer", gmVariable(a_machine.AllocFunctionObject(gmf_entity_playInstrumentWithMixer)));
    a_table->Set(&a_machine, "setIsDeletable", gmVariable(a_machine.AllocFunctionObject(gmf_entity_noop)));
    a_table->Set(&a_machine, "getObjectID", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getObjectID)));
    a_table->Set(&a_machine, "getUID", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getUID)));
    a_table->Set(&a_machine, "getDescriptionBits", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getDescriptionBits)));
    a_table->Set(&a_machine, "setDescriptionBits", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setDescriptionBits)));
    a_table->Set(&a_machine, "getMinCX", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMinCX)));
    a_table->Set(&a_machine, "getMaxCX", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMaxCX)));
    a_table->Set(&a_machine, "getMinCY", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMinCY)));
    a_table->Set(&a_machine, "getMaxCY", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getMaxCY)));
    a_table->Set(&a_machine, "setUIDData", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setUIDData)));
    a_table->Set(&a_machine, "loadAnimation", gmVariable(a_machine.AllocFunctionObject(gmf_entity_loadAnimation)));
    a_table->Set(&a_machine, "unloadAnimation", gmVariable(a_machine.AllocFunctionObject(gmf_entity_unloadAnimation)));
    a_table->Set(&a_machine, "unloadAnimationAll", gmVariable(a_machine.AllocFunctionObject(gmf_entity_unloadAnimationAll)));
    a_table->Set(&a_machine, "isInView", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isInView)));
    a_table->Set(&a_machine, "tagForDeletion", gmVariable(a_machine.AllocFunctionObject(gmf_entity_tagForDeletion)));
    a_table->Set(&a_machine, "isTaggedForDeletion", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isTaggedForDeletion)));
    a_table->Set(&a_machine, "isType", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isType)));
    a_table->Set(&a_machine, "isSameType", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isSameType)));
    a_table->Set(&a_machine, "callMethod", gmVariable(a_machine.AllocFunctionObject(gmf_callMethodByName)));
    a_table->Set(&a_machine, "isInGameBoard", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isInGameBoard)));
    a_table->Set(&a_machine, "setIsBoundedToGameBoard", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setIsBoundedToGameBoard)));
    a_table->Set(&a_machine, "isBoundedToGameBoard", gmVariable(a_machine.AllocFunctionObject(gmf_entity_isBoundedToGameBoard)));
    a_table->Set(&a_machine, "setupDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setupDrawList)));
    a_table->Set(&a_machine, "setupStaticDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_entity_setupStaticDrawList)));
    a_table->Set(&a_machine, "getVertexShaderParamIndex", gmVariable(a_machine.AllocFunctionObject(gmf_entity_getVertexShaderParamIndex)));
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
    // BaseNotePlant registers the raw Entity handle for off-screen drawing;
    // its script method lives on the returned behavior table.
    if (a_table->Get(&a_machine, "drawExplosionIfGot").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "drawExplosionIfGot",
                     gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_drawExplosionIfGot)));
    }
    if (a_table->Get(&a_machine, "updateExplosionIfGot").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "updateExplosionIfGot",
                     gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_updateExplosionIfGot)));
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
    // Remember whether this entity's configure() actually produced a main
    // physics body. ConfigureUnconfiguredEntities uses this to tell "has no
    // body because it never has one" (text-only//logic entities) apart from
    // "had a body and it was killed", which must be re-configured -- see
    // the re-configure check there.
    if (self->Get(&m, "mainpb").m_type != GM_NULL) {
        self->Set(&m, "__hadMainpb", gmVariable((gmint)1));
    }

    // GE3_TEXT_DIAG=1 -- report what configure() actually received. An
    // entity configured against a configureData that lacks posX/posY ends
    // up at the world origin, which for a TutorialMessage means its text
    // draws ~488 units off-camera and looks exactly like "no text".
    if (getenv("GE3_TEXT_DIAG")) {
        gmVariable cdv = self->Get(&m, "configureData");
        const char* tn = "?";
        gmVariable tnv = self->Get(&m, "__typeName");
        if (tnv.m_type == GM_STRING) tn = ((gmStringObject*)m.GetObject(tnv.m_value.m_ref))->GetString();
        if (cdv.m_type == GM_TABLE) {
            gmTableObject* cd = (gmTableObject*)m.GetObject(cdv.m_value.m_ref);
            gmVariable px = cd->Get(&m, "posX"), py = cd->Get(&m, "posY");
            gmVariable et = cd->Get(&m, "entType");
            std::fprintf(stderr, "[cfg] %-28s entType=%d posX=%s posY=%s\n", tn,
                         (int)et.m_type,
                         px.m_type == GM_NULL ? "NULL" : (px.m_type == GM_FLOAT ? "float" : "int"),
                         py.m_type == GM_NULL ? "NULL" : (py.m_type == GM_FLOAT ? "float" : "int"));
        } else {
            std::fprintf(stderr, "[cfg] %-28s configureData NOT A TABLE (type %d)\n", tn, (int)cdv.m_type);
        }
    }

    if (result.m_type != GM_NULL) a_thread->Push(result);
    return GM_OK;
}

gmVariable CreateEntityFromFactory(gmMachine& a_machine, const std::string& a_typeName,
                                   int a_uid, int a_physicsType) {
    gmTableObject* handleTable = a_machine.AllocTableObject();
    InjectEntityHandleMethods(a_machine, handleTable, a_uid, a_typeName, a_physicsType);
    // ENGINE PATCH: the raw handle is entirely C++-built -- unlike the
    // factory's RETURNED table below, which is script-authored gameplay
    // logic with native methods dual-injected on top of it and must stay
    // unmarked (see strict_calls.h's MarkNative) -- so a missing method
    // looked up directly on this object is unambiguously an engine gap.
    StrictCalls::MarkScriptExtensibleNative(a_machine, handleTable);
    gmVariable handle(handleTable);

    gmVariable globalFunc = a_machine.GetGlobals()->Get(&a_machine, a_typeName.c_str());
    if (globalFunc.m_type != GM_FUNCTION && TryAutoLoadEntityScript(a_machine, a_typeName)) {
        globalFunc = a_machine.GetGlobals()->Get(&a_machine, a_typeName.c_str());
    }
    if (globalFunc.m_type != GM_FUNCTION) {
        if (a_typeName.empty()) return handle;
        std::fprintf(stderr, "ENTITYMANAGER: Failed to create entity type \"%s\" (missing local constructor)\n",
                     a_typeName.c_str());
        return gmVariable::s_null;
    }

    gmCall call;
    if (!call.BeginGlobalFunction(&a_machine, a_typeName.c_str())) {
        std::fprintf(stderr, "ENTITYMANAGER: Failed to create entity type \"%s\" (constructor call failed)\n",
                     a_typeName.c_str());
        return gmVariable::s_null;
    }
    call.AddParam(handle);
    call.End();

    if (!call.DidReturnVariable() || call.GetReturnedVariable().m_type != GM_TABLE) {
        std::fprintf(stderr, "ENTITYMANAGER: Failed to create entity type \"%s\" (constructor returned no table)\n",
                     a_typeName.c_str());
        return gmVariable::s_null;
    }

    gmVariable entityTable = call.GetReturnedVariable();
    gmTableObject* table = (gmTableObject*)a_machine.GetObject(entityTable.m_value.m_ref);
    // Dual injection, same reasoning as InjectProcessHandleMethods: real
    // scripts call these on the RETURNED table too (see
    // gmf_entity_getPhysicsData's header comment), not just the handle.
    InjectEntityHandleMethods(a_machine, table, a_uid, a_typeName, a_physicsType);

    // Link the bare handle back to its real behavior table -- see
    // gmf_entity_delegate_die's header comment for why this exists: real
    // call sites (Main.gm's KillPlayer handler) call die()/isWizard()/
    // isInvincible() directly on `this.Entity` (the handle), expecting the
    // same object identity retail's own unified Entity/Player has.
    handleTable->Set(&a_machine, "__behaviorOwner", entityTable);
    // Retail exposes one unified Entity object, so dynamically installed
    // EntityBase methods are callable through either `this` or `this.Entity`.
    // Some packed pooled-projectile scripts call giveBackToBulletCache on
    // `this.Entity`; the desktop's behavior-table split otherwise turns that
    // valid retail call into a missing-method no-op and leaves the projectile
    // visible/active after its effect completes.
    if (handleTable->Get(&a_machine, "giveBackToBulletCache").m_type != GM_FUNCTION) {
        handleTable->Set(&a_machine, "giveBackToBulletCache",
            gmVariable(a_machine.AllocFunctionObject(
                gmf_entity_delegate_giveBackToBulletCache)));
    }
    SyncEntityNativeFields(a_machine, handleTable, table);

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
    // Main.gm stores raw Entity handles in each screen's musicTriggerEntities
    // table, while gameplay classes define onMusicTrigger on the behavior
    // table returned by their factory. Install the forwarding method only
    // for classes that actually have the callback, preserving Main.gm's
    // `?e.onMusicTrigger` registration check.
    if (table->Get(&a_machine, "onMusicTrigger").m_type == GM_FUNCTION) {
        handleTable->Set(&a_machine, "onMusicTrigger",
            gmVariable(a_machine.AllocFunctionObject(gmf_entity_delegate_onMusicTrigger)));
    }
    // Constructors install their final configure() chain (for example,
    // SauceBase and BaseNotePlant replace it while installing their mixins).
    // Wrapping before constructor() let those assignments overwrite the
    // trampoline. __nativeConfigured then stayed unset and the lazy sweep
    // recreated these entities and their Box2D bodies every frame.
    gmVariable configureFn = table->Get(&a_machine, "configure");
    if (configureFn.m_type == GM_FUNCTION) {
        table->Set(&a_machine, "__origConfigure", configureFn);
        table->Set(&a_machine, "configure", gmVariable(a_machine.AllocFunctionObject(gmf_entity_configureTrampoline)));
    }
    return entityTable;
}

gmVariable GetOrCreateDummyEntity(gmMachine& a_machine) {
    auto& registry = Entities();
    if (registry.dummyEntity.m_type == GM_TABLE &&
        a_machine.GetObject(registry.dummyEntity.m_value.m_ref)) {
        return registry.dummyEntity;
    }
    gmTableObject* table = a_machine.AllocTableObject();
    InjectEntityHandleMethods(a_machine, table, -1, "DummyEntity", 0);
    table->Set(&a_machine, "__dummyEntity", gmVariable((gmint)1));
    registry.dummyEntity = gmVariable(table);
    RootGmVariable(a_machine, registry.dummyEntity);
    StrictCalls::MarkScriptExtensibleNative(a_machine, registry.dummyEntity);
    return registry.dummyEntity;
}

// Argument forms, transcribed from the decompiled body in full
// and the 1-argument form branches on the argument's TYPE:
//
//   0 args              -> factory(this, 0, 0)
//   1 arg, int (tag 1)  -> factory(this, 0, <int>)
//   1 arg, str (tag 0x25) -> factory(this, <string>, 0)      the type name
//   2 args              -> factory(this, <string>, <arg1>)
//   anything else       -> return -1
//
// The 2-argument form is the one the level loader uses on every placed
// entity -- Platformer/Main.gm's configureEntities does
// `e = EntityManager.createEntity(cd.entType, pt);` -- so rejecting it is
// fatal: configureEntities throws, never advances its progress record, and
// the incremental loader retries forever. The loading screen then never
// clears this.setupData, isLoadingLevel() stays true, and Main.gm's
// onKeyPress refuses to accept the level (it requires !isLoadingLevel()),
// so the level can never be entered at all.
//
// The physics-type slot is forwarded through the handle into its lazily
// created PhysicsData. Retail's native factory contract is confirmed by
// FUN_000c3334 -> FUN_000c1980 -> FUN_000c1710; the local adapter exposes
// that selected type via PhysicsData.getType().
gmVariable CreateAndQueueEntity(gmMachine& m, const char* typeName,
                                int physicsType, bool isPlayer) {
    // Reserve the native object ID before invoking the script constructor.
    // Constructors may recursively call EntityManager.createEntity() (for
    // example PlayerCheckpoint creates its CarVehicle in its constructor).
    // Reserving afterward gave the nested entity the same ID as its parent,
    // so getEntity(vehicleEntityID) resolved back to the checkpoint and the
    // vehicle never received its configure/body setup.
    const int uid = Entities().nextId++;
    // GE3_TEXT_DIAG=1 -- pair with [cfg]. An entity type created more
    // times than it is configured has instances left with a null mainpb,
    // which is what puts tutorial text at the world origin.
    if (getenv("GE3_TEXT_DIAG")) {
        std::fprintf(stderr, "[mk] %s\n", typeName[0] ? typeName : "(none)");
    }
    gmVariable var = CreateEntityFromFactory(m, typeName, uid, physicsType);
    if (var.m_type != GM_TABLE) return var;
    gmTableObject* table = (gmTableObject*)m.GetObject(var.m_value.m_ref);
    if (isPlayer) {
        SetEntityNativeField(m, table, "__isPlayer", gmVariable((gmint)1));
        InjectPlayerRendererLifecycleMethods(m, table);
        gmVariable handleVar = table ? table->Get(&m, "Entity") : gmVariable::s_null;
        gmTableObject* handle = handleVar.m_type == GM_TABLE
            ? static_cast<gmTableObject*>(m.GetObject(handleVar.m_value.m_ref)) : nullptr;
        InjectPlayerRendererLifecycleMethods(m, handle);
        // Retail's PlayerGet native base exposes these methods on the player
        // behavior object. Keep script overrides, but supply the native
        // implementations when the factory table has no local method.
        if (table && table->Get(&m, "testfn").m_type != GM_FUNCTION)
            table->Set(&m, "testfn", gmVariable(m.AllocFunctionObject(gmf_player_testfn)));
        if (table && table->Get(&m, "hangTimeUpdate").m_type != GM_FUNCTION)
            table->Set(&m, "hangTimeUpdate", gmVariable(m.AllocFunctionObject(gmf_entity_noop)));
    }
    RootGmVariable(m, var);
    Entities().pendingCreation.push_back({uid, var});
    gmVariable scriptUid = table->Get(&m, "__uid");
    if (scriptUid.m_type == GM_INT && scriptUid.m_value.m_int != -1)
        Entities().entitiesByScriptUID[scriptUid.m_value.m_int] = var;
    return var;
}
int GM_CDECL gmf_EM_createEntity(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    const int n = a_thread->GetNumParams();
    if (n > 2) return GM_EXCEPTION; // retail returns -1
    const char* typeName = "";
    int physicsType = 0;
    if (n >= 1) {
        gmVariable p0 = a_thread->Param(0);
        if (p0.m_type == GM_STRING) typeName = a_thread->ParamString(0, "");
        else if (n == 1 && p0.m_type == GM_INT)
            physicsType = p0.m_value.m_int;
        else if (n == 1) return GM_EXCEPTION; // retail accepts int or string
        else if (n == 2) return GM_EXCEPTION; // 2-arg form requires a string first
    }
    if (n == 2) {
        // Retail forwards the argument union word unchanged to the native
        // factory; a numeric conversion would change GM_FLOAT semantics.
        physicsType = a_thread->Param(1).m_value.m_int;
    }
    a_thread->Push(CreateAndQueueEntity(m, typeName, physicsType, false));
    return GM_OK;
}
// createEntityAsPlayer's retail native takes zero or one type-name argument;
// it is a distinct factory entry point rather than createEntity plus a flag.
int GM_CDECL gmf_EM_createEntityAsPlayer(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    const int n = a_thread->GetNumParams();
    if (n > 1) return GM_EXCEPTION;
    const char* typeName = "";
    if (n == 1) {
        if (a_thread->Param(0).m_type != GM_STRING) return GM_EXCEPTION;
        typeName = a_thread->ParamString(0, "");
    }
    a_thread->Push(CreateAndQueueEntity(m, typeName, 0, true));
    return GM_OK;
}
int GM_CDECL gmf_EM_getEntity(gmThread* a_thread) {
    // GM_FLOAT_OR_INT_PARAM, not GM_INT_PARAM: real level entity uids come
    // from XML data (`<element key="uid" value="620"/>`), which always
    // parses as a GM_FLOAT (gamemonkeyscript_bindings.cpp's ParseLeafValue)
    // -- the strict int-only param silently threw a blank exception for
    // every real configureEntities() call (same bug class as
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(uidF, 0, 0.0f);
    int uid = (int)uidF;
    // EntityManager.getEntity checks committed object IDs and the pending
    // creation queue; a miss returns retail's shared DummyEntity rather than
    // GM_NULL (FUN_000c3acc -> DAT_00423570).
    gmVariable* e = FindEntity(uid);
    if (e) a_thread->Push(*e);
    else a_thread->Push(GetOrCreateDummyEntity(*a_thread->GetMachine()));
    return GM_OK;
}
int GM_CDECL gmf_EM_getEntityFromUID(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    // Retail FUN_00098604 forwards the argument's raw value word to
    // FUN_000c4314 without inspecting its GM type. In particular, a
    // float-tagged UID is looked up by its IEEE-754 bits, not its integer
    // conversion; setUIDData performs its own float-to-int conversion.
    const int uid = a_thread->Param(0).m_value.m_int;
    gmVariable* entity = FindEntityByScriptUID(*a_thread->GetMachine(), uid);
    if (entity) a_thread->Push(*entity); else a_thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_EM_isEntityIDValid(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable id = a_thread->Param(0);
    // Retail Entity.c:1048 converts GM_FLOAT to int, but otherwise uses the
    // raw value word. Its manager helper probes the committed object-ID map
    // directly; unlike getEntity(), it does not scan pending creations.
    const int objectId = id.m_type == GM_FLOAT
        ? (int)id.m_value.m_float : id.m_value.m_int;
    const bool found = !Entities().deleteAllInProgress &&
        std::any_of(Entities().entities.begin(), Entities().entities.end(),
        [objectId](const auto& entity) { return entity.first == objectId; });
    a_thread->PushInt(found ? 1 : 0);
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
    // Retail walks a circular list. When the cursor is the sentinel after
    // the last item, goToNext follows sentinel.next back to the first item.
    // An empty list's sentinel points to itself and therefore remains invalid.
    if (it) {
        if (it->index < it->entities.size()) ++it->index;
        else if (!it->entities.empty()) it->index = 0;
    }
    return GM_OK;
}
int GM_CDECL gmf_entityListIter_get(gmThread* a_thread) {
    EntityListIterData* it = (EntityListIterData*)a_thread->ThisUserCheckType(g_entityListIterType);
    if (it && it->index < it->entities.size()) a_thread->Push(it->entities[it->index]);
    else a_thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_entityListIter_reset(gmThread* a_thread) {
    // Retail reset assigns the backing circular-list head to current.
    EntityListIterData* it = (EntityListIterData*)a_thread->ThisUserCheckType(g_entityListIterType);
    if (it) it->index = 0;
    return GM_OK;
}
int GM_CDECL gmf_entityListIter_size(gmThread* a_thread) {
    // Retail returns the backing list's stored element count (zero if absent).
    EntityListIterData* it = (EntityListIterData*)a_thread->ThisUserCheckType(g_entityListIterType);
    a_thread->PushInt(it ? (gmint)it->entities.size() : 0);
    return GM_OK;
}
gmFunctionEntry s_entityListIterLib[] = {
    {"isValid", gmf_entityListIter_isValid, nullptr},
    {"goToNext", gmf_entityListIter_goToNext, nullptr},
    {"get", gmf_entityListIter_get, nullptr},
    {"reset", gmf_entityListIter_reset, nullptr},
    {"size", gmf_entityListIter_size, nullptr},
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

// getAllEntities([typeNameOrId[, iterator]]) -- retail Entity.c 0x000986dc
// dispatches zero args to all entities, one int to FUN_000c231c (type-ID
// filter), one string to FUN_000c3c14 (name filter), and (string, iterator)
// to refresh the supplied iterator in place. The prior string-only handling
// silently returned every entity for numeric type IDs and ignored a reused
// iterator in the two-argument form.
// The single-string form uses the same by-name lookup ENTITY_GetType/isType
// already back via InternTypeId here. Previously it was accepted but
// SILENTLY IGNORED (every real filtered call, e.g. Main.gm:9119's
// `getAllEntities("Platformer_PlayerDeathBits")` or EntityBase.gm:5657's
// `getAllEntities("...Checkpoint")`, returned literally every entity in the
// level instead of just the requested type) -- on top of the missing-
// iterator bug above, so both bugs compounded (returned the wrong SET,
// wrapped in a shape callers couldn't even walk).
int GM_CDECL gmf_EM_getAllEntities(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    const int numParams = a_thread->GetNumParams();
    if (numParams < 0 || numParams > 2) return GM_EXCEPTION;

    const gmVariable* filter = numParams > 0 ? &a_thread->Param(0) : nullptr;
    if (filter && filter->m_type != GM_INT && filter->m_type != GM_STRING)
        return GM_EXCEPTION;
    if (numParams == 2 && filter->m_type != GM_STRING)
        return GM_EXCEPTION;

    const bool filterById = filter && filter->m_type == GM_INT;
    bool filterByName = false;
    std::string typeName;
    if (filter && filter->m_type == GM_STRING) {
        char buf[256];
        const char* s = filter->AsString(&m, buf, sizeof(buf));
        if (!s) return GM_EXCEPTION;
        filterByName = true;
        typeName = s;
    }
    const int typeId = filterById ? filter->m_value.m_int : 0;
    std::vector<gmVariable> matches;
    for (auto& e : Entities().entities) {
        if (!filter) { matches.push_back(e.second); continue; }
        if (filterById) {
            if (e.second.m_type != GM_TABLE) continue;
            gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
            if (!t) continue;
            gmVariable id = t->Get(&m, "__typeId");
            if (id.m_type == GM_INT && id.m_value.m_int == typeId)
                matches.push_back(e.second);
            continue;
        }
        if (!filterByName) continue;
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        gmVariable tn = t->Get(&m, "__typeName");
        if (tn.m_type != GM_STRING) continue;
        char buf[256];
        const char* s = tn.AsString(&m, buf, sizeof(buf));
        if (s && typeName == s) matches.push_back(e.second);
    }

    if (numParams == 2) {
        gmVariable container = a_thread->Param(1);
        gmUserObject* object = container.m_type == g_entityListIterType
            ? (gmUserObject*)m.GetObject(container.m_value.m_ref) : nullptr;
        if (!object || object->m_userType != g_entityListIterType || !object->m_user)
            return GM_EXCEPTION;
        EntityListIterData* iterator = (EntityListIterData*)object->m_user;
        iterator->entities = std::move(matches);
        iterator->index = 0;
        a_thread->Push(container);
        return GM_OK;
    }
    PushEntityListIterator(a_thread, std::move(matches));
    return GM_OK;
}
// Retail maintains distinct manual/non-manual entity lists. The manual
// constructor native marks its entity before registration; our script-table
// representation carries that flag as __isManual for both enumeration and
// count queries.
int GM_CDECL gmf_EM_getAllNonManualEntities(gmThread* a_thread) {
    std::vector<gmVariable> all;
    gmMachine& machine = *a_thread->GetMachine();
    for (auto& e : Entities().entities) {
        gmTableObject* table = e.second.m_type == GM_TABLE
            ? (gmTableObject*)machine.GetObject(e.second.m_value.m_ref) : nullptr;
        gmVariable manual = table ? table->Get(&machine, "__isManual") : gmVariable::s_null;
        if (manual.m_type != GM_INT || manual.m_value.m_int == 0)
            all.push_back(e.second);
    }
    if (a_thread->GetNumParams() > 0) {
        gmVariable container = a_thread->Param(0);
        gmUserObject* object = container.m_type == g_entityListIterType
            ? (gmUserObject*)machine.GetObject(container.m_value.m_ref) : nullptr;
        if (!object || object->m_userType != g_entityListIterType || !object->m_user)
            return GM_EXCEPTION;
        EntityListIterData* iterator = (EntityListIterData*)object->m_user;
        iterator->entities = std::move(all);
        iterator->index = 0;
        a_thread->Push(container);
    } else {
        PushEntityListIterator(a_thread, std::move(all));
    }
    return GM_OK;
}
int GM_CDECL gmf_EM_getAllManualEntities(gmThread* a_thread) {
    std::vector<gmVariable> manualEntities;
    gmMachine& machine = *a_thread->GetMachine();
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* table = (gmTableObject*)machine.GetObject(e.second.m_value.m_ref);
        if (!table) continue;
        gmVariable manual = table->Get(&machine, "__isManual");
        if (manual.m_type == GM_INT && manual.m_value.m_int != 0)
            manualEntities.push_back(e.second);
    }
    PushEntityListIterator(a_thread, std::move(manualEntities));
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
int GM_CDECL gmf_EM_getAllPlayerEntitiesRandomly(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    std::vector<gmVariable> players;
    for (const auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* table = (gmTableObject*)machine.GetObject(e.second.m_value.m_ref);
        if (!table) continue;
        gmVariable player = table->Get(&machine, "__isPlayer");
        if (player.m_type == GM_INT && player.m_value.m_int != 0)
            players.push_back(e.second);
    }
    // Retail copies FUN_000c3c54's player-list traversal into an owned list
    // via FUN_0031c604, then wraps it with FUN_0031ad28(..., 1). The final
    // flag transfers list ownership; the iterator still walks the copied
    // links in manager order. No RNG or permutation occurs in this path.
    PushEntityListIterator(a_thread, std::move(players));
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
    // Retail's shared AABB query refuses inverted bounds. Negative radii
    // therefore yield an empty result, including when an entity AABB would
    // otherwise span both inverted corners.
    if (rx >= 0.0f && ry >= 0.0f) {
        for (auto& e : Entities().entities) {
            if (e.second.m_type != GM_TABLE) continue;
            gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
            if (t && EntityOverlapsBox(m, t, x - rx, y - ry, x + rx, y + ry))
                matches.push_back(e.second);
        }
    }
    // Retail's five-argument branch updates the caller-owned list container
    // in place (Entity.c:getEntitiesNear, 0x00099740). TouchEditProcessBase
    // keeps one ENTITYLISTREFCONTAINER and passes it back on every query.
    // Candidate membership/order is still only approximated by this local
    // registry scan; retail dispatches through the PhysicsManager AABB query.
    if (a_thread->GetNumParams() >= 5) {
        gmVariable container = a_thread->Param(4);
        gmUserObject* object = container.m_type == g_entityListIterType
            ? (gmUserObject*)m.GetObject(container.m_value.m_ref) : nullptr;
        if (!object || object->m_userType != g_entityListIterType || !object->m_user)
            return GM_EXCEPTION;
        EntityListIterData* iterator = (EntityListIterData*)object->m_user;
        iterator->entities = std::move(matches);
        iterator->index = 0;
        a_thread->Push(container);
        return GM_OK;
    }
    PushEntityListIterator(a_thread, std::move(matches));
    return GM_OK;
}
int GM_CDECL gmf_EM_getNumEntities(gmThread* a_thread) {
    const int numParams = a_thread->GetNumParams();
    if (numParams < 0 || numParams > 1) return GM_EXCEPTION;
    if (numParams == 0) {
        a_thread->PushInt((int)(Entities().entities.size() + Entities().pendingCreation.size()));
        return GM_OK;
    }

    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable& filter = a_thread->Param(0);
    if (filter.m_type != GM_INT && filter.m_type != GM_STRING) return GM_EXCEPTION;
    int typeId = 0;
    if (filter.m_type == GM_INT) {
        typeId = filter.m_value.m_int;
    } else {
        char buf[256];
        const char* name = filter.AsString(&machine, buf, sizeof(buf));
        if (!name) return GM_EXCEPTION;
        typeId = InternTypeId(name);
    }

    int count = 0;
    auto countType = [&machine, typeId, &count](const auto& entries) {
        for (const auto& entry : entries) {
            if (entry.second.m_type != GM_TABLE) continue;
            gmTableObject* entity = (gmTableObject*)machine.GetObject(entry.second.m_value.m_ref);
            if (!entity) continue;
            gmVariable id = entity->Get(&machine, "__typeId");
            if (id.m_type == GM_INT && id.m_value.m_int == typeId) ++count;
        }
    };
    countType(Entities().entities);
    countType(Entities().pendingCreation);
    a_thread->PushInt(count);
    return GM_OK;
}
int GM_CDECL gmf_EM_getNumManualEntities(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    int count = 0;
    auto countManual = [&machine, &count](const auto& entries) {
        for (const auto& e : entries) {
            if (e.second.m_type != GM_TABLE) continue;
            gmTableObject* table = (gmTableObject*)machine.GetObject(e.second.m_value.m_ref);
            if (!table) continue;
            gmVariable manual = table->Get(&machine, "__isManual");
            if (manual.m_type == GM_INT && manual.m_value.m_int != 0) ++count;
        }
    };
    // Retail reads the dedicated EntityManager manual-list count (+0x44).
    // Pending creates are not linked into that list until the manager flushes
    // them (FUN_000c1fd4), so only committed registry entries contribute.
    countManual(Entities().entities);
    a_thread->PushInt(count);
    return GM_OK;
}
int GM_CDECL gmf_EM_getNumNonManualEntities(gmThread* a_thread) {
    // Retail reads the dedicated non-manual-list count (+0x38), which is
    // likewise updated only when a pending entity is committed to that list.
    const int total = (int)Entities().entities.size();
    gmMachine& machine = *a_thread->GetMachine();
    int manualCount = 0;
    auto countManual = [&machine, &manualCount](const auto& entries) {
        for (const auto& e : entries) {
            if (e.second.m_type != GM_TABLE) continue;
            gmTableObject* table = (gmTableObject*)machine.GetObject(e.second.m_value.m_ref);
            if (!table) continue;
            gmVariable manual = table->Get(&machine, "__isManual");
            if (manual.m_type == GM_INT && manual.m_value.m_int != 0) ++manualCount;
        }
    };
    countManual(Entities().entities);
    a_thread->PushInt(total - manualCount);
    return GM_OK;
}
int GM_CDECL gmf_EM_getNumPlayers(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    int count = 0;
    auto countPlayers = [&machine, &count](const auto& entries) {
        for (const auto& e : entries) {
            if (e.second.m_type != GM_TABLE) continue;
            gmTableObject* table = (gmTableObject*)machine.GetObject(e.second.m_value.m_ref);
            if (!table) continue;
            gmVariable player = table->Get(&machine, "__isPlayer");
            if (player.m_type == GM_INT && player.m_value.m_int != 0) ++count;
        }
    };
    // Retail reads the dedicated player-list count (+0x50). Pending players
    // join that list only when EntityManager commits their creation.
    countPlayers(Entities().entities);
    a_thread->PushInt(count);
    return GM_OK;
}
// Retail accepts either an integer type ID or a type name (e.g.
// `EntityManager.getRandomEntity("Platformer_EntityPacks_SpawnPoint")`,
// game_data/Platformer/Main.gm:7290) and picks among entities of THAT type
// only. The string helper resolves to the same ID-indexed entity map used by
// the integer overload (FUN_000c3cc0 -> FUN_000c231c). Was previously
// ignoring the argument entirely and returning any
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
    if (a_thread->GetNumParams() != 1) return GM_EXCEPTION;
    const gmVariable& type = a_thread->Param(0);
    if (type.m_type != GM_INT && type.m_type != GM_STRING) return GM_EXCEPTION;
    const bool filterById = type.m_type == GM_INT;
    int typeId = filterById ? type.m_value.m_int : 0;
    std::string typeName;
    if (!filterById) {
        char buf[256];
        const char* name = type.AsString(&m, buf, sizeof(buf));
        if (!name) return GM_EXCEPTION;
        typeName = name;
        typeId = InternTypeId(typeName);
    }
    std::vector<gmVariable*> matches;
    for (auto& e : Entities().entities) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m.GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        if (filterById) {
            gmVariable id = t->Get(&m, "__typeId");
            if (id.m_type == GM_INT && id.m_value.m_int == typeId)
                matches.push_back(&e.second);
        } else {
            gmVariable tn = t->Get(&m, "__typeName");
            if (tn.m_type != GM_STRING) continue;
            char buf[256];
            const char* s = tn.AsString(&m, buf, sizeof(buf));
            if (s && typeName == s) matches.push_back(&e.second);
        }
    }
    if (matches.empty()) { a_thread->PushNull(); return GM_OK; }
    const int index = GetEngineContext().gameSystem.NextRandomInteger(
        0, static_cast<int>(matches.size()) - 1);
    a_thread->Push(*matches[static_cast<size_t>(index)]);
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
    const int index = GetEngineContext().gameSystem.NextRandomInteger(
        0, static_cast<int>(matches.size()) - 1);
    a_thread->Push(*matches[static_cast<size_t>(index)]);
    return GM_OK;
}
int GM_CDECL gmf_EM_getDummyEntity(gmThread* a_thread) {
    a_thread->Push(GetOrCreateDummyEntity(*a_thread->GetMachine()));
    return GM_OK;
}
int GM_CDECL gmf_EM_deleteAllEntities(gmThread* a_thread) {
    // Retail sets EntityManager's clear-all flag; the entities stay present
    // until the next flush, which invokes their destructors.
    Entities().deleteAllPending = true;
    return GM_OK;
}
int GM_CDECL gmf_EM_flush(gmThread* a_thread) {
    FlushEntityDeletions(*a_thread->GetMachine(), Entities().deleteAllPending);
    return GM_OK;
}
int GM_CDECL gmf_EM_setEntityPerfCost(gmThread* a_thread) {
    // Retail Entity.setEntityPerfCost (EBOOT 0x0009a0a0) accepts
    // (type, firstInstanceCost[, additionalInstanceCost]). Its manager
    // helper defaults the latter to the former, and the native unconditionally
    // flushes EntityManager after updating the cost table.
    const int argumentCount = a_thread->GetNumParams();
    if (argumentCount == 2 || argumentCount == 3) {
        GM_STRING_PARAM(typeName, 0, "");
        const gmint firstCost = a_thread->Param(1).m_value.m_int;
        gmint additionalCost = firstCost;
        if (argumentCount == 3)
            additionalCost = a_thread->Param(2).m_value.m_int;
        if (additionalCost == 0) additionalCost = firstCost;
        EntityPerfCosts()[typeName] = {firstCost, additionalCost};
    }
    FlushEntityDeletions(*a_thread->GetMachine(), Entities().deleteAllPending);
    return GM_OK;
}
int GM_CDECL gmf_EM_getEntityPerfCost(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(typeName, 0, "");
    gmMachine& machine = *a_thread->GetMachine();
    const auto found = EntityPerfCosts().find(typeName);
    const gmint first = found == EntityPerfCosts().end() ? 0 : found->second.firstInstance;
    const gmint additional = found == EntityPerfCosts().end()
        ? 0 : found->second.additionalInstances;
    // Retail 0x0009a174 emits cost and firstInstCost from the first native
    // word, additionalInstCost from the second, all as GM_INT values.
    gmTableObject* result = machine.AllocTableObject();
    result->Set(&machine, "cost", gmVariable(first));
    result->Set(&machine, "firstInstCost", gmVariable(first));
    result->Set(&machine, "additionalInstCost", gmVariable(additional));
    a_thread->Push(gmVariable(result));
    return GM_OK;
}
int GM_CDECL gmf_EM_setEntityPerfMaxCost(gmThread* a_thread) {
    // Retail FUN_000c3ac4 copies the GM argument's value word directly into
    // manager+0xd8 without numeric conversion.
    if (a_thread->GetNumParams() > 0)
        Entities().perfMaxCost = a_thread->Param(0).m_value.m_int;
    return GM_OK;
}
int GM_CDECL gmf_EM_getEntityPerfMaxCost(gmThread* a_thread) {
    a_thread->PushInt(Entities().perfMaxCost);
    return GM_OK;
}

gmFunctionEntry s_entityManagerLib[] = {
    {"createEntity", gmf_EM_createEntity, nullptr},
    {"createEntityAsPlayer", gmf_EM_createEntityAsPlayer, nullptr},
    {"getEntity", gmf_EM_getEntity, nullptr},
    {"getEntityFromUID", gmf_EM_getEntityFromUID, nullptr},
    {"isEntityIDValid", gmf_EM_isEntityIDValid, nullptr},
    {"getAllEntities", gmf_EM_getAllEntities, nullptr},
    {"getAllManualEntities", gmf_EM_getAllManualEntities, nullptr},
    {"getAllNonManualEntities", gmf_EM_getAllNonManualEntities, nullptr},
    {"getEntitiesNear", gmf_EM_getEntitiesNear, nullptr},
    {"getEntitiesRoughlyWithin", gmf_EM_getEntitiesRoughlyWithin, nullptr},
    {"getAllPlayerEntities", gmf_EM_getAllPlayerEntities, nullptr},
    {"getAllPlayerEntitiesRandomly", gmf_EM_getAllPlayerEntitiesRandomly, nullptr},
    {"getNumEntities", gmf_EM_getNumEntities, nullptr},
    {"getRandomEntity", gmf_EM_getRandomEntity, nullptr},
    {"getDummyEntity", gmf_EM_getDummyEntity, nullptr},
    {"deleteAllEntities", gmf_EM_deleteAllEntities, nullptr},
    {"flush", gmf_EM_flush, nullptr},
    // The portable tables now retain retail's manual/player classification;
    // on-screen simulation state still has no complete native counterpart.
    {"getNumManualEntities", gmf_EM_getNumManualEntities, nullptr},
    {"getNumNonManualEntities", gmf_EM_getNumNonManualEntities, nullptr},
    {"getNumPlayers", gmf_EM_getNumPlayers, nullptr},
    {"getRandomPlayer", gmf_EM_getRandomPlayer, nullptr},
    {"getEntityPerfCost", gmf_EM_getEntityPerfCost, nullptr},
    {"getEntityPerfMaxCost", gmf_EM_getEntityPerfMaxCost, nullptr},
    {"setEntityPerfCost", gmf_EM_setEntityPerfCost, nullptr},
    {"setEntityPerfMaxCost", gmf_EM_setEntityPerfMaxCost, nullptr},
};

// Retail Instrument loader: FUN_00199f98 lists the directory, FUN_00199548
// parses note/velocity names, sorts by note, and builds the paths returned
// by FUN_0019a618. The game_data Instrument.gm block documents the same
// algorithm but is commented out; the retail implementation is native.
struct InstrumentSample { int note; std::string name; };
struct InstrumentData {
    std::string sampleDir;
    std::string fileType = "wav";
    std::vector<InstrumentSample> samples;
    int highestVelocity = 1;
};
gmType g_instrumentType = -1;

InstrumentData* ThisInstrument(gmThread* thread) {
    return (InstrumentData*)thread->ThisUserCheckType(g_instrumentType);
}

int InstrumentNoteNumber(const std::string& name) {
    if (name.size() < 2) return 0;
    static const std::string letters = "C D EF G A B";
    const size_t pos = letters.find((char)std::toupper((unsigned char)name[0]));
    if (pos == std::string::npos || name[0] == ' ') return 0;
    int semitone = (int)pos;
    size_t i = 1;
    if (i < name.size() && (name[i] == '#' || name[i] == 'S' || name[i] == 's')) { ++semitone; ++i; }
    if (i >= name.size() || name[i] < '0' || name[i] > '9') return 0;
    int octave = 0;
    for (; i < name.size(); ++i) {
        if (name[i] < '0' || name[i] > '9') return 0;
        octave = octave * 10 + name[i] - '0';
    }
    return octave * 12 + semitone;
}

void LoadInstrumentSamples(InstrumentData& instrument, const std::string& directoryName) {
    instrument.samples.clear();
    instrument.highestVelocity = 1;
    std::string relative = directoryName;
    if (!relative.empty() && relative.front() == '@') relative = "Platformer/sounds/" + relative.substr(1);
    // Instrument samples are assets, not host files. Retail enumerates the
    // virtual data directory, so this must see PSARC entries as well as loose
    // files and patch overlays. Scanning PrimaryRoot() made every instrument
    // empty in the supported archive-only launch layout.
    FileHelper* fileHelper = GetEngineContext().fileHelper;
    if (!fileHelper) return;
    for (const std::string& filename : fileHelper->ListFiles(relative)) {
        if (filename.empty() || filename.front() == '.') continue;
        const size_t v = filename.find('V');
        if (v == std::string::npos) continue;
        const std::string name = filename.substr(0, v);
        // Retail keeps one entry per listed file, including duplicate notes
        // from different velocities. Its fixed native array has 32 slots.
        if (instrument.samples.size() < 32) instrument.samples.push_back({InstrumentNoteNumber(name), name});
        if (v + 1 < filename.size() && filename[v + 1] >= '0' && filename[v + 1] <= '9')
            instrument.highestVelocity = std::max(instrument.highestVelocity, filename[v + 1] - '0');
    }
    std::stable_sort(instrument.samples.begin(), instrument.samples.end(),
                     [](const InstrumentSample& a, const InstrumentSample& b) { return a.note < b.note; });
}

int GM_CDECL gmf_Instrument_setSampleDir(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    InstrumentData* instrument = ThisInstrument(a_thread);
    if (!instrument) return GM_EXCEPTION;
    instrument->sampleDir.assign(path, std::min<size_t>(std::strlen(path), 255));
    LoadInstrumentSamples(*instrument, instrument->sampleDir);
    return GM_OK;
}
int GM_CDECL gmf_Instrument_loadSamples(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    InstrumentData* instrument = ThisInstrument(a_thread);
    if (!instrument) return GM_EXCEPTION;
    // Native loadSamples supplies a listing path without changing the
    // sampleDir stored by setSampleDir/constructor.
    LoadInstrumentSamples(*instrument, path);
    return GM_OK;
}
int GM_CDECL gmf_Instrument_setFileType(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(type, 0, "");
    InstrumentData* instrument = ThisInstrument(a_thread);
    if (!instrument) return GM_EXCEPTION;
    instrument->fileType.assign(type, std::min<size_t>(std::strlen(type), 127));
    return GM_OK;
}
int GM_CDECL gmf_Instrument_getSampleInfo(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(note, 0, 0);
    int velocity = 1;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(v, 1, 1); velocity = v; }
    InstrumentData* instrument = ThisInstrument(a_thread);
    if (!instrument) return GM_EXCEPTION;
    std::string sound = "Platformer/sounds/error.mp3";
    int playNote = 48;
    if (!instrument->samples.empty()) {
        auto it = std::upper_bound(instrument->samples.begin(), instrument->samples.end(), note,
            [](int n, const InstrumentSample& sample) { return n < sample.note; });
        if (it == instrument->samples.begin()) it = instrument->samples.begin();
        else --it;
        if (velocity < 1 || velocity > instrument->highestVelocity) velocity = 1;
        sound = instrument->sampleDir + "/" + it->name + "V" + std::to_string(velocity) + "." + instrument->fileType;
        playNote = note - it->note + 48;
    }
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* result = machine.AllocTableObject();
    result->Set(&machine, "sound", gmVariable(machine.AllocStringObject(sound.c_str())));
    result->Set(&machine, "note", gmVariable((gmint)playNote));
    a_thread->PushTable(result);
    return GM_OK;
}

gmFunctionEntry s_instrumentLib[] = {
    {"setSampleDir", gmf_Instrument_setSampleDir, nullptr},
    {"loadSamples", gmf_Instrument_loadSamples, nullptr},
    {"setFileType", gmf_Instrument_setFileType, nullptr},
    {"getSampleInfo", gmf_Instrument_getSampleInfo, nullptr},
};

// EntityBase.playSoundWithMixer and playInstrumentWithMixer both end at the
// retail voice factory FUN_001c0ee8. Their arity ladders are recovered in
// sample/instrument, and note; the 4th is the gate/velocity flag, while the
// optional tail supplies volume then pan. In SwayingTree.gm the eight-arg
// call passes (1, -1) in those slots: full volume, hard-left pan. The retail
// arity ladder forwards those two doubles to FUN_001c0ee8 in that order.
// These wrappers share the script-visible voice registry and frontend audio output
// with Audio.playSound.
void ReadMixerPlaybackTail(gmThread* a_thread, int a_minArgs,
                           float& a_pan, float& a_volume) {
    a_pan = 0.0f;
    a_volume = 1.0f;
    const int count = a_thread->GetNumParams();
    if (count >= a_minArgs + 1) {
        a_volume = a_thread->ParamFloatOrInt(a_minArgs, 1.0f);
    }
    if (count >= a_minArgs + 2) {
        a_pan = a_thread->ParamFloatOrInt(a_minArgs + 1, 0.0f);
    }
}

int GM_CDECL gmf_entity_playSoundWithMixer(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 3 || count > 8) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 1, "");
    (void)a_thread->ParamInt(0, 0); // mixer channel; routing is not yet audible.
    const int note = (int)a_thread->ParamFloatOrInt(2, 96.0f);
    float pan, volume;
    ReadMixerPlaybackTail(a_thread, 4, pan, volume);
    a_thread->PushInt(CreateVoiceNative(path, pan, volume, note));
    return GM_OK;
}

int GM_CDECL gmf_entity_playInstrumentWithMixer(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 4 || count > 9) return GM_EXCEPTION;
    (void)a_thread->ParamInt(0, 0); // mixer channel
    InstrumentData* instrument = (InstrumentData*)a_thread->ParamUserCheckType(1, g_instrumentType);
    if (!instrument) return GM_EXCEPTION;
    const int note = a_thread->ParamInt(2, 0);
    int velocity = a_thread->ParamInt(3, 1);
    if (velocity < 1 || velocity > instrument->highestVelocity) velocity = 1;
    std::string path = "Platformer/sounds/error.mp3";
    int playNote = 48;
    if (!instrument->samples.empty()) {
        auto it = std::upper_bound(instrument->samples.begin(), instrument->samples.end(), note,
            [](int value, const InstrumentSample& sample) { return value < sample.note; });
        if (it == instrument->samples.begin()) it = instrument->samples.begin();
        else --it;
        path = instrument->sampleDir + "/" + it->name + "V" + std::to_string(velocity) + "." + instrument->fileType;
        playNote = note - it->note + 48;
    }
    float pan, volume;
    ReadMixerPlaybackTail(a_thread, 5, pan, volume);
    a_thread->PushInt(CreateVoiceNative(path, pan, volume, playNote));
    return GM_OK;
}

int GM_CDECL gmf_entity_loadAnimation(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    std::string resolved;
    auto r = GetEngineContext().fileHelper->Resolve(path);
    if (r) resolved = *r;
    else std::fprintf(stderr, "Entity.loadAnimation: '%s' not found\n", path);
    gmTableObject* entity = a_thread->ThisTable();
    gmMachine& machine = *a_thread->GetMachine();
    gmVariable enabled = GetEntityNativeField(machine, entity, "__enabled");
    const bool attachToManager = enabled.m_type == GM_INT && enabled.m_value.m_int != 0;
    PushNewAnimation(a_thread, resolved, attachToManager);
    // Retail Entity.loadAnimation records each returned resource in the
    // entity-owned list released by unloadAnimationAll (Entity offset
    // +0x3c..+0x40; FUN_000c13a8). Keep the GM references in a field on the
    // handle so their lifetime follows the entity and can be released here.
    gmVariable* result = a_thread->GetTop() - 1;
    if (entity && result && result->m_type >= GM_USER) {
        gmVariable listVar = GetEntityNativeField(machine, entity, "__nativeOwnedAnimations");
        gmTableObject* list = listVar.m_type == GM_TABLE
            ? (gmTableObject*)machine.GetObject(listVar.m_value.m_ref) : nullptr;
        if (!list) {
            list = machine.AllocTableObject();
            SetEntityNativeField(machine, entity, "__nativeOwnedAnimations", gmVariable(list));
        }
        int nextIndex = list->Count();
        while (list->Get(nextIndex).m_type != GM_NULL) ++nextIndex;
        list->Set(&machine, nextIndex, *result);
    }
    return GM_OK;
}

void ForgetEntityAnimation(gmMachine& machine, gmTableObject* entity,
                           const gmVariable& animation) {
    if (!entity || animation.m_type < GM_USER) return;
    gmVariable listVar = entity->Get(&machine, "__nativeOwnedAnimations");
    if (listVar.m_type != GM_TABLE) return;
    gmTableObject* list = (gmTableObject*)machine.GetObject(listVar.m_value.m_ref);
    if (!list) return;
    std::vector<gmVariable> keysToRemove;
    gmTableIterator it;
    for (gmTableNode* node = list->GetFirst(it); !list->IsNull(it);
         node = list->GetNext(it)) {
        if (node && node->m_value.m_type == animation.m_type &&
            node->m_value.m_value.m_ref == animation.m_value.m_ref) {
            keysToRemove.push_back(node->m_key);
        }
    }
    for (const gmVariable& key : keysToRemove)
        list->Set(&machine, key, gmVariable::s_null);
}

int GM_CDECL gmf_entity_unloadAnimation(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable animation = a_thread->Param(0);
    UnloadAnimation(machine, animation);
    ForgetEntityAnimation(machine, a_thread->ThisTable(), animation);
    return GM_OK;
}

int GM_CDECL gmf_entity_unloadAnimationAll(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(0);
    gmTableObject* entity = a_thread->ThisTable();
    if (!entity) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    gmVariable listVar = entity->Get(&machine, "__nativeOwnedAnimations");
    if (listVar.m_type != GM_TABLE) return GM_OK;
    gmTableObject* list = (gmTableObject*)machine.GetObject(listVar.m_value.m_ref);
    if (!list) return GM_OK;

    // Copy references first because releasing animation resources can invoke
    // callbacks that collect or otherwise mutate GM tables.
    std::vector<gmVariable> animations;
    gmTableIterator it;
    for (gmTableNode* node = list->GetFirst(it); !list->IsNull(it);
         node = list->GetNext(it)) {
        if (node && node->m_value.m_type >= GM_USER)
            animations.push_back(node->m_value);
    }
    for (const gmVariable& animation : animations)
        DetachAnimationFromManager(machine, animation);
    entity->Set(&machine, "__nativeOwnedAnimations", gmVariable::s_null);
    return GM_OK;
}
int GM_CDECL gmf_INSTRUMENT(gmThread* a_thread) {
    if (a_thread->GetNumParams() > 1) return GM_EXCEPTION;
    std::string path;
    if (a_thread->GetNumParams() == 1) {
        GM_STRING_PARAM(value, 0, "");
        path.assign(value, std::min<size_t>(std::strlen(value), 255));
    }
    auto* instrument = new InstrumentData();
    instrument->sampleDir = path;
    if (!path.empty()) LoadInstrumentSamples(*instrument, path);
    a_thread->PushNewUser(instrument, g_instrumentType);
    return GM_OK;
}
void GM_CDECL InstrumentGCDestruct(gmMachine*, gmUserObject* a_object) { delete (InstrumentData*)a_object->m_user; }

// RETAIL PPC VERIFIED: GAME_CalculateFillVerts (0x1ae400 -> 0x196a20)
// accepts two or three arguments. Two args use the wrapper's 0.61 default
// inset; three args supply the inset. The helper reads ax/ay..dx/dy from the
// entity and posX/posY from configureData, constructs four trimmed corner
// records, and appends eight samples. See
// the recovered formulas; PPC float identity and runtime output comparisons
// are still unverified.
int GM_CDECL gmf_GAME_CalculateFillVerts(gmThread* a_thread) {
    const int numParams = a_thread->GetNumParams();
    if (numParams != 2 && numParams != 3) return GM_EXCEPTION;
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

    // The two-argument wrapper path loads DAT_001ae3f8 (0x3f1c28f6).
    float inset = 0.61f;
    if (numParams == 3) {
        const gmVariable insetVar = a_thread->Param(2);
        if (insetVar.m_type == GM_FLOAT) inset = insetVar.m_value.m_float;
        else if (insetVar.m_type == GM_INT)
            inset = static_cast<float>(insetVar.m_value.m_int);
        else inset = 0.0f;
    }

    const char* xKeys[4] = {"ax", "bx", "cx", "dx"};
    const char* yKeys[4] = {"ay", "by", "cy", "dy"};
    b2Vec2 corners[4];
    for (int i = 0; i < 4; ++i)
        corners[i].Set(getf(entity, xKeys[i]), getf(entity, yKeys[i]));

    b2Vec2 insetCorners[4];
    for (int i = 0; i < 4; ++i) {
        const b2Vec2 current = corners[i];
        const b2Vec2 toNext = corners[(i + 1) % 4] - current;
        const b2Vec2 toPrevious = corners[(i + 3) % 4] - current;
        const float nextLength = GroundRenderMath::RetailGroundLength(
            toNext.x, toNext.y);
        const float previousLength = GroundRenderMath::RetailGroundLength(
            toPrevious.x, toPrevious.y);
        const float nextX = nextLength > 0.0f ? toNext.x / nextLength : 0.0f;
        const float nextY = nextLength > 0.0f ? toNext.y / nextLength : 0.0f;
        const float previousX = previousLength > 0.0f
            ? toPrevious.x / previousLength : 0.0f;
        const float previousY = previousLength > 0.0f
            ? toPrevious.y / previousLength : 0.0f;
        insetCorners[i].Set(
            std::fma(nextX + previousX, inset, current.x),
            std::fma(nextY + previousY, inset, current.y));
    }

    constexpr float kMaxTrim = 0.2318f;
    constexpr float kAmplitudeScale = 0.388f;
    constexpr float kPi = 3.1415927410125732f;
    constexpr float kStartPhase = 0.0f;
    for (int i = 0; i < 4; ++i) {
        const b2Vec2 corner = insetCorners[i];
        const b2Vec2 toPrevious = insetCorners[(i + 3) % 4] - corner;
        const b2Vec2 toNext = insetCorners[(i + 1) % 4] - corner;
        const float previousLength = GroundRenderMath::RetailGroundLength(
            toPrevious.x, toPrevious.y);
        const float nextLength = GroundRenderMath::RetailGroundLength(
            toNext.x, toNext.y);
        const float previousTrim = std::min(previousLength * 0.4f, kMaxTrim);
        const float nextTrim = std::min(nextLength * 0.4f, kMaxTrim);
        // PPC VERIFIED: rectangle endpoint coordinates use fmadds for
        // direction*trim + corner; keep one rounding via std::fma.
        const b2Vec2 p0 = previousLength > 0.0f
            ? b2Vec2(std::fma(toPrevious.x / previousLength, previousTrim,
                              corner.x),
                     std::fma(toPrevious.y / previousLength, previousTrim,
                              corner.y))
            : corner;
        const b2Vec2 p1 = nextLength > 0.0f
            ? b2Vec2(std::fma(toNext.x / nextLength, nextTrim, corner.x),
                     std::fma(toNext.y / nextLength, nextTrim, corner.y))
            : corner;
        const b2Vec2 chord = p1 - p0;
        const float chordLength = GroundRenderMath::RetailGroundLength(
            chord.x, chord.y);
        const b2Vec2 midpoint = 0.5f * (p0 + p1);
        const b2Vec2 controlOffset = corner - midpoint;
        const float amplitude = GroundRenderMath::RetailGroundLength(
            controlOffset.x, controlOffset.y) * kAmplitudeScale;
        const float normalX = chordLength > 0.0f ? chord.y / chordLength : 0.0f;
        const float normalY = chordLength > 0.0f ? -chord.x / chordLength : 0.0f;
        for (int sample = 0; sample < 2; ++sample) {
            const float phase = kStartPhase + static_cast<float>(sample) * kPi;
            const float curve = GroundRenderMath::RetailGroundSinCos(phase, false);
            const b2Vec2 base = sample == 0 ? p0 : p1;
            const float x = std::fma(curve * amplitude, normalX, base.x) + posX;
            const float y = std::fma(curve * amplitude, normalY, base.y) + posY;
            vertices->Set(&m, i * 2 + sample, MakeVector2D(m, x, y));
        }
    }
    return GM_OK;
}

// RETAIL PPC VERIFIED: GAME_CalculateTriangleFillVerts at 0x1ae4fc requires
// exactly three arguments and calls FUN_001975d0 with the inset, output table,
// and entity. FUN_001975d0 reads the three corners and position from
// configureData, offsets each corner by inset times the sum of its two
// normalized adjacent-edge directions, then writes the three world-space
// points in input order. The initial PPC loop and final append loop establish
// this output formula/order; its later stack-only calculations do not feed the
int GM_CDECL gmf_GAME_CalculateTriangleFillVerts(gmThread* a_thread) {
    if (a_thread->GetNumParams() != 3) return GM_EXCEPTION;
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

    gmVariable cdVar = entity->Get(&m, "configureData");
    if (cdVar.m_type != GM_TABLE) return GM_OK;
    gmTableObject* config = (gmTableObject*)m.GetObject(cdVar.m_value.m_ref);
    if (!config) return GM_OK;

    float inset = 0.0f;
    const gmVariable insetVar = a_thread->Param(2);
    if (insetVar.m_type == GM_FLOAT) inset = insetVar.m_value.m_float;
    else if (insetVar.m_type == GM_INT) inset = static_cast<float>(insetVar.m_value.m_int);

    const char* xKeys[3] = {"ax", "bx", "cx"};
    const char* yKeys[3] = {"ay", "by", "cy"};
    b2Vec2 corners[3];
    for (int i = 0; i < 3; ++i) {
        corners[i].x = getf(config, xKeys[i]);
        corners[i].y = getf(config, yKeys[i]);
    }

    // PPC forms each corner's two edge directions, sums them, then uses
    // fmadds(sum, inset, corner). Zero-length edges contribute zero.
    float posX = getf(config, "posX");
    float posY = getf(config, "posY");
    for (int i = 0; i < 3; ++i) {
        const b2Vec2 current = corners[i];
        const b2Vec2 next = corners[(i + 1) % 3] - current;
        const b2Vec2 previous = corners[(i + 2) % 3] - current;
        const float nextLength = GroundRenderMath::RetailGroundLength(
            next.x, next.y);
        const float previousLength = GroundRenderMath::RetailGroundLength(
            previous.x, previous.y);
        const float nextX = nextLength > 0.0f ? next.x / nextLength : 0.0f;
        const float nextY = nextLength > 0.0f ? next.y / nextLength : 0.0f;
        const float previousX = previousLength > 0.0f ? previous.x / previousLength : 0.0f;
        const float previousY = previousLength > 0.0f ? previous.y / previousLength : 0.0f;
        const float x = std::fma(nextX + previousX, inset, current.x) + posX;
        const float y = std::fma(nextY + previousY, inset, current.y) + posY;
        vertices->Set(&m, i, MakeVector2D(m, x, y));
    }
    return GM_OK;
}

// CurvedGround's PSARC source marks its former GM vertex builder as ported to
// this native (Game.GAME_CalculateCurveFillVerts, wrapper 0x001ae5c4). The
// archived block defines 31 quarter-arc samples, hard-coded linear endpoint
// exponents, and an outer contour offset by the normalized left normal times
// thickness. The third argument is reserved (the retail caller passes 0).
// Math uses the PPC-backed helpers also used by the MATH_* bindings. A direct
// retail vertex-buffer comparison is still required before claiming 1:1.
int GM_CDECL gmf_GAME_CalculateCurveFillVerts(gmThread* a_thread) {
    if (a_thread->GetNumParams() != 3) return GM_EXCEPTION;
    const gmVariable pointsVar = a_thread->Param(0);
    const gmVariable entityVar = a_thread->Param(1);
    if (pointsVar.m_type != GM_TABLE || entityVar.m_type != GM_TABLE)
        return GM_OK;

    gmMachine& machine = *a_thread->GetMachine();
    auto* points = (gmTableObject*)machine.GetObject(pointsVar.m_value.m_ref);
    auto* entity = (gmTableObject*)machine.GetObject(entityVar.m_value.m_ref);
    if (!points || !entity) return GM_OK;
    const gmVariable innerVar = points->Get(0);
    const gmVariable outerVar = points->Get(1);
    if (innerVar.m_type != GM_TABLE || outerVar.m_type != GM_TABLE)
        return GM_OK;
    auto* inner = (gmTableObject*)machine.GetObject(innerVar.m_value.m_ref);
    auto* outer = (gmTableObject*)machine.GetObject(outerVar.m_value.m_ref);
    if (!inner || !outer) return GM_OK;

    const gmVariable configVar = entity->Get(&machine, "configureData");
    if (configVar.m_type != GM_TABLE) return GM_OK;
    auto* config = (gmTableObject*)machine.GetObject(configVar.m_value.m_ref);
    if (!config) return GM_OK;
    auto getFloat = [&](gmTableObject* table, const char* key) -> float {
        const gmVariable value = table->Get(&machine, key);
        if (value.m_type == GM_FLOAT) return value.m_value.m_float;
        if (value.m_type == GM_INT) return static_cast<float>(value.m_value.m_int);
        return 0.0f;
    };

    constexpr int kSamples = 31;
    // Retail FUN_00197e20 loads this exact binary32 increment from
    // DAT_00197e00 (EBOOT VMA 0x00197e00), then advances the angle with an
    // `fadds` on each iteration. Multiplying the step by the sample index
    // produces different binary32 values from sample 21 onward.
    const float kAngleStep = GroundRenderMath::FloatFromBits(0xbd567750u);
    constexpr float kPiOverTwo = 1.5707963705062866f;
    const float curve = getFloat(config, "curve");
    // Retail loads the rounded binary32 thickness constant
    // DAT_00197e0c (1.2200000286) directly with `lfs`; it does not read the
    // mutable GameMonkey SW global here. This equals the shipped script's
    // SW*2 at SW=0.61, while keeping the native helper's fixed constant.
    const float thickness = GroundRenderMath::FloatFromBits(0x3f9c28f6u);
    const float radius = getFloat(config, "editStretchValue") * 2.0f;
    const float outerRadius = radius + thickness;
    const float offsetX = -radius / 2.0f;
    const float offsetY = radius / 2.0f;
    b2Vec2 innerPoints[kSamples];

    auto signedPow = [](float value, float exponent) {
        const float magnitude = GroundRenderMath::RetailPlayerPow(
            std::fabs(value), exponent);
        return std::signbit(value) ? -magnitude : magnitude;
    };
    auto pointAt = [&](float angle, float exponent, float r) {
        return b2Vec2(
            std::fma(signedPow(GroundRenderMath::RetailGroundSinCos(angle, true), exponent),
                     r, offsetX),
            std::fma(signedPow(GroundRenderMath::RetailGroundSinCos(angle, false), exponent),
                     r, offsetY));
    };

    const b2Vec2 innerStart = pointAt(0.0f, 1.0f, radius);
    const b2Vec2 outerStart = pointAt(0.0f, 1.0f, outerRadius);
    innerPoints[0] = innerStart;
    inner->Set(&machine, 0, MakeVector2D(machine, innerStart.x, innerStart.y));
    outer->Set(&machine, 0, MakeVector2D(machine, outerStart.x, outerStart.y));
    float angle = kAngleStep;
    for (int i = 1; i < kSamples - 1; ++i) {
        innerPoints[i] = pointAt(angle, curve, radius);
        inner->Set(&machine, i, MakeVector2D(machine,
            innerPoints[i].x, innerPoints[i].y));
        // Retail's PPC loop executes a single-precision fadds after each
        // point. Preserve that accumulation order instead of deriving each
        // angle independently from its integer index.
        angle = static_cast<float>(angle + kAngleStep);
    }
    for (int i = 1; i < kSamples - 1; ++i) {
        const float dx = innerPoints[i].x - innerPoints[i - 1].x;
        const float dy = innerPoints[i].y - innerPoints[i - 1].y;
        // FUN_00197e20 forms dy*dy first, then fmadds(dx, dx, dySquared),
        // fsqrts, and divides dy and dx separately. This order is specific to
        // the CurvedGround outer pass; the shared ground contour helper uses a
        // different fused operand order.
        const float dySquared = dy * dy;
        const float length = std::sqrt(std::fma(dx, dx, dySquared));
        const float nx = length > 0.0f ? -dy / length : 0.0f;
        const float ny = length > 0.0f ? dx / length : 0.0f;
        // Retail computes the X offset with separate multiply/add instructions
        // and the Y offset with fmadds. Volatile preserves the X intermediate
        // from host FMA contraction.
        volatile float scaledNormalX = nx * thickness;
        const float outerX = innerPoints[i].x + scaledNormalX;
        const float outerY = std::fma(ny, thickness, innerPoints[i].y);
        outer->Set(&machine, i, MakeVector2D(machine,
            outerX, outerY));
    }
    const b2Vec2 innerEnd = pointAt(-kPiOverTwo, 1.0f, radius);
    const b2Vec2 outerEnd = pointAt(-kPiOverTwo, 1.0f, outerRadius);
    innerPoints[kSamples - 1] = innerEnd;
    inner->Set(&machine, kSamples - 1, MakeVector2D(machine, innerEnd.x, innerEnd.y));
    outer->Set(&machine, kSamples - 1, MakeVector2D(machine, outerEnd.x, outerEnd.y));
    return GM_OK;
}

// classes/Game.c 0x001ae784/0x001ae7f8). Retail loads retained singleton
// pointers from GameSystem+0x8310/+0x8314 and returns those same objects on
// every call. The desktop bindings below preserve that identity and lifetime.
// RETAIL PPC VERIFIED: ENTITYLISTREFCONTAINER (0x695f8) constructs an empty
// 0x14-byte wrapper around a linked-list sentinel via 0x31ad28. The desktop
// returns the same script-visible empty iterator state with index zero; native
// storage layout and GC lifetime differ internally.
// ENTITYLISTREFCONTAINER() returns the reusable entity-list iterator passed
// to EntityManager.getAllNonManualEntities(container).
int GM_CDECL gmf_ENTITYLISTREFCONTAINER(gmThread* a_thread) {
    PushEntityListIterator(a_thread, {});
    return GM_OK;
}

// These three Game globals follow the fixed-slot wrapper conversions in
// the raw 32-bit payload. The native helpers then narrow their stored fields
// to f32. Selection fragment consumption remains a renderer parity gap.
static double GetRetailGameNumericArgument(const gmVariable& a_arg) {
    return a_arg.m_type == GM_FLOAT
        ? static_cast<double>(a_arg.m_value.m_float)
        : static_cast<double>(static_cast<int32_t>(a_arg.m_value.m_int));
}

int GM_CDECL gmf_GAME_SetSelectionPulseRate(gmThread* a_thread) {
    if (!a_thread || a_thread->GetNumParams() < 1) return GM_OK;
    GetEngineContext().video.SetSelectionPulseRate(
        GetRetailGameNumericArgument(a_thread->Param(0)));
    return GM_OK;
}
int GM_CDECL gmf_GAME_SetSelectionAlphaPerc(gmThread* a_thread) {
    if (!a_thread || a_thread->GetNumParams() < 1) return GM_OK;
    GetEngineContext().video.SetSelectionAlphaPerc(
        GetRetailGameNumericArgument(a_thread->Param(0)));
    return GM_OK;
}
int GM_CDECL gmf_GAME_SetIsSelectionPulseEnabled(gmThread* a_thread) {
    if (!a_thread || a_thread->GetNumParams() < 1) return GM_OK;
    // The PPC wrapper tests the raw payload, so integer 0 and floating-point
    // +0.0 disable, while any nonzero bit pattern enables.
    GetEngineContext().video.SetSelectionPulseEnabled(
        a_thread->Param(0).m_value.m_int != 0);
    return GM_OK;
}

// GAME_SetIsUsingSelectionFragmentProgram(bool, entity) -- retail resolves
// entity.setupDrawList() to native Entity+0x14, then writes this flag to
// DrawList+0x10. Preserve the raw GM payload != 0 test used by the PPC
// wrapper. The draw-config shader-variant consumer is documented in the API
// trace TODO and is still pending.
int GM_CDECL gmf_GAME_SetIsUsingSelectionFragmentProgram(gmThread* a_thread) {
    if (!a_thread || a_thread->GetNumParams() < 2) return GM_OK;
    const gmVariable& enabledArg = a_thread->Param(0);
    const gmVariable& entityArg = a_thread->Param(1);
    if (entityArg.m_type != GM_TABLE) return GM_OK;

    gmMachine& machine = *a_thread->GetMachine();
    auto* entity = static_cast<gmTableObject*>(machine.GetObject(entityArg.m_value.m_ref));
    if (!entity) return GM_OK;
    const gmVariable drawListArg = entity->Get(&machine, "drawList");
    if (drawListArg.m_type < GM_USER) return GM_OK;
    auto* drawListObject = static_cast<gmUserObject*>(machine.GetObject(drawListArg.m_value.m_ref));
    if (!drawListObject || drawListObject->m_userType != GetDrawListType()) return GM_OK;
    auto* drawList = static_cast<DrawListData*>(drawListObject->m_user);
    if (drawList)
        drawList->useSelectionFragmentProgram = enabledArg.m_value.m_int != 0;
    return GM_OK;
}

int GM_CDECL gmf_GAME_GetPreDrawList(gmThread* a_thread) {
    PushPersistentGameDrawList(a_thread, s_gamePreDrawList);
    return GM_OK;
}
int GM_CDECL gmf_GAME_GetPostDrawList(gmThread* a_thread) {
    PushPersistentGameDrawList(a_thread, s_gamePostDrawList);
    return GM_OK;
}

} // namespace

gmVariable* FindEntityByObjectID(int a_objectId) {
    return FindEntity(a_objectId);
}

bool PushOrRefillEntityListIterator(gmThread* a_thread,
    std::vector<gmVariable> a_entities, const gmVariable* a_reuse) {
    if (!a_reuse) {
        PushEntityListIterator(a_thread, std::move(a_entities));
        return true;
    }
    gmMachine& machine = *a_thread->GetMachine();
    gmUserObject* object = a_reuse->m_type == g_entityListIterType
        ? (gmUserObject*)machine.GetObject(a_reuse->m_value.m_ref) : nullptr;
    if (!object || object->m_userType != g_entityListIterType || !object->m_user)
        return false;
    EntityListIterData* iterator = (EntityListIterData*)object->m_user;
    iterator->entities = std::move(a_entities);
    iterator->index = 0;
    a_thread->Push(*a_reuse);
    return true;
}

// Retail's entity-manager update callback (FUN_000c3fb0, reached through
// the manager's registered callback table) drains the manager at the end of
// its update event by calling FUN_000c3e70. That operation promotes queued
// createEntity results into the committed iteration list before drawing and
// runs deferred deletion callbacks. The GameMonkey port has no native
// EntityManager callback object, so mirror that lifecycle boundary once per
// engine frame after all process/entity update callbacks have completed.
void FlushEntityManagerFrameBoundary() {
    gmMachine* machine = GetEngineContext().machine;
    if (!machine) return;
    FlushEntityDeletions(*machine, Entities().deleteAllPending);
}

void RequestEntityManagerDeleteAll() {
    Entities().deleteAllPending = true;
}

int GetEntityManagerEntityCount() {
    const auto& registry = Entities();
    return static_cast<int>(registry.entities.size() + registry.pendingCreation.size());
}

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

    bool skipPairedEntry = false;
    for (int i = 0; ; ++i) {
        gmVariable entryVar = animData->Get(i);
        if (entryVar.m_type != GM_TABLE) break;
        if (skipPairedEntry) {
            skipPairedEntry = false;
            continue;
        }
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
        gmVariable paletteBankVar = entry->Get(&m, "paletteBank");
        const int paletteBank = paletteBankVar.m_type == GM_INT
            ? paletteBankVar.m_value.m_int : 0;
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
        SetAnimationPaletteBank(m, animVar, paletteBank);
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
        // SauceBase marks the second member of each pair as an overlay and
        // attachOverlayAnim stores that Animation on the primary object.
        // Retail's DrawList opcode 9 submits the primary shape once with both
        // frame/palette resources; it is not two independent sprite draws.
        gmVariable nextEntryVar = animData->Get(i + 1);
        gmTableObject* nextEntry = nextEntryVar.m_type == GM_TABLE
            ? (gmTableObject*)m.GetObject(nextEntryVar.m_value.m_ref) : nullptr;
        gmVariable pairedFlag = nextEntry ? nextEntry->Get(&m, "attachedAsOverlay") : gmVariable::s_null;
        const bool hasAttachedEntry = pairedFlag.m_type == GM_INT && pairedFlag.m_value.m_int != 0;
        bool pairDrawn = false;
        if (hasAttachedEntry) {
            gmVariable frontAnimVar = nextEntry->Get(&m, "anim");
            gmVariable frontC1Var = nextEntry->Get(&m, "colour1");
            gmVariable frontC2Var = nextEntry->Get(&m, "colour2");
            gmVariable frontPaletteBankVar = nextEntry->Get(&m, "paletteBank");
            const int frontPaletteBank = frontPaletteBankVar.m_type == GM_INT
                ? frontPaletteBankVar.m_value.m_int : 0;
            std::string frontC1, frontC2;
            if (frontC1Var.m_type == GM_STRING)
                frontC1 = ((gmStringObject*)m.GetObject(frontC1Var.m_value.m_ref))->GetString();
            if (frontC2Var.m_type == GM_STRING)
                frontC2 = ((gmStringObject*)m.GetObject(frontC2Var.m_value.m_ref))->GetString();
            float frontFillR = 1, frontFillG = 1, frontFillB = 1;
            float frontOutR = 1, frontOutG = 1, frontOutB = 1;
            if (!frontC1.empty()) GetNamedColourRGB(frontC1, frontFillR, frontFillG, frontFillB);
            if (!frontC2.empty()) GetNamedColourRGB(frontC2, frontOutR, frontOutG, frontOutB);
            SetAnimationPaletteBank(m, frontAnimVar, frontPaletteBank);
            SetAnimationPaletteColour(m, frontAnimVar,
                frontOutR, frontOutG, frontOutB,
                frontFillR, frontFillG, frontFillB);
            pairDrawn = DrawAnimationOverlayWithTransform(m, animVar,
                entryMat.a, entryMat.b, entryMat.c, entryMat.d,
                entryMat.e, entryMat.f, entryMat.z);
        }
        if (pairDrawn) {
            skipPairedEntry = true;
        } else {
            DrawAnimationWithTransform(m, animVar, entryMat.a, entryMat.b, entryMat.c, entryMat.d,
                                        entryMat.e, entryMat.f, entryMat.z);
        }
    }
    (void)a_entityVar;
}

std::vector<int> DrawEntitiesWithNativeDrawHook() {
    std::vector<int> drawn;
    gmMachine* m = GetEngineContext().machine;
    if (!m) return drawn;
    std::vector<DrawListSubmission> orderedLists;
    // Colour writes ON before the entity draw sweep -- same reasoning as
    // DrawGroundsNative (see its comment). Script draw lists mask colour to
    // lay down stencil and do not reliably balance it back, so an entity
    // drawing after one of those would silently write nothing.
    if (Renderer* r = GetEngineContext().renderer) r->SetColourMask(false);
    // draw() can call EntityManager.flush(), which mutates the active vector.
    // Keep only IDs here and resolve each live entity just before dispatch.
    std::vector<int> ids;
    const auto drawMap = GetEngineContext().video.drawMapEntities;
    std::set<int> drawMapIds;
    std::set<int> queuedIds;
    const bool cachedVisibility = GetCachedVisibleEntityIds(*m, true, ids);
    if (!cachedVisibility) {
        ids.reserve(Entities().entities.size() + drawMap.size());
        for (const auto& e : Entities().entities) ids.push_back(e.first);
    }
    for (int id : ids) queuedIds.insert(id);
    // Retail's Video draw-map is a second ordered source of drawables. The
    // desktop ENTITY adapter shares this dispatcher so an entity that is
    // disabled for movement sync can still draw. Avoid double dispatch when
    // the same entity is also in the ordinary manager list.
    for (const auto& entry : drawMap) {
        drawMapIds.insert(entry.second);
        if (queuedIds.insert(entry.second).second) ids.push_back(entry.second);
    }
    bool playerSubmissionQueued = false;
    for (int id : ids) {
        const bool drawMapEntity = drawMapIds.find(id) != drawMapIds.end();
        gmVariable* live = FindEntity(id);
        if (!live || live->m_type != GM_TABLE) continue;
        gmVariable entity = *live;
        gmTableObject* t = (gmTableObject*)m->GetObject(entity.m_value.m_ref);
        if (!t) continue;
        const gmVariable playerFlag = t->Get(m, "__isPlayer");
        if (playerFlag.m_type == GM_INT && playerFlag.m_value.m_int != 0) {
            // Retail dispatches the native callback at this point in the
            // traversal and queues its DrawList immediately afterward. Keep
            // this position in the stable equal-key sort instead of moving
            // the player to the end of the whole entity batch.
            orderedLists.push_back(CaptureNativePlayerDrawSubmission());
            playerSubmissionQueued = true;
            continue;
        }
        const bool hasScriptDraw = t->Get(m, "draw").m_type == GM_FUNCTION;
        gmVariable sauceFlag = gmVariable::s_null;
        if (!hasScriptDraw) {
            sauceFlag = t->Get(m, "__sauceBase");
            if (sauceFlag.m_type != GM_INT || sauceFlag.m_value.m_int == 0)
                continue;
        }
        // Bullet-cache reclaim hides pooled projectiles with the native
        // Entity.setIsInvisible flag but keeps their entity/body allocated
        // for reuse. The real entity draw dispatcher honours this flag;
        // without this gate, BlockCannonBullet's final sparse explosion
        // frame (the small residual pixels in its shipped FSA) remains
        // visible after giveBackToBulletCache().
        gmVariable invisible = t->Get(m, "__invisible");
        if (invisible.m_type == GM_INT && invisible.m_value.m_int != 0) {
            continue;
        }
        // Entities created during a process update are visible to this draw
        // sweep before the next frame's configure sweep. Retail only draws
        // configured scene entities; calling their script draw early reads
        // unset fields (BlockCannon.width/height is a reproducible case).
        gmVariable configured = t->Get(m, "__nativeConfigured");
        if (configured.m_type != GM_INT || configured.m_value.m_int == 0) {
            // Runtime projectiles such as VicVolcanoSpit are constructed
            // with a body and then claimed from a bullet cache. They never
            // receive a level configureData.entType or call configure(), so
            // the scene-entity gate above hid every projectile despite the
            // music trigger creating and updating it. A claimed projectile
            // is enabled and has a body; a placed scene entity still waits
            // for its configure trampoline before drawing.
            gmVariable cd = t->Get(m, "configureData");
            gmTableObject* config = cd.m_type == GM_TABLE
                ? (gmTableObject*)m->GetObject(cd.m_value.m_ref) : nullptr;
            gmVariable enabled = t->Get(m, "__enabled");
            if (config && config->Get(m, "entType").m_type != GM_NULL) {
                continue;
            }
            if (enabled.m_type != GM_INT || enabled.m_value.m_int == 0 ||
                !PhysicsDataHasBodies(*m, t->Get(m, "PhysicsData"))) {
                continue;
            }
        }
        // Retail's screen lists never dispatch draw for a level entity whose
        // registered extent excludes the current screen. Our flat entity
        // vector otherwise draws old-screen flowers whose heads remain in
        // view after their stems and bodies have been culled.
        if (!drawMapEntity && !cachedVisibility &&
            !EntityIsOnCurrentScreen(*m, t, true)) continue;
        gmVariable enabled = t->Get(m, "__enabled");
        gmVariable physicsData = t->Get(m, "PhysicsData");
        if (enabled.m_type == GM_INT && enabled.m_value.m_int == 0 &&
            !PhysicsDataHasBodies(*m, physicsData) && !drawMapEntity) continue;
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
        if (!hasScriptDraw) {
            // See DrawSauceBaseEntityNative's own header comment -- SauceBase-
            // family entities (background clouds/bushes/trees) never get a
            // real `draw` GM_FUNCTION at all (their reference implementation
            // is real, live, dead script code, moved to native on retail),
            // so without this they were silently skipped entirely.
            if (sauceFlag.m_type == GM_INT && sauceFlag.m_value.m_int != 0) {
                DrawSauceBaseEntityNative(*m, t, entity);
                drawn.push_back(id);
            }
            continue;
        }

        gmCall call;
        // Per-ENTITY colour-write reset, not just once per sweep: each
        // entity's draw() is an independent unit, and one entity that masks
        // colour for a stencil pass without balancing it back would
        // otherwise silently blank every entity drawn after it. Measured
        // doing exactly that -- the tutorial's TutorialMessage glyphs were
        // being emitted with colorMask=0000.
        if (Renderer* rr = GetEngineContext().renderer) rr->SetColourMask(false);
        if (call.BeginTableFunction(m, "draw", t, entity)) {
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
        // `t`/`entity`: draw() can run arbitrary script, including deletion.
        gmVariable* freshVar = FindEntity(id);
        if (freshVar && freshVar->m_type == GM_TABLE) {
            gmTableObject* freshTable = (gmTableObject*)m->GetObject(freshVar->m_value.m_ref);
            gmVariable listVar = freshTable ? freshTable->Get(m, "drawList") : gmVariable::s_null;
            if (listVar.m_type >= GM_USER) { // see process_manager.cpp's FlushDrawList for why not `==`
                gmUserObject* obj = (gmUserObject*)m->GetObject(listVar.m_value.m_ref);
                if (obj && obj->m_userType == GetDrawListType()) {
                    DrawListData* listData = (DrawListData*)obj->m_user;
                    DrawListSubmission submission =
                        CaptureDrawListSubmission(listData);
                    orderedLists.push_back(submission);
                }
            }
        }
        drawn.push_back(id);
    }
    // Retail's native player draw is not gated by desktop script-draw
    // eligibility checks. Ordinarily it was queued at its traversal position
    // above; retain a fallback for frames where its entity is absent from the
    // active registry, such as the transition into or out of a level.
    if (!playerSubmissionQueued)
        orderedLists.push_back(CaptureNativePlayerDrawSubmission());
    ReplayDrawListsInOrder(orderedLists);
    return drawn;
}

void PreDrawEntitiesWithNativeHook() {
    gmMachine* m = GetEngineContext().machine;
    if (!m) return;
    std::vector<DrawListSubmission> orderedLists;
    std::vector<int> ids;
    const bool cachedVisibility = GetCachedVisibleEntityIds(*m, true, ids);
    if (!cachedVisibility) {
        ids.reserve(Entities().entities.size());
        for (const auto& e : Entities().entities) ids.push_back(e.first);
    }
    for (int id : ids) {
        gmVariable* live = FindEntity(id);
        if (!live || live->m_type != GM_TABLE) continue;
        gmVariable entity = *live;
        gmTableObject* t = (gmTableObject*)m->GetObject(entity.m_value.m_ref);
        if (!t) continue;
        if (t->Get(m, "preDraw").m_type != GM_FUNCTION) continue;
        gmVariable configured = t->Get(m, "__nativeConfigured");
        if (configured.m_type != GM_INT || configured.m_value.m_int == 0) continue;
        if (!cachedVisibility && !EntityIsOnCurrentScreen(*m, t, true)) continue;
        gmVariable enabled = t->Get(m, "__enabled");
        gmVariable physicsData = t->Get(m, "PhysicsData");
        if (enabled.m_type == GM_INT && enabled.m_value.m_int == 0 &&
            !PhysicsDataHasBodies(*m, physicsData)) continue;
        if (Renderer* r = GetEngineContext().renderer) r->SetColourMask(false);
        gmCall call;
        if (call.BeginTableFunction(m, "preDraw", t, entity)) call.End();

        // Entity preDraw uses the same instance drawList as draw(). Replay
        // immediately, before normal entity/player rendering can reset it.
        gmVariable* freshVar = FindEntity(id);
        if (!freshVar || freshVar->m_type != GM_TABLE) continue;
        gmTableObject* freshTable = (gmTableObject*)m->GetObject(freshVar->m_value.m_ref);
        gmVariable listVar = freshTable ? freshTable->Get(m, "drawList") : gmVariable::s_null;
        if (listVar.m_type < GM_USER) continue;
        gmUserObject* obj = (gmUserObject*)m->GetObject(listVar.m_value.m_ref);
        if (obj && obj->m_userType == GetDrawListType())
            orderedLists.push_back(CaptureDrawListSubmission((DrawListData*)obj->m_user));
    }
    ReplayDrawListsInOrder(orderedLists);
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
    std::vector<DrawListSubmission> orderedLists;
    std::vector<int> ids;
    const bool cachedVisibility = GetCachedVisibleEntityIds(*m, true, ids);
    if (!cachedVisibility) {
        ids.reserve(Entities().entities.size());
        for (const auto& e : Entities().entities) ids.push_back(e.first);
    }
    for (int id : ids) {
        gmVariable* live = FindEntity(id);
        if (!live || live->m_type != GM_TABLE) continue;
        gmVariable entity = *live;
        gmTableObject* t = (gmTableObject*)m->GetObject(entity.m_value.m_ref);
        if (!t) continue;
        if (t->Get(m, "drawAlpha").m_type != GM_FUNCTION) continue;
        gmVariable invisible = t->Get(m, "__invisible");
        if (invisible.m_type == GM_INT && invisible.m_value.m_int != 0) continue;
        if (!cachedVisibility && !EntityIsOnCurrentScreen(*m, t, true)) continue;
        gmCall call;
        if (call.BeginTableFunction(m, "drawAlpha", t, entity)) {
            call.End();
        }
        // Same drawList-replay requirement as the draw() pass above --
        // drawAlpha() bodies use the identical `this.drawList`/Video.*
        // immediate-mode calls (PekkoAndJon/Player.gm's drawAlpha:
        // Video.pushMatrix/translate/scale/setColour/popMatrix directly,
        // no separate drawList in this corpus, but other real drawAlpha
        // implementations could use one the same way draw() does).
        gmVariable* freshVar = FindEntity(id);
        if (freshVar && freshVar->m_type == GM_TABLE) {
            gmTableObject* freshTable = (gmTableObject*)m->GetObject(freshVar->m_value.m_ref);
            gmVariable listVar = freshTable ? freshTable->Get(m, "drawList") : gmVariable::s_null;
            if (listVar.m_type >= GM_USER) {
                gmUserObject* obj = (gmUserObject*)m->GetObject(listVar.m_value.m_ref);
                if (obj && obj->m_userType == GetDrawListType()) {
                    orderedLists.push_back(CaptureDrawListSubmission((DrawListData*)obj->m_user));
                }
            }
        }
        drawn.push_back(id);
    }
    ReplayDrawListsInOrder(orderedLists);
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
// reimplementation doesn't have that system, so nothing was ever driving
// that second half. isOnCurrentScreen() now has retail's bounds comparison,
// but that query alone does not perform lazy configuration.
// gmf_entity_setIsEnabled's own new lazy-configure hook (above) covers the
// case where SOMETHING does eventually call setIsEnabled(true) on an
// unconfigured entity, but for entities where NOTHING ever does (since
// registerEntity, the thing that calls setIsEnabled(true), only runs from
// inside configure()'s own tail -- a chicken-and-egg gap for anything
// configure() was never called on in the first place), that hook never
// fires at all. This sweep is the other half: once per frame, configure any
// eligible entity that's had configureData set
// but still isn't __nativeConfigured. If retail's saved screen range is
// present, the sweep waits for that range to become active; legacy/runtime
// entities without a range still use the compatibility fallback below.
// `configuring` (in-flight) entities are skipped naturally since
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

        // A screen transition may have asked to enable an entity before its
        // first lazy configure pass. Reapply only that recorded request, and
        // only once the real body array exists and the game has resumed
        // entity simulation. This closes the port's process-update/configure
        // ordering gap while leaving the bytecode-verified native setter's
        // bodyless rejection intact.
        gmVariable pendingEnable = t->Get(m, "__pendingEnableAfterConfigure");
        const bool hasPendingEnable = pendingEnable.m_type == GM_INT &&
                                      pendingEnable.m_value.m_int != 0;
        if (hasPendingEnable &&
            GetEngineContext().gameSystem.shouldUpdateEntities &&
            PhysicsDataHasBodies(*m, t->Get(m, "PhysicsData")) &&
            EntityIsOnCurrentScreen(*m, t, false)) {
            t->Set(m, "__pendingEnableAfterConfigure", gmVariable((gmint)0));
            gmVariable selfVar(t);
            gmCall enableCall;
            if (enableCall.BeginTableFunction(m, "setIsEnabled", t, selfVar)) {
                enableCall.AddParamInt(1);
                enableCall.End();
            }
            static const bool s_craigEnableDebug =
                std::getenv("GE3_CRAIG_UPDATE_DEBUG") != nullptr;
            if (s_craigEnableDebug) {
                gmVariable id = t->Get(m, "__objectID");
                std::fprintf(stderr, "[entity-enable] replayed deferred request: id=%d\n",
                    id.m_type == GM_INT ? id.m_value.m_int : e.first);
            }
        }

        gmVariable configuredVar = t->Get(m, "__nativeConfigured");
        if (configuredVar.m_type == GM_INT && configuredVar.m_value.m_int != 0) {
            // Already configured -- but NOT necessarily still valid.
            // EntityBase.gm's ENTITYBASE_killBodiesIfItShould() tears an
            // entity's bodies down when it leaves the screen:
            //     me.PhysicsData.removeBody(-1);
            //     me.mainpb = null;
            // Retail pairs that with a lazy RE-configure when the entity
            // scrolls back into view. This sweep only ever configured each
            // entity once, so after a kill the entity was skipped forever
            // and left with a permanently null mainpb.
            //
            // That single defect produced two separate visible symptoms:
            //   - every tutorial string drew at the world origin, because
            //     TutorialMessage.gm does
            //     `drawList.translate(this.mainpb.getPos(), z)` and a null
            //     vector translate keeps z but zeroes x/y (measured: 24766
            //     null-vector translates, 12810 text draws at world(0,0)
            //     with correct z of +-19.91)
            //   - MATHUTIL_GetAngleRad's "expecting a vector2D" storm from
            //     GoAroundGuy/BrianMover's `this.mainpb.getDir()`,
            //     ~2420 errors per run
            //
            // Only re-configure entities that DID have a body once
            // (__hadMainpb, set by the configure trampoline). Entities with
            // no physics at all must not be re-configured every frame.
            gmVariable hadBody = t->Get(m, "__hadMainpb");
            bool everHadBody = (hadBody.m_type == GM_INT && hadBody.m_value.m_int != 0);
            if (!everHadBody || t->Get(m, "mainpb").m_type != GM_NULL) continue;
            // Bodies were killed -- fall through and configure() again,
            // which rebuilds them and repositions from configureData.
            t->Set(m, "__nativeConfigured", gmVariable((gmint)0));
        }
        gmVariable inFlightVar = t->Get(m, "__nativeConfiguring");
        if (inFlightVar.m_type == GM_INT && inFlightVar.m_value.m_int != 0) continue;

        gmVariable cdVar = t->Get(m, "configureData");
        if (cdVar.m_type != GM_TABLE) continue; // nothing to configure with yet
        // ...and not until the loader has actually FILLED it. A bare
        // `configureData = {}` exists on an entity before its level-data
        // fields are copied in, and configuring then runs configure() against
        // an empty table: every field it reads comes back null, so e.g.
        // TutorialMessage's `pb.setPos(configureData.posX, configureData.posY)`
        // silently placed the body at the origin (measured: the text's
        // draw-time matrix carried translation e=0,f=0 while its z came
        // through correctly, and every glyph in the level rendered at world
        // (0,0) -- NDC (25,7) from the spawn camera, far off screen).
        //
        // `entType` is the game's own marker for a populated configureData --
        // EntityBase.gm's endConfigure() gates on exactly `if (!?this.
        // configureData.entType)` -- and the tutorial level's data carries it
        // on all 230 placed entities.
        gmTableObject* cdTable = (gmTableObject*)m->GetObject(cdVar.m_value.m_ref);
        if (!cdTable || cdTable->Get(m, "entType").m_type == GM_NULL) continue;
        // Main.gm deliberately leaves eligible level entities unconfigured
        // until their saved screen range becomes active. The portable
        // fallback used to configure every placed entity immediately,
        // creating offscreen physics bodies that Box2D continued simulating
        // while the per-screen entity update pass skipped their callbacks.
        // For entities with the native screen bounds installed by
        // initializeScreenExtents, preserve that lazy activation boundary.
        // Runtime entities without a recorded range retain the fallback
        // path below because they have no retail screen-membership metadata.
        if (t->Get(m, "__screenMinCX").m_type != GM_NULL &&
            !EntityIsOnCurrentScreen(*m, t, false)) continue;
        gmVariable configureFn = t->Get(m, "configure");
        if (configureFn.m_type != GM_FUNCTION) continue; // no configure() at all, e.g. non-ENTITYBASE types

        gmVariable selfVar(t);
        gmCall call;
        if (call.BeginTableFunction(m, "configure", t, selfVar)) {
            call.AddParam(cdVar);
            call.End();
        }
        // configure() can be the call that finally builds a body for an
        // entity whose activation request arrived during this frame's process
        // update. Run the narrow pending-enable check again now that its body
        // array and screen registration are current.
        pendingEnable = t->Get(m, "__pendingEnableAfterConfigure");
        if (pendingEnable.m_type == GM_INT && pendingEnable.m_value.m_int != 0) {
            const bool shouldUpdate = GetEngineContext().gameSystem.shouldUpdateEntities;
            const bool hasBodies = PhysicsDataHasBodies(*m, t->Get(m, "PhysicsData"));
            const bool onCurrentScreen = EntityIsOnCurrentScreen(*m, t, false);
            gmVariable id = t->Get(m, "__objectID");
            static std::set<int> s_pendingEnableReported;
            const int objectId = id.m_type == GM_INT ? id.m_value.m_int : e.first;
            if (s_pendingEnableReported.insert(objectId).second) {
                std::fprintf(stderr,
                    "[entity-enable] replay check id=%d update=%d bodies=%d onScreen=%d\n",
                    objectId, shouldUpdate ? 1 : 0, hasBodies ? 1 : 0,
                    onCurrentScreen ? 1 : 0);
            }
            if (shouldUpdate && hasBodies && onCurrentScreen) {
                t->Set(m, "__pendingEnableAfterConfigure", gmVariable((gmint)0));
                gmCall enableCall;
                if (enableCall.BeginTableFunction(m, "setIsEnabled", t, selfVar)) {
                    enableCall.AddParamInt(1);
                    enableCall.End();
                }
            }
        }
        // configure() runs EntityBase.endConfigure(), which records screen
        // extents, registers the entity with Main.gm, and calls
        // setIsEnabled(isOnCurrentScreen()). Preserve that retail result;
        // forcing true here reinserted off-screen entities into the native
        // update/draw/physics memberships immediately after registration.
    }
}

// Calls entity.update(msElapsed) on the manager's enabled-entity membership.
// Retail bytecode verification: FUN_0018836c traverses the enabled-entity
// tree and tests the entity's native enabled byte (+0xd); Main.gm maintains
// that membership in registerEntity and on screen transitions. Do not add a
// second screen-bounds test here: that would suppress callbacks for an entity
// retail still considers enabled. GameMonkey scripts drive their OWN
// per-type state machines from inside `update` (e.g.
// EntityPacks/SpawnPoint.gm's `update=function(msElapsed){ ...
// this.state.update(msElapsed); }`, which is what actually advances a
// spawn/trigger/idle FSA once fsa/bci playback has been kicked off by
// `play()`/`playOnce()`) -- with this never invoked, EVERY entity's update
// was silently dead code: no animation ever advanced, no per-entity timer
// ever ticked, regardless of how correctly its own state-machine logic was
// written. Retail's constructorSetNoUpdate entities are excluded from this
// native callback list; the portable sweep honors the corresponding marker.
void UpdateEntitiesWithNativeUpdateHook(float a_msElapsed) {
    gmMachine* m = GetEngineContext().machine;
    if (!m) return;
    ConfigureUnconfiguredEntities();
    // Snapshot first: an entity's own update() commonly creates/destroys
    // other entities (spawning, death, projectiles, ...), which would
    // reallocate/mutate Entities().entities out from under a live range-for.
    std::vector<std::pair<int, gmVariable>> snapshot = Entities().entities;
    static const bool s_craigUpdateDebug = std::getenv("GE3_CRAIG_UPDATE_DEBUG") != nullptr;
    static std::map<int, int> s_craigUpdateState;
    static const bool s_craigPhysicsTrace =
        std::getenv("GE3_CRAIG_PHYSICS_TRACE") != nullptr;
    static unsigned int s_craigPhysicsTraceTick = 0;
    const unsigned int craigPhysicsTraceTick = ++s_craigPhysicsTraceTick;
    for (auto& e : snapshot) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        gmVariable noUpdate = t->Get(m, "__noUpdate");
        const bool markedNoUpdate = noUpdate.m_type == GM_INT && noUpdate.m_value.m_int != 0;
        // RETAIL PPC: FUN_0018836c walks the EntityManager enabled-entity
        // tree and filters each native ENTITY on byte +0xd. Entity.setIsEnabled
        // (FUN_000c02f8) removes/reinserts that object in the tree immediately.
        // Screen membership alone is insufficient: beginMoveSyncScheme and
        // screen transitions can disable an entity while its PhysicsData still
        // owns bodies. Those bodies must not make its script update run.
        gmVariable enabled = t->Get(m, "__enabled");
        const bool isEnabled = enabled.m_type != GM_INT || enabled.m_value.m_int != 0;
        const bool hasUpdate = t->Get(m, "update").m_type == GM_FUNCTION;
        const gmVariable playerFlag = t->Get(m, "__isPlayer");
        const bool isPlayerEntity = playerFlag.m_type == GM_INT &&
                                    playerFlag.m_value.m_int != 0;
        if (s_craigUpdateDebug || s_craigPhysicsTrace) {
            gmVariable typeVar = t->Get(m, "__typeName");
            const char* typeName = typeVar.m_type == GM_STRING && typeVar.m_value.m_ref
                ? ((gmStringObject*)m->GetObject(typeVar.m_value.m_ref))->GetString() : "";
            if (std::strstr(typeName, "CraigElevator") ||
                std::strstr(typeName, "CraigElevatorExit") ||
                std::strstr(typeName, "CraigLiftComputer") ||
                std::strstr(typeName, "CraigOfficeDrone") ||
                std::strstr(typeName, "CraigBoardroomExec") ||
                std::strstr(typeName, "CraigCoffeeDrinker")) {
                // Screen bounds are diagnostic only. Retail membership is
                // the enabled-entity tree, mirrored by __enabled above.
                const bool onScreen = EntityIsOnCurrentScreen(*m, t, true);
                const int state = (onScreen ? 1 : 0) | (isEnabled ? 2 : 0) |
                    (markedNoUpdate ? 4 : 0) | (hasUpdate ? 8 : 0);
                auto previous = s_craigUpdateState.find(e.first);
                gmVariable scriptState = t->Get(m, "state");
                gmVariable scriptStateId = gmVariable::s_null;
                if (scriptState.m_type == GM_TABLE) {
                    auto* stateTable = (gmTableObject*)m->GetObject(scriptState.m_value.m_ref);
                    if (stateTable) scriptStateId = stateTable->Get(m, "id");
                }
                const int stateId = scriptStateId.m_type == GM_INT
                    ? scriptStateId.m_value.m_int : -1;
                const int loggedState = state | ((stateId < 0 ? 0xff : stateId) << 4);
                gmVariable mainBody = t->Get(m, "mainpb");
                const b2Body* physicsBody = GetB2BodyFromVar(*m, mainBody);
                const b2Vec2 bodyPos = physicsBody
                    ? physicsBody->GetPosition() : b2Vec2(0.0f, 0.0f);
                if (s_craigUpdateDebug &&
                    (previous == s_craigUpdateState.end() || previous->second != loggedState)) {
                    gmVariable idVar = t->Get(m, "__objectID");
                    gmVariable cdVar = t->Get(m, "configureData");
                    gmVariable configured = t->Get(m, "__nativeConfigured");
                    gmVariable entType = gmVariable::s_null;
                    gmVariable parent = gmVariable::s_null;
                    if (cdVar.m_type == GM_TABLE) {
                        auto* cd = (gmTableObject*)m->GetObject(cdVar.m_value.m_ref);
                        if (cd) {
                            parent = cd->Get(m, "parentEntity");
                            entType = cd->Get(m, "entType");
                        }
                    }
                    std::fprintf(stderr,
                        "[craig-update] id=%d type=%s onScreen=%d enabled=%d noUpdate=%d "
                        "hasUpdate=%d configured=%d stateId=%d mainpbType=%d "
                        "pos=(%.3f,%.3f) entTypeType=%d parentEntityType=%d\n",
                        idVar.m_type == GM_INT ? idVar.m_value.m_int : e.first,
                        typeName, onScreen ? 1 : 0, isEnabled ? 1 : 0,
                        markedNoUpdate ? 1 : 0, hasUpdate ? 1 : 0,
                        configured.m_type == GM_INT ? configured.m_value.m_int : -1,
                        stateId, mainBody.m_type, bodyPos.x, bodyPos.y,
                        entType.m_type, parent.m_type);
                    s_craigUpdateState[e.first] = loggedState;
                }
                // Diagnostic for the reported Corporeal room-entry failure.
                // State/position-only transition logs can miss gravity or
                // drift between state changes, so sample live Box2D mass,
                // velocity, awake state, and fixtures at a low rate.
                if (s_craigPhysicsTrace && isEnabled && physicsBody &&
                    (craigPhysicsTraceTick % 60u) == 0u) {
                    int fixtureCount = 0;
                    int sensorCount = 0;
                    for (const b2Fixture* fixture = physicsBody->GetFixtureList();
                         fixture; fixture = fixture->GetNext()) {
                        ++fixtureCount;
                        if (fixture->IsSensor()) ++sensorCount;
                    }
                    const b2Vec2 velocity = physicsBody->GetLinearVelocity();
                    std::fprintf(stderr,
                        "[craig-physics] tick=%u id=%d type=%s stateId=%d "
                        "pos=(%.4f,%.4f) velocityBox2D=(%.6f,%.6f) "
                        "mass=%.6f awake=%d fixtures=%d sensors=%d\n",
                        craigPhysicsTraceTick,
                        e.first,
                        typeName, stateId, bodyPos.x, bodyPos.y,
                        velocity.x, velocity.y, physicsBody->GetMass(),
                        physicsBody->IsAwake() ? 1 : 0,
                        fixtureCount, sensorCount);
                }
            }
            // VERIFICATION INSTRUMENTATION: include the player in the same
            // low-rate body trace so screen-to-screen route probes can prove
            // whether movement reached a later room before drawing conclusions
            // about its workers/lifts. This is diagnostic only and does not
            // change retail update order or entity state.
            if (s_craigPhysicsTrace && isPlayerEntity && isEnabled &&
                (craigPhysicsTraceTick % 60u) == 0u) {
                gmVariable playerBodyVar = t->Get(m, "mainpb");
                const b2Body* playerBody = GetB2BodyFromVar(*m, playerBodyVar);
                if (playerBody) {
                    const b2Vec2 playerPosition = playerBody->GetPosition();
                    const b2Vec2 velocity = playerBody->GetLinearVelocity();
                    std::fprintf(stderr,
                        "[player-physics] tick=%u id=%d pos=(%.4f,%.4f) "
                        "velocityBox2D=(%.6f,%.6f) mass=%.6f awake=%d\n",
                        craigPhysicsTraceTick, e.first,
                        playerPosition.x, playerPosition.y, velocity.x, velocity.y,
                        playerBody->GetMass(), playerBody->IsAwake() ? 1 : 0);
                }
            }
        }
        if (markedNoUpdate || !isEnabled) continue;

        if (hasUpdate) {
            gmCall call;
            if (call.BeginTableFunction(m, "update", t, e.second)) {
                call.AddParamFloat(a_msElapsed);
                call.End();
            }
        }

        // Retail PlayerRenderer::update (FUN_001a0b60) invokes its base
        // update first; FUN_0005d17c runs the cached Player update callback,
        // then the derived method advances the draw-scale transition. Keep
        // that native scale tick adjacent to the enabled player's script
        // update, and skip it with disabled/no-update entities.
        if (isPlayerEntity) TickPlayerNativeRenderer(a_msElapsed);
    }
}

void PostUpdateEntitiesWithNativeHook(float a_msElapsed) {
    gmMachine* m = GetEngineContext().machine;
    if (!m) return;
    // Match the update pass's snapshot and eligibility rules. Entity
    // postUpdate can create/destroy entities too.
    std::vector<std::pair<int, gmVariable>> snapshot = Entities().entities;
    for (auto& e : snapshot) {
        if (e.second.m_type != GM_TABLE) continue;
        gmTableObject* t = (gmTableObject*)m->GetObject(e.second.m_value.m_ref);
        if (!t) continue;
        gmVariable noUpdate = t->Get(m, "__noUpdate");
        if (noUpdate.m_type == GM_INT && noUpdate.m_value.m_int != 0) continue;
        // Same recovered enabled-tree membership as update: retail's entity
        // callback dispatcher does not invoke postUpdate on disabled entities
        // just because their PhysicsData still owns bodies.
        gmVariable enabled = t->Get(m, "__enabled");
        if (enabled.m_type == GM_INT && enabled.m_value.m_int == 0) continue;
        if (t->Get(m, "postUpdate").m_type != GM_FUNCTION) continue;

        gmCall call;
        if (call.BeginTableFunction(m, "postUpdate", t, e.second)) {
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

    g_instrumentType = a_machine.CreateUserType("Instrument");
    a_machine.RegisterTypeLibrary(g_instrumentType, s_instrumentLib, sizeof(s_instrumentLib) / sizeof(s_instrumentLib[0]));
    a_machine.RegisterUserCallbacks(g_instrumentType, nullptr, InstrumentGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("INSTRUMENT", gmf_INSTRUMENT);
    a_machine.RegisterLibraryFunction("ENTITY_GetType", gmf_ENTITY_GetType);
    a_machine.RegisterLibraryFunction("ENTITY_GetTypeName", gmf_ENTITY_GetTypeName);
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
    a_machine.RegisterLibraryFunction("GAME_CalculateCurveFillVerts", gmf_GAME_CalculateCurveFillVerts);
    a_machine.RegisterLibraryFunction("GAME_GetPreDrawList", gmf_GAME_GetPreDrawList);
    a_machine.RegisterLibraryFunction("GAME_GetPostDrawList", gmf_GAME_GetPostDrawList);
    a_machine.RegisterLibraryFunction("GAME_SetSelectionPulseRate", gmf_GAME_SetSelectionPulseRate);
    a_machine.RegisterLibraryFunction("GAME_SetSelectionAlphaPerc", gmf_GAME_SetSelectionAlphaPerc);
    a_machine.RegisterLibraryFunction("GAME_SetIsSelectionPulseEnabled", gmf_GAME_SetIsSelectionPulseEnabled);
    a_machine.RegisterLibraryFunction("GAME_SetIsUsingSelectionFragmentProgram", gmf_GAME_SetIsUsingSelectionFragmentProgram);
    a_machine.RegisterLibraryFunction("ENTITYLISTREFCONTAINER", gmf_ENTITYLISTREFCONTAINER);

    // Real ENTITYLISTREFCONTAINER-shaped iterator every EntityManager.
    // getAllXXXEntities()/getEntitiesNear()/getEntitiesRoughlyWithin() call
    // returns -- see PushEntityListIterator's own header comment for why
    // this replaced a bare gm_table().
    g_entityListIterType = a_machine.CreateUserType("ENTITYLISTREFCONTAINER");
    a_machine.RegisterTypeLibrary(g_entityListIterType, s_entityListIterLib, sizeof(s_entityListIterLib) / sizeof(s_entityListIterLib[0]));
    a_machine.RegisterUserCallbacks(g_entityListIterType, nullptr, EntityListIterGCDestruct, nullptr);

    a_machine.RegisterLibrary(s_entityManagerLib, sizeof(s_entityManagerLib) / sizeof(s_entityManagerLib[0]), "EntityManager");

    // Retail FUN_000bf9a0 (called by ENTITY_GetMixerIDFromType) adds
    // 0x000f4240 to the type ID. Mixer.gm uses this same global offset
    // when registering per-entity mixer channels.
    a_machine.GetGlobals()->Set(&a_machine, "MIXER_AUTO_ID_START", gmVariable((gmint)1000000));
}
