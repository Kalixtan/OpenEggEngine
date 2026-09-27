#pragma once

// Draws every live ground-category Box2D body for real, every frame.
//
// This is NOT core engine/render infrastructure -- it's a reimplementation
// of GAME logic (Platformer/GroundProcessNEW.gm) that retail itself moved
// out of GameMonkeyScript and into native C++ for performance. Confirmed by
// the shipped script itself: `Platformer_GroundProcessNEW` (the process
// actually registered and run) is a near-empty stub, while game_data still
// contains a complete, never-called sibling function named
// `PORTED_TO_CPP______Platformer_GroundProcessNEW` with the FULL original
// pre-port drawing algorithm still intact -- left in the file, unreachable,
// as Queasy Games' own historical reference for what the native code
// replicates. The real native function itself isn't reachable the way
// every other native binding this project has ported so far was (it was
// never exposed to script at all, so it has no gmFunctionEntry name to
// search for, and a blind byte-level search of a stripped PS3 binary for
// an anonymous internal function isn't tractable) -- so this is a careful,
// direct 1:1 port of that left-in GM reference to C++ instead, which is
// the best real evidence available for retail's actual behavior. See
// ground_renderer.cpp for the per-style breakdown and what's simplified.
//
// Anything else that turns out to be a similar "ported to native in
// retail" case (e.g. per-entity sprite rendering) belongs in this same
// folder, for the same reason: it's a real behavioral port, not engine
// infrastructure, and keeping it separate from src/core, src/render, and
// src/bindings keeps that distinction visible.
void DrawGroundsNative();
