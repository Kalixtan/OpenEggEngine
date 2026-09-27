#pragma once
#include "gmMachine.h"

// decompiled methods), used 7 times across the corpus. Found while
// chasing why `Platformer/Main.gm`'s tutorial-level setup never created
// `this.player`: `this.saveEnv=ENVELOPE();` was completely unregistered,
// so the constructor call itself silently no-op'd (null-call leniency)
// and `this.saveEnv` stayed null, throwing on the very next line
// (`this.saveEnv.appendPoint(0,1);`) and killing the whole `initForReal()`
// function before it ever reached `this.player=EntityManager.createEntityAsPlayer(...)`.
//
// Modelled as a simple time/value point list plus a playback-position
// timer (`setTime`/`getCurrentTime`, confirmed getter/setter pair;
// `getMaxTime` = the largest time among added points) -- `update`/
// `isDone`/`reset` (confirmed real by usage only, no decompiled bodies,
// same evidence gap as PulsePerc's identically-named methods) read as a
// periodic-timer idiom matching real usage exactly: `game_data/Platformer/Main.gm`
// appends points, calls `setTime(9999999)` (jump past the end, so
// `isDone()` starts true), then on a cadence calls `reset()` (currentTime
// back to 0) and `update(msElapsed)` (advance it) until `isDone()`
// (currentTime >= getMaxTime()) fires again -- a straightforward
// periodic auto-save trigger.
void RegisterEnvelopeBindings(gmMachine& a_machine);
