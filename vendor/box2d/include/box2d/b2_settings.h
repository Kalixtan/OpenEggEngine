// MIT License

// Copyright (c) 2019 Erin Catto

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef B2_SETTINGS_H
#define B2_SETTINGS_H

#include "b2_types.h"
#include "b2_api.h"

/// @file
/// Settings that can be overriden for your application
///

/// Define this macro in your build if you want to override settings
#ifdef B2_USER_SETTINGS

/// This is a user file that includes custom definitions of the macros, structs, and functions
/// defined below.
#include "b2_user_settings.h"

#else

#include <stdarg.h>
#include <stdint.h>

// Tunable Constants

/// You can use this to change the length scale used by your game.
/// For example for inches you could use 39.4.
///
/// ENGINE PATCH: this game's own world-space objects are tiny relative to
/// Box2D's meters-kilograms-seconds tuning assumption -- the player's own
/// radius is ~0.49 world-units (confirmed: `this.radius=(SW*2-8*
/// BACK_CONVERSION)/2-8*BACK_CONVERSION`, SW=0.61 confirmed live against
/// real RPCS3), but the real per-frame velocities real scripts command
/// during "stuck to a surface" movement (Player.gm's own
/// `maxHangSpeed=(SW/(60.0-9.0))*0.7` ~= 0.0084 units/sec) are smaller
/// than Box2D's DEFAULT b2_maxLinearCorrection (0.2 units -- ~40% of the
/// player's own radius corrected in a single solver step) and sit right at
/// b2_linearSleepTolerance (0.01) -- i.e. Box2D's default absolute
/// collision-tolerance constants are disproportionately large relative to
/// both this game's object sizes AND its real, confirmed script-commanded
/// velocities, observed in practice as a player body getting stuck
/// (commanded velocity effectively cancelled by position correction) when
/// resting across a seam between two adjacent static ground pieces.
/// Scaling this down brings b2_maxLinearCorrection/b2_linearSlop/
/// b2_linearSleepTolerance/b2_aabbExtension/b2_maxTranslation (all defined
/// in terms of this constant, b2_common.h) proportionally back in line
/// with this game's actual object/velocity scale. Not decompiled-confirmed
/// (retail's real internal Box2D tuning, if it differs from stock at all,
/// isn't recoverable from the script corpus) -- a reasoned engine-side fix
/// for a directly observed solver behavior, not a guess pulled from
/// nowhere. NEVER set this to 0 -- tried during investigation, produced
/// NaN velocities within ~8 frames (something divides by a term derived
/// from this scale without guarding zero) and pegged the host CPU.
///
/// ENGINE PATCH (player-movement-speed session): 0.2f was correct for the
/// SOUND SHAPES world scale the comment above describes (~0.49-unit player
/// radius, ~22-unit tall world) -- but this FoE fork's world is roughly
/// three orders of magnitude larger: GameBoard is 8000 units tall
/// (kGameBoardHalfHeight=4000, foe_compat_bindings.cpp) and Player.gm's own
/// body is setAsCircle(49). At 0.2f, b2_maxTranslation (=2*this, b2_common.h)
/// was 0.4 units per step, which b2_island.cpp:289/487 enforces as a HARD
/// per-step position-delta clamp -- capping EVERY body at 0.4*60 = 24
/// units/sec regardless of the velocity actually set on it. Crossing FoE's
/// 8000-unit board at that cap takes ~5.5 minutes, and no amount of
/// maxSpeed/thrust/decay tuning upstream could ever exceed it -- the
/// clamp silently discarded the excess every step. This was the real cause
/// of "player movement way too slow"; two earlier attempts at the problem
/// (rescaling kScriptVelocityToSimScale, and reinterpreting the decay-rate
/// constant) changed nothing precisely because this clamp sat downstream of
/// both. Rescaled by the same ~364x factor the world itself grew by
/// (8000-unit board vs Sound Shapes' ~22), keeping every derived tolerance
/// (b2_linearSlop/b2_maxLinearCorrection/b2_linearSleepTolerance/
/// b2_aabbExtension) in the same proportion to object size that the
/// original 0.2f established -- so the anti-stuck fix above is preserved,
/// just expressed at this game's actual scale. Resulting cap:
/// 2*72.8*60 = 8736 units/sec, comfortably above Player.gm's real
/// commanded speed (maxSpeed 0.972 -> 972 units/sec, see
/// kScriptVelocityToSimScale) instead of ~40x below it.
#define b2_lengthUnitsPerMeter 72.8f

/// The maximum number of vertices on a convex polygon. You cannot increase
/// this too much because b2BlockAllocator has a maximum object size.
#define b2_maxPolygonVertices	8

// User data

/// You can define this to inject whatever data you want in b2Body
struct B2_API b2BodyUserData
{
	b2BodyUserData()
	{
		pointer = 0;
	}

	/// For legacy compatibility
	uintptr_t pointer;
};

/// You can define this to inject whatever data you want in b2Fixture
struct B2_API b2FixtureUserData
{
	b2FixtureUserData()
	{
		pointer = 0;
	}

	/// For legacy compatibility
	uintptr_t pointer;
};

/// You can define this to inject whatever data you want in b2Joint
struct B2_API b2JointUserData
{
	b2JointUserData()
	{
		pointer = 0;
	}

	/// For legacy compatibility
	uintptr_t pointer;
};

// Memory Allocation

/// Default allocation functions
B2_API void* b2Alloc_Default(int32 size);
B2_API void b2Free_Default(void* mem);

/// Implement this function to use your own memory allocator.
inline void* b2Alloc(int32 size)
{
	return b2Alloc_Default(size);
}

/// If you implement b2Alloc, you should also implement this function.
inline void b2Free(void* mem)
{
	b2Free_Default(mem);
}

/// Default logging function
B2_API void b2Log_Default(const char* string, va_list args);

/// Implement this to use your own logging.
inline void b2Log(const char* string, ...)
{
	va_list args;
	va_start(args, string);
	b2Log_Default(string, args);
	va_end(args);
}

#endif // B2_USER_SETTINGS

#include "b2_common.h"

#endif
