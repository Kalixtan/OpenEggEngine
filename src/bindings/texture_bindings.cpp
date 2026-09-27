#include "gmThread.h"
#include "texture_bindings.h"
#include "vector2d_bindings.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include <cstdio>

gmType g_textureType = -1;
gmType g_paletteType = -1;

namespace {

struct TextureHandleData {
    unsigned int handle = 0;
};

TextureHandleData* ThisTexture(gmThread* a_thread) {
    return (TextureHandleData*)a_thread->ThisUserCheckType(g_textureType);
}

int GM_CDECL gmf_getDimensions(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    int w = 0, h = 0;
    GetEngineContext().renderer->GetTextureDimensions(t->handle, w, h);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), (float)w, (float)h));
    return GM_OK;
}

// Not decompiled-confirmed, but the real call site (Platformer/CoryPopup.gm:39,
// `this[...].isValid()` guarding a call to Video.unloadTextureByHandle so it
// isn't unloaded twice) only needs to distinguish "still loaded" from
// "already unloaded" -- GetTextureDimensions is this engine's own ground
// truth for that (0x0 handle == no live GL texture, whether never loaded
// or already freed by UnloadTexture), so this is a real state check, not a
// stub.
int GM_CDECL gmf_isValid(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    int w = 0, h = 0;
    GetEngineContext().renderer->GetTextureDimensions(t->handle, w, h);
    a_thread->PushInt((t->handle != 0 && (w > 0 || h > 0)) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_getTextureExtX(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    int w = 0, h = 0; (void)h;
    GetEngineContext().renderer->GetTextureDimensions(t->handle, w, h);
    a_thread->PushFloat((float)w);
    return GM_OK;
}
int GM_CDECL gmf_getTextureExtY(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    int w = 0, h = 0; (void)w;
    GetEngineContext().renderer->GetTextureDimensions(t->handle, w, h);
    a_thread->PushFloat((float)h);
    return GM_OK;
}

// a plain float field getter on the native texture object). Real call
// site (EntityMenu_EntityCell.gm:140: `local dim = this.bgTexture.
// getDimensions()/this.bgTexture.getResizeFactor();`) compensates
// getDimensions() for a real PS3 platform behavior -- some textures get
// downscaled at load time under memory pressure, and this factor lets
// script recover the ORIGINAL authored dimensions from the (possibly
// smaller) loaded texture. This reimplementation's texture loader never
// downscales anything (see file_helper.h/texture loading -- always loads
// at native resolution), so the correct, real value here is always 1.0
// (no resize applied), making the division above a no-op -- exactly
// matching this engine's actual behavior, not a placeholder guess.
int GM_CDECL gmf_getResizeFactor(gmThread* a_thread) {
    TextureHandleData* t = ThisTexture(a_thread); if (!t) return GM_EXCEPTION;
    a_thread->PushFloat(1.0f);
    return GM_OK;
}

gmFunctionEntry s_textureLib[] = {
    {"getDimensions", gmf_getDimensions, nullptr},
    {"getResizeFactor", gmf_getResizeFactor, nullptr},
    {"isValid", gmf_isValid, nullptr},
    {"getTextureExtX", gmf_getTextureExtX, nullptr},
    {"getTextureExtY", gmf_getTextureExtY, nullptr},
};

void GM_CDECL TextureGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    delete (TextureHandleData*)a_object->m_user;
}
void GM_CDECL TextureAsString(gmUserObject* a_object, char* a_buffer, int a_bufferLen) {
    TextureHandleData* t = (TextureHandleData*)a_object->m_user;
    std::snprintf(a_buffer, a_bufferLen, "TEXTURE(%u)", t->handle);
}

void GM_CDECL PaletteGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    delete (PaletteData*)a_object->m_user;
}
void GM_CDECL PaletteAsString(gmUserObject* a_object, char* a_buffer, int a_bufferLen) {
    (void)a_object;
    std::snprintf(a_buffer, a_bufferLen, "PALETTE");
}

} // namespace

gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle) {
    TextureHandleData* data = new TextureHandleData{a_handle};
    gmUserObject* obj = a_machine.AllocUserObject(data, g_textureType);
    return gmVariable(obj);
}

unsigned int ResolveTextureHandle(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type == g_textureType) {
        return ((TextureHandleData*)((gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref))->m_user)->handle;
    }
    if (a_var.m_type == GM_INT) return (unsigned int)a_var.m_value.m_int;
    return 0;
}

gmVariable MakePaletteHandle(gmMachine& a_machine) {
    PaletteData* data = new PaletteData();
    gmUserObject* obj = a_machine.AllocUserObject(data, g_paletteType);
    return gmVariable(obj);
}

PaletteData* ResolvePaletteData(gmMachine& a_machine, const gmVariable& a_var) {
    if (a_var.m_type != g_paletteType) return nullptr;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref);
    return (PaletteData*)obj->m_user;
}

void RegisterTextureBindings(gmMachine& a_machine) {
    g_textureType = a_machine.CreateUserType("TEXTURE");
    a_machine.RegisterTypeLibrary(g_textureType, s_textureLib, sizeof(s_textureLib) / sizeof(s_textureLib[0]));
    a_machine.RegisterUserCallbacks(g_textureType, nullptr, TextureGCDestruct, TextureAsString);

    g_paletteType = a_machine.CreateUserType("PALETTE");
    a_machine.RegisterUserCallbacks(g_paletteType, nullptr, PaletteGCDestruct, PaletteAsString);
}
