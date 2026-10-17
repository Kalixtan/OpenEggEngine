#pragma once
#include "../render/draw_list.h"

void DrawPlayerNative();

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

void TickPlayerNativeRenderer(float a_deltaMs);
