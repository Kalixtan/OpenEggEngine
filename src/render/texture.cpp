#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cstdio>
#include <climits>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <iterator>
#include <zlib.h>

namespace {

uint32_t ReadBE32(const unsigned char* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

int Paeth(int a, int b, int c) {
    const int p = a + b - c;
    const int pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    return pa <= pb && pa <= pc ? a : (pb <= pc ? b : c);
}

// Preserve the actual palette indices for indexed PNG textures. stb_image
// expands them to RGBA for ordinary rendering, which loses the source index
// needed by retail TEXTURE.setPaletteColour/setTextureSubImg.
bool DecodeIndexedPng(const std::string& path, int expectedW, int expectedH,
                      std::vector<unsigned char>& outIndices,
                      std::vector<unsigned char>& outPalette, size_t& outPaletteEntries,
                      bool& outIndexedSource, int& outSourceBitsPerPixel) {
    outIndexedSource = false;
    outSourceBitsPerPixel = 0;
    std::ifstream in(std::filesystem::u8path(path), std::ios::binary);
    if (!in) return false;
    std::vector<unsigned char> file((std::istreambuf_iterator<char>(in)), {});
    static const unsigned char signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (file.size() < 8 || std::memcmp(file.data(), signature, 8) != 0) return false;

    int width = 0, height = 0, bitDepth = 0, colourType = -1, interlace = 1;
    std::vector<unsigned char> plte, trns, compressed;
    size_t pos = 8;
    while (pos + 12 <= file.size()) {
        const uint32_t size = ReadBE32(file.data() + pos);
        if ((uint64_t)pos + 12u + size > file.size()) return false;
        const unsigned char* type = file.data() + pos + 4;
        const unsigned char* chunk = file.data() + pos + 8;
        if (std::memcmp(type, "IHDR", 4) == 0 && size == 13) {
            width = (int)ReadBE32(chunk);
            height = (int)ReadBE32(chunk + 4);
            bitDepth = chunk[8];
            colourType = chunk[9];
            interlace = chunk[12];
            outIndexedSource = colourType == 3;
            const int sourceChannels = colourType == 0 ? 1 :
                (colourType == 2 ? 3 : (colourType == 3 ? 1 :
                (colourType == 4 ? 2 : (colourType == 6 ? 4 : 0))));
            outSourceBitsPerPixel = bitDepth * sourceChannels;
        } else if (std::memcmp(type, "PLTE", 4) == 0) {
            plte.assign(chunk, chunk + size);
        } else if (std::memcmp(type, "tRNS", 4) == 0) {
            trns.assign(chunk, chunk + size);
        } else if (std::memcmp(type, "IDAT", 4) == 0) {
            compressed.insert(compressed.end(), chunk, chunk + size);
        } else if (std::memcmp(type, "IEND", 4) == 0) {
            break;
        }
        pos += (size_t)size + 12;
    }

    if (colourType != 3 || interlace != 0 || width != expectedW || height != expectedH ||
        plte.empty() || plte.size() % 3 != 0 || plte.size() > 256 * 3 ||
        (bitDepth != 1 && bitDepth != 2 && bitDepth != 4 && bitDepth != 8)) return false;

    const size_t rowBytes = ((size_t)width * bitDepth + 7) / 8;
    const size_t inflatedBytes = (rowBytes + 1) * (size_t)height;
    std::vector<unsigned char> filtered(inflatedBytes);
    uLongf actualBytes = (uLongf)filtered.size();
    if (uncompress(filtered.data(), &actualBytes, compressed.data(), (uLong)compressed.size()) != Z_OK ||
        actualBytes != inflatedBytes) return false;

    std::vector<unsigned char> rows(rowBytes * (size_t)height);
    size_t src = 0;
    for (int y = 0; y < height; ++y) {
        const int filter = filtered[src++];
        unsigned char* row = rows.data() + (size_t)y * rowBytes;
        const unsigned char* previous = y > 0 ? row - rowBytes : nullptr;
        for (size_t x = 0; x < rowBytes; ++x) {
            const int raw = filtered[src++];
            const int left = x > 0 ? row[x - 1] : 0;
            const int up = previous ? previous[x] : 0;
            const int upperLeft = previous && x > 0 ? previous[x - 1] : 0;
            int predictor = 0;
            switch (filter) {
                case 0: break;
                case 1: predictor = left; break;
                case 2: predictor = up; break;
                case 3: predictor = (left + up) / 2; break;
                case 4: predictor = Paeth(left, up, upperLeft); break;
                default: return false;
            }
            row[x] = (unsigned char)(raw + predictor);
        }
    }

    outIndices.assign((size_t)width * height, 0);
    const int mask = (1 << bitDepth) - 1;
    for (int y = 0; y < height; ++y) {
        const unsigned char* row = rows.data() + (size_t)y * rowBytes;
        const size_t dstY = (size_t)(height - 1 - y) * width; // GL bottom-up
        for (int x = 0; x < width; ++x) {
            const int bit = x * bitDepth;
            const int shift = 8 - bitDepth - (bit & 7);
            outIndices[dstY + x] = (unsigned char)((row[bit / 8] >> shift) & mask);
        }
    }

    outPalette.assign(256 * 4, 0);
    const size_t entries = plte.size() / 3;
    outPaletteEntries = entries;
    for (size_t i = 0; i < entries; ++i) {
        outPalette[i * 4 + 0] = plte[i * 3 + 0];
        outPalette[i * 4 + 1] = plte[i * 3 + 1];
        outPalette[i * 4 + 2] = plte[i * 3 + 2];
        outPalette[i * 4 + 3] = i < trns.size() ? trns[i] : 255;
    }
    return true;
}

GLint RetailTextureFilterForSource(int channels, int sourceBitsPerPixel, bool indexedSource) {
    const bool retailLinearFormat = sourceBitsPerPixel == 24 || sourceBitsPerPixel == 32;
    const bool retailLinearGray8 = sourceBitsPerPixel == 8 && channels == 1 && !indexedSource;
    return (retailLinearFormat || retailLinearGray8) ? GL_LINEAR : GL_NEAREST;
}

constexpr GLint kRetailTextureWrap = GL_CLAMP_TO_EDGE;

void DrainTextureGlErrors(const char* a_path, const char* a_stage) {
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
        std::fprintf(stderr,
            "[texture-gl] pre-existing error=0x%04x before %s for '%s'\n",
            static_cast<unsigned>(error), a_stage, a_path);
    }
}

bool UploadRgbaTexture(const char* a_path, int a_width, int a_height,
                       const unsigned char* a_pixels, GLint a_filter,
                       GLuint& a_textureId) {
    DrainTextureGlErrors(a_path, "upload");
    a_textureId = 0;
    glGenTextures(1, &a_textureId);
    if (!a_textureId) {
        const GLenum error = glGetError();
        std::fprintf(stderr,
            "[texture-gl] glGenTextures failed error=0x%04x size=%dx%d path='%s'\n",
            static_cast<unsigned>(error), a_width, a_height, a_path);
        return false;
    }

    GLint previousBinding = 0;
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousBinding);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glBindTexture(GL_TEXTURE_2D, a_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, a_width, a_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, a_pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, a_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, a_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kRetailTextureWrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, kRetailTextureWrap);

    bool succeeded = true;
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
        std::fprintf(stderr,
            "[texture-gl] upload error=0x%04x size=%dx%d path='%s'\n",
            static_cast<unsigned>(error), a_width, a_height, a_path);
        succeeded = false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousBinding));
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
        std::fprintf(stderr,
            "[texture-gl] state-restore error=0x%04x path='%s'\n",
            static_cast<unsigned>(error), a_path);
        succeeded = false;
    }

    if (!succeeded) {
        glDeleteTextures(1, &a_textureId);
        a_textureId = 0;
    }
    return succeeded;
}

} // namespace

// OpenGL backend helper: pad dimensions for the current desktop texture
// upload path. This behavior is not yet verified against the PS3 renderer.
int NextPowerOfTwo(int a_v) {
    if (a_v <= 1) return 1;
    int p = 1;
    while (p < a_v) p <<= 1;
    return p;
}

bool Texture::LoadFromFile(const std::string& a_path, Texture& a_out) {
    stbi_set_flip_vertically_on_load(true); // matches GL's bottom-left-origin UV convention
    std::ifstream input(std::filesystem::u8path(a_path), std::ios::binary);
    const bool inputOpened = input.is_open();
    std::vector<unsigned char> encoded((std::istreambuf_iterator<char>(input)), {});
    int w = 0, h = 0, channels = 0;
    unsigned char* data = inputOpened && encoded.size() <= (size_t)std::numeric_limits<int>::max()
        ? stbi_load_from_memory(encoded.data(), (int)encoded.size(), &w, &h, &channels, 0)
        : nullptr;
    if (!data) {
        std::fprintf(stderr, "Texture::LoadFromFile: failed to load '%s'\n", a_path.c_str());
        return false;
    }

    std::vector<unsigned char> indexedPixels, sourcePalette;
    size_t sourcePaletteEntries = 0;
    bool indexedSource = false;
    int sourceBitsPerPixel = 0;
    const bool hasIndexedPixels = DecodeIndexedPng(a_path, w, h, indexedPixels, sourcePalette,
        sourcePaletteEntries, indexedSource, sourceBitsPerPixel);
    if (sourceBitsPerPixel <= 0) sourceBitsPerPixel = channels * 8;

    const int retailShaderType = sourceBitsPerPixel == 8 && channels == 1 && !indexedSource
        ? 8
        : (sourceBitsPerPixel == 8 && !indexedSource ? 3
           : (indexedSource ? 5 : 0));

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

    const int potW = w;
    const int potH = h;
    std::vector<unsigned char> rgba((size_t)potW * potH * 4, 0);
    const unsigned char keyR8 = (unsigned char)(keyR * 255.0f + 0.5f);
    const unsigned char keyG8 = (unsigned char)(keyG * 255.0f + 0.5f);
    const unsigned char keyB8 = (unsigned char)(keyB * 255.0f + 0.5f);
    for (int y = 0; y < h; ++y) {
        unsigned char* dstRow = &rgba[(size_t)y * potW * 4];
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
    // Preserve the decoded template before applying the key. Its mixed edge
    // entries contain coverage information needed by generated recolouring.
    std::vector<unsigned char> paletteTemplate;
    if (isChromaKeyMask) {
        paletteTemplate = rgba;
        for (size_t i = 0; i < (size_t)potW * potH; ++i) {
            unsigned char* p = &rgba[i * 4];
            if (p[0] == keyR8 && p[1] == keyG8 && p[2] == keyB8)
                p[0] = p[1] = p[2] = p[3] = 0;
        }
    }

    // The retail texture constructor selects NEAREST for indexed and low-bit
    // depth sources, LINEAR for 24/32-bit sources and its 8-bit grayscale
    // class, and CLAMP_TO_EDGE wrapping.
    // Keep this per uploaded texture so shader changes cannot inherit another
    // resource's sampler state.
    const GLint filter = RetailTextureFilterForSource(channels, sourceBitsPerPixel, indexedSource);
    GLuint id = 0;
    if (!UploadRgbaTexture(a_path.c_str(), potW, potH, rgba.data(), filter, id)) {
        stbi_image_free(data);
        return false;
    }

    stbi_image_free(data);

    a_out.id = id;
    a_out.width = w;
    a_out.height = h;
    a_out.sourceBitsPerPixel = sourceBitsPerPixel;
    a_out.shaderType = retailShaderType;
    a_out.potWidth = potW;
    a_out.potHeight = potH;
    a_out.uMax = 1.0f;
    a_out.vMax = 1.0f;
    a_out.grayscale = grayscale;
    a_out.isChromaKeyMask = isChromaKeyMask;
    a_out.keyR = keyR; a_out.keyG = keyG; a_out.keyB = keyB;
    a_out.hasSecondaryMask = hasSecondaryMask;
    a_out.secondaryR = secondaryR; a_out.secondaryG = secondaryG; a_out.secondaryB = secondaryB;
    a_out.hasOutlineMask = hasOutlineMask;
    a_out.outlineMaskR = outlineMaskR; a_out.outlineMaskG = outlineMaskG; a_out.outlineMaskB = outlineMaskB;
    a_out.paletteTemplateRgba = std::move(paletteTemplate);
    a_out.hasIndexedPixels = hasIndexedPixels;
    a_out.indexedPixels = std::move(indexedPixels);
    a_out.sourcePaletteRgba = std::move(sourcePalette);
    a_out.sourcePaletteEntries = sourcePaletteEntries;
    if (a_out.hasIndexedPixels && a_out.sourcePaletteRgba.size() == 256 * 4) {
        a_out.subTexturePalette = std::make_shared<PaletteData>();
        a_out.subTexturePalette->entryCount = sourcePaletteEntries;
        for (size_t i = 0; i < 256; ++i) {
            const size_t p = i * 4;
            a_out.subTexturePalette->rgba[p + 0] = a_out.sourcePaletteRgba[p + 3];
            a_out.subTexturePalette->rgba[p + 1] = a_out.sourcePaletteRgba[p + 2];
            a_out.subTexturePalette->rgba[p + 2] = a_out.sourcePaletteRgba[p + 1];
            a_out.subTexturePalette->rgba[p + 3] = a_out.sourcePaletteRgba[p + 0];
        }
    }
    return true;
}

bool Texture::LoadFromEncodedMemory(const unsigned char* a_data, size_t a_size, Texture& a_out) {
    if (!a_data || a_size == 0 || a_size > (size_t)INT_MAX) return false;
    int width = 0, height = 0, channels = 0;
    // LoadFromMemoryRGBA performs the same top-to-bottom -> GL row flip as
    // LoadFromFile's stb flip. Decode without stb's global flip to avoid
    // applying that orientation conversion twice.
    stbi_set_flip_vertically_on_load(false);
    unsigned char* rgba = stbi_load_from_memory(a_data, (int)a_size,
                                                 &width, &height, &channels, STBI_rgb_alpha);
    stbi_set_flip_vertically_on_load(true);
    if (!rgba) return false;
    const bool loaded = LoadFromMemoryRGBA(rgba, width, height, a_out);
    stbi_image_free(rgba);
    if (loaded) a_out.sourceBitsPerPixel = channels * 8;
    return loaded;
}

bool Texture::LoadFromMemoryRGBA(const unsigned char* a_rgba, int a_width, int a_height,
                                 Texture& a_out, bool a_nearestFilter) {
    if (!a_rgba || a_width <= 0 || a_height <= 0) return false;

    // Row-flip to match stbi_set_flip_vertically_on_load(true)'s effect in
    // LoadFromFile -- see this function's header comment.
    std::vector<unsigned char> flipped((size_t)a_width * a_height * 4);
    size_t rowBytes = (size_t)a_width * 4;
    for (int y = 0; y < a_height; ++y) {
        std::memcpy(&flipped[(size_t)(a_height - 1 - y) * rowBytes], a_rgba + (size_t)y * rowBytes, rowBytes);
    }

    // Runtime-generated RGBA textures use their actual dimensions. This is
    // also how the retail RGB/RGBA constructor reports its texture extents.
    const int potW = a_width;
    const int potH = a_height;
    std::vector<unsigned char> rgba((size_t)potW * potH * 4, 0);
    for (int y = 0; y < a_height; ++y) {
        std::memcpy(&rgba[(size_t)y * potW * 4], &flipped[(size_t)y * rowBytes], rowBytes);
    }

    const GLint filter = a_nearestFilter ? GL_NEAREST : GL_LINEAR;
    GLuint id = 0;
    if (!UploadRgbaTexture("<memory texture>", potW, potH, rgba.data(), filter, id))
        return false;

    a_out.id = id;
    a_out.width = a_width;
    a_out.height = a_height;
    a_out.potWidth = potW;
    a_out.potHeight = potH;
    a_out.uMax = 1.0f;
    a_out.vMax = 1.0f;
    a_out.grayscale = false;
    a_out.hasIndexedPixels = false;
    a_out.indexedPixels.clear();
    a_out.sourcePaletteRgba.clear();
    a_out.sourcePaletteEntries = 0;
    a_out.subTexturePalette.reset();
    return true;
}
