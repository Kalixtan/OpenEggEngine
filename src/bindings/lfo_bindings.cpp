#include "gmThread.h"
#include "lfo_bindings.h"
#include <cstdlib>

namespace {

gmType g_triangleLfoType = -1;

struct TriangleLfoData {
    float periodMs = 1000.0f;  // setRate()/getRate()
    float rangeMin = 0.0f;     // setRange()
    float rangeMax = 1.0f;
    float contour = 0.5f;      // setContour() -- fraction of period spent rising
    float phase01 = 0.0f;      // 0..1 through the current cycle
    float value = 0.0f;        // getValue()
};

TriangleLfoData* ThisTriangleLfo(gmThread* a_thread) {
    return (TriangleLfoData*)a_thread->ThisUserCheckType(g_triangleLfoType);
}

void RecomputeValue(TriangleLfoData* p) {
    float rise = p->contour;
    if (rise <= 0.0f) rise = 0.0001f;
    if (rise >= 1.0f) rise = 0.9999f;
    float t;
    if (p->phase01 < rise) {
        t = p->phase01 / rise;
    } else {
        t = 1.0f - (p->phase01 - rise) / (1.0f - rise);
    }
    p->value = p->rangeMin + (p->rangeMax - p->rangeMin) * t;
}

int GM_CDECL gmf_TRIANGLELFO(gmThread* a_thread) {
    TriangleLfoData* data = new TriangleLfoData();
    RecomputeValue(data);
    a_thread->PushNewUser(data, g_triangleLfoType);
    return GM_OK;
}

int GM_CDECL gmf_setRate(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 1000.0f);
    p->periodMs = v;
    return GM_OK;
}

int GM_CDECL gmf_getRate(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->periodMs);
    return GM_OK;
}

int GM_CDECL gmf_setRange(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(b, 1, 1.0f);
    p->rangeMin = a;
    p->rangeMax = b;
    RecomputeValue(p);
    return GM_OK;
}

int GM_CDECL gmf_setPhase(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    float ph = v;
    ph -= (float)(long)ph;
    if (ph < 0.0f) ph += 1.0f;
    p->phase01 = ph;
    RecomputeValue(p);
    return GM_OK;
}

int GM_CDECL gmf_setRandomPhase(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    p->phase01 = (float)std::rand() / (float)RAND_MAX;
    RecomputeValue(p);
    return GM_OK;
}

int GM_CDECL gmf_setContour(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.5f);
    p->contour = v;
    RecomputeValue(p);
    return GM_OK;
}


int GM_CDECL gmf_update(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(deltaMs, 0, 0.0f);
    if (p->periodMs > 0.0f) {
        p->phase01 += deltaMs / p->periodMs;
        p->phase01 -= (float)(long)p->phase01;
        if (p->phase01 < 0.0f) p->phase01 += 1.0f;
    }
    RecomputeValue(p);
    return GM_OK;
}

int GM_CDECL gmf_getValue(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    a_thread->PushFloat(p->value);
    return GM_OK;
}

int GM_CDECL gmf_reset(gmThread* a_thread) {
    TriangleLfoData* p = ThisTriangleLfo(a_thread); if (!p) return GM_EXCEPTION;
    p->phase01 = 0.0f;
    RecomputeValue(p);
    return GM_OK;
}

gmFunctionEntry s_triangleLfoLib[] = {
    {"setRate", gmf_setRate, nullptr},
    {"getRate", gmf_getRate, nullptr},
    {"setRange", gmf_setRange, nullptr},
    {"setPhase", gmf_setPhase, nullptr},
    {"setRandomPhase", gmf_setRandomPhase, nullptr},
    {"setContour", gmf_setContour, nullptr},
    {"update", gmf_update, nullptr},
    {"getValue", gmf_getValue, nullptr},
    {"reset", gmf_reset, nullptr},
};

void GM_CDECL TriangleLfoGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (TriangleLfoData*)a_object->m_user;
}

} // namespace

void RegisterLfoBindings(gmMachine& a_machine) {
    g_triangleLfoType = a_machine.CreateUserType("TRIANGLELFO");
    a_machine.RegisterTypeLibrary(g_triangleLfoType, s_triangleLfoLib,
                                   sizeof(s_triangleLfoLib) / sizeof(s_triangleLfoLib[0]));
    a_machine.RegisterUserCallbacks(g_triangleLfoType, nullptr, TriangleLfoGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("TRIANGLELFO", gmf_TRIANGLELFO);
}
