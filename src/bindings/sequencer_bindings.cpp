#include "gmThread.h"
#include "sequencer_bindings.h"
#include "gmCall.h"
#include "gmTableObject.h"
#include "../core/process_manager.h" // RootGmVariable/UnrootGmVariable
#include "../core/strict_calls.h"
#include <vector>
#include <cmath>
#include <cstdio>

namespace {

gmType g_sequencerType = -1;

struct SequencerTrigger {
    int step = 0;
    gmVariable action = gmVariable::s_null; // whatever was passed to setTrigger -- commonly a SEQUENCERACTION table
    // SequencerAction.c) -- NOT a fire/don't-fire mask (that was this
    // engine's previous, unconfirmed "enabled" reading). Every real call
    // site is consistent with "does this trigger keep firing every time
    // the pattern loops back to its step (true), or fire once and then
    // remove itself (false)": SequencerHelper.gm passes false for one-shot
    // sync/volume-fade actions and true for repeating rhythm-pattern
    // triggers. The exact retail removal mechanics aren't decompiled (no
    // sequencer-tick body in the corpus), so "fire once, then erase" is
    // our best-supported reading, not a confirmed decompiled port.
    bool looped = true;
    // Confirmed real: setTrigger/setTrigger16/setTriggerOnNext return a
    // native trigger-id result via a SEQUENCERACTION-typed object), and
    // getAllTriggers()/moveTriggerToFront operate on that same handle, not
    // a bare index -- see CreateSequencerActionTable.
    gmVariable handle = gmVariable::s_null;
};

// Confirmed direct-offset fields from the decompiled body (currentStep at
// +0x28, currentPattern at +0x24 -- both read/written with no function call
// at all, unlike every other property here) map onto currentStep/
// currentPattern below. Timing (msPerStep etc) is real BPM-driven math, not
// a stub -- see UpdateAllSequencers.
struct SequencerData {
    int bpm = 120;               // getBPM/setBPM confirmed real int getter/setter
    int numStepsPerPattern = 16; // getNumStepsPerPattern/setNumStepsPerPattern confirmed real int pair
    int loopLen = 0;             // setLoopLen confirmed real setter, 0 = "use numStepsPerPattern" (no confirmed getter)
    int currentStep = 0;         // confirmed direct-offset field (+0x28)
    int currentPattern = 0;      // confirmed direct-offset field (+0x24)
    float volume = 1.0f;         // setVolume confirmed real setter (no confirmed getter)
    float pan = 0.0f;            // setPan confirmed real setter (no confirmed getter)
    bool isPlaying = false;      // start/stop confirmed real actions; isPlaying is real usage (Main.gm) but not
                                  // in the confirmed decompiled method list -- see header comment on "play"/"isPlaying"
    std::vector<SequencerTrigger> triggers;
    float msAccumulator = 0.0f;  // ms accumulated toward the next step, drives real playback

    int EffectiveLen() const { return loopLen > 0 ? loopLen : (numStepsPerPattern > 0 ? numStepsPerPattern : 16); }
    // Assumes 4 steps per beat (16th-note steps) -- a common step-sequencer
    // convention, not a confirmed retail constant (the real subdivision
    // isn't recoverable from the decompiled body alone).
    float MsPerStep() const { return bpm > 0 ? (60000.0f / (float)bpm / 4.0f) : 0.0f; }
};

std::vector<SequencerData*>& AllSequencers() {
    static std::vector<SequencerData*> all;
    return all;
}
bool g_allPaused = false;         // SEQUENCER_PauseAllSequencers/UnpauseAllSequencers
float g_masterPulse = 0.0f;       // SEQUENCER_SetMasterPulse

int WrapStep(int a_step, int a_len) {
    if (a_len <= 0) return 0;
    int w = a_step % a_len;
    if (w < 0) w += a_len;
    return w;
}

void FireTrigger(gmMachine& a_machine, const gmVariable& a_action) {
    if (a_action.m_type != GM_TABLE) return;
    gmTableObject* table = (gmTableObject*)a_machine.GetObject(a_action.m_value.m_ref);
    gmCall call;
    // Matches real usage exactly (game_data/Main.gm): act.execute=function(){...};
    if (call.BeginTableFunction(&a_machine, "execute", table, a_action)) {
        call.End();
    }
}

SequencerData* ThisSequencer(gmThread* a_thread) {
    return (SequencerData*)a_thread->ThisUserCheckType(g_sequencerType);
}

// Forward declaration -- defined alongside the rest of the SEQUENCERACTION
// section below, but needed here by setTrigger/setTriggerOnNext.
gmTableObject* CreateSequencerActionTable(gmMachine& a_machine, int a_initialStep);

bool IsHandleTagged(gmMachine& a_machine, const gmVariable& a_handle) {
    if (a_handle.m_type != GM_TABLE) return false;
    gmTableObject* t = (gmTableObject*)a_machine.GetObject(a_handle.m_value.m_ref);
    if (!t) return false;
    gmVariable v = t->Get(&a_machine, "__tagged");
    return v.m_type == GM_INT && v.m_value.m_int != 0;
}

// ---------------------------------------------------------------------
// SEQUENCER
// ---------------------------------------------------------------------

// Confirmed real: 0 or 1 args. The 1-arg form's exact meaning isn't
// recoverable from the decompiled body (it's stashed at a fixed offset,
// no further use visible in this function), but an initial-BPM reading is
// the most useful/plausible fit given every other confirmed field on this
// type is playback/timing related.
int GM_CDECL gmf_SEQUENCER(gmThread* a_thread) {
    int initial = a_thread->GetNumParams() >= 1 ? a_thread->ParamInt(0, 120) : 120;
    a_thread->Push(CreateSequencerInstance(*a_thread->GetMachine(), initial));
    return GM_OK;
}

int GM_CDECL gmf_getNumStepsPerPattern(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->numStepsPerPattern);
    return GM_OK;
}
int GM_CDECL gmf_setNumStepsPerPattern(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(n, 0, 16);
    s->numStepsPerPattern = n;
    return GM_OK;
}
int GM_CDECL gmf_getBPM(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->bpm);
    return GM_OK;
}
int GM_CDECL gmf_setLoopLen(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(n, 0, 0);
    s->loopLen = n;
    return GM_OK;
}
int GM_CDECL gmf_setCurrentStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(n, 0, 0);
    s->currentStep = n; // confirmed real: direct field write, no wrapping/side effects observed
    return GM_OK;
}
int GM_CDECL gmf_setVolume(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    s->volume = v;
    return GM_OK;
}
int GM_CDECL gmf_setPan(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    s->pan = v;
    return GM_OK;
}
// Confirmed real: 1 or 2 args (bpm, [flag]) -- flag's purpose (immediate vs.
// smoothed transition is the plausible guess) isn't recoverable, read and
// ignored.
int GM_CDECL gmf_setBPM(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    // GM_FLOAT_OR_INT_PARAM, not GM_INT_PARAM: real callers pass a value
    // straight from level XML/JSON data (Platformer/ScaleOfTheWorld.gm's
    // setMinBPM -> MainSequencer.setBPM(this.minBPM), fed by
    // levelData.bpm), and every XML/JSON leaf number parses to a GM_FLOAT
    // (gamemonkeyscript_bindings.cpp's ParseLeafValue/JsonToGmVariable) --
    // GM_INT_PARAM's strict GM_INT-only check silently threw here (blank
    // exception, no message) for the exact real value 100.0.
    GM_FLOAT_OR_INT_PARAM(bpm, 0, 120.0f);
    s->bpm = (int)bpm;
    return GM_OK;
}
// Confirmed real: 0 or 1 args (restart from the top, or restart at a given step).
int GM_CDECL gmf_restart(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    int step = a_thread->GetNumParams() >= 1 ? a_thread->ParamInt(0, 0) : 0;
    s->currentStep = WrapStep(step, s->EffectiveLen());
    s->msAccumulator = 0.0f;
    s->isPlaying = true;
    return GM_OK;
}
int GM_CDECL gmf_start(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    s->isPlaying = true;
    return GM_OK;
}
int GM_CDECL gmf_stop(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    s->isPlaying = false;
    return GM_OK;
}
// Real usage (game_data/Main.gm: `MainSequencer.isPlaying()`,
// `MainSequencer.play()`) -- not in the confirmed decompiled method list
// so these fill a real gap in coverage rather than replacing a confirmed
// function. "play" aliases start(); isPlaying() reports our own state flag.
int GM_CDECL gmf_play(gmThread* a_thread) { return gmf_start(a_thread); }
int GM_CDECL gmf_isPlaying(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->isPlaying ? 1 : 0);
    return GM_OK;
}

// Confirmed real: 1 arg (a note-duration id). NoteTable.gm's real
// subdivision scheme isn't consulted here (out of scope for this pass) --
// previously approximated as halving the pattern length per increment of
// noteType (0=whole pattern, 1=half, 2=quarter, ...), a standard
// power-of-two note tree documented as an approximation, not the
// confirmed mapping.
GE3_REMOVED(gmf_getNumStepsPerNote, "SEQUENCER.getNumStepsPerNote")
// Depends on the same unconfirmed noteType->step-subdivision approximation
// as getNumStepsPerNote above.
GE3_REMOVED(gmf_getNoteTime, "SEQUENCER.getNoteTime")
int GM_CDECL gmf_getMSPerStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushFloat(s->MsPerStep());
    return GM_OK;
}

// or 4 args. The arg-count/type dispatch itself is confirmed from the
// decompiled body:
//   2 args            -> (step, action), looped defaults true
//   3 args, arg1 is int -> (step, pattern, action), looped defaults true
//   3 args, arg1 not int -> (step, action, looped)
//   4 args            -> (step, pattern, action, looped) unconditionally
//                         (no type check on arg1 for the 4-arg form)
// Only the (step, action, looped) 3-arg form is exercised anywhere in the
// real script corpus (confirmed by grep: every real `.setTrigger(` call
// site uses exactly 3 args, step+action+bool) -- the pattern-indexed forms
// are accepted and the pattern value is recorded (getPattern() on the
// returned handle reflects it) but nothing in this reimplementation's
// firing logic gates on it (no decompiled sequencer-tick body to confirm
// what pattern-scoping should do).
int GM_CDECL gmf_setTrigger(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    int n = a_thread->GetNumParams();
    if (n < 2 || n > 4) { GM_EXCEPTION_MSG("setTrigger: expecting 2-4 params"); return GM_EXCEPTION; }
    GM_INT_PARAM(step, 0, 0);
    gmVariable action;
    int pattern = 0;
    bool looped = true;
    if (n == 2) {
        action = a_thread->Param(1);
    } else if (n == 3 && a_thread->Param(1).m_type == GM_INT) {
        // (step, pattern, action) -- looped defaults true.
        pattern = a_thread->Param(1).m_value.m_int;
        action = a_thread->Param(2);
    } else if (n == 3) {
        // (step, action, looped)
        action = a_thread->Param(1);
        GM_INT_PARAM(lp, 2, 1);
        looped = (lp != 0);
    } else {
        // n == 4: (step, pattern, action, looped) -- always this shape,
        // regardless of arg1's type (confirmed: the decompiled 4-arg branch
        // has no type check, unlike the 3-arg branch).
        GM_INT_PARAM(pat, 1, 0);
        pattern = pat;
        action = a_thread->Param(2);
        GM_INT_PARAM(lp, 3, 1);
        looped = (lp != 0);
    }

    int wrappedStep = WrapStep(step, s->EffectiveLen());
    gmTableObject* handleTable = CreateSequencerActionTable(m, wrappedStep);
    handleTable->Set(&m, "__pattern", gmVariable((gmint)pattern));
    handleTable->Set(&m, "__looped", gmVariable((gmint)(looped ? 1 : 0)));
    handleTable->Set(&m, "__action", action);
    gmVariable handleVar(handleTable);

    SequencerTrigger t;
    t.step = wrappedStep;
    t.action = action;
    t.looped = looped;
    t.handle = handleVar;
    RootGmVariable(m, t.action);  // see process_manager.h
    RootGmVariable(m, t.handle);
    s->triggers.push_back(t);
    // Confirmed real: setTrigger returns a SEQUENCERACTION-shaped handle,
    // not a bare index (previously PushInt'd our own list index).
    a_thread->Push(handleVar);
    return GM_OK;
}
// setTrigger16: confirmed real, same overload shape as setTrigger. The
// "16" resolution distinction isn't modeled (see getCurrentStep16 note) --
// delegates to the same trigger list/step-space as setTrigger.
int GM_CDECL gmf_setTrigger16(gmThread* a_thread) { return gmf_setTrigger(a_thread); }

// Confirmed real: 2 or 3 args (step, action, [looped]) -- unlike setTrigger,
// no pattern-indexed overload exists for this one (the decompiled 3-arg
// branch is unconditional, no int-vs-callback type check on arg1).
int GM_CDECL gmf_setTriggerOnNext(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    int n = a_thread->GetNumParams();
    if (n < 2 || n > 3) { GM_EXCEPTION_MSG("setTriggerOnNext: expecting 2-3 params"); return GM_EXCEPTION; }
    GM_INT_PARAM(step, 0, 0);
    gmVariable action = a_thread->Param(1);
    bool looped = true;
    if (n == 3) { GM_INT_PARAM(lp, 2, 1); looped = (lp != 0); }

    int wrappedStep = WrapStep(step, s->EffectiveLen());
    gmTableObject* handleTable = CreateSequencerActionTable(m, wrappedStep);
    handleTable->Set(&m, "__looped", gmVariable((gmint)(looped ? 1 : 0)));
    handleTable->Set(&m, "__action", action);
    gmVariable handleVar(handleTable);

    SequencerTrigger t;
    t.step = wrappedStep;
    t.action = action;
    t.looped = looped;
    t.handle = handleVar;
    RootGmVariable(m, t.action); // see process_manager.h
    RootGmVariable(m, t.handle);
    s->triggers.insert(s->triggers.begin(), t); // "OnNext": front of the queue, see moveTriggerToFront below
    a_thread->Push(handleVar); // confirmed real: same SEQUENCERACTION-shaped handle as setTrigger
    return GM_OK;
}
// Confirmed real: 1 arg, a trigger handle -- the decompiled body
// dereferences it as a user object (`*(*(arg0)+0x14)`), i.e. the
// SEQUENCERACTION-shaped handle setTrigger/setTrigger16/setTriggerOnNext
// returned, not a bare integer index (this engine's previous reading).
int GM_CDECL gmf_moveTriggerToFront(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmVariable handle = a_thread->Param(0);
    if (handle.m_type == GM_TABLE) {
        for (size_t i = 0; i < s->triggers.size(); ++i) {
            if (s->triggers[i].handle.m_type == GM_TABLE &&
                s->triggers[i].handle.m_value.m_ref == handle.m_value.m_ref) {
                SequencerTrigger t = s->triggers[i];
                s->triggers.erase(s->triggers.begin() + i);
                s->triggers.insert(s->triggers.begin(), t);
                break;
            }
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_clearTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    for (auto& t : s->triggers) {
        UnrootGmVariable(*a_thread->GetMachine(), t.action);
        UnrootGmVariable(*a_thread->GetMachine(), t.handle);
    }
    s->triggers.clear();
    return GM_OK;
}
int GM_CDECL gmf_getNumTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt((int)s->triggers.size());
    return GM_OK;
}
int GM_CDECL gmf_getWrappedStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(step, 0, 0);
    a_thread->PushInt(WrapStep(step, s->EffectiveLen()));
    return GM_OK;
}
// Confirmed real: 1 or 2 args (step, [flag]). Flag's plausible meaning
// (reset the ms-accumulator vs. keep phase) -- implemented as "reset phase"
// since that's the only observable state we have to apply it to.
int GM_CDECL gmf_gotoStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(step, 0, 0);
    bool resetPhase = true;
    if (a_thread->GetNumParams() >= 2) { GM_INT_PARAM(f, 1, 1); resetPhase = (f != 0); }
    s->currentStep = WrapStep(step, s->EffectiveLen());
    if (resetPhase) s->msAccumulator = 0.0f;
    return GM_OK;
}
int GM_CDECL gmf_getCurrentStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->currentStep); // confirmed real: direct field read
    return GM_OK;
}
// "16" resolution variant not modeled (see class header comment) -- returns
// the same step index as getCurrentStep rather than fabricating a scaled
// value we have no evidence for.
int GM_CDECL gmf_getCurrentStep16(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->currentStep);
    return GM_OK;
}
int GM_CDECL gmf_getCurrentPattern(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->currentPattern); // confirmed real: direct field read
    return GM_OK;
}
int GM_CDECL gmf_getMSTillNextStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushFloat(s->MsPerStep() - s->msAccumulator);
    return GM_OK;
}
int GM_CDECL gmf_getMSTillNextStepAccurate(gmThread* a_thread) { return gmf_getMSTillNextStep(a_thread); }
int GM_CDECL gmf_getMSTillNextStep16(gmThread* a_thread) { return gmf_getMSTillNextStep(a_thread); }
// Depends on the same unconfirmed noteType->step-subdivision approximation
// as getNumStepsPerNote above.
GE3_REMOVED(gmf_getMSTillNextNoteStep, "SEQUENCER.getMSTillNextNoteStep")
// Confirmed real by usage: getAllTriggers() returns an object walked with
// isValid()/get()/goToNext() -- SequencerHelper.gm's
// SEQUENCERHELPER_printSequencer/_getLastTrigger and ScaleOfTheWorld.gm's
// noteRecorder_addTrigger all do
// `for(it=seq.getAllTriggers();it.isValid();it.goToNext()){local tr=it.get(); ...}`
// then call tr.getStep()/tr.getAction() -- matching the confirmed
// decompiled body's FUN_0031aa10 wrap (the same ref-container shape as
// EntityListRefContainer/IntListRefContainer, TYPES.md) around a list of
// SEQUENCERACTION-typed elements. Previously returned a plain
// integer-indexed table (no isValid/get/goToNext at all), which null-called
// on the very first isValid() check in every one of those real call sites.
int GM_CDECL gmf_getAllTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* list = m.AllocTableObject();
    for (size_t i = 0; i < s->triggers.size(); ++i) {
        list->Set(&m, (int)i, s->triggers[i].handle);
    }
    gmTableObject* it = m.AllocTableObject();
    it->Set(&m, "__list", gmVariable(list));
    it->Set(&m, "__idx", gmVariable((gmint)0));
    it->Set(&m, "isValid", gmVariable(m.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            gmMachine& mm = *a_thread->GetMachine();
            gmVariable listVar = self->Get(&mm, "__list");
            gmVariable idxVar = self->Get(&mm, "__idx");
            int idx = (idxVar.m_type == GM_INT) ? idxVar.m_value.m_int : 0;
            int count = (listVar.m_type == GM_TABLE) ? ((gmTableObject*)mm.GetObject(listVar.m_value.m_ref))->Count() : 0;
            a_thread->PushInt(idx < count ? 1 : 0);
            return GM_OK;
        })));
    it->Set(&m, "get", gmVariable(m.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            gmMachine& mm = *a_thread->GetMachine();
            gmVariable listVar = self->Get(&mm, "__list");
            gmVariable idxVar = self->Get(&mm, "__idx");
            int idx = (idxVar.m_type == GM_INT) ? idxVar.m_value.m_int : 0;
            if (listVar.m_type == GM_TABLE) {
                a_thread->Push(((gmTableObject*)mm.GetObject(listVar.m_value.m_ref))->Get((gmint)idx));
            } else {
                a_thread->PushNull();
            }
            return GM_OK;
        })));
    it->Set(&m, "goToNext", gmVariable(m.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            gmMachine& mm = *a_thread->GetMachine();
            gmVariable idxVar = self->Get(&mm, "__idx");
            int idx = (idxVar.m_type == GM_INT) ? idxVar.m_value.m_int : 0;
            self->Set(&mm, "__idx", gmVariable((gmint)(idx + 1)));
            return GM_OK;
        })));
    a_thread->PushTable(it);
    return GM_OK;
}
// Depends on the same unconfirmed noteType->step-subdivision approximation
// as getNumStepsPerNote above.
GE3_REMOVED(gmf_getNextNoteStep, "SEQUENCER.getNextNoteStep")
GE3_REMOVED(gmf_getNextNotePattern, "SEQUENCER.getNextNotePattern")

gmFunctionEntry s_sequencerLib[] = {
    {"getNumStepsPerPattern", gmf_getNumStepsPerPattern, nullptr},
    {"setNumStepsPerPattern", gmf_setNumStepsPerPattern, nullptr},
    {"getBPM", gmf_getBPM, nullptr},
    {"setLoopLen", gmf_setLoopLen, nullptr},
    {"setCurrentStep", gmf_setCurrentStep, nullptr},
    {"setVolume", gmf_setVolume, nullptr},
    {"setPan", gmf_setPan, nullptr},
    {"setBPM", gmf_setBPM, nullptr},
    {"restart", gmf_restart, nullptr},
    {"start", gmf_start, nullptr},
    {"stop", gmf_stop, nullptr},
    {"play", gmf_play, nullptr},
    {"isPlaying", gmf_isPlaying, nullptr},
    {"getNumStepsPerNote", gmf_getNumStepsPerNote, nullptr},
    {"getNoteTime", gmf_getNoteTime, nullptr},
    {"getMSPerStep", gmf_getMSPerStep, nullptr},
    {"setTrigger", gmf_setTrigger, nullptr},
    {"setTrigger16", gmf_setTrigger16, nullptr},
    {"setTriggerOnNext", gmf_setTriggerOnNext, nullptr},
    {"moveTriggerToFront", gmf_moveTriggerToFront, nullptr},
    {"clearTriggers", gmf_clearTriggers, nullptr},
    {"getNumTriggers", gmf_getNumTriggers, nullptr},
    {"getWrappedStep", gmf_getWrappedStep, nullptr},
    {"gotoStep", gmf_gotoStep, nullptr},
    {"getCurrentStep", gmf_getCurrentStep, nullptr},
    {"getCurrentStep16", gmf_getCurrentStep16, nullptr},
    {"getCurrentPattern", gmf_getCurrentPattern, nullptr},
    {"getMSTillNextStep", gmf_getMSTillNextStep, nullptr},
    {"getMSTillNextStepAccurate", gmf_getMSTillNextStepAccurate, nullptr},
    {"getMSTillNextStep16", gmf_getMSTillNextStep16, nullptr},
    {"getMSTillNextNoteStep", gmf_getMSTillNextNoteStep, nullptr},
    {"getAllTriggers", gmf_getAllTriggers, nullptr},
    {"getNextNoteStep", gmf_getNextNoteStep, nullptr},
    {"getNextNotePattern", gmf_getNextNotePattern, nullptr},
};

void GM_CDECL SequencerGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    SequencerData* data = (SequencerData*)a_object->m_user;
    for (auto& t : data->triggers) {
        UnrootGmVariable(*a_machine, t.action); // see process_manager.h
        UnrootGmVariable(*a_machine, t.handle);
    }
    auto& all = AllSequencers();
    for (auto it = all.begin(); it != all.end(); ++it) {
        if (*it == data) { all.erase(it); break; }
    }
    delete data;
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

int GM_CDECL gmf_SEQUENCER_GetAllSequencers(gmThread* a_thread) {
    gmTableObject* table = a_thread->GetMachine()->AllocTableObject();
    int i = 0;
    for (SequencerData* s : AllSequencers()) {
        (void)s;
        // We don't keep a gmVariable handle alongside SequencerData (only the
        // raw pointer, for GC cleanup) -- listing live instances by count is
        // honest about that limitation rather than fabricating handles.
        table->Set(a_thread->GetMachine(), i++, gmVariable((gmint)1));
    }
    a_thread->PushTable(table);
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCER_PauseAllSequencers(gmThread* a_thread) { g_allPaused = true; return GM_OK; }
int GM_CDECL gmf_SEQUENCER_UnpauseAllSequencers(gmThread* a_thread) { g_allPaused = false; return GM_OK; }
// No decompiled evidence of what this actually does downstream (no
// observable effect recovered) -- previously a bare no-op.
GE3_REMOVED(gmf_SEQUENCER_OnNoteCollect, "SEQUENCER_OnNoteCollect")
int GM_CDECL gmf_SEQUENCER_SetMasterPulse(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(pulse, 0, 0.0f);
    g_masterPulse = pulse;
    return GM_OK;
}

// ---------------------------------------------------------------------
// SEQUENCERACTION -- plain GM table + injected native methods, not a
// distinct native user type. Real usage (game_data/Main.gm) sets arbitrary
// fields directly on an action instance (`act.parent=this;
// act.execute=function(){...};`) that aren't among the 8 confirmed native
// methods, meaning the real type supports generic property access beyond
// its native methods -- a plain table with injected methods gets the same
// externally-visible behavior (arbitrary property set/get + the confirmed
// methods) without us having to replicate a GETDOT/SETDOT fallback
// mechanism on a native user type. Internal state uses "__"-prefixed keys
// to stay out of the way of real script fields like "parent"/"execute".
// ---------------------------------------------------------------------

gmTableObject* ThisActionTable(gmThread* a_thread) {
    return a_thread->ThisTable();
}

// Shared builder for both the user-facing `SEQUENCERACTION()` constructor
// and the internal per-trigger handle setTrigger/setTrigger16/
// setTriggerOnNext create (see CreateSequencerActionTable's forward
// declaration above) -- both are the same native type/method set per the
gmTableObject* CreateSequencerActionTable(gmMachine& a_machine, int a_initialStep) {
    gmTableObject* table = a_machine.AllocTableObject();
    table->Set(&a_machine, "__pattern", gmVariable((gmint)0));
    table->Set(&a_machine, "__step", gmVariable((gmint)a_initialStep));
    table->Set(&a_machine, "__looped", gmVariable((gmint)0));
    table->Set(&a_machine, "__tagged", gmVariable((gmint)0));
    table->Set(&a_machine, "__boundTable", gmVariable::s_null);
    // Confirmed real field (SequencerAction.getAction reads a distinct
    // offset from __boundTable/setTable's field, never written by any of
    // the 8 recovered SequencerAction.c methods -- it's populated by the
    // SEQUENCER's own setTrigger/setTriggerOnNext, see below) -- defaults
    // null for a plain script-constructed `SEQUENCERACTION()`.
    table->Set(&a_machine, "__action", gmVariable::s_null);

    table->Set(&a_machine, "setTable", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            GM_CHECK_NUM_PARAMS(1);
            self->Set(a_thread->GetMachine(), "__boundTable", a_thread->Param(0));
            return GM_OK;
        })));
    table->Set(&a_machine, "tagForRemoval", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            self->Set(a_thread->GetMachine(), "__tagged", gmVariable((gmint)1));
            return GM_OK;
        })));
    table->Set(&a_machine, "isTaggedForRemoval", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            gmVariable v = self->Get(a_thread->GetMachine(), "__tagged");
            a_thread->PushInt(v.m_type == GM_INT && v.m_value.m_int != 0 ? 1 : 0);
            return GM_OK;
        })));
    table->Set(&a_machine, "getPattern", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            a_thread->Push(self->Get(a_thread->GetMachine(), "__pattern"));
            return GM_OK;
        })));
    table->Set(&a_machine, "getStep", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            a_thread->Push(self->Get(a_thread->GetMachine(), "__step"));
            return GM_OK;
        })));
    table->Set(&a_machine, "getLooped", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            gmVariable v = self->Get(a_thread->GetMachine(), "__looped");
            a_thread->PushInt(v.m_type == GM_INT && v.m_value.m_int != 0 ? 1 : 0);
            return GM_OK;
        })));
    // Confirmed real: the decompiled body (SequencerAction.getAction,
    // field and pushes it as-is -- it does NOT invoke execute() (this
    // engine's previous reading). Confirmed by real usage too:
    // ScaleOfTheWorld.gm's `local currentAct=tr.getAction();
    // if(currentAct.entityID==entityID)` reads a property the script set
    // directly on the original action table (`act.entityID=entityID;`) --
    // that only works if getAction() returns the bound table itself, not
    // whatever calling its execute() happened to return (none of the real
    // execute() functions in the corpus return anything).
    table->Set(&a_machine, "getAction", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            a_thread->Push(self->Get(a_thread->GetMachine(), "__action"));
            return GM_OK;
        })));
    return table;
}

// Confirmed real: 0 or 1 args -- the 1-arg form's value ends up stored at a
// fixed offset with no further use visible in the constructor itself. Real
// usage (`act=SEQUENCERACTION();`) is always 0-arg, so the 1-arg form's
// exact meaning is unconfirmed; stored as the initial step.
int GM_CDECL gmf_SEQUENCERACTION(gmThread* a_thread) {
    int initialStep = a_thread->GetNumParams() >= 1 ? a_thread->ParamInt(0, 0) : 0;
    gmTableObject* table = CreateSequencerActionTable(*a_thread->GetMachine(), initialStep);
    a_thread->PushTable(table);
    return GM_OK;
}

// SEQUENCE_FromTable(sourceTable, destSequence) -- confirmed real, live
// call site EntityBase.gm:706 (`SEQUENCE_FromTable(this.configureData.
// customSequence,this.customSequence);`, loading a per-entity custom
// rhythm/timing pattern from saved level JSON). Was entirely unbound --
// hard "getdot failed" aborting entity configure() for every entity with
// a table-form (as opposed to string-form) saved customSequence. Its
// return value is never read at any real call site and this
// reimplementation's sequencer has no custom-pattern-playback consumer
// yet (out of scope, matching MixerParams' own "stored for completeness,
// no subsystem consumes it yet" precedent above) -- genuine no-op,
// consumed rather than left as a null-call.
int GM_CDECL gmf_SEQUENCE_FromTable(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// SEQUENCE_ToString(seq)/SEQUENCE_FromString(str, seq)/SEQUENCE_ToTable(seq,
// (EntityBase.gm:175 `SEQUENCE_ToString(obj.customSequence)`, :312
// `SEQUENCE_ToTable(obj.customSequence,jsonTable.customSequence)`, :704
// `SEQUENCE_FromString(this.configureData.customSequence,
// this.customSequence)`; Main.gm's screen-clipboard copy/paste) but were
// entirely UNBOUND -- a hard null-call on every one of those, same gap
// SEQUENCE_FromTable above was found and fixed for. The real encoding
// (SEQUENCE_ToString's 'z'-prefixed per-step character packing, keyed off
// an external, unrecovered lookup table -- DAT_003726f4 -- and helper
// functions FUN_000f193c/FUN_000f2150 with no decompiled body in this
// corpus) isn't recoverable here, so a byte-faithful port of custom-
// rhythm-pattern save/load isn't possible with what this pass has -- same
// "no real customSequence-playback consumer yet" scope gap as
// SEQUENCE_FromTable. These are honest, arity-correct stubs rather than a
// guessed-at encoding: ToString returns the confirmed real 'z' prefix with
// an empty (no active steps) body, FromString is consumed as a no-op
// (matching FromTable's own precedent), and ToTable returns its own arg1
// (confirmed real: the decompiled body's return value type-tags as
// GM_TABLE and its value IS arg1) unmodified rather than null, since real
// callers immediately re-serialize that exact table to JSON.
int GM_CDECL gmf_SEQUENCE_ToString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    a_thread->PushString(a_thread->GetMachine()->AllocStringObject("z"));
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCE_FromString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCE_ToTable(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    a_thread->Push(a_thread->Param(1));
    return GM_OK;
}

} // namespace

void RegisterSequencerBindings(gmMachine& a_machine) {
    g_sequencerType = a_machine.CreateUserType("SEQUENCER");
    a_machine.RegisterTypeLibrary(g_sequencerType, s_sequencerLib,
                                   sizeof(s_sequencerLib) / sizeof(s_sequencerLib[0]));
    a_machine.RegisterUserCallbacks(g_sequencerType, nullptr, SequencerGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("SEQUENCER", gmf_SEQUENCER);

    a_machine.RegisterLibraryFunction("SEQUENCER_GetAllSequencers", gmf_SEQUENCER_GetAllSequencers);
    a_machine.RegisterLibraryFunction("SEQUENCER_PauseAllSequencers", gmf_SEQUENCER_PauseAllSequencers);
    a_machine.RegisterLibraryFunction("SEQUENCER_UnpauseAllSequencers", gmf_SEQUENCER_UnpauseAllSequencers);
    a_machine.RegisterLibraryFunction("SEQUENCER_OnNoteCollect", gmf_SEQUENCER_OnNoteCollect);
    a_machine.RegisterLibraryFunction("SEQUENCER_SetMasterPulse", gmf_SEQUENCER_SetMasterPulse);
    a_machine.RegisterLibraryFunction("SEQUENCE_FromTable", gmf_SEQUENCE_FromTable);
    a_machine.RegisterLibraryFunction("SEQUENCE_ToString", gmf_SEQUENCE_ToString);
    a_machine.RegisterLibraryFunction("SEQUENCE_FromString", gmf_SEQUENCE_FromString);
    a_machine.RegisterLibraryFunction("SEQUENCE_ToTable", gmf_SEQUENCE_ToTable);

    a_machine.RegisterLibraryFunction("SEQUENCERACTION", gmf_SEQUENCERACTION);
}

void UpdateAllSequencers(gmMachine& a_machine, float a_deltaMs) {
    if (g_allPaused) return;
    for (SequencerData* s : AllSequencers()) {
        if (!s->isPlaying) continue;
        float msPerStep = s->MsPerStep();
        if (msPerStep <= 0.0f) continue;
        s->msAccumulator += a_deltaMs;
        int guard = 0;
        while (s->msAccumulator >= msPerStep && guard++ < 1000) {
            s->msAccumulator -= msPerStep;
            s->currentStep = WrapStep(s->currentStep + 1, s->EffectiveLen());
            if (s->currentStep == 0) s->currentPattern++;

            // Prune any trigger a script tagged for removal (SequencerAction.
            // SequencerAction.c) before firing this step's triggers, e.g.
            // ScaleOfTheWorld.gm scanning getAllTriggers() and tagging ones
            // to drop.
            for (size_t i = 0; i < s->triggers.size(); ) {
                if (IsHandleTagged(a_machine, s->triggers[i].handle)) {
                    UnrootGmVariable(a_machine, s->triggers[i].action);
                    UnrootGmVariable(a_machine, s->triggers[i].handle);
                    s->triggers.erase(s->triggers.begin() + i);
                } else {
                    ++i;
                }
            }

            for (size_t i = 0; i < s->triggers.size(); ) {
                SequencerTrigger& t = s->triggers[i];
                if (t.step == s->currentStep) {
                    FireTrigger(a_machine, t.action);
                    // See SequencerTrigger::looped's comment: a non-looped
                    // trigger fires once and removes itself; a looped one
                    // can still self-remove via tagForRemoval() from inside
                    // its own execute() (SequencerHelper.gm's
                    // SEQUENCERHELPER_AddTriggerWithSkip does exactly this).
                    if (!t.looped || IsHandleTagged(a_machine, t.handle)) {
                        UnrootGmVariable(a_machine, t.action);
                        UnrootGmVariable(a_machine, t.handle);
                        s->triggers.erase(s->triggers.begin() + i);
                        continue;
                    }
                }
                ++i;
            }
        }
    }
}

gmVariable CreateSequencerInstance(gmMachine& a_machine, int a_initialBpm) {
    SequencerData* data = new SequencerData();
    data->bpm = a_initialBpm > 0 ? a_initialBpm : 120;
    AllSequencers().push_back(data);
    gmUserObject* obj = a_machine.AllocUserObject(data, g_sequencerType);
    return gmVariable(obj);
}
