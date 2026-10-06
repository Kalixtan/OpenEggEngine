#pragma once
#include "gmThread.h"
#include <vector>
#include <functional>

// "Video.* and drawList.* really are (mostly) the same underlying command
// list"): depth-test/depth-write flags, font render attributes, and the
// 2D matrix stack (pushMatrix/popMatrix/translate/rotate/scale) are all
// called interchangeably through both the `Video` global and any `drawList`
// instance in real scripts (e.g. `drawList.pushMatrix()` +
// `Video.beginShape(...)` composing together, ConsoleProcess.gm). Rather
// than modelling a real per-drawList command buffer (out of scope -- this
// engine executes draws immediately, see draw_list.h), there's exactly one
// of these, shared by drawList's gmf_* handlers, Video's, and shape
// plotVertex's transform lookup.
//
// The matrix stack and beginShape/plotVertex/endShape have no decompiled
// instead (500+ call sites across the corpus), not a confirmed decompiled
// port. depthTest/fontEdge/fontWidth are confirmed real (already used by
// drawList before this file existed); depthWrite/setupOrthoView/
// setupAspectRatioOrthoView/printUntranslated are confirmed real via
struct DrawContext {
    // Depth test defaults ON, to match the GL state Renderer::Init actually
    // establishes (`glEnable(GL_DEPTH_TEST)`). This shadow copy defaulting
    // to false meant every script-recorded draw ran with no depth test, so
    // the *_Z values those draws carry were ignored -- e.g. SpawnPoint.gm
    // sets `this.drawZ = PLAYER_Z-ZINCER` (-17.83) and translates by it,
    // which should place the spawner just behind the player and BEHIND the
    // ground (-10), but it drew over the ground instead.
    //
    // The z itself was always plumbed correctly (shape_bindings.cpp passes
    // `ctx.Top().z + s->localZ` into DrawPolygon); nothing was consuming
    // it. Scripts still turn the test off explicitly via setAttrDepthTest
    // where they mean to, which is what that opcode is for.
    bool depthTest = true;
    bool depthWrite = true;
    // "is the colour mask enabled", i.e. true = colour writes BLOCKED (see
    // Renderer::SetColourMask). The default must be false: a frame starts
    // drawing visible colour, and only a stencil-only pass turns it off.
    bool colourMask = false;
    int stencilFunc = 8;   // VIDEOPARAMS_DISABLE
    int stencilRef = 0;
    int stencilSfail = 100, stencilDpfail = 100, stencilDppass = 100; // VIDEOPARAMS_KEEP
    int culling = 0; // setAttrCulling: 0 disabled, 1 back/CW, 2 front/CCW
    float fontEdge = 0.5f;
    float fontWidth = 0.5f;

    // setTextureByHandle/clearTexture -- confirmed real recorded draw-list
    // opcode into the SAME command list drawShape/setColour/etc. push into,
    // not an immediate GL state change), see DC_gmf_setTextureByHandle's
    // own header comment (draw_context.cpp) for the live crash this fixes.
    // A plain mutable field here (rather than a push/pop stack) matches
    // every real call site's own bracketing pattern: `setTextureByHandle(t);
    // drawShape(...); ...; clearTexture();` -- bind, draw N shapes, unbind,
    // never nested.
    unsigned int currentTextureHandle = 0;

    // pushAttributes/popAttributes -- confirmed real save/restore stack
    // (Ghidra: DrawList.pushAttributes/popAttributes, 0x0007e434/
    // 0x0007e4a4, push/pop real opcodes 0x1a/0x1b into the command
    // buffer), NOT the no-op this engine originally treated it as. Was a
    // genuine bug, not just an accuracy gap: real screens (e.g.
    // HomeScreenBase.gm's logo draw) do
    // `pushAttributes(); ...colourMask(false)...; popAttributes();`
    // expecting colourMask (and depth/stencil state) to snap back
    // afterward -- with popAttributes doing nothing, colourMask(false)
    // stayed off for the rest of the program, hiding everything drawn
    // after that point forever.
    struct Attrs {
        bool depthTest, depthWrite, colourMask;
        int stencilFunc, stencilRef, stencilSfail, stencilDpfail, stencilDppass;
        int culling;
    };
    std::vector<Attrs> attrStack;

    // 2D affine: x' = a*x + c*y + e ; y' = b*x + d*y + f
    // z is a separate plain accumulator, not part of the affine block --
    // no real usage ever rotates/scales Z (translate(x,y,z) is the only
    // Z-bearing call in the whole corpus), it just sums through nested
    // pushMatrix/translate the same way a stack of pure Z-nudges would
    // (e.g. ConsoleProcess.gm's repeated `drawList.translate(0,0,z);
    // z-=ZINCER;` per line to draw text slightly nearer each time).
    struct Mat {
        float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
        float z = 0;
    };
    std::vector<Mat> stack{Mat{}};

    Mat& Top() { return stack.back(); }
};

inline DrawContext::Mat DC_Multiply(const DrawContext::Mat& outer, const DrawContext::Mat& inner) {
    // outer applied after inner: result(p) = outer(inner(p))
    DrawContext::Mat r;
    r.a = outer.a * inner.a + outer.c * inner.b;
    r.b = outer.b * inner.a + outer.d * inner.b;
    r.c = outer.a * inner.c + outer.c * inner.d;
    r.d = outer.b * inner.c + outer.d * inner.d;
    r.e = outer.a * inner.e + outer.c * inner.f + outer.e;
    r.f = outer.b * inner.e + outer.d * inner.f + outer.f;
    r.z = outer.z + inner.z;
    return r;
}

inline void DC_TransformPoint(const DrawContext::Mat& a_m, float a_x, float a_y, float& a_outX, float& a_outY) {
    a_outX = a_m.a * a_x + a_m.c * a_y + a_m.e;
    a_outY = a_m.b * a_x + a_m.d * a_y + a_m.f;
}

// The single shared instance, see class comment.
DrawContext& GetDrawContext();

// Shared record-or-execute dispatch used throughout draw_context.cpp and
// shape_bindings.cpp -- see draw_list.h's RecordingTarget/DrawListData
// header comment for why: a real drawList instance must record what's
// called on it (a replay closure) for later callDrawList()/flush, never
// draw immediately; the bare `Video.*` global form still executes right
// away, unchanged. Always returns GM_OK.
int DC_RecordOrRun(gmThread* a_thread, std::function<void()> a_apply);

// Bindings shared between the `drawList` native type (draw_list.cpp) and
// the `Video` global (video_bindings.cpp) -- both register these under the
// same names. Each reads/writes GetDrawContext() and/or calls into
// GetEngineContext().renderer directly.
int GM_CDECL DC_gmf_pushAttributes(gmThread* a_thread);
int GM_CDECL DC_gmf_popAttributes(gmThread* a_thread);
int GM_CDECL DC_gmf_setAttrDepthTest(gmThread* a_thread);
int GM_CDECL DC_gmf_setAttrDepthWrite(gmThread* a_thread);
int GM_CDECL DC_gmf_setColourMask(gmThread* a_thread);
int GM_CDECL DC_gmf_setAttrStencil(gmThread* a_thread);
int GM_CDECL DC_gmf_setAttrDefaultStencil(gmThread* a_thread);
int GM_CDECL DC_gmf_setAttrCulling(gmThread* a_thread);
int GM_CDECL DC_gmf_setOrderTransparent(gmThread* a_thread);
int GM_CDECL DC_gmf_setOrderLast(gmThread* a_thread);
int GM_CDECL DC_gmf_setDistanceTextureRenderAttributes(gmThread* a_thread);
// setColour/pushMatrix/popMatrix/translate/rotate(Z)/scale: confirmed real
// names (found directly in EBOOT.elf's VIDEO/VMDRAWLIST method tables) but
// no decompiled body reachable without live Ghidra cross-reference access
// -- routed through the strict-call trap (see draw_context.cpp) rather than
// running on the previous corpus-inferred implementation.
int GM_CDECL gmf_DC_setColour_video(gmThread* a_thread);
int GM_CDECL gmf_DC_setColour_drawList(gmThread* a_thread);
int GM_CDECL gmf_DC_setColourRef(gmThread* a_thread);
int GM_CDECL DC_gmf_setFontRenderAttributes(gmThread* a_thread);
int GM_CDECL DC_gmf_resetFontRenderAttributes(gmThread* a_thread);
int GM_CDECL DC_gmf_print(gmThread* a_thread);
int GM_CDECL DC_gmf_printUntranslated(gmThread* a_thread);
int GM_CDECL DC_gmf_printArray(gmThread* a_thread);
int GM_CDECL DC_gmf_setupWorldView(gmThread* a_thread);
int GM_CDECL DC_gmf_setupOrthoView(gmThread* a_thread);
int GM_CDECL DC_gmf_setupAspectRatioOrthoView(gmThread* a_thread);
int GM_CDECL DC_gmf_setupWorldOrthoView(gmThread* a_thread);
int GM_CDECL gmf_DC_pushMatrix(gmThread* a_thread);
int GM_CDECL gmf_DC_popMatrix(gmThread* a_thread);
int GM_CDECL gmf_DC_translate(gmThread* a_thread);
int GM_CDECL gmf_DC_rotate(gmThread* a_thread);
int GM_CDECL gmf_DC_scale(gmThread* a_thread);
int GM_CDECL DC_gmf_clear(gmThread* a_thread);
int GM_CDECL DC_gmf_drawTextureByHandle(gmThread* a_thread);
int GM_CDECL DC_gmf_setTextureByHandle(gmThread* a_thread);
int GM_CDECL DC_gmf_clearTexture(gmThread* a_thread);
int GM_CDECL DC_gmf_setTextureSubImg(gmThread* a_thread);
int GM_CDECL DC_gmf_drawTexture(gmThread* a_thread);
