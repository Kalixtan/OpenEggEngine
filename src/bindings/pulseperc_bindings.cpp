#include "gmThread.h"
#include "pulseperc_bindings.h"
#include "../core/engine_context.h"
#include <algorithm>
#include <cmath>

namespace {

gmType g_pulsePercType = -1;

struct PulsePercData {
    float attackRate = 171.0f;
    float attackTime = 131.0f;
    float decayRate = 171.0f;
    float decayTime = 131.0f;
    bool byRate = true;
    float sustainTime = 0.0f;
    float phase = -1.0f; // retail field +0x18: -1 decay/idle, +1 attack
    float value = 0.0f;  // retail field +0x1c
    float phaseElapsedMs = 0.0f;
    float lastDeltaMs = 0.0f;
};

float RetailExp(float rate, float deltaMs) {
    return std::clamp(std::exp(-rate * deltaMs), 0.0f, 1.0f);
}

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
    p->phase = 1.0f;
    p->phaseElapsedMs = 0.0f;
    // Retail pulse() preserves the current value when retriggered.
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
int GM_CDECL gmf_setByRate(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    const auto asDouble = [](const gmVariable& value) {
        return value.m_type == GM_FLOAT
            ? static_cast<double>(value.m_value.m_float)
            : static_cast<double>(value.m_value.m_int);
    };
    p->attackRate = static_cast<float>(1.0 / asDouble(a_thread->Param(0)));
    p->decayRate = static_cast<float>(1.0 / asDouble(a_thread->Param(1)));
    p->byRate = true;
    return GM_OK;
}
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

int GM_CDECL gmf_update(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(deltaMs, 0, 0.0f);

    p->lastDeltaMs = deltaMs;
    if (p->byRate) {
        if (p->phase >= 0.0f) {
            const float change = (1.0f - p->value) *
                (1.0f - RetailExp(p->attackRate, deltaMs));
            p->value += change;
            if (std::fabs(change) < 0.01f) {
                p->phaseElapsedMs += deltaMs;
                if (p->phaseElapsedMs >= p->sustainTime) p->phase = -1.0f;
            }
        } else {
            p->value *= RetailExp(p->decayRate, deltaMs);
        }
    } else if (p->phase >= 1.0f) {
        const float attackReciprocal = 1.0f / p->attackTime;
        const float attackValue = std::fma(attackReciprocal, deltaMs, p->value);
        p->value = attackValue >= 1.0f ? 1.0f : attackValue;
        if (p->value >= 1.0f) {
            p->phaseElapsedMs += deltaMs;
            if (p->phaseElapsedMs >= p->sustainTime) p->phase = -1.0f;
        }
    } else {
        // Retail divides 1 by decayTime with `fdivs`, then uses
        // `fnmsubs reciprocal, deltaMs, value` and fsel-clamps at zero.
        const float decayReciprocal = 1.0f / p->decayTime;
        const float decayValue = std::fma(-decayReciprocal, deltaMs, p->value);
        p->value = decayValue >= 0.0f ? decayValue : 0.0f;
    }
    return GM_OK;
}

int GM_CDECL gmf_getValue(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->value);
    return GM_OK;
}

int GM_CDECL gmf_getDir(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->phase);
    return GM_OK;
}

int GM_CDECL gmf_isDone(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    const bool done = p->byRate
        ? p->phase < 0.0f && p->value * RetailExp(p->decayRate, p->lastDeltaMs) < 0.01f
        : p->phase < 0.0f && p->value <= 0.0f;
    a_thread->PushInt(done ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_reset(gmThread* a_thread) {
    PulsePercData* p = ThisPulsePerc(a_thread); if (!p) return GM_EXCEPTION;
    p->phase = -1.0f;
    p->value = 0.0f;
    // Retail reset does not clear the elapsed-time field.
    return GM_OK;
}

gmFunctionEntry s_pulsePercLib[] = {
    {"update", gmf_update, nullptr},
    {"isDone", gmf_isDone, nullptr},
    {"pulse", gmf_pulse, nullptr},
    {"reset", gmf_reset, nullptr},
    {"getDir", gmf_getDir, nullptr},
    {"getValue", gmf_getValue, nullptr},
    {"setValue", gmf_setValue, nullptr},
    {"setByRate", gmf_setByRate, nullptr},
    {"setByTime", gmf_setByTime, nullptr},
    {"getAttackRate", gmf_getAttackRate, nullptr},
    {"getAttackTime", gmf_getAttackTime, nullptr},
    {"getDecayRate", gmf_getDecayRate, nullptr},
    {"getDecayTime", gmf_getDecayTime, nullptr},
    {"isByRate", gmf_isByRate, nullptr},
    {"setSustainTime", gmf_setSustainTime, nullptr},
    {"getSustainTime", gmf_getSustainTime, nullptr},
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
