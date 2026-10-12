#pragma once
#include "gmMachine.h"
#include <string>
#include <vector>

class gmThread;

// EntityBase.c -- 81+ functions between them, a whole subsystem far
// outside this pass's scope) -- just enough to unblock
// Platformer_Mixer_Init's boot-time data load (ENTITY_GetType,
// ENTITYBASE_ClearMixer, ENTITYBASE_SetMixerData, MIXER_AUTO_ID_START).
// Not a general Entity implementation.
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

// FUN_000c3d0c: committed entities plus entities awaiting promotion from the
// pending-create array. Used by the native cleanup controller's state 0.
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

// Calls entity.draw() -- the real, script-authored per-entity-type render
// hook (EntityPacks/*.gm's `t.draw=function(){ local drawList=this.
// drawList; drawList.reset(); ... drawList.drawTextureByHandle(...); }`
// pattern, confirmed real via corpus usage, e.g. BaseNotePlant.gm) -- for
// every registered entity that defines one. `this.drawList` is a real
// per-entity DrawList created via the confirmed-real `Video.
// createDrawList(...)`, and every draw call on it goes through the same
// immediate-mode native bindings as the main scene's drawList, so this
// "just works" once invoked at the right point in the frame (camera-view
// mode active, matching where drawStaticSurface() itself is called from
// real preDraw()). Returns the uids actually drawn this way so the
// debug-diamond marker fallback (draw_list.cpp's DrawEntityMarkers) can
// skip them instead of drawing both.
std::vector<int> DrawEntitiesWithNativeDrawHook();

// Calls entity.drawAlpha() on every registered entity that defines one --
// the alpha/transparent-pass counterpart to DrawEntitiesWithNativeDrawHook
// above, run as a SEPARATE second pass after every entity's normal draw()
// has already run. Confirmed real and structurally distinct from draw()
// (Ghidra, FoE PC binary): the string "drawAlpha" (0x00513d9c) exists
// standalone in the binary, referenced by its own small wrapper functions
// (FUN_0042cfe0/FUN_0044c960) with the exact same "does this entity
// override this method; if not, fall back" shape as the confirmed "draw"
// wrapper (FUN_0042cfc0/FUN_0044c940) -- but drawAlpha's own fallback
// (FUN_004066f0) is a bare `return;`, unlike draw()'s fallback
// (FUN_00418db0), which draws a real default rect/ellipse shape. This
// engine's PekkoAndJon corpus splits ALL of its actual visuals between the
// two: Player.gm/Thing.gm's own `draw()` is empty and every real pixel is
// drawn from `drawAlpha()`; Point.gm is the mirror image. Without this
// hook, the player and every "Thing" enemy/cannon never rendered at all,
// no matter how correct their physics/entity-creation was -- their entire
// visual output lived in a method nothing ever called.
std::vector<int> DrawEntitiesWithNativeDrawAlphaHook();

// Calls entity.preDraw() and replays its drawList before normal entity draw.
// Player's retail-native draw caches and invokes this callback separately.
void PreDrawEntitiesWithNativeHook();

// Calls entity.update(a_msElapsed) on every registered entity every frame --
// see this function's own header comment in entity_bindings.cpp for why
// this exists (nothing else in the engine drives entity-side update logic).
void UpdateEntitiesWithNativeUpdateHook(float a_msElapsed);

// Calls entity.postUpdate(a_msElapsed) after the update event has completed.
// Player.gm uses this distinct phase to commit state-machine velocity and
// sensor changes before Platformer/Main.gm's process postUpdate moves camera.
void PostUpdateEntitiesWithNativeHook(float a_msElapsed);

// Force-configures any entity that has configureData but whose configure()
// was never invoked -- see the long header comment on the definition
// (entity_bindings.cpp) for why Main.gm's level-load loop leaves most placed
// entities in that state and why retail's lazy "configure once it scrolls on
// screen" system, which finishes the job there, has no equivalent here.
//
// The definition was written to run once per frame and then never called
// from anywhere, so nothing drove it at all: every entity kept the fields
// its configure() would have assigned as null. That surfaced as a steady
// drip of null-field script errors from draw(), each one aborting that
// entity's ENTIRE draw call -- `-this.width` (BlockCannon), `this.dirSign`
// (GoombaCritterGuitar), `this.fillcn` (BrianMover).
void ConfigureUnconfiguredEntities();

// Calls entity.onKeyPress(kc)/onKeyRelease(kc) on every registered entity
// that defines one -- the key-event counterpart to
// UpdateEntitiesWithNativeUpdateHook above. Same gap, same reason: nothing
// else in this engine ever called it. ProcessManager::DispatchKeyEventNamed
// (process_manager.cpp) only broadcasts to Processes, never to Entities()
// -- and Player is a registered Entity, not a Process, so its own real
// onKeyPress (Player.gm: `onKeyPress=function(kc){ ...
// this.state.call("onKeyPress",kc,this); }`, which is what HANG_STATEID's
// onKeyPress uses to trigger doJump()/jumpAction()) was never being called
// at all -- jump silently never fired, with no exception/getdot to signal
// it, because the call site itself was simply never reached.
void DispatchKeyEventToEntities(int a_keycode, bool a_isPress);

// First live entity with __isPlayer set (createEntityAsPlayer's marker --
// see gmf_EM_getRandomPlayer's identical scan). For ported_native/
// player_renderer.cpp, which needs to read the player's own script-side
// animation-state fields (radius, squintPerc, lastEyeDirDrawn, ...)
// directly off its table rather than reimplementing the state machine
// that computes them. Returns GM_NULL if no player entity exists yet
// (before level load finishes, or between levels).
gmVariable FindPlayerEntity(gmMachine& a_machine);
