#include "gmThread.h"
#include "process_handle_bindings.h"
#include "gmTableObject.h"
#include "gmCall.h"
#include "../core/engine_context.h"
#include "../core/process_manager.h"
#include "../core/strict_calls.h"
#include "../render/draw_list.h"
#include "audio_bindings.h"
#include "foe_compat_bindings.h"
#include "vector2d_bindings.h"
#include <algorithm>
#include <cmath>

namespace {

gmTableObject* ThisHandleTable(gmThread* a_thread) { return a_thread->ThisTable(); }

// Every method re-looks-up the Process by id rather than capturing a
// pointer -- see header comment. Returns nullptr (and raises a GM
// exception) if the process has since gone away.
Process* LookupProcess(gmThread* a_thread, gmTableObject* a_self) {
    gmVariable idVar = a_self->Get(a_thread->GetMachine(), "__id");
    if (idVar.m_type != GM_INT) return nullptr;
    return GetEngineContext().processManager->Get(idVar.m_value.m_int);
}

// callMethod(name, ...args) -- confirmed real (Ghidra: retail FUN_0043e1f0,
// same generic string-dispatch native as the Entity-handle version in
// entity_bindings.cpp -- see its header comment for the full cross-type
// rationale). Real call site: Point.gm:393's
// `GameSystem.getProcess("PekkoAndJon_Main").callMethod("addScore",1)`.
// Simpler than the Entity version -- no __behaviorOwner fallback needed,
// since a process handle IS the same table as its own behavior table here
// (dual injection, see this file's header comment above on why a
// DelegateToProcessTable() attempt caused infinite recursion).
int GM_CDECL gmf_process_callMethod(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    GM_STRING_PARAM(name, 0, "");
    gmCall call;
    if (call.BeginTableFunction(&m, name, self, gmVariable(self))) {
        for (int i = 1; i < a_thread->GetNumParams(); ++i) call.AddParam(a_thread->Param(i));
        call.End();
        a_thread->Push(call.GetReturnedVariable());
    } else {
        a_thread->PushNull();
    }
    return GM_OK;
}

int GM_CDECL gmf_setIsStealsInputFocus(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    proc->stealsInputFocus = (v != 0);
    return GM_OK;
}

int GM_CDECL gmf_setIsUpdatesEntities(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    proc->updatesEntities = (v != 0);
    return GM_OK;
}

// A few authored entity packs query their owning gameplay process while
// configuring (`this.gameProcess.isBeingEdited()`). In a normal play level
// there is no active editor operation, so retail returns false. This is
// distinct from an entity handle's per-entity editing flag.
int GM_CDECL gmf_process_isBeingEdited(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}

// classes/EntityRegistration.c 0x001d0520), real call site Main.gm:4424
// `this.groundProcess.drawGrounds(drawList);` on the GroundProcessNEW
// process handle. A genuine no-op here: ground-piece rendering is already
// done natively and unconditionally every frame by DrawGroundsNative()
// (engine/src/ported_native/ground_renderer.cpp, wired into main.cpp's
// render loop directly, not through any script draw hook) -- see that
// file's header comment on why GroundProcessNEW's own real script-level
// drawGrounds is dead weight ported out to C++ in retail too. Calling the
// real drawList param again here would double-draw every ground piece.
int GM_CDECL gmf_process_drawGrounds(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// Main.gm invokes onMusicTrigger on every process without testing whether
// the process authored a callback. Retail's base process supplies the empty
// callback for ControllerConfig, menus, and the native ground process.
// Script callbacks already present on a returned process table take priority.
int GM_CDECL gmf_process_onMusicTriggerDefault(gmThread*) { return GM_OK; }

// Retail EBOOT also exports refreshFunctionCache on Process. This port's
// ProcessManager resolves callback fields when dispatching each event, so a
// changed method is visible immediately without a separate pointer cache.
int GM_CDECL gmf_process_refreshFunctionCache(gmThread*) { return GM_OK; }

// EntityRegistration.setFireThickness/setIsRefreshDrawListOnChange/
// 0x001d07cc/0x001d06b4/0x001d0700), all called on the GroundProcessNEW
// process handle (GroundProcessNEW.gm's constructor: `this.Process.
// setFireThickness(SW*0.2);`; Main.gm:7320-7321: `this.groundProcess.
// setupDrawLists(); this.groundProcess.setIsRefreshDrawListOnChange(true);`).
// Same "ground rendering is native and unconditional, nothing here to
// configure" situation as drawGrounds above -- genuine no-ops, not
// approximations of missing behavior.
int GM_CDECL gmf_process_groundConfigNoop(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// EntityRegistration.registerEntity(entity,style,selected)/unregisterEntity
// 0x001d0430/0x001d04bc), called on the SAME GroundProcessNEW process
// handle as drawGrounds/setFireThickness above (GroundBase.gm's real
// destructor()/constructor() call sites: `proc.unregisterEntity(this.
// Entity,this.Entity.getStyle());` / `proc.registerEntity(this.Entity,
// this.Entity.getStyle(),selected);`). Same "ground rendering/registration
// is native and unconditional here, nothing to track" situation as
// drawGrounds -- DrawGroundsNative() (ground_renderer.cpp) iterates
// entities directly every frame rather than consulting a registered-ground
// list, so there's no real registry for these to maintain.
int GM_CDECL gmf_process_groundRegistrationNoop(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// classes/EntityRegistration.c 0x001d0920), same GroundProcessNEW handle
// as registerEntity/unregisterEntity above. Real call site (Main.gm's
// getEntityCostPerc(): `local groundCost=...getGroundCountOnScreen(this.
// screenX,this.screenY)*ENTITY_COSTS_COSTPERGROUND;`) feeds a per-screen
// entity-budget/complexity estimate, not gameplay-correctness-critical.
// Same "no real ground registry exists, rendering is native and
// unconditional" situation as registerEntity/unregisterEntity -- 0 grounds
// registered is the honest, consistent answer (matches always returning
// success from those no-op registration calls).
int GM_CDECL gmf_process_getGroundCountOnScreen(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(0);
    return GM_OK;
}

// isLoading() on a process handle -- confirmed real call site (Main.gm
// :1666: `local gp=GameSystem.getProcess(this.parentProcessID); if
// (gp.isLoading()) {...}`, where parentProcessID is Main's OWN process id,
// so `gp` here is Main's own returned table, which never defines
// "isLoading" itself -- only GameSystem.isLoading()/setIsLoading() do (see
// game_system_state.h's isLoadingFlag). Reads the exact same shared flag
// directly (not a cross-table delegate/lookup -- see gmf_setScaleIndex's
// header comment above for why that pattern is unsafe for process tables)
// so this always agrees with whatever GameSystem.setIsLoading(...) last
// set, regardless of which process handle it's read through.
int GM_CDECL gmf_process_isLoading(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isLoadingFlag ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_terminate(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GetEngineContext().processManager->QueueTerminate(proc->id);
    return GM_OK;
}

int GM_CDECL gmf_isInited(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->PushInt(proc->initialized ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_setupPreDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    // 0-arg tolerant: real call sites are 2-arg (`this.setupPreDrawList(w,h)`)
    // EXCEPT the bare "setupDrawList" alias (ScaleOfTheWorld.gm:20's
    // `this.drawList=this.setupDrawList();`, no args at all) that also
    // routes through this same function -- see that registration's own
    // header comment. A strict 2-arg GM_CHECK_NUM_PARAMS threw on every
    // real 0-arg call, aborting ScaleOfTheWorld's own constructor() before
    // it finished (same "blank exception aborts the whole calling chain"
    // class of bug as this session's other GM_CHECK_NUM_PARAMS fixes).
    int w = (int)a_thread->ParamInt(0, 0);
    int h = (int)a_thread->ParamInt(1, 0);
    PushNewDrawList(a_thread, w, h);
    UnrootGmVariable(*a_thread->GetMachine(), proc->preDrawList); // no-op if s_null; see process_manager.h
    proc->preDrawList = *(a_thread->GetTop() - 1); // copy what we just pushed, see header comment
    RootGmVariable(*a_thread->GetMachine(), proc->preDrawList);
    return GM_OK;
}

int GM_CDECL gmf_setupPostDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    // 0-arg tolerant, matching retail: the decompiled body accepts 0..2
    // setupPreDrawList above, which was already fixed for the same reason.
    // A hard GM_CHECK_NUM_PARAMS(2) here throws a blank exception on any
    // real 0- or 1-arg call and aborts the whole calling chain -- the same
    // failure mode that broke ScaleOfTheWorld's constructor via
    // setupDrawList(). This mismatch stayed invisible until check_arity.py
    // learned the Set(...AllocFunctionObject(...)) registration form.
    int w = (int)a_thread->ParamInt(0, 0);
    int h = (int)a_thread->ParamInt(1, 0);
    PushNewDrawList(a_thread, w, h);
    UnrootGmVariable(*a_thread->GetMachine(), proc->postDrawList); // no-op if s_null; see process_manager.h
    proc->postDrawList = *(a_thread->GetTop() - 1);
    RootGmVariable(*a_thread->GetMachine(), proc->postDrawList);
    return GM_OK;
}

int GM_CDECL gmf_getPreDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->preDrawList);
    return GM_OK;
}

int GM_CDECL gmf_getPostDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->postDrawList);
    return GM_OK;
}

// Real usage (Platformer/TouchEditProcessBase.gm, commented out) calls this
// with no further disambiguation between pre/post -- aliased to the pre
// list, matching how the un-suffixed this.setupPreDrawList/drawList is the
// "main" one in every real process that only sets up a single list.
int GM_CDECL gmf_getDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->preDrawList);
    return GM_OK;
}

int GM_CDECL gmf_getObjectID(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->PushInt(proc->id);
    return GM_OK;
}

// MusicTheory.c 0x001a4d28/0x001a4d9c), real call sites all on
// `this.worldScaleProcess` (ScaleOfTheWorld.gm's own returned table).
// NOTE ON WHY THIS ISN'T A CROSS-TABLE DELEGATE: InjectProcessHandleMethods
// is called BOTH on the bare process handle `p` (CreateProcessHandle) AND,
// via InjectProcessBaseMethods (video_bindings.cpp), directly on a
// process's OWN returned behavior table (FinishBringUp, process_manager.cpp)
// -- i.e. for a process, "the handle" and "the behavior table" are dual-
// injected with the SAME method set, unlike entities where a genuinely
// separate __behaviorOwner has to be looked up. A first attempt at a
// DelegateToProcessTable() helper (look up proc->table, forward the call to
// whatever method is defined there) crashed with a stack-overflowing
// infinite recursion: since InjectProcessHandleMethods runs directly on
// proc->table too, "the method defined on proc->table" WAS the delegate
// itself, calling itself forever. Fixed by writing directly to whichever
// table the call landed on (`self`, via ThisTable()) instead of forwarding
// anywhere -- matches the exact field ScaleOfTheWorld.gm's own real
// getScaleIndex/transposeUp read (`this.currentScaleIndex`/
// `this.transposeValue`), and is guarded (only fires if the table doesn't
// already define a real one) so it can never stomp ScaleOfTheWorld.gm's
// own real setScaleIndex when InjectProcessBaseMethods dual-injects onto
// its returned table.
int GM_CDECL gmf_setScaleIndex(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    self->Set(a_thread->GetMachine(), "currentScaleIndex", a_thread->Param(0));
    return GM_OK;
}
int GM_CDECL gmf_setTranspose(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    // MusicTheory.setTranspose (0x001a4d9c) converts a float argument to
    // int before storing the native transpose word at +0xae0.
    const gmVariable value = a_thread->Param(0);
    const gmint transpose = value.m_type == GM_FLOAT
        ? (gmint)value.m_value.m_float : value.m_value.m_int;
    self->Set(a_thread->GetMachine(), "transposeValue", gmVariable(transpose));
    return GM_OK;
}
// MusicTheory.getTranspose (0x001a4e10) returns that word as GM_INT.
// ScaleOfTheWorld.gm keeps the same value in transposeValue and does not
// define its own getter, so supply the native method to both process tables.
int GM_CDECL gmf_getTranspose(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    const gmVariable value = self->Get(a_thread->GetMachine(), "transposeValue");
    const gmint transpose = value.m_type == GM_FLOAT
        ? (gmint)value.m_value.m_float :
          value.m_type == GM_INT ? value.m_value.m_int : 0;
    a_thread->PushInt(transpose);
    return GM_OK;
}
// getScaleIndex() -- native fallback, same guarded shape as setScaleIndex
// above (only fires if the process's own table doesn't already define a
// real one -- ScaleOfTheWorld.gm:247 DOES define its own real
// `getScaleIndex=function(){return this.currentScaleIndex;}`, so this
// should be redundant in principle, but strict survey caught a genuine
// live "unbound: getScaleIndex" (Main.gm:10938: `this.origScaleIndex=
// this.worldScaleProcess.getScaleIndex();`, part of the player-death
// music-rewind flow) -- meaning some real call path reaches this method on
// a table where the script's own version wasn't found. Cheap, safe,
// guarded insurance reading the exact same `currentScaleIndex` field
// either version would.
int GM_CDECL gmf_getScaleIndex(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "currentScaleIndex");
    a_thread->Push((v.m_type == GM_INT || v.m_type == GM_FLOAT) ? v : gmVariable((gmint)0));
    return GM_OK;
}

// MusicTheory.getNoteWithPositionAndOffset (retail 0x001a51fc) accepts a
// vector and optional integer offset. The native helpers at 0x001a3358 and
// 0x001a3454 use a 21.96-unit vertical cycle, 2.44 units per pitch step,
// current scale's note list, mode, and transpose, then clamp to MIDI 0..95.
// Both float constants were read from EBOOT.elf at 0x001a3348/0x001a3350.
// The C++-marked reference in ScaleOfTheWorld.gm confirms the note-list
// indexing and offset-minus-one rule. Scale data itself is the unchanged
// script's scaleList, which its constructor fills before gameplay calls.
int GM_CDECL gmf_getNoteWithPositionAndOffset(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 1 || a_thread->GetNumParams() > 2) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    float x, y;
    if (!GetVector2D(machine, a_thread->Param(0), x, y)) return GM_EXCEPTION;
    const int offset = a_thread->GetNumParams() == 2 ? a_thread->ParamInt(1, 0) : 0;

    gmVariable scalesVar = self->Get(&machine, "scaleList");
    if (scalesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* scales = (gmTableObject*)machine.GetObject(scalesVar.m_value.m_ref);
    auto readInt = [&](gmTableObject* table, const char* key) {
        gmVariable v = table->Get(&machine, key);
        return v.m_type == GM_FLOAT ? (int)v.m_value.m_float
            : v.m_type == GM_INT ? (int)v.m_value.m_int : 0;
    };
    const int scaleIndex = readInt(self, "currentScaleIndex");
    gmVariable scaleVar = scales->Get(scaleIndex);
    if (scaleVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* scale = (gmTableObject*)machine.GetObject(scaleVar.m_value.m_ref);
    gmVariable notesVar = scale->Get(&machine, "noteList");
    if (notesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* notes = (gmTableObject*)machine.GetObject(notesVar.m_value.m_ref);
    int noteCount = 0;
    while (noteCount < 128 && notes->Get(noteCount).m_type != GM_NULL) ++noteCount;
    if (noteCount == 0 || noteCount == 128) return GM_EXCEPTION;

    constexpr float kCycle = 21.96000099182129f;
    constexpr float kStep = 2.440000057220459f;
    float phase = std::fmod(y, kCycle);
    if (phase < 0.0f) phase += kCycle;
    int absoluteIndex = (int)(phase / kStep) + offset - 1;
    if (absoluteIndex < 0) absoluteIndex = 0;
    const int octaveQuotient = absoluteIndex / noteCount;
    const int baseIndex = absoluteIndex % noteCount;
    const int mode = readInt(self, "modeIndex");
    int index, octaveShift;
    if (mode >= 0) {
        index = (baseIndex + mode % noteCount) % noteCount;
        octaveShift = 2 * (octaveQuotient + mode / noteCount);
    } else {
        const int positiveMode = -mode;
        index = (baseIndex + noteCount - positiveMode % noteCount) % noteCount;
        octaveShift = -(2 * (octaveQuotient + positiveMode / noteCount) + 12);
    }
    gmVariable noteVar = notes->Get(index);
    if (noteVar.m_type != GM_INT && noteVar.m_type != GM_FLOAT) return GM_EXCEPTION;
    const int baseNote = noteVar.m_type == GM_INT ? noteVar.m_value.m_int : (int)noteVar.m_value.m_float;
    const int transpose = readInt(self, "transposeValue");
    const int note = std::max(0, std::min(95, baseNote + transpose + octaveShift));
    a_thread->PushInt(note);
    return GM_OK;
}

int GM_CDECL gmf_getTable(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->table);
    return GM_OK;
}

// getScreenWidth/getScreenHeight -- confirmed real (Platformer/Main.gm
// calls `this.getScreenWidth()`/`this.getScreenHeight()` directly on its
// own process table, e.g. `t.height=this.getScreenHeight();` while setting
// up the tutorial level's boundary walls) but Main.gm's OWN script-side
// definitions for them (same file, ~line 2351) are dead code sitting
// inside a `/* C++ ... */` comment block -- the comment is retail's own
// note that these got moved to a native engine implementation. video_bindings.cpp
// already exposes the equivalent as `Video.getScreenWidth/getScreenHeight`;
// these thin wrappers put the same values on every process handle/table so
// `this.getScreenWidth()` resolves the way retail's native version did,
// instead of silently no-op'ing to null via GM's null-call leniency (which
// is exactly what was happening before: `t.height=this.getScreenHeight()`
// evaluated to null, then crashed several lines later at
// `sd.addBox(t.width,t.height,...)` since a native float param can't be a
// null argument).
// raw PIXEL dimensions (1280x720-ish), reusing Video.getScreenWidth/Height
// directly -- wrong for this call. "screen" here means one of Sound
// Shapes' logical level-grid cells, sized in WORLD units (the same units
// entity positions/physics use), not literal window pixels. The
// commented-out original script version confirms the formula
// (Platformer/Main.gm ~line 2351: `return this.getNumCellsX()*SW*2*2;`) --
// getNumCellsX()/getNumCellsY() are the confirmed 16/9 logical grid
// (video_bindings.cpp), SW is the confirmed 0.61 real constant
// (constants_bindings.cpp). Getting this wrong matters a lot more than it
// looks: `initForReal()`'s `Video.setCameraPos(this.getScreenWidth()/2,
// this.getScreenHeight()/2, ...)` was placing the camera at world
// (640,360) -- pixel-scale, nowhere near the real level geometry
// (typically within +/-100 world units of the origin) -- so nothing was
// ever in view even once entity/level setup finished with zero errors.
int GM_CDECL gmf_getScreenWidth(gmThread* a_thread) {
    a_thread->PushFloat(16.0f * 0.61f * 2.0f * 2.0f);
    return GM_OK;
}
int GM_CDECL gmf_getScreenHeight(gmThread* a_thread) {
    a_thread->PushFloat(9.0f * 0.61f * 2.0f * 2.0f);
    return GM_OK;
}

// getScreenX(x) / getScreenY(y) -- "which screen index does this world
// coordinate fall on". Same "dead script code in a /* C++ */ comment, moved
// to a native implementation" situation as getScreenWidth/getScreenHeight
// above (Platformer/Main.gm 5856-5873 -- the commented-out reference reads
//
//     getScreenX=function(x) {
//         local sx=MATH_INT(x/this.getScreenWidth());
//         if (x<0) { sx-=1; }
//         return sx;
//     },
//
// ) and these were the last two of that family still unimplemented.
//
// 0x0019f478 / 0x0019f5a0), which computes
//
//     iVar4 = (int)(x / ((float)field[0xd4] * DAT_0019f468
//                        * DAT_0019f46c * DAT_0019f46c));
//     if (x < DAT_0019f470) iVar4 = iVar4 + -1;
//
// with those three constants reading 0.61f, 2.0f and 0.0f out of EBOOT.elf
// -- i.e. exactly `numCells * SW * 2 * 2` as the divisor (the same
// expression getScreenWidth uses) and a compare against 0.0 for the
// negative adjustment. The int cast truncates toward zero, so subtracting
// one for negative inputs turns it into a floor -- screen -1 is the screen
// immediately left of the origin, not a second copy of screen 0.
//
// Real call site: Main.gm's per-frame "did we change screens?" check
// (`cx=this.getScreenX(ppos.x); cy=this.getScreenY(ppos.y);`), which drives
// the camera's screen-to-screen panning during play.
int GM_CDECL gmf_getScreenX(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    const float screenWidth = 16.0f * 0.61f * 2.0f * 2.0f;
    int sx = (int)(x / screenWidth);
    if (x < 0.0f) sx -= 1;
    a_thread->PushInt(sx);
    return GM_OK;
}
int GM_CDECL gmf_getScreenY(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(y, 0, 0.0f);
    const float screenHeight = 9.0f * 0.61f * 2.0f * 2.0f;
    int sy = (int)(y / screenHeight);
    if (y < 0.0f) sy -= 1;
    a_thread->PushInt(sy);
    return GM_OK;
}

// getCurrentScreenX/getCurrentScreenY -- same "dead script code in a /*
// C++ */ comment, moved to a native implementation" situation as
// getScreenWidth/getScreenHeight above, a few thousand lines further into
// the same file (Platformer/Main.gm ~4213: `getCurrentScreenX=function() {
// return this.currentScreenX; }` commented out). This crashed
// Platformer/Main.gm's own getScreenData(csx,csy) (line 6665's `if
// (!?csy) { csy=this.getCurrentScreenY(); }` guard, meant to fill in a
// missing csy, silently no-op'd instead since the call resolved to
// nothing) -- csy stayed null all the way to
// `this.screenData[csx][csy]`, a null table index, which is a hard error
// unlike null property access. `this.currentScreenX`/`currentScreenY` are
// real plain fields on the same table (confirmed:
// `currentScreenX=0,currentScreenY=0,` in the table literal, ~line 1887),
// so this just reads them back the same way the commented-out script
// version did.
int GM_CDECL gmf_getCurrentScreenX(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "currentScreenX");
    a_thread->Push((v.m_type == GM_INT || v.m_type == GM_FLOAT) ? v : gmVariable((gmint)0));
    return GM_OK;
}
int GM_CDECL gmf_getCurrentScreenY(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "currentScreenY");
    a_thread->Push((v.m_type == GM_INT || v.m_type == GM_FLOAT) ? v : gmVariable((gmint)0));
    return GM_OK;
}

// setCurrentScreenX/setCurrentScreenY -- the setters matching the two
// getters above, and the same "dead script code in a /* C++ */ comment,
// moved native" family as getScreenWidth/getScreenX (Platformer/Main.gm's
// own commented reference stores straight into this.currentScreenX).
//
// Real call site is Main.gm's per-frame screen-change handler, immediately
// after the getScreenX/getScreenY pair that computes the new indices:
//
//     this.setCurrentScreenX(cx);
//     this.setCurrentScreenY(cy);
//     this.destroyBullets();
//
// so leaving these unbound meant the game recomputed which screen the
// player had moved to and then never recorded it -- getCurrentScreenX/Y
// kept reporting the old screen forever, and every "did we change screens?"
// comparison downstream stayed permanently true.
//
// Stored on the process's own script table under the same key the getters
// read, so the two sides cannot drift apart.
int GM_CDECL gmf_setCurrentScreenX(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    self->Set(a_thread->GetMachine(), "currentScreenX", a_thread->Param(0));
    return GM_OK;
}
int GM_CDECL gmf_setCurrentScreenY(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    self->Set(a_thread->GetMachine(), "currentScreenY", a_thread->Param(0));
    return GM_OK;
}

// setGlobalVolume -- confirmed real call site (PauseProcessBase.gm:
// `this.gameProcess.setGlobalVolume(gv)`, called every update() while the
// pause menu slides in/out); not otherwise found in the corpus with a
// disambiguated implementation, so it delegates to the same master-volume
// state Audio.setMasterVolume/getMasterVolume already track (see
// audio_bindings.h).
int GM_CDECL gmf_setGlobalVolume(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(vol, 0, 1.0f);
    SetMasterVolumeNative(vol);
    return GM_OK;
}

// setAsPauseProcess -- confirmed process method at 0x0019f850 (misclassified
// wrapper at 0x00070e48. The native queue operation is the mechanism that
// pause/menu/popup overlays in the corpus use to freeze gameplay
// (PauseProcessBase.gm, PopupProcessBase.gm, ResizeScreenProcess.gm,
// DebugMenu*.gm, TrophyWinProcess.gm, WinProcess.gm, ...). Three call shapes
// found across the corpus, all handled here:
//   1. `someProcess.setAsPauseProcess(true)` -- self-target: ThisTable() IS
//      the process to exempt from the freeze (TrophyWinProcess.gm:192).
//   2. `GameSystem.setAsPauseProcess(active, process)` -- explicit target,
//      the overwhelmingly common form (PauseProcessBase.gm and friends).
//   3. `GameSystem.setAsPauseProcess(true)` -- no target at all
//      (WinProcess.gm's win-screen). Every real call site that reaches this
//      form also calls setIsStealsInputFocus(true) on itself first (see
//      WinProcess.gm's constructor), so falling back to whichever process
//      currently holds input focus recovers the same target without this
//      engine needing to track "the calling process" generically.
// See ProcessManager::QueuePauseState for what "exempt" actually does: every
// OTHER process's update()/postUpdate() stops running (draw is untouched --
// the frozen world stays visible behind the overlay).
int GM_CDECL gmf_setAsPauseProcess(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(active, 0, 0);

    ProcessManager& pm = *GetEngineContext().processManager;
    int exemptId = -1;

    gmTableObject* self = ThisHandleTable(a_thread);
    Process* selfProc = self ? LookupProcess(a_thread, self) : nullptr;
    if (selfProc) {
        exemptId = selfProc->id;
    } else if (a_thread->GetNumParams() >= 2) {
        gmVariable procVar = a_thread->Param(1);
        if (procVar.m_type == GM_TABLE) {
            gmTableObject* procTable = (gmTableObject*)a_thread->GetMachine()->GetObject(procVar.m_value.m_ref);
            gmVariable idVar = procTable ? procTable->Get(a_thread->GetMachine(), "__id") : gmVariable(GM_NULL, 0);
            if (idVar.m_type == GM_INT) exemptId = idVar.m_value.m_int;
        }
    } else {
        for (int i = 0; i < pm.GetCount(); ++i) {
            Process* p = pm.GetByIndex(i);
            if (p && p->stealsInputFocus) { exemptId = p->id; break; }
        }
    }

    pm.QueuePauseState(active != 0, exemptId);
    return GM_OK;
}

} // namespace

void InjectProcessHandleMethods(gmMachine& a_machine, gmTableObject* a_table, int a_processId) {
    a_table->Set(&a_machine, "__id", gmVariable((gmint)a_processId));
    a_table->Set(&a_machine, "refreshFunctionCache",
        gmVariable(a_machine.AllocFunctionObject(gmf_process_refreshFunctionCache)));
    if (a_table->Get(&a_machine, "onMusicTrigger").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "onMusicTrigger",
            gmVariable(a_machine.AllocFunctionObject(gmf_process_onMusicTriggerDefault)));
    }
    a_table->Set(&a_machine, "setIsStealsInputFocus", gmVariable(a_machine.AllocFunctionObject(gmf_setIsStealsInputFocus)));
    a_table->Set(&a_machine, "terminate", gmVariable(a_machine.AllocFunctionObject(gmf_terminate)));
    a_table->Set(&a_machine, "callMethod", gmVariable(a_machine.AllocFunctionObject(gmf_process_callMethod)));
    a_table->Set(&a_machine, "setIsUpdatesEntities", gmVariable(a_machine.AllocFunctionObject(gmf_setIsUpdatesEntities)));
    a_table->Set(&a_machine, "drawGrounds", gmVariable(a_machine.AllocFunctionObject(gmf_process_drawGrounds)));
    a_table->Set(&a_machine, "registerEntity", gmVariable(a_machine.AllocFunctionObject(gmf_process_groundRegistrationNoop)));
    a_table->Set(&a_machine, "unregisterEntity", gmVariable(a_machine.AllocFunctionObject(gmf_process_groundRegistrationNoop)));
    a_table->Set(&a_machine, "getGroundCountOnScreen", gmVariable(a_machine.AllocFunctionObject(gmf_process_getGroundCountOnScreen)));
    if (a_table->Get(&a_machine, "isLoading").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "isLoading", gmVariable(a_machine.AllocFunctionObject(gmf_process_isLoading)));
    }
    a_table->Set(&a_machine, "setFireThickness", gmVariable(a_machine.AllocFunctionObject(gmf_process_groundConfigNoop)));
    a_table->Set(&a_machine, "setIsRefreshDrawListOnChange", gmVariable(a_machine.AllocFunctionObject(gmf_process_groundConfigNoop)));
    a_table->Set(&a_machine, "setupDrawLists", gmVariable(a_machine.AllocFunctionObject(gmf_process_groundConfigNoop)));
    // Guarded: never stomp ScaleOfTheWorld.gm's own real setScaleIndex when
    // dual-injected onto its returned table -- see gmf_setScaleIndex's
    // header comment above.
    if (a_table->Get(&a_machine, "setScaleIndex").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "setScaleIndex", gmVariable(a_machine.AllocFunctionObject(gmf_setScaleIndex)));
    }
    if (a_table->Get(&a_machine, "getScaleIndex").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "getScaleIndex", gmVariable(a_machine.AllocFunctionObject(gmf_getScaleIndex)));
    }
    if (a_table->Get(&a_machine, "getNoteWithPositionAndOffset").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "getNoteWithPositionAndOffset",
                     gmVariable(a_machine.AllocFunctionObject(gmf_getNoteWithPositionAndOffset)));
    }
    if (a_table->Get(&a_machine, "setTranspose").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "setTranspose", gmVariable(a_machine.AllocFunctionObject(gmf_setTranspose)));
    }
    if (a_table->Get(&a_machine, "getTranspose").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "getTranspose", gmVariable(a_machine.AllocFunctionObject(gmf_getTranspose)));
    }
    a_table->Set(&a_machine, "isInited", gmVariable(a_machine.AllocFunctionObject(gmf_isInited)));
    a_table->Set(&a_machine, "setupPreDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_setupPreDrawList)));
    a_table->Set(&a_machine, "setupPostDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_setupPostDrawList)));
    // setupDrawList -- confirmed real (ScaleOfTheWorld.gm:20's
    // `this.drawList=this.setupDrawList();`), same un-disambiguated-alias
    // convention as getDrawList aliasing getPreDrawList above.
    a_table->Set(&a_machine, "setupDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_setupPreDrawList)));
    a_table->Set(&a_machine, "getPreDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_getPreDrawList)));
    a_table->Set(&a_machine, "getPostDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_getPostDrawList)));
    a_table->Set(&a_machine, "getDrawList", gmVariable(a_machine.AllocFunctionObject(gmf_getDrawList)));
    a_table->Set(&a_machine, "getObjectID", gmVariable(a_machine.AllocFunctionObject(gmf_getObjectID)));
    a_table->Set(&a_machine, "getTable", gmVariable(a_machine.AllocFunctionObject(gmf_getTable)));
    a_table->Set(&a_machine, "getScreenWidth", gmVariable(a_machine.AllocFunctionObject(gmf_getScreenWidth)));
    a_table->Set(&a_machine, "getScreenHeight", gmVariable(a_machine.AllocFunctionObject(gmf_getScreenHeight)));
    a_table->Set(&a_machine, "getScreenX", gmVariable(a_machine.AllocFunctionObject(gmf_getScreenX)));
    a_table->Set(&a_machine, "getScreenY", gmVariable(a_machine.AllocFunctionObject(gmf_getScreenY)));
    a_table->Set(&a_machine, "setCurrentScreenX", gmVariable(a_machine.AllocFunctionObject(gmf_setCurrentScreenX)));
    a_table->Set(&a_machine, "setCurrentScreenY", gmVariable(a_machine.AllocFunctionObject(gmf_setCurrentScreenY)));
    a_table->Set(&a_machine, "getCurrentScreenX", gmVariable(a_machine.AllocFunctionObject(gmf_getCurrentScreenX)));
    a_table->Set(&a_machine, "getCurrentScreenY", gmVariable(a_machine.AllocFunctionObject(gmf_getCurrentScreenY)));
    a_table->Set(&a_machine, "setGlobalVolume", gmVariable(a_machine.AllocFunctionObject(gmf_setGlobalVolume)));
    a_table->Set(&a_machine, "setAsPauseProcess", gmVariable(a_machine.AllocFunctionObject(gmf_setAsPauseProcess)));
    a_table->Set(&a_machine, "isBeingEdited", gmVariable(a_machine.AllocFunctionObject(gmf_process_isBeingEdited)));

    // FoE compatibility methods (alarm/LFO management) -- injected from foe_compat_bindings
    a_table->Set(&a_machine, "getUniqueAlarmID", gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueAlarmID)));
    a_table->Set(&a_machine, "addAlarm", gmVariable(a_machine.AllocFunctionObject(gmf_addAlarm)));
    a_table->Set(&a_machine, "deleteAlarm", gmVariable(a_machine.AllocFunctionObject(gmf_deleteAlarm)));
    a_table->Set(&a_machine, "getAlarm", gmVariable(a_machine.AllocFunctionObject(gmf_getAlarm)));
    a_table->Set(&a_machine, "getUniqueLFOID", gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueLFOID)));
    a_table->Set(&a_machine, "addLFO", gmVariable(a_machine.AllocFunctionObject(gmf_addLFO)));
    a_table->Set(&a_machine, "deleteLFO", gmVariable(a_machine.AllocFunctionObject(gmf_deleteLFO)));
}

void RegisterProcessHandleBindings(gmMachine& a_machine) {
    // GameSystem.setAsPauseProcess(active, process) -- see gmf_setAsPauseProcess's
    // header comment. GameSystem itself is registered elsewhere
    // (gamesystem_bindings.cpp); appended to the existing table here
    // (RegisterLibraryFunction's a_newTable defaults false, so this reuses
    // the table RegisterGameSystemBindings already created).
    a_machine.RegisterLibraryFunction("setAsPauseProcess", gmf_setAsPauseProcess, "GameSystem");
}

gmVariable CreateProcessHandle(gmMachine& a_machine, int a_processId) {
    gmTableObject* table = a_machine.AllocTableObject();
    InjectProcessHandleMethods(a_machine, table, a_processId);
    // ENGINE PATCH: this handle is entirely C++-built (unlike a process's own
    // returned behavior table, which is script-authored and must stay
    // unmarked -- see strict_calls.h's MarkNative), so a missing method here
    // is unambiguously an engine gap.
    StrictCalls::MarkScriptExtensibleNative(a_machine, table);
    return gmVariable(table);
}
