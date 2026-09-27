#include "gmThread.h"
#include "pulseperc_bindings.h"
#include <algorithm>

namespace {

gmType g_pulsePercType = -1;

enum class Phase { IDLE, ATTACK, SUSTAIN, DECAY };

// Confirmed real fields (setByRate/setByTime/setSustainTime/setValue/
// isByRate/getAttackRate/getAttackTime/getDecayRate/getDecayTime/
// needed for update()/getValue()/isDone()/reset() (confirmed real by
// usage, no decompiled body -- see header comment).
struct PulsePercData {
    bool byRate = false;      // isByRate() / set by setByRate() vs setByTime()
    float attackRate = 0.0f;  // units/sec when byRate; getAttackRate()
    float decayRate = 0.0f;   // units/sec when byRate; getDecayRate()
    float attackTime = 0.0f;  // ms when !byRate; getAttackTime()
    float decayTime = 0.0f;   // ms when !byRate; getDecayTime()
    float sustainTime = 0.0f; // ms hold at peak between attack and decay; getSustainTime()/setSustainTime()

    Phase phase = Phase::IDLE;
    float value = 0.0f;         // current 0..1 value, getValue()/setValue()
    float phaseElapsedMs = 0.0f;
};

PulsePercData* ThisPulsePerc(gmThread* a_thread) {
    return (PulsePercData*)a_thread->ThisUserCheckType(g_pulsePercType);
}

int GM_CDECL gmf_PULSEPERC(gmThread* a_thread) {
    PulsePercData* data = new PulsePercData();
    a_thread->PushNewUser(data, g_pulsePercType);
    return GM_OK;
}

int GM_CDECL gmf_pulse(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    p->phase = Phase::ATTACK;
    p->phaseElapsedMs = 0.0f;
    return GM_OK;
}

int GM_CDECL gmf_setValue(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->value = v;
    return GM_OK;
}

int GM_CDECL gmf_getAttackRate(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->attackRate);
    return GM_OK;
}
int GM_CDECL gmf_getAttackTime(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->attackTime);
    return GM_OK;
}
int GM_CDECL gmf_getDecayRate(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->decayRate);
    return GM_OK;
}
int GM_CDECL gmf_getDecayTime(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->decayTime);
    return GM_OK;
}
int GM_CDECL gmf_isByRate(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushInt(p->byRate ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getSustainTime(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->sustainTime);
    return GM_OK;
}
int GM_CDECL gmf_setSustainTime(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    p->sustainTime = v;
    return GM_OK;
}
// Confirmed real: 2 args (attackRate, decayRate) -- units/sec, e.g. real
// usage `setByRate(61,61)` -- a full 0..1 sweep in ~1000/61 ~= 16ms, about
// one frame at 60fps, matching how these are used for snappy UI pulses.
int GM_CDECL gmf_setByRate(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(attack, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(decay, 1, 0.0f);
    p->attackRate = attack;
    p->decayRate = decay;
    p->byRate = true;
    return GM_OK;
}
// Confirmed real: 2 args (attackTime, decayTime) in ms -- real usage
// `setByTime(msPerStep*2*2, msPerStep*2*2)` (Main.gm:653) confirms ms units
// (msPerStep comes straight from SEQUENCER's BPM-derived timing).
int GM_CDECL gmf_setByTime(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(attack, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(decay, 1, 0.0f);
    p->attackTime = attack;
    p->decayTime = decay;
    p->byRate = false;
    return GM_OK;
}

// --- confirmed-real-by-usage, no decompiled body (see header comment) ---

int GM_CDECL gmf_update(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(deltaMs, 0, 0.0f);

    if (p->phase == Phase::ATTACK) {
        if (p->byRate) {
            p->value += (p->attackRate / 1000.0f) * deltaMs;
            if (p->value >= 1.0f) {
                p->value = 1.0f;
                p->phase = p->sustainTime > 0.0f ? Phase::SUSTAIN : Phase::DECAY;
                p->phaseElapsedMs = 0.0f;
            }
        } else {
            p->phaseElapsedMs += deltaMs;
            p->value = p->attackTime > 0.0f ? std::min(1.0f, p->phaseElapsedMs / p->attackTime) : 1.0f;
            if (p->phaseElapsedMs >= p->attackTime) {
                p->value = 1.0f;
                p->phase = p->sustainTime > 0.0f ? Phase::SUSTAIN : Phase::DECAY;
                p->phaseElapsedMs = 0.0f;
            }
        }
    } else if (p->phase == Phase::SUSTAIN) {
        p->phaseElapsedMs += deltaMs;
        if (p->phaseElapsedMs >= p->sustainTime) {
            p->phase = Phase::DECAY;
            p->phaseElapsedMs = 0.0f;
        }
    } else if (p->phase == Phase::DECAY) {
        if (p->byRate) {
            p->value -= (p->decayRate / 1000.0f) * deltaMs;
            if (p->value <= 0.0f) {
                p->value = 0.0f;
                p->phase = Phase::IDLE;
            }
        } else {
            p->phaseElapsedMs += deltaMs;
            p->value = p->decayTime > 0.0f ? std::max(0.0f, 1.0f - p->phaseElapsedMs / p->decayTime) : 0.0f;
            if (p->phaseElapsedMs >= p->decayTime) {
                p->value = 0.0f;
                p->phase = Phase::IDLE;
            }
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_getValue(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->value);
    return GM_OK;
}

int GM_CDECL gmf_isDone(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushInt(p->phase == Phase::IDLE ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_reset(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    p->phase = Phase::IDLE;
    p->value = 0.0f;
    p->phaseElapsedMs = 0.0f;
    return GM_OK;
}

gmFunctionEntry s_pulsePercLib[] = {
    {"pulse", gmf_pulse, nullptr},
    {"setValue", gmf_setValue, nullptr},
    {"getAttackRate", gmf_getAttackRate, nullptr},
    {"getAttackTime", gmf_getAttackTime, nullptr},
    {"getDecayRate", gmf_getDecayRate, nullptr},
    {"getDecayTime", gmf_getDecayTime, nullptr},
    {"isByRate", gmf_isByRate, nullptr},
    {"getSustainTime", gmf_getSustainTime, nullptr},
    {"setSustainTime", gmf_setSustainTime, nullptr},
    {"setByRate", gmf_setByRate, nullptr},
    {"setByTime", gmf_setByTime, nullptr},
    {"update", gmf_update, nullptr},
    {"getValue", gmf_getValue, nullptr},
    {"isDone", gmf_isDone, nullptr},
    {"reset", gmf_reset, nullptr},
};

void GM_CDECL PulsePercGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (PulsePercData*)a_object->m_user;
}

} // namespace

void RegisterPulsePercBindings(gmMachine& a_machine) {
    g_pulsePercType = a_machine.CreateUserType("PULSEPERC");
    a_machine.RegisterTypeLibrary(g_pulsePercType, s_pulsePercLib,
                                   sizeof(s_pulsePercLib) / sizeof(s_pulsePercLib[0]));
    a_machine.RegisterUserCallbacks(g_pulsePercType, nullptr, PulsePercGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("PULSEPERC", gmf_PULSEPERC);
}
