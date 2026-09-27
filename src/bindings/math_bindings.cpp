#include "gmThread.h"
#include "math_bindings.h"
#include <cmath>
#include <algorithm>

namespace {

float Sign(float a_v) { return (a_v > 0.0f) ? 1.0f : (a_v < 0.0f ? -1.0f : 0.0f); }

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
int GM_CDECL gmf_MATH_COS(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::cos(x));
    return GM_OK;
}
// (ang, curve) -- sign-preserving power curve applied to cos(ang) (the
// "curve" easing-exponent idiom also seen in ENVELOPE.setCurve/
// MATHUTIL_*); pow() of a negative base with a non-integer exponent is
// NaN in C, so the magnitude and sign are separated rather than calling
// pow(cos(ang), curve) directly.
int GM_CDECL gmf_MATH_COS_POW(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
    float c = std::cos(ang);
    a_thread->PushFloat(Sign(c) * std::pow(std::fabs(c), curve));
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
    a_thread->PushFloat(std::pow(a, b));
    return GM_OK;
}
int GM_CDECL gmf_MATH_REAL(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(x);
    return GM_OK;
}
int GM_CDECL gmf_MATH_ROUND(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushInt((gmint)std::lround(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_SIGN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(Sign(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_SIN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::sin(x));
    return GM_OK;
}
int GM_CDECL gmf_MATH_SIN_POW(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
    float s = std::sin(ang);
    a_thread->PushFloat(Sign(s) * std::pow(std::fabs(s), curve));
    return GM_OK;
}
int GM_CDECL gmf_MATH_SQRT(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushFloat(std::sqrt(std::max(x, 0.0f)));
    return GM_OK;
}
// IsNull(x) -- confirmed real (10 call sites, e.g.
// Platformer/EntityPacks/TutorialGraphicsEntity.gm's
// `if (IsNull(this.configureData.pngIndex)) { this.configureData.pngIndex=0; }`)
// but never registered -- silently null-called to null (not
// false/true!) via leniency, and `if (null)` is falsy in GM, so the
// "was actually null, reset it to 0" branch was ALWAYS skipped, leaving
// a genuinely-null pngIndex to index TutorialGraphics_PNGList with,
// crashing the very next line ("getdot failed" on `this.ad.filename`
// once `this.ad` came back null). A plain type check, equivalent to `!?x`.
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
}
