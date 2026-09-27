#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace {
// RETAIL MATCH (retail FUN_00421a40): round up to the next power of two.
// Retail's callers assert the result is >= the input ("tw>=w"/"th>=h",
// source lines 722/723), so an already-power-of-two input returns itself.
int NextPowerOfTwo(int a_v) {
    if (a_v <= 1) return 1;
    int p = 1;
    while (p < a_v) p <<= 1;
    return p;
}
} // namespace

bool Texture::LoadFromFile(const std::string& a_path, Texture& a_out) {
    stbi_set_flip_vertically_on_load(true); // matches GL's bottom-left-origin UV convention
    int w, h, channels;
    unsigned char* data = stbi_load(a_path.c_str(), &w, &h, &channels, 0);
    if (!data) {
        std::fprintf(stderr, "Texture::LoadFromFile: failed to load '%s'\n", a_path.c_str());
        return false;
    }

    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA8;
    bool grayscale = false;
    switch (channels) {
        case 1: format = GL_RED; internalFormat = GL_R8; grayscale = true; break;
        case 2: format = GL_RG; internalFormat = GL_RG8; break;
        case 3: format = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: format = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: break;
    }

    // See texture.h's isChromaKeyMask comment. Sample all 4 corners; if
    // they agree AND the colour is "pure-ish" (one channel near max, the
    // other two near zero -- a paint-tool primary, not something a real
    // photo/gradient background would land on by chance), treat it as a
    // chroma-key mask template.
    bool isChromaKeyMask = false;
    float keyR = 0.0f, keyG = 0.0f, keyB = 0.0f;
    if (channels >= 3 && w > 1 && h > 1) {
        auto pixelAt = [&](int x, int y, unsigned char out[3]) {
            const unsigned char* p = data + ((size_t)y * w + x) * channels;
            out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
        };
        unsigned char c00[3], c10[3], c01[3], c11[3];
        pixelAt(0, 0, c00);
        pixelAt(w - 1, 0, c10);
        pixelAt(0, h - 1, c01);
        pixelAt(w - 1, h - 1, c11);
        bool cornersMatch = std::memcmp(c00, c10, 3) == 0 && std::memcmp(c00, c01, 3) == 0 &&
                             std::memcmp(c00, c11, 3) == 0;
        if (cornersMatch) {
            int maxC = std::max({(int)c00[0], (int)c00[1], (int)c00[2]});
            int minC = std::min({(int)c00[0], (int)c00[1], (int)c00[2]});
            if (maxC >= 200 && minC <= 40) {
                isChromaKeyMask = true;
                keyR = c00[0] / 255.0f; keyG = c00[1] / 255.0f; keyB = c00[2] / 255.0f;
            }
        }
    }

    // See texture.h's hasSecondaryMask comment. These templates use a fixed
    // hue convention, not an area convention: pure-red-dominant pixels are
    // always the fill mask (colour1 slot) and pure-blue-dominant pixels are
    // always the outline mask (colour2 slot), regardless of which region
    // covers more of the sprite -- confirmed across the asset corpus (e.g.
    // sway_tree_idle.png has blue covering the majority of the sprite yet
    // blue is still the outline/colour2 slot, matching SwayingTree.gm's
    // "struct_out" petal colour).
    bool hasSecondaryMask = false;
    float secondaryR = 0.0f, secondaryG = 0.0f, secondaryB = 0.0f;
    bool hasOutlineMask = false;
    float outlineMaskR = 0.0f, outlineMaskG = 0.0f, outlineMaskB = 0.0f;
    if (isChromaKeyMask) {
        std::unordered_map<uint32_t, int> redHistogram, blueHistogram;
        size_t pixelCount = (size_t)w * h;
        uint32_t bgKey = ((uint32_t)(unsigned char)(keyR * 255.0f + 0.5f) << 16) |
                          ((uint32_t)(unsigned char)(keyG * 255.0f + 0.5f) << 8) |
                          (unsigned char)(keyB * 255.0f + 0.5f);
        for (size_t i = 0; i < pixelCount; ++i) {
            const unsigned char* p = data + i * channels;
            int maxC = std::max({(int)p[0], (int)p[1], (int)p[2]});
            int minC = std::min({(int)p[0], (int)p[1], (int)p[2]});
            if (maxC < 200 || minC > 40) continue; // not "pure"
            uint32_t key = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
            if (key == bgKey) continue; // background, already handled
            if (p[0] > p[2]) redHistogram[key]++;
            else if (p[2] > p[0]) blueHistogram[key]++;
        }
        auto pickBest = [](const std::unordered_map<uint32_t, int>& hist, uint32_t& outKey, int& outCount) {
            outKey = 0; outCount = 0;
            for (auto& kv : hist) {
                if (kv.second > outCount) { outCount = kv.second; outKey = kv.first; }
            }
        };
        uint32_t redKey, blueKey;
        int redCount, blueCount;
        pickBest(redHistogram, redKey, redCount);
        pickBest(blueHistogram, blueKey, blueCount);
        if (redCount >= 4) {
            hasSecondaryMask = true;
            secondaryR = ((redKey >> 16) & 0xFF) / 255.0f;
            secondaryG = ((redKey >> 8) & 0xFF) / 255.0f;
            secondaryB = (redKey & 0xFF) / 255.0f;
        }
        if (blueCount >= 4) {
            hasOutlineMask = true;
            outlineMaskR = ((blueKey >> 16) & 0xFF) / 255.0f;
            outlineMaskG = ((blueKey >> 8) & 0xFF) / 255.0f;
            outlineMaskB = (blueKey & 0xFF) / 255.0f;
        }
    }

    // RETAIL MATCH (retail FUN_00421a60, see texture.h's potWidth comment
    // and reveng_pc/RETAIL_GL_GROUND_TRUTH.md): expand to RGBA, pad up to
    // power-of-two with zeroed (fully transparent) pixels, upload the padded
    // buffer as GL_RGBA, and record the occupied fraction as uMax/vMax.
    //
    // Retail's upload is unconditionally GL_RGBA/GL_UNSIGNED_BYTE -- it has
    // no single-channel or RGB upload path at all -- so a 1-channel source
    // becomes white RGB with the coverage value as ALPHA. That is exactly
    // what the font atlas wants under fixed-function GL_MODULATE (glColor4f
    // supplies the text colour, the atlas supplies coverage), and it avoids
    // the GL_RED swizzle problem the old shader path needed kMaskFS for.
    const int potW = NextPowerOfTwo(w);
    const int potH = NextPowerOfTwo(h);
    std::vector<unsigned char> padded((size_t)potW * potH * 4, 0);
    for (int y = 0; y < h; ++y) {
        unsigned char* dstRow = &padded[(size_t)y * potW * 4];
        const unsigned char* srcRow = data + (size_t)y * w * channels;
        for (int x = 0; x < w; ++x) {
            const unsigned char* s = srcRow + (size_t)x * channels;
            unsigned char* d = dstRow + (size_t)x * 4;
            switch (channels) {
                case 1: d[0] = 255;  d[1] = 255;  d[2] = 255;  d[3] = s[0]; break;
                case 2: d[0] = s[0]; d[1] = s[0]; d[2] = s[0]; d[3] = s[1]; break;
                case 3: d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;  break;
                default: d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3]; break;
            }
        }
    }

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, potW, potH, 0, GL_RGBA, GL_UNSIGNED_BYTE, padded.data());
    // ENGINE PATCH REVERTED (this session): the GL_NEAREST reasoning below
    // was an unverified guess ("matching retail's visual style"), never
    // actually checked against the binary. Ghidra now confirms otherwise:
    // full analysis + IAT cross-reference sweep of the real texture-upload
    // function (retail FUN_00421a60) found exactly one glTexParameteri
    // pair, GL_TEXTURE_MIN_FILTER/GL_TEXTURE_MAG_FILTER both set to
    // GL_LINEAR (0x2601) -- no GL_NEAREST anywhere in the whole binary's
    // OPENGL32.dll import usage. Reverted to GL_LINEAR to match.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // ENGINE PATCH (FoE compat): PekkoAndJon/bg.gm's title-screen backdrop
    // samples this exact texture with UV explicitly driven outside [0,1]
    // (getVertex(i).u/.v = 0.5 + cos/sin(ang)*this.amt, growing past 1.0 as
    // this.amt animates) -- CLAMP_TO_EDGE just smeared the same edge pixel
    // outward once the radius left the texture, instead of the tiled
    // repeat retail's kaleidoscope pattern needs. GL_REPEAT is a no-op
    // difference for the overwhelming majority of textures (sprites/icons
    // sampled with UV already inside [0,1], where wrap mode never
    // triggers) so this is safe as the new default rather than something
    // gated to just this one texture.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    a_out.id = id;
    a_out.width = w;
    a_out.height = h;
    a_out.potWidth = potW;
    a_out.potHeight = potH;
    a_out.uMax = (float)w / (float)potW;
    a_out.vMax = (float)h / (float)potH;
    a_out.grayscale = grayscale;
    a_out.isChromaKeyMask = isChromaKeyMask;
    a_out.keyR = keyR; a_out.keyG = keyG; a_out.keyB = keyB;
    a_out.hasSecondaryMask = hasSecondaryMask;
    a_out.secondaryR = secondaryR; a_out.secondaryG = secondaryG; a_out.secondaryB = secondaryB;
    a_out.hasOutlineMask = hasOutlineMask;
    a_out.outlineMaskR = outlineMaskR; a_out.outlineMaskG = outlineMaskG; a_out.outlineMaskB = outlineMaskB;
    return true;
}

bool Texture::LoadFromMemoryRGBA(const unsigned char* a_rgba, int a_width, int a_height, Texture& a_out) {
    if (!a_rgba || a_width <= 0 || a_height <= 0) return false;

    // Row-flip to match stbi_set_flip_vertically_on_load(true)'s effect in
    // LoadFromFile -- see this function's header comment.
    std::vector<unsigned char> flipped((size_t)a_width * a_height * 4);
    size_t rowBytes = (size_t)a_width * 4;
    for (int y = 0; y < a_height; ++y) {
        std::memcpy(&flipped[(size_t)(a_height - 1 - y) * rowBytes], a_rgba + (size_t)y * rowBytes, rowBytes);
    }

    // RETAIL MATCH: retail has exactly ONE texture-upload function
    // (FUN_00421a60), so a procedurally-generated texture goes through the
    // same power-of-two padding, GL_RGBA upload, GL_LINEAR filtering and
    // default GL_REPEAT wrap as a file-loaded one. Previously this path
    // uploaded unpadded and used GL_CLAMP_TO_EDGE, diverging from the file
    // path on both counts.
    const int potW = NextPowerOfTwo(a_width);
    const int potH = NextPowerOfTwo(a_height);
    std::vector<unsigned char> padded((size_t)potW * potH * 4, 0);
    for (int y = 0; y < a_height; ++y) {
        std::memcpy(&padded[(size_t)y * potW * 4], &flipped[(size_t)y * rowBytes], rowBytes);
    }

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, potW, potH, 0, GL_RGBA, GL_UNSIGNED_BYTE, padded.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    a_out.id = id;
    a_out.width = a_width;
    a_out.height = a_height;
    a_out.potWidth = potW;
    a_out.potHeight = potH;
    a_out.uMax = (float)a_width / (float)potW;
    a_out.vMax = (float)a_height / (float)potH;
    a_out.grayscale = false;
    return true;
}
