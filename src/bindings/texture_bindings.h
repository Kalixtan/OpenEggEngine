#pragma once
#include "gmMachine.h"

// entry #: "TEXTURE"), previously threaded through this engine as a plain
// gmint GL handle instead (a known, documented architecture gap) --
// confirmed wrong two ways: (1) real retail throws `cannot do: TEXTURE + ""`
// when a script tries to string-concatenate a texture handle (confirmed
// live on real RPCS3 via game_data_testcart), which a plain int could never
// do; (2) real usage calls `.getDimensions()` as a METHOD directly on a
// texture (`Platformer/Main.gm`: `this.loadingScreen.texture.getDimensions()`,
// used immediately as a vector2D via `.x`/`.y`), which a plain int has no
// way to support. Implemented as a lightweight native user type wrapping
// the same underlying GL handle this engine already used, so every
// existing texture-consuming call (drawTextureByHandle, unloadTexture,
// ...) keeps working via ResolveTextureHandle() below.
void RegisterTextureBindings(gmMachine& a_machine);

// Wraps a_handle (an existing GL texture handle from Renderer::LoadTexture)
// as a real TEXTURE object.
gmVariable MakeTextureHandle(gmMachine& a_machine, unsigned int a_handle);

// Extracts the underlying GL handle from a_var. Accepts either a real
// TEXTURE object (the normal case) or a bare gmint (defensive fallback --
// nothing in this engine should still be passing a raw int for a texture
// once every call site is converted, but a stray 0/null default is
// harmless to tolerate). Returns 0 (an always-invalid handle) for
// anything else.
unsigned int ResolveTextureHandle(gmMachine& a_machine, const gmVariable& a_var);

extern gmType g_textureType;

// PALETTE -- confirmed real native type (Ghidra: Video.createPalette @
// 0x00076498, a factory/constructor allocating a native object, not a
// stub). Previously implemented as `a_thread->PushNull()` ("no palette-
// texture pipeline"), which silently broke every real caller: retail's
// paletted-sprite recolour path is COLOURSCHEME_SetGeneratedPaletteColour
// (texture_bindings.h's TEXTURE handles the *unpaletted* variant of the
// same native) called with a PALETTE, not a TEXTURE, followed by
// drawList.setTextureSubImg(texture, palette) to bind the two together
// (SafeVine.gm's thornTexture/thornPalette, MarshmallowTree.gm's
// idleTexture/idlePalette -- both real corpus call sites, both silently
// producing untinted textures with the stub). This engine's tint storage
// is keyed by TEXTURE handle (Renderer::SetTextureGeneratedTint), so a
// PALETTE object just holds the pending fill/outline colours until
// setTextureSubImg pairs it with a real texture handle and forwards them
// on -- see DC_gmf_setTextureSubImg.
struct PaletteData {
    bool hasPendingTint = false;
    float fillR = 1.0f, fillG = 1.0f, fillB = 1.0f;
    float outlineR = 1.0f, outlineG = 1.0f, outlineB = 1.0f;
};

// Wraps a fresh PaletteData as a real PALETTE object (Video.createPalette).
gmVariable MakePaletteHandle(gmMachine& a_machine);

// Returns the PaletteData for a_var if it's a real PALETTE object, else
// nullptr (defensive -- callers should treat that as "not a palette").
PaletteData* ResolvePaletteData(gmMachine& a_machine, const gmVariable& a_var);

extern gmType g_paletteType;
