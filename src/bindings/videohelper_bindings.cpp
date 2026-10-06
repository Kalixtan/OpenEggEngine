#include "gmThread.h"
#include "videohelper_bindings.h"
#include "shape_bindings.h"
#include "vector2d_bindings.h"
#include "gmTableObject.h"
#include <vector>
#include <cmath>

// See videohelper_bindings.h for why these two live here in C++ rather than
// in game_data/VideoHelper.gm.

namespace {

// SHAPETYPE_* numbering, matching shape_bindings.cpp's own ShapeType enum
// (which is read off retail's beginShape switch, FUN_00421810).
constexpr int kShapeTypeQuads = 0;
constexpr int kShapeTypeTriangleFan = 2;
constexpr int kShapeTypeTriangleStrip = 3;
constexpr int kShapeTypeLineLoop = 6;

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
// argument-count dispatch and the same shape return). VideoHelper.gm's dead
// reference keeps BOTH forms of the geometry -- a live one that scales the
// shared VIDEOHELPER_circleShape, and, commented out directly above it, the
// explicit fan that shape is itself built from:
//
//     local q=20; local angInc=TWOPI/MATH_REAL(q);
//     beginShape(SHAPETYPE_TRIANGLEFAN);
//     plotVertex(x,y,z);
//     for (i=0;i<=q;i+=1) { plotVertex(x+cos(ang)*r, y+sin(ang)*r, z); ang+=angInc; }
//
// i.e. a centre vertex plus q+1 rim vertices (the last repeating the first to
// close the fan). Built directly here rather than via a shared pre-scaled
// shape, so each call's own radius is baked in and the result is a normal
// standalone shape like every other function in this file returns.
//
// 174 real call sites use the 4-arg form (e.g. Player.gm:5318's
// `this.particleFillShape=VIDEOHELPER_DrawFilledCircle(0,0,0,1)` -- a unit
// circle built once and reused under a scale).
int GM_CDECL gmf_VIDEOHELPER_DrawFilledCircle(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmUserObject* existing = (GM_NUM_PARAMS > 0)
        ? AsShapeObject(m, a_thread->Param(0)) : nullptr;
    const int base = existing ? 1 : 0;
    if (GM_NUM_PARAMS < base + 4) return GM_EXCEPTION;

    const float x = a_thread->ParamFloatOrInt(base + 0, 0.0f);
    const float y = a_thread->ParamFloatOrInt(base + 1, 0.0f);
    const float z = a_thread->ParamFloatOrInt(base + 2, 0.0f);
    const float r = a_thread->ParamFloatOrInt(base + 3, 0.0f);

    constexpr int kQ = 20; // the reference implementation's own step count
    const float angInc = 6.28318530718f / (float)kQ;
    std::vector<float> xy;
    xy.reserve(((size_t)kQ + 2) * 2);
    xy.push_back(x);
    xy.push_back(y);
    float ang = 0.0f;
    for (int i = 0; i <= kQ; ++i) {
        xy.push_back(x + std::cos(ang) * r);
        xy.push_back(y + std::sin(ang) * r);
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
// increment. The allocate-new path emits quality points and explicitly adds
// the first point again to close the outline. Its shape is a line loop; the
// duplicate closing point is retained to match retail's stored geometry.
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

    PushShapeFromVerts(a_thread, kShapeTypeLineLoop, xy.data(),
                       (int)(xy.size() / 2), (float)z, existing,
                       /*preserve existing primitive*/ existing != nullptr,
                       /*retail pushes only newly created shapes*/ existing == nullptr);
    return GM_OK;
}

// VIDEOHELPER_DrawFilledDonut(x, y, z, rx, ry, tx, ty [, q])        -> shape
// VIDEOHELPER_DrawFilledDonut(shape, x, y, z, rx, ry, tx, ty [, q]) -> shape
//
// Real native in retail (0x001b67c4; decompiled body confirms the
// argument-count dispatch). Geometry from VideoHelper.gm's dead reference:
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
    int q = (GM_NUM_PARAMS > base + 7) ? (int)a_thread->ParamInt(base + 7, 22) : 22;
    if (q < 2) q = 22; // reference default; q-1 is a divisor below

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
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawThickLine2D", gmf_VIDEOHELPER_DrawThickLine2D);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledPolygon", gmf_VIDEOHELPER_DrawFilledPolygon);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledRectCentered", gmf_VIDEOHELPER_DrawFilledRectCentered);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledCircle", gmf_VIDEOHELPER_DrawFilledCircle);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawCircle", gmf_VIDEOHELPER_DrawCircle);
    a_machine.RegisterLibraryFunction("VIDEOHELPER_DrawFilledDonut", gmf_VIDEOHELPER_DrawFilledDonut);
}
