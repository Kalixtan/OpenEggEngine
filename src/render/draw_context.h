#pragma once
#include "gmThread.h"
#include <vector>
#include <functional>

struct DrawContext {
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
    float fontWidth = 0.45f;
    // Native render-config IDs differ from Cg paramno: TextureDist ID 2 is
    // aDiscardLevel and ID 3 is aSmoothLevel. The setters/reset now map these
    // values directly. ID 4 is aSize; nested replay writes to IDs 5/6 are out
    // of range for this five-entry config. Full grouped-pass parity remains
    // open.
    bool distanceTextureEnabled = false;
    float distanceTextureDiscardLevel = 0.5f;
    float distanceTextureSmoothLevel = 0.5f;
    // Retail ReplayDrawListCommands copies DrawList+0x10 into the render
    // context before replay. The desktop carries that selector through the
    // same scope; shader-config selection consumes it in the renderer port.
    bool selectionFragmentProgram = false;

    unsigned int currentTextureHandle = 0;

    struct Attrs {
        bool depthTest, depthWrite, colourMask;
        int stencilFunc, stencilRef, stencilSfail, stencilDpfail, stencilDppass;
        int culling;
    };
    std::vector<Attrs> attrStack;

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
int GM_CDECL DC_gmf_setOrderAlphaTest(gmThread* a_thread);
int GM_CDECL DC_gmf_setOrderLast(gmThread* a_thread);
int GM_CDECL DC_gmf_setDistanceTextureRenderAttributes(gmThread* a_thread);
// The Video setter is bound separately to its audited retail no-op wrapper;
// drawList.setColour below records its own deferred color-state command.
int GM_CDECL gmf_DC_setColour_drawList(gmThread* a_thread);
int GM_CDECL gmf_DC_setColourRef(gmThread* a_thread);
int GM_CDECL DC_gmf_setFontRenderAttributes(gmThread* a_thread);
int GM_CDECL DC_gmf_resetFontRenderAttributes(gmThread* a_thread);
int GM_CDECL DC_gmf_print(gmThread* a_thread);
int GM_CDECL DC_gmf_printTranslated(gmThread* a_thread);
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
int GM_CDECL gmf_DC_multMatrix(gmThread* a_thread);
int GM_CDECL DC_gmf_clear(gmThread* a_thread);
int GM_CDECL DC_gmf_drawModTexture(gmThread* a_thread);
int GM_CDECL DC_gmf_drawTextureByHandle(gmThread* a_thread);
int GM_CDECL DC_gmf_setTextureByHandle(gmThread* a_thread);
int GM_CDECL DC_gmf_setTexture(gmThread* a_thread);
int GM_CDECL DC_gmf_clearTexture(gmThread* a_thread);
int GM_CDECL DC_gmf_setTextureSubImg(gmThread* a_thread);
int GM_CDECL DC_gmf_drawTexture(gmThread* a_thread);
