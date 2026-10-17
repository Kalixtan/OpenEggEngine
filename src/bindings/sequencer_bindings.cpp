#include "gmThread.h"
#include "sequencer_bindings.h"
#include "gmCall.h"
#include "gmTableObject.h"
#include "gmUserObject.h"
#include "gmOperators.h"
#include "../core/process_manager.h" // RootGmVariable/UnrootGmVariable
#include "../core/strict_calls.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <limits>
#include <string>
#include <utility>

namespace {

gmType g_sequencerType = -1;
gmType g_sequencerListRefType = -1;
gmType g_sequencerTriggerType = -1;
gmType g_sequencerTriggerListType = -1;
gmType g_sequencerActionType = -1;
uint64_t g_nextSequencerSerial = 1;

// Retail's setNumStepsPerPattern wrapper uses PPC fctiwz for GM_FLOAT and
// copies the payload word directly for every other GM type. Keep the float
// conversion's truncation and invalid-result behavior explicit instead of
// relying on an out-of-range C++ float-to-int cast.
int32_t RetailFctiwz(float value) {
    constexpr float kIntMin = -2147483648.0f;
    constexpr float kIntMaxExclusive = 2147483648.0f;
    if (std::isnan(value) || value < kIntMin) {
        return std::numeric_limits<int32_t>::min();
    }
    if (value >= kIntMaxExclusive) return std::numeric_limits<int32_t>::max();
    return static_cast<int32_t>(value);
}

float RetailFselMaxZero(float value) {
    // PPC fsel selects its original second operand for NaN comparisons; the
    // ordinary std::max ordering would instead return the zero operand.
    return std::isnan(value) ? value : std::max(0.0f, value);
}

struct SequencerTrigger {
    uint64_t serial = 0;
    int pattern = 0;
    int step = 0;
    gmVariable action = gmVariable::s_null; // normally a SEQUENCERACTION native object
    // Confirmed real field (SequencerTrigger.getLooped in the sequencer
    // type's four-method table) -- NOT a fire/don't-fire mask (that was this
    // engine's previous, unconfirmed "enabled" reading). Every real call
    // site is consistent with "does this trigger keep firing every time
    // the pattern loops back to its step (true), or fire once and then
    // remove itself (false)": SequencerHelper.gm passes false for one-shot
    // sync/volume-fade actions and true for repeating rhythm-pattern
    // triggers. The exact retail removal mechanics aren't decompiled (no
    // sequencer-tick body in the corpus), so "fire once, then erase" is
    // our best-supported reading, not a confirmed decompiled port.
    bool looped = true;
    // restart(step) inserts a native one-shot action whose callback resets
    // the sequencer. It does not carry a script action or returned handle.
    bool restartAction = false;
    // Confirmed real: setTrigger/setTrigger16/setTriggerOnNext return a
    // trigger object using DAT_0043c394), and
    // getAllTriggers()/moveTriggerToFront operate on that same handle, not
    // a bare index.
    gmVariable handle = gmVariable::s_null;
};

struct SequencerTriggerList {
    gmMachine* machine = nullptr;
    uint64_t nextSerial = 1;
    std::vector<SequencerTrigger> values;
    ~SequencerTriggerList() {
        if (!machine) return;
        for (const auto& trigger : values)
            UnrootGmVariable(*machine, trigger.handle);
    }
};

// Retail setTrigger returns the SEQUENCERTRIGGER native type (DAT_0043c394),
// distinct from the SEQUENCERACTION passed into it (DAT_0043c390). The
// trigger exposes its own metadata and retains the original action object.
struct SequencerTriggerHandle {
    int pattern = 0;
    int step = 0;
    bool looped = true;
    gmVariable action = gmVariable::s_null;
    gmMachine* machine = nullptr;
};

// Confirmed direct-offset fields from the decompiled body (currentStep at
// +0x28, currentPattern at +0x24 -- both read/written with no function call
// at all, unlike every other property here) map onto currentStep/
// currentPattern below. Timing (msPerStep etc) is real BPM-driven math, not
// a stub -- see UpdateAllSequencers.
struct SequencerData {
    int bpm = 98;                // retail constructor +0x4c
    int numStepsPerPattern = 32; // retail constructor +0x54
    int loopLen = 1;             // number of patterns before +0x24 wraps, retail +0x58
    int currentStep = -1;        // retail constructor +0x28; first tick reaches step zero
    int currentPattern = 0;      // confirmed direct-offset field (+0x24)
    float volume = 1.0f;         // setVolume confirmed real setter (no confirmed getter)
    float pan = 0.0f;            // setPan confirmed real setter (no confirmed getter)
    bool isPlaying = false;      // start/stop confirmed real actions; isPlaying is real usage (Main.gm) but not
                                  // in the confirmed decompiled method list -- see header comment on "play"/"isPlaying"
    gmVariable handle = gmVariable::s_null; // GM sequencer object returned by SEQUENCER()
    uint64_t serial = 0;         // insertion order in retail's manager list
    std::shared_ptr<SequencerTriggerList> triggers = std::make_shared<SequencerTriggerList>();
    float msTillNextStep = 0.0f; // retail countdown at +0x48
    float msPerStep = (1.0f / (98.0f * 0.01666666753590107f)) * 1000.0f * 0.125f;

    int EffectiveLen() const { return numStepsPerPattern; }
    // Retail FUN_0016bd48/FUN_0016c8b8 store this at +0x50. The binary
    // constants are 1, 1/60, 1000 and 1/8, in that operation order.
    float CalculateMsPerStep() const {
        return (1.0f / ((float)bpm * 0.01666666753590107f)) * 1000.0f * 0.125f;
    }
    float MsPerStep() const { return msPerStep; }
};

struct SequencerTriggerListRefData {
    std::shared_ptr<SequencerTriggerList> list;
    uint64_t currentSerial = 0; // zero is the end sentinel
};

struct SequencerActionData {
    gmVariable boundTable = gmVariable::s_null;
    bool tagged = false;
};

std::vector<SequencerData*>& AllSequencers() {
    static std::vector<SequencerData*> all;
    return all;
}

struct SequencerListRefData { uint64_t currentSerial = 0; };

SequencerTrigger* FindTriggerBySerial(SequencerTriggerList* list, uint64_t serial) {
    if (!list || serial == 0) return nullptr;
    for (auto& trigger : list->values)
        if (trigger.serial == serial) return &trigger;
    return nullptr;
}

SequencerTriggerListRefData* ThisTriggerListRef(gmThread* thread) {
    return (SequencerTriggerListRefData*)thread->ThisUserCheckType(g_sequencerTriggerListType);
}
int GM_CDECL gmf_triggerList_isValid(gmThread* thread) {
    auto* ref = ThisTriggerListRef(thread);
    thread->PushInt(ref && FindTriggerBySerial(ref->list.get(), ref->currentSerial) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_triggerList_get(gmThread* thread) {
    auto* ref = ThisTriggerListRef(thread);
    SequencerTrigger* trigger = ref ? FindTriggerBySerial(ref->list.get(), ref->currentSerial) : nullptr;
    if (trigger) thread->Push(trigger->handle);
    else thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_triggerList_goToNext(gmThread* thread) {
    auto* ref = ThisTriggerListRef(thread);
    if (!ref || !ref->list) return GM_OK;
    if (ref->currentSerial == 0) {
        ref->currentSerial = ref->list->values.empty() ? 0 : ref->list->values.front().serial;
        return GM_OK;
    }
    for (size_t i = 0; i < ref->list->values.size(); ++i) {
        if (ref->list->values[i].serial != ref->currentSerial) continue;
        ref->currentSerial = i + 1 < ref->list->values.size()
            ? ref->list->values[i + 1].serial : 0;
        return GM_OK;
    }
    ref->currentSerial = 0;
    return GM_OK;
}
int GM_CDECL gmf_triggerList_reset(gmThread* thread) {
    auto* ref = ThisTriggerListRef(thread);
    if (ref)
        ref->currentSerial = ref->list && !ref->list->values.empty()
            ? ref->list->values.front().serial : 0;
    return GM_OK;
}
int GM_CDECL gmf_triggerList_size(gmThread* thread) {
    auto* ref = ThisTriggerListRef(thread);
    thread->PushInt(ref && ref->list ? (gmint)ref->list->values.size() : 0);
    return GM_OK;
}
void GM_CDECL TriggerListRefGCDestruct(gmMachine*, gmUserObject* object) {
    delete (SequencerTriggerListRefData*)object->m_user;
}
gmFunctionEntry s_sequencerTriggerListLib[] = {
    {"get", gmf_triggerList_get, nullptr},
    {"isValid", gmf_triggerList_isValid, nullptr},
    {"goToNext", gmf_triggerList_goToNext, nullptr},
    {"reset", gmf_triggerList_reset, nullptr},
    {"size", gmf_triggerList_size, nullptr},
};

SequencerData* FindSequencerBySerial(uint64_t serial) {
    if (serial == 0) return nullptr;
    for (SequencerData* s : AllSequencers())
        if (s && s->serial == serial) return s;
    return nullptr;
}

uint64_t FirstSequencerSerialAfter(uint64_t serial) {
    uint64_t next = 0;
    for (SequencerData* s : AllSequencers())
        if (s && s->serial > serial && (next == 0 || s->serial < next)) next = s->serial;
    return next;
}

int GM_CDECL gmf_sequencerListRef_isValid(gmThread* a_thread) {
    auto* it = (SequencerListRefData*)a_thread->ThisUserCheckType(g_sequencerListRefType);
    a_thread->PushInt(it && FindSequencerBySerial(it->currentSerial) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_sequencerListRef_goToNext(gmThread* a_thread) {
    auto* it = (SequencerListRefData*)a_thread->ThisUserCheckType(g_sequencerListRefType);
    if (it) {
        it->currentSerial = it->currentSerial == 0
            ? FirstSequencerSerialAfter(0)
            : FirstSequencerSerialAfter(it->currentSerial);
    }
    return GM_OK;
}

int GM_CDECL gmf_sequencerListRef_get(gmThread* a_thread) {
    auto* it = (SequencerListRefData*)a_thread->ThisUserCheckType(g_sequencerListRefType);
    SequencerData* s = it ? FindSequencerBySerial(it->currentSerial) : nullptr;
    if (s) a_thread->Push(s->handle);
    else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL gmf_sequencerListRef_reset(gmThread* a_thread) {
    auto* it = (SequencerListRefData*)a_thread->ThisUserCheckType(g_sequencerListRefType);
    if (it) it->currentSerial = FirstSequencerSerialAfter(0);
    return GM_OK;
}

int GM_CDECL gmf_sequencerListRef_size(gmThread* a_thread) {
    a_thread->PushInt((gmint)AllSequencers().size());
    return GM_OK;
}

gmFunctionEntry s_sequencerListRefLib[] = {
    {"get", gmf_sequencerListRef_get, nullptr},
    {"isValid", gmf_sequencerListRef_isValid, nullptr},
    {"goToNext", gmf_sequencerListRef_goToNext, nullptr},
    {"reset", gmf_sequencerListRef_reset, nullptr},
    {"size", gmf_sequencerListRef_size, nullptr},
};

void GM_CDECL SequencerListRefGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (SequencerListRefData*)a_object->m_user;
}

float g_masterPulse = 0.0f;       // SEQUENCER_SetMasterPulse

int WrapStep(int a_step, int a_len) {
    if (a_len <= 0) return 0;
    int w = a_step % a_len;
    if (w < 0) w += a_len;
    return w;
}

int StepsForNoteType(int a_type) {
    // Retail FUN_0016ceb0: note types 0..5 map to 1,2,4,8,16,32.
    return a_type >= 0 && a_type <= 5 ? 1 << a_type : 0;
}

struct TriggerPosition { int pattern; int step; };

TriggerPosition NextNotePosition(const SequencerData& a_seq, int a_noteType) {
    const int spacing = StepsForNoteType(a_noteType);
    if (spacing <= 0) return {a_seq.currentPattern, a_seq.currentStep};
    int pattern = a_seq.currentPattern;
    int step = (a_seq.currentStep / spacing) * spacing + spacing;
    if (a_seq.numStepsPerPattern > 0 && step >= a_seq.numStepsPerPattern) {
        ++pattern;
        step -= a_seq.numStepsPerPattern;
        if (pattern >= a_seq.loopLen) pattern = 0;
    }
    return {pattern, step};
}

void InsertTrigger(SequencerData& a_seq, SequencerTrigger a_trigger) {
    // FUN_0016c97c inserts after existing triggers at the same
    // (pattern, step), preserving their call order.
    if (a_trigger.serial == 0)
        a_trigger.serial = a_seq.triggers->nextSerial++;
    auto& triggers = a_seq.triggers->values;
    auto it = std::upper_bound(triggers.begin(), triggers.end(), a_trigger,
        [](const SequencerTrigger& value, const SequencerTrigger& existing) {
            return value.pattern < existing.pattern ||
                (value.pattern == existing.pattern && value.step < existing.step);
        });
    triggers.insert(it, std::move(a_trigger));
}

void FireTrigger(gmMachine& a_machine, const gmVariable& a_action) {
    if (a_action.m_type != g_sequencerActionType) return;
    auto* userObject = static_cast<gmUserObject*>(
        a_machine.GetObject(a_action.m_value.m_ref));
    auto* action = userObject && userObject->GetType() == g_sequencerActionType
        ? static_cast<SequencerActionData*>(userObject->m_user) : nullptr;
    if (!action || action->boundTable.m_type != GM_TABLE) return;
    // Retail SEQUENCERACTION's virtual slot +0x14 (FUN_000b1804) resolves
    // `execute` on +0x70. This is also the dynamic property table used by
    // the inherited GETDOT/SETDOT methods; setTable() replaces both routes.
    // Its script receiver remains the action object (+0x7c), while the
    // GameSystem* passed by FUN_0016d580 is unused by the retail callback.
    gmTableObject* callbackTable = static_cast<gmTableObject*>(
        a_machine.GetObject(action->boundTable.m_value.m_ref));
    if (!callbackTable) return;
    static const bool s_traceTriggerActions =
        std::getenv("GE3_TRACE_SEQUENCER_STEPS") != nullptr;
    if (s_traceTriggerActions) {
        gmVariable parent = callbackTable->Get(&a_machine, "parent");
        int setup = -1;
        if (parent.m_type == GM_TABLE) {
            auto* parentTable = static_cast<gmTableObject*>(a_machine.GetObject(parent.m_value.m_ref));
            gmVariable called = parentTable
                ? parentTable->Get(&a_machine, "calledSetupLevel") : gmVariable::s_null;
            setup = called.m_type == GM_INT ? called.m_value.m_int : -1;
        }
        std::fprintf(stderr, "[sequencer-fire] execute=%d setup=%d\n",
            callbackTable->Get(&a_machine, "execute").m_type == GM_FUNCTION ? 1 : 0,
            setup);
    }
    gmCall call;
    // The callback is looked up on the selected table but called with the
    // SEQUENCERACTION instance as `this`.
    if (call.BeginTableFunction(&a_machine, "execute", callbackTable, a_action)) {
        call.End();
    }
}

SequencerData* ThisSequencer(gmThread* a_thread) {
    return (SequencerData*)a_thread->ThisUserCheckType(g_sequencerType);
}

gmVariable CreateSequencerTriggerHandle(gmMachine& a_machine, int a_pattern,
                                        int a_step, bool a_looped,
                                        const gmVariable& a_action);

SequencerTriggerHandle* ThisTriggerHandle(gmThread* a_thread) {
    return (SequencerTriggerHandle*)a_thread->ThisUserCheckType(g_sequencerTriggerType);
}
int GM_CDECL gmf_trigger_getPattern(gmThread* a_thread) {
    auto* trigger = ThisTriggerHandle(a_thread); if (!trigger) return GM_EXCEPTION;
    a_thread->PushInt(trigger->pattern);
    return GM_OK;
}
int GM_CDECL gmf_trigger_getStep(gmThread* a_thread) {
    auto* trigger = ThisTriggerHandle(a_thread); if (!trigger) return GM_EXCEPTION;
    a_thread->PushInt(trigger->step);
    return GM_OK;
}
int GM_CDECL gmf_trigger_getLooped(gmThread* a_thread) {
    auto* trigger = ThisTriggerHandle(a_thread); if (!trigger) return GM_EXCEPTION;
    a_thread->PushInt(trigger->looped ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_trigger_getAction(gmThread* a_thread) {
    auto* trigger = ThisTriggerHandle(a_thread); if (!trigger) return GM_EXCEPTION;
    a_thread->Push(trigger->action);
    return GM_OK;
}
gmFunctionEntry s_sequencerTriggerLib[] = {
    {"getPattern", gmf_trigger_getPattern, nullptr},
    {"getStep", gmf_trigger_getStep, nullptr},
    {"getLooped", gmf_trigger_getLooped, nullptr},
    {"getAction", gmf_trigger_getAction, nullptr},
};
void GM_CDECL SequencerTriggerGCDestruct(gmMachine*, gmUserObject* a_object) {
    auto* trigger = (SequencerTriggerHandle*)a_object->m_user;
    if (trigger->machine) UnrootGmVariable(*trigger->machine, trigger->action);
    delete trigger;
}

bool IsActionTagged(gmMachine& a_machine, const gmVariable& a_action) {
    if (a_action.m_type != g_sequencerActionType) return false;
    auto* object = static_cast<gmUserObject*>(
        a_machine.GetObject(a_action.m_value.m_ref));
    auto* action = object && object->GetType() == g_sequencerActionType
        ? static_cast<SequencerActionData*>(object->m_user) : nullptr;
    return action && action->tagged;
}

// ---------------------------------------------------------------------
// SEQUENCER
// ---------------------------------------------------------------------

// The retail constructor's argument is a GameSystem pointer at +0x60;
// its own timing defaults are BPM 98 and 32 steps per pattern.
int GM_CDECL gmf_SEQUENCER(gmThread* a_thread) {
    a_thread->Push(CreateSequencerInstance(*a_thread->GetMachine(), 98));
    return GM_OK;
}

int GM_CDECL gmf_getNumStepsPerPattern(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->numStepsPerPattern);
    return GM_OK;
}
int GM_CDECL gmf_setNumStepsPerPattern(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    const gmVariable& arg = a_thread->Param(0);
    const int32_t numSteps = arg.m_type == GM_FLOAT
        ? RetailFctiwz(arg.m_value.m_float)
        : arg.m_value.m_int;
    s->numStepsPerPattern = numSteps;
    // Retail FUN_0016c8b8 also recomputes +0x50 from the current BPM. This
    // matters when changing pattern length after setBPM (the interval is
    // normally independent of length, but retail still writes it here).
    // Keep the same operation order as the native helper.
    s->msPerStep = s->CalculateMsPerStep();
    s->msTillNextStep = 0.0f;
    return GM_OK;
}
int GM_CDECL gmf_getBPM(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->bpm);
    return GM_OK;
}
int GM_CDECL gmf_setLoopLen(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(n, 0, 1.0f);
    s->loopLen = (int)n;
    return GM_OK;
}
int GM_CDECL gmf_setCurrentStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    const gmVariable& arg = a_thread->Param(0);
    s->currentStep = arg.m_type == GM_FLOAT
        ? RetailFctiwz(arg.m_value.m_float)
        : arg.m_value.m_int; // Retail writes directly, without wrapping or side effects.
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
// Retail FUN_0016bd48 recomputes +0x50 and resets the +0x48 countdown
// unless its optional second argument is truthy.
int GM_CDECL gmf_setBPM(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    if (a_thread->GetNumParams() < 1 || a_thread->GetNumParams() > 2) return GM_EXCEPTION;
    // Retail branches on the exact GM_FLOAT tag, applying fctiwz to floats
    // and loading the raw word for every other type. Real callers pass a value
    // straight from level XML/JSON data (Platformer/ScaleOfTheWorld.gm's
    // setMinBPM -> MainSequencer.setBPM(this.minBPM), fed by
    // levelData.bpm), and every XML/JSON leaf number parses to a GM_FLOAT
    // (gamemonkeyscript_bindings.cpp's ParseLeafValue/JsonToGmVariable) --
    // so float input is required even though the native field is integer.
    const gmVariable& bpmArg = a_thread->Param(0);
    s->bpm = bpmArg.m_type == GM_FLOAT
        ? RetailFctiwz(bpmArg.m_value.m_float)
        : bpmArg.m_value.m_int;
    s->msPerStep = s->CalculateMsPerStep();
    if (a_thread->GetNumParams() == 1 || a_thread->Param(1).m_value.m_int == 0)
        s->msTillNextStep = 0.0f;
    return GM_OK;
}
// Retail FUN_0016c960 resets pattern/step and starts playback. Its one-arg
// overload FUN_0016cbc8 schedules a nonlooped native action at pattern 0,
// with FUN_0016c9c8 wrapping the step. The action callback FUN_0033279c
// applies the same three field writes when that step is reached.
int GM_CDECL gmf_restart(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    if (a_thread->GetNumParams() > 1) return GM_EXCEPTION;
    if (a_thread->GetNumParams() == 1) {
        GM_FLOAT_OR_INT_PARAM(step, 0, 0.0f);
        const int integerStep = (int)step;
        SequencerTrigger trigger;
        trigger.step = s->numStepsPerPattern > 0
            ? WrapStep(integerStep, s->numStepsPerPattern) : std::max(0, integerStep);
        trigger.looped = false;
        trigger.restartAction = true;
        gmMachine& machine = *a_thread->GetMachine();
        trigger.handle = CreateSequencerTriggerHandle(machine, trigger.pattern,
            trigger.step, trigger.looped, trigger.action);
        RootGmVariable(machine, trigger.handle);
        InsertTrigger(*s, std::move(trigger));
        return GM_OK;
    }
    s->currentPattern = 0;
    s->currentStep = -1;
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

int GM_CDECL gmf_getNumStepsPerNote(gmThread* a_thread) {
    if (!ThisSequencer(a_thread)) return GM_EXCEPTION;
    const int noteType = a_thread->Param(0).m_value.m_int;
    a_thread->PushInt(StepsForNoteType(noteType));
    return GM_OK;
}
int GM_CDECL gmf_getNoteTime(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    const int noteType = a_thread->Param(0).m_value.m_int;
    a_thread->PushFloat((float)StepsForNoteType(noteType) * s->CalculateMsPerStep());
    return GM_OK;
}
int GM_CDECL gmf_getMSPerStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushFloat(s->MsPerStep());
    return GM_OK;
}

// or 4 args. The arg-count/type dispatch itself is confirmed from the
// decompiled body:
//   2 args            -> (step, action), looped defaults true
//   3 args, arg1 is int -> (pattern, step, action), looped defaults true
//   3 args, arg1 not int -> (step, action, looped)
//   4 args            -> (pattern, step, action, looped) unconditionally
//                         (no type check on arg1 for the 4-arg form)
// Retail FUN_0016ca1c normalizes the implicit-pattern form to pattern 0;
// FUN_0016cd08 stores explicit (pattern, step) coordinates unchanged.
int SetTriggerImpl(gmThread* a_thread, bool a_sixteenStep) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    int n = a_thread->GetNumParams();
    if (n < 2 || n > 4) { GM_EXCEPTION_MSG("setTrigger: expecting 2-4 params"); return GM_EXCEPTION; }
    GM_INT_PARAM(first, 0, 0);
    gmVariable action;
    int pattern = 0;
    int step = first;
    bool looped = true;
    if (n == 2) {
        action = a_thread->Param(1);
    } else if (n == 3 && a_thread->Param(1).m_type == GM_INT) {
        // (pattern, step, action)
        pattern = first;
        step = a_thread->Param(1).m_value.m_int;
        action = a_thread->Param(2);
    } else if (n == 3) {
        // (step, action, looped)
        action = a_thread->Param(1);
        GM_INT_PARAM(lp, 2, 1);
        looped = (lp != 0);
    } else {
        // n == 4: (pattern, step, action, looped) -- always this shape,
        // regardless of arg1's type (confirmed: the decompiled 4-arg branch
        // has no type check, unlike the 3-arg branch).
        GM_INT_PARAM(explicitStep, 1, 0);
        pattern = first;
        step = explicitStep;
        action = a_thread->Param(2);
        GM_INT_PARAM(lp, 3, 1);
        looped = (lp != 0);
    }

    if (a_sixteenStep) step *= StepsForNoteType(1);
    const bool explicitPattern = n == 4 || (n == 3 && a_thread->Param(1).m_type == GM_INT);
    if (!explicitPattern) step = s->numStepsPerPattern > 0
        ? WrapStep(step, s->numStepsPerPattern) : std::max(0, step);
    SequencerTrigger t;
    t.pattern = pattern;
    t.step = step;
    t.action = action;
    t.looped = looped;
    t.handle = CreateSequencerTriggerHandle(m, pattern, step, looped, action);
    gmVariable handle = t.handle;
    RootGmVariable(m, t.handle);
    InsertTrigger(*s, std::move(t));
    // Confirmed real: setTrigger returns a SEQUENCERTRIGGER native handle,
    // not a bare index (previously PushInt'd our own list index).
    a_thread->Push(handle);
    return GM_OK;
}
int GM_CDECL gmf_setTrigger(gmThread* a_thread) { return SetTriggerImpl(a_thread, false); }
int GM_CDECL gmf_setTrigger16(gmThread* a_thread) { return SetTriggerImpl(a_thread, true); }

// Retail FUN_0016d090 uses FUN_0016cffc to schedule the next boundary
// for the requested note subdivision, not the literal step named by arg0.
int GM_CDECL gmf_setTriggerOnNext(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    gmMachine& m = *a_thread->GetMachine();
    int n = a_thread->GetNumParams();
    if (n < 2 || n > 3) { GM_EXCEPTION_MSG("setTriggerOnNext: expecting 2-3 params"); return GM_EXCEPTION; }
    GM_INT_PARAM(noteType, 0, 0);
    gmVariable action = a_thread->Param(1);
    bool looped = true;
    if (n == 3) { GM_INT_PARAM(lp, 2, 1); looped = (lp != 0); }

    const TriggerPosition pos = NextNotePosition(*s, noteType);
    SequencerTrigger t;
    t.pattern = pos.pattern;
    t.step = pos.step;
    t.action = action;
    t.looped = looped;
    t.handle = CreateSequencerTriggerHandle(m, pos.pattern, pos.step, looped, action);
    gmVariable handle = t.handle;
    RootGmVariable(m, t.handle);
    InsertTrigger(*s, std::move(t));
    a_thread->Push(handle);
    return GM_OK;
}
// Confirmed real: 1 arg, a SEQUENCERTRIGGER handle returned by one of the
// trigger-creation methods, not a bare integer index.
int GM_CDECL gmf_moveTriggerToFront(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmVariable handle = a_thread->Param(0);
    if (handle.m_type == g_sequencerTriggerType) {
        auto& triggers = s->triggers->values;
        for (size_t i = 0; i < triggers.size(); ++i) {
            if (triggers[i].handle.m_type == g_sequencerTriggerType &&
                triggers[i].handle.m_value.m_ref == handle.m_value.m_ref) {
                SequencerTrigger t = triggers[i];
                triggers.erase(triggers.begin() + i);
                // Retail FUN_0016d240 unlinks the matching native node, then
                // walks to the insertion point by (pattern, step). Equal
                // coordinates are passed over, so this moves the handle to
                // the end of its coordinate group rather than the list head.
                InsertTrigger(*s, std::move(t));
                break;
            }
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_clearTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    for (auto& t : s->triggers->values) {
        UnrootGmVariable(*a_thread->GetMachine(), t.handle);
    }
    s->triggers->values.clear();
    return GM_OK;
}
int GM_CDECL gmf_getNumTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt((int)s->triggers->values.size());
    return GM_OK;
}
int GM_CDECL gmf_getWrappedStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(step, 0, 0.0f);
    a_thread->PushInt(WrapStep(step, s->EffectiveLen()));
    return GM_OK;
}
// Retail FUN_0016ccbc always resets the pattern. Its optional flag chooses
// wrapped (default) or direct step assignment; it does not reset timing.
int GM_CDECL gmf_gotoStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    if (a_thread->GetNumParams() < 1 || a_thread->GetNumParams() > 2) return GM_EXCEPTION;
    GM_FLOAT_OR_INT_PARAM(step, 0, 0.0f);
    const int integerStep = (int)step;
    bool wrap = a_thread->GetNumParams() == 1 || a_thread->Param(1).m_value.m_int != 0;
    s->currentPattern = 0;
    s->currentStep = wrap ? (s->numStepsPerPattern > 0
        ? WrapStep(integerStep, s->numStepsPerPattern) : std::max(0, integerStep))
        : integerStep;
    return GM_OK;
}
int GM_CDECL gmf_getCurrentStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->currentStep); // confirmed real: direct field read
    return GM_OK;
}
// Retail FUN_0016d4d0 divides the current step by two, truncating toward zero.
int GM_CDECL gmf_getCurrentStep16(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->currentStep / 2);
    return GM_OK;
}
int GM_CDECL gmf_getCurrentPattern(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushInt(s->currentPattern); // confirmed real: direct field read
    return GM_OK;
}
int GM_CDECL gmf_getMSTillNextStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    a_thread->PushFloat(RetailFselMaxZero(s->msTillNextStep));
    return GM_OK;
}
int GM_CDECL gmf_getMSTillNextStepAccurate(gmThread* a_thread) { return gmf_getMSTillNextStep(a_thread); }
int GM_CDECL gmf_getMSTillNextStep16(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    const float remaining = std::max(0.0f, s->msTillNextStep);
    a_thread->PushFloat((s->currentStep % 2 == 0) ? remaining * 2.0f : remaining);
    return GM_OK;
}
// Retail FUN_0016d3f8 computes the position, but returns the literal
// float zero at 0x0016d3f4 in this build.
int GM_CDECL gmf_getMSTillNextNoteStep(gmThread* a_thread) {
    if (!ThisSequencer(a_thread)) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    a_thread->PushFloat(0.0f);
    return GM_OK;
}
// Retail's getAllTriggers() wraps the trigger linked list itself with
// FUN_0031aa10 and a SEQUENCERTRIGGER element type. The native list wrapper
// keeps the list alive and follows its current nodes, rather than taking a
// snapshot. Shipped callers walk it with
// isValid()/get()/goToNext() -- SequencerHelper.gm's
// SEQUENCERHELPER_printSequencer/_getLastTrigger and ScaleOfTheWorld.gm's
// noteRecorder_addTrigger all do
// `for(it=seq.getAllTriggers();it.isValid();it.goToNext()){local tr=it.get(); ...}`
// then call tr.getStep()/tr.getAction().
int GM_CDECL gmf_getAllTriggers(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    auto* ref = new SequencerTriggerListRefData();
    ref->list = s->triggers;
    if (!ref->list->values.empty())
        ref->currentSerial = ref->list->values.front().serial;
    a_thread->PushNewUser(ref, g_sequencerTriggerListType);
    return GM_OK;
}
int GM_CDECL gmf_getNextNoteStep(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(noteType, 0, 0);
    a_thread->PushInt(NextNotePosition(*s, noteType).step);
    return GM_OK;
}
int GM_CDECL gmf_getNextNotePattern(gmThread* a_thread) {
    SequencerData* s = ThisSequencer(a_thread); if (!s) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(noteType, 0, 0);
    a_thread->PushInt(NextNotePosition(*s, noteType).pattern);
    return GM_OK;
}

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
    (void)a_machine; // the shared native trigger-list owns its handle roots
    auto& all = AllSequencers();
    for (auto it = all.begin(); it != all.end(); ++it) {
        if (*it == data) { all.erase(it); break; }
    }
    delete data;
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

int GM_CDECL gmf_SEQUENCER_GetAllSequencers(gmThread* a_thread) {
    auto* it = new SequencerListRefData();
    it->currentSerial = FirstSequencerSerialAfter(0);
    a_thread->PushNewUser(it, g_sequencerListRefType);
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCER_PauseAllSequencers(gmThread* a_thread) {
    // RETAIL FUN_0016b8a4 clears +0x5c on each currently playing sequencer.
    for (SequencerData* sequencer : AllSequencers())
        if (sequencer) sequencer->isPlaying = false;
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCER_UnpauseAllSequencers(gmThread* a_thread) {
    // RETAIL FUN_0016b954 sets +0x5c on every registered sequencer, including
    // ones that were already stopped before the global unpause call.
    for (SequencerData* sequencer : AllSequencers())
        if (sequencer) sequencer->isPlaying = true;
    return GM_OK;
}
// Retail wrapper 0x000aa7cc calls FUN_0016b87c; PPC at 0x0016b87c is a
// single `blr`, so this native intentionally has no state or stack effect.
int GM_CDECL gmf_SEQUENCER_OnNoteCollect(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCER_SetMasterPulse(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(pulse, 0, 0.0f);
    g_masterPulse = pulse;
    return GM_OK;
}

// ---------------------------------------------------------------------
// SEQUENCERACTION -- retail native userdata with a dynamic property table.
// Retail has three own type-library methods (setTable, tagForRemoval,
// isTaggedForRemoval) plus two inherited registrations, and its GM object
// receiver supports arbitrary script fields such as execute and parent.
// ---------------------------------------------------------------------

SequencerActionData* ThisAction(gmThread* a_thread) {
    return static_cast<SequencerActionData*>(
        a_thread->ThisUserCheckType(g_sequencerActionType));
}

void GM_CDECL SequencerActionGetDot(gmThread* a_thread, gmVariable* a_operands) {
    auto* object = static_cast<gmUserObject*>(
        a_thread->GetMachine()->GetObject(a_operands[0].m_value.m_ref));
    auto* action = object && object->GetType() == g_sequencerActionType
        ? static_cast<SequencerActionData*>(object->m_user) : nullptr;
    gmTableObject* table = action && action->boundTable.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(a_thread->GetMachine()->GetObject(
            action->boundTable.m_value.m_ref)) : nullptr;
    a_operands[0] = table ? table->Get(a_operands[1]) : gmVariable::s_null;
}

void GM_CDECL SequencerActionSetDot(gmThread* a_thread, gmVariable* a_operands) {
    auto* object = static_cast<gmUserObject*>(
        a_thread->GetMachine()->GetObject(a_operands[0].m_value.m_ref));
    auto* action = object && object->GetType() == g_sequencerActionType
        ? static_cast<SequencerActionData*>(object->m_user) : nullptr;
    gmTableObject* table = action && action->boundTable.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(a_thread->GetMachine()->GetObject(
            action->boundTable.m_value.m_ref)) : nullptr;
    if (table) {
        table->Set(a_thread->GetMachine(), a_operands[2], a_operands[1]);
    }
}

int GM_CDECL gmf_action_setTable(gmThread* a_thread) {
    SequencerActionData* action = ThisAction(a_thread);
    if (!action) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    action->boundTable = a_thread->Param(0);
    if (action->boundTable.IsReference() && a_thread->GetMachine()->IsGCRunning()) {
        gmObject* object = a_thread->GetMachine()->GetObject(
            action->boundTable.m_value.m_ref);
        if (object) a_thread->GetMachine()->GetGC()->GetNextObject(object);
    }
    return GM_OK;
}

int GM_CDECL gmf_action_tagForRemoval(gmThread* a_thread) {
    SequencerActionData* action = ThisAction(a_thread);
    if (!action) return GM_EXCEPTION;
    action->tagged = true;
    return GM_OK;
}

int GM_CDECL gmf_action_isTaggedForRemoval(gmThread* a_thread) {
    SequencerActionData* action = ThisAction(a_thread);
    a_thread->PushInt(action && action->tagged ? 1 : 0);
    return GM_OK;
}

gmFunctionEntry s_sequencerActionLib[] = {
    {"setTable", gmf_action_setTable, nullptr},
    {"tagForRemoval", gmf_action_tagForRemoval, nullptr},
    {"isTaggedForRemoval", gmf_action_isTaggedForRemoval, nullptr},
};

void GM_CDECL SequencerActionGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete static_cast<SequencerActionData*>(a_object->m_user);
}

#if GM_USE_INCGC
bool GM_CDECL SequencerActionGCTrace(gmMachine* a_machine, gmUserObject* a_object,
                                    gmGarbageCollector* a_gc,
                                    const int, int& a_workDone) {
    auto* action = static_cast<SequencerActionData*>(a_object->m_user);
    if (action && action->boundTable.IsReference()) {
        gmObject* object = a_machine->GetObject(action->boundTable.m_value.m_ref);
        if (object) {
            a_gc->GetNextObject(object);
            ++a_workDone;
        }
    }
    ++a_workDone;
    return true;
}
#else
void GM_CDECL SequencerActionGCMark(gmMachine* a_machine, gmUserObject* a_object,
                                   gmuint32 a_mark) {
    auto* action = static_cast<SequencerActionData*>(a_object->m_user);
    auto mark = [a_machine, a_mark](gmObject* object) {
        if (object && object->NeedsMark(a_mark)) object->Mark(a_machine, a_mark);
    };
    if (action) {
        if (action->boundTable.IsReference())
            mark(a_machine->GetObject(action->boundTable.m_value.m_ref));
    }
}
#endif

int GM_CDECL gmf_SEQUENCERACTION(gmThread* a_thread) {
    const int numParams = a_thread->GetNumParams();
    if (numParams < 0 || numParams > 1) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    auto* action = new SequencerActionData();
    const bool gcWasEnabled = machine->IsGCEnabled();
    machine->EnableGC(false);
    if (numParams == 1) {
        action->boundTable = a_thread->Param(0);
    } else {
        action->boundTable = gmVariable(machine->AllocTableObject());
    }
    a_thread->PushNewUser(action, g_sequencerActionType);
    machine->EnableGC(gcWasEnabled);
    return GM_OK;
}

gmVariable CreateSequencerTriggerHandle(gmMachine& a_machine, int a_pattern,
                                        int a_step, bool a_looped,
                                        const gmVariable& a_action) {
    auto* data = new SequencerTriggerHandle();
    data->pattern = a_pattern;
    data->step = a_step;
    data->looped = a_looped;
    data->action = a_action;
    data->machine = &a_machine;
    RootGmVariable(a_machine, data->action);
    return gmVariable(a_machine.AllocUserObject(data, g_sequencerTriggerType));
}

// Retail SEQUENCERACTION accepts exactly zero or one argument. The optional
// argument is stored at +0x70, the same field used by setTable(), and is the
// table used for resolving execute().
// Retail's four SEQUENCE_* wrappers at 0x000aa83c..0x000aae30 convert
// script sequence tables. The table form stores sixteen two-bit steps in
// each 32-bit integer, lowest step in the low bits. The shipped level JSON
// contains 2, 4, and 16-word patterns (32, 64, and 256 steps). Retail's
// string alphabet at EBOOT.elf 0x003726f4 is "012" after a leading 'z'.
// A blank customSequence is allocated by EntityBase.gm before import, so
// preserve its methods/parent fields and replace only numbered steps.
gmTableObject* SequenceTable(gmMachine& a_machine, const gmVariable& a_value) {
    return a_value.m_type == GM_TABLE
        ? (gmTableObject*)a_machine.GetObject(a_value.m_value.m_ref) : nullptr;
}

int GM_CDECL gmf_SEQUENCE_FromTable(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* source = SequenceTable(m, a_thread->Param(0));
    gmTableObject* dest = SequenceTable(m, a_thread->Param(1));
    if (!source || !dest || source->Count() < 1) {
        a_thread->PushNull();
        return GM_OK;
    }
    int decodedSteps = 0;
    for (int word = 0; word < 16; ++word) {
        gmVariable packed = source->Get(word);
        if (packed.m_type != GM_INT) continue;
        const unsigned bits = (unsigned)packed.m_value.m_int;
        for (int step = 0; step < 16; ++step)
            dest->Set(&m, decodedSteps + step,
                      gmVariable((gmint)((bits >> (step * 2)) & 3)));
        decodedSteps += 16;
    }
    dest->Set(&m, "size", gmVariable((gmint)decodedSteps));
    a_thread->Push(a_thread->Param(1));
    return GM_OK;
}

int GM_CDECL gmf_SEQUENCE_ToString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* source = SequenceTable(m, a_thread->Param(0));
    if (!source) { a_thread->PushNull(); return GM_OK; }
    gmVariable size = source->Get(&m, "size");
    const int count = size.m_type == GM_INT
        ? std::clamp((int)size.m_value.m_int, 0, 1024) : 0;
    std::string encoded = "z";
    encoded.reserve((size_t)count + 1);
    for (int step = 0; step < count; ++step) {
        gmVariable value = source->Get(step);
        if (value.m_type != GM_INT) continue;
        int state = value.m_value.m_int;
        // The fourth byte of retail's "012\0" lookup terminates the
        // returned C string when an otherwise valid two-bit state is 3.
        if (state == 3) break;
        encoded += "012"[(state >= 0 && state <= 2) ? state : 0];
    }
    a_thread->PushString(m.AllocStringObject(encoded.c_str()));
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCE_FromString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* dest = SequenceTable(m, a_thread->Param(1));
    if (a_thread->Param(0).m_type != GM_STRING || !dest) {
        a_thread->PushNull();
        return GM_OK;
    }
    char buffer[2048];
    const char* encoded = a_thread->Param(0).AsString(&m, buffer, sizeof(buffer));
    if (!encoded || encoded[0] != 'z') {
        a_thread->PushNull();
        return GM_OK;
    }
    int step = 0;
    for (const char* c = encoded + 1; *c; ++c, ++step)
        dest->Set(&m, step, gmVariable((gmint)((signed char)*c - '0')));
    dest->Set(&m, "size", gmVariable((gmint)step));
    a_thread->Push(a_thread->Param(1));
    return GM_OK;
}
int GM_CDECL gmf_SEQUENCE_ToTable(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* source = SequenceTable(m, a_thread->Param(0));
    gmTableObject* dest = SequenceTable(m, a_thread->Param(1));
    if (!source || !dest) { a_thread->PushNull(); return GM_OK; }
    gmVariable size = source->Get(&m, "size");
    const int count = size.m_type == GM_INT
        ? std::clamp((int)size.m_value.m_int, 0, 512) : 0;
    std::vector<unsigned> states;
    states.reserve((size_t)count);
    for (int step = 0; step < count; ++step) {
        gmVariable value = source->Get(step);
        if (value.m_type != GM_INT) continue;
        const int state = value.m_value.m_int;
        states.push_back((unsigned)((state >= 0 && state <= 3) ? state : 0));
    }
    for (size_t word = 0; word < states.size() / 16; ++word) {
        unsigned bits = 0;
        for (size_t step = 0; step < 16; ++step)
            bits |= states[word * 16 + step] << (step * 2);
        dest->Set(&m, (int)word, gmVariable((gmint)bits));
    }
    a_thread->Push(a_thread->Param(1));
    return GM_OK;
}

} // namespace

void RegisterSequencerBindings(gmMachine& a_machine) {
    g_sequencerType = a_machine.CreateUserType("SEQUENCER");
    g_sequencerListRefType = a_machine.CreateUserType("SEQUENCERLISTREFCONTAINER");
    g_sequencerActionType = a_machine.CreateUserType("SEQUENCERACTION");
    g_sequencerTriggerType = a_machine.CreateUserType("SEQUENCERTRIGGER");
    g_sequencerTriggerListType = a_machine.CreateUserType("SEQUENCERTRIGGERLISTREFCONTAINER");
    a_machine.RegisterTypeLibrary(g_sequencerType, s_sequencerLib,
                                   sizeof(s_sequencerLib) / sizeof(s_sequencerLib[0]));
    a_machine.RegisterUserCallbacks(g_sequencerType, nullptr, SequencerGCDestruct, nullptr);
    a_machine.RegisterTypeLibrary(g_sequencerActionType, s_sequencerActionLib,
        sizeof(s_sequencerActionLib) / sizeof(s_sequencerActionLib[0]));
#if GM_USE_INCGC
    a_machine.RegisterUserCallbacks(g_sequencerActionType, SequencerActionGCTrace,
        SequencerActionGCDestruct, nullptr);
#else
    a_machine.RegisterUserCallbacks(g_sequencerActionType, SequencerActionGCMark,
        SequencerActionGCDestruct, nullptr);
#endif
    a_machine.RegisterTypeOperator(g_sequencerActionType, O_GETDOT, nullptr,
        SequencerActionGetDot);
    a_machine.RegisterTypeOperator(g_sequencerActionType, O_SETDOT, nullptr,
        SequencerActionSetDot);
    a_machine.RegisterTypeLibrary(g_sequencerTriggerType, s_sequencerTriggerLib,
        sizeof(s_sequencerTriggerLib) / sizeof(s_sequencerTriggerLib[0]));
    a_machine.RegisterUserCallbacks(g_sequencerTriggerType, nullptr,
        SequencerTriggerGCDestruct, nullptr);
    a_machine.RegisterTypeLibrary(g_sequencerTriggerListType, s_sequencerTriggerListLib,
        sizeof(s_sequencerTriggerListLib) / sizeof(s_sequencerTriggerListLib[0]));
    a_machine.RegisterUserCallbacks(g_sequencerTriggerListType, nullptr,
        TriggerListRefGCDestruct, nullptr);
    a_machine.RegisterTypeLibrary(g_sequencerListRefType, s_sequencerListRefLib,
        sizeof(s_sequencerListRefLib) / sizeof(s_sequencerListRefLib[0]));
    a_machine.RegisterUserCallbacks(g_sequencerListRefType, nullptr,
        SequencerListRefGCDestruct, nullptr);
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
    static const bool s_traceSteps =
        std::getenv("GE3_TRACE_SEQUENCER_STEPS") != nullptr;
    // Retail walks its live intrusive list and reads the next node after each
    // sequencer update. Trigger callbacks execute arbitrary GM code, which can
    // construct another sequencer and grow this vector. Index the registry
    // afresh each iteration so a vector reallocation cannot invalidate the
    // active traversal; the growing bound also lets a callback-added
    // sequencer participate in this frame, as a node appended before retail's
    // list sentinel does.
    for (size_t sequencerIndex = 0; sequencerIndex < AllSequencers().size(); ++sequencerIndex) {
        SequencerData* s = AllSequencers()[sequencerIndex];
        if (!s) continue;
        float msPerStep = s->MsPerStep();
        // Retail FUN_0016d8c0 decrements the countdown for every sequencer,
        // even while stopped. It advances at most one step per update and
        // discards excess lateness beyond a whole step. The PPC routine has
        // no positive/finite-duration guard, so preserve its behavior for
        // zero and negative BPM-derived intervals as well.
        s->msTillNextStep -= a_deltaMs;
        if (s->msTillNextStep <= 0.0f) {
            s->msTillNextStep += msPerStep;
            if (s->msTillNextStep < 0.0f) s->msTillNextStep = msPerStep;
            if (!s->isPlaying) continue;
            ++s->currentStep;
            if (s->currentStep < 0) continue;
            if (s->numStepsPerPattern > 0 && s->currentStep >= s->numStepsPerPattern) {
                s->currentStep = 0;
                ++s->currentPattern;
                if (s->currentPattern >= s->loopLen) s->currentPattern = 0;
            }
            if (s_traceSteps) {
                std::fprintf(stderr,
                    "[sequencer-step] serial=%llu pattern=%d step=%d bpm=%d triggers=%zu\n",
                    static_cast<unsigned long long>(s->serial), s->currentPattern,
                    s->currentStep, s->bpm, s->triggers ? s->triggers->values.size() : 0);
            }

            // Prune any trigger a script tagged for removal (SequencerAction.
            // SequencerAction.c) before firing this step's triggers, e.g.
            // ScaleOfTheWorld.gm scanning getAllTriggers() and tagging ones
            // to drop.
            auto& triggers = s->triggers->values;
            for (size_t i = 0; i < triggers.size(); ) {
                if (IsActionTagged(a_machine, triggers[i].action)) {
                    UnrootGmVariable(a_machine, triggers[i].handle);
                    triggers.erase(triggers.begin() + i);
                } else {
                    ++i;
                }
            }

            // Retail FUN_0016d580 collects due actions, removes one-shot
            // triggers, then invokes callbacks. A callback can add or tag a
            // trigger without invalidating this list walk.
            struct DueAction { gmVariable action; bool restart; };
            std::vector<DueAction> dueActions;
            gmTableObject* dueRoot = nullptr;
            for (size_t i = 0; i < triggers.size(); ) {
                SequencerTrigger& t = triggers[i];
                if (t.pattern == s->currentPattern && t.step == s->currentStep) {
                    if (t.restartAction || !IsActionTagged(a_machine, t.action)) {
                        if (!t.restartAction) {
                            if (!dueRoot) {
                                dueRoot = a_machine.AllocTableObject();
                                RootGmVariable(a_machine, gmVariable(dueRoot));
                            }
                            dueRoot->Set(&a_machine, (int)dueActions.size(), t.action);
                        }
                        dueActions.push_back({t.action, t.restartAction});
                    }
                    if (!t.looped) {
                        UnrootGmVariable(a_machine, t.handle);
                        triggers.erase(triggers.begin() + i);
                        continue;
                    }
                }
                ++i;
            }
            for (const DueAction& due : dueActions) {
                if (due.restart) {
                    s->currentPattern = 0;
                    s->currentStep = -1;
                    s->isPlaying = true;
                } else {
                    FireTrigger(a_machine, due.action);
                }
            }
            if (dueRoot) UnrootGmVariable(a_machine, gmVariable(dueRoot));
            for (size_t i = 0; i < triggers.size(); ) {
                if (IsActionTagged(a_machine, triggers[i].action)) {
                    UnrootGmVariable(a_machine, triggers[i].handle);
                    triggers.erase(triggers.begin() + i);
                } else {
                    ++i;
                }
            }
        }
    }
}

gmVariable CreateSequencerInstance(gmMachine& a_machine, int a_initialBpm) {
    SequencerData* data = new SequencerData();
    data->bpm = a_initialBpm;
    data->msPerStep = data->CalculateMsPerStep();
    data->triggers->machine = &a_machine;
    gmUserObject* obj = a_machine.AllocUserObject(data, g_sequencerType);
    data->handle = gmVariable(obj);
    data->serial = g_nextSequencerSerial++;
    AllSequencers().push_back(data);
    return data->handle;
}

float GetMainSequencerMsPerStep(gmMachine& a_machine) {
    gmTableObject* globals = a_machine.GetGlobals();
    if (!globals) return 0.0f;
    const gmVariable sequencer = globals->Get(&a_machine, "MainSequencer");
    if (sequencer.m_type != g_sequencerType) return 0.0f;
    gmObject* object = a_machine.GetObject(sequencer.m_value.m_ref);
    gmUserObject* user = object ? static_cast<gmUserObject*>(object) : nullptr;
    if (!user || user->m_userType != g_sequencerType || !user->m_user) return 0.0f;
    return static_cast<SequencerData*>(user->m_user)->MsPerStep();
}

void StopMainSequencerForProcessListReset(gmMachine& a_machine) {
    gmTableObject* globals = a_machine.GetGlobals();
    if (!globals) return;
    const gmVariable sequencer = globals->Get(&a_machine, "MainSequencer");
    if (sequencer.m_type != g_sequencerType) return;
    gmObject* object = a_machine.GetObject(sequencer.m_value.m_ref);
    auto* user = object ? static_cast<gmUserObject*>(object) : nullptr;
    if (!user || user->m_userType != g_sequencerType || !user->m_user) return;
    static_cast<SequencerData*>(user->m_user)->isPlaying = false;
}
