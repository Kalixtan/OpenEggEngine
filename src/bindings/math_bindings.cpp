#include "gmThread.h"
#include "math_bindings.h"
#include "../ported_native/ground_math.h"
#include <cmath>
#include <algorithm>

namespace {

float Sign(float a_v) { return (a_v > 0.0f) ? 1.0f : (a_v < 0.0f ? -1.0f : 0.0f); }

float RetailSinCos(float a_angle, bool a_cosine) {
    return GroundRenderMath::RetailGroundSinCos(a_angle, a_cosine);
}

float RetailSinCosPow(float a_value, float a_curve) {
    const float magnitude = std::fabs(a_value);
    const float powered = GroundRenderMath::RetailPlayerPow(magnitude, a_curve);
    // Retail negates only when the raw trig result compares below zero. This
    // preserves +1 for pow(-0,0) and NaN propagation for unordered compares.
    return a_value < 0.0f ? -powered : powered;
}

int GM_CDECL gmf_MATH_ABS(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::fabs(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_ACOS(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::acos(std::clamp(x, -1.0f, 1.0f)));
    return GM_OK;
}
float CosPow(float ang, float curve) {
    return RetailSinCosPow(RetailSinCos(ang, true), curve);
}
// Verification: retail MATH_SIN(ang,curve) and MATH_SIN_POW use selector 0
// and the same absolute-power/sign restoration sequence as MATH_COS_POW.
float SinPow(float ang, float curve) {
    return RetailSinCosPow(RetailSinCos(ang, false), curve);
}
// Verification: retail Math.c @ 0xa6628 dispatches one argument to cosine
// and two arguments to CosPow; use the PPC-derived trig helper in both cases.
int GM_CDECL gmf_MATH_COS(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f);
    if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
        a_thread->PushFloat(CosPow(ang, curve));
    } else {
        a_thread->PushFloat(RetailSinCos(ang, true));
    }
    return GM_OK;
}
int GM_CDECL gmf_MATH_COS_POW(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
    a_thread->PushFloat(CosPow(ang, curve));
    return GM_OK;
}
int GM_CDECL gmf_MATH_EXP(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::exp(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_FLOOR(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::floor(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_INT(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushInt((gmint)x);
    return GM_OK;
}
int GM_CDECL gmf_MATH_MAX(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
    a_thread->PushFloat(std::max(a, b));
    return GM_OK;
}
int GM_CDECL gmf_MATH_MAXI(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_INT_PARAM(a, 0, 0); GM_INT_PARAM(b, 1, 0);
    a_thread->PushInt(std::max(a, b));
    return GM_OK;
}
int GM_CDECL gmf_MATH_MIN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
    a_thread->PushFloat(std::min(a, b));
    return GM_OK;
}
int GM_CDECL gmf_MATH_MINI(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_INT_PARAM(a, 0, 0); GM_INT_PARAM(b, 1, 0);
    a_thread->PushInt(std::min(a, b));
    return GM_OK;
}
int GM_CDECL gmf_MATH_MOD(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(b, 1, 1.0f);
    a_thread->PushFloat(std::fmod(a, b));
    return GM_OK;
}
int GM_CDECL gmf_MATH_POW(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(b, 1, 1.0f);
    a_thread->PushFloat(GroundRenderMath::RetailPlayerPow(a, b));
    return GM_OK;
}
int GM_CDECL gmf_MATH_REAL(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(x);
    return GM_OK;
}
int GM_CDECL gmf_MATH_ROUND(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushInt((gmint)(x + 0.5f));
    return GM_OK;
}
int GM_CDECL gmf_MATH_SIGN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushInt((gmint)Sign(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_SIN(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f);
    if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
        a_thread->PushFloat(SinPow(ang, curve));
    } else {
        a_thread->PushFloat(RetailSinCos(ang, false));
    }
    return GM_OK;
}
int GM_CDECL gmf_MATH_SIN_POW(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
    a_thread->PushFloat(SinPow(ang, curve));
    return GM_OK;
}
int GM_CDECL gmf_MATH_SQRT(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::sqrt(std::max(x, 0.0f)));
    return GM_OK;
}
int GM_CDECL gmf_MATH_TAN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::tan(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_ASIN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::asin(std::clamp(x, -1.0f, 1.0f)));
    return GM_OK;
}
int GM_CDECL gmf_MATH_ATAN2(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(y, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(x, 1, 0.0f);
    a_thread->PushFloat(std::atan2(y, x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_LN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::log(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_LOG(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::log(x));
    return GM_OK;
}
int GM_CDECL gmf_IsNull(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    a_thread->PushInt(a_thread->Param(0).m_type == GM_NULL ? 1 : 0);
    return GM_OK;
}

} // namespace

void RegisterMathBindings(gmMachine& a_machine) {
    a_machine.GetGlobals()->Set(&a_machine, "IsNull", gmVariable(a_machine.AllocFunctionObject(gmf_IsNull)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_ABS", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_ABS)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_ACOS", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_ACOS)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_COS", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_COS)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_COS_POW", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_COS_POW)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_EXP", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_EXP)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_FLOOR", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_FLOOR)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_INT", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_INT)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_MAX", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_MAX)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_MAXI", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_MAXI)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_MIN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_MIN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_MINI", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_MINI)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_MOD", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_MOD)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_POW", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_POW)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_REAL", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_REAL)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_ROUND", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_ROUND)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_SIGN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_SIGN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_SIN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_SIN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_SIN_POW", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_SIN_POW)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_SQRT", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_SQRT)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_TAN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_TAN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_ASIN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_ASIN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_ATAN2", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_ATAN2)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_LN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_LN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_LOG", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_LOG)));
}
