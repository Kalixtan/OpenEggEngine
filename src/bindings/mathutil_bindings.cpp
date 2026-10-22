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

int GM_CDECL gmf_GETINRANGE(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    float v = a_thread->ParamFloatOrInt(0, 0.0f);
    float lo = a_thread->ParamFloatOrInt(1, 0.0f);
    float hi = a_thread->ParamFloatOrInt(2, 0.0f);
    if (lo > hi) std::swap(lo, hi);
    a_thread->PushFloat(std::min(hi, std::max(lo, v)));
    return GM_OK;
}
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

int GM_CDECL gmf_GetPercentageInRange(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(lo, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 2, 0.0f);
    float range = hi - lo;
    a_thread->PushFloat(range != 0.0f ? (v - lo) / range : 0.0f);
    return GM_OK;
}

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

int GM_CDECL gmf_Rotate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    float x, y;
    if (!ReadVec2(a_thread, 0, x, y)) { GM_EXCEPTION_MSG("MATHUTIL_Rotate: expecting a vector2D"); return GM_EXCEPTION; }
    GM_FLOAT_OR_INT_PARAM(ang, 1, 0.0f);
    float c = std::cos(ang), s = std::sin(ang);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), x * c - y * s, x * s + y * c));
    return GM_OK;
}

int GM_CDECL gmf_GetRadiansBetween(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    float ax, ay, bx, by;
    if (!ReadVec2(a_thread, 0, ax, ay) || !ReadVec2(a_thread, 1, bx, by)) {
        GM_EXCEPTION_MSG("MATHUTIL_GetRadiansBetween: expecting two vector2D params");
        return GM_EXCEPTION;
    }
    // Retail compares the two atan2 headings, so a zero vector has heading 0.
    float angleA = std::atan2(ay, ax);
    float angleB = std::atan2(by, bx);
    float difference = std::fabs(angleA - angleB);
    if (difference > kPi) difference = 2.0f * kPi - difference;
    a_thread->PushFloat(difference);
    return GM_OK;
}

float WrapAngle(float a_angle) { // wraps to (-pi, pi]
    while (a_angle <= -kPi) a_angle += 2.0f * kPi;
    while (a_angle > kPi) a_angle -= 2.0f * kPi;
    return a_angle;
}

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
    // Retail returns the positive amount to travel in the negative direction.
    a_thread->PushFloat(d);
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

int GM_CDECL gmf_CalculateExponentialDecay(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(rate, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(dt, 1, 0.0f);
    // The retail helper clamps its exp result to [0, 1] with ordered compares.
    float value = std::exp(-rate * dt);
    float clamped = 0.0f;
    if (0.0f - value < 0.0f) clamped = value;
    if (1.0f - clamped < 0.0f) clamped = 1.0f;
    a_thread->PushFloat(clamped);
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
int GM_CDECL gmf_CalculateExponentialGrowth(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(rate, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(dt, 1, 0.0f);
    (void)rate;
    (void)dt;
    // Retail exposes this call without pushing a script result.
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
