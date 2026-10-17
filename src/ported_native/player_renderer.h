#pragma once
#include "../render/draw_list.h"

// Draws the player character every frame, for real -- retail's own player
// draw path is native C++ (Entity.forceDraw @ 0x0009b828 dispatches
// through a per-entity vtable slot instead of calling a script draw()
// hook the way every other entity does), so there's no live GM script
// equivalent to lean on the way most of this engine's other bindings can.
//
// GameMonkey is only consulted for the state values passed to retail's native
// renderer. Renderer behavior is traced from the retail PPC draw entry
// FUN_001a1660 and particle pass FUN_001a0be8. The body ring, aura, eye/run
// meshes, and particle pass use recovered retail formulas. Vector-angle
// conversion now uses PPC-derived FUN_001dbcb8/FUN_001dcb70. Geometry is
// captured at the player's native traversal point as ordered desktop draw
// commands, then replayed at its sorted list position; the live particle
// shape is retained through replay. Aura, body, eye, and run passes now use
// persistent player-owned VMSHAPE objects and deferred transformed shape
// submissions. Native buffer allocation/capacity details and matched retail
// buffer/pixel comparison remain open.
//
// Verification: retail PPC entry FUN_001a1660 and particle pass
// FUN_001a0be8 are decompiled/traced in SOUND_SHAPES_GROUND_TRUTH.md and the
// ported, and per-pass transforms use the traced command ordering. Native
// VMSHAPE lifecycle parity and matched output comparison remain open.
// This is game-specific behavior, not generic OpenGL infrastructure.
void DrawPlayerNative();

// PPC-derived FUN_001dbcb8 vector angle used by the retail player renderer
// and Entity.getRadiansToFace. Returns the same binary32 approximation as
// the retail helper rather than delegating to platform libm.
float RetailAtan2(float a_y, float a_x);

// Capture the native player pass as a per-frame DrawList submission. Retail
// resets the player-owned list to order key 0x200000ff before recording its
// particle/shape commands; the desktop callback draws those commands when the
// ordered entity lists replay, inheriting live render state at that point as
// retail opcode 0x1a snapshots the active tuple during replay. Geometry is
// captured during traversal and replayed as desktop commands; aura, body,
// eye, and run geometry updates persistent VMSHAPE objects and uses the
// shared deferred shape path. Exact native ring-buffer allocation and
// per-submit buffer-version behavior remain open.
DrawListSubmission CaptureNativePlayerDrawSubmission();

// Native callbacks FUN_001a30c4 / FUN_001a3124 set the transition state and
// FUN_001a0b60 advances it after the base Player update callback. The enabled
// entity dispatcher calls this immediately after the player's script update.
void TickPlayerNativeRenderer(float a_deltaMs);
