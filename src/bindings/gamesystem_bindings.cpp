#include "gmThread.h"
#include "gamesystem_bindings.h"
#include "../core/engine_context.h"
#include "../core/process_manager.h"
#include "gmTableObject.h"
#include "vector2d_bindings.h"
#include "../core/strict_calls.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <cmath>
#include <random>
#include <vector>

namespace {

std::mt19937& Rng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}
float Random01() {
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(Rng());
}

// ---------------------------------------------------------------------
// Bookkeeping flags (isBeingNice/isShouldUpdateEntities/setALevelLoaded/
// isLoading/debug_setStop -- all confirmed real getter/setter pairs over
// see core/game_system_state.h for what each flag means and why.
// ---------------------------------------------------------------------

int GM_CDECL gmf_getCurrentTick(gmThread* a_thread) {
    a_thread->PushInt((gmint)GetEngineContext().gameSystem.currentTick);
    return GM_OK;
}
int GM_CDECL gmf_isBeingNice(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isBeingNice ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsBeingNice(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isBeingNice = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_isShouldUpdateEntities(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.shouldUpdateEntities ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsShouldUpdateEntities(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.shouldUpdateEntities = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_setALevelLoaded(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isALevelLoaded = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsLoading(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isLoadingFlag = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_isLoading(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isLoadingFlag ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsInputEnabled(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isInputEnabled = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_debug_setStop(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.debugStopped = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_debug_isStopped(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.debugStopped ? 1 : 0);
    return GM_OK;
}

// ---------------------------------------------------------------------
// Process management -- confirmed real, all backed by ProcessManager.
// ---------------------------------------------------------------------

// Confirmed real: flushProcesses' decompiled body is literally `return 0;`
// with no calls at all -- a genuine no-op in retail too, not a decompile
// gap (compare every other "empty-looking" function here, which all call
// at least one FUN_ helper).
int GM_CDECL gmf_flushProcesses(gmThread* a_thread) {
    return GM_OK;
}

// Confirmed real: accepts 1 or 2 args (name, [flag]) -- the second bool's
// purpose isn't recoverable from the decompiled body alone (no observable
// effect beyond being passed through to the native constructor), so it's
// read and currently ignored rather than guessed at.
// Return value CONFIRMED real (re-checked after re-deriving from usage):
// every real call site (e.g. Platformer/GenericOkDialogProcess.gm:
// `local proc = GameSystem.runProcess(...); proc.configure(...);`) calls a
// method directly on the returned value -- it has to be the process's own
// table, not a bare id, or `.configure(...)` couldn't resolve. Pushing a
// gmint here (the original implementation) silently broke every caller
// that configures a process this way, e.g. Platformer/PopupProcess.gm's
// Safe drop-in replacement for Platformer/Main.gm's own
// `this.scrAddEntityLessObj.fn=function(a,b){return a.configureData.uid<b.configureData.uid;}`
// (constructor(), ~line 1015) -- some entity types' configure() chain
// doesn't end up setting `configureData` at all (confirmed: not every
// entity type installs the EntityBase.gm mixin that does
// `this.configureData=configureData;`, e.g. Platformer/EntityPacks/
// GroundResizeNoRotate.gm only calls Platformer_GroundBase_Install, and
// this pass hasn't traced every other entity type's own install
// convention), so the real comparator's direct `.configureData.uid`
// property chain hard-crashes ("getdot failed") the moment two same-type
// entities are compared and one has a null configureData -- which then
// permanently wedges Main.gm's incremental level-load loop (the
// exception aborts configureEntities() before it can ever advance
// `progress.startIndex`/mark `.done`, so it retries every frame,
// forever). Reads both sides defensively (missing configureData/uid ->
// 0) instead of trying to guarantee every entity type always has one.
int GM_CDECL gmf_native_scrAddEntityLessObj(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    auto readUid = [&](int a_param) -> double {
        gmVariable v = a_thread->Param(a_param);
        if (v.m_type != GM_TABLE) return 0.0;
        gmTableObject* t = (gmTableObject*)m.GetObject(v.m_value.m_ref);
        gmVariable cd = t->Get(&m, "configureData");
        if (cd.m_type != GM_TABLE) return 0.0;
        gmVariable uid = ((gmTableObject*)m.GetObject(cd.m_value.m_ref))->Get(&m, "uid");
        if (uid.m_type == GM_INT) return (double)uid.m_value.m_int;
        if (uid.m_type == GM_FLOAT) return (double)uid.m_value.m_float;
        return 0.0;
    };
    a_thread->PushInt(readUid(0) < readUid(1) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_startNewProcessList(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(name, 0, "");
    ProcessManager* pm = GetEngineContext().processManager;
    int id = pm ? pm->StartNewProcessList(name) : -1;
    Process* proc = (id >= 0 && pm) ? pm->Get(id) : nullptr;

    if (proc && std::strcmp(name, "Platformer_Main") == 0) {
        gmMachine& m = *a_thread->GetMachine();
        gmTableObject* mainTable = (gmTableObject*)m.GetObject(proc->table.m_value.m_ref);
        gmVariable lessObj = mainTable->Get(&m, "scrAddEntityLessObj");
        if (lessObj.m_type == GM_TABLE) {
            gmTableObject* lessObjTable = (gmTableObject*)m.GetObject(lessObj.m_value.m_ref);
            lessObjTable->Set(&m, "fn", gmVariable(m.AllocFunctionObject(gmf_native_scrAddEntityLessObj)));
        }
    }

    // ENGINE PATCH (2026-08-17): teardown of every OTHER live process used
    // to be gated behind an optional 2nd bool arg (see retail_symbols.inc's
    // "GameSystem.startNewProcessList" -- confirmed real, but this specific
    // 2nd-arg gate was an unconfirmed guess, "No decompiled evidence for
    // its exact effect"). That guess broke the single most common real call
    // pattern in the whole corpus: EVERY "Platformer_MenuLauncher" call site
    // (PauseProcessBase.gm's own Exit button, WinScreenProcessBase.gm,
    // Main.gm's several menu-return paths) passes a single bare-string arg,
    // no 2nd arg at all -- confirmed live: exiting a paused level left
    // Platformer_Main/Platformer_PauseProcess/etc. still alive and, worse,
    // ProcessManager::UpdateAll's own pause gate (`m_paused && proc.id !=
    // m_pauseExemptProcessId`) kept skipping every process INCLUDING the
    // freshly-started Platformer_MenuLauncher itself, since the old
    // Platformer_PauseProcess (still the pause-exempt id) was never told to
    // quit -- a permanent black-screen hang with 0% chance of a real
    // retail player ever hitting it exiting a level normally. The function's
    // own name ("start a NEW process list") reads as an unconditional
    // list-replacement, not a conditional one -- so this now always tears
    // down every other process, matching every real "MenuLauncher"/
    // "Platformer_Main" call site's actual required behavior. Only real
    // counter-evidence was an unconfirmed assumption about DevMain.gm/
    // ConsoleProcess.gm (developer test-picker menus, not real playable
    // content -- checked directly: every live, non-commented-out call site
    // in both files already only starts ONE process at a time, so this
    // changes nothing for them either). Termination alone is enough to also
    // fix the pause-hang specifically: ProcessManager's own reap pass
    // (process_manager.cpp, `if (id == m_pauseExemptProcessId)
    // SetPauseState(false, -1);`) already unpauses automatically the moment
    // the exempt process is actually reaped.
    if (proc && pm) {
        for (int i = 0; i < pm->GetCount(); ++i) {
            if (Process* other = pm->GetByIndex(i)) {
                if (other->id != id) other->wantsQuit = true;
            }
        }
    }

    if (proc) a_thread->Push(proc->table); else a_thread->PushInt(-1);
    return GM_OK;
}

// Confirmed real (re-checked against the decompiled body): 1 or 2 args
// (name, [flag]) -- reads a GM_STRING argument via the exact same
// stack-offset pattern startNewProcessList uses for its name argument, NOT
// a table as originally assumed. Real usage confirms this too
// (game_data/Main.gm: `GameSystem.runProcess("ControllerConfig");`,
// `GameSystem.runProcess("Platformer_ResizeScreenProcess");` -- always a
// bare name string). The 2nd bool arg's effect isn't recoverable from the
// decompiled body alone (same "flag not recoverable" situation as
// startNewProcessList) -- accepted and ignored.
int GM_CDECL gmf_runProcess(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(name, 0, "");
    ProcessManager* pm = GetEngineContext().processManager;
    int id = pm ? pm->StartNewProcessList(name) : -1;
    Process* proc = (id >= 0 && pm) ? pm->Get(id) : nullptr;
    if (proc) a_thread->Push(proc->table); else a_thread->PushInt(-1);
    return GM_OK;
}

// Confirmed real: accepts EITHER an int (process id) or a string (process
// name) -- same dual-type argument as getProcess (see its comment below),
// and confirmed by every real call site in the corpus (100% pass a
// string name, e.g. `GameSystem.isProcessRunning("Platformer_MainMenu5")`
// -- never an int). The previous int-only implementation silently
// defaulted to id=-1 for every real (string) call, so this always
// returned false regardless of whether the named process was actually
// running -- broke every "don't start this again if it's already
// running" check in the corpus, causing the caller to keep relaunching
// the same process every single frame.
int GM_CDECL gmf_isProcessRunning(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    ProcessManager* pm = GetEngineContext().processManager;
    Process* proc = nullptr;
    if (pm) {
        const gmVariable& p = a_thread->Param(0);
        if (p.m_type == GM_STRING) {
            char buf[256];
            proc = pm->GetByName(p.AsString(a_thread->GetMachine(), buf, sizeof(buf)));
        } else if (p.m_type == GM_INT) {
            proc = pm->Get(p.m_value.m_int);
        }
    }
    a_thread->PushInt((proc && proc->initialized && !proc->wantsQuit) ? 1 : 0);
    return GM_OK;
}

// Confirmed real: accepts EITHER an int (process id) or a string (process
// name) -- the decompiled body branches on the argument's type tag
// (GM_NULL/GM_INT vs. the retail string tag) and calls a different native
// lookup for each.
int GM_CDECL gmf_getProcess(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    ProcessManager* pm = GetEngineContext().processManager;
    Process* proc = nullptr;
    if (pm) {
        const gmVariable& p = a_thread->Param(0);
        if (p.m_type == GM_STRING) {
            char buf[256];
            proc = pm->GetByName(p.AsString(a_thread->GetMachine(), buf, sizeof(buf)));
        } else if (p.m_type == GM_INT) {
            proc = pm->Get(p.m_value.m_int);
        }
    }
    if (proc) a_thread->Push(proc->table);
    else a_thread->PushNull();
    return GM_OK;
}

// Confirmed real: 0-arg getter for "the" gameplay process -- every real
// call site (EntityBase.gm:1703's `t.gameProcess=GameSystem.getEntityUpdateProcess();`,
// Player.gm:19, etc) stores the result and later reads gameplay-session
// fields off it (`.inMenuMode`, `.mode`, `.refreshScripts()`) that only
// exist on Platformer/Main.gm's own returned table (confirmed:
// `this.inMenuMode=menuMode;` in Platformer/Main.gm's constructor) -- and
// every real start site for that process
// (ConsoleProcess.gm/DebugMenu.gm/DevMain.gm/TouchEditProcessNGP.gm) names
// it literally "Platformer_Main". Returns null (as before) if that process
// isn't running yet, e.g. still on the title screen.
int GM_CDECL gmf_getEntityUpdateProcess(gmThread* a_thread) {
    ProcessManager* pm = GetEngineContext().processManager;
    Process* proc = pm ? pm->GetByName("Platformer_Main") : nullptr;
    if (proc) a_thread->Push(proc->table); else a_thread->PushNull();
    return GM_OK;
}

// Confirmed real: returns some kind of list/container of all processes.
// The decompiled body allocates what looks like a ref-container object
// (matches the confirmed ENTITYLISTREFCONTAINER/INTLISTREFCONTAINER
// plain sequential-integer-keyed GM table of process ids instead, which
// `foreach`/tableCount() work on the same way a real array would.
int GM_CDECL gmf_getAllProcesses(gmThread* a_thread) {
    ProcessManager* pm = GetEngineContext().processManager;
    gmTableObject* table = a_thread->GetMachine()->AllocTableObject();
    if (pm) {
        int n = pm->GetCount();
        for (int i = 0; i < n; ++i) {
            Process* p = pm->GetByIndex(i);
            if (p) table->Set(a_thread->GetMachine(), i, gmVariable(GM_INT, p->id));
        }
    }
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_getProcessCount(gmThread* a_thread) {
    ProcessManager* pm = GetEngineContext().processManager;
    a_thread->PushInt(pm ? pm->GetCount() : 0);
    return GM_OK;
}

int GM_CDECL gmf_getProcessByIndex(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(index, 0, 0);
    ProcessManager* pm = GetEngineContext().processManager;
    Process* p = pm ? pm->GetByIndex(index) : nullptr;
    if (p) a_thread->Push(p->table);
    else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL gmf_quit(gmThread* a_thread) {
    GetEngineContext().quitRequested = true;
    return GM_OK;
}

// openConsole() -- confirmed real (PS3 dev-kit debug/TTY console toggle),
// live call sites BaseNotePlant.gm:549 and Player.gm:4455, both inside
// "ERROR: how did i..." diagnostic branches (`GameSystem.trace("ERROR:
// ...");GameSystem.openConsole();`) -- was entirely unbound, meaning
// hitting either error branch hard-crashed ("getdot failed") instead of
// just logging and continuing, turning a soft diagnostic warning into a
// fatal abort. This reimplementation already prints every GameSystem.trace
// straight to stderr with no separate console UI to open, so genuine
// no-op -- consumed rather than left as a null-call.
int GM_CDECL gmf_openConsole(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// ---------------------------------------------------------------------
// Timers -- confirmed real, backed by GameSystemState's steady_clock
// timers. See game_system_state.h for the resetSimulationTimer note.
// ---------------------------------------------------------------------

int GM_CDECL gmf_resetSimulationTimer(gmThread* a_thread) {
    GetEngineContext().gameSystem.ResetPlayTimer();
    return GM_OK;
}
int GM_CDECL gmf_getMSAlive(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().gameSystem.GetMsAlive());
    return GM_OK;
}
int GM_CDECL gmf_getMSSincePlayStart(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().gameSystem.GetMsSincePlayStart());
    return GM_OK;
}
// under the GameMonkeyScript class generically like several other
// GameSystem.* getters in that index, but called script-side as
// `GameSystem.getUpdateInterval()` -- e.g. EntityPacks/VicVolcano.gm:200's
// `local tinc=GameSystem.getUpdateInterval();` feeding a fixed-step
// trajectory precompute loop) -- but never bound to the GameSystem
// library table at all, so the call resolved to null (a valid table with
// an absent key, not a strict-trap-covered case) and every arithmetic use
// of the result threw a real "operator X undefined for type null" script
// exception, breaking that entity's whole configure() step. The
// decompiled body itself is opaque (single call into an unnamed FUN_
// helper, no traceable formula) but a getter with no args, returning a
// float, whose real call sites all use it as a fixed per-tick time delta
// -- the standard PS3-era 60Hz assumption already implicit everywhere
// else in this engine (SDL_GetTicks-driven deltaMs updates).
int GM_CDECL gmf_getUpdateInterval(gmThread* a_thread) {
    a_thread->PushFloat(1000.0f / 60.0f);
    return GM_OK;
}
int GM_CDECL gmf_resetPlayTimer(gmThread* a_thread) {
    GetEngineContext().gameSystem.ResetPlayTimer();
    return GM_OK;
}

// ---------------------------------------------------------------------
// Random -- confirmed real arities from the decompiled arg-count
// dispatch (each branch confirmed by reading which gmThread argument
// slots get dereferenced). A real std::mt19937, not rand().
// ---------------------------------------------------------------------

// Confirmed real: TWO args (min, max) -- not one. An earlier version of
// this binding only took a single "max" argument; fixed after re-checking
// the decompiled body, which clearly reads two argument slots (param base
// and param base+0xc, i.e. arg0 and arg1) before calling the native RNG.
int GM_CDECL gmf_getRandomInteger(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 1, 0.0f);
    int a = (int)lo, b = (int)hi;
    if (a > b) std::swap(a, b);
    std::uniform_int_distribution<int> dist(a, b);
    a_thread->PushInt(dist(Rng()));
    return GM_OK;
}

int GM_CDECL gmf_getRandomBoolean(gmThread* a_thread) {
    a_thread->PushInt(Random01() < 0.5f ? 1 : 0);
    return GM_OK;
}

// Confirmed real: 0 args, returns a vector2D (retail type tag 3 -- two
// gmVariable type-tag table). This engine DOES have a native vector2D type
// (vector2d_bindings.h's MakeVector2D, used throughout Player.gm etc) --
// this used to return a plain {x=,y=} table instead, which superficially
// worked for field access (`dir.x`) but not for vector2D operators/methods
// (`dir*scalar`, `.length()`, `.normalize()`...). Confirmed a real crash,
// not just theoretical: EntityPacks/BrianMover.gm's particle spawner does
// `GameSystem.getRandomDir()*GameSystem.getRandomReal(...)`, which threw
// "operator mul undefined for type table and float" once entity update()
// dispatch actually started running (see UpdateEntitiesWithNativeUpdateHook).
int GM_CDECL gmf_getRandomDir(gmThread* a_thread) {
    float angle = Random01() * 6.28318530718f;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), std::cos(angle), std::sin(angle)));
    return GM_OK;
}

int GM_CDECL gmf_getRandomAngle(gmThread* a_thread) {
    a_thread->PushFloat(Random01() * 6.28318530718f);
    return GM_OK;
}

int GM_CDECL gmf_getRandomSign(gmThread* a_thread) {
    a_thread->PushFloat(Random01() < 0.5f ? -1.0f : 1.0f);
    return GM_OK;
}

// Arity (1 or 2 args) and formula now fully decompiled
// helpers FUN_0018bd38 (1-arg)/FUN_0018bdd0 (2-arg)): both are
// `randomSign() * uniform(lo,hi)` where the 1-arg form's lo is a fixed
// 0.0 constant (DAT_0018bd34) and hi is the sole argument, and randomSign
// is its own independent coin-flip draw (FUN_000c8ee0(state,0,1)==0 -> +1
// else -1) -- a SEPARATE random draw from the magnitude's, not derived
// from it, exactly matching this engine's existing gmf_getRandomSign.
int GM_CDECL gmf_getRandomSignAndReal(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    float lo, hi;
    if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
        lo = a; hi = b;
    } else if (n == 1) {
        GM_FLOAT_OR_INT_PARAM(b, 0, 0.0f);
        lo = 0.0f; hi = b;
    } else {
        return GM_EXCEPTION;
    }
    float sign = (Random01() < 0.5f) ? 1.0f : -1.0f;
    float mag = lo + (hi - lo) * Random01();
    a_thread->PushFloat(sign * mag);
    return GM_OK;
}

// Arity (0, 2, or 4 args) and formula now fully decompiled
// native helpers FUN_0018bb74/FUN_0018bba0/FUN_0018bbcc -> FUN_000c8e90/
// FUN_000c9080, which bottom out in `rand01() = rawUint32 * DAT_000c8e8c`
// and `uniform(min,max) = min + (max-min)*rand01()`):
//   0 args: rand01()
//   2 args (min,max): min + (max-min)*rand01()
//   4 args (min,max,biasMin,biasMax): min + (max-min)*uniform(biasMin,biasMax)
//     i.e. the interpolation factor itself is drawn from [biasMin,biasMax]
//     instead of [0,1] -- confirmed by FUN_0018bbcc's body, which is
//     exactly `(max-min)*FUN_000c9080(biasMin,biasMax) + min`.
int GM_CDECL gmf_getRandomReal(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n >= 4) {
        GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
        GM_FLOAT_OR_INT_PARAM(biasLo, 2, 0.0f);
        GM_FLOAT_OR_INT_PARAM(biasHi, 3, 1.0f);
        float t = biasLo + (biasHi - biasLo) * Random01();
        a_thread->PushFloat(lo + (hi - lo) * t);
    } else if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
        a_thread->PushFloat(lo + (hi - lo) * Random01());
    } else {
        a_thread->PushFloat(Random01());
    }
    return GM_OK;
}

// Fixed arity 3 (min, max, shaping exponent) -- confirmed decompiled
// (FUN_0018bc20 -> FUN_001dc1c0, PowerPC libm's pow(x,y) special-case
// wrapper): result = min + (max-min)*pow(rand01(), exponent). This
// matches the engine's earlier (previously unconfirmed) guess exactly.
int GM_CDECL gmf_getRandomRealExp(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
    GM_FLOAT_OR_INT_PARAM(expParam, 2, 1.0f);
    float t = std::pow(Random01(), expParam);
    a_thread->PushFloat(lo + (hi - lo) * t);
    return GM_OK;
}

int GM_CDECL gmf_getSimulationSpeed(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().gameSystem.simulationSpeed);
    return GM_OK;
}
int GM_CDECL gmf_setSimulationSpeed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(speed, 0, 1.0f);
    GetEngineContext().gameSystem.simulationSpeed = speed;
    return GM_OK;
}

// ---------------------------------------------------------------------
// Temporary storage (entity copy/paste clipboard) -- confirmed real 0-GM-
// arg functions (no gmThread param reads, no gmThread pushes in the
// decompiled body), so they must operate on an implicit value rather than
// an explicit argument/return. See game_system_state.h for why `this`
// (the calling table) is that implicit value.
// ---------------------------------------------------------------------

int GM_CDECL gmf_TemporaryStorageCopy(gmThread* a_thread) {
    gmTableObject* thisTable = a_thread->ThisTable();
    if (!thisTable) return GM_EXCEPTION; // confirmed: real body returns exception on failure
    GetEngineContext().gameSystem.temporaryStorage = *a_thread->GetThis();
    return GM_OK;
}

int GM_CDECL gmf_TemporaryStoragePaste(gmThread* a_thread) {
    GameSystemState& state = GetEngineContext().gameSystem;
    if (state.temporaryStorage.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* dst = a_thread->ThisTable();
    gmTableObject* src = (gmTableObject*)a_thread->GetMachine()->GetObject(state.temporaryStorage.m_value.m_ref);
    if (!dst || !src) return GM_EXCEPTION;
    // Shallow copy every key from the clipboard table onto `this`.
    gmTableIterator it;
    for (gmTableNode* node = src->GetFirst(it); !src->IsNull(it); node = src->GetNext(it)) {
        dst->Set(a_thread->GetMachine(), node->m_key, node->m_value);
    }
    return GM_OK;
}

// ---------------------------------------------------------------------
// Autoload (dev/debug "jump straight into a level") -- confirmed real,
// all read-only getters here (no setters in this class -- real retail
// almost certainly set these from command-line args before the VM boots).
// Defaults to "off"; see game_system_state.h.
// ---------------------------------------------------------------------

int GM_CDECL gmf_isAutoLoadLevel(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isAutoLoadLevel ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getAutoLoadLevel(gmThread* a_thread) {
    a_thread->PushNewString(GetEngineContext().gameSystem.autoLoadLevel.c_str());
    return GM_OK;
}
int GM_CDECL gmf_getAutoLoadPath(gmThread* a_thread) {
    a_thread->PushNewString(GetEngineContext().gameSystem.autoLoadPath.c_str());
    return GM_OK;
}
// Confirmed real: returns null when the underlying value is unset/zero,
// a string otherwise (see the decompiled body's explicit branch on the
// native getter's return value).
int GM_CDECL gmf_getAutoLoadMode(gmThread* a_thread) {
    const std::string& mode = GetEngineContext().gameSystem.autoLoadMode;
    if (mode.empty()) a_thread->PushNull();
    else a_thread->PushNewString(mode.c_str());
    return GM_OK;
}
int GM_CDECL gmf_getAutoLoadArea(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.autoLoadArea);
    return GM_OK;
}

// Not part of the confirmed GameSystem.c decompile (address-clustering
// filed it under a different cluster) but confirmed real via direct
// retail script usage (game_data/Main.gm's InitGame() save-state machine
// calls GameSystem.trace(...) directly). Kept alongside the 40 confirmed
// functions since real scripts call it on this table.
// ENGINE PATCH (2026-08-17): flood guard. `stderr` is unbuffered by design
// (so trace output survives a crash) -- fine for normal, occasional real
// script traces, but confirmed to cause a genuine ~37-SECOND level-load
// stall once a script-level bug (BaseNotePlant.gm's loop-graphic cell-index
// started calling this thousands of times per second during incremental
// level load. Each call is a real unbuffered write() syscall; at that
// volume the syscall overhead alone dwarfs actual game logic, and from the
// player's side a 37-second black screen is functionally indistinguishable
// from a hang/crash. Rather than chase the exact retail multi-screen
// coordinate math that triggers it (unconfirmed, low-confidence), this
// throttles at the sink: more than 200 calls within any running 1-second
// window get silently dropped (with a single "suppressing" notice) until
// the rate drops back down -- a systemic safety net against ANY future
// runaway trace() burst, not just this one script's bug. Legitimate,
// occasional real traces (the overwhelming majority of real call sites)
// are completely unaffected.
int GM_CDECL gmf_GameSystem_trace(gmThread* a_thread) {
    return GM_OK; // ignore for now.
    
    using clock = std::chrono::steady_clock;
    static clock::time_point windowStart = clock::now();
    static int windowCount = 0;
    static bool suppressing = false;
    constexpr int kMaxPerWindow = 200;

    auto now = clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - windowStart).count() >= 1000) {
        windowStart = now;
        windowCount = 0;
        suppressing = false;
    }
    ++windowCount;
    if (windowCount > kMaxPerWindow) {
        if (!suppressing) {
            std::fprintf(stderr, "[trace] (suppressing further trace() calls this second -- rate exceeded %d/sec)\n", kMaxPerWindow);
            suppressing = true;
        }
        return GM_OK;
    }

    char buffer[256];
    std::fprintf(stderr, "[trace]");
    for (int i = 0; i < a_thread->GetNumParams(); ++i) {
        std::fprintf(stderr, " %s", a_thread->Param(i).AsString(a_thread->GetMachine(), buffer, sizeof(buffer)));
    }
    std::fprintf(stderr, "\n");
    return GM_OK;
}

gmFunctionEntry s_gameSystemLib[] = {
    {"getCurrentTick", gmf_getCurrentTick, nullptr},
    {"isBeingNice", gmf_isBeingNice, nullptr},
    {"setIsBeingNice", gmf_setIsBeingNice, nullptr},
    {"isShouldUpdateEntities", gmf_isShouldUpdateEntities, nullptr},
    {"setIsShouldUpdateEntities", gmf_setIsShouldUpdateEntities, nullptr},
    {"setALevelLoaded", gmf_setALevelLoaded, nullptr},
    {"setIsLoading", gmf_setIsLoading, nullptr},
    {"isLoading", gmf_isLoading, nullptr},
    {"flushProcesses", gmf_flushProcesses, nullptr},
    {"startNewProcessList", gmf_startNewProcessList, nullptr},
    {"runProcess", gmf_runProcess, nullptr},
    {"isProcessRunning", gmf_isProcessRunning, nullptr},
    {"getProcess", gmf_getProcess, nullptr},
    {"getEntityUpdateProcess", gmf_getEntityUpdateProcess, nullptr},
    {"getAllProcesses", gmf_getAllProcesses, nullptr},
    {"getProcessCount", gmf_getProcessCount, nullptr},
    {"getProcessByIndex", gmf_getProcessByIndex, nullptr},
    {"quit", gmf_quit, nullptr},
    {"openConsole", gmf_openConsole, nullptr},
    {"resetSimulationTimer", gmf_resetSimulationTimer, nullptr},
    {"getMSAlive", gmf_getMSAlive, nullptr},
    {"getMSSincePlayStart", gmf_getMSSincePlayStart, nullptr},
    {"getUpdateInterval", gmf_getUpdateInterval, nullptr},
    {"resetPlayTimer", gmf_resetPlayTimer, nullptr},
    {"getRandomInteger", gmf_getRandomInteger, nullptr},
    {"getRandomBoolean", gmf_getRandomBoolean, nullptr},
    {"getRandomDir", gmf_getRandomDir, nullptr},
    {"getRandomAngle", gmf_getRandomAngle, nullptr},
    {"getRandomSign", gmf_getRandomSign, nullptr},
    {"getRandomSignAndReal", gmf_getRandomSignAndReal, nullptr},
    {"getRandomReal", gmf_getRandomReal, nullptr},
    {"getRandomRealExp", gmf_getRandomRealExp, nullptr},
    {"getSimulationSpeed", gmf_getSimulationSpeed, nullptr},
    {"setSimulationSpeed", gmf_setSimulationSpeed, nullptr},
    {"setIsInputEnabled", gmf_setIsInputEnabled, nullptr},
    {"debug_setStop", gmf_debug_setStop, nullptr},
    {"debug_isStopped", gmf_debug_isStopped, nullptr},
    {"TemporaryStorageCopy", gmf_TemporaryStorageCopy, nullptr},
    {"TemporaryStoragePaste", gmf_TemporaryStoragePaste, nullptr},
    {"isAutoLoadLevel", gmf_isAutoLoadLevel, nullptr},
    {"getAutoLoadLevel", gmf_getAutoLoadLevel, nullptr},
    {"getAutoLoadPath", gmf_getAutoLoadPath, nullptr},
    {"getAutoLoadMode", gmf_getAutoLoadMode, nullptr},
    {"getAutoLoadArea", gmf_getAutoLoadArea, nullptr},
    {"trace", gmf_GameSystem_trace, nullptr},
};

// Confirmed real by widespread usage (30+ real .gm files) -- a bare
// global debug-dump utility, not GameSystem-scoped. No decompiled body,
// but every real call site (CampaignScreenAlbumSelect.gm:314 etc.) is a
// bare statement whose return value is never used -- it's a debug print,
// not something script logic branches on. Implemented as a one-level
// key/value dump to stderr, reusing gmf_GameSystem_trace's AsString
// convention; nested tables are named by address rather than recursed
// into, matching a typical debug dumper and avoiding infinite loops on
// self-referential tables (e.g. `DumpObject(GameMonkeyScript)`).
// see strict_calls survey; real call site Main.gm:10703
// `ClearTable(killList);`, a real bare-global 1-arg action) empties a table
// in place, matching gmTableObject::RemoveAndDeleteAll exactly (the real
// decompiled body's single call, FUN_000f24d0, takes the table + machine +
// a 0 flag -- the same 2-arg shape as this real gm API).
int GM_CDECL gmf_ClearTable(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable v = a_thread->Param(0);
    if (v.m_type == GM_TABLE) {
        gmMachine* m = a_thread->GetMachine();
        gmTableObject* t = (gmTableObject*)m->GetObject(v.m_value.m_ref);
        if (t) {
            // RemoveAndDeleteAll is private (gmMachine-only) -- same
            // observable effect (every key reads back null afterward,
            // matching Main.gm:10703's `while (?this.panels[this.counter])`
            // guard pattern used right after a ClearTable call elsewhere in
            // this corpus) via the public Set() API instead: collect keys
            // first since Set() while iterating would invalidate the
            // iterator.
            std::vector<gmVariable> keys;
            gmTableIterator it;
            for (gmTableNode* node = t->GetFirst(it); !t->IsNull(it); node = t->GetNext(it)) {
                keys.push_back(node->m_key);
            }
            for (const gmVariable& key : keys) t->Set(m, key, gmVariable::s_null);
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_DumpObject(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmMachine* m = a_thread->GetMachine();
    gmVariable v = a_thread->Param(0);
    char buffer[256];
    if (v.m_type != GM_TABLE) {
        std::fprintf(stderr, "[DumpObject] %s\n", v.AsString(m, buffer, sizeof(buffer)));
        return GM_OK;
    }
    gmTableObject* t = (gmTableObject*)m->GetObject(v.m_value.m_ref);
    std::fprintf(stderr, "[DumpObject] table:0x%p {\n", (void*)t);
    gmTableIterator it;
    for (gmTableNode* node = t->GetFirst(it); !t->IsNull(it); node = t->GetNext(it)) {
        char keyBuf[128];
        std::fprintf(stderr, "  %s = %s\n",
                     node->m_key.AsString(m, keyBuf, sizeof(keyBuf)),
                     node->m_value.AsString(m, buffer, sizeof(buffer)));
    }
    std::fprintf(stderr, "}\n");
    return GM_OK;
}

// Watermelon_Prepare_Screenshot()/Watermelon_Clear_Prepared_Screenshot() --
// confirmed real, bare global functions (PS3 "Watermelon" grief-report
// subsystem: captures a framebuffer snapshot to attach to a community-level
// abuse report). Live call sites PauseProcessBase.gm:101 ("we may want to
// post a grief report from this menu so let's take a screenshot now"),
// OnlineStartMenu.gm:15/67, CommunityLevelInfoPopupTableBase.gm:89 -- both
// were entirely unbound, hard "getdot failed" aborting PauseProcessBase's
// own init() (see CreateEntityFromFactory-era bugs this session for what
// an aborted init this early does to everything downstream) every single
// time a level was paused. This reimplementation has no grief-report
// backend to attach a screenshot to -- genuine no-op pair, consumed rather
// than left as null-calls.
int GM_CDECL gmf_Watermelon_Prepare_Screenshot(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_Watermelon_Clear_Prepared_Screenshot(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

} // namespace

void RegisterGameSystemBindings(gmMachine& a_machine) {
    a_machine.RegisterLibrary(s_gameSystemLib, sizeof(s_gameSystemLib) / sizeof(s_gameSystemLib[0]), "GameSystem");
    a_machine.RegisterLibraryFunction("DumpObject", gmf_DumpObject);
    a_machine.RegisterLibraryFunction("ClearTable", gmf_ClearTable);
    a_machine.RegisterLibraryFunction("Watermelon_Prepare_Screenshot", gmf_Watermelon_Prepare_Screenshot);
    a_machine.RegisterLibraryFunction("Watermelon_Clear_Prepared_Screenshot", gmf_Watermelon_Clear_Prepared_Screenshot);
}
