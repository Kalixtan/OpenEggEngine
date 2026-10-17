#include "gmThread.h"
#include "videohelper_bindings.h"
#include "shape_bindings.h"
#include "vector2d_bindings.h"
#include "gmTableObject.h"
#include "../ported_native/ground_math.h"
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// See videohelper_bindings.h for why these two live here in C++ rather than
// in game_data/VideoHelper.gm.

namespace {

// Retail shape-mode values verified against VMSHAPE.endShape at 0x7ba38 and
// its primitive table at 0x3a1384. See shape_bindings.cpp for the full map.
constexpr int kShapeTypeQuads = 7;
constexpr int kShapeTypeTriangleFan = 6;
constexpr int kShapeTypeTriangleStrip = 5;
constexpr int kShapeTypeLineStrip = 2;

// Retail VIDEOHELPER_DrawRoundedRectCentered (EBOOT 0x001b72dc) passes a raw
// VMSHAPE mode of 0x102 and capacity q*2 to its native builder at 0x001acd00.
// That builder emits q curve samples, then repeats the first point to close
// the line strip. The filled companion
// (0x001b77ec -> 0x001acfa8) uses mode 5, capacity q+2, and emits paired
// signed-sine vertices while i < q/2. Both call the retail sin/cos and
// sign-preserving power helpers, so use the same PPC-derived math routines.
// Both builders scale signed powered cosine and sine independently by the
// half-width and half-height. The filled builder differs in its paired,
// mirrored vertex layout; it does not add straight-edge offsets.
void RetailRoundedRectOutlinePoint(float angle, float x, float y,
                                   float halfWidth, float halfHeight,
                                   float curve, float& outX, float& outY) {
    const float rawCos = GroundRenderMath::RetailGroundSinCos(angle, true);
    const float powCos = GroundRenderMath::RetailPlayerPow(std::fabs(rawCos), curve);
    const float rawSin = GroundRenderMath::RetailGroundSinCos(angle, false);
    const float powSin = GroundRenderMath::RetailPlayerPow(std::fabs(rawSin), curve);
    const float signedCos = rawCos < 0.0f ? -powCos : powCos;
    const float signedSin = rawSin < 0.0f ? -powSin : powSin;
    outX = x + signedCos * halfWidth;
    outY = y + signedSin * halfHeight;
}

int RetailQualityArg(gmThread* a_thread, int index) {
    const gmVariable& value = a_thread->Param(index);
    if (value.m_type == GM_FLOAT)
        return std::isfinite(value.m_value.m_float)
            ? static_cast<int>(value.m_value.m_float) : 0;
    return value.m_value.m_int;
}

int RoundedRectArgs(gmThread* a_thread, int& base, gmUserObject*& existing) {
    gmMachine& machine = *a_thread->GetMachine();
    existing = GM_NUM_PARAMS > 0
        ? AsShapeObject(machine, a_thread->Param(0)) : nullptr;
    // Retail's native wrapper always reserves argument 0 for the optional
    // shape cache, including when it is null; geometry starts at argument 1.
    base = 1;
    // x, y, z, width, height, curve; quality is optional and defaults to 61.
    return GM_NUM_PARAMS >= base + 6 ? GM_OK : GM_EXCEPTION;
}

int GM_CDECL gmf_VIDEOHELPER_DrawRoundedRectCentered(gmThread* a_thread) {
    int base = 0;
    gmUserObject* existing = nullptr;
    if (RoundedRectArgs(a_thread, base, existing) != GM_OK) return GM_EXCEPTION;

    const float x = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float z = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float hw = a_thread->ParamFloatOrInt(base + 3, 0.0f) * 0.5f;
    const float hh = a_thread->ParamFloatOrInt(base + 4, 0.0f) * 0.5f;
    const float curve = a_thread->ParamFloatOrInt(base + 5, 1.0f);
    int q = GM_NUM_PARAMS > base + 6
        ? RetailQualityArg(a_thread, base + 6) : 61;

    constexpr float kTau = 6.2831854820251465f;
    // Retail calculates the step with fdivs, then advances the binary32
    // angle with fadds on every iteration.
    const float angleStep = kTau / static_cast<float>(q);
    std::vector<float> xy;
    xy.reserve(static_cast<size_t>(std::max(q, 0)) * 2 + 2);
    float angle = 0.0f;
    for (int i = 0; i < q; ++i) {
        float px = 0.0f, py = 0.0f;
        RetailRoundedRectOutlinePoint(angle, x, y, hw, hh, curve, px, py);
        xy.push_back(px); xy.push_back(py);
        angle += angleStep;
    }
    if (std::getenv("GE3_TRACE_ROUNDED_RECT")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 32) {
            std::fprintf(stderr,
                "[roundrect] outline q=%d count=%zu curve=%.6g half=(%.6g,%.6g) first=(%.6g,%.6g)\n",
                q, xy.size() / 2, curve, hw, hh, xy.empty() ? 0.0f : xy[0],
                xy.size() < 2 ? 0.0f : xy[1]);
        }
    }
    // The retail builder explicitly appends its initial rightmost point.
    xy.push_back(x + hw); xy.push_back(y);
    PushShapeFromVerts(a_thread, 0x102, xy.data(),
        static_cast<int>(xy.size() / 2), z, existing,
        /*preserve existing primitive*/ existing != nullptr,
        /*push the shape return value*/ true);
    return GM_OK;
}

int GM_CDECL gmf_VIDEOHELPER_DrawFilledRoundedRectCentered(gmThread* a_thread) {
    int base = 0;
    gmUserObject* existing = nullptr;
    if (RoundedRectArgs(a_thread, base, existing) != GM_OK) return GM_EXCEPTION;

    const float x = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float z = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float hw = std::fabs(a_thread->ParamFloatOrInt(base + 3, 0.0f)) * 0.5f;
    const float hh = std::fabs(a_thread->ParamFloatOrInt(base + 4, 0.0f)) * 0.5f;
    // Keep the existing script parameter contract, interpreting the curve
    // amount as the fraction of the limiting half-extent used for each corner.
    // This prevents wide controls from becoming full capsules while preserving
    // each caller's intended degree of rounding.
    const float curve = a_thread->ParamFloatOrInt(base + 5, 1.0f);
    int q = GM_NUM_PARAMS > base + 6
        ? RetailQualityArg(a_thread, base + 6) : 61;
    if (!std::isfinite(hw) || !std::isfinite(hh)) return GM_EXCEPTION;
    if (q < 4 || q > 4096) q = 61;

    const float curveAmount = std::isfinite(curve)
        ? std::clamp(curve, 0.0f, 1.0f) : 0.4f;
    const float radius = std::min(hw, hh) * curveAmount;
    constexpr float kHalfPi = 1.57079632679489661923f;
    constexpr float kPi = 3.14159265358979323846f;
    const int segmentsPerCorner = std::max(1, q / 4);
    const float cornerX[] = {x + hw - radius, x - hw + radius,
                             x - hw + radius, x + hw - radius};
    const float cornerY[] = {y + hh - radius, y + hh - radius,
                             y - hh + radius, y - hh + radius};
    const float startAngle[] = {0.0f, kHalfPi, kPi, 3.0f * kHalfPi};
    std::vector<float> xy;
    xy.reserve(static_cast<size_t>(4 * segmentsPerCorner + 3) * 2);
    xy.push_back(x);
    xy.push_back(y);
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i <= segmentsPerCorner; ++i) {
            if (corner != 0 && i == 0) continue;
            const float angle = startAngle[corner] +
                kHalfPi * static_cast<float>(i) / segmentsPerCorner;
            xy.push_back(cornerX[corner] + radius * std::cos(angle));
            xy.push_back(cornerY[corner] + radius * std::sin(angle));
        }
    }
    xy.push_back(xy[2]);
    xy.push_back(xy[3]);
    if (std::getenv("GE3_TRACE_ROUNDED_RECT")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 32) {
            std::fprintf(stderr,
                "[roundrect] fill q=%d count=%zu curve=%.6g radius=%.6g half=(%.6g,%.6g) first=(%.6g,%.6g)\n",
                q, xy.size() / 2, curve, radius, hw, hh, xy.empty() ? 0.0f : xy[0],
                xy.size() < 2 ? 0.0f : xy[1]);
        }
    }
    PushShapeFromVerts(a_thread, kShapeTypeTriangleFan, xy.data(),
        static_cast<int>(xy.size() / 2), z, existing,
        /*set the rounded-rectangle primitive*/ false,
        /*push the shape return value*/ true);
    return GM_OK;
}

// VIDEOHELPER_DrawFilledRect(x1, y1, x2, y2 [, z])          -> shape
// VIDEOHELPER_DrawFilledRect(shape, x1, y1, x2, y2 [, z])   -> that shape
//
// Confirmed from the decompiled body (0x001b25d8): it branches on whether
// the FIRST argument is the VMSHAPE user type (`*(int *)(iVar3 + iVar2) ==
// DAT_0043c438`) -- the shape form repopulates the shape passed in, the
// numeric form allocates a new one. Both read four float-or-int coordinates,
// and z is optional: the body takes a 5th coordinate only when the argument
// count exceeds the base form's (`if (*(short *)(param_1 + 0x54) < 6)` /
// `< 5` for the two overloads respectively), otherwise substituting
// DAT_001b25d4, which is 0.0 read straight out of EBOOT.elf.
//
// The vertex layout is a 4-vertex GL_QUADS winding
// (x1,y1) (x2,y1) (x2,y2) (x1,y2), independently corroborated by
// VideoHelper.gm's dead reference implementation of the same function.
int GM_CDECL gmf_VIDEOHELPER_DrawFilledRect(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* existing = (GM_NUM_PARAMS > 0)
        ? AsShapeObject(m, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;           // index of x1
    if (GM_NUM_PARAMS < base + 4) return GM_EXCEPTION;

    const float x1 = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y1 = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float x2 = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float y2 = a_thread->ParamFloatOrInt(base + 3, 0.0f);
    const float z = a_thread->ParamFloatOrInt(base + 4, 0.0f);

    const float xy[8] = { x1, y1,  x2, y1,  x2, y2,  x1, y2 };
    PushShapeFromVerts(a_thread, kShapeTypeQuads, xy, 4, z, existing);
    return GM_OK;
}

// Retail 0x001b4d64 dispatches on a leading VMSHAPE: six numeric
// arguments allocate and return a shape; seven update the supplied shape
// without pushing a return value. VideoHelper.gm's reference implementation
// gives the quad winding and half-thickness normal used by the native helper.
int GM_CDECL gmf_VIDEOHELPER_DrawThickLine2D(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    gmUserObject* existing = GM_NUM_PARAMS > 0
        ? AsShapeObject(machine, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;
    if (GM_NUM_PARAMS != base + 6) return GM_EXCEPTION;

    const float x1 = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y1 = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float x2 = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float y2 = a_thread->ParamFloatOrInt(base + 3, 0.0f);
    const float z = a_thread->ParamFloatOrInt(base + 4, 0.0f);
    const float thickness = a_thread->ParamFloatOrInt(base + 5, 0.0f);
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float length = std::sqrt(dx * dx + dy * dy);
    const float nx = length > 0.0f ? -dy / length * thickness * 0.5f : 0.0f;
    const float ny = length > 0.0f ? dx / length * thickness * 0.5f : 0.0f;
    const float xy[8] = {
        x1 - nx, y1 - ny, x1 + nx, y1 + ny,
        x2 + nx, y2 + ny, x2 - nx, y2 - ny,
    };
    PushShapeFromVerts(a_thread, kShapeTypeQuads, xy, 4, z, existing,
                       /*preserve existing primitive*/ existing != nullptr,
                       /*retail pushes only newly created shapes*/ existing == nullptr);
    return GM_OK;
}

// VIDEOHELPER_DrawFilledPolygon(plist, n [, z])         -> shape
// VIDEOHELPER_DrawFilledPolygon(shape, plist, n [, z])  -> that shape
//
// Same leading-SHAPE overload dispatch as DrawFilledRect above, confirmed
// the same way in the decompiled body (0x001b5c78, which tests the first
// argument against the same DAT_0043c438 user type).
//
// Builds an n+1 vertex GL_TRIANGLE_FAN: the polygon's centroid first, then
// the n perimeter points walked so the fan closes back on its start.
// VideoHelper.gm's dead reference implementation gives the exact order --
//
//     local cpos = MATHUTIL_GetCentroid(plist, n);
//     beginShape(SHAPETYPE_TRIANGLEFAN, n+1);
//     plotVertex(cpos, z);
//     local j = n-1;
//     for (i=0; i<=n; i+=1) { plotVertex(plist[j], z); j = i; }
//
// -- note `i<=n` with j lagging one behind, which emits plist[n-1] first and
// then plist[0..n-1], i.e. the last point doubles as the closing vertex. The
// centroid is the mean of the n points (MATHUTIL_GetCentroid, whose own
// binding this mirrors rather than calls, to avoid a script round-trip).
int GM_CDECL gmf_VIDEOHELPER_DrawFilledPolygon(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* existing = (GM_NUM_PARAMS > 0)
        ? AsShapeObject(m, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;           // index of plist
    if (GM_NUM_PARAMS < base + 2) return GM_EXCEPTION;

    gmTableObject* plist = a_thread->ParamTable(base + 0);
    const int n = (int)a_thread->ParamInt(base + 1, 0);
    const float z = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    if (!plist || n <= 0) return GM_EXCEPTION;

    // Perimeter points, in the list's own order.
    std::vector<float> pts;
    pts.reserve((size_t)n * 2);
    float cx = 0.0f, cy = 0.0f;
    int counted = 0;
    for (int i = 0; i < n; ++i) {
        float x = 0.0f, y = 0.0f;
        if (!GetVector2D(m, plist->Get(i), x, y)) continue;
        pts.push_back(x);
        pts.push_back(y);
        cx += x; cy += y;
        ++counted;
    }
    if (counted <= 0) return GM_EXCEPTION;
    cx /= (float)counted;
    cy /= (float)counted;

    // centroid, then plist[n-1], plist[0], plist[1], ... plist[n-1]
    std::vector<float> xy;
    xy.reserve(((size_t)counted + 2) * 2);
    xy.push_back(cx);
    xy.push_back(cy);
    int j = counted - 1;
    for (int i = 0; i <= counted; ++i) {
        xy.push_back(pts[(size_t)j * 2 + 0]);
        xy.push_back(pts[(size_t)j * 2 + 1]);
        j = (i < counted) ? i : j;
    }

    PushShapeFromVerts(a_thread, kShapeTypeTriangleFan, xy.data(),
                       (int)(xy.size() / 2), z, existing);
    return GM_OK;
}

// VIDEOHELPER_DrawFilledRectCentered(x, y, z, w, h)          -> shape
// VIDEOHELPER_DrawFilledRectCentered(shape, x, y, z, w, h)   -> that shape
//
// leading-SHAPE overload and shape-return shape as DrawFilledRect above.
// Note the argument ORDER differs from DrawFilledRect: z comes third, before
// the extents. VideoHelper.gm's dead reference gives the geometry exactly:
//
//     local hw=w/2; local hh=h/2;
//     VIDEOHELPER_DrawFilledRect(x-hw, y-hh, x+hw, y+hh, z);
//
// Real call sites are 4-arg + z (Main.gm:4771's
// `this.loadingScreenFillShape=VIDEOHELPER_DrawFilledRectCentered(0,0,0,1,1)`
// -- a unit quad built once and reused under a scale, 132 call sites of that
// shape) and 1-arg rebuilds of an existing shape.
int GM_CDECL gmf_VIDEOHELPER_DrawFilledRectCentered(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* existing = (GM_NUM_PARAMS > 0)
        ? AsShapeObject(m, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;
    if (GM_NUM_PARAMS < base + 5) return GM_EXCEPTION;

    const float x = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float z = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float w = a_thread->ParamFloatOrInt(base + 3, 0.0f);
    const float h = a_thread->ParamFloatOrInt(base + 4, 0.0f);
    const float hw = w * 0.5f, hh = h * 0.5f;

    const float xy[8] = { x - hw, y - hh,  x + hw, y - hh,
                          x + hw, y + hh,  x - hw, y + hh };
    PushShapeFromVerts(a_thread, kShapeTypeQuads, xy, 4, z, existing);
    return GM_OK;
}

// VIDEOHELPER_DrawFilledCircle(x, y, z, r)          -> shape
// VIDEOHELPER_DrawFilledCircle(shape, x, y, z, r)   -> that shape
//
// Real native in retail (0x001b3624; its decompiled body confirms the
// argument-count dispatch and the same shape return). The four-argument
// wrapper passes 0x3d (61) rim vertices to FUN_001aa590; that builder uses
// 2*pi/(61-1) and emits a center plus 61 rim vertices, with the last rim
// vertex repeating the first. The q=20 geometry below in VideoHelper.gm is
// inside its inactive branch; it is not the retail native default:
//
//     local q=20; local angInc=TWOPI/MATH_REAL(q);
//     beginShape(SHAPETYPE_TRIANGLEFAN);
//     plotVertex(x,y,z);
//     for (i=0;i<=q;i+=1) { plotVertex(x+cos(ang)*r, y+sin(ang)*r, z); ang+=angInc; }
//
// That dead script version is a center plus 21 rim vertices. The native
// implementation below instead matches the Ghidra-confirmed 62-vertex fan.
//
// 174 real call sites use the 4-arg form (e.g. Player.gm:5318's
// `this.particleFillShape=VIDEOHELPER_DrawFilledCircle(0,0,0,1)` -- a unit
// circle built once and reused under a scale).
int GM_CDECL gmf_VIDEOHELPER_DrawFilledCircle(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* existing = (GM_NUM_PARAMS > 0)
        ? AsShapeObject(m, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;
    const int count = a_thread->GetNumParams();
    if ((!existing && count != 4 && count != 5) ||
        (existing && count != 5 && count != 6)) return GM_EXCEPTION;

    const float x = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float z = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float r = a_thread->ParamFloatOrInt(base + 3, 0.0f);

    const int rimVertexCount = count == base + 5
        ? a_thread->ParamInt(base + 4, 61) : 61;
    if (rimVertexCount < 2) return GM_EXCEPTION;
    const float angInc = 6.28318548203f / (float)(rimVertexCount - 1);
    std::vector<float> xy;
    xy.reserve(((size_t)rimVertexCount + 1) * 2);
    xy.push_back(x);
    xy.push_back(y);
    float ang = 0.0f;
    for (int i = 0; i < rimVertexCount; ++i) {
        // The retail native builder FUN_001aa590 evaluates the retail
        // MATH_COS/MATH_SIN wrappers and uses fmadds for center + unit*r.
        // Player.gm creates particleFillShape through this four-argument
        // helper, so host libm and a separate multiply/add changed the
        // cached particle mesh even though the later native particle pass
        // reuses that shape unchanged.
        const float unitX = GroundRenderMath::RetailGroundSinCos(ang, true);
        const float unitY = GroundRenderMath::RetailGroundSinCos(ang, false);
        xy.push_back(std::fma(unitX, r, x));
        xy.push_back(std::fma(unitY, r, y));
        ang += angInc;
    }
    PushShapeFromVerts(a_thread, kShapeTypeTriangleFan, xy.data(),
                       (int)(xy.size() / 2), z, existing);
    return GM_OK;
}

// VIDEOHELPER_DrawCircle(x, y, z, r [, quality]) or
// VIDEOHELPER_DrawCircle(shape, x, y, z, r [, quality]). The native body is
// at EBOOT.elf 0x001b3ad4. Disassembly confirms exact arities (4/5 and 5/6),
// default quality 0x3d (61), start angle 0, and a float 2*pi/quality angle
// increment. The allocate-new path constructs VMSHAPE with raw mode 0x102;
// endShape masks that field to byte 2, selecting RSX line strip. It emits
// quality points and explicitly adds the first point again to close the
// outline. Keep the duplicate point and line-strip mode to match retail.
// The leading-shape path instead updates that shape in place with exactly
// quality points and preserves its original primitive type. Retail returns
// a shape only for the allocate-new overload; the reuse overload does not
// push a return value.
int GM_CDECL gmf_VIDEOHELPER_DrawCircle(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    gmUserObject* existing = (GM_NUM_PARAMS > 0)
        ? AsShapeObject(machine, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;
    const int count = a_thread->GetNumParams();
    if ((!existing && count != 4 && count != 5) ||
        (existing && count != 5 && count != 6)) return GM_EXCEPTION;

    auto readRetailReal = [&](int index) -> double {
        const gmVariable value = a_thread->Param(index);
        return value.m_type == GM_FLOAT
            ? (double)value.m_value.m_float
            : (double)value.m_value.m_int;
    };
    const double x = readRetailReal(base + 0);
    const double y = readRetailReal(base + 1);
    const double z = readRetailReal(base + 2);
    const double radius = readRetailReal(base + 3);
    const int qualityIndex = base + 4;
    const int quality = count == qualityIndex + 1
        ? a_thread->Param(qualityIndex).m_value.m_int
        : 61;

    std::vector<float> xy;
    const int pointCount = existing ? quality : quality + 1;
    if (pointCount > 0) xy.reserve((size_t)pointCount * 2);
    const float angleStep = 6.28318548203f / (float)quality;
    float angle = 0.0f;
    for (int i = 0; i < quality; ++i) {
        xy.push_back((float)(x + std::cos((double)angle) * radius));
        xy.push_back((float)(y + std::sin((double)angle) * radius));
        angle = (float)((double)angle + (double)angleStep);
    }
    if (!existing) {
        xy.push_back((float)(x + radius));
        xy.push_back((float)y);
    }

    PushShapeFromVerts(a_thread, kShapeTypeLineStrip, xy.data(),
                       (int)(xy.size() / 2), (float)z, existing,
                       /*preserve existing primitive*/ existing != nullptr,
                       /*retail pushes only newly created shapes*/ existing == nullptr);
    return GM_OK;
}

// VIDEOHELPER_DrawFilledDonut(x, y, z, rx, ry, tx, ty [, q])        -> shape
// VIDEOHELPER_DrawFilledDonut(shape, x, y, z, rx, ry, tx, ty [, q]) -> shape
//
// Real native in retail (0x001b67c4; decompiled body confirms the
// argument-count dispatch). Geometry from VideoHelper.gm's dead reference;
// the native seven-argument default is confirmed separately as q=31 by the
// wrapper's immediate at 0x001b6a68 (not the script reference's q=22):
//
//     if (!?q) { q=22; }
//     local angInc=TWOPI/MATH_REAL(q-1);
//     local htx=tx/2; local hty=ty/2;
//     beginShape(SHAPETYPE_TRIANGLESTRIP,q);
//     for (i=0;i<q;i+=1) {
//         dx=cos(ang); dy=sin(ang);
//         plotVertex(dx*(rx+htx), dy*(ry+hty));   // outer rim
//         plotVertex(dx*(rx-htx), dy*(ry-hty));   // inner rim
//         ang+=angInc;
//     }
//
// -- a triangle strip alternating outer/inner rim vertices, with angInc
// spanning the full circle over q-1 steps so the last step closes the ring.
// The reference emits the ring around the ORIGIN and positions it with an
// enclosing translate(x,y,z); this bakes x,y into the vertices instead and
// carries z as the shape's own depth, which composes identically under the
// caller's transform and matches what the other builders here return.
//
// The leading-SHAPE overload is exercised by a real call site:
// CampaignScreenRecordPlayer.gm:1072's
// `VIDEOHELPER_DrawFilledDonut(t.grooveShape,0,0,0,t.r,t.r,th,th,q)` -- nine
// arguments, rebuilding an existing shape in place each frame.
int GM_CDECL gmf_VIDEOHELPER_DrawFilledDonut(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* existing = (GM_NUM_PARAMS > 0)
        ? AsShapeObject(m, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;
    if (GM_NUM_PARAMS < base + 7) return GM_EXCEPTION;

    const float x  = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y  = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float z  = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float rx = a_thread->ParamFloatOrInt(base + 3, 0.0f);
    const float ry = a_thread->ParamFloatOrInt(base + 4, 0.0f);
    const float tx = a_thread->ParamFloatOrInt(base + 5, 0.0f);
    const float ty = a_thread->ParamFloatOrInt(base + 6, 0.0f);
    // The retail seven-argument overload supplies 0x1f (31) to
    // FUN_001aae20; its q-1 angular divisor and q outer/inner pairs produce
    // 62 triangle-strip vertices. The optional explicit-quality overload
    // forwards its supplied value.
    int q = (GM_NUM_PARAMS > base + 7) ? (int)a_thread->ParamInt(base + 7, 31) : 31;
    if (q < 2) q = 31; // avoid a zero divisor; retail default is 31

    const float htx = tx * 0.5f, hty = ty * 0.5f;
    const float angInc = 6.28318530718f / (float)(q - 1);
    std::vector<float> xy;
    xy.reserve((size_t)q * 4);
    float ang = 0.0f;
    for (int i = 0; i < q; ++i) {
        const float dx = std::cos(ang), dy = std::sin(ang);
        xy.push_back(x + dx * (rx + htx));
        xy.push_back(y + dy * (ry + hty));
        xy.push_back(x + dx * (rx - htx));
        xy.push_back(y + dy * (ry - hty));
        ang += angInc;
    }
    PushShapeFromVerts(a_thread, kShapeTypeTriangleStrip, xy.data(),
                       (int)(xy.size() / 2), z, existing);
    return GM_OK;
}

} // namespace

void RegisterVideoHelperBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledRect", gmf_VIDEOHELPER_DrawFilledRect);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawRoundedRectCentered", gmf_VIDEOHELPER_DrawRoundedRectCentered);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledRoundedRectCentered", gmf_VIDEOHELPER_DrawFilledRoundedRectCentered);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawThickLine2D", gmf_VIDEOHELPER_DrawThickLine2D);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledPolygon", gmf_VIDEOHELPER_DrawFilledPolygon);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledRectCentered", gmf_VIDEOHELPER_DrawFilledRectCentered);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledCircle", gmf_VIDEOHELPER_DrawFilledCircle);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawCircle", gmf_VIDEOHELPER_DrawCircle);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledDonut", gmf_VIDEOHELPER_DrawFilledDonut);
}
