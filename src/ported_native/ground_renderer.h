#pragma once

// Verification: retail EntityRegistration.drawGrounds is bytecode-verified
// at FUN_001d0520 and calls native FUN_001ce4a0. Ground registration,
// camera-cell range, palette names, and draw-list setup have corresponding
// against the shipped GroundProcessNEW.gm and VideoHelper.gm reference code.
// This port is not yet parity-complete: exact native draw-list ordering,
// stencil lifetime, and fire animation/update behavior remain under audit.

struct DrawListData;

// Called by EntityRegistration.drawGrounds while Main.gm is recording its
// preDraw list. Appends native ground rendering at retail's list position.
void RecordGroundsNative(DrawListData* a_drawList);
