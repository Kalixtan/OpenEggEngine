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
    bool enabled = true;
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

// Confirmed real: 2, 3, or 4 args -- (step, action, [enabled]) or
// (step, pattern, action, [enabled]). Only the (step, action, [enabled])
// form is exercised anywhere in the real script corpus (confirmed by grep:
// every real `.setTrigger(` call site uses exactly 3 args, step+action+bool)
// so that's the form actually implemented; the pattern-indexed 4-arg form
// is accepted syntactically (arg count checked, no exception) but the
// pattern argument is currently ignored rather than guessed at.
int GM_CDECL gmf_setTrigger(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    int n = a_thread->GetNumParams();
    if (n < 2 || n > 4) { GM_EXCEPTION_MSG("setTrigger: expecting 2-4 params"); return GM_EXCEPTION; }
    GM_INT_PARAM(step, 0, 0);
    gmVariable action;
    bool enabled = true;
    if (n == 2) {
        action = a_thread->Param(1);
    } else if (n == 3 && a_thread->Param(1).m_type == GM_INT) {
        // (step, pattern, action) -- pattern ignored, see comment above.
        action = a_thread->Param(2);
    } else {
        action = a_thread->Param(1);
        GM_INT_PARAM(en, (n == 4 ? 3 : 2), 1);
        enabled = (en != 0);
    }
    SequencerTrigger t;
    t.step = WrapStep(step, s->EffectiveLen());
    t.action = action;
    t.enabled = enabled;
    RootGmVariable(*a_thread->GetMachine(), t.action); // see process_manager.h
    s->triggers.push_back(t);
    a_thread->PushInt((int)s->triggers.size() - 1); // handle: index into our own trigger list
    return GM_OK;
}
// setTrigger16: confirmed real, same overload shape as setTrigger. The
// "16" resolution distinction isn't modeled (see getCurrentStep16 note) --
// delegates to the same trigger list/step-space as setTrigger.
int GM_CDECL gmf_setTrigger16(gmThread* a_thread) { return gmf_setTrigger(a_thread); }

int GM_CDECL gmf_setTriggerOnNext(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    int n = a_thread->GetNumParams();
    if (n < 2 || n > 3) { GM_EXCEPTION_MSG("setTriggerOnNext: expecting 2-3 params"); return GM_EXCEPTION; }
    GM_INT_PARAM(step, 0, 0);
    gmVariable action = a_thread->Param(1);
    bool enabled = true;
    if (n == 3) { GM_INT_PARAM(en, 2, 1); enabled = (en != 0); }
    SequencerTrigger t;
    t.step = WrapStep(step, s->EffectiveLen());
    t.action = action;
    t.enabled = enabled;
    RootGmVariable(*a_thread->GetMachine(), t.action); // see process_manager.h
    s->triggers.insert(s->triggers.begin(), t); // "OnNext": front of the queue, see moveTriggerToFront below
    a_thread->PushInt(0);
    return GM_OK;
}
// Confirmed real: 1 arg, a trigger handle. Our handle is the index
// setTrigger/setTrigger16/setTriggerOnNext returned.
int GM_CDECL gmf_moveTriggerToFront(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(handle, 0, -1);
    if (handle >= 0 && handle < (int)s->triggers.size() && handle != 0) {
        SequencerTrigger t = s->triggers[handle];
        s->triggers.erase(s->triggers.begin() + handle);
        s->triggers.insert(s->triggers.begin(), t);
    }
    return GM_OK;
}
int GM_CDECL gmf_clearTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    for (auto& t : s->triggers) UnrootGmVariable(*a_thread->GetMachine(), t.action);
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
int GM_CDECL gmf_getAllTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    gmTableObject* table = a_thread->GetMachine()->AllocTableObject();
    for (size_t i = 0; i < s->triggers.size(); ++i) {
        table->Set(a_thread->GetMachine(), (int)i, s->triggers[i].action);
    }
    a_thread->PushTable(table);
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
    for (auto& t : data->triggers) UnrootGmVariable(*a_machine, t.action); // see process_manager.h
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

// Confirmed real: 0 or 1 args -- the 1-arg form's value ends up stored at a
// fixed offset with no further use visible in the constructor itself. Real
// usage (`act=SEQUENCERACTION();`) is always 0-arg, so the 1-arg form's
// exact meaning is unconfirmed; stored as the initial step.
int GM_CDECL gmf_SEQUENCERACTION(gmThread* a_thread) {
    gmTableObject* table = a_thread->GetMachine()->AllocTableObject();
    int initialStep = a_thread->GetNumParams() >= 1 ? a_thread->ParamInt(0, 0) : 0;
    table->Set(a_thread->GetMachine(), "__pattern", gmVariable((gmint)0));
    table->Set(a_thread->GetMachine(), "__step", gmVariable((gmint)initialStep));
    table->Set(a_thread->GetMachine(), "__looped", gmVariable((gmint)0));
    table->Set(a_thread->GetMachine(), "__tagged", gmVariable((gmint)0));
    table->Set(a_thread->GetMachine(), "__boundTable", gmVariable::s_null);

    table->Set(a_thread->GetMachine(), "setTable", gmVariable(a_thread->GetMachine()->AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            GM_CHECK_NUM_PARAMS(1);
            self->Set(a_thread->GetMachine(), "__boundTable", a_thread->Param(0));
            return GM_OK;
        })));
    table->Set(a_thread->GetMachine(), "tagForRemoval", gmVariable(a_thread->GetMachine()->AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            self->Set(a_thread->GetMachine(), "__tagged", gmVariable((gmint)1));
            return GM_OK;
        })));
    table->Set(a_thread->GetMachine(), "isTaggedForRemoval", gmVariable(a_thread->GetMachine()->AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            gmVariable v = self->Get(a_thread->GetMachine(), "__tagged");
            a_thread->PushInt(v.m_type == GM_INT && v.m_value.m_int != 0 ? 1 : 0);
            return GM_OK;
        })));
    table->Set(a_thread->GetMachine(), "getPattern", gmVariable(a_thread->GetMachine()->AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            a_thread->Push(self->Get(a_thread->GetMachine(), "__pattern"));
            return GM_OK;
        })));
    table->Set(a_thread->GetMachine(), "getStep", gmVariable(a_thread->GetMachine()->AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            a_thread->Push(self->Get(a_thread->GetMachine(), "__step"));
            return GM_OK;
        })));
    table->Set(a_thread->GetMachine(), "getLooped", gmVariable(a_thread->GetMachine()->AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            gmVariable v = self->Get(a_thread->GetMachine(), "__looped");
            a_thread->PushInt(v.m_type == GM_INT && v.m_value.m_int != 0 ? 1 : 0);
            return GM_OK;
        })));
    // Confirmed body passes the calling thread itself into a native helper
    // -- the most plausible read is "invoke the bound action and return its
    // result", which matches real usage exactly (act.execute=function(){...}).
    table->Set(a_thread->GetMachine(), "getAction", gmVariable(a_thread->GetMachine()->AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = ThisActionTable(a_thread);
            if (!self) return GM_EXCEPTION;
            gmVariable boundOrSelf = self->Get(a_thread->GetMachine(), "__boundTable");
            gmTableObject* callTarget = self;
            gmVariable callTargetVar = *a_thread->GetThis();
            if (boundOrSelf.m_type == GM_TABLE) {
                callTarget = (gmTableObject*)a_thread->GetMachine()->GetObject(boundOrSelf.m_value.m_ref);
                callTargetVar = boundOrSelf;
            }
            gmCall call;
            if (call.BeginTableFunction(a_thread->GetMachine(), "execute", callTarget, callTargetVar)) {
                call.End();
                if (call.DidReturnVariable()) a_thread->Push(call.GetReturnedVariable());
                else a_thread->PushNull();
            } else {
                a_thread->PushNull();
            }
            return GM_OK;
        })));

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
            for (auto& t : s->triggers) {
                if (t.enabled && t.step == s->currentStep) {
                    FireTrigger(a_machine, t.action);
                }
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
