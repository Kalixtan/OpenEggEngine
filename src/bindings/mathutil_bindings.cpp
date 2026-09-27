#include "gmThread.h"
#include "mathutil_bindings.h"
#include "vector2d_bindings.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace {

constexpr float kPi = 3.14159265358979323846f;

bool ReadVec2(gmThread* a_thread, int a_param, float& a_x, float& a_y) {
    return GetVector2D(*a_thread->GetMachine(), a_thread->Param(a_param), a_x, a_y);
}

// Confirmed real: 3 args (value, min, max) -- clamp. Real usage always
// float-ish (e.g. `MATHUTIL_GETINRANGE(1-this.fadeInPerc,0,1)`).
// Widened from the strict GM_FLOAT_OR_INT_PARAM macro (throws GM_EXCEPTION
// on a non-numeric arg) to the non-throwing ParamFloatOrInt() overload --
// confirmed real, load-bearing gap: TouchEditProcessPS3.gm's cursorTool
// state machine reassigns `this.env`/resets other cursor fields across
// HOVERING/STAMP/MARQUEE state transitions, and ScaleOfTheWorld.gm's own
// update() (`this.fallPerc = MATHUTIL_GETINRANGE(this.fallPerc,0.0,1.0);`)
// observably sees `this.fallPerc` as GM_NULL on some frames in practice
// (confirmed via direct instrumentation: p0.type=0/GM_NULL on a live run) --
// most likely a transient state during the very first update() tick before
// a field a different code path expects is fully settled. Whatever the
// precise cause, a null value being clamped is exactly the "coerce toward
// a sane default instead of hard-aborting the calling function" situation
// this codebase's own GETINRANGEI already established the pattern for
// (see its header comment) -- a null argument is treated as 0.0 rather
// than throwing, so the calling update()'s LATER statements (this frame's
// scale animation, camera logic, etc, all previously aborted mid-function
// by the uncaught exception) keep running instead of silently skipping
// the rest of the frame.
//
// FIXED (this session): was `min(hi, max(lo, v))`, which silently returns
// a wrong (out-of-order) result whenever hi<lo -- e.g. GETINRANGE(5,10,0)
// @ 0xa369c (MATHUTIL_GETINRANGE): retail computes
// `min(max(lo,hi), max(v, (lo<hi) ? max(v,lo) : max(v,hi)))`-shaped algebra
// that reduces, for lo<=hi, to the ordinary clamp(v,lo,hi), but for lo>hi
// clamps against [hi,lo] instead of always trusting arg order -- i.e. it's
// robust to the two bound args being swapped. Reproduced here by min/max-ing
// the bounds up front, which is algebraically identical for every ordering.
int GM_CDECL gmf_GETINRANGE(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    float v = a_thread->ParamFloatOrInt(0, 0.0f);
    float lo = a_thread->ParamFloatOrInt(1, 0.0f);
    float hi = a_thread->ParamFloatOrInt(2, 0.0f);
    if (lo > hi) std::swap(lo, hi);
    a_thread->PushFloat(std::min(hi, std::max(lo, v)));
    return GM_OK;
}
// Int-typed variant (e.g. `MATHUTIL_GETINRANGEI(tableCount(this.panels),0,n)`).
// Despite the name/int return, real usage isn't always integer-valued --
// Platformer/PauseProcessBase.gm:851 calls
// `MATHUTIL_GETINRANGEI(this.slideInPerc, 0.1, 1.0)` (a 0..1 volume-fade
// percentage) straight into `this.gameProcess.setGlobalVolume(gv)`, same
// a strict int-only param threw a blank exception on the very first real
// non-integer call, permanently breaking that frame's pause-menu update()
// (and, via the exception aborting mid-function, its own
// setGlobalVolume() call right after). Widened to accept float args like
// GETINRANGE does; truncates to int on return to match the other
// confirmed-integer real call site (array index/count clamping) --
// FIXED (this session): the code here was pushing a float despite this
// very comment already saying "truncates to int on return"; ground truth
// (int), like MATH_MINI/MATH_MAXI. Same bound-order-robustness fix as
// GETINRANGE above -- identical algebra, just an int return-slot tag.
int GM_CDECL gmf_GETINRANGEI(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(lo, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 2, 0.0f);
    if (lo > hi) std::swap(lo, hi);
    a_thread->PushInt((gmint)std::min(hi, std::max(lo, v)));
    return GM_OK;
}

int GM_CDECL gmf_DEGTORAD(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(deg, 0, 0.0f);
    a_thread->PushFloat(deg * (kPi / 180.0f));
    return GM_OK;
}

// Confirmed real: 3 args (value, min, max) -- (value-min)/(max-min), NOT
// clamped (real usage separately wraps it in MATHUTIL_GETINRANGE(...)
// when clamping is actually wanted, e.g.
// `MATHUTIL_GETINRANGE(1-MATHUTIL_GetPercentageInRange(...),0,1)`, which
// would be redundant if this already clamped).
int GM_CDECL gmf_GetPercentageInRange(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(lo, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 2, 0.0f);
    float range = hi - lo;
    a_thread->PushFloat(range != 0.0f ? (v - lo) / range : 0.0f);
    return GM_OK;
}

// Confirmed real 2-arg (percent, peakPercent) -> float shape
// both args read via the same float-or-int branch as every other MATHUTIL_*
// function here) -- but the actual curve math lives in an un-decompiled
// helper (FUN_000c6720), so the exact formula below is still inferred from
// name + real usage, not read off the decompiled body. Both real call sites
// feed the result straight into something expecting a plain 0..1 value
// (EntityPacks/StaticDangerAsteroid.gm:205's
// `Audio.setVoiceVolume(id, MATHUTIL_GetTrianglePercentage(volPerc,0.5)*0.39)`
// as a volume multiplier; EntityPacks/SwayingTree.gm:473's
// `MATH_POW(MATHUTIL_GetTrianglePercentage(perc,0.9),0.7)` feeding a power
// curve that also expects 0..1). A triangular (linear ramp up, linear ramp
// down) shape peaking at 1.0 when percent==peakPercent, 0 at percent==0
// and percent==1, matches both: StaticDangerAsteroid's symmetric 0.5 peak
// reads as "loudest at the midpoint of some cycle", SwayingTree's 0.9 peak
// reads as "fast ramp up, fast snap back" (a branch/leaf recoiling from a
// touch, matching the entity's name).
int GM_CDECL gmf_GetTrianglePercentage(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(percent, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(peak, 1, 0.5f);
    float result;
    if (percent <= peak) {
        result = (peak > 0.0f) ? (percent / peak) : 1.0f;
    } else {
        float falling = 1.0f - peak;
        result = (falling > 0.0f) ? ((1.0f - percent) / falling) : 0.0f;
    }
    a_thread->PushFloat(std::min(1.0f, std::max(0.0f, result)));
    return GM_OK;
}

// Confirmed real: 1 vector2D arg. atan2(y,x), range (-pi,pi].
int GM_CDECL gmf_GetAngleRad(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    float x, y;
    if (!ReadVec2(a_thread, 0, x, y)) { GM_EXCEPTION_MSG("MATHUTIL_GetAngleRad: expecting a vector2D"); return GM_EXCEPTION; }
    a_thread->PushFloat(std::atan2(y, x));
    return GM_OK;
}
// Same but wrapped to [0, 2pi).
int GM_CDECL gmf_GetAngleRad360(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    float x, y;
    if (!ReadVec2(a_thread, 0, x, y)) { GM_EXCEPTION_MSG("MATHUTIL_GetAngleRad360: expecting a vector2D"); return GM_EXCEPTION; }
    float a = std::atan2(y, x);
    if (a < 0.0f) a += 2.0f * kPi;
    a_thread->PushFloat(a);
    return GM_OK;
}

// Confirmed real: 2 args (vector2D, angleRad) -- rotates the vector,
// returns a new vector2D.
int GM_CDECL gmf_Rotate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    float x, y;
    if (!ReadVec2(a_thread, 0, x, y)) { GM_EXCEPTION_MSG("MATHUTIL_Rotate: expecting a vector2D"); return GM_EXCEPTION; }
    GM_FLOAT_OR_INT_PARAM(ang, 1, 0.0f);
    float c = std::cos(ang), s = std::sin(ang);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), x * c - y * s, x * s + y * c));
    return GM_OK;
}

// Confirmed real: 2 vector2D args -- unsigned angle between them, [0,pi],
// via acos of the normalized dot product.
int GM_CDECL gmf_GetRadiansBetween(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    float ax, ay, bx, by;
    if (!ReadVec2(a_thread, 0, ax, ay) || !ReadVec2(a_thread, 1, bx, by)) {
        GM_EXCEPTION_MSG("MATHUTIL_GetRadiansBetween: expecting two vector2D params");
        return GM_EXCEPTION;
    }
    float la = std::sqrt(ax * ax + ay * ay), lb = std::sqrt(bx * bx + by * by);
    if (la <= 0.0f || lb <= 0.0f) { a_thread->PushFloat(0.0f); return GM_OK; }
    float dot = (ax * bx + ay * by) / (la * lb);
    dot = std::min(1.0f, std::max(-1.0f, dot));
    a_thread->PushFloat(std::acos(dot));
    return GM_OK;
}

float WrapAngle(float a_angle) { // wraps to (-pi, pi]
    while (a_angle <= -kPi) a_angle += 2.0f * kPi;
    while (a_angle > kPi) a_angle -= 2.0f * kPi;
    return a_angle;
}

// Confirmed real: 2 angle (radian float) args -- signed shortest angular
// distance from the first to the second, (-pi,pi].
int GM_CDECL gmf_GetSignedAngleDistance(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
    a_thread->PushFloat(WrapAngle(b - a));
    return GM_OK;
}
// Unsigned version -- magnitude of the shortest angular distance, [0,pi].
int GM_CDECL gmf_GetAngleDistance(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
    a_thread->PushFloat(std::fabs(WrapAngle(b - a)));
    return GM_OK;
}
// Directional variants -- distance going only the positive/negative way
// around the circle, [0, 2pi).
int GM_CDECL gmf_GetAngleDistancePositive(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
    float d = std::fmod(b - a, 2.0f * kPi);
    if (d < 0.0f) d += 2.0f * kPi;
    a_thread->PushFloat(d);
    return GM_OK;
}
int GM_CDECL gmf_GetAngleDistanceNegative(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
    float d = std::fmod(a - b, 2.0f * kPi);
    if (d < 0.0f) d += 2.0f * kPi;
    a_thread->PushFloat(-d);
    return GM_OK;
}

int GM_CDECL gmf_distance(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    float ax, ay, bx, by;
    if (!ReadVec2(a_thread, 0, ax, ay) || !ReadVec2(a_thread, 1, bx, by)) {
        GM_EXCEPTION_MSG("MATHUTIL_distance: expecting two vector2D params");
        return GM_EXCEPTION;
    }
    a_thread->PushFloat(std::sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay)));
    return GM_OK;
}

// Confirmed real: 2 args (a point-list-like table, count) -- average of
// the first n vector2D entries.
// 5 args (circle1Center vec2, r1, circle2Center vec2, r2, outTable) --
// standard circle-circle intersection, writes outTable.p1/p2 (vector2D)
// and outTable.found (int 0/1). Real call site (MainMenu5/
// CampaignScreenRecordPlayer.gm's turntable tone-arm placement) only
// reads tmp.p1/tmp.p2 unconditionally right after calling this, so both
// fields always need a sane (non-null) value even in the degenerate
// (no-intersection / concentric) case -- the decompiled body's exact
// register-level math is too mangled by this toolchain to recover
// function), so this uses the textbook two-circle intersection formula,
// falling back to the midpoint of the two centers (found=0) when the
// circles don't actually intersect.
int GM_CDECL gmf_FindCircleCircleIntersection(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(5);
    float c1x, c1y, c2x, c2y;
    if (!ReadVec2(a_thread, 0, c1x, c1y) || !ReadVec2(a_thread, 2, c2x, c2y)) {
        GM_EXCEPTION_MSG("MATHUTIL_FindCircleCircleIntersection: expecting vector2D params");
        return GM_EXCEPTION;
    }
    GM_FLOAT_OR_INT_PARAM(r1, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(r2, 3, 0.0f);
    GM_CHECK_TABLE_PARAM(out, 4);

    float dx = c2x - c1x, dy = c2y - c1y;
    float d = std::sqrt(dx * dx + dy * dy);

    bool found = false;
    float p1x, p1y, p2x, p2y;
    if (d > 0.0f && d <= r1 + r2 && d >= std::fabs(r1 - r2)) {
        float a = (r1 * r1 - r2 * r2 + d * d) / (2.0f * d);
        float h2 = r1 * r1 - a * a;
        float h = h2 > 0.0f ? std::sqrt(h2) : 0.0f;
        float mx = c1x + a * dx / d, my = c1y + a * dy / d;
        p1x = mx + h * dy / d; p1y = my - h * dx / d;
        p2x = mx - h * dy / d; p2y = my + h * dx / d;
        found = true;
    } else {
        p1x = p2x = (c1x + c2x) * 0.5f;
        p1y = p2y = (c1y + c2y) * 0.5f;
    }

    out->Set(a_thread->GetMachine(), "p1", MakeVector2D(*a_thread->GetMachine(), p1x, p1y));
    out->Set(a_thread->GetMachine(), "p2", MakeVector2D(*a_thread->GetMachine(), p2x, p2y));
    out->Set(a_thread->GetMachine(), "found", gmVariable((gmint)(found ? 1 : 0)));
    return GM_OK;
}

int GM_CDECL gmf_GetCentroid(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_CHECK_TABLE_PARAM(list, 0);
    GM_INT_PARAM(n, 1, 0);
    float sx = 0.0f, sy = 0.0f;
    int counted = 0;
    for (int i = 0; i < n; ++i) {
        float x, y;
        if (GetVector2D(*a_thread->GetMachine(), list->Get(i), x, y)) { sx += x; sy += y; ++counted; }
    }
    if (counted > 0) { sx /= counted; sy /= counted; }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), sx, sy));
    return GM_OK;
}

// Shared geometry helper: projects a_query onto the infinite line through
// a_linePoint with direction a_lineDir (not required to be unit length),
// optionally clamped to the [0,1] segment range.
void ClosestPointOnLine(float lpx, float lpy, float dx, float dy, float qx, float qy, bool clampSeg, float& outX, float& outY) {
    float len2 = dx * dx + dy * dy;
    float t = (len2 > 0.0f) ? ((qx - lpx) * dx + (qy - lpy) * dy) / len2 : 0.0f;
    if (clampSeg) t = std::min(1.0f, std::max(0.0f, t));
    outX = lpx + dx * t;
    outY = lpy + dy * t;
}

// Confirmed real: 3 vector2D args (a point ON the line, the line's
// direction, the query point) -- closest point on the INFINITE line.
int GM_CDECL gmf_FindClosestPointOnLine(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    float lpx, lpy, dx, dy, qx, qy;
    if (!ReadVec2(a_thread, 0, lpx, lpy) || !ReadVec2(a_thread, 1, dx, dy) || !ReadVec2(a_thread, 2, qx, qy)) {
        GM_EXCEPTION_MSG("MATHUTIL_FindClosestPointOnLine: expecting three vector2D params");
        return GM_EXCEPTION;
    }
    float ox, oy;
    ClosestPointOnLine(lpx, lpy, dx, dy, qx, qy, false, ox, oy);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), ox, oy));
    return GM_OK;
}
// Same shape, but the direction arg is the segment's full extent (b-a)
// and the result is clamped onto the segment (t in [0,1]).
int GM_CDECL gmf_GetClosestPointOnLineSegment(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    float ax, ay, dx, dy, qx, qy;
    if (!ReadVec2(a_thread, 0, ax, ay) || !ReadVec2(a_thread, 1, dx, dy) || !ReadVec2(a_thread, 2, qx, qy)) {
        GM_EXCEPTION_MSG("MATHUTIL_GetClosestPointOnLineSegment: expecting three vector2D params");
        return GM_EXCEPTION;
    }
    float ox, oy;
    ClosestPointOnLine(ax, ay, dx, dy, qx, qy, true, ox, oy);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), ox, oy));
    return GM_OK;
}
int GM_CDECL gmf_GetClosestDistanceFromLineToPoint(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    float lpx, lpy, dx, dy, qx, qy;
    if (!ReadVec2(a_thread, 0, lpx, lpy) || !ReadVec2(a_thread, 1, dx, dy) || !ReadVec2(a_thread, 2, qx, qy)) {
        GM_EXCEPTION_MSG("MATHUTIL_GetClosestDistanceFromLineToPoint: expecting three vector2D params");
        return GM_EXCEPTION;
    }
    float ox, oy;
    ClosestPointOnLine(lpx, lpy, dx, dy, qx, qy, false, ox, oy);
    a_thread->PushFloat(std::sqrt((qx - ox) * (qx - ox) + (qy - oy) * (qy - oy)));
    return GM_OK;
}
int GM_CDECL gmf_GetClosestDistanceFromLineSegmentToPoint(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    float ax, ay, dx, dy, qx, qy;
    if (!ReadVec2(a_thread, 0, ax, ay) || !ReadVec2(a_thread, 1, dx, dy) || !ReadVec2(a_thread, 2, qx, qy)) {
        GM_EXCEPTION_MSG("MATHUTIL_GetClosestDistanceFromLineSegmentToPoint: expecting three vector2D params");
        return GM_EXCEPTION;
    }
    float ox, oy;
    ClosestPointOnLine(ax, ay, dx, dy, qx, qy, true, ox, oy);
    a_thread->PushFloat(std::sqrt((qx - ox) * (qx - ox) + (qy - oy) * (qy - oy)));
    return GM_OK;
}

int GM_CDECL gmf_GetSeconds(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    a_thread->PushFloat(ms / 1000.0f);
    return GM_OK;
}
int GM_CDECL gmf_GetMinutes(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    a_thread->PushFloat(ms / 60000.0f);
    return GM_OK;
}

// Confirmed real: 2 args (rate, deltaMs) -- fraction REMAINING after
// deltaMs at the given rate, exp(-rate*dt). Real usage always
// `x *= MATHUTIL_CalculateExponentialDecay(rate,msElapsed)`-shaped (decay
// toward 0).
int GM_CDECL gmf_CalculateExponentialDecay(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(rate, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(dt, 1, 0.0f);
    a_thread->PushFloat(std::exp(-rate * dt));
    return GM_OK;
}
// The complement -- fraction of the way FROM current TOWARD target after
// deltaMs, 1-exp(-rate*dt). Real usage always
// `x += (target-x) * MATHUTIL_CalculateExponentialDecayReversed(rate,msElapsed)`-shaped
// (frame-rate-independent ease toward a target, e.g. the title screen's
// own fade-to-tutorial: `fadePerc += (1-fadePerc) *
// MATHUTIL_CalculateExponentialDecayReversed(1.0/131.0, msElapsed);`).
int GM_CDECL gmf_CalculateExponentialDecayReversed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(rate, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(dt, 1, 0.0f);
    a_thread->PushFloat(1.0f - std::exp(-rate * dt));
    return GM_OK;
}
// FIXED (this session): was `exp(rate*dt) - 1.0`, which is UNBOUNDED and
// grows past 1.0 for any large frame delta. Its only real call site is
// PekkoAndJon/Main.gm's
// `this.transitionPerc = this.transitionPerc + (1 - this.transitionPerc) * local1;`
// -- the standard `x += (target - x) * fraction` easing idiom, which is
// only stable when `fraction` stays within [0,1]. A single startup frame
// hitch (dt ~3800ms under software rendering) made the old formula return
// 1.815, pushing transitionPerc past 1.0, which in turn made Main.gm's
// backdrop-position step (`pos = (target - pos) * transitionPerc`, a
// contraction only while |transitionPerc| < 1) diverge into a violent
// oscillator. Confirmed by live instrumentation: the backdrop centre was
// teleporting (35,-20) -> (9678,-5377) -> (-7226,4014) between consecutive
// frames, which is what made the kaleidoscope render as incoherent
// scrambled plaid instead of retail's stable concentric vortex.
//
// Correct formula is the complement of CalculateExponentialDecay
// (`exp(-rate*dt)`, 1 -> 0), i.e. `1 - exp(-rate*dt)` (0 -> 1) -- exactly
// what this function's own original comment described ("mirrors Decay's
// shape but growing from 0 rather than shrinking from 1"), just not what
// the code did. Bounded to [0,1) for all dt >= 0, so both easing steps
// above stay contractions no matter how bad a frame hitch gets.
//
// Independently CONFIRMED against the real retail process: a winedbg
// capture of retail's own first-frame backdrop translate implies
// transitionPerc = 0.004530 (see RENDER_PIPELINE_VERIFICATION.md). With
// Main.gm's real rate constant and a 60fps frame (16.67ms):
//   1 - exp(-0.00027240533381700516 * 16.67) = 0.0045295  -- matches.
//   exp( 0.00027240533381700516 * 16.67) - 1 = 0.0045501  -- does not.
// (This makes it numerically identical to CalculateExponentialDecayReversed;
// that's expected -- they're two names for the same "fraction of the way
// toward the target" quantity, and the corpus uses both spellings.)
int GM_CDECL gmf_CalculateExponentialGrowth(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(rate, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(dt, 1, 0.0f);
    a_thread->PushFloat(1.0f - std::exp(-rate * dt));
    return GM_OK;
}

} // namespace

void RegisterMathUtilBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("MATHUTIL_GETINRANGE", gmf_GETINRANGE);
    a_machine.RegisterLibraryFunction("MATHUTIL_GETINRANGEI", gmf_GETINRANGEI);
    a_machine.RegisterLibraryFunction("MATHUTIL_DEGTORAD", gmf_DEGTORAD);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetPercentageInRange", gmf_GetPercentageInRange);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetTrianglePercentage", gmf_GetTrianglePercentage);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetAngleRad", gmf_GetAngleRad);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetAngleRad360", gmf_GetAngleRad360);
    a_machine.RegisterLibraryFunction("MATHUTIL_Rotate", gmf_Rotate);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetRadiansBetween", gmf_GetRadiansBetween);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetSignedAngleDistance", gmf_GetSignedAngleDistance);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetAngleDistance", gmf_GetAngleDistance);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetAngleDistancePositive", gmf_GetAngleDistancePositive);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetAngleDistanceNegative", gmf_GetAngleDistanceNegative);
    a_machine.RegisterLibraryFunction("MATHUTIL_distance", gmf_distance);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetCentroid", gmf_GetCentroid);
    a_machine.RegisterLibraryFunction("MATHUTIL_FindCircleCircleIntersection", gmf_FindCircleCircleIntersection);
    a_machine.RegisterLibraryFunction("MATHUTIL_FindClosestPointOnLine", gmf_FindClosestPointOnLine);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetClosestPointOnLineSegment", gmf_GetClosestPointOnLineSegment);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetClosestDistanceFromLineToPoint", gmf_GetClosestDistanceFromLineToPoint);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetClosestDistanceFromLineSegmentToPoint", gmf_GetClosestDistanceFromLineSegmentToPoint);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetSeconds", gmf_GetSeconds);
    a_machine.RegisterLibraryFunction("MATHUTIL_GetMinutes", gmf_GetMinutes);
    a_machine.RegisterLibraryFunction("MATHUTIL_CalculateExponentialDecay", gmf_CalculateExponentialDecay);
    a_machine.RegisterLibraryFunction("MATHUTIL_CalculateExponentialDecayReversed", gmf_CalculateExponentialDecayReversed);
    a_machine.RegisterLibraryFunction("MATHUTIL_CalculateExponentialGrowth", gmf_CalculateExponentialGrowth);
}
