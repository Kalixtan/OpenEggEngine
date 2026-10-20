#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "physics_world.h"
#include "engine_context.h"
#include "../bindings/physics_body_bindings.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "gmCall.h"
#include <box2d/box2d.h>
#include <algorithm>
#include <set>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
b2World* g_world = nullptr;
int g_numFramesToIgnoreCollisions = 0;

static std::set<b2Body*> g_restitutionDisabledBodies;
// Retail disableContact ORs a bit into the native contact object. Keep the
// local equivalent for the lifetime of that contact; Box2D resets its own
// enabled flag at each step, so PreSolve reapplies suppression until
// EndContact removes the contact from the native contact map.
static std::set<b2Contact*> g_disabledContacts;
static std::set<b2Body*> g_entityPhysicsBodiesBeingDestroyed;

struct DeferredContactEvent {
    gmVariable self;
    gmVariable other;
    const char* method;
};
void DispatchContactEvent(gmMachine* a_machine, const DeferredContactEvent& a_event);
bool IsPlayerBody(gmMachine& a_machine, b2Body* a_body) {
    gmVariable owner = FindOwnerEntityForB2Body(a_body);
    if (owner.m_type != GM_TABLE) return false;
    gmTableObject* table = (gmTableObject*)a_machine.GetObject(owner.m_value.m_ref);
    if (!table) return false;
    gmVariable flag = table->Get(&a_machine, "__isPlayer");
    return flag.m_type == GM_INT && flag.m_value.m_int != 0;
}

bool IsCreatureDebugEntity(gmMachine* a_machine, const gmVariable& a_entity,
                           int& a_id) {
    if (!a_machine || a_entity.m_type != GM_TABLE) return false;
    auto* table = (gmTableObject*)a_machine->GetObject(a_entity.m_value.m_ref);
    if (!table) return false;
    const gmVariable type = table->Get(a_machine, "__typeName");
    if (type.m_type != GM_STRING || !type.m_value.m_ref) return false;
    auto* name = (gmStringObject*)a_machine->GetObject(type.m_value.m_ref);
    if (!name || (std::strcmp(name->GetString(),
            "Platformer_EntityPacks_GoombaCritterGuitar3") != 0 &&
        std::strcmp(name->GetString(),
            "Platformer_EntityPacks_SlimeEyes") != 0)) return false;
    const gmVariable id = table->Get(a_machine, "__objectID");
    a_id = id.m_type == GM_INT ? id.m_value.m_int : -1;
    return true;
}

bool CreatureDebugEnabled() {
    static const bool enabled = std::getenv("GE3_CREATURE_DEBUG") != nullptr;
    return enabled;
}

int EntityCollisionId(gmMachine* a_machine, const gmVariable& a_entity) {
    if (!a_machine || a_entity.m_type != GM_TABLE) return -1;
    auto* table = (gmTableObject*)a_machine->GetObject(a_entity.m_value.m_ref);
    if (!table) return -1;
    const gmVariable id = table->Get(a_machine, "__objectID");
    return id.m_type == GM_INT ? id.m_value.m_int : -1;
}

bool IsNativeContactRecordMember(gmMachine* a_machine, const gmVariable& a_entity) {
    if (!a_machine || a_entity.m_type != GM_TABLE) return false;
    auto* table = (gmTableObject*)a_machine->GetObject(a_entity.m_value.m_ref);
    if (!table) return false;
    const gmVariable manual = table->Get(a_machine, "__isManual");
    const gmVariable noPhysicsCollide = table->Get(a_machine, "__noPhysicsCollide");
    return !(manual.m_type == GM_INT && manual.m_value.m_int != 0) &&
           !(noPhysicsCollide.m_type == GM_INT && noPhysicsCollide.m_value.m_int != 0);
}

bool IsEntityEnabledForPhysicsManager(gmMachine* a_machine, const gmVariable& a_entity) {
    if (!a_machine || a_entity.m_type != GM_TABLE) return false;
    auto* table = (gmTableObject*)a_machine->GetObject(a_entity.m_value.m_ref);
    if (!table) return false;
    const gmVariable enabled = table->Get(a_machine, "__enabled");
    return enabled.m_type == GM_INT && enabled.m_value.m_int != 0;
}

using EntityPairKey = std::pair<uintptr_t, uintptr_t>;
struct PairContactMembership {
    bool firstSide = false;
    bool secondSide = false;
    int firstMyShapeId = -1;
    int firstOtherShapeId = -1;
    float firstNormalSign = 1.0f;
    int secondMyShapeId = -1;
    int secondOtherShapeId = -1;
    float secondNormalSign = 1.0f;
};
struct ActiveEntityPair {
    gmVariable firstEntity = gmVariable::s_null;
    gmVariable secondEntity = gmVariable::s_null;
    int firstEntityId = -1;
    int secondEntityId = -1;
    uintptr_t firstPhysicsData = 0;
    uintptr_t secondPhysicsData = 0;
    std::vector<b2Contact*> contactOrder;
    std::unordered_map<b2Contact*, PairContactMembership> contacts;
};
std::map<EntityPairKey, ActiveEntityPair> g_activeEntityPairs;
std::unordered_map<b2Contact*, EntityPairKey> g_entityPairByContact;

uintptr_t EntityObjectIdentity(gmMachine* a_machine,
                               const gmVariable& a_entity) {
    if (!a_machine || a_entity.m_type != GM_TABLE) return 0;
    gmObject* object = a_machine->GetObject(a_entity.m_value.m_ref);
    return reinterpret_cast<uintptr_t>(object);
}

// CollisionInfo is keyed by directed PhysicsData identity in retail. Keep
// this record view separate from entity-pair callback aggregation because a
// single Entity may own multiple PhysicsData objects.
// Native PhysicsData pair maps are keyed by the other Entity's +4 ID. The
// local first component identifies which PhysicsData owns the directed map.
using PhysicsDataPairKey = std::pair<uintptr_t, uint32_t>;
struct ActivePhysicsDataPair {
    int myEntityId = -1;
    int otherEntityId = -1;
    uintptr_t otherPhysicsData = 0;
    gmVariable myEntity = gmVariable::s_null;
    gmVariable otherEntity = gmVariable::s_null;
    int createdTick = 0;
    std::vector<b2Contact*> contactOrder;
    std::unordered_map<b2Contact*, PhysicsContactRecordView> contacts;
};
std::map<PhysicsDataPairKey, ActivePhysicsDataPair> g_activePhysicsDataPairs;
std::unordered_map<b2Contact*, std::vector<PhysicsDataPairKey>>
    g_physicsDataPairsByContact;

struct SeparateContactEvent {
    PhysicsDataPairKey pairKey;
    gmVariable self;
    gmVariable other;
};

void RemovePhysicsDataContactRecord(
    b2Contact* a_contact, std::vector<SeparateContactEvent>* a_separateEvents = nullptr) {
    if (!a_contact) return;
    auto contactIt = g_physicsDataPairsByContact.find(a_contact);
    if (contactIt == g_physicsDataPairsByContact.end()) return;
    const std::vector<PhysicsDataPairKey> pairKeys = std::move(contactIt->second);
    g_physicsDataPairsByContact.erase(contactIt);
    for (const PhysicsDataPairKey& key : pairKeys) {
        auto pairIt = g_activePhysicsDataPairs.find(key);
        if (pairIt == g_activePhysicsDataPairs.end()) continue;
        auto& pair = pairIt->second;
        pair.contacts.erase(a_contact);
        pair.contactOrder.erase(
            std::remove(pair.contactOrder.begin(), pair.contactOrder.end(), a_contact),
            pair.contactOrder.end());
        if (pair.contacts.empty()) {
            if (a_separateEvents) {
                // Retail calls onEntitySeparate before erasing the now-empty
                // outer pair-map entry. Preserve that query-visible window.
                a_separateEvents->push_back(
                    {key, pair.myEntity, pair.otherEntity});
                ++pairIt;
            } else {
                InvalidatePhysicsCollisionInfoView(key.first, key.second);
                pairIt = g_activePhysicsDataPairs.erase(pairIt);
            }
        }
    }
}

void RegisterPhysicsDataContactRecord(b2Contact* a_contact,
                                      uintptr_t a_myPhysicsData,
                                      uintptr_t a_otherPhysicsData,
                                      int a_myShapeId, int a_otherShapeId,
                                      float a_normalSign, int a_myEntityId,
                                      int a_otherEntityId,
                                      const gmVariable& a_myEntity,
                                      const gmVariable& a_otherEntity) {
    if (!a_contact || a_myPhysicsData == 0 || a_otherPhysicsData == 0 ||
        a_myPhysicsData == a_otherPhysicsData) return;
    const PhysicsDataPairKey key{a_myPhysicsData,
                                 static_cast<uint32_t>(a_otherEntityId)};
    auto [pairIt, pairInserted] = g_activePhysicsDataPairs.try_emplace(key);
    ActivePhysicsDataPair& pair = pairIt->second;
    if (pairInserted) {
        // Retail initializes the pair value only when the Entity-ID key is
        // first inserted. Further fixture contacts append to its inner map;
        // they do not replace the mapped other PhysicsData pointer.
        pair.myEntityId = a_myEntityId;
        pair.otherEntityId = a_otherEntityId;
        pair.otherPhysicsData = a_otherPhysicsData;
        pair.myEntity = a_myEntity;
        pair.otherEntity = a_otherEntity;
        pair.createdTick = GetEngineContext().gameSystem.currentTick;
    }
    auto [it, inserted] = pair.contacts.try_emplace(
        a_contact, PhysicsContactRecordView{a_contact, a_myShapeId,
                                            a_otherShapeId, a_normalSign});
    (void)it;
    if (inserted) {
        pair.contactOrder.push_back(a_contact);
        g_physicsDataPairsByContact[a_contact].push_back(
            {a_myPhysicsData, static_cast<uint32_t>(a_otherEntityId)});
    }
}

void RegisterPairContactSide(gmMachine* a_machine, b2Contact* a_contact,
                             const gmVariable& a_self, const gmVariable& a_other) {
    if (!a_contact || !IsNativeContactRecordMember(a_machine, a_self)) return;
    const int selfId = EntityCollisionId(a_machine, a_self);
    const int otherId = EntityCollisionId(a_machine, a_other);
    if (selfId < 0 || otherId < 0 || selfId == otherId) return;

    const uintptr_t selfIdentity = EntityObjectIdentity(a_machine, a_self);
    const uintptr_t otherIdentity = EntityObjectIdentity(a_machine, a_other);
    if (selfIdentity == 0 || otherIdentity == 0 || selfIdentity == otherIdentity)
        return;
    const EntityPairKey key{std::min(selfIdentity, otherIdentity),
                            std::max(selfIdentity, otherIdentity)};
    ActiveEntityPair& pair = g_activeEntityPairs[key];
    if (pair.contacts.empty()) {
        // The native queue preserves whichever Entity ordering first reaches
        // it. Keep the first eligible contact side's order for the dispatch
        // recheck; only the lookup key is canonicalized for pair deduplication.
        pair.firstEntity = a_self;
        pair.secondEntity = a_other;
        pair.firstEntityId = selfId;
        pair.secondEntityId = otherId;
    }
    const bool selfIsFirst =
        selfIdentity == EntityObjectIdentity(a_machine, pair.firstEntity);
    auto [contactIt, inserted] = pair.contacts.try_emplace(a_contact);
    if (inserted) pair.contactOrder.push_back(a_contact);
    PairContactMembership& membership = contactIt->second;
    gmVariable fixtureOwnerA = FindOwnerEntityForB2Body(
        a_contact->GetFixtureA()->GetBody());
    const bool selfIsFixtureA =
        EntityObjectIdentity(a_machine, fixtureOwnerA) == selfIdentity;
    const int selfShapeId = static_cast<int>(
        selfIsFixtureA ? a_contact->GetFixtureA()->GetUserData().pointer
                       : a_contact->GetFixtureB()->GetUserData().pointer);
    const int otherShapeId = static_cast<int>(
        selfIsFixtureA ? a_contact->GetFixtureB()->GetUserData().pointer
                       : a_contact->GetFixtureA()->GetUserData().pointer);
    const float normalSign = selfIsFixtureA ? -1.0f : 1.0f;
    if (selfIsFirst) {
        membership.firstSide = true;
        membership.firstMyShapeId = selfShapeId;
        membership.firstOtherShapeId = otherShapeId;
        membership.firstNormalSign = normalSign;
    } else {
        membership.secondSide = true;
        membership.secondMyShapeId = selfShapeId;
        membership.secondOtherShapeId = otherShapeId;
        membership.secondNormalSign = normalSign;
    }
    g_entityPairByContact[a_contact] = key;
    const uintptr_t selfPhysicsData = static_cast<uintptr_t>(
        selfIsFixtureA ? a_contact->GetFixtureA()->GetBody()->GetUserData().pointer
                       : a_contact->GetFixtureB()->GetBody()->GetUserData().pointer);
    const uintptr_t otherPhysicsData = static_cast<uintptr_t>(
        selfIsFixtureA ? a_contact->GetFixtureB()->GetBody()->GetUserData().pointer
                       : a_contact->GetFixtureA()->GetBody()->GetUserData().pointer);
    if (selfIsFirst && pair.firstPhysicsData == 0)
        pair.firstPhysicsData = selfPhysicsData;
    if (!selfIsFirst && pair.secondPhysicsData == 0)
        pair.secondPhysicsData = selfPhysicsData;
    RegisterPhysicsDataContactRecord(a_contact, selfPhysicsData,
        otherPhysicsData, selfShapeId, otherShapeId, normalSign,
        selfId, otherId, a_self, a_other);
}

void RemovePairContact(b2Contact* a_contact,
                       std::vector<SeparateContactEvent>& a_separateEvents) {
    RemovePhysicsDataContactRecord(a_contact, &a_separateEvents);
    auto mapIt = g_entityPairByContact.find(a_contact);
    if (mapIt == g_entityPairByContact.end()) return;
    auto pairIt = g_activeEntityPairs.find(mapIt->second);
    if (pairIt != g_activeEntityPairs.end()) {
        pairIt->second.contacts.erase(a_contact);
        auto& order = pairIt->second.contactOrder;
        order.erase(std::remove(order.begin(), order.end(), a_contact), order.end());
        if (pairIt->second.contacts.empty()) g_activeEntityPairs.erase(pairIt);
    }
    g_entityPairByContact.erase(mapIt);
}

void DispatchSeparateEvents(const std::vector<SeparateContactEvent>& a_events,
                            bool a_callHandlers) {
    gmMachine* machine = a_callHandlers ? GetEngineContext().machine : nullptr;
    for (const SeparateContactEvent& event : a_events) {
        if (machine) {
            DispatchContactEvent(machine, DeferredContactEvent{
                event.self, event.other, "onEntitySeparate"});
        }
        auto pairIt = g_activePhysicsDataPairs.find(event.pairKey);
        if (pairIt != g_activePhysicsDataPairs.end() &&
            pairIt->second.contacts.empty()) {
            InvalidatePhysicsCollisionInfoView(event.pairKey.first,
                                               event.pairKey.second);
            g_activePhysicsDataPairs.erase(pairIt);
        }
    }
}

void RemoveEntityFromActivePairs(int a_entityId) {
    if (a_entityId < 0) return;
    for (auto pairIt = g_activeEntityPairs.begin(); pairIt != g_activeEntityPairs.end();) {
        if (pairIt->second.firstEntityId != a_entityId &&
            pairIt->second.secondEntityId != a_entityId) {
            ++pairIt;
            continue;
        }
        for (const auto& contact : pairIt->second.contacts) {
            g_entityPairByContact.erase(contact.first);
            g_physicsDataPairsByContact.erase(contact.first);
        }
        pairIt = g_activeEntityPairs.erase(pairIt);
    }
    for (auto pairIt = g_activePhysicsDataPairs.begin();
         pairIt != g_activePhysicsDataPairs.end();) {
        if (pairIt->second.myEntityId == a_entityId ||
            pairIt->second.otherEntityId == a_entityId) {
            InvalidatePhysicsCollisionInfoView(pairIt->first.first,
                                               pairIt->first.second);
            pairIt = g_activePhysicsDataPairs.erase(pairIt);
        } else
            ++pairIt;
    }
}

void RemoveBodyFromActivePairs(b2Body* a_body) {
    if (!a_body) return;
    std::vector<b2Contact*> contacts;
    for (const auto& [contact, keys] : g_physicsDataPairsByContact) {
        (void)keys;
        if (contact->GetFixtureA()->GetBody() == a_body ||
            contact->GetFixtureB()->GetBody() == a_body)
            contacts.push_back(contact);
    }
    std::vector<SeparateContactEvent> separateEvents;
    for (b2Contact* contact : contacts) RemovePairContact(contact, separateEvents);
    DispatchSeparateEvents(separateEvents,
        g_entityPhysicsBodiesBeingDestroyed.count(a_body) == 0);
}

void DispatchContactEvent(gmMachine* a_machine, const DeferredContactEvent& a_event) {
    if (!a_machine) return;
    if (a_event.self.m_type != GM_TABLE || a_event.other.m_type != GM_TABLE) return;
    gmTableObject* table = (gmTableObject*)a_machine->GetObject(a_event.self.m_value.m_ref);
    if (!table || table->Get(a_machine, a_event.method).m_type != GM_FUNCTION) return;
    gmCall call;
    gmObject* selfObject = a_machine->GetObject(a_event.self.m_value.m_ref);
    gmObject* otherObject = a_machine->GetObject(a_event.other.m_value.m_ref);
    // An earlier callback can delete an entity before another callback is
    // reached. Never pass a stale GM reference as the callback argument.
    if (!selfObject || !otherObject) return;
    auto* otherTable = static_cast<gmTableObject*>(otherObject);
    static const bool s_eventDispatchDebug = std::getenv("GE3_CONTACT_EVENT_DEBUG") != nullptr;
    if (s_eventDispatchDebug) {
        gmVariable selfPlayer = table->Get(a_machine, "__isPlayer");
        gmVariable otherPlayer = otherTable->Get(a_machine, "__isPlayer");
        if ((selfPlayer.m_type == GM_INT && selfPlayer.m_value.m_int != 0) ||
            (otherPlayer.m_type == GM_INT && otherPlayer.m_value.m_int != 0)) {
            gmVariable selfId = table->Get(a_machine, "__objectID");
            gmVariable otherId = otherTable->Get(a_machine, "__objectID");
            gmVariable selfType = table->Get(a_machine, "__typeName");
            gmVariable otherType = otherTable->Get(a_machine, "__typeName");
            const char* selfTypeText = selfType.m_type == GM_STRING && selfType.m_value.m_ref
                ? ((gmStringObject*)a_machine->GetObject(selfType.m_value.m_ref))->GetString() : "?";
            const char* otherTypeText = otherType.m_type == GM_STRING && otherType.m_value.m_ref
                ? ((gmStringObject*)a_machine->GetObject(otherType.m_value.m_ref))->GetString() : "?";
            std::fprintf(stderr, "[contact-dispatch] %s %d:%s -> %d:%s\n",
                a_event.method, selfId.m_type == GM_INT ? selfId.m_value.m_int : -1,
                selfTypeText, otherId.m_type == GM_INT ? otherId.m_value.m_int : -1,
                otherTypeText);
        }
    }
    const bool rootSelf = selfObject &&
        !a_machine->IsCPPOwnedGMObject(selfObject);
    const bool rootOther = otherObject && otherObject != selfObject &&
        !a_machine->IsCPPOwnedGMObject(otherObject);
    if (rootSelf) a_machine->AddCPPOwnedGMObject(selfObject);
    if (rootOther) a_machine->AddCPPOwnedGMObject(otherObject);
    if (call.BeginTableFunction(a_machine, a_event.method, table, a_event.self)) {
        call.AddParam(a_event.other);
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

struct ContactPairDispatch {
    EntityPairKey key;
    gmVariable first;
    gmVariable second;
    const char* method;
};

void DispatchContactPair(gmMachine* a_machine, const ContactPairDispatch& a_event) {
    if (!a_machine || !GetEngineContext().gameSystem.shouldUpdateEntities) return;
    // Retail's update-phase dispatcher rechecks active PhysicsData pair-map
    // membership before calling either endpoint. It checks once per pair, so
    // removal in the first callback does not suppress the second callback.
    if (g_activeEntityPairs.find(a_event.key) == g_activeEntityPairs.end()) return;
    auto activePair = g_activeEntityPairs.find(a_event.key);
    if (activePair == g_activeEntityPairs.end() ||
        activePair->second.firstPhysicsData == 0 ||
        g_activePhysicsDataPairs.find({
            activePair->second.firstPhysicsData,
            static_cast<uint32_t>(activePair->second.secondEntityId)}) ==
            g_activePhysicsDataPairs.end())
        return;
    gmObject* firstObject = a_machine->GetObject(a_event.first.m_value.m_ref);
    gmObject* secondObject = a_machine->GetObject(a_event.second.m_value.m_ref);
    if (!firstObject || !secondObject) return;
    const bool rootFirst = !a_machine->IsCPPOwnedGMObject(firstObject);
    const bool rootSecond = secondObject != firstObject &&
        !a_machine->IsCPPOwnedGMObject(secondObject);
    if (rootFirst) a_machine->AddCPPOwnedGMObject(firstObject);
    if (rootSecond) a_machine->AddCPPOwnedGMObject(secondObject);
    DispatchContactEvent(a_machine,
        DeferredContactEvent{a_event.first, a_event.second, a_event.method});
    DispatchContactEvent(a_machine,
        DeferredContactEvent{a_event.second, a_event.first, a_event.method});
    if (rootFirst && a_machine->IsCPPOwnedGMObject(firstObject))
        a_machine->RemoveCPPOwnedGMObject(firstObject);
    if (rootSecond && a_machine->IsCPPOwnedGMObject(secondObject))
        a_machine->RemoveCPPOwnedGMObject(secondObject);
}

class ScriptContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* a_contact) override {
        gmMachine* m = GetEngineContext().machine;
        if (!m) return;
        const gmVariable ownerA = FindOwnerEntityForB2Body(
            a_contact->GetFixtureA()->GetBody());
        const gmVariable ownerB = FindOwnerEntityForB2Body(
            a_contact->GetFixtureB()->GetBody());
        if (ownerA.m_type != GM_TABLE || ownerB.m_type != GM_TABLE) return;
        if (CreatureDebugEnabled()) {
            int idA = -1, idB = -1;
            const bool creatureA = IsCreatureDebugEntity(m, ownerA, idA);
            const bool creatureB = IsCreatureDebugEntity(m, ownerB, idB);
            if (creatureA || creatureB) {
                b2Body* bodyA = a_contact->GetFixtureA()->GetBody();
                b2Body* bodyB = a_contact->GetFixtureB()->GetBody();
                std::fprintf(stderr,
                    "[creature-begin] tick=%u entities=%d/%d shapes=%u/%u "
                    "sensor=%d/%d pos=(%.5f,%.5f)/(%.5f,%.5f) "
                    "velMs=(%.7f,%.7f)/(%.7f,%.7f)\n",
                    GetEngineContext().gameSystem.currentTick, idA, idB,
                    (unsigned)a_contact->GetFixtureA()->GetUserData().pointer,
                    (unsigned)a_contact->GetFixtureB()->GetUserData().pointer,
                    a_contact->GetFixtureA()->IsSensor() ? 1 : 0,
                    a_contact->GetFixtureB()->IsSensor() ? 1 : 0,
                    bodyA->GetPosition().x, bodyA->GetPosition().y,
                    bodyB->GetPosition().x, bodyB->GetPosition().y,
                    bodyA->GetLinearVelocity().x,
                    bodyA->GetLinearVelocity().y,
                    bodyB->GetLinearVelocity().x,
                    bodyB->GetLinearVelocity().y);
            }
        }
        if (EntityCollisionId(m, ownerA) == EntityCollisionId(m, ownerB)) return;

        // Retail's contact-add listener mutates each side in order: add that
        // side's record, invoke its +0x50 callback, then test/add the other
        // side. A callback can therefore affect the second side's eligibility.
        RegisterPairContactSide(m, a_contact, ownerA, ownerB);
        if (IsNativeContactRecordMember(m, ownerA))
            DispatchNow(m, ownerA, ownerB, "onEntityCollidePreSolve");
        RegisterPairContactSide(m, a_contact, ownerB, ownerA);
        if (IsNativeContactRecordMember(m, ownerB))
            DispatchNow(m, ownerB, ownerA, "onEntityCollidePreSolve");
    }
    void EndContact(b2Contact* a_contact) override {
        g_disabledContacts.erase(a_contact);
        std::vector<SeparateContactEvent> separateEvents;
        RemovePairContact(a_contact, separateEvents);
        const bool suppressHandlers = g_entityPhysicsBodiesBeingDestroyed.count(
                a_contact->GetFixtureA()->GetBody()) != 0 ||
            g_entityPhysicsBodiesBeingDestroyed.count(
                a_contact->GetFixtureB()->GetBody()) != 0;
        // Removing a body in a script callback also takes this path before
        // the other endpoint receives its queued collision callback.
        DispatchSeparateEvents(separateEvents, !suppressHandlers);
    }
    void PreSolve(b2Contact* a_contact, const b2Manifold*) override {
        if (gmMachine* m = GetEngineContext().machine) {
            b2Fixture* a = a_contact->GetFixtureA();
            b2Fixture* b = a_contact->GetFixtureB();
            float fa = a->GetFriction();
            float fb = b->GetFriction();
            if (IsPlayerBody(*m, b->GetBody())) fa = GetFixturePlayerFriction(a);
            else if (IsPlayerBody(*m, a->GetBody())) fb = GetFixturePlayerFriction(b);
            a_contact->SetFriction(std::sqrt(fa * fb));
        }
        // Apply the step-scoped restitution mark and the contact-scoped
        // disableContact mark. Box2D honors SetEnabled(false) for one step,
        // so the latter is reapplied until EndContact removes the native
        // contact record. See physics_world.h for the retail evidence.
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
        // GE3_CONTACT_DEBUG=1 records the actual solid/sensor and solver
        // state for player contacts after the retail script callback has
        // had its chance to call disableContact(). This is deliberately
        // diagnostic-only: rolling through an occasional object cannot be
        // attributed to a filter or contact-normal mismatch without seeing
        // which fixture/contact the retail-equivalent pre-solve code saw.
        static const bool s_contactDebug = std::getenv("GE3_CONTACT_DEBUG") != nullptr;
        if (s_contactDebug) {
            gmMachine* m = GetEngineContext().machine;
            b2Body* bodyA = a_contact->GetFixtureA()->GetBody();
            b2Body* bodyB = a_contact->GetFixtureB()->GetBody();
            if (m && (IsPlayerBody(*m, bodyA) || IsPlayerBody(*m, bodyB))) {
                b2WorldManifold manifold{};
                a_contact->GetWorldManifold(&manifold);
                const b2Filter fa = a_contact->GetFixtureA()->GetFilterData();
                const b2Filter fb = a_contact->GetFixtureB()->GetFilterData();
                const gmVariable ownerA = FindOwnerEntityForB2Body(bodyA);
                const gmVariable ownerB = FindOwnerEntityForB2Body(bodyB);
                const int idA = ownerA.m_type == GM_TABLE
                    ? [&]() { auto* t = (gmTableObject*)m->GetObject(ownerA.m_value.m_ref);
                              gmVariable v = t ? t->Get(m, "__objectID") : gmVariable::s_null;
                              return v.m_type == GM_INT ? v.m_value.m_int : -1; }() : -1;
                const int idB = ownerB.m_type == GM_TABLE
                    ? [&]() { auto* t = (gmTableObject*)m->GetObject(ownerB.m_value.m_ref);
                              gmVariable v = t ? t->Get(m, "__objectID") : gmVariable::s_null;
                              return v.m_type == GM_INT ? v.m_value.m_int : -1; }() : -1;
                const bool playerIsA = IsPlayerBody(*m, bodyA);
                b2Body* playerBody = playerIsA ? bodyA : bodyB;
                const gmVariable& playerOwner = playerIsA ? ownerA : ownerB;
                int playerClimbing = -1;
                if (playerOwner.m_type == GM_TABLE) {
                    auto* playerTable = (gmTableObject*)m->GetObject(playerOwner.m_value.m_ref);
                    gmVariable climbing = playerTable
                        ? playerTable->Get(m, "climbing") : gmVariable::s_null;
                    if (climbing.m_type == GM_INT) playerClimbing = climbing.m_value.m_int;
                }
                const uint32_t shapeA = (uint32_t)a_contact->GetFixtureA()->GetUserData().pointer;
                const uint32_t shapeB = (uint32_t)a_contact->GetFixtureB()->GetUserData().pointer;
                const b2Vec2 normalToPlayer = playerIsA
                    ? b2Vec2(-manifold.normal.x, -manifold.normal.y) : manifold.normal;
                const b2Vec2 playerPos = playerBody->GetPosition();
                const b2Vec2 playerVel = playerBody->GetLinearVelocity();
                std::fprintf(stderr,
                    "[contact] ids=%d/%d shapes=%u/%u player=(%.3f,%.3f) "
                    "velMs=(%.4f,%.4f) angle=%.3f climbing=%d "
                    "normalToPlayer=(%.3f,%.3f) catmask=%04x/%04x,%04x/%04x "
                    "sensor=%d/%d enabled=%d disabledByScript=%d\n",
                    idA, idB, shapeA, shapeB, playerPos.x, playerPos.y,
                    playerVel.x, playerVel.y,
                    playerBody->GetAngle(), playerClimbing,
                    normalToPlayer.x, normalToPlayer.y,
                    fa.categoryBits, fa.maskBits, fb.categoryBits, fb.maskBits,
                    a_contact->GetFixtureA()->IsSensor() ? 1 : 0,
                    a_contact->GetFixtureB()->IsSensor() ? 1 : 0,
                    a_contact->IsEnabled() ? 1 : 0,
                    g_disabledContacts.count(a_contact) ? 1 : 0);
            }
        }
    }
    void PostSolve(b2Contact* a_contact,
                   const b2ContactImpulse* a_impulse) override {
        if (!CreatureDebugEnabled() || !a_impulse) return;
        gmMachine* m = GetEngineContext().machine;
        if (!m) return;
        b2Fixture* fixtureA = a_contact->GetFixtureA();
        b2Fixture* fixtureB = a_contact->GetFixtureB();
        b2Body* bodyA = fixtureA->GetBody();
        b2Body* bodyB = fixtureB->GetBody();
        const gmVariable ownerA = FindOwnerEntityForB2Body(bodyA);
        const gmVariable ownerB = FindOwnerEntityForB2Body(bodyB);
        int idA = -1, idB = -1;
        const bool creatureA = IsCreatureDebugEntity(m, ownerA, idA);
        const bool creatureB = IsCreatureDebugEntity(m, ownerB, idB);
        if (!creatureA && !creatureB) return;
        b2WorldManifold manifold{};
        a_contact->GetWorldManifold(&manifold);
        const b2Vec2 velA = bodyA->GetLinearVelocity();
        const b2Vec2 velB = bodyB->GetLinearVelocity();
        const int count = std::min(a_impulse->count, b2_maxManifoldPoints);
        float normalImpulse = 0.0f, tangentImpulse = 0.0f;
        for (int i = 0; i < count; ++i) {
            normalImpulse += a_impulse->normalImpulses[i];
            tangentImpulse += a_impulse->tangentImpulses[i];
        }
        const int ownerTypeA = (int)ownerA.m_type;
        const int ownerTypeB = (int)ownerB.m_type;
        const uintptr_t ownerRefA = ownerA.m_type == GM_TABLE ? ownerA.m_value.m_ref : 0;
        const uintptr_t ownerRefB = ownerB.m_type == GM_TABLE ? ownerB.m_value.m_ref : 0;
        auto ownerLabel = [m](const gmVariable& owner) -> const char* {
            if (owner.m_type != GM_TABLE) return "<null>";
            auto* table = (gmTableObject*)m->GetObject(owner.m_value.m_ref);
            const gmVariable type = table ? table->Get(m, "__typeName") : gmVariable::s_null;
            if (type.m_type != GM_STRING) return "<no-type>";
            auto* name = (gmStringObject*)m->GetObject(type.m_value.m_ref);
            return name ? name->GetString() : "<bad-type>";
        };
        const char* ownerNameA = ownerLabel(ownerA);
        const char* ownerNameB = ownerLabel(ownerB);
        std::fprintf(stderr,
            "[creature-postsolve] tick=%u entities=%d/%d shapes=%u/%u "
            "pd=%p/%p ownerType=%d/%d ownerRef=%llu/%llu ownerName=%s/%s "
            "normal=(%.5f,%.5f) points=%d impulse=(%.7g,%.7g) "
            "velMs=(%.7f,%.7f)/(%.7f,%.7f)\n",
            GetEngineContext().gameSystem.currentTick, idA, idB,
            (unsigned)fixtureA->GetUserData().pointer,
            (unsigned)fixtureB->GetUserData().pointer,
            (void*)bodyA->GetUserData().pointer,
            (void*)bodyB->GetUserData().pointer,
            ownerTypeA, ownerTypeB,
            (unsigned long long)ownerRefA, (unsigned long long)ownerRefB,
            ownerNameA, ownerNameB,
            manifold.normal.x, manifold.normal.y, count,
            normalImpulse, tangentImpulse, velA.x,
            velA.y, velB.x, velB.y);
    }

private:
    void DispatchNow(gmMachine* m, const gmVariable& self, const gmVariable& other,
                     const char* method) {
        if (!m || self.m_type != GM_TABLE || other.m_type != GM_TABLE) return;
        gmTableObject* table = (gmTableObject*)m->GetObject(self.m_value.m_ref);
        if (!table || table->Get(m, method).m_type != GM_FUNCTION) return;
        gmCall call;
        if (!call.BeginTableFunction(m, method, table, self)) return;
        call.AddParam(other);
        call.End();
    }

};

ScriptContactListener g_contactListener;
} // namespace

b2World& GetPhysicsWorld() {
    if (!g_world) {
        g_world = new b2World(b2Vec2(0.0f, 0.0f));
        g_world->SetContactListener(&g_contactListener);
    }
    return *g_world;
}

void SetPhysicsNumFramesToIgnoreCollisions(int a_frames) {
    g_numFramesToIgnoreCollisions = a_frames;
}

int GetPhysicsNumFramesToIgnoreCollisions() {
    return g_numFramesToIgnoreCollisions;
}

void RemovePhysicsContactPairsForEntity(int a_entityId) {
    RemoveEntityFromActivePairs(a_entityId);
}

void RemovePhysicsContactPairsForBody(b2Body* a_body) {
    RemoveBodyFromActivePairs(a_body);
}

bool GetPhysicsContactRecordsForPhysicsData(
    uintptr_t a_myPhysicsData, uint32_t a_otherEntityId,
    std::vector<PhysicsContactRecordView>& a_out) {
    a_out.clear();
    if (a_myPhysicsData == 0) return false;
    auto pairIt = g_activePhysicsDataPairs.find(
        {a_myPhysicsData, a_otherEntityId});
    if (pairIt == g_activePhysicsDataPairs.end()) return false;
    const ActivePhysicsDataPair& pair = pairIt->second;
    a_out.reserve(pair.contactOrder.size());
    for (b2Contact* contact : pair.contactOrder) {
        auto contactIt = pair.contacts.find(contact);
        if (contactIt == pair.contacts.end()) continue;
        a_out.push_back(contactIt->second);
    }
    return true;
}

bool GetPhysicsContactManifoldPointCount(b2Contact* a_contact, int& a_count) {
    if (!a_contact || g_physicsDataPairsByContact.find(a_contact) ==
                           g_physicsDataPairsByContact.end())
        return false;
    a_count = a_contact->GetManifold()->pointCount;
    if (a_count == 0 && a_contact->IsTouching() &&
        (a_contact->GetFixtureA()->IsSensor() ||
         a_contact->GetFixtureB()->IsSensor()) &&
        a_contact->GetFixtureA()->GetShape()->GetType() == b2Shape::e_circle &&
        a_contact->GetFixtureB()->GetShape()->GetType() == b2Shape::e_circle)
        a_count = 1;
    return true;
}

bool SensorCircleContact(b2Contact* a_contact, b2Vec2& a_normal,
                         b2Vec2& a_point) {
    b2Fixture* fixtureA = a_contact->GetFixtureA();
    b2Fixture* fixtureB = a_contact->GetFixtureB();
    if (!a_contact->IsTouching() ||
        !(fixtureA->IsSensor() || fixtureB->IsSensor()) ||
        fixtureA->GetShape()->GetType() != b2Shape::e_circle ||
        fixtureB->GetShape()->GetType() != b2Shape::e_circle)
        return false;
    const auto* circleA = static_cast<const b2CircleShape*>(fixtureA->GetShape());
    const auto* circleB = static_cast<const b2CircleShape*>(fixtureB->GetShape());
    const b2Vec2 centerA = fixtureA->GetBody()->GetWorldPoint(circleA->m_p);
    const b2Vec2 centerB = fixtureB->GetBody()->GetWorldPoint(circleB->m_p);
    a_normal = centerB - centerA;
    if (a_normal.LengthSquared() > 0.0f) a_normal.Normalize();
    else a_normal.Set(1.0f, 0.0f);
    a_point = 0.5f * ((centerA + circleA->m_radius * a_normal) +
                      (centerB - circleB->m_radius * a_normal));
    return true;
}

bool GetPhysicsContactWorldNormal(b2Contact* a_contact, float a_orientationSign,
                                  float& a_x, float& a_y) {
    if (!a_contact || g_physicsDataPairsByContact.find(a_contact) ==
                           g_physicsDataPairsByContact.end())
        return false;
    b2Vec2 sensorNormal, sensorPoint;
    if (SensorCircleContact(a_contact, sensorNormal, sensorPoint)) {
        a_x = sensorNormal.x * a_orientationSign;
        a_y = sensorNormal.y * a_orientationSign;
        return true;
    }
    b2WorldManifold manifold;
    a_contact->GetWorldManifold(&manifold);
    a_x = manifold.normal.x * a_orientationSign;
    a_y = manifold.normal.y * a_orientationSign;
    return true;
}

bool GetPhysicsContactWorldPoint(b2Contact* a_contact, int a_pointIndex,
                                 float& a_x, float& a_y) {
    if (!a_contact || a_pointIndex < 0 || a_pointIndex >= b2_maxManifoldPoints ||
        g_physicsDataPairsByContact.find(a_contact) ==
            g_physicsDataPairsByContact.end())
        return false;
    b2Vec2 sensorNormal, sensorPoint;
    if (a_pointIndex == 0 &&
        SensorCircleContact(a_contact, sensorNormal, sensorPoint)) {
        a_x = sensorPoint.x;
        a_y = sensorPoint.y;
        return true;
    }
    b2WorldManifold manifold;
    a_contact->GetWorldManifold(&manifold);
    const b2Vec2 point = manifold.points[a_pointIndex];
    a_x = point.x;
    a_y = point.y;
    return true;
}

void GetPhysicsCollidingEntitiesForPhysicsData(
    uintptr_t a_myPhysicsData, std::vector<gmVariable>& a_out) {
    a_out.clear();
    if (a_myPhysicsData == 0) return;
    for (const auto& [key, pair] : g_activePhysicsDataPairs) {
        if (key.first == a_myPhysicsData)
            a_out.push_back(pair.otherEntity);
    }
}

bool PhysicsDataHasAnyCollisionPairs(uintptr_t a_myPhysicsData) {
    if (a_myPhysicsData == 0) return false;
    for (const auto& [key, pair] : g_activePhysicsDataPairs) {
        (void)pair;
        if (key.first == a_myPhysicsData) return true;
    }
    return false;
}

bool PhysicsDataPairIsColliding(uintptr_t a_myPhysicsData,
                                uint32_t a_otherEntityId) {
    if (a_myPhysicsData == 0) return false;
    return g_activePhysicsDataPairs.find(
        {a_myPhysicsData, a_otherEntityId}) != g_activePhysicsDataPairs.end();
}

void StepPhysicsWorld(float a_dt) {
    // Retail physics uses milliseconds for both the step and body velocity.
    static const bool s_playerFrameDebug =
        std::getenv("GE3_CONTACT_FRAME_DEBUG") != nullptr;
    gmMachine* machine = GetEngineContext().machine;
    if (s_playerFrameDebug && machine) {
        for (b2Body* body = GetPhysicsWorld().GetBodyList(); body;
             body = body->GetNext()) {
            if (!IsPlayerBody(*machine, body)) continue;
            const b2Vec2 pos = body->GetPosition();
            const b2Vec2 vel = body->GetLinearVelocity();
            const gmVariable owner = FindOwnerEntityForB2Body(body);
            int objectId = -1;
            if (owner.m_type == GM_TABLE) {
                auto* table = (gmTableObject*)machine->GetObject(owner.m_value.m_ref);
                const gmVariable id = table
                    ? table->Get(machine, "__objectID") : gmVariable::s_null;
                if (id.m_type == GM_INT) objectId = id.m_value.m_int;
            }
            for (b2Fixture* fixture = body->GetFixtureList(); fixture;
                 fixture = fixture->GetNext()) {
                const b2Filter filter = fixture->GetFilterData();
                std::fprintf(stderr,
                    "[physics-frame] dtMs=%.3f id=%d pos=(%.6f,%.6f) "
                    "velMs=(%.6f,%.6f) angle=%.6f bullet=%d awake=%d "
                    "shape=%u type=%d filter=%04x/%04x group=%d sensor=%d\n",
                    a_dt, objectId, pos.x, pos.y, vel.x,
                    vel.y, body->GetAngle(), body->IsBullet() ? 1 : 0,
                    body->IsAwake() ? 1 : 0,
                    (unsigned)fixture->GetUserData().pointer,
                    (int)fixture->GetShape()->GetType(), filter.categoryBits,
                    filter.maskBits, filter.groupIndex,
                    fixture->IsSensor() ? 1 : 0);
            }
        }
    }
    b2World& world = GetPhysicsWorld();
    const float stepMilliseconds = a_dt;
    std::vector<b2Body*> sweptBodies;
    for (b2Body* body = world.GetBodyList(); body; body = body->GetNext()) {
        if (body->GetType() != b2_dynamicBody || body->IsBullet()) continue;
        const b2Vec2 travel = stepMilliseconds * body->GetLinearVelocity();
        constexpr float kStockTranslationLimit = 2.0f * b2_lengthUnitsPerMeter;
        if (travel.LengthSquared() > kStockTranslationLimit * kStockTranslationLimit) {
            // Retail resolves these crossings even when isBullet() is false.
            // Use Box2D's sweep for this step, then restore the script flag.
            body->SetBullet(true);
            sweptBodies.push_back(body);
        }
    }
    world.Step(stepMilliseconds, 2, 2);
    for (b2Body* body : sweptBodies) body->SetBullet(false);
    if (CreatureDebugEnabled() && machine) {
        for (b2Body* body = GetPhysicsWorld().GetBodyList(); body;
             body = body->GetNext()) {
            int entityId = -1;
            if (!IsCreatureDebugEntity(machine,
                    FindOwnerEntityForB2Body(body), entityId)) continue;
            const b2Vec2 pos = body->GetPosition();
            const b2Vec2 vel = body->GetLinearVelocity();
            int fixtureCount = 0;
            for (b2Fixture* fixture = body->GetFixtureList(); fixture;
                 fixture = fixture->GetNext()) ++fixtureCount;
            std::fprintf(stderr,
                "[creature-world] tick=%u dtMs=%.6f entity=%d pos=(%.5f,%.5f) "
                "velMs=(%.7f,%.7f) angle=%.6f omegaMs=%.7f fixtures=%d "
                "mass=%.6g awake=%d\n",
                GetEngineContext().gameSystem.currentTick, a_dt, entityId,
                pos.x, pos.y, vel.x, vel.y,
                body->GetAngle(), body->GetAngularVelocity(),
                fixtureCount, body->GetMass(), body->IsAwake() ? 1 : 0);
        }
    }

    std::vector<ContactPairDispatch> pairEvents;
    if (g_numFramesToIgnoreCollisions > 0) {
        --g_numFramesToIgnoreCollisions;
    } else if (GetEngineContext().gameSystem.shouldUpdateEntities) {
        const int tick = GetEngineContext().gameSystem.currentTick;
        for (const auto& [key, pair] : g_activeEntityPairs) {
            const auto nativePair = g_activePhysicsDataPairs.find({
                pair.firstPhysicsData,
                static_cast<uint32_t>(pair.secondEntityId)});
            if (nativePair == g_activePhysicsDataPairs.end()) continue;
            bool isManagerMember = false;
            for (const auto& [contact, membership] : pair.contacts) {
                (void)contact;
                if ((membership.firstSide &&
                     IsEntityEnabledForPhysicsManager(machine, pair.firstEntity)) ||
                    (membership.secondSide &&
                     IsEntityEnabledForPhysicsManager(machine, pair.secondEntity))) {
                    isManagerMember = true;
                    break;
                }
            }
            if (!isManagerMember) continue;
            pairEvents.push_back({key, pair.firstEntity, pair.secondEntity,
                nativePair->second.createdTick == tick
                    ? "onEntityCollide" : "onEntityTouch"});
        }
    }

    // Retail's queue stores unique unordered Entity-pointer pairs and
    // dispatches one event kind to both endpoints. The local pointer key now
    // matches pair identity; traversal order and manager membership remain
    // local adaptations pending matched runtime comparison.
    for (const ContactPairDispatch& event : pairEvents)
        DispatchContactPair(machine, event);

    g_restitutionDisabledBodies.clear();
    FlushDeferredPhysicsBodyDestructions();
}

void DisableRestitutionThisFrameForBody(b2Body* a_body) {
    if (a_body) g_restitutionDisabledBodies.insert(a_body);
}

void MarkContactDisabled(b2Contact* a_contact) {
    if (!a_contact || g_physicsDataPairsByContact.find(a_contact) ==
                          g_physicsDataPairsByContact.end())
        return;
    // Retail disableContact marks the native contact object immediately. Its
    // bit remains part of that contact until the native EndContact path erases
    // the record. Box2D resets SetEnabled(false) each step, so preserve this
    // contact-scoped mark and reapply it in PreSolve until EndContact.
    a_contact->SetEnabled(false);
    g_disabledContacts.insert(a_contact);
}

void MarkEntityPhysicsBodyForTeardown(b2Body* a_body) {
    if (a_body) g_entityPhysicsBodiesBeingDestroyed.insert(a_body);
}

void ClearEntityPhysicsBodyTeardown(b2Body* a_body) {
    if (a_body) g_entityPhysicsBodiesBeingDestroyed.erase(a_body);
}
