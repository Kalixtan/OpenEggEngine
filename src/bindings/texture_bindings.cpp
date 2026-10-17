#include "gmThread.h"
#include "texture_bindings.h"
#include "vector2d_bindings.h"
#include "colour_bindings.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <unordered_map>

gmType g_textureType = -1;

namespace {

struct TextureHandleData {
    unsigned int handle = 0;
    std::shared_ptr<PaletteData> palette;
    bool isSubTextureChild = false;
    std::shared_ptr<void> resourceLease;
    // Retail TEXTURE +0x3c is returned by getResizeFactor and multiplies the
    // decoded pixel dimensions. Native no-override initialization is 1.0;
    // Video's live PS3_ resource-existence override is selected per load.
    float resizeFactor = 1.0f;
    int nativeWidth = 0;
    int nativeHeight = 0;
};

struct TextureResourceLease {
    Renderer* renderer = nullptr;
    unsigned int handle = 0;
    int area = 1;
    std::string path;

    ~TextureResourceLease() {
        if (renderer && handle != 0) renderer->UnloadTexture(handle);
    }
};

std::unordered_map<std::string, std::weak_ptr<TextureResourceLease>>& TextureResources() {
    static std::unordered_map<std::string, std::weak_ptr<TextureResourceLease>> resources;
    return resources;
}

std::vector<std::weak_ptr<TextureResourceLease>>& AllTextureResources() {
    static std::vector<std::weak_ptr<TextureResourceLease>> resources;
    return resources;
}

// Video.loadTexture retains a manager-owned resource without returning a
// script TEXTURE object. Keep that reference until a matching unload call.
std::unordered_map<std::string, std::shared_ptr<TextureResourceLease>>& ManagedTextureResources() {
    static std::unordered_map<std::string, std::shared_ptr<TextureResourceLease>> resources;
    return resources;
}

std::string TextureResourceKey(int area, const std::string& path) {
    return std::to_string(area) + "\n" + path;
}

std::shared_ptr<TextureResourceLease> TextureLeaseFor(gmMachine& machine,
                                                       const gmVariable& value) {
    if (value.m_type != g_textureType) return nullptr;
    gmUserObject* object = (gmUserObject*)machine.GetObject(value.m_value.m_ref);
    if (!object || object->m_userType != g_textureType) return nullptr;
    TextureHandleData* data = (TextureHandleData*)object->m_user;
    return std::static_pointer_cast<TextureResourceLease>(data->resourceLease);
}

std::shared_ptr<PaletteData> TexturePalette(TextureHandleData* texture) {
    if (!texture) return nullptr;
    if (texture->isSubTextureChild || texture->handle == 0) return texture->palette;
    return GetEngineContext().renderer->GetTextureSubPalette(texture->handle);
}

void MarkParentPaletteDirty(TextureHandleData* texture, int a_mode = -1) {
    // A getSubTexture() handle is the native +0x4c child palette, but it
    // still carries the owning image handle. Retail palette methods recurse
    // to the owner and mark both objects dirty; do not drop that propagation
    // just because the script reached the palette through the child handle.
    if (!texture || texture->handle == 0) return;
    Renderer* renderer = GetEngineContext().renderer;
    renderer->MarkTexturePaletteDirty(texture->handle);
    if (a_mode >= 0 && !texture->isSubTextureChild)
        renderer->SetTextureShaderType(texture->handle, a_mode);
}

TextureHandleData* ThisTexture(gmThread* a_thread) {
    return (TextureHandleData*)a_thread->ThisUserCheckType(g_textureType);
}

int GM_CDECL gmf_getDimensions(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread);
    if (!t) return GM_EXCEPTION;
    if (t->handle == 0 && !t->palette) {
        a_thread->PushNull();
        return GM_OK;
    }
    int w = 0, h = 0;
    if (t->handle != 0)
        GetEngineContext().renderer->GetTextureDimensions(t->handle, w, h);
    else {
        w = t->nativeWidth;
        h = t->nativeHeight;
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(),
        RetailScaledTextureDimension(static_cast<uint32_t>(w), t->resizeFactor),
        RetailScaledTextureDimension(static_cast<uint32_t>(h), t->resizeFactor)));
    return GM_OK;
}

int GM_CDECL gmf_isValid(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    int w = 0, h = 0;
    GetEngineContext().renderer->GetTextureDimensions(t->handle, w, h);
    a_thread->PushInt((t->handle != 0 && (w > 0 || h > 0)) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_getTextureExt(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    float u, v;
    GetEngineContext().renderer->GetTextureUVExtent(t->handle, u, v);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), u, v));
    return GM_OK;
}

// Retail getResizeFactor (Video.c 0x7ae68) returns texture field +0x3c.
// Retail's no-override constructor path stores 1.0; Video.getTexture selects
// the per-resource PS3_ override before constructing the desktop handle.
int GM_CDECL gmf_getResizeFactor(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    a_thread->PushFloat(t->resizeFactor);
    return GM_OK;
}

// Video.getSubTexture returns the native +0x4c child. Indexed loose PNGs
// synthesize that child as shared palette storage, retaining the parent GL
// handle for dimensions/resource identity.
int GM_CDECL gmf_getSubTexture(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    if (t->isSubTextureChild || t->handle == 0) {
        a_thread->PushNull();
        return GM_OK;
    }
    std::shared_ptr<PaletteData> palette =
        GetEngineContext().renderer->GetTextureSubPalette(t->handle);
    if (!palette) { a_thread->PushNull(); return GM_OK; }
    TextureHandleData* child = new TextureHandleData{t->handle, palette, true,
                                                      t->resourceLease, t->resizeFactor};
    gmUserObject* obj = a_thread->GetMachine()->AllocUserObject(child, g_textureType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}

// Retail hasSubTexture tests the +0x4c pointer directly.
int GM_CDECL gmf_hasSubTexture(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    const bool hasChild = !t->isSubTextureChild && t->handle != 0 &&
        GetEngineContext().renderer->GetTextureSubPalette(t->handle) != nullptr;
    a_thread->PushInt(hasChild ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_setTextureShaderType(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread);
    if (!t || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const int selector = a_thread->Param(0).m_value.m_int;
    if (selector >= 10) return GM_EXCEPTION;
    GetEngineContext().renderer->SetTextureShaderType(t->handle, selector);
    return GM_OK;
}

float TextureFloatParam(gmThread* a_thread, int index) {
    const gmVariable& value = a_thread->Param(index);
    return value.m_type == GM_FLOAT
        ? value.m_value.m_float
        : (float)value.m_value.m_int;
}

int GM_CDECL gmf_setPaletteColour(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread);
    if (!t || a_thread->GetNumParams() < 5) return GM_EXCEPTION;
    std::shared_ptr<PaletteData> palette = TexturePalette(t);
    if (!palette) {
        MarkParentPaletteDirty(t);
        return GM_OK;
    }
    const unsigned int index = (unsigned int)a_thread->Param(0).m_value.m_int;
    auto byteValue = [](float value) -> uint8_t {
        const int truncated = (int)std::trunc(value * 255.0f);
        return (uint8_t)(truncated & 0xff);
    };
    if (index < palette->entryCount && index < 256) {
        const size_t offset = (size_t)index * 4;
        palette->rgba[offset + 3] = byteValue(TextureFloatParam(a_thread, 1));
        palette->rgba[offset + 2] = byteValue(TextureFloatParam(a_thread, 2));
        palette->rgba[offset + 1] = byteValue(TextureFloatParam(a_thread, 3));
        palette->rgba[offset + 0] = byteValue(TextureFloatParam(a_thread, 4));
    }
    palette->dirty = true;
    MarkParentPaletteDirty(t);
    if ((index == 1 || index == 6) && index < palette->entryCount) {
        const size_t tintIndex = (size_t)index * 4;
        float r = palette->rgba[tintIndex + 3] / 255.0f;
        float g = palette->rgba[tintIndex + 2] / 255.0f;
        float b = palette->rgba[tintIndex + 1] / 255.0f;
        if (index == 1) {
            palette->fillR = r; palette->fillG = g; palette->fillB = b;
        } else {
            palette->outlineR = r; palette->outlineG = g; palette->outlineB = b;
        }
        palette->hasPendingTint = true;
    }
    return GM_OK;
}

int GM_CDECL gmf_convertPaletteToAlpha(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread);
    if (!t || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    std::shared_ptr<PaletteData> palette = TexturePalette(t);
    const uint8_t alpha = (uint8_t)(a_thread->Param(0).m_value.m_int & 0xff);
    if (palette && palette->rgba.size() >= 4 && palette->rgba[0] == alpha && palette->rgba[3] == 0) {
        // The child returns without stores, while the recursive parent still
        // stores mode 4 and dirty=1 after that call completes.
        MarkParentPaletteDirty(t, 4);
        return GM_OK;
    }
    if (palette) {
        for (size_t i = 0; i < palette->entryCount * 4 && i + 3 < palette->rgba.size(); i += 4) {
            const uint8_t oldFirst = palette->rgba[i];
            palette->rgba[i + 0] = alpha;
            palette->rgba[i + 1] = alpha;
            palette->rgba[i + 2] = alpha;
            palette->rgba[i + 3] = oldFirst;
        }
        palette->mode = 4;
        palette->dirty = true;
    }
    MarkParentPaletteDirty(t, 4);
    return GM_OK;
}

uint8_t PaletteColourByte(float channel) {
    return (uint8_t)(((int)std::trunc(channel * 255.0f)) & 0xff);
}

void StoreGeneratedPalette(PaletteData& palette, uint32_t c1, uint32_t c2) {
    std::fill_n(palette.rgba.begin(), 4, 0);
    auto put = [&](int index, uint32_t bgr, uint8_t alpha) {
        const size_t p = (size_t)index * 4;
        palette.rgba[p + 0] = alpha;
        palette.rgba[p + 1] = (uint8_t)(bgr >> 16);
        palette.rgba[p + 2] = (uint8_t)(bgr >> 8);
        palette.rgba[p + 3] = (uint8_t)bgr;
    };
    float opacity = 1.0f;
    const float opacityStep = 1.0f / 6.0f;
    for (int i = 0; i < 5; ++i) {
        const uint8_t alpha = (uint8_t)(int)(opacity * 255.0f);
        put(1 + i, c1, alpha);
        put(6 + i, c2, alpha);
        opacity -= opacityStep;
    }
    float t = opacityStep;
    const float byteScale = 1.0f / 255.0f;
    for (int i = 0; i < 5; ++i) {
        uint32_t mixed = 0;
        for (int shift : {0, 8, 16}) {
            const int a = (c1 >> shift) & 0xff;
            const int b = (c2 >> shift) & 0xff;
            const float aScaled = (float)a * byteScale;
            const float delta = std::fma((float)b, byteScale, -aScaled);
            const float interpolated = std::fma(delta, t, aScaled);
            const float rounded = std::fma(interpolated, 255.0f, 0.5f);
            const int channel = (int)std::trunc(rounded);
            mixed |= (uint32_t)(channel & 0xff) << shift;
        }
        put(11 + i, mixed, 255);
        t += opacityStep;
    }
    palette.dirty = true;
}

int GM_CDECL gmf_setGeneratedPalette(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread);
    if (!t || a_thread->GetNumParams() < 2) return GM_EXCEPTION;
    float r1, g1, b1, r2, g2, b2;
    uint32_t c1, c2;
    if (a_thread->Param(0).m_type == GM_INT) {
        c1 = (uint32_t)a_thread->Param(0).m_value.m_int;
        c2 = (uint32_t)a_thread->Param(1).m_value.m_int;
        r1 = (c1 & 0xff) / 255.0f; g1 = ((c1 >> 8) & 0xff) / 255.0f; b1 = ((c1 >> 16) & 0xff) / 255.0f;
        r2 = (c2 & 0xff) / 255.0f; g2 = ((c2 >> 8) & 0xff) / 255.0f; b2 = ((c2 >> 16) & 0xff) / 255.0f;
    } else {
        if (a_thread->Param(0).m_type != g_colourType ||
            a_thread->Param(1).m_type != g_colourType) return GM_EXCEPTION;
        if (!ResolveColourRGB(a_thread, a_thread->Param(0), r1, g1, b1) ||
            !ResolveColourRGB(a_thread, a_thread->Param(1), r2, g2, b2)) return GM_EXCEPTION;
        c1 = ((uint32_t)PaletteColourByte(b1) << 16) |
             ((uint32_t)PaletteColourByte(g1) << 8) | PaletteColourByte(r1);
        c2 = ((uint32_t)PaletteColourByte(b2) << 16) |
             ((uint32_t)PaletteColourByte(g2) << 8) | PaletteColourByte(r2);
    }
    std::shared_ptr<PaletteData> palette = TexturePalette(t);
    if (palette) {
        StoreGeneratedPalette(*palette, c1, c2);
        palette->hasPendingTint = true;
        palette->fillR = r1; palette->fillG = g1; palette->fillB = b1;
        palette->outlineR = r2; palette->outlineG = g2; palette->outlineB = b2;
        MarkParentPaletteDirty(t);
    } else if (t->handle != 0) {
        GetEngineContext().renderer->SetTextureGeneratedTint(t->handle, r1, g1, b1, r2, g2, b2);
        MarkParentPaletteDirty(t);
    }
    return GM_OK;
}

gmFunctionEntry s_textureLib[] = {
    {"hasSubTexture", gmf_hasSubTexture, nullptr},
    {"getSubTexture", gmf_getSubTexture, nullptr},
    {"convertPaletteToAlpha", gmf_convertPaletteToAlpha, nullptr},
    {"setGeneratedPalette", gmf_setGeneratedPalette, nullptr},
    {"isValid", gmf_isValid, nullptr},
    {"getDimensions", gmf_getDimensions, nullptr},
    {"getTextureExt", gmf_getTextureExt, nullptr},
    {"setPaletteColour", gmf_setPaletteColour, nullptr},
    {"getResizeFactor", gmf_getResizeFactor, nullptr},
    {"setTextureShaderType", gmf_setTextureShaderType, nullptr},
};

void GM_CDECL TextureGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    TextureHandleData* data = (TextureHandleData*)a_object->m_user;
    delete data;
}
void GM_CDECL TextureAsString(gmUserObject* a_object, char* a_buffer, int a_bufferLen) {
    TextureHandleData* t = (TextureHandleData*)a_object->m_user;
    std::snprintf(a_buffer, a_bufferLen, "TEXTURE(%u)", t->handle);
}

} // namespace

float RetailScaledTextureDimension(uint32_t a_nativeDimension, float a_factor) {
    const float scaled = static_cast<float>(a_nativeDimension) * a_factor;
    constexpr double kInt64Limit = 9223372036854775808.0;
    uint32_t lowBits = 0;
    if (std::isfinite(scaled) && static_cast<double>(scaled) >= -kInt64Limit &&
        static_cast<double>(scaled) < kInt64Limit) {
        const int64_t truncated = static_cast<int64_t>(std::trunc(scaled));
        lowBits = static_cast<uint32_t>(static_cast<uint64_t>(truncated));
    }
    return static_cast<float>(lowBits);
}

void SetGeneratedPaletteColours(PaletteData& a_palette,
                                float a_fillR, float a_fillG, float a_fillB,
                                float a_outlineR, float a_outlineG, float a_outlineB) {
    const uint32_t fill = ((uint32_t)PaletteColourByte(a_fillB) << 16) |
                          ((uint32_t)PaletteColourByte(a_fillG) << 8) |
                           (uint32_t)PaletteColourByte(a_fillR);
    const uint32_t outline = ((uint32_t)PaletteColourByte(a_outlineB) << 16) |
                             ((uint32_t)PaletteColourByte(a_outlineG) << 8) |
                              (uint32_t)PaletteColourByte(a_outlineR);
    StoreGeneratedPalette(a_palette, fill, outline);
    a_palette.hasPendingTint = true;
    a_palette.fillR = a_fillR; a_palette.fillG = a_fillG; a_palette.fillB = a_fillB;
    a_palette.outlineR = a_outlineR; a_palette.outlineG = a_outlineG; a_palette.outlineB = a_outlineB;
}

void SetTexturePaletteColourByHandle(unsigned int a_handle, int a_index,
                                     uint8_t a_r, uint8_t a_g,
                                     uint8_t a_b, uint8_t a_a) {
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer || a_handle == 0) return;
    std::shared_ptr<PaletteData> palette = renderer->GetTextureSubPalette(a_handle);
    if (palette && a_index >= 0 &&
        static_cast<size_t>(a_index) < palette->entryCount && a_index < 256) {
        const size_t offset = static_cast<size_t>(a_index) * 4;
        palette->rgba[offset + 3] = a_r;
        palette->rgba[offset + 2] = a_g;
        palette->rgba[offset + 1] = a_b;
        palette->rgba[offset + 0] = a_a;
        palette->dirty = true;
        if (a_index == 1 || a_index == 6) {
            const float r = a_r / 255.0f;
            const float g = a_g / 255.0f;
            const float b = a_b / 255.0f;
            if (a_index == 1) {
                palette->fillR = r; palette->fillG = g; palette->fillB = b;
            } else {
                palette->outlineR = r; palette->outlineG = g; palette->outlineB = b;
            }
            palette->hasPendingTint = true;
        }
    }
    renderer->MarkTexturePaletteDirty(a_handle);
}

gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle) {
    TextureHandleData* data = new TextureHandleData{a_handle};
    gmUserObject* obj = a_machine.AllocUserObject(data, g_textureType);
    return gmVariable(obj);
}

gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle,
                              std::shared_ptr<void> a_resourceLease) {
    return MakeTextureHandle(a_machine, a_handle, std::move(a_resourceLease), 1.0f);
}

gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle,
                              std::shared_ptr<void> a_resourceLease,
                              float a_resizeFactor) {
    TextureHandleData* data = new TextureHandleData{a_handle, nullptr, false,
                                                     std::move(a_resourceLease),
                                                     a_resizeFactor};
    gmUserObject* obj = a_machine.AllocUserObject(data, g_textureType);
    return gmVariable(obj);
}

std::shared_ptr<void> MakeTextureResourceLease(unsigned int a_handle, int a_area,
                                                const std::string& a_path) {
    if (a_handle == 0) return nullptr;
    const std::string key = TextureResourceKey(a_area, a_path);
    auto& resources = TextureResources();
    auto found = resources.find(key);
    if (found != resources.end()) {
        if (auto existing = found->second.lock()) {
            if (existing->handle == a_handle) return existing;
        }
    }

    auto lease = std::make_shared<TextureResourceLease>();
    lease->renderer = GetEngineContext().renderer;
    lease->handle = a_handle;
    lease->area = a_area;
    lease->path = a_path;
    resources[key] = lease;
    AllTextureResources().push_back(lease);
    return lease;
}

void RetainManagedTextureResource(unsigned int a_handle, int a_area,
                                  const std::string& a_path) {
    if (a_handle == 0) return;
    const std::string key = TextureResourceKey(a_area, a_path);
    auto lease = std::static_pointer_cast<TextureResourceLease>(
        MakeTextureResourceLease(a_handle, a_area, a_path));
    if (lease) ManagedTextureResources()[key] = std::move(lease);
}

std::size_t PurgeUnusedManagedTextureResources() {
    auto& managed = ManagedTextureResources();
    std::size_t purged = 0;
    for (auto it = managed.begin(); it != managed.end();) {
        // The manager's map owns one strong reference. Any additional owner
        // is a live script TEXTURE or a deferred draw command and must keep
        // the GL object alive. Erasing the sole-owner entry runs the resource
        // lease destructor and releases that texture through Renderer.
        if (it->second && it->second.use_count() == 1) {
            it = managed.erase(it);
            ++purged;
        } else {
            ++it;
        }
    }
    return purged;
}

void ReleaseTextureResourceLeases(Renderer& a_renderer) {
    // Texture leases may outlive the VM and are held in function-static
    // registries. Disconnect them while Renderer is still alive so static
    // destruction cannot call UnloadTexture through a dangling pointer.
    auto& managed = ManagedTextureResources();
    for (auto it = managed.begin(); it != managed.end();) {
        if (it->second && it->second->renderer == &a_renderer) {
            it->second->renderer = nullptr;
            it = managed.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& weakLease : AllTextureResources()) {
        if (auto lease = weakLease.lock()) {
            if (lease->renderer == &a_renderer) lease->renderer = nullptr;
        }
    }
}

std::shared_ptr<void> ResolveTextureResourceLease(gmMachine& a_machine,
                                                   const gmVariable& a_value) {
    return TextureLeaseFor(a_machine, a_value);
}

bool UnloadTextureResourceByPath(int a_area, const std::string& a_path) {
    auto& resources = TextureResources();
    const std::string key = TextureResourceKey(a_area, a_path);
    auto found = resources.find(key);
    if (found == resources.end()) return false;

    std::shared_ptr<TextureResourceLease> lease = found->second.lock();
    resources.erase(found);
    if (!lease) return false;
    ManagedTextureResources().erase(key);
    if (lease->renderer) lease->renderer->DetachTexturePath(lease->handle);
    return true;
}

bool UnloadTextureResourceByHandle(gmMachine& a_machine, const gmVariable& a_value,
                                   int a_area) {
    auto lease = TextureLeaseFor(a_machine, a_value);
    if (!lease || lease->area != a_area) return false;
    auto& resources = TextureResources();
    const std::string key = TextureResourceKey(lease->area, lease->path);
    auto found = resources.find(key);
    if (found == resources.end()) return false;
    auto registered = found->second.lock();
    if (registered != lease) return false;
    resources.erase(found);
    ManagedTextureResources().erase(key);
    if (lease->renderer) lease->renderer->DetachTexturePath(lease->handle);
    return true;
}

std::size_t GetLiveTextureResourceCount() {
    auto& resources = AllTextureResources();
    std::size_t count = 0;
    for (auto it = resources.begin(); it != resources.end();) {
        if (it->expired()) it = resources.erase(it);
        else { ++count; ++it; }
    }
    return count;
}

unsigned int ResolveTextureHandle(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type == g_textureType) {
        return ((TextureHandleData*)((gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref))->m_user)->handle;
    }
    if (a_var.m_type == GM_INT) return (unsigned int)a_var.m_value.m_int;
    return 0;
}

float ResolveTextureDimensionScale(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type != g_textureType) return 1.0f;
    gmUserObject* object = (gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref);
    if (!object || object->m_userType != g_textureType) return 1.0f;
    const TextureHandleData* data = (const TextureHandleData*)object->m_user;
    return data ? data->resizeFactor : 1.0f;
}

int GetTextureShaderType(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type != g_textureType) return 0;
    gmUserObject* object = (gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref);
    if (!object || object->m_userType != g_textureType) return 0;
    const TextureHandleData* data = (TextureHandleData*)object->m_user;
    return GetEngineContext().renderer->GetTextureShaderType(data->handle);
}

gmVariable MakePaletteHandle(gmMachine& a_machine) {
    TextureHandleData* data = new TextureHandleData();
    data->palette = std::make_shared<PaletteData>();
    data->nativeWidth = 256;
    data->nativeHeight = 1;
    gmUserObject* obj = a_machine.AllocUserObject(data, g_textureType);
    return gmVariable(obj);
}

bool GetTextureNativeDimensions(gmMachine& a_machine, const gmVariable& a_value,
                                int& a_width, int& a_height) {
    if (a_value.m_type != g_textureType) return false;
    gmUserObject* object = (gmUserObject*)a_machine.GetObject(a_value.m_value.m_ref);
    if (!object || object->m_userType != g_textureType || !object->m_user) return false;
    const TextureHandleData* data = (const TextureHandleData*)object->m_user;
    if (data->handle != 0) {
        GetEngineContext().renderer->GetTextureDimensions(data->handle, a_width, a_height);
        return true;
    }
    if (!data->palette) return false;
    a_width = data->nativeWidth;
    a_height = data->nativeHeight;
    return true;
}

PaletteData* ResolvePaletteData(gmMachine& a_machine, const gmVariable& a_var) {
    return ResolvePaletteDataLease(a_machine, a_var).get();
}

std::shared_ptr<PaletteData> ResolvePaletteDataLease(gmMachine& a_machine,
                                                      const gmVariable& a_var) {
    if (a_var.m_type != g_textureType) return nullptr;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref);
    if (!obj || obj->m_userType != g_textureType) return nullptr;
    return TexturePalette((TextureHandleData*)obj->m_user);
}

void BindTextureSubTexture(unsigned int a_handle, std::shared_ptr<PaletteData> a_palette) {
    GetEngineContext().renderer->BindTextureSubPalette(a_handle, std::move(a_palette));
}

void RegisterTextureBindings(gmMachine& a_machine) {
    g_textureType = a_machine.CreateUserType("TEXTURE");
    a_machine.RegisterTypeLibrary(g_textureType, s_textureLib, sizeof(s_textureLib) / sizeof(s_textureLib[0]));
    a_machine.RegisterUserCallbacks(g_textureType, nullptr, TextureGCDestruct, TextureAsString);

}
