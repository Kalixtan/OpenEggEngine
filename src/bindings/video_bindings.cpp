#include "gmThread.h"
#include "video_bindings.h"
#include "process_base_bindings.h"
#include "process_handle_bindings.h"
#include "texture_bindings.h"
#include "shape_bindings.h"
#include "animation_bindings.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "../render/draw_list.h"
#include "../render/draw_context.h"
#include <cstdio>

namespace {


int GM_CDECL gmf_getDeviceWidth(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().renderer->GetWidth());
    return GM_OK;
}
int GM_CDECL gmf_getDeviceHeight(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().renderer->GetHeight());
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
    a_thread->PushInt(32); // this renderer is always a 32bpp GL default framebuffer
    return GM_OK;
}
int GM_CDECL gmf_isFullScreen(gmThread* a_thread) {
    a_thread->PushInt(0); // windowed SDL renderer, never real fullscreen
    return GM_OK;
}
int GM_CDECL gmf_setIsDrawingPhysics(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    GetEngineContext().video.isDrawingPhysics = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_isDrawingPhysics(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.isDrawingPhysics ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getAspectRatio(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.aspectRatio);
    return GM_OK;
}
int GM_CDECL gmf_setAspectRatio(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 16.0f / 9.0f);
    GetEngineContext().video.aspectRatio = v;
    return GM_OK;
}
int GM_CDECL gmf_getCameraPosX(gmThread* a_thread) { a_thread->PushFloat(GetEngineContext().video.camPosX); return GM_OK; }
int GM_CDECL gmf_getCameraPosY(gmThread* a_thread) { a_thread->PushFloat(GetEngineContext().video.camPosY); return GM_OK; }
int GM_CDECL gmf_getCameraPosZ(gmThread* a_thread) { a_thread->PushFloat(GetEngineContext().video.camPosZ); return GM_OK; }
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
    GetEngineContext().renderer->SetCameraPos(x, y, z);
    return GM_OK;
}
int GM_CDECL gmf_setCameraRotation(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(4);
    GM_FLOAT_OR_INT_PARAM(angle, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(x, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y, 2, 0.0f); GM_FLOAT_OR_INT_PARAM(z, 3, 0.0f);
    VideoState& v = GetEngineContext().video;
    v.camRotAngle = angle; v.camRotX = x; v.camRotY = y; v.camRotZ = z;
    return GM_OK;
}
int GM_CDECL gmf_setGrayscalePerc(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    GetEngineContext().video.grayscalePerc = v;
    return GM_OK;
}
int GM_CDECL gmf_getGrayscalePerc(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.grayscalePerc);
    return GM_OK;
}
int GM_CDECL gmf_isShowingFPS(gmThread* a_thread) { a_thread->PushInt(GetEngineContext().video.showFPS ? 1 : 0); return GM_OK; }
int GM_CDECL gmf_setIsShowingFPS(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    GetEngineContext().video.showFPS = (v != 0);
    return GM_OK;
}
// Real per-frame FPS tracking isn't wired up (no rolling frame-time
// buffer yet) -- previously a fixed vsync-cap placeholder.
GE3_REMOVED(gmf_getFPS, "Video.getFPS")
int GM_CDECL gmf_getFrameTime(gmThread* a_thread) {
    a_thread->PushFloat(1000.0f / 60.0f);
    return GM_OK;
}
int GM_CDECL gmf_traceFPSInfoNextFrame(gmThread* a_thread) { return GM_OK; } // logging-only in retail, no state to model
// setLineWidth() -- confirmed real call with ZERO args (PekkoAndJon/
// Main.gm:92's init(): `Video.setLineWidth();`, no argument at all despite
// this codebase's memory previously misremembering it as `(0.3)`).
// Ghidra-confirmed (this session, full analysis + IAT xref sweep): retail's
// own GL device-init function passes the literal float 2.0 into the
// internal SetLineWidth method right after enabling GL_LINE_SMOOTH -- i.e.
// 2.0 IS the real default this zero-arg call resets back to, not 1.0.
int GM_CDECL gmf_setLineWidth(gmThread* a_thread) {
    float w = 2.0f;
    if (a_thread->GetNumParams() >= 1) w = a_thread->ParamFloatOrInt(0, 2.0f);
    GetEngineContext().video.lineWidth = w;
    // Actually apply it. This previously only stored the value and never
    // reached GL, so every setLineWidth call was silently inert. Retail's
    // device method (FUN_00421a10) scales by deviceWidth/800 before calling
    // glLineWidth -- Renderer::SetLineWidth does that.
    if (Renderer* r = GetEngineContext().renderer) r->SetLineWidth(w);
    return GM_OK;
}
int GM_CDECL gmf_setTextShadowSize(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    GetEngineContext().video.textShadowSize = v;
    return GM_OK;
}
int GM_CDECL gmf_getTextShadowSize(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.textShadowSize);
    return GM_OK;
}
int GM_CDECL gmf_getTextWidth(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_STRING_PARAM(text, 0, "");
    EngineContext& ctx = GetEngineContext();
    float w = (ctx.currentFont) ? ctx.currentFont->MeasureWidth(text, 1.0f) : 0.0f;
    a_thread->PushFloat(w);
    return GM_OK;
}
int GM_CDECL gmf_setIsShouldDrawEntities(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 1);
    GetEngineContext().video.shouldDrawEntities = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_isShouldDrawEntities(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.shouldDrawEntities ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setCurrentScreenX(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    GetEngineContext().video.currentScreenX = v;
    return GM_OK;
}
int GM_CDECL gmf_setCurrentScreenY(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    GetEngineContext().video.currentScreenY = v;
    return GM_OK;
}
int GM_CDECL gmf_getScreenWidth(gmThread* a_thread) {
    a_thread->PushFloat((float)GetEngineContext().renderer->GetWidth());
    return GM_OK;
}
int GM_CDECL gmf_getScreenHeight(gmThread* a_thread) {
    a_thread->PushFloat((float)GetEngineContext().renderer->GetHeight());
    return GM_OK;
}
// getNumCellsX/Y: real retail's screen is divided into a fixed logical
// cell grid (per the decompiled body, a device field read directly, no
// computation) -- this engine has no such grid; previously a plausible
// constant (16x9) stood in.
GE3_REMOVED(gmf_getNumCellsX, "Video.getNumCellsX")
GE3_REMOVED(gmf_getNumCellsY, "Video.getNumCellsY")
int GM_CDECL gmf_getScreenX(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.currentScreenX);
    return GM_OK;
}
int GM_CDECL gmf_getScreenY(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.currentScreenY);
    return GM_OK;
}
int GM_CDECL gmf_setAsPauseProcess(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0);
    GetEngineContext().video.isPauseProcess = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_flushStaticSurface(gmThread* a_thread) {
    GetEngineContext().video.needsFlushStaticSurface = true;
    return GM_OK;
}
// 0x0007a680), a 0-arg getter delegating to FUN_0004f23c. Decompiled that
// helper directly (Ghidra, EBOOT.elf): `return DAT_00f68398[8] -
// *DAT_00f68398;` -- real retail's libgcm/RSX texture-pool struct, index 0
// holding bytes currently used and index 8 holding the pool's total size,
// so the real formula is a plain "total - used" free-byte count. No other
// function in the reveng corpus touches DAT_00f68398 (checked directly),
// so there's no sibling accessor to port alongside it, and there's no
// equivalent fixed-size GPU memory pool in this engine at all (plain
// OpenGL textures, allocated/freed through the driver, not a hand-rolled
// RSX allocator) -- so "total"/"used" have nothing real to compute from
// here. The ONE real call site in the corpus (Platformer/Main.gm:9301) is
// a dev debug-console command ("purge textures delayed") that only traces
// this value before calling purgeUnusedTexturesDelayed(), never branches
// on it, so a byte-exact figure isn't reachable or necessary -- returns a
// generous constant in the general range of a real PS3 RSX texture pool
// (well under the ~256MB total VRAM) so that trace print sees a plausible,
// always-positive "not out of memory" figure instead of a fabricated,
// hidden-behind-a-halt placeholder.
int GM_CDECL gmf_getTextureMemFree(gmThread* a_thread) {
    a_thread->PushInt(96 * 1024 * 1024); // 96MB, plausible RSX-texture-pool-scale figure
    return GM_OK;
}
int GM_CDECL gmf_getResizeFactor(gmThread* a_thread) { a_thread->PushFloat(1.0f); return GM_OK; }
int GM_CDECL gmf_getNumVertices(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; } // no bound-shader vertex tracking

// --- confirmed literal no-ops in retail (see agent survey of Video.c) ---
int GM_CDECL gmf_clearDepthBuffer(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_clearColourBuffer(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_resetViewport(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_destroyAllRenderBufferTextures(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_clearStencilBuffer(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_setDebugID(gmThread* a_thread) { return GM_OK; }
// Confirmed real body is unconditionally `return 0xffffffffffffffff;` (the
// same "-1" convention every other confirmed function here uses for a bad
// argcount) with no branching at all -- looks like retail itself shipped
// this disabled/broken. Only real call site in the whole corpus
// (game_data/Platformer/EntityMenuTopBase.gm:77) is commented out, so this
// is never actually exercised.
int GM_CDECL gmf_purgeUnusedTextures(gmThread* a_thread) { return GM_EXCEPTION; }
int GM_CDECL gmf_isPointInView(gmThread* a_thread) {
    std::fprintf(stderr, "[trace] WARNING: Video.isPointInView not longer used.\n"); // confirmed real deprecation string
    return GM_OK;
}
int GM_CDECL gmf_getPostProcessTexture(gmThread* a_thread) { a_thread->PushNull(); return GM_OK; }
int GM_CDECL gmf_createRenderBufferTexture(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }
int GM_CDECL gmf_getRenderBufferMode(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }
int GM_CDECL gmf_getRenderBufferModeTextureID(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }

// --- deep GPU-pipeline surface this renderer doesn't have yet (render
// buffers/RTT, shaders, palettes, scissor/stencil funcs, screenshot) --
// previously honest no-ops so scripts kept running rather than crash. Not
// confirmed no-ops in retail (unlike the cluster above) -- genuinely
// deferred, so trapped rather than silently succeeding.
GE3_REMOVED(gmf_createDrawBuffer, "Video.createDrawBuffer")
GE3_REMOVED(gmf_destroyDrawBuffer, "Video.destroyDrawBuffer")
int GM_CDECL gmf_getNumDrawBuffers(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
GE3_REMOVED(gmf_useRenderBufferInShader, "Video.useRenderBufferInShader")
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
GE3_REMOVED(gmf_destroyRenderBufferTexture, "Video.destroyRenderBufferTexture")
GE3_REMOVED(gmf_setRenderBufferMode, "Video.setRenderBufferMode")
GE3_REMOVED(gmf_beginRenderBufferTexture, "Video.beginRenderBufferTexture")
GE3_REMOVED(gmf_endRenderBufferTexture, "Video.endRenderBufferTexture")
int GM_CDECL gmf_debugSetWorldMarker(gmThread* a_thread) { return GM_OK; } // no debug-overlay pipeline
int GM_CDECL gmf_debugSetWorldLineMarker(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_debugClearWorldMarkers(gmThread* a_thread) { return GM_OK; }
GE3_REMOVED(gmf_setScissorBox, "Video.setScissorBox")
GE3_REMOVED(gmf_enableScissorBox, "Video.enableScissorBox")
GE3_REMOVED(gmf_disableScissorBox, "Video.disableScissorBox")
GE3_REMOVED(gmf_setTexturePaletteColour, "Video.setTexturePaletteColour")
GE3_REMOVED(gmf_setGeneratedPalette, "Video.setGeneratedPalette")
GE3_REMOVED(gmf_convertPaletteToAlpha, "Video.convertPaletteToAlpha")
int GM_CDECL gmf_takeScreenShot(gmThread* a_thread) {
    std::fprintf(stderr, "[trace] Video.takeScreenShot: not implemented (no screenshot-to-disk pipeline), ignored\n");
    return GM_OK;
}
GE3_REMOVED(gmf_setTextureShaderType, "Video.setTextureShaderType")
GE3_REMOVED(gmf_bindEntityShader, "Video.bindEntityShader")
GE3_REMOVED(gmf_bindShader, "Video.bindShader")
GE3_REMOVED(gmf_setShaderBasedOnTexture, "Video.setShaderBasedOnTexture")
int GM_CDECL gmf_getDefaultPostProcessShader(gmThread* a_thread) { a_thread->PushNull(); return GM_OK; }
int GM_CDECL gmf_getShader(gmThread* a_thread) { a_thread->PushNull(); return GM_OK; }
GE3_REMOVED(gmf_destroyShader, "Video.destroyShader")
// Video.destroyAllShaders() -- NOT a fabricated/guessed stub: confirmed
// absent from retail. Platformer/Main.gm:1419 (genuine, unmodified retail
// script) really does call `Video.destroyAllShaders();`, but an exhaustive
// raw-byte scan of the retail EBOOT.elf (every initialized memory block,
// found the literal string "destroyAllShaders" NOWHERE in the binary --
// zero hits, vs. e.g. "setFont" which the same scan finds 4 times exactly
// where function_offsets.csv says it should be. A gmFunctionEntry can't be
// bound under a name that isn't a string in the binary, so retail itself
// never had this method registered on Video: the shipped game's own GM
// dispatch resolves this call as a null/unbound method call and no-ops,
// same as any other GM call to a nonexistent method (see OnNullCall,
// strict_calls.cpp). Matching that: a genuine no-op, not a port.
int GM_CDECL gmf_destroyAllShaders(gmThread* a_thread) { return GM_OK; }
// Video.setStencilFunc(func, [ref]) -- confirmed real (GroundProcessNEW.gm:
// `Video.setStencilFunc(VIDEOPARAMS_GREATER,this.parent.OUTLINE_STENCIL_VALUE);`
// and the 1-arg disable form `Video.setStencilFunc(VIDEOPARAMS_DISABLE);`).
// See Renderer::SetStencilFunc/drawList.setAttrStencil (draw_context.cpp)
// for the same VIDEOPARAMS_* func encoding.
int GM_CDECL gmf_setStencilFunc(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(func, 0, 8 /* VIDEOPARAMS_DISABLE */);
    int ref = 0;
    if (a_thread->GetNumParams() >= 2) { GM_INT_PARAM(r, 1, 0); ref = r; }
    GetEngineContext().renderer->SetStencilFunc(func, ref);
    return GM_OK;
}
// Video.setStencilOp(sfail, dpfail, dppass) -- confirmed real
// (GroundProcessNEW.gm: `Video.setStencilOp(VIDEOPARAMS_KEEP,
// VIDEOPARAMS_KEEP,VIDEOPARAMS_REPLACE);`).
int GM_CDECL gmf_setStencilOp(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(sfail, 0, 100); GM_INT_PARAM(dpfail, 1, 100); GM_INT_PARAM(dppass, 2, 100);
    GetEngineContext().renderer->SetStencilOp(sfail, dpfail, dppass);
    return GM_OK;
}
// ColourManager.makeInterpolatedColour is the real interpolation path we
// DO implement; this raw Video-level variant isn't -- genuinely deferred.
GE3_REMOVED(gmf_setInterpolatedColour, "Video.setInterpolatedColour")
GE3_REMOVED(gmf_refreshAllGraphicColours, "Video.refreshAllGraphicColours")
GE3_REMOVED(gmf_setGraphicSpriteAnimation, "Video.setGraphicSpriteAnimation")
int GM_CDECL gmf_addToDrawMap(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_removeFromDrawMap(gmThread* a_thread) { return GM_OK; }
GE3_REMOVED(gmf_drawModTexture, "Video.drawModTexture")
GE3_REMOVED(gmf_drawShape, "Video.drawShape")
GE3_REMOVED(gmf_setTexture, "Video.setTexture")
GE3_REMOVED(gmf_setTextureByHandle, "Video.setTextureByHandle")
GE3_REMOVED(gmf_clearTexture, "Video.clearTexture")
GE3_REMOVED(gmf_setTextureSubImg, "Video.setTextureSubImg")
int GM_CDECL gmf_appendToPostProcessDList(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_artTestUnloadTexturesFromFile(gmThread* a_thread) { return GM_OK; } // debug/test tool, no-op is correct outside the art pipeline
int GM_CDECL gmf_purgeUnusedTexturesDelayed(gmThread* a_thread) { return GM_OK; }

// --- texture management (reuses Renderer's cache/refcounting) ---

int GM_CDECL gmf_Video_getTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    if (a_thread->GetNumParams() >= 2) { GM_INT_PARAM(area, 1, 0); (void)area; } // FILEHELPER_AREA_* -- only GAMEDATA matters for now

    EngineContext& ctx = GetEngineContext();
    auto resolved = ctx.fileHelper->Resolve(path);
    if (!resolved) {
        std::fprintf(stderr, "Video.getTexture: '%s' not found\n", path);
        a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(), 0));
        return GM_OK;
    }
    unsigned int handle = ctx.renderer->LoadTexture(*resolved);
    a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(), handle));
    return GM_OK;
}

int GM_CDECL gmf_loadTexture(gmThread* a_thread) {
    // Confirmed real: 1-3 args (path,[area],[out info table]). The 3-arg
    // form's table gets width/height/bpp written into it -- confirmed real
    // and load-bearing, not just an opts bag: EntityPacks/
    // TutorialGraphicsEntity.gm's `Video.loadTexture(this.ad.filename,fa,
    // this.tli);` (the in-level tutorial-hint/"Welcome to Sound Shapes!"
    // graphics) branches its entire draw() on `this.tli.bpp==32`/`==8`,
    // and sizes itself from `this.tli.width`/`.height` -- leaving the table
    // untouched (previously just forwarding to gmf_Video_getTexture, which
    // doesn't even look at param 2) meant bpp stayed null forever, so
    // NEITHER branch matched and drawTexture() was never reached at all.
    // bpp: this loader doesn't replicate retail's paletted/8bpp texture
    // path (see Texture::LoadFromFile/stb_image) -- every loaded PNG here
    // is direct RGB(A). Hardcoded 32 rather than computed from channel
    // count since that's what the corpus's real assets for this call site
    // (TutorialGraphicsEntity's PNGs) actually are, confirmed by directly
    // inspecting the PNG headers, and it's the "plain textured draw, no
    // palette tint" branch every consumer expects for a direct-colour image.
    int rv = gmf_Video_getTexture(a_thread);
    if (rv != GM_OK) return rv;
    if (a_thread->GetNumParams() >= 3 && a_thread->Param(2).m_type == GM_TABLE) {
        gmVariable handleVar = *(a_thread->GetTop() - 1);
        unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), handleVar);
        int w = 0, h = 0;
        if (handle) GetEngineContext().renderer->GetTextureDimensions(handle, w, h);
        gmTableObject* info = (gmTableObject*)a_thread->GetMachine()->GetObject(a_thread->Param(2).m_value.m_ref);
        info->Set(a_thread->GetMachine(), "width", gmVariable((float)w));
        info->Set(a_thread->GetMachine(), "height", gmVariable((float)h));
        info->Set(a_thread->GetMachine(), "bpp", gmVariable((gmint)32));
    }
    return GM_OK;
}

int GM_CDECL gmf_loadTextureFromSaveRequest(gmThread* a_thread) {
    // No save-request/screenshot-gallery infra exists in this engine --
    // honest stub rather than a guessed implementation.
    a_thread->PushInt(0);
    return GM_OK;
}

int GM_CDECL gmf_unloadTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    GetEngineContext().renderer->UnloadTexture(handle);
    a_thread->PushInt(0);
    return GM_OK;
}

int GM_CDECL gmf_unloadAllTextures(gmThread* a_thread) { return GM_OK; } // per-texture unload already refcounted; whole-cache sweep not tracked separately

int GM_CDECL gmf_getSubTexture(gmThread* a_thread) {
    // No texture-atlas sub-rect tracking -- returns the same handle back,
    // which is correct whenever the "sub" region is really the whole
    // texture (true for every non-atlas texture, i.e. everything this
    // renderer currently loads).
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(), handle));
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
    // Confirmed real: reads dimensions of the currently-bound texture; this
    // engine has no persistent "currently bound texture" concept outside
    // an explicit draw call, so this takes the handle as an argument
    // instead (documented deviation) -- every real call site we can drive
    // (getTextureExtX/Y below) already passes a handle/object explicitly.
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    int w = 0, h = 0;
    GetEngineContext().renderer->GetTextureDimensions(handle, w, h);
    gmTableObject* table = a_thread->GetMachine()->AllocTableObject();
    table->Set(a_thread->GetMachine(), "w", gmVariable((gmint)w));
    table->Set(a_thread->GetMachine(), "h", gmVariable((gmint)h));
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_getTextureExtX(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    int w = 0, h = 0; (void)h;
    GetEngineContext().renderer->GetTextureDimensions(handle, w, h);
    a_thread->PushFloat((float)w);
    return GM_OK;
}
int GM_CDECL gmf_getTextureExtY(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    int w = 0, h = 0; (void)w;
    GetEngineContext().renderer->GetTextureDimensions(handle, w, h);
    a_thread->PushFloat((float)h);
    return GM_OK;
}

// --- draw lists / immediate draw calls ---

int GM_CDECL gmf_createDrawList(gmThread* a_thread) {
    gmint size = 0x400, align = 0x20;
    if (a_thread->GetNumParams() >= 1) size = a_thread->ParamInt(0, gmint(size));
    if (a_thread->GetNumParams() >= 2) align = a_thread->ParamInt(1, gmint(align));
    PushNewDrawList(a_thread, size, align);
    return GM_OK;
}
int GM_CDECL gmf_createDynamicDrawList(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(size, 0, 0x400);
    GM_INT_PARAM(align, 1, 0x20);
    PushNewDrawList(a_thread, size, align);
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

// Confirmed real: 0, 3, or 4 args. 3/4-arg form is (width,height,fullscreen,
// [bitDepth]) based on real usage `Video.restartVideo(1280,720,0,16)`
// (game_data/Main.gm) -- matches getScreenBithDepth/isFullScreen's existing
// fields. Actually resizes the window rather than just accepting the call.
int GM_CDECL gmf_restartVideo(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n != 0 && n != 3 && n != 4) { GM_EXCEPTION_MSG("restartVideo: expecting 0, 3 or 4 params"); return GM_EXCEPTION; }
    if (n >= 3) {
        GM_INT_PARAM(w, 0, 0);
        GM_INT_PARAM(h, 1, 0);
        GetEngineContext().renderer->Resize(w, h);
    }
    return GM_OK;
}

int GM_CDECL gmf_Video_setNormalFont(gmThread* a_thread) {
    GetEngineContext().currentFont = GetEngineContext().normalFont;
    return GM_OK;
}

int GM_CDECL gmf_Video_setHeavyFont(gmThread* a_thread) {
    GetEngineContext().currentFont = GetEngineContext().heavyFont;
    return GM_OK;
}

int GM_CDECL gmf_Video_unloadTextureByHandle(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    GetEngineContext().renderer->UnloadTexture(handle);
    return GM_OK;
}

// FoE texture scope markers -- not in the FoE PC binary's own
// gmFunctionEntry tables (string exists, no registered function pointer,
// see foe_compat_bindings.cpp's header comment), but every real call site
// (PekkoAndJon/bg.gm's `Video.beginTexture(path); Video.beginShape(...);
// ...; Video.endShape(); Video.endTexture();`) brackets a beginShape/
// plotVertex/endShape block exactly the way drawList.setTextureByHandle/
// clearTexture already bracket drawShape() calls (see DrawContext::
// currentTextureHandle's header comment) -- same underlying "bind texture
// for the next N shape draws" scope, just spelled differently. Reusing
// that existing field makes DrawShapeData's beginShape/endShape path
// texture-aware for free (shape_bindings.cpp), instead of leaving FoE's
// textured backdrops rendering as a flat colour fill.
int GM_CDECL gmf_Video_beginTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    EngineContext& ctx = GetEngineContext();
    auto resolved = ctx.fileHelper->Resolve(path);
    if (resolved) {
        GetDrawContext().currentTextureHandle = ctx.renderer->LoadTexture(*resolved);
    }
    return GM_OK;
}

int GM_CDECL gmf_Video_endTexture(gmThread* a_thread) {
    (void)a_thread;
    GetDrawContext().currentTextureHandle = 0;
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
    {"setColourWithShade", gmf_DC_setColour_video, nullptr}, // shade arg folded into existing setColour(r,g,b,[a]) shape -- see gmf_DC_setColour_video's comment (draw_context.cpp)
    {"clearDepthBuffer", gmf_clearDepthBuffer, nullptr},
    {"clearColourBuffer", gmf_clearColourBuffer, nullptr},
    {"isShowingFPS", gmf_isShowingFPS, nullptr},
    {"setIsShowingFPS", gmf_setIsShowingFPS, nullptr},
    {"getFPS", gmf_getFPS, nullptr},
    {"getFrameTime", gmf_getFrameTime, nullptr},
    {"traceFPSInfoNextFrame", gmf_traceFPSInfoNextFrame, nullptr},
    {"setLineWidth", gmf_setLineWidth, nullptr},
    {"setFont", gmf_Video_setNormalFont, nullptr}, // confirmed real but font-by-name not modelled; normal font is the safe default
    {"setTextShadowSize", gmf_setTextShadowSize, nullptr},
    {"getTextShadowSize", gmf_getTextShadowSize, nullptr},
    {"getTextWidth", gmf_getTextWidth, nullptr},
    {"loadTexture", gmf_loadTexture, nullptr},
    {"loadTextureFromSaveRequest", gmf_loadTextureFromSaveRequest, nullptr},
    {"unloadTexture", gmf_unloadTexture, nullptr},
    {"artTestUnloadTexturesFromFile", gmf_artTestUnloadTexturesFromFile, nullptr},
    {"unloadAllTextures", gmf_unloadAllTextures, nullptr},
    {"restartVideo", gmf_restartVideo, nullptr},
    {"debug_setWorldMarker", gmf_debugSetWorldMarker, nullptr}, // deferred debug overlay, see cluster comment above
    {"debug_setWorldLineMarker", gmf_debugSetWorldLineMarker, nullptr},
    {"debug_clearWorldMarkers", gmf_debugClearWorldMarkers, nullptr},
    {"resetViewport", gmf_resetViewport, nullptr},
    {"isPointInView", gmf_isPointInView, nullptr},
    {"createDrawBuffer", gmf_createDrawBuffer, nullptr},
    {"destroyDrawBuffer", gmf_destroyDrawBuffer, nullptr},
    {"getNumDrawBuffers", gmf_getNumDrawBuffers, nullptr},
    {"useRenderBufferInShader", gmf_useRenderBufferInShader, nullptr},
    {"deactivateShaders", gmf_deactivateShaders, nullptr},
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
    {"getSubTexture", gmf_getSubTexture, nullptr},
    {"setGeneratedPalette", gmf_setGeneratedPalette, nullptr},
    {"convertPaletteToAlpha", gmf_convertPaletteToAlpha, nullptr},
    {"getDimensions", gmf_getDimensions, nullptr},
    {"getResizeFactor", gmf_getResizeFactor, nullptr},
    {"setTextureShaderType", gmf_setTextureShaderType, nullptr},
    {"bindEntityShader", gmf_bindEntityShader, nullptr},
    {"bindShader", gmf_bindShader, nullptr},
    {"getNumVertices", gmf_getNumVertices, nullptr},
    {"setShaderBasedOnTexture", gmf_setShaderBasedOnTexture, nullptr},
    {"setDebugID", gmf_setDebugID, nullptr},
    {"drawModTexture", gmf_drawModTexture, nullptr},
    {"drawShape", gmf_drawShape, nullptr},
    {"drawShapeWithTexture", DC_gmf_drawShapeWithTexture, nullptr},
    {"setTexture", gmf_setTexture, nullptr},
    {"setTextureByHandle", gmf_setTextureByHandle, nullptr},
    {"clearTexture", gmf_clearTexture, nullptr},
    {"setTextureSubImg", gmf_setTextureSubImg, nullptr},
    {"drawTexture", gmf_drawTexture, nullptr},
    {"drawTextureByHandle", gmf_drawTextureByHandle, nullptr},
    {"drawAnimation", gmf_drawAnimation, nullptr},
    {"setCurrentScreenX", gmf_setCurrentScreenX, nullptr},
    {"setCurrentScreenY", gmf_setCurrentScreenY, nullptr},
    {"getScreenWidth", gmf_getScreenWidth, nullptr},
    {"getScreenHeight", gmf_getScreenHeight, nullptr},
    {"getNumCellsX", gmf_getNumCellsX, nullptr},
    {"getNumCellsY", gmf_getNumCellsY, nullptr},
    {"getScreenX", gmf_getScreenX, nullptr},
    {"getScreenY", gmf_getScreenY, nullptr},
    {"getShader", gmf_getShader, nullptr},
    {"setAsPauseProcess", gmf_setAsPauseProcess, nullptr},

    // Shared drawList vocabulary confirmed also callable on Video directly
    // Video.beginShape, VideoHelper.gm) -- same handlers as the drawList
    // type, see draw_context.h/shape_bindings.h.
    {"beginShape", gmf_Shape_beginShape, nullptr},
    {"plotVertex", gmf_Shape_plotVertexCurrent, nullptr},
    {"endShape", gmf_Shape_endShapeCurrent, nullptr},
    {"setColour", gmf_DC_setColour_video, nullptr},
    {"print", DC_gmf_print, nullptr},
    {"printUntranslated", DC_gmf_printUntranslated, nullptr},
    {"printArray", DC_gmf_printArray, nullptr},
    {"pushAttributes", DC_gmf_pushAttributes, nullptr},
    {"popAttributes", DC_gmf_popAttributes, nullptr},
    {"setAttrDepthTest", DC_gmf_setAttrDepthTest, nullptr},
    {"setAttrDepthWrite", DC_gmf_setAttrDepthWrite, nullptr},
    {"setupWorldView", DC_gmf_setupWorldView, nullptr},
    {"setupOrthoView", DC_gmf_setupOrthoView, nullptr},
    {"setupWorldOrthoView", DC_gmf_setupWorldOrthoView, nullptr},
    {"setupAspectRatioOrthoView", DC_gmf_setupAspectRatioOrthoView, nullptr},
    {"pushMatrix", gmf_DC_pushMatrix, nullptr},
    {"popMatrix", gmf_DC_popMatrix, nullptr},
    {"translate", gmf_DC_translate, nullptr},
    {"rotate", gmf_DC_rotate, nullptr},
    {"rotateZ", gmf_DC_rotate, nullptr},
    {"scale", gmf_DC_scale, nullptr},
    {"clear", DC_gmf_clear, nullptr},
    {"setFontRenderAttributes", DC_gmf_setFontRenderAttributes, nullptr},
    {"resetFontRenderAttributes", DC_gmf_resetFontRenderAttributes, nullptr},

    {"destroyShader", gmf_destroyShader, nullptr},
    {"destroyAllShaders", gmf_destroyAllShaders, nullptr},
    {"setStencilFunc", gmf_setStencilFunc, nullptr},
    {"setStencilOp", gmf_setStencilOp, nullptr},
    {"setInterpolatedColour", gmf_setInterpolatedColour, nullptr},
    {"refreshAllGraphicColours", gmf_refreshAllGraphicColours, nullptr},
    {"setGraphicSpriteAnimation", gmf_setGraphicSpriteAnimation, nullptr},

    // FoE texture scope markers (stubs)
    {"beginTexture", gmf_Video_beginTexture, nullptr},
    {"endTexture", gmf_Video_endTexture, nullptr},
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

void InjectProcessBaseMethods(gmMachine& a_machine, gmTableObject* a_table, int a_processId) {
    InjectProcessHandleMethods(a_machine, a_table, a_processId);
    if (a_table->Get(&a_machine, "notifyEvent").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "notifyEvent", gmVariable(a_machine.AllocFunctionObject(gmf_defaultNotifyEvent)));
    }
}

void RegisterVideoBindings(gmMachine& a_machine) {
    a_machine.RegisterLibrary(s_videoLib, sizeof(s_videoLib) / sizeof(s_videoLib[0]), "Video");
    RegisterDrawListType(a_machine);
    RegisterShapeBindings(a_machine);
    a_machine.RegisterLibraryFunction("FONTHELPER_SetNormalLargeFont", gmf_FONTHELPER_SetNormalLargeFont);
}
