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
#include "../core/system_config.h"
#include "../core/strict_calls.h"
#include "../render/renderer.h"
#include "../render/draw_list.h"
#include "../render/draw_context.h"
#include "../core/save_data.h"
#include <cstdio>
#include <array>
#include <filesystem>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>

namespace {

void LogTextureFallbackOnce(int a_area, const char* a_path,
                            const char* a_reason) {
    static std::unordered_set<std::string> logged;
    static std::mutex loggedMutex;
    const std::string requested = a_path ? a_path : "";
    const std::string key = std::to_string(a_area) + ":" + requested + ":" +
                            (a_reason ? a_reason : "unknown");
    std::lock_guard<std::mutex> lock(loggedMutex);
    if (logged.insert(key).second) {
        std::fprintf(stderr,
            "[texture-fallback] area=%d path='%s' reason=%s\n",
            a_area, requested.c_str(), a_reason ? a_reason : "unknown");
    }
}

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


int GM_CDECL gmf_getDeviceWidth(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.activeDeviceWidth);
    return GM_OK;
}
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
    a_thread->PushInt(GetEngineContext().video.screenBitDepth);
    return GM_OK;
}
int GM_CDECL gmf_isFullScreen(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.isFullscreen ? 1 : 0);
    return GM_OK;
}
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
int GM_CDECL gmf_getAspectRatio(gmThread* a_thread) {
    const auto& engine = GetEngineContext();
    const float aspect = !SystemConfig::Get().letterbox && engine.renderer
        ? engine.renderer->GetAspect() : engine.video.aspectRatio;
    a_thread->PushFloat(aspect);
    return GM_OK;
}
int GM_CDECL gmf_setAspectRatio(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 16.0f / 9.0f);
    GetEngineContext().video.aspectRatio = v;
    return GM_OK;
}
int GM_CDECL gmf_getCameraPosX(gmThread* a_thread) {
    a_thread->PushFloat(static_cast<float>(GetEngineContext().video.GetRetailCameraX()));
    return GM_OK;
}
int GM_CDECL gmf_getCameraPosY(gmThread* a_thread) {
    a_thread->PushFloat(static_cast<float>(GetEngineContext().video.GetRetailCameraY()));
    return GM_OK;
}
int GM_CDECL gmf_getCameraPosZ(gmThread* a_thread) {
    a_thread->PushFloat(static_cast<float>(GetEngineContext().video.GetRetailCameraZ()));
    return GM_OK;
}
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
    GetEngineContext().renderer->SetCameraPos(
        static_cast<float>(v.GetRetailCameraX()),
        static_cast<float>(v.GetRetailCameraY()),
        static_cast<float>(v.GetRetailCameraZ()));
    return GM_OK;
}
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
int GM_CDECL gmf_setGrayscalePerc(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f);
    GetEngineContext().video.grayscalePerc = VideoState::RetailGrayscaleValue(v);
    return GM_OK;
}
int GM_CDECL gmf_getGrayscalePerc(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.grayscalePerc);
    return GM_OK;
}
int GM_CDECL gmf_isShowingFPS(gmThread* a_thread) { a_thread->PushInt(GetEngineContext().video.showFPS ? 1 : 0); return GM_OK; }
int GM_CDECL gmf_setIsShowingFPS(gmThread* a_thread) {
    GetEngineContext().video.showFPS = RetailRawBoolArgument(a_thread);
    return GM_OK;
}
int GM_CDECL gmf_getFPS(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.GetRetailFPS());
    return GM_OK;
}
int GM_CDECL gmf_getFrameTime(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.frameTimeMs);
    return GM_OK;
}
int GM_CDECL gmf_traceFPSInfoNextFrame(gmThread* a_thread) {
    GetEngineContext().video.traceFPSInfo = true;
    return GM_OK;
}
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
    const float v = RetailRawFloatOrIntArgument(a_thread, 0);
    GetEngineContext().video.textShadowSize = v;
    return GM_OK;
}
int GM_CDECL gmf_getTextShadowSize(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().video.textShadowSize);
    return GM_OK;
}
int GM_CDECL gmf_getTextWidth(gmThread* a_thread) {
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
    GetEngineContext().video.shouldDrawEntities = RetailRawBoolArgument(a_thread);
    return GM_OK;
}
int GM_CDECL gmf_isShouldDrawEntities(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().video.shouldDrawEntities ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getScreenWidth(gmThread* a_thread) {
    a_thread->PushFloat(VideoState::RetailScreenExtent(
        GetEngineContext().video.numCellsX));
    return GM_OK;
}
int GM_CDECL gmf_getScreenHeight(gmThread* a_thread) {
    a_thread->PushFloat(VideoState::RetailScreenExtent(
        GetEngineContext().video.numCellsY));
    return GM_OK;
}
int GM_CDECL gmf_flushStaticSurface(gmThread* a_thread) {
    GetEngineContext().video.needsFlushStaticSurface = true;
    return GM_OK;
}
int GM_CDECL gmf_getTextureMemFree(gmThread* a_thread) {
    Renderer* renderer = GetEngineContext().renderer;
    a_thread->PushInt(renderer ? renderer->GetTextureMemoryFreeBytes() : 0);
    return GM_OK;
}
int GM_CDECL gmf_getResizeFactor(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable& texture = a_thread->Param(0);
    if (texture.m_type != g_textureType) return GM_EXCEPTION;
    a_thread->PushFloat(ResolveTextureDimensionScale(*a_thread->GetMachine(), texture));
    return GM_OK;
}

int GM_CDECL gmf_clearDepthBuffer(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_clearColourBuffer(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_resetViewport(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_destroyAllRenderBufferTextures(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_clearStencilBuffer(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_setDebugID(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_purgeUnusedTextures(gmThread* a_thread) { return GM_EXCEPTION; }
int GM_CDECL gmf_isPointInView(gmThread* a_thread) {
    (void)a_thread;
    std::fprintf(stderr, "WARNING: Video.isPointInView not longer used.\n");
    return GM_OK;
}
int GM_CDECL gmf_getPostProcessTexture(gmThread* a_thread) { a_thread->PushNull(); return GM_OK; }
int GM_CDECL gmf_createRenderBufferTexture(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }
int GM_CDECL gmf_getRenderBufferMode(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }
int GM_CDECL gmf_getRenderBufferModeTextureID(gmThread* a_thread) { a_thread->PushInt(-1); return GM_OK; }

int GM_CDECL gmf_useRenderBufferInShader(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_deactivateShaders(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

int GM_CDECL gmf_destroyAllShaders(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_destroyRenderBufferTexture(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_setRenderBufferMode(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    return (count == 1 || count == 2) ? GM_OK : GM_EXCEPTION;
}
int GM_CDECL gmf_beginRenderBufferTexture(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_endRenderBufferTexture(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_debugSetWorldMarker(gmThread* a_thread) {
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
    (void)a_thread;
    GetEngineContext().video.debugWorldMarkers.clear();
    return GM_OK;
}
int GM_CDECL gmf_setScissorBox(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_enableScissorBox(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_disableScissorBox(gmThread* a_thread) { (void)a_thread; return GM_OK; }
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

    const auto resolved = ResolveTextureAssetPath(path, 1);
    unsigned int handle = 0;
    if (resolved) {
        Renderer* renderer = GetEngineContext().renderer;
        handle = renderer->LoadTexture(*resolved, "1:" + std::string(path));
        if (handle) {
            RetainManagedTextureResource(handle, 1, path);
        } else {
            LogTextureFallbackOnce(1, path, "decode-or-upload-failed");
            // Match the failed-resource branch above: retail still applies
            // the palette update to Video's shared badTexture_Yo object.
            handle = EnsureBadTextureFallback();
        }
    } else {
        LogTextureFallbackOnce(1, path, "resource-not-found");
        handle = EnsureBadTextureFallback();
    }
    SetTexturePaletteColourByHandle(handle, index, r, g, b, alpha);
    return GM_OK;
}
GE3_REMOVED(gmf_setGeneratedPalette, "Video.setGeneratedPalette")
GE3_REMOVED(gmf_convertPaletteToAlpha, "Video.convertPaletteToAlpha")
int GM_CDECL gmf_takeScreenShot(gmThread* a_thread) {
    if (a_thread->GetNumParams() > 1 && a_thread->Param(1).m_value.m_int != 0)
        std::fprintf(stderr, "FILEHELPER[ln 457] file writing in retail is not permitted!\n");
    return GM_OK;
}
GE3_REMOVED(gmf_setTextureShaderType, "Video.setTextureShaderType")
GE3_REMOVED(gmf_setShaderBasedOnTexture, "Video.setShaderBasedOnTexture")
int GM_CDECL gmf_getDefaultPostProcessShader(gmThread* a_thread) {
    a_thread->Push(CreateDefaultPostProcessShaderHandle(*a_thread->GetMachine()));
    return GM_OK;
}
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
    const int count = a_thread->GetNumParams();
    return count <= 4 ? GM_OK : GM_EXCEPTION;
}
int GM_CDECL gmf_appendToPostProcessDList(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_artTestUnloadTexturesFromFile(gmThread* a_thread) { return GM_OK; } // debug/test tool, no-op is correct outside the art pipeline
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
    if (!std::filesystem::is_regular_file(std::filesystem::u8path(fullPath), ec) || ec) return std::nullopt;
    return fullPath;
}

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
        LogTextureFallbackOnce(area, path, "resource-not-found");
        a_thread->Push(MakeTextureHandle(*a_thread->GetMachine(),
                                         EnsureBadTextureFallback()));
        return GM_OK;
    }
    const std::string cacheKey = std::to_string(area) + ":" + path;
    unsigned int handle = ctx.renderer->LoadTexture(*resolved, cacheKey);
    if (handle == 0) {
        LogTextureFallbackOnce(area, path, "decode-or-upload-failed");
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
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > 3) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    const int area = count >= 2 ? a_thread->Param(1).m_value.m_int : 1;
    auto resolved = ResolveTextureAssetPath(path, area);
    unsigned int handle = 0;
    if (resolved) {
        const std::string cacheKey = std::to_string(area) + ":" + path;
        handle = GetEngineContext().renderer->LoadTexture(*resolved, cacheKey);
        RetainManagedTextureResource(handle, area, path);
        if (!handle) LogTextureFallbackOnce(area, path, "decode-or-upload-failed");
    } else {
        LogTextureFallbackOnce(area, path, "resource-not-found");
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

int GM_CDECL gmf_createPalette(gmThread* a_thread) {
    a_thread->Push(MakePaletteHandle(*a_thread->GetMachine()));
    return GM_OK;
}

int GM_CDECL gmf_getDimensions(gmThread* a_thread) {
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
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    float uMax = 1.0f, vMax = 1.0f;
    GetEngineContext().renderer->GetTextureUVExtent(handle, uMax, vMax);
    a_thread->PushFloat(uMax);
    return GM_OK;
}
int GM_CDECL gmf_getTextureExtY(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    float uMax = 1.0f, vMax = 1.0f;
    GetEngineContext().renderer->GetTextureUVExtent(handle, uMax, vMax);
    a_thread->PushFloat(vMax);
    return GM_OK;
}

// --- draw lists / immediate draw calls ---

int GM_CDECL gmf_createDrawList(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count > 2) return GM_EXCEPTION;
    gmint size = 0x400, align = 0x20;
    if (count >= 1) size = a_thread->Param(0).m_value.m_int;
    if (count >= 2) align = a_thread->Param(1).m_value.m_int;
    PushNewDrawList(a_thread, size, align);
    return GM_OK;
}
int GM_CDECL gmf_createDynamicDrawList(gmThread* a_thread) {
    if (a_thread->GetNumParams() == 2) {
        const gmint size = a_thread->Param(0).m_value.m_int;
        const gmint align = a_thread->Param(1).m_value.m_int;
        PushNewDrawList(a_thread, size, align);
    }
    return GM_OK;
}

int GM_CDECL gmf_drawTexture(gmThread* a_thread) { return DC_gmf_drawTexture(a_thread); }
// Same shared-vocabulary reasoning -- drawList.drawAnimation (draw_list.cpp)
// uses the identical DC_gmf_drawAnimation implementation (animation_bindings.cpp).
int GM_CDECL gmf_drawAnimation(gmThread* a_thread) { return DC_gmf_drawAnimation(a_thread); }

int GM_CDECL gmf_drawTextureByHandle(gmThread* a_thread) { return DC_gmf_drawTextureByHandle(a_thread); }

// --- restartVideo: the actual reported blocker ---

int GM_CDECL gmf_restartVideo(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    // Retail returns -1 for invalid arities without logging an error.
    if (n != 0 && n != 3 && n != 4) return GM_EXCEPTION;
    if (n >= 3) {
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


int GM_CDECL gmf_defaultNotifyEvent(gmThread*) { return GM_OK; }

} // namespace

std::optional<std::string> ResolvePlatformResourcePath(const char* a_path, int a_area) {
    return ResolvePlatformResourcePathImpl(a_path, a_area);
}

std::optional<std::string> ResolveTextureAssetPath(const char* a_path, int a_area) {
    return ResolvePlatformResourcePathImpl(a_path, a_area);
}

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
