#pragma once
#include <GL/glew.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// Native TEXTURE palette storage (retail offsets +0x40/+0x78), shared with
// its optional +0x4c subtexture object.
struct PaletteData {
    std::vector<uint8_t> rgba = std::vector<uint8_t>(256 * 4, 0); // retail ABGR byte order
    size_t entryCount = 256; // retail TEXTURE +0x24
    int mode = 0;
    bool dirty = false;
    bool hasPendingTint = false;
    float fillR = 1.0f, fillG = 1.0f, fillB = 1.0f;
    float outlineR = 1.0f, outlineG = 1.0f, outlineB = 1.0f;
};

int NextPowerOfTwo(int a_value);

struct Texture {
    GLuint id = 0;
    // Desktop source identity, used by renderer compatibility paths whose
    // retail draw contract depends on the loaded resource class/path.
    std::string sourcePath;
    int width = 0;
    int height = 0;
    int sourceBitsPerPixel = 32; // decoded resource-record +0x10 equivalent
    // Retail texture object's selector at +0x48; keep it on the shared
    // resource so multiple GM handles for one cached texture see the same
    // value, as repeated Video.getTexture() calls do in retail.
    int shaderType = 0;
    // Native parent texture's dirty flag (+0x78), separate from its optional
    // child palette object's state. +0x48 is shared with shaderType.
    bool paletteDirty = false;

    // GL allocation dimensions. PS3 Video's texture constructor uses the
    // decoded dimensions for the ordinary image formats supported here.
    int potWidth = 0;
    int potHeight = 0;
    // Retail TEXTURE +0x34/+0x38 UV extents; ordinary decoded textures set
    // both to 1.0 because source and allocation dimensions are equal.
    float uMax = 1.0f;
    float vMax = 1.0f;

    bool grayscale = false; // single-channel source (font atlas); see LoadFromFile
    bool isChromaKeyMask = false;
    float keyR = 0.0f, keyG = 0.0f, keyB = 0.0f;

    // ENGINE PATCH (2026-08-17): a second, distinct "pure" mask colour,
    // present in some of these templates alongside the background key
    // (e.g. gameAssets/SwayTree/sway_tree_idle.png: pure green background +
    // pure BLUE canopy body (857 sampled px) + pure RED trunk/outline (92
    // sampled px), not just a single background/shape split) -- detected
    // the same way as the background key (a histogram of "pure" -- one
    // channel >=200, other two <=40 -- pixel colours, picking the two most
    // common non-background ones). Real script usage confirms this is
    // meaningful, not incidental anti-aliasing noise:
    // EntityPacks/SwayingTree.gm:493's `COLOURSCHEME_SetGeneratedPaletteColour(
    // this.headTexture,this.colour1,this.colour2)` explicitly passes TWO
    // colour names (colour1 -> fill/body, colour2 -> outline/trunk) for
    // this exact texture. secondaryR/G/B holds the second mask colour's
    // detected RGB; hasSecondaryMask is false for single-tone templates
    // (e.g. DangerVine/petal_big-1.png, which only has background+one
    // shape colour) so they keep using the single uColour tint unchanged.
    // fillMask = the MOST common non-background pure colour (the shape's
    // main body -- "colour1" target, also what single-tone masks like
    // DangerVine/petal_big-1.png use as their only shape colour).
    // outlineMask = the SECOND most common non-background pure colour, if
    // one exists in real enough quantity (the "colour2" target, e.g. the
    // sway tree's trunk/outline) -- absent for genuinely single-tone masks.
    bool hasSecondaryMask = false;
    float secondaryR = 0.0f, secondaryG = 0.0f, secondaryB = 0.0f;
    bool hasOutlineMask = false;
    float outlineMaskR = 0.0f, outlineMaskG = 0.0f, outlineMaskB = 0.0f;
    bool hasGeneratedTint = false;
    float fillTintR = 1.0f, fillTintG = 1.0f, fillTintB = 1.0f;
    float outlineTintR = 1.0f, outlineTintG = 1.0f, outlineTintB = 1.0f;
    std::vector<unsigned char> paletteTemplateRgba;
    // Original PNG indexed-colour data, retained so retail TEXTURE palette
    // operations can replace the PLTE lookup table without losing indices.
    bool hasIndexedPixels = false;
    std::vector<unsigned char> indexedPixels; // bottom-up, one byte per texel
    std::vector<unsigned char> sourcePaletteRgba; // 256 entries, ordinary RGBA order
    size_t sourcePaletteEntries = 0;
    std::shared_ptr<PaletteData> subTexturePalette;

    static bool LoadFromFile(const std::string& a_path, Texture& a_out);
    // Decodes an encoded image payload (for example a completed save request)
    // and uploads it with the same orientation/sampler path as a file texture.
    static bool LoadFromEncodedMemory(const unsigned char* a_data, size_t a_size, Texture& a_out);

    // Uploads a caller-owned RGBA8 buffer (top-to-bottom row order, same as
    // any decoded image) directly, for procedurally-generated textures that
    // have no file to load from (animation_bindings.cpp's palette-tinted
    // .fsa/.bci frames). Flips rows on upload, matching LoadFromFile's
    // stbi_set_flip_vertically_on_load(true) -- both paths must agree on
    // the bottom-left-origin GL UV convention every other texture user
    // (drawTextureByHandle, the bitmap font atlas) already relies on.
    static bool LoadFromMemoryRGBA(const unsigned char* a_rgba, int a_width, int a_height,
                                   Texture& a_out, bool a_nearestFilter = false);
};
