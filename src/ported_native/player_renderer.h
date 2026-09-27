#pragma once

// Draws the player character every frame, for real -- retail's own player
// draw path is native C++ (Entity.forceDraw @ 0x0009b828 dispatches
// through a per-entity vtable slot instead of calling a script draw()
// hook the way every other entity does), so there's no live GM script
// equivalent to lean on the way most of this engine's other bindings can.
//
// See player_renderer.cpp's header comment for the evidence this is
// actually built from -- NOT reverse-engineered from the native vtable
// target directly (that address wasn't recoverable via headless Ghidra
// analysis alone), but from Platformer/PlayerOLD.gm's still-present
// `testDraw()` function: a real, functioning procedural-shape draw
// routine for this exact entity (same radius/maxSpeedX/lastJumpNormal
// fields the CURRENT live Player.gm still uses), left in game_data the
// same way GroundProcessNEW.gm's dead reference code was.
//
// This is a reimplementation of GAME logic that retail itself ported out
// of GameMonkeyScript into native C++ for performance -- not core engine/
// rendering infrastructure -- so it lives here in ported_native/, not
// src/render/ or src/bindings/, per this project's convention of keeping
// that distinction visible in the file layout.
void DrawPlayerNative();
