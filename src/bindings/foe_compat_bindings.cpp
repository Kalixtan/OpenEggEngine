#include "gmThread.h"
#include "foe_compat_bindings.h"
#include "gmTableObject.h"
#include "gmCall.h"
#include "../core/engine_context.h"
#include "vector2d_bindings.h"
#include <cstdlib>
#include <cmath>
#include <vector>

// Flowers of Error compatibility bindings for functions present in the PC
// build but not directly in the Sound Shapes engine. Ghidra decompilation of
// the FoE win32 executable (flowers-of-error_win32_may18_08.exe) combined
// with porting of the Sound Shapes engine's existing equivalent implementations.

namespace {
float Random01() {
    return (float)rand() / (float)RAND_MAX;
}
} // namespace

// RANDOM_GetRandomReal(void) or RANDOM_GetRandomReal(float lo, float hi)
// Decompiled from FoE @ 0x00401910 (via Ghidra on flowers of error.exe)
// Sound Shapes equivalent: GameSystem.getRandomReal() in gamesystem_bindings.cpp
int GM_CDECL gmf_RANDOM_GetRandomReal(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n >= 2) {
        // Two-parameter form: (lo, hi)
        GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
        a_thread->PushFloat(lo + (hi - lo) * Random01());
    } else if (n >= 1) {
        // One-parameter form: (hi) -- lo=0
        GM_FLOAT_OR_INT_PARAM(hi, 0, 1.0f);
        a_thread->PushFloat(hi * Random01());
    } else {
        // Zero-parameter form: random [0, 1)
        a_thread->PushFloat(Random01());
    }
    return GM_OK;
}

// RANDOM_GetRandomInteger(lo, hi) -- confirmed real (Ghidra: retail
// FUN_004414e0 @ 0x004414e0). Was completely unbound despite 12 corpus call
// sites (Main.gm enemy-type/spawn-chance rolls, Thing.gm split-count/wall-
// type rolls) -- every one of those silently null-called to null, which
// then flowed into things like `PekkoAndJon_Thing_SpawnWall(null)` (array
// index by null -- leniently treated as index 0, so every "random" wall
// spawn was always the SAME type). Inclusive [lo,hi] range (both endpoints
// reachable) -- confirmed by the real call sites' own shapes, e.g.
// `RANDOM_GetRandomInteger(1,2)` (Thing.gm:526) is only meaningful as a
// binary choice if BOTH 1 and 2 are reachable.
int GM_CDECL gmf_RANDOM_GetRandomInteger(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 1, 0.0f);
    int ilo = (int)lo, ihi = (int)hi;
    if (ihi < ilo) std::swap(ilo, ihi);
    int range = ihi - ilo + 1;
    a_thread->PushInt(ilo + (range > 0 ? (std::rand() % range) : 0));
    return GM_OK;
}
// RANDOM_GetRandomBoolean() -- confirmed real (Ghidra: retail FUN_004415f0
// @ 0x004415f0), no corpus call sites yet but added alongside its two
// RANDOM_ siblings for full parity (this file's whole point is 1:1
// coverage of the RANDOM_ family, not just what's used today).
int GM_CDECL gmf_RANDOM_GetRandomBoolean(gmThread* a_thread) {
    a_thread->PushInt((std::rand() % 2) ? 1 : 0);
    return GM_OK;
}

// REALPOINT type -- SUPERSEDED, see GitHub issue #9.
//
// This used to be a working (x,y)-constructing global, ported unmodified
// from Flowers-of-Error's own, different binary (the decompiled address
// below, 0x0042d1f0, is FoE's x86 executable, not Sound Shapes' PS3 one --
// same class of carried-over mistake as the old SetDepthTest).
//
// REALPOINT_MakeRandomDir(), and getRef() all unconditionally
// `return 0xffffffffffffffff` (the GM exception sentinel) with zero
// argument-count branching -- unlike every other native constructor in the
// corpus, which at least dispatches on arg count before failing. RealPoint
// values genuinely exist at runtime (Vector2D's own constructor has a
// branch that reads one by offset), they just can't be constructed or
// queried directly from a script; retail keeps RealPoint an internal,
// opaque C++ value passed between other natives. Zero real `RealPoint(`
// call sites anywhere in game_data/, so this has no observable gameplay
// impact -- fixed for correctness, not because anything was broken in
// practice.
int GM_CDECL gmf_REALPOINT(gmThread* a_thread) {
    (void)a_thread;
    return GM_EXCEPTION;
}

// getUniqueAlarmID() -- allocate and return a unique alarm ID
// Decompiled from FoE @ 0x0043ced0 (via Ghidra on flowers of error.exe)
static int g_alarmIDCounter = 0;
int GM_CDECL gmf_getUniqueAlarmID(gmThread* a_thread) {
    a_thread->PushInt(g_alarmIDCounter++);
    return GM_OK;
}

// getUniqueLFOID() -- similar to getUniqueAlarmID
// Not found in FoE decompilation (may be FoE-specific or unregistered)
static int g_lfoIDCounter = 0;
int GM_CDECL gmf_getUniqueLFOID(gmThread* a_thread) {
    a_thread->PushInt(g_lfoIDCounter++);
    return GM_OK;
}

// ---------------------------------------------------------------------
// ALARM -- confirmed real native type (Ghidra: retail ALARM ctor @
// 0x0042e830, registered right alongside setInterval/hardReset/reset/
// isAlarming/etc in the same contiguous table as everything else in this
// file, see reveng_pc/VMSHAPE_VIDEO_VERIFICATION.md). Was a total stub
// before this pass (addAlarm/getAlarm both unconditionally returned null),
// so EVERY alarm-driven mechanic in PekkoAndJon never fired at all:
// Main.gm's SPAWNWALL_ALARMID (enemy spawning), TYPETEXT_ALARMID (the
// score-text typewriter timing), and Thing.gm's JIGIT_ALARMID (per-enemy
// wiggle timing) -- i.e. without this, enemies never spawn and the game is
// unplayable past the title screen.
//
// Real usage pattern confirmed across every call site (Main.gm/Thing.gm):
// `owner.addAlarm(id, intervalMs, repeat)` registers a timer against
// `owner` (an Entity or Process handle); `owner.onAlarm=function(alarmId)
// {...}` is a plain script method the SAME owner table defines; when the
// timer elapses, retail calls `owner.onAlarm(alarmId)`. `owner.getAlarm(id)`
// returns a live handle to that same timer's state so the callback can
// retune it in place (`local0=owner.getAlarm(arg0); local0.setInterval(
// ...); local0.hardReset();`, Main.gm:326/339-340).
// ---------------------------------------------------------------------

struct AlarmState {
    gmVariable owner;      // the table addAlarm was called on (Entity or Process handle)
    int alarmId = 0;
    float intervalMs = 0.0f;
    float elapsedMs = 0.0f;
    bool repeat = false;
    bool enabled = true;
};
std::vector<AlarmState> g_alarms;

AlarmState* FindAlarm(gmMachine& a_machine, const gmVariable& a_owner, int a_id) {
    for (auto& a : g_alarms) {
        if (a.alarmId == a_id && a.owner.m_type == GM_TABLE && a_owner.m_type == GM_TABLE &&
            a.owner.m_value.m_ref == a_owner.m_value.m_ref) {
            return &a;
        }
    }
    return nullptr;
}

gmType g_alarmHandleType = -1;

struct AlarmHandleData {
    gmVariable owner;
    int alarmId;
};

AlarmState* ThisAlarm(gmThread* a_thread) {
    AlarmHandleData* h = (AlarmHandleData*)a_thread->ThisUserCheckType(g_alarmHandleType);
    if (!h) return nullptr;
    return FindAlarm(*a_thread->GetMachine(), h->owner, h->alarmId);
}

// setInterval(ms) -- confirmed real (Ghidra FUN_0042ea30 @ 0x0042ea30),
// bare field store, does NOT reset elapsed (real callers always pair it
// with an explicit hardReset() right after, e.g. Main.gm:339-340).
int GM_CDECL gmf_alarm_setInterval(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread); if (!a) return GM_OK; // leniency: stale/deleted alarm
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    a->intervalMs = ms;
    return GM_OK;
}
// hardReset() -- confirmed real (Ghidra FUN_0042eb90 @ 0x0042eb90): zeroes
// elapsed time AND re-enables the alarm (so a one-shot alarm that already
// fired can be restarted, matching real usage restarting the SAME alarm id
// repeatedly rather than allocating a new one each time).
int GM_CDECL gmf_alarm_hardReset(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread); if (!a) return GM_OK;
    a->elapsedMs = 0.0f;
    a->enabled = true;
    return GM_OK;
}
int GM_CDECL gmf_alarm_reset(gmThread* a_thread) { return gmf_alarm_hardReset(a_thread); }
int GM_CDECL gmf_alarm_getInterval(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushFloat(a ? a->intervalMs : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_alarm_isAlarming(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushInt((a && a->enabled && a->elapsedMs >= a->intervalMs) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_alarm_getTimeTillAlarm(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushFloat(a ? std::max(0.0f, a->intervalMs - a->elapsedMs) : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_alarm_getAlarmPercentage(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    float pct = (a && a->intervalMs > 0.0001f) ? std::min(1.0f, a->elapsedMs / a->intervalMs) : 0.0f;
    a_thread->PushFloat(pct);
    return GM_OK;
}
int GM_CDECL gmf_alarm_getMSElapsed(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushFloat(a ? a->elapsedMs : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_alarm_isEnabled(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushInt((a && a->enabled) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_alarm_setIsEnabled(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread); if (!a) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 1);
    a->enabled = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_alarm_noop(gmThread* a_thread) { return GM_OK; } // update()/randomReset()/phaseReset() -- ticked centrally, see TickAlarms

gmVariable MakeAlarmHandle(gmMachine& a_machine, const gmVariable& a_owner, int a_id) {
    if (g_alarmHandleType < 0) {
        g_alarmHandleType = a_machine.CreateUserType("ALARM");
        static gmFunctionEntry lib[] = {
            {"setInterval", gmf_alarm_setInterval, nullptr},
            {"hardReset", gmf_alarm_hardReset, nullptr},
            {"reset", gmf_alarm_reset, nullptr},
            {"phaseReset", gmf_alarm_noop, nullptr},
            {"randomReset", gmf_alarm_noop, nullptr},
            {"update", gmf_alarm_noop, nullptr},
            {"getInterval", gmf_alarm_getInterval, nullptr},
            {"isAlarming", gmf_alarm_isAlarming, nullptr},
            {"getTimeTillAlarm", gmf_alarm_getTimeTillAlarm, nullptr},
            {"getAlarmPercentage", gmf_alarm_getAlarmPercentage, nullptr},
            {"getMSElapsed", gmf_alarm_getMSElapsed, nullptr},
            {"isEnabled", gmf_alarm_isEnabled, nullptr},
            {"setIsEnabled", gmf_alarm_setIsEnabled, nullptr},
        };
        a_machine.RegisterTypeLibrary(g_alarmHandleType, lib, sizeof(lib) / sizeof(lib[0]));
    }
    AlarmHandleData* h = new AlarmHandleData{a_owner, a_id};
    gmVariable v; v.SetUser(a_machine.AllocUserObject(h, g_alarmHandleType));
    return v;
}

// addAlarm(id, intervalMs, repeat) -- see this section's header comment.
// Real callers all ignore the return value except via a SEPARATE
// getAlarm(id) call later, but returning a real handle here (rather than
// null) costs nothing and matches retail's own `ALARM(...)`-return shape.
int GM_CDECL gmf_addAlarm(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(id, 0, 0);
    GM_FLOAT_OR_INT_PARAM(intervalMs, 1, 0.0f);
    // Third arg is "start already elapsed", NOT "repeat" -- Ghidra-confirmed
    // from retail's real alarm constructor (0x0044fe90, reached via
    // Process.addAlarm @ 0x0043f100):
    //     this->interval = interval;
    //     this->elapsed  = startReady ? interval : 0;
    //     this->enabled  = true;
    // i.e. passing true makes the alarm fire on its very first tick. Alarms
    // in this engine always repeat (the handler re-arms them via
    // setInterval/hardReset, exactly as retail's scripts do), so nothing is
    // lost by dropping the misread `repeat` meaning.
    //
    // This was why PekkoAndJon enemies never spawned. Main.gm:111 registers
    // the spawn alarm as `addAlarm(SPAWNWALL_ALARMID, 9999999, 1)` -- that
    // 9999999 ms (~2.8h) is a deliberate placeholder that is never meant to
    // elapse: the `1` makes it fire immediately, and the onAlarm handler
    // (Main.gm:362) then installs the real 3911-6981 ms interval and
    // hardResets. Read as `repeat`, the alarm instead sat waiting out the
    // full 2.8 hours, so onAlarm never ran and no wall of Things was ever
    // spawned. (Confirmed the 9999999 literal is genuinely in the original
    // PekkoAndJon/Main.gmc bytecode, not a decompiler artifact.)
    bool startReady = (a_thread->GetNumParams() >= 3) ? (a_thread->ParamInt(2, 0) != 0) : false;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable owner = gmVariable(self);
    AlarmState* existing = FindAlarm(m, owner, id);
    if (existing) {
        existing->intervalMs = intervalMs;
        existing->repeat = true;
        existing->elapsedMs = startReady ? intervalMs : 0.0f;
        existing->enabled = true;
    } else {
        AlarmState a;
        a.owner = owner; a.alarmId = id; a.intervalMs = intervalMs; a.repeat = true;
        a.elapsedMs = startReady ? intervalMs : 0.0f;
        g_alarms.push_back(a);
    }
    a_thread->Push(MakeAlarmHandle(m, owner, id));
    return GM_OK;
}

int GM_CDECL gmf_deleteAlarm(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(id, 0, 0);
    gmVariable owner = gmVariable(self);
    for (size_t i = 0; i < g_alarms.size(); ++i) {
        if (g_alarms[i].alarmId == id && g_alarms[i].owner.m_type == GM_TABLE &&
            g_alarms[i].owner.m_value.m_ref == owner.m_value.m_ref) {
            g_alarms.erase(g_alarms.begin() + (long)i);
            break;
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_getAlarm(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(id, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    gmVariable owner = gmVariable(self);
    if (!FindAlarm(m, owner, id)) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(MakeAlarmHandle(m, owner, id));
    return GM_OK;
}

// Ticks every live alarm and fires owner.onAlarm(alarmId) for any that
// elapsed this frame. ONE registry, called ONCE per frame from
// ProcessManager::UpdateAll -- covers both process-owned alarms (e.g.
// this.Process.addAlarm in Main.gm) and entity-owned alarms (e.g.
// this.Entity.addAlarm in Thing.gm) since `owner` is stored generically at
// addAlarm() time, whichever handle type it was called on. Entity handles
// split method/state across __behaviorOwner in this engine (unlike
// retail), so onAlarm is looked up there first if the bare handle doesn't
// define it directly -- mirrors gmf_callMethodByName's exact fallback.
void TickAlarms(gmMachine& a_machine, float a_deltaMs) {
    // Snapshot ids, not pointers -- onAlarm() can itself call addAlarm/
    // deleteAlarm (real usage: Main.gm's onAlarm handler calls hardReset()
    // on the SAME alarm it's handling), which would reallocate g_alarms
    // out from under a live loop.
    struct FireEvent { gmVariable owner; int id; };
    std::vector<FireEvent> toFire;
    for (auto& a : g_alarms) {
        if (!a.enabled) continue;
        a.elapsedMs += a_deltaMs;
        if (a.elapsedMs >= a.intervalMs) {
            toFire.push_back({a.owner, a.alarmId});
            if (a.repeat) a.elapsedMs -= a.intervalMs;
            else a.enabled = false;
        }
    }
    for (auto& ev : toFire) {
        if (ev.owner.m_type != GM_TABLE) continue;
        gmTableObject* self = (gmTableObject*)a_machine.GetObject(ev.owner.m_value.m_ref);
        if (!self) continue;
        gmTableObject* target = self;
        gmVariable targetVar = ev.owner;
        if (self->Get(&a_machine, "onAlarm").m_type != GM_FUNCTION) {
            gmVariable behaviorOwner = self->Get(&a_machine, "__behaviorOwner");
            if (behaviorOwner.m_type == GM_TABLE) {
                gmTableObject* ownerTbl = (gmTableObject*)a_machine.GetObject(behaviorOwner.m_value.m_ref);
                if (ownerTbl && ownerTbl->Get(&a_machine, "onAlarm").m_type == GM_FUNCTION) {
                    target = ownerTbl; targetVar = behaviorOwner;
                }
            }
        }
        if (target->Get(&a_machine, "onAlarm").m_type != GM_FUNCTION) continue;
        gmCall call;
        if (call.BeginTableFunction(&a_machine, "onAlarm", target, targetVar)) {
            call.AddParamInt(ev.id);
            call.End();
        }
    }
}

// ---------------------------------------------------------------------
// SAWLFO/PULSELFO/TRIANGLELFO -- confirmed real (Ghidra: retail ctors @
// 0x0042f130/0x0042f090/0x0042f1d0, all delegate into a shared base
// constructor FUN_0042d350 -- same "distinct subclasses, shared base"
// shape as ALARM). Real usage (Player.gm:23, bg.gm:20, Thing.gm:278-288)
// only ever constructs one and immediately hands it to `owner.addLFO(id,
// lfoObj)` -- no corpus script reads an LFO's live value (getValue()) or
// retunes it (setRate/setRange/setPhase), so unlike ALARM this doesn't
// need a real oscillator tick, just to not be null (a null addLFO return
// silently no-ops on `this.testlfo1.someMethod()` later via the usual
// leniency, but `this.testlfo1=...addLFO(...)` itself doesn't crash
// either way) -- kept as lightweight real objects (not stubs) so any
// FUTURE corpus script that DOES read one isn't silently wrong.
// ---------------------------------------------------------------------
gmType g_lfoType = -1;
struct LFOData { float phase = 0.0f; float rate = 1.0f; float lo = 0.0f; float hi = 1.0f; int waveform = 0; };
LFOData* ThisLFO(gmThread* a_thread) { return (LFOData*)a_thread->ThisUserCheckType(g_lfoType); }
int GM_CDECL gmf_lfo_setPhase(gmThread* a_thread) { LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK; GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f); l->phase = v; return GM_OK; }
int GM_CDECL gmf_lfo_setRate(gmThread* a_thread) { LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK; GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f); l->rate = v; return GM_OK; }
int GM_CDECL gmf_lfo_setRange(gmThread* a_thread) { LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK; GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f); l->lo = lo; l->hi = hi; return GM_OK; }
int GM_CDECL gmf_lfo_getValue(gmThread* a_thread) {
    LFOData* l = ThisLFO(a_thread);
    if (!l) { a_thread->PushFloat(0.0f); return GM_OK; }
    float t = l->phase - std::floor(l->phase); // [0,1) sawtooth phase
    float shaped;
    if (l->waveform == 1) { // PULSELFO -- square
        shaped = (t < 0.5f) ? 0.0f : 1.0f;
    } else if (l->waveform == 2) { // TRIANGLELFO
        shaped = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
    } else { // SAWLFO
        shaped = t;
    }
    a_thread->PushFloat(l->lo + (l->hi - l->lo) * shaped);
    return GM_OK;
}
int GM_CDECL gmf_lfo_update(gmThread* a_thread) {
    LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    l->phase += (ms * 0.001f) * l->rate;
    return GM_OK;
}
gmVariable MakeLFO(gmMachine& a_machine, int a_waveform) {
    if (g_lfoType < 0) {
        g_lfoType = a_machine.CreateUserType("LFO");
        static gmFunctionEntry lib[] = {
            {"setPhase", gmf_lfo_setPhase, nullptr},
            {"setRandomPhase", gmf_lfo_setPhase, nullptr},
            {"setRate", gmf_lfo_setRate, nullptr},
            {"setRange", gmf_lfo_setRange, nullptr},
            {"setContour", gmf_alarm_noop, nullptr},
            {"getValue", gmf_lfo_getValue, nullptr},
            {"update", gmf_lfo_update, nullptr},
        };
        a_machine.RegisterTypeLibrary(g_lfoType, lib, sizeof(lib) / sizeof(lib[0]));
    }
    LFOData* l = new LFOData();
    l->waveform = a_waveform;
    gmVariable v; v.SetUser(a_machine.AllocUserObject(l, g_lfoType));
    return v;
}
int GM_CDECL gmf_SAWLFO(gmThread* a_thread) { a_thread->Push(MakeLFO(*a_thread->GetMachine(), 0)); return GM_OK; }
int GM_CDECL gmf_PULSELFO(gmThread* a_thread) { a_thread->Push(MakeLFO(*a_thread->GetMachine(), 1)); return GM_OK; }
// No gmf_TRIANGLELFO here -- lfo_bindings.cpp registers the real global
// "TRIANGLELFO" constructor now (see RegisterFoECompatBindings's own
// comment on why this file no longer competes with it).

// addLFO(slotId, lfoObj) -- confirmed real (Player.gm:23:
// `this.testlfo1=this.Entity.addLFO(0,TRIANGLELFO());`) -- returns the
// SAME lfo object passed in (matches every real call site assigning the
// return value directly, never a separate wrapper).
int GM_CDECL gmf_addLFO(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    a_thread->Push(a_thread->Param(1));
    return GM_OK;
}
int GM_CDECL gmf_deleteLFO(gmThread* a_thread) { return GM_OK; }

// ---------------------------------------------------------------------
// GameBoard -- confirmed real global (Ghidra: getMinPos/getMaxPos/
// getWidth/getRangeX/getRangeY/makeRandomPos etc all in the same
// contiguous table, 0x00440960-0x00441230). Was entirely unbound before
// this pass -- every one of these silently null-called, and Main.gm's own
// mouse-to-world mapping (`GameBoard.getMinPos().x + Input.getMousePosX()
// * GameBoard.getRangeX()`) reading null.x/null* is a hard "getdot
// failed"/leniently-null-propagating chain, so click-and-drag (this
// game's ENTIRE stated control scheme, see the title screen's own "click
// and drag everything") never resolved a real world position at all.
//
// RETAIL MATCH -- these are the real values, read as literals out of the
// GameBoard constructor (retail FUN_0040df90):
//
//     *param_1   = 0xc515f000;   // minX
//     param_1[1] = 0xc4a76000;   // minY
//     param_1[2] = 0x4515f000;   // maxX
//     param_1[3] = 0x44a76000;   // maxY
//
// decoding as IEEE-754 float to exactly:
//
//     min = (-2399.0, -1339.0)      max = (+2399.0, +1339.0)
//     width = 4798.0   height = 2678.0   aspect = 1.7916
//
// The GameBoard object itself is a plain 16-byte AABB of four floats
// (minX, minY, maxX, maxY), reached as ((device + 0x2c)) -> +0xd4c, with
// FUN_0040dfc0 copying out (minX, minY) and FUN_0040dfe0 copying out
// (maxX, maxY). Retail also exposes it as an ortho view
// (FUN_004215a0: gluOrtho2D(minX, maxX, minY, maxY)).
//
// This supersedes a guess of half-height 4000 with half-width derived as
// 4000*16/9 = 7111, on the stated grounds that "retail's actual bounds
// aren't Ghidra-recoverable -- every GameBoard accessor is an opaque vtable
// call into a runtime singleton with no visible constant". They were
// recoverable; the accessors are opaque but the CONSTRUCTOR is not. That
// guess was ~3x too large on both axes, across the 47 real call sites
// (getMinPos x22, getMaxPos x18, getWidth x2, getRangeX/Y x4,
// makeRandomPos x1) that drive mouse-to-world mapping and enemy spawn
// placement.
//
// Note the real board is NOT a 16:9 rectangle (1.7916, not 1.7778), so the
// two axes are independent literals rather than one constant and a derived
// aspect.
//
// Independent cross-check: the recovered perspective projection (fov
// 53.13010 deg, so tan(fov/2) is exactly 0.5, i.e. visible world height
// equals camera Z) predicts visible half-extents of (2389.3, 1344.0) at
// PekkoAndJon/Main.gm's baseCameraZ = 2688. The constructor's literals are
// (2399, 1339) -- agreement within 0.4% from two completely independent
// sources, which corroborates both the projection port and these bounds.
// ---------------------------------------------------------------------
constexpr float kGameBoardMinX = -2399.0f;
constexpr float kGameBoardMinY = -1339.0f;
constexpr float kGameBoardMaxX = 2399.0f;
constexpr float kGameBoardMaxY = 1339.0f;
constexpr float kGameBoardHalfWidth = kGameBoardMaxX;   // board is origin-centred
constexpr float kGameBoardHalfHeight = kGameBoardMaxY;

int GM_CDECL gmf_GameBoard_getMinPos(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), -kGameBoardHalfWidth, -kGameBoardHalfHeight));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getMaxPos(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), kGameBoardHalfWidth, kGameBoardHalfHeight));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getMidPos(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), 0.0f, 0.0f));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getWidth(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfWidth * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getHeight(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfHeight * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getRangeX(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfWidth * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getRangeY(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfHeight * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getRange(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), kGameBoardHalfWidth * 2.0f, kGameBoardHalfHeight * 2.0f));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getHypotenuse(gmThread* a_thread) {
    float w = kGameBoardHalfWidth * 2.0f, h = kGameBoardHalfHeight * 2.0f;
    a_thread->PushFloat(std::sqrt(w * w + h * h));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_makeRandomPos(gmThread* a_thread) {
    float x = -kGameBoardHalfWidth + (kGameBoardHalfWidth * 2.0f) * Random01();
    float y = -kGameBoardHalfHeight + (kGameBoardHalfHeight * 2.0f) * Random01();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), x, y));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_isInGameBoard(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    float x = 0, y = 0;
    bool ok = false;
    if (a_thread->GetNumParams() >= 2) {
        GM_FLOAT_OR_INT_PARAM(px, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(py, 1, 0.0f);
        x = px; y = py; ok = true;
    } else {
        ok = GetVector2D(m, a_thread->Param(0), x, y);
    }
    bool inBoard = ok && x >= -kGameBoardHalfWidth && x <= kGameBoardHalfWidth &&
                    y >= -kGameBoardHalfHeight && y <= kGameBoardHalfHeight;
    a_thread->PushInt(inBoard ? 1 : 0);
    return GM_OK;
}
// setMinPos/setMaxPos -- confirmed real setters but no corpus call site;
// this engine's GameBoard bounds are fixed constants (see above), so these
// are honest no-ops rather than a half-implemented mutable-bounds system
// nothing exercises.
int GM_CDECL gmf_GameBoard_setBoundsNoop(gmThread* a_thread) { return GM_OK; }

gmFunctionEntry s_gameBoardLib[] = {
    {"getMinPos", gmf_GameBoard_getMinPos, nullptr},
    {"getMaxPos", gmf_GameBoard_getMaxPos, nullptr},
    {"getMidPos", gmf_GameBoard_getMidPos, nullptr},
    {"getWidth", gmf_GameBoard_getWidth, nullptr},
    {"getHeight", gmf_GameBoard_getHeight, nullptr},
    {"getRangeX", gmf_GameBoard_getRangeX, nullptr},
    {"getRangeY", gmf_GameBoard_getRangeY, nullptr},
    {"getRange", gmf_GameBoard_getRange, nullptr},
    {"getHypotenuse", gmf_GameBoard_getHypotenuse, nullptr},
    {"makeRandomPos", gmf_GameBoard_makeRandomPos, nullptr},
    {"makeRandomPosOutsideOfGameBoard", gmf_GameBoard_makeRandomPos, nullptr},
    {"isInGameBoard", gmf_GameBoard_isInGameBoard, nullptr},
    {"setMinPos", gmf_GameBoard_setBoundsNoop, nullptr},
    {"setMaxPos", gmf_GameBoard_setBoundsNoop, nullptr},
};

// setTexture() -- Stub (not in binary)
int GM_CDECL gmf_setTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    return GM_OK;
}

// setIsUsesTexture() -- Stub
int GM_CDECL gmf_setIsUsesTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    return GM_OK;
}

// isUsesTexture() -- Stub
int GM_CDECL gmf_isUsesTexture(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}

// beginTexture() -- Stub (not in binary)
int GM_CDECL gmf_beginTexture(gmThread* a_thread) {
    return GM_OK;
}

// endTexture() -- Stub (not in binary)
int GM_CDECL gmf_endTexture(gmThread* a_thread) {
    return GM_OK;
}

// loadTexture() -- Stub
int GM_CDECL gmf_loadTexture(gmThread* a_thread) {
    a_thread->PushNull();
    return GM_OK;
}

// deleteLoadedTextures() -- Stub
int GM_CDECL gmf_deleteLoadedTextures(gmThread* a_thread) {
    return GM_OK;
}

// drawTexture() -- Stub
int GM_CDECL gmf_drawTexture(gmThread* a_thread) {
    return GM_OK;
}

void RegisterFoECompatBindings(gmMachine& a_machine) {
    // Global random functions
    a_machine.GetGlobals()->Set(&a_machine, "RANDOM_GetRandomReal",
        gmVariable(a_machine.AllocFunctionObject(gmf_RANDOM_GetRandomReal)));
    a_machine.GetGlobals()->Set(&a_machine, "RANDOM_GetRandomInteger",
        gmVariable(a_machine.AllocFunctionObject(gmf_RANDOM_GetRandomInteger)));
    a_machine.GetGlobals()->Set(&a_machine, "RANDOM_GetRandomBoolean",
        gmVariable(a_machine.AllocFunctionObject(gmf_RANDOM_GetRandomBoolean)));
    a_machine.GetGlobals()->Set(&a_machine, "SAWLFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_SAWLFO)));
    a_machine.GetGlobals()->Set(&a_machine, "PULSELFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_PULSELFO)));
    // TRIANGLELFO deliberately NOT registered here (see gmf_TRIANGLELFO's
    // issue #2: lfo_bindings.cpp's RegisterLfoBindings (main.cpp, runs
    // BEFORE this function) already registers a real "TRIANGLELFO" global
    // backed by a full oscillator matching the confirmed decompiled
    // setRandomPhase/setContour, all confirmed bodies) -- this file's own
    // simpler stand-in was clobbering it here since RegisterFoECompatBindings
    // runs last, making the real implementation permanently dead code.
    // PulseLfo.c only have their CONSTRUCTOR recovered (no further methods),
    // and lfo_bindings.cpp doesn't register global constructors for either
    // -- this file's shared LFOData stand-in is the only implementation
    // either one has.

    // REALPOINT type (2D point/vector)
    a_machine.GetGlobals()->Set(&a_machine, "REALPOINT",
        gmVariable(a_machine.AllocFunctionObject(gmf_REALPOINT)));

    // Alarm ID management
    a_machine.GetGlobals()->Set(&a_machine, "getUniqueAlarmID",
        gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueAlarmID)));
    a_machine.GetGlobals()->Set(&a_machine, "addAlarm",
        gmVariable(a_machine.AllocFunctionObject(gmf_addAlarm)));
    a_machine.GetGlobals()->Set(&a_machine, "deleteAlarm",
        gmVariable(a_machine.AllocFunctionObject(gmf_deleteAlarm)));
    a_machine.GetGlobals()->Set(&a_machine, "getAlarm",
        gmVariable(a_machine.AllocFunctionObject(gmf_getAlarm)));

    // LFO ID management
    a_machine.GetGlobals()->Set(&a_machine, "getUniqueLFOID",
        gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueLFOID)));
    a_machine.GetGlobals()->Set(&a_machine, "addLFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_addLFO)));
    a_machine.GetGlobals()->Set(&a_machine, "deleteLFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_deleteLFO)));

    // Texture functions
    a_machine.GetGlobals()->Set(&a_machine, "setTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_setTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "setIsUsesTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_setIsUsesTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "isUsesTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_isUsesTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "loadTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_loadTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "deleteLoadedTextures",
        gmVariable(a_machine.AllocFunctionObject(gmf_deleteLoadedTextures)));
    a_machine.GetGlobals()->Set(&a_machine, "drawTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_drawTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "beginTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_beginTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "endTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_endTexture)));

    a_machine.RegisterLibrary(s_gameBoardLib, sizeof(s_gameBoardLib) / sizeof(s_gameBoardLib[0]), "GameBoard");
}

bool IsPointInGameBoard(float a_x, float a_y) {
    return a_x >= -kGameBoardHalfWidth && a_x <= kGameBoardHalfWidth &&
           a_y >= -kGameBoardHalfHeight && a_y <= kGameBoardHalfHeight;
}
