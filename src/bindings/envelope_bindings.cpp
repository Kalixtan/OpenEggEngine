#include "gmThread.h"
#include "envelope_bindings.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace {

gmType g_envelopeType = -1;

struct EnvelopePoint { float time = 0.0f, value = 0.0f; };

struct EnvelopeData {
    std::vector<EnvelopePoint> points;
    float currentTime = 0.0f;
    bool looping = false;
    float curve = 0.0f; // setCurve's value, stored but not consumed (no real usage reads it back)
};

EnvelopeData* ThisEnvelope(gmThread* a_thread) {
    return (EnvelopeData*)a_thread->ThisUserCheckType(g_envelopeType);
}

float MaxTime(const EnvelopeData& e) {
    float m = 0.0f;
    for (auto& p : e.points) m = std::max(m, p.time);
    return m;
}

int GM_CDECL gmf_ENVELOPE(gmThread* a_thread) {
    a_thread->PushNewUser(new EnvelopeData(), g_envelopeType);
    return GM_OK;
}

int GM_CDECL gmf_getCurrentTime(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    a_thread->PushFloat(e->currentTime);
    return GM_OK;
}
int GM_CDECL gmf_setTime(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(t, 0, 0.0f);
    e->currentTime = t;
    return GM_OK;
}
int GM_CDECL gmf_getMaxTime(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    a_thread->PushFloat(MaxTime(*e));
    return GM_OK;
}
int GM_CDECL gmf_getNumPoints(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    a_thread->PushInt((int)e->points.size());
    return GM_OK;
}
int GM_CDECL gmf_getPointTime(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(index, 0, 0);
    a_thread->PushFloat((index >= 0 && index < (int)e->points.size()) ? e->points[(size_t)index].time : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_isLooping(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    a_thread->PushInt(e->looping ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsLooping(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    e->looping = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_setCurve(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(c, 0, 0.0f);
    e->curve = c;
    return GM_OK;
}
int GM_CDECL gmf_appendPoint(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(t, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(v, 1, 0.0f);
    e->points.push_back({t, v});
    return GM_OK;
}
// Same (time, value) shape -- inserted in sorted time order rather than
// always appended to the end (the distinction "add" vs "append" implies).
int GM_CDECL gmf_addPoint(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(t, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(v, 1, 0.0f);
    EnvelopePoint p{t, v};
    auto it = std::upper_bound(e->points.begin(), e->points.end(), p,
                                [](const EnvelopePoint& a, const EnvelopePoint& b) { return a.time < b.time; });
    e->points.insert(it, p);
    return GM_OK;
}
int GM_CDECL gmf_copy(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_USER_PARAM(EnvelopeData*, g_envelopeType, other, 0);
    *e = *other;
    return GM_OK;
}

int GM_CDECL gmf_update(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(dt, 0, 0.0f);
    e->currentTime += dt;
    if (e->looping) {
        float maxT = MaxTime(*e);
        if (maxT > 0.0f) e->currentTime = std::fmod(e->currentTime, maxT);
    }
    return GM_OK;
}
int GM_CDECL gmf_isDone(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    a_thread->PushInt(e->currentTime >= MaxTime(*e) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_reset(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    e->currentTime = 0.0f;
    return GM_OK;
}

int GM_CDECL gmf_getValue(gmThread* a_thread) {
    EnvelopeData* e = ThisEnvelope(a_thread); if (!e) return GM_EXCEPTION;
    if (e->points.empty()) { a_thread->PushFloat(0.0f); return GM_OK; }
    if (e->points.size() == 1 || e->currentTime <= e->points.front().time) {
        a_thread->PushFloat(e->points.front().value);
        return GM_OK;
    }
    if (e->currentTime >= e->points.back().time) {
        a_thread->PushFloat(e->points.back().value);
        return GM_OK;
    }
    for (size_t i = 1; i < e->points.size(); ++i) {
        const EnvelopePoint& a = e->points[i - 1];
        const EnvelopePoint& b = e->points[i];
        if (e->currentTime <= b.time) {
            float span = b.time - a.time;
            float t = (span > 0.0f) ? (e->currentTime - a.time) / span : 0.0f;
            a_thread->PushFloat(a.value + (b.value - a.value) * t);
            return GM_OK;
        }
    }
    a_thread->PushFloat(e->points.back().value);
    return GM_OK;
}

gmFunctionEntry s_envelopeLib[] = {
    {"getCurrentTime", gmf_getCurrentTime, nullptr},
    {"setTime", gmf_setTime, nullptr},
    {"getMaxTime", gmf_getMaxTime, nullptr},
    {"getNumPoints", gmf_getNumPoints, nullptr},
    {"getPointTime", gmf_getPointTime, nullptr},
    {"isLooping", gmf_isLooping, nullptr},
    {"setIsLooping", gmf_setIsLooping, nullptr},
    {"setCurve", gmf_setCurve, nullptr},
    {"appendPoint", gmf_appendPoint, nullptr},
    {"addPoint", gmf_addPoint, nullptr},
    {"copy", gmf_copy, nullptr},
    {"update", gmf_update, nullptr},
    {"isDone", gmf_isDone, nullptr},
    {"reset", gmf_reset, nullptr},
    {"getValue", gmf_getValue, nullptr},
};

void GM_CDECL EnvelopeGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    delete (EnvelopeData*)a_object->m_user;
}

} // namespace

void RegisterEnvelopeBindings(gmMachine& a_machine) {
    g_envelopeType = a_machine.CreateUserType("Envelope");
    a_machine.RegisterTypeLibrary(g_envelopeType, s_envelopeLib, sizeof(s_envelopeLib) / sizeof(s_envelopeLib[0]));
    a_machine.RegisterUserCallbacks(g_envelopeType, nullptr, EnvelopeGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("ENVELOPE", gmf_ENVELOPE);
}
