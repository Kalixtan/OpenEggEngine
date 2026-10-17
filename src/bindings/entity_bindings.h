#pragma once
#include "gmMachine.h"
#include <string>
#include <vector>

class gmThread;

void RegisterEntityBindings(gmMachine& a_machine);

// EntityManager.getEntity(objectID) lookup used by native renderer passes.
// The returned pointer is a borrowed view into the live entity registry.
gmVariable* FindEntityByObjectID(int a_objectId);

// Use the same ENTITYLISTREFCONTAINER type for EntityManager queries and
// PhysicsData.getCollidingEntities. With a_reuse, refill and return the
// caller's container; otherwise allocate a fresh iterator.
bool PushOrRefillEntityListIterator(gmThread* a_thread,
    std::vector<gmVariable> a_entities, const gmVariable* a_reuse = nullptr);

// Completes the EntityManager update-event boundary: activate queued
// creations and process deferred removals after script callbacks have ended.
void FlushEntityManagerFrameBoundary();

// Entity.deleteAllEntities request issued by the retail
// ParanoidCleanUpProcess constructor when cleanup-enabled process-list
// transitions are created. The request is consumed at the manager's normal
// frame-boundary deletion service.
void RequestEntityManagerDeleteAll();

int GetEntityManagerEntityCount();

// Debug-draw support: one row per live entity in the native EntityManager
// registry (entity_bindings.cpp's anonymous-namespace EntityRegistry --
// NOT the same thing as a Box2D body; plenty of real entities, e.g. Note,
// BG_Mob, spawnpoints, never touch physics at all). Read-only snapshot for
// draw_list.cpp's DrawAllPhysicsBodies to also plot every entity's
// configureData position, independent of whether it has a body.
struct EntityDebugInfo {
    int uid;
    std::string typeName;
    float x, y;
    bool isPlayer;
    bool hasPos; // false if configureData/posX/posY were never set (e.g. UI-only processes, not level entities) -- not meaningful to plot at a fabricated (0,0)
};
std::vector<EntityDebugInfo> GetAllEntityDebugPositions();

std::vector<int> DrawEntitiesWithNativeDrawHook();

std::vector<int> DrawEntitiesWithNativeDrawAlphaHook();

// Calls entity.preDraw() and replays its drawList before normal entity draw.
// Player's retail-native draw caches and invokes this callback separately.
void PreDrawEntitiesWithNativeHook();

// Calls entity.update(a_msElapsed) on every registered entity every frame --
// see this function's own header comment in entity_bindings.cpp for why
// this exists (nothing else in the engine drives entity-side update logic).
void UpdateEntitiesWithNativeUpdateHook(float a_msElapsed);

void PostUpdateEntitiesWithNativeHook(float a_msElapsed);

void ConfigureUnconfiguredEntities();

void DispatchKeyEventToEntities(int a_keycode, bool a_isPress);

// First live entity with __isPlayer set (createEntityAsPlayer's marker --
// see gmf_EM_getRandomPlayer's identical scan). For ported_native/
// player_renderer.cpp, which needs to read the player's own script-side
// animation-state fields (radius, squintPerc, lastEyeDirDrawn, ...)
// directly off its table rather than reimplementing the state machine
// that computes them. Returns GM_NULL if no player entity exists yet
// (before level load finishes, or between levels).
gmVariable FindPlayerEntity(gmMachine& a_machine);
