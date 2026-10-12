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
// overloaded in retail -- called with 1 arg it's plain cos(ang); called
// with 2 args it computes the exact same sign-preserving power-curve as
// the separate MATH_COS_POW function below (same helper calls, same
// "negate if raw cos < 0" branch). No confirmed corpus call site actually
// uses the 2-arg form of bare MATH_COS (every real curve call goes through
// MATH_COS_POW/MATH_SIN_POW instead), but the retail dispatch is
// unambiguous from the decompiled arg-count branch, so it's honored here
// too rather than silently throwing GM_EXCEPTION on 2 args.
float CosPow(float ang, float curve) {
    float c = std::cos(ang);
    return Sign(c) * std::pow(std::fabs(c), curve);
}
float SinPow(float ang, float curve) {
    float s = std::sin(ang);
    return Sign(s) * std::pow(std::fabs(s), curve);
}
int GM_CDECL gmf_MATH_COS(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f);
    if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
        a_thread->PushFloat(CosPow(ang, curve));
    } else {
        a_thread->PushFloat(std::cos(ang));
    }
    return GM_OK;
}
// (ang, curve) -- sign-preserving power curve applied to cos(ang) (the
// "curve" easing-exponent idiom also seen in ENVELOPE.setCurve/
// MATHUTIL_*); pow() of a negative base with a non-integer exponent is
// NaN in C, so the magnitude and sign are separated rather than calling
// @ 0xa6858, and identically inlined into 2-arg MATH_COS @ 0xa6628 above).
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
// both arguments accept float or int and the result is pushed as float. The
// retail helper 0x1dc1c0 is a custom float-oriented power implementation
// matches the mathematical operation, but its exact rounding and exceptional
// cases are not asserted to match the PowerPC helper bit for bit.
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
// FIXED (this session): was `std::lround(x)` (round-half-away-from-zero).
// `(int)(x + DAT_000a73d8)` where DAT_000a73d8 was extracted directly from
// the retail EBOOT.elf at that address (bytes 3f000000 = 0.5f) -- i.e. the
// naive "add 0.5 then truncate toward zero" trick, which is NOT symmetric
// for negative numbers (e.g. retail MATH_ROUND(-2.5) == -2, not -3;
// MATH_ROUND(-2.9) == -2 as well, since -2.9+0.5=-2.4 truncates to -2).
// std::lround would give -3 and -3 respectively. Not exercised by any
// corpus call site with a negative half-integer, but this is the confirmed
// retail formula, bug-for-bug.
int GM_CDECL gmf_MATH_ROUND(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushInt((gmint)(x + 0.5f));
    return GM_OK;
}
// FIXED (this session): was pushing a float. Confirmed real
// (float) -- retail MATH_SIGN returns an int (-1/0/1), and the compare
// threshold DAT_000a74a4 was extracted from the EBOOT.elf as 0.0f, matching
// the existing Sign() helper's own >0/<0/==0 split exactly.
int GM_CDECL gmf_MATH_SIGN(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    a_thread->PushInt((gmint)Sign(x));
    return GM_OK;
}
// MATH_SIN is arity-overloaded the same way MATH_COS is -- see comment
int GM_CDECL gmf_MATH_SIN(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    GM_FLOAT_OR_INT_PARAM(ang, 0, 0.0f);
    if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(curve, 1, 1.0f);
        a_thread->PushFloat(SinPow(ang, curve));
    } else {
        a_thread->PushFloat(std::sin(ang));
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
// plain 1-arg (2 for ATAN2) libm wrappers, same shape as MATH_SIN/COS/SQRT
// above. Previously entirely unregistered (silent null-call no-op).
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
// Confirmed against retail Math.MATH_LN (0xa6468) and MATH_LOG (0xa6544):
// both call 0x1dcfb0 with the same arguments. The helper body and EBOOT
// 0x3f317216 is ln(2), and the normalized-mantissa polynomial plus exponent
// term computes the natural logarithm. Both retail bindings therefore share
// natural-log semantics, matching std::log for ordinary finite inputs. Exact
// PowerPC float rounding and exceptional-value behavior remain unverified.
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
    a_machine.GetGlobals()->Set(&a_machine, "MATH_TAN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_TAN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_ASIN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_ASIN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_ATAN2", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_ATAN2)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_LN", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_LN)));
    a_machine.GetGlobals()->Set(&a_machine, "MATH_LOG", gmVariable(a_machine.AllocFunctionObject(gmf_MATH_LOG)));
}
