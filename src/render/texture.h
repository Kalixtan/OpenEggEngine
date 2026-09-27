#pragma once
#include <GL/glew.h>
#include <string>

// Loose confirmed contract: Video.getTexture(path, area) returns a handle;
// drawList.drawTextureByHandle(...) draws it. We use the raw GL texture id
// as the "handle" value seen by GM scripts -- good enough until/unless we
// need handles to survive texture-atlas repacking.
struct Texture {
    GLuint id = 0;
    int width = 0;
    int height = 0;

    // RETAIL MATCH (reveng_pc/RETAIL_GL_GROUND_TRUTH.md, retail
    // FUN_00421a60): retail rounds every texture UP to power-of-two
    // dimensions, zero-fills the padding, uploads the padded buffer, and
    // then compensates at draw time by scaling UVs by the occupied
    // fraction. The real textured-quad draw (FUN_00421d90) reads exactly
    // these two floats out of the texture object (offsets +0x28/+0x2c) and
    // uses them as the quad's maximum U/V instead of a literal 1.0.
    //
    // This is NOT cosmetic: because retail never sets GL_TEXTURE_WRAP_S/T
    // (so wrap stays at the GL default GL_REPEAT), UVs outside [0,1] tile
    // across the PADDED extent -- i.e. the repeat period includes the
    // transparent padding, not just the image. PekkoAndJon/bg.gm's
    // kaleidoscope backdrop drives UVs far outside [0,1], so an unpadded
    // NPOT upload tiles with a different period and produces a visibly
    // different pattern.
    int potWidth = 0;      // width  rounded up to a power of two
    int potHeight = 0;     // height rounded up to a power of two
    float uMax = 1.0f;     // width  / potWidth
    float vMax = 1.0f;     // height / potHeight

    bool grayscale = false; // single-channel source (font atlas); see LoadFromFile
    // ENGINE PATCH (2026-08-17): chroma-key "paint by numbers" template
    // detection. Confirmed real, widespread asset convention across this
    // corpus's decorative sprites (EntityPacks/DangerVine/petal_big-1.png,
    // gameAssets/SwayTree/petal.png + sway_tree_idle.png, EntityPacks/
    // MarshmallowTree/marshmellow_*_idle.png -- all indexed PNGs whose
    // background is EXACTLY pure (0,255,0) at all 4 corners, contrasted
    // against a genuinely photographic asset like levels/vic1_ver2.png
    // whose corners are ordinary non-primary colours): these textures are
    // masks, not literal art -- pure green marks "background/transparent",
    // other flat regions mark shape areas meant to be tinted by whatever
    // colour-scheme colour is active at draw time (COLOURSCHEME_SetColour),
    // not the mask's own baked RGB. Drawing them through the plain
    // `texture * uColour` shader left the green background fully opaque
    // (visible as a solid green box) and tinted the shape by multiplying
    // uColour against the mask's arbitrary stencil hue (e.g. pure blue)
    // instead of replacing it outright -- confirmed wrong both ways (a real
    // "danger_fill" red scheme colour multiplied against a pure-blue mask
    // pixel can never produce red). detectedKeyColour is sampled once at
    // load time from the texture's 4 corners.
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
    // Persistent per-texture tint set by COLOURSCHEME_SetGeneratedPaletteColour
    // -- unlike COLOURSCHEME_SetColour's target drawList (transient, timing-
    // sensitive record/replay state), this native takes no drawList at all
    // and instead recolours the TEXTURE itself, so storing the override
    // here (keyed by texture identity, read fresh at every future draw of
    // this handle, no record/replay staleness possible) is the direct,
    // literal reading of its own real call shape -- confirmed real
    // signature (this texture handle, colour1, colour2), no drawList
    // parameter to defer onto.
    bool hasGeneratedTint = false;
    float fillTintR = 1.0f, fillTintG = 1.0f, fillTintB = 1.0f;
    float outlineTintR = 1.0f, outlineTintG = 1.0f, outlineTintB = 1.0f;

    static bool LoadFromFile(const std::string& a_path, Texture& a_out);

    // Uploads a caller-owned RGBA8 buffer (top-to-bottom row order, same as
    // any decoded image) directly, for procedurally-generated textures that
    // have no file to load from (animation_bindings.cpp's palette-tinted
    // .fsa/.bci frames). Flips rows on upload, matching LoadFromFile's
    // stbi_set_flip_vertically_on_load(true) -- both paths must agree on
    // the bottom-left-origin GL UV convention every other texture user
    // (drawTextureByHandle, the bitmap font atlas) already relies on.
    static bool LoadFromMemoryRGBA(const unsigned char* a_rgba, int a_width, int a_height, Texture& a_out);
};
