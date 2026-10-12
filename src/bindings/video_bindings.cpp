#include "gmThread.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "video_bindings.h"
#include "process_base_bindings.h"
#include "process_handle_bindings.h"
#include "texture_bindings.h"
#include "video_draw_buffer_bindings.h"
#include "shape_bindings.h"
#include "animation_bindings.h"
#include "colour_bindings.h"
#include "physics_body_bindings.h"
#include "vector2d_bindings.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "../render/draw_list.h"
#include "../render/draw_context.h"
#include "../core/save_data.h"
#include <cstdio>
#include <array>
#include <filesystem>
#include <cmath>
#include <cstdint>

namespace {

// BYTECODE VERIFIED: Video construction zeroes a 256-byte buffer, creates
// `badTexture_Yo` as an 8x8 resource with 32-byte rows (RGBA8), and stores it
// at Video+0x838c. The desktop owns its equivalent handle until Renderer
// teardown, matching the Video object's lifetime on this process singleton.
unsigned int EnsureBadTextureFallback() {
    VideoState& video = GetEngineContext().video;
    if (video.badTextureFallbackHandle != 0) return video.badTextureFallbackHandle;
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer) return 0;
    static const std::array<unsigned char, 8 * 8 * 4> transparentRgba{};
    video.badTextureFallbackHandle = renderer->CreateTextureFromRGBA(
        transparentRgba.data(), 8, 8);
    return video.badTextureFallbackHandle;
}


// RETAIL BYTECODE: reads graphics-state +0x819c (EBOOT 0x74c28). This is the
// active display width, separate from the aspect-adjusted +0x81a4 viewport.
int GM_CDECL gmf_getDeviceWidth(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.activeDeviceWidth);
    return GM_OK;
}
// RETAIL BYTECODE: reads graphics-state +0x81a0 (EBOOT 0x74cb0), the active
// display height rather than the +0x81a8 adjusted viewport height.
int GM_CDECL gmf_getDeviceHeight(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.activeDeviceHeight);
    return GM_OK;
}
int GM_CDECL gmf_getScreenViewScale(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.screenViewScale);
    return GM_OK;
}
int GM_CDECL gmf_setScreenViewScale(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    GetEngineContext().video.screenViewScale = v;
    return GM_OK;
}
int GM_CDECL gmf_isNeedSetScreenViewScale(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.needSetScreenViewScale ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getScreenBithDepth(gmThread* a_thread) {
    // RETAIL BYTECODE: reads +0x81b0; constructor initializes it to zero.
    a_thread->PushInt(GetEngineContext().video.screenBitDepth);
    return GM_OK;
}
int GM_CDECL gmf_isFullScreen(gmThread* a_thread) {
    // RETAIL BYTECODE: reads byte +0x81b8, set by restartVideo.
    a_thread->PushInt(GetEngineContext().video.isFullscreen ? 1 : 0);
    return GM_OK;
}
// RETAIL BYTECODE: these wrappers compare the raw 32-bit argument payload at
// gmVariable+4 against zero, without checking the GM type or argument count.
// This intentionally treats nonzero float bit patterns as true, like PPC.
bool RetailRawBoolArgument(gmThread* a_thread) {
    return a_thread->GetNumParams() > 0 && a_thread->Param(0).m_value.m_int != 0;
}
float RetailRawFloatOrIntArgument(gmThread* a_thread, int a_index,
                                  float a_default = 0.0f) {
    if (a_thread->GetNumParams() <= a_index) return a_default;
    const gmVariable& value = a_thread->Param(a_index);
    return value.m_type == GM_FLOAT
        ? value.m_value.m_float
        : static_cast<float>(value.m_value.m_int);
}
int GM_CDECL gmf_setIsDrawingPhysics(gmThread* a_thread) {
    GetEngineContext().video.isDrawingPhysics = RetailRawBoolArgument(a_thread);
    return GM_OK;
}
int GM_CDECL gmf_isDrawingPhysics(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.isDrawingPhysics ? 1 : 0);
    return GM_OK;
}
// RETAIL BYTECODE: direct float load from graphics-state +0x81bc.
int GM_CDECL gmf_getAspectRatio(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.aspectRatio);
    return GM_OK;
}
// RETAIL BYTECODE: converts int arguments to float, then stores +0x81bc;
// viewport recomputation occurs in the frame setup path, not in this wrapper.
int GM_CDECL gmf_setAspectRatio(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 16.0f / 9.0f);
    GetEngineContext().video.aspectRatio = v;
    return GM_OK;
}
// RETAIL BYTECODE: Video.getCameraPosX calls FUN_00052934, which reads raw
// X at +0x81d4 and applies the enabled +0x81f8 bounds using +0x81e0/+0x81ec.
int GM_CDECL gmf_getCameraPosX(gmThread* a_thread) {
    a_thread->PushFloat(static_cast<float>(GetEngineContext().video.GetRetailCameraX()));
    return GM_OK;
}
// RETAIL BYTECODE: Video.getCameraPosY calls FUN_000529a0, which reads raw
// Y at +0x81d8 and applies the enabled +0x81f8 bounds using +0x81e4/+0x81f0.
int GM_CDECL gmf_getCameraPosY(gmThread* a_thread) {
    a_thread->PushFloat(static_cast<float>(GetEngineContext().video.GetRetailCameraY()));
    return GM_OK;
}
// RETAIL BYTECODE: Video.getCameraPosZ calls FUN_00052a0c, which reads raw
// Z at +0x81dc and applies the enabled +0x81f8 bounds using +0x81e8/+0x81f4.
int GM_CDECL gmf_getCameraPosZ(gmThread* a_thread) {
    a_thread->PushFloat(static_cast<float>(GetEngineContext().video.GetRetailCameraZ()));
    return GM_OK;
}
// BYTECODE VERIFIED: the four retail getters call helpers that directly load
// graphics-state floats +0x81fc, +0x8200, +0x8204, and +0x8208.
int GM_CDECL gmf_getCameraRotationAngle(gmThread* a_thread) { a_thread->PushFloat(GetEngineContext().video.camRotAngle); return GM_OK; }
int GM_CDECL gmf_getCameraRotationX(gmThread* a_thread) { a_thread->PushFloat(GetEngineContext().video.camRotX); return GM_OK; }
int GM_CDECL gmf_getCameraRotationY(gmThread* a_thread) { a_thread->PushFloat(GetEngineContext().video.camRotY); return GM_OK; }
int GM_CDECL gmf_getCameraRotationZ(gmThread* a_thread) { a_thread->PushFloat(GetEngineContext().video.camRotZ); return GM_OK; }
int GM_CDECL gmf_setCameraPos(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(y, 1, 0.0f); GM_FLOAT_OR_INT_PARAM(z, 2, 0.0f);
    if (getenv("GE3_DEATH_DIAG")) fprintf(stderr, "[camsetdiag] setCameraPos(%.3f, %.3f, %.3f)\n", x, y, z);
    VideoState& v = GetEngineContext().video;
    v.camPosX = x; v.camPosY = y; v.camPosZ = z;
    // RETAIL BYTECODE: FUN_00052908 stores raw arguments at +0x81d4/+0x81d8/
    // +0x81dc. The retail world-view routine calls the bounded getters at
    // 0x52934/0x529a0/0x52a0c; apply those effective values to desktop view,
    // culling, and camera-relative draws while retaining the raw fields.
    GetEngineContext().renderer->SetCameraPos(
        static_cast<float>(v.GetRetailCameraX()),
        static_cast<float>(v.GetRetailCameraY()),
        static_cast<float>(v.GetRetailCameraZ()));
    return GM_OK;
}
// Verification: Video.setCameraRotation's retail wrapper forwards four
// numeric arguments to FUN_00052a78, which stores angle/axis at
// +0x81fc/+0x8200/+0x8204/+0x8208. FUN_0004cea0 reads those fields for the
// world matrix. The GL transform is an axis-angle matrix adaptation; its
// composition still needs a retail numerical comparison.
int GM_CDECL gmf_setCameraRotation(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(4);
    GM_FLOAT_OR_INT_PARAM(angle, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(x, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y, 2, 0.0f); GM_FLOAT_OR_INT_PARAM(z, 3, 0.0f);
    VideoState& v = GetEngineContext().video;
    v.camRotAngle = angle; v.camRotX = x; v.camRotY = y; v.camRotZ = z;
    if (GetEngineContext().renderer)
        GetEngineContext().renderer->SetCameraRotation(angle, x, y, z);
    return GM_OK;
}
// RETAIL BYTECODE: wrapper at 0x75b40 converts float/int to a numeric value
// and calls FUN_00052af8; PPC 0x52af8 clamps to [0,1] before storing +0x820c.
int GM_CDECL gmf_setGrayscalePerc(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    GetEngineContext().video.grayscalePerc = VideoState::RetailGrayscaleValue(v);
    return GM_OK;
}
// RETAIL BYTECODE: wrapper at 0x75bd0 calls FUN_00052b28, a direct lfs of
// +0x820c. Since the native setter stores a bounded value, return that state.
int GM_CDECL gmf_getGrayscalePerc(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.grayscalePerc);
    return GM_OK;
}
int GM_CDECL gmf_isShowingFPS(gmThread* a_thread) { a_thread->PushInt(GetEngineContext().video.showFPS ? 1 : 0); return GM_OK; }
int GM_CDECL gmf_setIsShowingFPS(gmThread* a_thread) {
    GetEngineContext().video.showFPS = RetailRawBoolArgument(a_thread);
    return GM_OK;
}
// RETAIL BYTECODE: Video.getFPS calls FUN_00051d10. PPC computes single-
// precision 1000 ms / current frame time, then truncates with fctiwz.
int GM_CDECL gmf_getFPS(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.GetRetailFPS());
    return GM_OK;
}
// RETAIL BYTECODE: Video.getFrameTime calls FUN_00051d48, a direct float
// load of the timing manager's current-frame elapsed milliseconds at +0xeb8.
int GM_CDECL gmf_getFrameTime(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.frameTimeMs);
    return GM_OK;
}
// RETAIL BYTECODE: Video.traceFPSInfoNextFrame calls FUN_0004f208, which sets
// the graphics-state byte +0x8320 to 1. The retail draw path checks it under
// showFPS (+0x81c1); this byte is not cleared in the inspected PPC path.
int GM_CDECL gmf_traceFPSInfoNextFrame(gmThread* a_thread) {
    GetEngineContext().video.traceFPSInfo = true;
    return GM_OK;
}
// RETAIL BYTECODE: Video.setLineWidth accepts exactly zero or one argument:
// zero selects DAT_000774dc (2.0), one converts float payload or signed int
// payload to float, and counts above one do nothing. FUN_00052550 stores +0x8210.
int GM_CDECL gmf_setLineWidth(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count > 1) return GM_OK;
    const float w = count == 0
        ? 2.0f
        : RetailRawFloatOrIntArgument(a_thread, 0, 2.0f);
    GetEngineContext().video.lineWidth = w;
    // Apply the state to the desktop renderer. No Sound Shapes evidence
    // supports the former deviceWidth/800 scaling rule.
    if (Renderer* r = GetEngineContext().renderer) r->SetLineWidth(w);
    return GM_OK;
}
int GM_CDECL gmf_setTextShadowSize(gmThread* a_thread) {
    // RETAIL BYTECODE: no count/type guard; float tag reads float payload,
    // every other tag is sign-extended as an integer before float conversion.
    const float v = RetailRawFloatOrIntArgument(a_thread, 0);
    GetEngineContext().video.textShadowSize = v;
    return GM_OK;
}
int GM_CDECL gmf_getTextShadowSize(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.textShadowSize);
    return GM_OK;
}
int GM_CDECL gmf_getTextWidth(gmThread* a_thread) {
    // RETAIL BYTECODE: arg 0 is a GM string, arg 1 is the scale. The wrapper
    // reads arg 1's payload as float for GM_FLOAT or signed int otherwise;
    // the integer path uses fcfid followed by frsp before FUN_00052c20.
    GM_STRING_PARAM(text, 0, "");
    EngineContext& ctx = GetEngineContext();
    const float scale = RetailRawFloatOrIntArgument(a_thread, 1);
    float w = ctx.currentFont
        ? ctx.currentFont->MeasureRetailTextWidth(text, static_cast<double>(scale))
        : 0.0f;
    a_thread->PushFloat(w);
    return GM_OK;
}
int GM_CDECL gmf_setIsShouldDrawEntities(gmThread* a_thread) {
    // RETAIL BYTECODE: calls FUN_0004f21c with (argument payload != 0);
    // no GM type or argument-count validation is performed by the wrapper.
    GetEngineContext().video.shouldDrawEntities = RetailRawBoolArgument(a_thread);
    return GM_OK;
}
int GM_CDECL gmf_isShouldDrawEntities(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.shouldDrawEntities ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getScreenWidth(gmThread* a_thread) {
    // RETAIL BYTECODE: reads native +0xd4, converts it to f32, then applies
    // fmuls by 0.61f, 2.0f, and 2.0f in that order (0x19f1f0).
    a_thread->PushFloat(VideoState::RetailScreenExtent(
        GetEngineContext().video.numCellsX));
    return GM_OK;
}
int GM_CDECL gmf_getScreenHeight(gmThread* a_thread) {
    // RETAIL BYTECODE: same operation order using native +0xd8 (0x19f2b0).
    a_thread->PushFloat(VideoState::RetailScreenExtent(
        GetEngineContext().video.numCellsY));
    return GM_OK;
}
int GM_CDECL gmf_flushStaticSurface(gmThread* a_thread) {
    GetEngineContext().video.needsFlushStaticSurface = true;
    return GM_OK;
}
// RETAIL BYTECODE VERIFIED at 0x7a680: no arguments are read; calls
// FUN_0004f23c and pushes its signed integer result. The helper returns the
// engine texture-pool capacity minus bytes used. Renderer tracks desktop
// texture allocations against the recovered 170 MiB pool capacity; exact
// native format/alignment accounting remains under audit.
int GM_CDECL gmf_getTextureMemFree(gmThread* a_thread) {
    Renderer* renderer = GetEngineContext().renderer;
    a_thread->PushInt(renderer ? renderer->GetTextureMemoryFreeBytes() : 0);
    return GM_OK;
}
int GM_CDECL gmf_getResizeFactor(gmThread* a_thread) {
    // RETAIL BYTECODE: Video.getResizeFactor at 0x7ae68 reads its sole
    // argument's native TEXTURE pointer, then loads the float at +0x3c and
    // returns it unchanged. This is the library-function spelling of
    // TEXTURE.getResizeFactor; it does not read Video state.
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable& texture = a_thread->Param(0);
    if (texture.m_type != g_textureType) return GM_EXCEPTION;
    a_thread->PushFloat(ResolveTextureDimensionScale(*a_thread->GetMachine(), texture));
    return GM_OK;
}

// --- confirmed literal no-ops in retail (see agent survey of Video.c) ---
// BYTECODE VERIFIED NO-OP: retail 0x7723c is `li r3,0; blr`.
int GM_CDECL gmf_clearDepthBuffer(gmThread* a_thread) { return GM_OK; }
// BYTECODE VERIFIED NO-OP: retail 0x77244 is `li r3,0; blr`.
int GM_CDECL gmf_clearColourBuffer(gmThread* a_thread) { return GM_OK; }
// BYTECODE VERIFIED NO-OP: retail 0x7903c is `li r3,0; blr`.
int GM_CDECL gmf_resetViewport(gmThread* a_thread) { return GM_OK; }
// BYTECODE VERIFIED NO-OP: retail 0x7937c is `li r3,0; blr`.
int GM_CDECL gmf_destroyAllRenderBufferTextures(gmThread* a_thread) { return GM_OK; }
// BYTECODE VERIFIED NO-OP: retail 0x79500 is `li r3,0; blr`.
int GM_CDECL gmf_clearStencilBuffer(gmThread* a_thread) { return GM_OK; }
// BYTECODE VERIFIED NO-OP: retail 0x7be9c is `li r3,0; blr`.
int GM_CDECL gmf_setDebugID(gmThread* a_thread) { return GM_OK; }
// Confirmed real body is unconditionally `return 0xffffffffffffffff;` (the
// same "-1" convention every other confirmed function here uses for a bad
// argcount) with no branching at all -- looks like retail itself shipped
// this disabled/broken. Only real call site in the whole corpus
// (game_data/Platformer/EntityMenuTopBase.gm:77) is commented out, so this
// is never actually exercised.
int GM_CDECL gmf_purgeUnusedTextures(gmThread* a_thread) { return GM_EXCEPTION; }
// RETAIL BYTECODE VERIFIED: EBOOT 0x79044 logs the exact deprecation text at
// level 0, pushes no return value, and returns zero. Host logging has no
// retail level-0 channel, so preserve the message verbatim on stderr.
int GM_CDECL gmf_isPointInView(gmThread* a_thread) {
    (void)a_thread;
    std::fprintf(stderr, "WARNING: Video.isPointInView not longer used.\n");
    return GM_OK;
}
// BYTECODE VERIFIED: retail 0x79a90 writes a null GM variable to the return
// slot and increments the stack top. No Video post-process texture exists.
int GM_CDECL gmf_getPostProcessTexture(gmThread* a_thread) { a_thread->PushNull(); return GM_OK; }
// Verification: Video.createRenderBufferTexture at 0x000792c4 always pushes
// integer -1. Retail's Video subsystem also constructs its private RenderBuffer
// texture during FUN_000566a8; that object is separate from this public factory.
int GM_CDECL gmf_createRenderBufferTexture(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }
// Verification: Video.getRenderBufferMode at 0x00079634 always pushes -1.
int GM_CDECL gmf_getRenderBufferMode(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }
// Verification: Video.getRenderBufferModeTextureID at 0x00079690 always pushes -1.
int GM_CDECL gmf_getRenderBufferModeTextureID(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }

// Draw-buffer manager lifecycle is implemented from PPC 0x79080/0x790dc/
// 0x79138 and helpers 0x525dc/0x527e4/0x528f4. VIDEODRAWBUFFER's remaining
// methods are individually trapped in video_draw_buffer_bindings.cpp until
// their bytecode paths are implemented.
// Verification: Video.useRenderBufferInShader (0x000791e4) only calls
// FUN_00052b38/FUN_00052d28. Both helpers perform list lookup and return the
// found pointer; their results are discarded and neither changes GPU state.
int GM_CDECL gmf_useRenderBufferInShader(gmThread* a_thread) { (void)a_thread; return GM_OK; }
// ENGINE PATCH (2026-08-17): moved out of the "genuinely deferred, trap
// rather than guess" cluster above -- confirmed real, HALTING crash
// (Platformer/Main.gm:1139, `destructor=function(){...Video.
// deactivateShaders();...}`), called UNCONDITIONALLY on every single real
// level teardown, before EntityManager.deleteAllEntities() a few lines
// later ever runs. Under this engine's default GE3_STRICT=halt mode this
// killed the whole process the instant a player exited ANY level; under
// survey mode it silently logged-and-continued, which is why this stayed
// hidden through this session's own survey-mode testing despite being the
// most commonly-hit trap in the whole cluster by a wide margin. This
// renderer has no persistent custom-shader-activation state to deactivate
// (see gmf_shapeSetShaderBasedOnTexture's own header comment for the same
// "no shader-variant system exists here" reasoning) -- genuine no-op.
int GM_CDECL gmf_deactivateShaders(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// RETAIL DISPATCH MATCH: exhaustive raw-string inspection of EBOOT found no
// Video.destroyAllShaders method entry. The shipped Main.gm still calls it;
// retail therefore takes GameMonkey's unbound-method path, which returns
// without side effects. Bind that same no-op locally so strict-call mode does
// not stop level transitions on a method retail itself never registered.
int GM_CDECL gmf_destroyAllShaders(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// Verification: Video.destroyRenderBufferTexture (0x00079320),
// beginRenderBufferTexture (0x00079440), and endRenderBufferTexture
// (0x000794a0) delegate to FUN_00052b90/FUN_00052bdc. Those helpers only
// look up the supplied ID and discard the result, so these retail calls have
// no externally visible rendering or lifetime effect.
int GM_CDECL gmf_destroyRenderBufferTexture(gmThread* a_thread) { (void)a_thread; return GM_OK; }
// Verification: Video.setRenderBufferMode (0x00079384) accepts one or two
// arguments; valid calls delegate to FUN_00052bb0, which only looks up the ID
// and discards the result. The retail wrapper returns -1 for other arities.
int GM_CDECL gmf_setRenderBufferMode(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    return (count == 1 || count == 2) ? GM_OK : GM_EXCEPTION;
}
// Verification: Video.beginRenderBufferTexture (0x00079440) only looks up
// the ID through FUN_00052bdc; the lookup result is discarded.
int GM_CDECL gmf_beginRenderBufferTexture(gmThread* a_thread) { (void)a_thread; return GM_OK; }
// Verification: Video.endRenderBufferTexture (0x000794a0) has the same
// lookup-only effect through FUN_00052bdc as the begin wrapper.
int GM_CDECL gmf_endRenderBufferTexture(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_debugSetWorldMarker(gmThread* a_thread) {
    // BYTECODE VERIFIED at 0x78808: only argc 9 and 10 reach the two helper
    // branches; all other counts return -1. The 9-arg branch reads arg0 as
    // vector2D then args1..7 as float-or-signed-int, with arg8's raw payload
    // as the flag. The 10-arg branch reads args0..8 as scalar values and
    // arg9's raw payload as the flag. FUN_0004ef20 appends the 9-float record;
    // PPC multiplies its final two size fields by the exact 0.5f literal at
    // 0x4ef1c before storing them at record offsets +0x20/+0x24.
    const int count = a_thread->GetNumParams();
    if (count != 9 && count != 10) return GM_EXCEPTION;

    VideoState::DebugWorldMarker marker;
    if (count == 9) {
        float x = 0.0f, y = 0.0f;
        if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y))
            return GM_EXCEPTION;
        marker.values[0] = x;
        marker.values[1] = y;
        for (int i = 1; i <= 7; ++i)
            marker.values[i + 1] = RetailRawFloatOrIntArgument(a_thread, i);
        marker.flag = a_thread->Param(8).m_value.m_int != 0;
    } else {
        for (int i = 0; i <= 8; ++i)
            marker.values[i] = RetailRawFloatOrIntArgument(a_thread, i);
        marker.flag = a_thread->Param(9).m_value.m_int != 0;
    }
    marker.values[7] = marker.values[7] * 0.5f;
    marker.values[8] = marker.values[8] * 0.5f;
    GetEngineContext().video.debugWorldMarkers.push_back(marker);
    return GM_OK;
}
int GM_CDECL gmf_debugSetWorldLineMarker(gmThread* a_thread) {
    // BYTECODE VERIFIED at 0x78c24: argc 8 reads two vector2D values followed
    // by five float-or-signed-int values and a raw-payload flag; argc 10 reads
    // nine scalar values and a raw-payload flag. argc 9 and other counts
    // return -1. FUN_0004f054 appends the same nine-float payload shape.
    const int count = a_thread->GetNumParams();
    if (count != 8 && count != 10) return GM_EXCEPTION;

    VideoState::DebugWorldMarker marker;
    marker.isLine = true;
    if (count == 8) {
        float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
        if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x0, y0) ||
            !GetVector2D(*a_thread->GetMachine(), a_thread->Param(1), x1, y1))
            return GM_EXCEPTION;
        marker.values[0] = x0;
        marker.values[1] = y0;
        marker.values[2] = x1;
        marker.values[3] = y1;
        for (int i = 2; i <= 6; ++i)
            marker.values[i + 2] = RetailRawFloatOrIntArgument(a_thread, i);
        marker.flag = a_thread->Param(7).m_value.m_int != 0;
    } else {
        for (int i = 0; i <= 8; ++i)
            marker.values[i] = RetailRawFloatOrIntArgument(a_thread, i);
        marker.flag = a_thread->Param(9).m_value.m_int != 0;
    }
    GetEngineContext().video.debugWorldMarkers.push_back(marker);
    return GM_OK;
}
int GM_CDECL gmf_debugClearWorldMarkers(gmThread* a_thread) {
    // BYTECODE VERIFIED at 0x78fec: FUN_0004f178 relinks the intrusive-list
    // sentinel, zeros the node count, then frees each list node. It does not
    // dereference the node payload in the inspected PPC, so marker-record
    // destruction remains unresolved. Locally this destroys vector elements;
    // std::vector::clear() retains capacity and is not native lifetime parity.
    (void)a_thread;
    GetEngineContext().video.debugWorldMarkers.clear();
    return GM_OK;
}
// RETAIL BYTECODE MATCH: these wrappers call literal one-instruction
// `blr` helpers: setScissorBox -> FUN_00052db0, enableScissorBox ->
// FUN_00052da8, and disableScissorBox -> FUN_00052dac. They do not change
// RSX or device state in this retail build; keep the desktop equivalents
// inert instead of turning ListView's normal calls into strict-mode traps.
int GM_CDECL gmf_setScissorBox(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_enableScissorBox(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_disableScissorBox(gmThread* a_thread) { (void)a_thread; return GM_OK; }
// The Video wrapper at EBOOT 0x79b08 accepts 3, 4, or 6 parameters. The
// short forms look up a named colour and optionally take alpha; the 6-param
// form supplies RGBA. All routes load/resolve the texture in GAMEDATA and
// call FUN_0031b254(index,R,G,B,A). Other arities return GM_EXCEPTION.
uint8_t RetailPaletteByteFromFloat(float a_value) {
    const float scaled = a_value * 255.0f; // PPC fmuls before fctiwz
    if (!std::isfinite(scaled) || scaled < -2147483648.0f ||
        scaled >= 2147483648.0f) return 0; // PPC invalid-conversion low byte
    const int32_t converted = static_cast<int32_t>(std::trunc(scaled));
    return static_cast<uint8_t>(static_cast<uint32_t>(converted) & 0xffu);
}

uint8_t RetailPaletteByteFromFloatFctidz(float a_value) {
    // RETAIL PPC: every numeric channel is converted to f32 first (integer
    // payloads use fcfid+frsp), multiplied by the f32 255.0 literal with
    // fmuls, then converted with fctidz. The wide integer conversion does
    // not make the preceding multiply double precision.
    volatile float scaled = a_value * 255.0f;
    constexpr double kInt64Min = -9223372036854775808.0;
    constexpr double kInt64Limit = 9223372036854775808.0;
    if (!std::isfinite(scaled) || static_cast<double>(scaled) < kInt64Min ||
        static_cast<double>(scaled) >= kInt64Limit)
        return 0;
    const int64_t converted = static_cast<int64_t>(std::trunc(static_cast<double>(scaled)));
    return static_cast<uint8_t>(static_cast<uint64_t>(converted) & 0xffu);
}

float RetailPaletteScalarAsFloat(const gmVariable& a_value) {
    // The PPC wrappers convert integer payloads with fcfid then frsp before
    // the fmuls, while GM_FLOAT payloads already contain f32.
    return a_value.m_type == GM_FLOAT
        ? a_value.m_value.m_float
        : static_cast<float>(a_value.m_value.m_int);
}

int GM_CDECL gmf_setTexturePaletteColour(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 3 && count != 4 && count != 6) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    const int index = a_thread->Param(1).m_value.m_int;

    uint8_t r = 0, g = 0, b = 0, alpha = 255;
    if (count == 3 || count == 4) {
        GM_STRING_PARAM(colourName, 2, "");
        float rf = 0.0f, gf = 0.0f, bf = 0.0f;
        // This resolves the desktop's current-scheme entry. Retail
        // FUN_000b35c8 can also interpolate while the ColourManager is in a
        // scheme transition; that transition state is not modeled here yet.
        GetNamedColourRGB(colourName, rf, gf, bf);
        r = RetailPaletteByteFromFloat(rf);
        g = RetailPaletteByteFromFloat(gf);
        b = RetailPaletteByteFromFloat(bf);
        if (count == 4) alpha = RetailPaletteByteFromFloatFctidz(
            RetailPaletteScalarAsFloat(a_thread->Param(3)));
    } else {
        r = RetailPaletteByteFromFloatFctidz(RetailPaletteScalarAsFloat(a_thread->Param(2)));
        g = RetailPaletteByteFromFloatFctidz(RetailPaletteScalarAsFloat(a_thread->Param(3)));
        b = RetailPaletteByteFromFloatFctidz(RetailPaletteScalarAsFloat(a_thread->Param(4)));
        alpha = RetailPaletteByteFromFloatFctidz(RetailPaletteScalarAsFloat(a_thread->Param(5)));
    }

    // FUN_0004bd50 uses the GAMEDATA resource manager in every branch. Keep
    // this manager-owned lease until Video's purge/unload path releases it.
    const auto resolved = ResolveTextureAssetPath(path, 1);
    unsigned int handle = 0;
    if (resolved) {
        Renderer* renderer = GetEngineContext().renderer;
        handle = renderer->LoadTexture(*resolved, "1:" + std::string(path));
        RetainManagedTextureResource(handle, 1, path);
    } else {
        // BYTECODE VERIFIED: FUN_0004bd50 returns the shared Video
        // badTexture_Yo resource when the GAMEDATA lookup fails; the wrapper
        // still calls FUN_0031b254 on that returned texture. Preserve that
        // dirty-state side effect instead of silently skipping the write.
        handle = EnsureBadTextureFallback();
    }
    SetTexturePaletteColourByHandle(handle, index, r, g, b, alpha);
    return GM_OK;
}
GE3_REMOVED(gmf_setGeneratedPalette, "Video.setGeneratedPalette")
GE3_REMOVED(gmf_convertPaletteToAlpha, "Video.convertPaletteToAlpha")
// BYTECODE: Video.takeScreenShot forwards its second argument (FILEHELPER
// area) into the native screenshot request. The retail request eventually
// reaches FUN_00170148: area 0 returns immediately, while any nonzero area
// logs the retail FILEHELPER line-457 warning that file writing is not
// permitted. The shipped ScreenshotScriptLine.gm passes GAMEDATA (1), so
// retail does not create its requested PNG. Keep the frame capture/write
// path inert for that retail behavior and reproduce the observed diagnostic.
int GM_CDECL gmf_takeScreenShot(gmThread* a_thread) {
    if (a_thread->GetNumParams() > 1 && a_thread->Param(1).m_value.m_int != 0)
        std::fprintf(stderr, "FILEHELPER[ln 457] file writing in retail is not permitted!\n");
    return GM_OK;
}
GE3_REMOVED(gmf_setTextureShaderType, "Video.setTextureShaderType")
GE3_REMOVED(gmf_setShaderBasedOnTexture, "Video.setShaderBasedOnTexture")
int GM_CDECL gmf_getDefaultPostProcessShader(gmThread* a_thread) {
    // BYTECODE VERIFIED at 0x7a570: allocate a four-byte native holder,
    // initialize it from Video+0x8150, retain the shader pointer if present,
    // and wrap the holder as SHADER. The Video constructor initializes
    // +0x8150 to null, but retail still returns a SHADER userdata wrapper.
    a_thread->Push(CreateDefaultPostProcessShaderHandle(*a_thread->GetMachine()));
    return GM_OK;
}
// ColourManager.makeInterpolatedColour is the real interpolation path we
// DO implement; this raw Video-level variant isn't -- genuinely deferred.
// Bytecode verified against the retail wrapper at EBOOT 0x76a10
// args 1 and 2 must each be a native SHAPECOLLISIONINFO (runtime type 37)
// or COLOUR object. The optional fourth scalar is converted using the same
// float-tag / signed-int-payload rule as arg 0. The called interpolation
// helpers compute a temporary RGB value and end at FUN_0004ef18, a literal
// `blr`, so no renderer colour or draw-list state changes. Do not redirect
// this to VMDRAWLIST.setInterpolatedColour, whose recording behavior differs.
int GM_CDECL gmf_setInterpolatedColour(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 3 && count != 4) return GM_EXCEPTION;

    const auto isRetailColourSource = [a_thread](int index) {
        const gmVariable& value = a_thread->Param(index);
        if (value.m_type != GetShapeCollisionInfoType() && value.m_type != g_colourType)
            return false;
        gmObject* object = a_thread->GetMachine()->GetObject(value.m_value.m_ref);
        if (!object) return false;
        auto* userObject = static_cast<gmUserObject*>(object);
        return userObject->m_userType == value.m_type && userObject->m_user != nullptr;
    };

    if (!isRetailColourSource(1) || !isRetailColourSource(2)) return GM_EXCEPTION;

    // Retail reads arg 0 and, for the 4-arg form, arg 3 as float when tagged
    // GM_FLOAT and otherwise sign-extends the raw payload. Those values only
    // feed temporary interpolation math before the verified terminal no-op;
    // there is consequently no persistent desktop state to update here.
    return GM_OK;
}
// RETAIL BYTECODE: wrappers at 0x7a268/0x7a2c8 forward Video and the raw
// object payload to FUN_0004e854/FUN_0004e7cc. For the desktop's represented
// ENTITY class, retain the table's object identity in Video's ordered draw
// map; native embedded drawable classes still need their own adapters.
int GM_CDECL gmf_addToDrawMap(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const gmVariable& value = a_thread->Param(0);
    if (value.m_type != GM_TABLE) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    gmObject* object = machine->GetObject(value.m_value.m_ref);
    gmTableObject* table = object ? static_cast<gmTableObject*>(object) : nullptr;
    if (!table) return GM_EXCEPTION;
    const gmVariable id = table->Get(machine, "__objectID");
    if (id.m_type != GM_INT || id.m_value.m_int <= 0) return GM_EXCEPTION;
    const uintptr_t key = reinterpret_cast<uintptr_t>(table);
    GetEngineContext().video.drawMapEntities[key] = id.m_value.m_int;
    return GM_OK;
}
int GM_CDECL gmf_removeFromDrawMap(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const gmVariable& value = a_thread->Param(0);
    if (value.m_type != GM_TABLE) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    gmObject* object = machine->GetObject(value.m_value.m_ref);
    gmTableObject* table = object ? static_cast<gmTableObject*>(object) : nullptr;
    if (!table) return GM_EXCEPTION;
    GetEngineContext().video.drawMapEntities.erase(reinterpret_cast<uintptr_t>(table));
    return GM_OK;
}
GE3_REMOVED(gmf_drawModTexture, "Video.drawModTexture")
GE3_REMOVED(gmf_drawShape, "Video.drawShape")
GE3_REMOVED(gmf_setTexture, "Video.setTexture")
GE3_REMOVED(gmf_setTextureByHandle, "Video.setTextureByHandle")
GE3_REMOVED(gmf_clearTexture, "Video.clearTexture")
GE3_REMOVED(gmf_setTextureSubImg, "Video.setTextureSubImg")
int GM_CDECL gmf_Video_clear(gmThread* a_thread) {
    // BYTECODE VERIFIED: Video.clear at EBOOT 0x759d8 accepts 0..4 arguments
    // and returns GM_OK for each count; 1/2-argument calls may resolve a
    // temporary color object, but the called path does not persist render
    // state. Counts above 4 return GM_EXCEPTION.
    const int count = a_thread->GetNumParams();
    return count <= 4 ? GM_OK : GM_EXCEPTION;
}
// BYTECODE VERIFIED: retail 0x799f4 only performs a temporary retain/release
// on its draw-list argument; it does not append to a post-process queue. For
// a live GM argument the net refcount effect is zero.
int GM_CDECL gmf_appendToPostProcessDList(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_artTestUnloadTexturesFromFile(gmThread* a_thread) { return GM_OK; } // debug/test tool, no-op is correct outside the art pipeline
// RETAIL BYTECODE: wrapper 0x7a72c accepts 0/1 args, defaults to delay 3,
// forwards the raw signed payload otherwise, and rejects all other arities.
// FUN_0004f258 stores max(delay, 1) at Video +0x83a4.
int GM_CDECL gmf_purgeUnusedTexturesDelayed(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count > 1) return GM_EXCEPTION;
    int delay = count == 0 ? 3 : a_thread->Param(0).m_value.m_int;
    if (delay <= 0) delay = 1;
    GetEngineContext().video.delayedTexturePurgeFrames = delay;
    return GM_OK;
}

// --- texture management (reuses Renderer's cache/refcounting) ---

static std::optional<std::string> ResolvePlatformResourcePathImpl(const char* a_path, int a_area) {
    EngineContext& ctx = GetEngineContext();
    if (a_area == 1 && ctx.fileHelper) {
        const std::string requested = a_path ? a_path : "";
        // RETAIL BYTECODE: FUN_0004ab18 constructs "PS3_" + requested path
        // when the PS3 graphics override is active. It checks that candidate
        // in the requested file area and loads it when present; only a miss
        // restores the original path. Keep this in texture resolution rather
        // than FileHelper's generic path rules, since script/data reads do not
        // use Video's graphics-mode override.
        if (ctx.video.textureResizeOverrideEnabled) {
            const auto ps3Path = ctx.fileHelper->Resolve("PS3_" + requested);
            if (ps3Path) return ps3Path;
        }
        return ctx.fileHelper->Resolve(requested);
    }
    std::string fullPath;
    if (a_area == 0) fullPath = SaveData::Dir() + "/" + a_path;
    else if (a_area == 2) fullPath = a_path;
    else return std::nullopt;
    std::error_code ec;
    if (!std::filesystem::is_regular_file(fullPath, ec) || ec) return std::nullopt;
    return fullPath;
}

// RETAIL BYTECODE: FUN_0004ab18 defaults TEXTURE+0x3c to 1.0. When the
// graphics-mode flag (+0x8184) is set, it checks for "PS3_" + requested path
// in the same file area and selects 0.5 only on a successful resource lookup.
float RetailTextureResizeFactor(int a_area, const std::string& a_path) {
    const VideoState& video = GetEngineContext().video;
    if (!video.textureResizeOverrideEnabled) {
        if (std::getenv("GE3_TRACE_TEXTURE_FACTOR"))
            std::fprintf(stderr, "[texture-factor] area=%d path=%s flag=0 factor=1.0\n",
                         a_area, a_path.c_str());
        return 1.0f;
    }
    const std::string ps3Path = "PS3_" + a_path;
    EngineContext& ctx = GetEngineContext();
    const auto overrideResource = a_area == 1 && ctx.fileHelper
        ? ctx.fileHelper->Resolve(ps3Path)
        : std::optional<std::string>{};
    const float factor = overrideResource ? 0.5f : 1.0f;
    if (std::getenv("GE3_TRACE_TEXTURE_FACTOR"))
        std::fprintf(stderr, "[texture-factor] area=%d path=%s candidate=%s hit=%d factor=%.1f\n",
                     a_area, a_path.c_str(), ps3Path.c_str(), overrideResource ? 1 : 0, factor);
    return factor;
}

int PushTextureForScript(gmThread* a_thread, int a_maxParams) {
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > a_maxParams) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    int area = 1; // Retail Video.getTexture defaults to FILEHELPER_AREA_GAMEDATA.
    if (count >= 2) area = a_thread->Param(1).m_value.m_int;

    EngineContext& ctx = GetEngineContext();
    auto resolved = ResolveTextureAssetPath(path, area);
    if (!resolved) {
        // RETAIL BYTECODE: FUN_0004ab18's failed lookup returns the Video
        // object's shared badTexture_Yo at +0x838c, rather than a null TEXTURE.
        a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(),
                                         EnsureBadTextureFallback()));
        return GM_OK;
    }
    const std::string cacheKey = std::to_string(area) + ":" + path;
    unsigned int handle = ctx.renderer->LoadTexture(*resolved, cacheKey);
    if (handle == 0) {
        // FUN_0004ab18 also takes its shared badTexture_Yo branch when the
        // resource exists but cannot be decoded/created as a TEXTURE.
        a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(),
                                         EnsureBadTextureFallback()));
        return GM_OK;
    }
    auto lease = MakeTextureResourceLease(handle, area, path);
    const float resizeFactor = RetailTextureResizeFactor(area, path);
    a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(), handle,
                                    std::move(lease), resizeFactor));
    return GM_OK;
}

int GM_CDECL gmf_Video_getTexture(gmThread* a_thread) {
    return PushTextureForScript(a_thread, 2);
}

int GM_CDECL gmf_loadTexture(gmThread* a_thread) {
    // Retail 0x77910 confirms 1-3 args (path,[area],[out info table]) and
    // returns no value. Its resource manager owns the load until an explicit
    // unload; this is distinct from getTexture, which returns TEXTURE userdata.
    // The 3-arg
    // form's table gets width/height/bpp written into it -- confirmed real
    // and load-bearing, not just an opts bag: EntityPacks/
    // TutorialGraphicsEntity.gm's `Video.loadTexture(this.ad.filename,fa,
    // this.tli);` (the in-level tutorial-hint/"Welcome to Sound Shapes!"
    // graphics) branches its entire draw() on `this.tli.bpp==32`/`==8`,
    // and sizes itself from `this.tli.width`/`.height` -- leaving the table
    // untouched (previously just forwarding to gmf_Video_getTexture, which
    // doesn't even look at param 2) meant bpp stayed null forever, so
    // NEITHER branch matched and drawTexture() was never reached at all.
    // The optional metadata record is populated from the loaded image's
    // source dimensions and encoded bits-per-pixel, then exposed as GM_INT
    // fields. Retail FUN_0004ab18 reads resource-record +8/+0c/+10 and the
    // wrapper's PPC stores the local variant tag 1 before each table write.
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > 3) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    // Retail forwards the area's raw 32-bit payload to FUN_0004b1d8.
    const int area = count >= 2 ? a_thread->Param(1).m_value.m_int : 1;
    auto resolved = ResolveTextureAssetPath(path, area);
    unsigned int handle = 0;
    if (resolved) {
        const std::string cacheKey = std::to_string(area) + ":" + path;
        handle = GetEngineContext().renderer->LoadTexture(*resolved, cacheKey);
        RetainManagedTextureResource(handle, area, path);
    }
    const float resizeFactor = RetailTextureResizeFactor(area, path);
    if (a_thread->GetNumParams() >= 3 && a_thread->Param(2).m_type == GM_TABLE) {
        int w = 0, h = 0;
        if (handle) GetEngineContext().renderer->GetTextureDimensions(handle, w, h);
        w = static_cast<int>(static_cast<double>(w) * resizeFactor);
        h = static_cast<int>(static_cast<double>(h) * resizeFactor);
        const int bpp = handle
            ? GetEngineContext().renderer->GetTextureSourceBitsPerPixel(handle) : 0;
        gmTableObject* info = (gmTableObject*)a_thread->GetMachine()->GetObject(a_thread->Param(2).m_value.m_ref);
        if (info) {
            info->Set(a_thread->GetMachine(), "width", gmVariable((gmint)w));
            info->Set(a_thread->GetMachine(), "height", gmVariable((gmint)h));
            info->Set(a_thread->GetMachine(), "bpp", gmVariable((gmint)bpp));
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_loadTextureFromSaveRequest(gmThread* a_thread) {
    // Retail 0x77c2c accepts a SAVEREQUEST, checks its error/data fields,
    // decodes the request bytes through FUN_0004b89c, and returns a TEXTURE
    // object (or the Video fallback texture on failure). Desktop save
    // requests are synchronous ticket tables; resolve their savedata path and
    // decode the same encoded image bytes into a cached TEXTURE resource.
    gmMachine& machine = *a_thread->GetMachine();
    std::string path;
    bool requestHasError = false;
    gmVariable request = a_thread->GetNumParams() > 0 ? a_thread->Param(0) : gmVariable();
    if (request.m_type == GM_TABLE) {
        gmTableObject* table = (gmTableObject*)machine.GetObject(request.m_value.m_ref);
        gmVariable filename = table ? table->Get(&machine, "filename") : gmVariable();
        gmVariable error = table ? table->Get(&machine, "error") : gmVariable();
        requestHasError = error.m_type == GM_INT && error.m_value.m_int != 0;
        if (filename.m_type == GM_STRING) {
            gmStringObject* string = (gmStringObject*)machine.GetObject(filename.m_value.m_ref);
            if (string) path = string->GetString();
        }
    } else if (request.m_type == GM_STRING) {
        gmStringObject* string = (gmStringObject*)machine.GetObject(request.m_value.m_ref);
        if (string) path = string->GetString();
    }

    auto pushFallback = [&]() {
        a_thread->Push(MakeTextureHandle(machine, EnsureBadTextureFallback()));
        return GM_OK;
    };
    // Retail tests SAVEREQUEST+0x50 bit 1 before examining the payload. The
    // synchronous desktop ticket exposes the same state as `error`.
    if (requestHasError) {
        std::fprintf(stderr, "ERROR: loading texture from save request had errors\n");
        return pushFallback();
    }

    std::string encoded;
    if (path.empty() || !SaveData::ReadFile(path, encoded) || encoded.empty()) {
        std::fprintf(stderr, "ERROR: loading texture from save request came back with 0 bytes\n");
        return pushFallback();
    }
    const std::string cacheKey = "0:" + path;
    Renderer* renderer = GetEngineContext().renderer;
    const unsigned int handle = renderer->LoadTextureFromMemory(
        (const unsigned char*)encoded.data(), encoded.size(), cacheKey);
    if (!handle) {
        std::fprintf(stderr, "ERROR: Failed to load texture \"%s\"\n", path.c_str());
        return pushFallback();
    }
    auto lease = MakeTextureResourceLease(handle, 0, path);
    RetainManagedTextureResource(handle, 0, path);
    a_thread->Push(MakeTextureHandle(machine, handle, std::move(lease)));
    return GM_OK;
}

int GM_CDECL gmf_unloadTexture(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 1) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    // Retail's wrapper reads the second raw gmVariable slot unconditionally;
    // unlike getTexture/unloadTextureByHandle, it has no argc/default branch.
    // Preserve that observable ABI, including its one-argument call-site
    // edge case, instead of inventing an area conversion/default here.
    const int area = a_thread->Param(1).m_value.m_int;
    a_thread->PushInt(UnloadTextureResourceByPath(area, path) ? 1 : 0);
    return GM_OK;
}

// FUN_0004a6bc resets the two native Video command-buffer structures. This
// renderer submits draw commands immediately and has no corresponding queues.
int GM_CDECL gmf_unloadAllTextures(gmThread* a_thread) { (void)a_thread; return GM_OK; }

int GM_CDECL gmf_getSubTexture(gmThread* a_thread) {
    // No texture-atlas sub-rect tracking -- returns the same handle back,
    // which is correct whenever the "sub" region is really the whole
    // texture (true for every non-atlas texture, i.e. everything this
    // renderer currently loads).
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(), handle,
        ResolveTextureResourceLease(*a_thread->GetMachine(), a_thread->Param(0)),
        ResolveTextureDimensionScale(*a_thread->GetMachine(), a_thread->Param(0))));
    return GM_OK;
}

// 0x00076498, a factory/constructor, not a stub). See texture_bindings.h's
// PALETTE type comment for the full pipeline this feeds
// (COLOURSCHEME_SetGeneratedPaletteColour(palette,...) then
// drawList.setTextureSubImg(texture,palette) to actually apply the tint).
int GM_CDECL gmf_createPalette(gmThread* a_thread) {
    a_thread->Push(MakePaletteHandle(*a_thread->GetMachine()));
    return GM_OK;
}

int GM_CDECL gmf_getDimensions(gmThread* a_thread) {
    // BYTECODE VERIFIED: the retail wrapper reads native +0x24/+0x28, applies
    // the TEXTURE +0x3c factor with `fmuls`, truncates each value with
    // `fctidz`, and returns the low 32-bit integer results as floats.
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable& value = a_thread->Param(0);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), value);
    int w = 0, h = 0;
    if (GetTextureNativeDimensions(*a_thread->GetMachine(), value, w, h)) {
        const float resizeFactor = ResolveTextureDimensionScale(*a_thread->GetMachine(), value);
        a_thread->Push(MakeVector2D(*a_thread->GetMachine(),
            RetailScaledTextureDimension(static_cast<uint32_t>(w), resizeFactor),
            RetailScaledTextureDimension(static_cast<uint32_t>(h), resizeFactor)));
        return GM_OK;
    }
    if (!handle) {
        a_thread->PushNull();
        return GM_OK;
    }
    GetEngineContext().renderer->GetTextureDimensions(handle, w, h);
    const float resizeFactor = ResolveTextureDimensionScale(*a_thread->GetMachine(),
                                                            value);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(),
        RetailScaledTextureDimension(static_cast<uint32_t>(w), resizeFactor),
        RetailScaledTextureDimension(static_cast<uint32_t>(h), resizeFactor)));
    return GM_OK;
}

int GM_CDECL gmf_getTextureExtX(gmThread* a_thread) {
    // RETAIL PPC VERIFIED FOR A LIVE TEXTURE: Video.getTextureExtX at 0x7a328 calls
    // FUN_0004c094, which returns the texture resource's float at +0x34.
    // The paired helper at 0x4c094 is a direct `lfs f1,0x34(r3); blr`.
    // Desktop resolves this through the GL handle map; native field lifetime
    // after unload and unchecked malformed-argument behavior remain open.
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    float uMax = 1.0f, vMax = 1.0f;
    GetEngineContext().renderer->GetTextureUVExtent(handle, uMax, vMax);
    a_thread->PushFloat(uMax);
    return GM_OK;
}
int GM_CDECL gmf_getTextureExtY(gmThread* a_thread) {
    // RETAIL PPC VERIFIED FOR A LIVE TEXTURE: Video.getTextureExtY at 0x7a44c calls
    // FUN_0004c0a0, whose body directly returns the texture resource's
    // float at +0x38 (`lfs f1,0x38(r3); blr`).
    // Desktop resolves this through the GL handle map; native field lifetime
    // after unload and unchecked malformed-argument behavior remain open.
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    float uMax = 1.0f, vMax = 1.0f;
    GetEngineContext().renderer->GetTextureUVExtent(handle, uMax, vMax);
    a_thread->PushFloat(vMax);
    return GM_OK;
}

// --- draw lists / immediate draw calls ---

int GM_CDECL gmf_createDrawList(gmThread* a_thread) {
    // BYTECODE VERIFIED: Video.createDrawList at EBOOT 0x796ec handles
    // argc 0/1/2 only and returns GM_EXCEPTION for argc > 2. It forwards
    // each supplied gmVariable's raw 32-bit payload, without type conversion.
    const int count = a_thread->GetNumParams();
    if (count > 2) return GM_EXCEPTION;
    gmint size = 0x400, align = 0x20;
    if (count >= 1) size = a_thread->Param(0).m_value.m_int;
    if (count >= 2) align = a_thread->Param(1).m_value.m_int;
    PushNewDrawList(a_thread, size, align);
    return GM_OK;
}
int GM_CDECL gmf_createDynamicDrawList(gmThread* a_thread) {
    // BYTECODE VERIFIED: the retail wrapper constructs only for argc == 2;
    // other counts return success without pushing a result. Both supplied
    // values are forwarded as raw 32-bit gmVariable payloads.
    if (a_thread->GetNumParams() == 2) {
        const gmint size = a_thread->Param(0).m_value.m_int;
        const gmint align = a_thread->Param(1).m_value.m_int;
        PushNewDrawList(a_thread, size, align);
    }
    return GM_OK;
}

// Confirmed real usage (game_data/Platformer/TouchEditProcess.gm) is
// exactly the 9-arg form: 8 corner coords + a texture path. Same shared-
// vocabulary reasoning as drawTextureByHandle below -- drawList.drawTexture
// (draw_list.cpp's registration) uses the identical DC_gmf_drawTexture
// implementation (draw_context.cpp); confirmed real there too via
// EntityPacks/TutorialGraphicsEntity.gm's in-level tutorial-hint graphics.
int GM_CDECL gmf_drawTexture(gmThread* a_thread) { return DC_gmf_drawTexture(a_thread); }
// Same shared-vocabulary reasoning -- drawList.drawAnimation (draw_list.cpp)
// uses the identical DC_gmf_drawAnimation implementation (animation_bindings.cpp).
int GM_CDECL gmf_drawAnimation(gmThread* a_thread) { return DC_gmf_drawAnimation(a_thread); }

// Confirmed real overload dispatch is more elaborate (see agent survey),
// but never exercised directly in the corpus -- the 9-arg handle form
// (matching drawList.drawTextureByHandle's confirmed shape) covers every
// case we can actually validate.
int GM_CDECL gmf_drawTextureByHandle(gmThread* a_thread) { return DC_gmf_drawTextureByHandle(a_thread); }

// --- restartVideo: the actual reported blocker ---

// RETAIL BYTECODE: 0, 3, or 4 args. Three dispatch to FUN_00051dc8; four
// dispatch to FUN_00051d5c. The wrapper forwards raw width/height payloads,
// but both helpers ignore them and copy Video+0x8168/+0x816c into
// +0x819c/+0x81a0. They store fullscreen from raw payload != 0 at +0x81b8;
// the 4-arg helper also stores raw depth at +0x81b4. Both call 0x51d58,
// which is a literal `blr`, so restartVideo does not resize or toggle the
// host window. Retail returns failure for every other arity.
int GM_CDECL gmf_restartVideo(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    // Retail returns -1 for invalid arities without logging an error.
    if (n != 0 && n != 3 && n != 4) return GM_EXCEPTION;
    if (n >= 3) {
        // The wrapper reads and forwards these payloads, but FUN_00051dc8/
        // FUN_00051d5c do not consume them. Both helpers copy Video mode
        // dimensions +0x8168/+0x816c into active dimensions +0x819c/+0x81a0.
        // Keep that script-visible state separate from the host window size.
        const gmint w = a_thread->Param(0).m_value.m_int;
        const gmint h = a_thread->Param(1).m_value.m_int;
        (void)w;
        (void)h;
        const gmint fullscreen = a_thread->Param(2).m_value.m_int;
        VideoState& video = GetEngineContext().video;
        video.activeDeviceWidth = video.configuredModeWidth;
        video.activeDeviceHeight = video.configuredModeHeight;
        video.isFullscreen = fullscreen != 0;
        if (n == 4) {
            const gmint bitDepth = a_thread->Param(3).m_value.m_int;
            video.requestedVideoModeBitDepth = bitDepth;
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_Video_setNormalFont(gmThread* a_thread) {
    GetEngineContext().currentFont = GetEngineContext().normalFont;
    return GM_OK;
}

int GM_CDECL gmf_Video_setFont(gmThread* a_thread) {
    // PPC verified: wrapper 0x775a4 directly reads arg 0's object payload at
    // +0x14, copies its NUL-terminated name, and installs the name-keyed
    // resource returned by the shared cache (0x1dabf0/0x1daf50). The wrapper
    // performs no argc or type-tag check. Retail's 0x42d98 constructor also
    // recognizes Russian, Chinese, Japanese, and Korean font resources. The
    // shipped Russian bitmap atlas is now loaded through its exact name; the
    // CJK resource formats and shared name-keyed cache lifetime remain open.
    GM_STRING_PARAM(name, 0, "");
    EngineContext& ctx = GetEngineContext();
    if (name == "courierFont" && ctx.courierFont) {
        ctx.currentFont = ctx.courierFont;
    } else if (name == "Helvetica_NeueLTCyrillic55_Russian" && ctx.russianFont) {
        ctx.currentFont = ctx.russianFont;
    } else if (name == "HelveticaNeueLTCom-Lt" && ctx.normalFont) {
        ctx.currentFont = ctx.normalFont;
    } else if (name == "HelveticaNeueLTCom-Hv" && ctx.heavyFont) {
        ctx.currentFont = ctx.heavyFont;
    }
    return GM_OK;
}

int GM_CDECL gmf_Video_setHeavyFont(gmThread* a_thread) {
    GetEngineContext().currentFont = GetEngineContext().heavyFont;
    return GM_OK;
}

int GM_CDECL gmf_Video_unloadTextureByHandle(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > 2) return GM_EXCEPTION;
    int area = 1;
    if (count == 2) area = a_thread->Param(1).m_value.m_int;
    a_thread->PushInt(UnloadTextureResourceByHandle(
        *a_thread->GetMachine(), a_thread->Param(0), area) ? 1 : 0);
    return GM_OK;
}

// Verification: retail VIDEO.setColourHSL is registered at 0x420c7c and
// resolves to PPC wrapper 0x76bf8. It accepts exactly 3/4 arguments, converts
// HSL into a stack temporary through 0x52210/0xc4c08, then calls 0x4ef18,
// whose complete body is `blr`; it has no persistent desktop color effect.
// The wrapper reads non-float arguments as signed integer payloads and does
// not type-check them, so only its exact arity is enforced here.
int GM_CDECL gmf_Video_setColourHSL(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 3 && count != 4) return GM_EXCEPTION;
    return GM_OK;
}

// Verification: retail VIDEO.setColourWithShade is registered at 0x420c70
// and resolves to PPC wrapper 0x768b8. It accepts 2/3 args, computes a
// temporary shaded colour through 0x51ed0/0xc4fb8, then calls 0x4ef18, whose
// body is `blr`. The wrapper performs no argument type validation.
int GM_CDECL gmf_Video_setColourWithShade(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 2 && count != 3) return GM_EXCEPTION;
    return GM_OK;
}

// Verification: retail VIDEO.setColourWithLightnessShift is registered at
// 0x420c88 and its PPC wrapper is 0x76e00. It accepts exactly 2/3 arguments;
// the last argument must be a string or COLOUR, while preceding values use
// float-or-signed-int payload conversion. Helpers 0x52288/0x523d0 create a
// temporary adjusted color and terminate at the literal `blr` 0x4ef18.
int GM_CDECL gmf_Video_setColourWithLightnessShift(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 2 && count != 3) return GM_EXCEPTION;
    const gmType colourType = a_thread->Param(count - 1).m_type;
    if (colourType != GM_STRING && colourType != g_colourType)
        return GM_EXCEPTION;
    return GM_OK;
}

// Verification: retail VIDEO.setColour at 0x76578 accepts 1-4 arguments.
// The 1/2-argument overloads require an integer ID, string, or COLOUR value;
// 3/4-argument overloads convert scalar payloads. Every valid branch ends at
// 0x4ef18 directly or through 0x51e28/0x521a4, whose endpoint is a literal
// `blr`, so this native method does not change persistent render color.
int GM_CDECL gmf_Video_setColour(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > 4) return GM_EXCEPTION;
    if (count <= 2) {
        const gmType type = a_thread->Param(0).m_type;
        if (type != GM_INT && type != GM_STRING && type != g_colourType)
            return GM_EXCEPTION;
    }
    return GM_OK;
}

gmFunctionEntry s_videoLib[] = {
    {"getTexture", gmf_Video_getTexture, nullptr},
    {"unloadTextureByHandle", gmf_Video_unloadTextureByHandle, nullptr},
    {"setNormalFont", gmf_Video_setNormalFont, nullptr},
    {"setHeavyFont", gmf_Video_setHeavyFont, nullptr},

    {"getDeviceWidth", gmf_getDeviceWidth, nullptr},
    {"getDeviceHeight", gmf_getDeviceHeight, nullptr},
    {"getScreenViewScale", gmf_getScreenViewScale, nullptr},
    {"setScreenViewScale", gmf_setScreenViewScale, nullptr},
    {"isNeedSetScreenViewScale", gmf_isNeedSetScreenViewScale, nullptr},
    {"getScreenBithDepth", gmf_getScreenBithDepth, nullptr},
    {"isFullScreen", gmf_isFullScreen, nullptr},
    {"setIsDrawingPhysics", gmf_setIsDrawingPhysics, nullptr},
    {"isDrawingPhysics", gmf_isDrawingPhysics, nullptr},
    {"getAspectRatio", gmf_getAspectRatio, nullptr},
    {"setAspectRatio", gmf_setAspectRatio, nullptr},
    {"getCameraPosX", gmf_getCameraPosX, nullptr},
    {"getCameraPosY", gmf_getCameraPosY, nullptr},
    {"getCameraPosZ", gmf_getCameraPosZ, nullptr},
    {"getCameraRotationAngle", gmf_getCameraRotationAngle, nullptr},
    {"getCameraRotationX", gmf_getCameraRotationX, nullptr},
    {"getCameraRotationY", gmf_getCameraRotationY, nullptr},
    {"getCameraRotationZ", gmf_getCameraRotationZ, nullptr},
    {"setCameraPos", gmf_setCameraPos, nullptr},
    {"setCameraRotation", gmf_setCameraRotation, nullptr},
    {"setGrayscalePerc", gmf_setGrayscalePerc, nullptr},
    {"getGrayscalePerc", gmf_getGrayscalePerc, nullptr},
    {"createPalette", gmf_createPalette, nullptr},
    {"setColourWithShade", gmf_Video_setColourWithShade, nullptr},
    {"setColourWithLightnessShift", gmf_Video_setColourWithLightnessShift, nullptr},
    {"setColourHSL", gmf_Video_setColourHSL, nullptr},
    {"clearDepthBuffer", gmf_clearDepthBuffer, nullptr},
    {"clearColourBuffer", gmf_clearColourBuffer, nullptr},
    {"isShowingFPS", gmf_isShowingFPS, nullptr},
    {"setIsShowingFPS", gmf_setIsShowingFPS, nullptr},
    {"getFPS", gmf_getFPS, nullptr},
    {"getFrameTime", gmf_getFrameTime, nullptr},
    {"traceFPSInfoNextFrame", gmf_traceFPSInfoNextFrame, nullptr},
    {"setLineWidth", gmf_setLineWidth, nullptr},
    {"setFont", gmf_Video_setFont, nullptr},
    {"setTextShadowSize", gmf_setTextShadowSize, nullptr},
    {"getTextShadowSize", gmf_getTextShadowSize, nullptr},
    {"getTextWidth", gmf_getTextWidth, nullptr},
    {"loadTexture", gmf_loadTexture, nullptr},
    {"loadTextureFromSaveRequest", gmf_loadTextureFromSaveRequest, nullptr},
    {"unloadTexture", gmf_unloadTexture, nullptr},
    {"artTestUnloadTexturesFromFile", gmf_artTestUnloadTexturesFromFile, nullptr},
    {"unloadAllTextures", gmf_unloadAllTextures, nullptr},
    {"restartVideo", gmf_restartVideo, nullptr},
    {"debug_setWorldMarker", gmf_debugSetWorldMarker, nullptr}, // records payload; retail marker draw consumer remains open
    {"debug_setWorldLineMarker", gmf_debugSetWorldLineMarker, nullptr},
    {"debug_clearWorldMarkers", gmf_debugClearWorldMarkers, nullptr},
    {"resetViewport", gmf_resetViewport, nullptr},
    {"isPointInView", gmf_isPointInView, nullptr},
    {"createDrawBuffer", gmf_Video_createDrawBuffer, nullptr},
    {"destroyDrawBuffer", gmf_Video_destroyDrawBuffer, nullptr},
    {"getNumDrawBuffers", gmf_Video_getNumDrawBuffers, nullptr},
    {"useRenderBufferInShader", gmf_useRenderBufferInShader, nullptr},
    {"deactivateShaders", gmf_deactivateShaders, nullptr},
    {"destroyAllShaders", gmf_destroyAllShaders, nullptr},
    {"createRenderBufferTexture", gmf_createRenderBufferTexture, nullptr},
    {"destroyRenderBufferTexture", gmf_destroyRenderBufferTexture, nullptr},
    {"destroyAllRenderBufferTextures", gmf_destroyAllRenderBufferTextures, nullptr},
    {"setRenderBufferMode", gmf_setRenderBufferMode, nullptr},
    {"beginRenderBufferTexture", gmf_beginRenderBufferTexture, nullptr},
    {"endRenderBufferTexture", gmf_endRenderBufferTexture, nullptr},
    {"clearStencilBuffer", gmf_clearStencilBuffer, nullptr},
    {"setScissorBox", gmf_setScissorBox, nullptr},
    {"enableScissorBox", gmf_enableScissorBox, nullptr},
    {"disableScissorBox", gmf_disableScissorBox, nullptr},
    {"getRenderBufferMode", gmf_getRenderBufferMode, nullptr},
    {"getRenderBufferModeTextureID", gmf_getRenderBufferModeTextureID, nullptr},
    {"createDrawList", gmf_createDrawList, nullptr},
    {"createDynamicDrawList", gmf_createDynamicDrawList, nullptr},
    {"appendToPostProcessDList", gmf_appendToPostProcessDList, nullptr},
    {"getPostProcessTexture", gmf_getPostProcessTexture, nullptr},
    {"setTexturePaletteColour", gmf_setTexturePaletteColour, nullptr},
    {"takeScreenShot", gmf_takeScreenShot, nullptr},
    {"setIsShouldDrawEntities", gmf_setIsShouldDrawEntities, nullptr},
    {"isShouldDrawEntities", gmf_isShouldDrawEntities, nullptr},
    {"addToDrawMap", gmf_addToDrawMap, nullptr},
    {"removeFromDrawMap", gmf_removeFromDrawMap, nullptr},
    {"getTextureExtX", gmf_getTextureExtX, nullptr},
    {"getTextureExtY", gmf_getTextureExtY, nullptr},
    {"getDefaultPostProcessShader", gmf_getDefaultPostProcessShader, nullptr},
    {"flushStaticSurface", gmf_flushStaticSurface, nullptr},
    {"getTextureMemFree", gmf_getTextureMemFree, nullptr},
    {"purgeUnusedTexturesDelayed", gmf_purgeUnusedTexturesDelayed, nullptr},
    {"purgeUnusedTextures", gmf_purgeUnusedTextures, nullptr},
    // `Video.clear` is a real method-table entry. Its receiver/argument paths
    // are still being reconstructed from PPC; keep the native name visible
    // and strict-report calls instead of aliasing DrawList.clear.
    {"clear", gmf_Video_clear, nullptr},
    {"getScreenWidth", gmf_getScreenWidth, nullptr},
    {"getScreenHeight", gmf_getScreenHeight, nullptr},

    // These VIDEO library entries have distinct retail bodies from VMSHAPE
    // methods: beginShape returns a new VMSHAPE, plotVertex's helpers are
    // retail no-ops, and endShape is a literal no-op.
    {"beginShape", gmf_Video_beginShape, nullptr},
    {"plotVertex", gmf_Video_plotVertex, nullptr},
    {"endShape", gmf_Video_endShape, nullptr},
    {"setColour", gmf_Video_setColour, nullptr},

    {"setInterpolatedColour", gmf_setInterpolatedColour, nullptr},
};

// --- FONTHELPER_* stand-ins ---
//
// The real FONTHELPER_SetNormalLargeFont is a plain global .gm function
// (confirmed, see game_data/FontHelper.gm):
//   global FONTHELPER_SetNormalLargeFont = function(drawList) {
//       Video.setNormalFont();
//       drawList.setFontRenderAttributes(0.1,0.9);
//   };
// FontHelper.gm now compiles and loads for real (see main.cpp), so this
// stand-in only remains registered as a fallback in case that load ever
// fails in some environment.
int GM_CDECL gmf_FONTHELPER_SetNormalLargeFont(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmUserObject* drawListObj = a_thread->ParamUserObject(0);
    GetEngineContext().currentFont = GetEngineContext().normalFont;
    if (!SetDrawListFontRenderAttributes(drawListObj, 0.1f, 0.9f)) {
        GM_EXCEPTION_MSG("FONTHELPER_SetNormalLargeFont: param 0 is not a drawList");
        return GM_EXCEPTION;
    }
    return GM_OK;
}


// ENGINE PATCH (2026-08-16): default no-op notifyEvent -- confirmed real
// necessary gap via SystemHelper.gm's SYSTEMHELPER_NotifyEvent (the ONLY
// call site Player.gm's checkDeath() uses to fire "KillPlayer" when
// touching a DANGER/FIRE-styled ground): `for (i=0;i<GameSystem.
// getProcessCount();i+=1) { GameSystem.getProcessByIndex(i).notifyEvent(
// eventName,a,b,c,d,e,f,g); }` -- an unconditional call on EVERY live
// process, but only 5 of the ~80+ real process .gm files in this corpus
// (Main.gm, StatsProcess.gm, EntityMenuTopBase.gm, OneTrailPlayMode.gm,
// ScaleOfTheWorld.gm) actually define their own notifyEvent. Calling a
// table method that doesn't exist is a hard "getdot failed" in this GM
// implementation (unlike calling an undefined GLOBAL function, which
// null-call-leniently returns null) -- so the very first live process
// WITHOUT its own notifyEvent (near-certain given how few define one)
// aborts SystemHelper's loop immediately, and "KillPlayer" never reaches
// Main.gm's real handler at all: touching a hazard silently did nothing,
// with no error surfaced anywhere near the actual touch. Injected here,
// same spot/pattern as every other InjectProcessBaseMethods method, but
// ONLY if the process's own factory function didn't already set one --
// real per-process handlers (Main.gm's own real event switch, including
// "KillPlayer" -> `player.die(killDir)`) must keep taking priority.
int GM_CDECL gmf_defaultNotifyEvent(gmThread*) { return GM_OK; }

} // namespace

std::optional<std::string> ResolvePlatformResourcePath(const char* a_path, int a_area) {
    return ResolvePlatformResourcePathImpl(a_path, a_area);
}

std::optional<std::string> ResolveTextureAssetPath(const char* a_path, int a_area) {
    return ResolvePlatformResourcePathImpl(a_path, a_area);
}

// Retail Video update consumer at EBOOT 0x51c44 skips negative values,
// decrements nonnegative values, and invokes the purge helper only after the
// decrement becomes negative. The desktop calls this once per render frame;
// retail's exact owner/call cadence is still under audit.
void UpdateVideoTextureMaintenance() {
    VideoState& video = GetEngineContext().video;
    if (video.delayedTexturePurgeFrames < 0) return;
    --video.delayedTexturePurgeFrames;
    if (video.delayedTexturePurgeFrames < 0) {
        video.delayedTexturePurgeFrames = -1;
        PurgeUnusedManagedTextureResources();
    }
}

void InjectProcessBaseMethods(gmMachine& a_machine, gmTableObject* a_table, int a_processId) {
    InjectProcessHandleMethods(a_machine, a_table, a_processId);
    if (a_table->Get(&a_machine, "notifyEvent").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "notifyEvent", gmVariable(a_machine.AllocFunctionObject(gmf_defaultNotifyEvent)));
    }
}

void RegisterVideoBindings(gmMachine& a_machine) {
    // Retail creates badTexture_Yo as part of Video construction, before any
    // script can request a texture from savedata.
    EnsureBadTextureFallback();
    RegisterVideoDrawBufferBindings(a_machine);
    a_machine.RegisterLibrary(s_videoLib, sizeof(s_videoLib) / sizeof(s_videoLib[0]), "Video");
    RegisterDrawListType(a_machine);
    RegisterShapeBindings(a_machine);
    a_machine.RegisterLibraryFunction("FONTHELPER_SetNormalLargeFont", gmf_FONTHELPER_SetNormalLargeFont);
}
