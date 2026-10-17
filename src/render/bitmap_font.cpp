#include "bitmap_font.h"
#include "../core/file_helper.h"
#include "rapidjson/document.h"
#include "stb_image.h" // declarations only -- implementation lives in texture.cpp
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../core/engine_context.h"
#include "../bindings/animation_bindings.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <cmath>

namespace {
enum { kAlignLeft = 0, kAlignRight = 1, kAlignCenter = 2 };

const char* ResolveIconAlias(const std::string& a_token) {
    if (a_token == "ACCEPT") return "CROSS";
    if (a_token == "CANCEL") return "CIRCLE";
    if (a_token == "CROSS_BUTTON") return "CROSS";
    return nullptr;
}

const char* RetailSpecialCharacterName(uint32_t a_codepoint) {
    static const char* const names[] = {
        "CROSS", "CIRCLE", "SQUARE", "TRIANGLE",
        "L1", "R1", "L2", "R2", "LS", "RS", "RSX", "RSY",
        "START", "SELECT", "LSX", "LSY", "DPAD", "CASSETTE",
        "SKULL", "SKULL_EMPTY"
    };
    if (a_codepoint < 0x2510 || a_codepoint > 0x2523) return nullptr;
    return names[a_codepoint - 0x2510];
}

uint32_t DecodeRetailCodepoint(const std::string& a_text, size_t& a_offset) {
    const unsigned char first = (unsigned char)a_text[a_offset++];
    unsigned int byteCount = first < 0x80 ? 1 : first < 0xe0 ? 2 :
                             first < 0xf0 ? 3 : first < 0xf8 ? 4 :
                             first < 0xfc ? 5 : 6;
    const unsigned char leadMasks[] = {0x00, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
    uint32_t codepoint = first & (unsigned char)~leadMasks[byteCount - 1];
    for (unsigned int i = 1; i < byteCount; ++i) {
        const unsigned char next = a_offset < a_text.size()
            ? (unsigned char)a_text[a_offset] : 0;
        if (a_offset < a_text.size()) ++a_offset;
        codepoint = (codepoint << 6) | (next & 0x3f);
    }
    return codepoint;
}

// Lazily loads Platformer/extFont/<NAME>.png, or asks the shared animation
// runtime for the current frame when retail's FSA-first path is present.
// Returns 0 (drawn as nothing, not a black quad) for unknown tokens --
// same "don't crash, don't draw garbage" leniency as the rest of this
// engine's native-binding gaps.
unsigned int GetIconTexture(const std::string& a_token, Renderer& a_renderer) {
    static std::unordered_map<std::string, unsigned int> s_cache;
    const char* alias = ResolveIconAlias(a_token);
    const std::string iconName = alias ? alias : a_token;
    EngineContext& ctx = GetEngineContext();
    if (ctx.fileHelper) {
        const std::string base = "Platformer/extFont/" + iconName;
        auto resolvedFsa = ctx.fileHelper->Resolve(base + ".fsa");
        if (resolvedFsa) {
            // Animated glyphs must resolve their current frame each draw;
            // caching the first GL handle here would freeze retail playback.
            int width = 0, height = 0;
            unsigned int handle = 0;
            if (GetFontAnimationFrame(*resolvedFsa, handle, width, height)) {
                // The desktop's extFont metrics currently assume the shipped
                // square assets, matching the extFont authoring requirement.
                return handle;
            } else {
                // Do not silently fall through to PNG: retail gives an
                // existing FSA priority even when its format is unsupported.
                static std::unordered_map<std::string, bool> s_warnedFsa;
                if (!s_warnedFsa[iconName]) {
                    std::fprintf(stderr,
                        "BitmapFont: unsupported FSA special character '%s'\n",
                        iconName.c_str());
                    s_warnedFsa[iconName] = true;
                }
            }
        }
    }

    // FSA takes priority in retail. Only consult/cache the static PNG when no
    // FSA asset exists at the resolved path.
    auto it = s_cache.find(a_token);
    if (it != s_cache.end()) return it->second;
    unsigned int handle = 0;
    if (ctx.fileHelper && !ctx.fileHelper->Resolve(
            "Platformer/extFont/" + iconName + ".fsa")) {
        auto resolved = ctx.fileHelper->Resolve(
            "Platformer/extFont/" + iconName + ".png");
        if (resolved) handle = a_renderer.LoadTexture(*resolved);
    }
    s_cache[a_token] = handle;
    return handle;
}

struct FontToken {
    bool isIcon = false;
    uint32_t codepoint = 0; // valid when !isIcon
    std::string iconToken;  // valid when isIcon
};

constexpr float kIconWidth = 0.98f;
constexpr float kIconHeight = 0.98f;
constexpr float kIconAdvance = 1.06f;
constexpr float kIconYOfs = 0.12446f;
constexpr float kIconBearingX = 0.04f;

std::vector<FontToken> Tokenize(const std::string& a_text) {
    std::vector<FontToken> out;
    size_t i = 0;
    while (i < a_text.size()) {
        // GameMonkey scripts use this legacy marker in place of a literal
        // UTF-8 registered sign. It is a font glyph, not an extFont image.
        if (a_text.compare(i, 5, "{(R)}") == 0) {
            FontToken t;
            t.codepoint = 0x00ae; // U+00AE REGISTERED SIGN
            out.push_back(t);
            i += 5;
            continue;
        }
        if (a_text[i] == '{') {
            size_t close = a_text.find('}', i + 1);
            if (close != std::string::npos && close - i <= 24 && close - i >= 2) {
                std::string name = a_text.substr(i + 1, close - i - 1);
                bool valid = !name.empty();
                for (char c : name) {
                    // Retail special-character names include L1/R1/L2/R2.
                    // Rejecting digits made these valid icon tokens render
                    // as the raw brace-delimited text instead.
                    if (!(std::isupper((unsigned char)c) ||
                          std::isdigit((unsigned char)c) || c == '_')) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    FontToken t;
                    t.isIcon = true;
                    t.iconToken = name;
                    out.push_back(t);
                    i = close + 1;
                    continue;
                }
            }
        }
        FontToken t;
        t.codepoint = DecodeRetailCodepoint(a_text, i);
        if (t.codepoint >= 0x2500 && t.codepoint <= 0x25ff) {
            const char* specialName = RetailSpecialCharacterName(t.codepoint);
            t.isIcon = true;
            if (specialName) {
                t.iconToken = specialName;
            } else {
                char iconName[8];
                std::snprintf(iconName, sizeof(iconName), "icon%02x",
                              (unsigned)(t.codepoint - 0x2500));
                t.iconToken = iconName;
            }
        }
        out.push_back(t);
    }
    return out;
}
}

bool BitmapFont::Load(const std::string& a_jsonPath, const std::string& a_imageDir, Renderer& a_renderer,
                      FileHelper* a_fileHelper) {
    std::ifstream f(std::filesystem::u8path(a_jsonPath), std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "BitmapFont::Load: could not open '%s'\n", a_jsonPath.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string text = ss.str();

    rapidjson::Document root;
    if (root.Parse<rapidjson::kParseTrailingCommasFlag>(text.c_str()).HasParseError() || !root.IsObject()) {
        std::fprintf(stderr, "BitmapFont::Load: JSON parse failed for '%s'\n", a_jsonPath.c_str());
        return false;
    }

    if (!root.HasMember("pages") || !root["pages"].IsArray() || root["pages"].Empty()) {
        std::fprintf(stderr, "BitmapFont::Load: no 'pages' in '%s'\n", a_jsonPath.c_str());
        return false;
    }
    const auto& page = root["pages"][0];

    if (!page.HasMember("image") || !page["image"].IsString()) return false;
    if (!page.HasMember("characters") || !page["characters"].IsArray()) return false;
    std::string imagePath = a_imageDir + "/" + page["image"].GetString();
    bool imagePathExists = true;
    if (a_fileHelper) {
        if (auto resolvedImage = a_fileHelper->Resolve(page["image"].GetString())) {
            imagePath = *resolvedImage;
        } else {
            imagePathExists = false;
        }
    }
    int atlasW = 0, atlasH = 0, atlasChannels = 0;
    m_textureHandle = imagePathExists ? a_renderer.LoadTexture(imagePath) : 0;
    if (!m_textureHandle && a_fileHelper &&
        std::string(page["image"].GetString()) == "courierFont_00.png") {
        // Retail creates Courier's atlas from courierFont.ttf at runtime; the
        // shipped JSON points at an atlas PNG that is absent from both PSARCs.
        // Keep the retail JSON metrics and placements, and rasterize the
        // corresponding compact signed-distance atlas from the shipped TTF.
        auto ttfPath = a_fileHelper->Resolve("courierFont.ttf");
        const int pixelSize = root.HasMember("pixelsize") && root["pixelsize"].IsInt()
            ? root["pixelsize"].GetInt() : 0;
        int maxU = 0, maxV = 0;
        for (const auto& c : page["characters"].GetArray()) {
            if (c.HasMember("u1") && c["u1"].IsNumber())
                maxU = std::max(maxU, c["u1"].GetInt());
            if (c.HasMember("v1") && c["v1"].IsNumber())
                maxV = std::max(maxV, c["v1"].GetInt());
        }
        auto roundAtlasDimension = [](int extent) {
            int result = 1;
            while (result < extent && result < 16384) result <<= 1;
            return result;
        };
        atlasW = roundAtlasDimension(maxU);
        atlasH = roundAtlasDimension(maxV);
        FT_Library ft = nullptr;
        FT_Face face = nullptr;
        bool generated = ttfPath && pixelSize > 0 && atlasW > 0 && atlasH > 0 &&
            FT_Init_FreeType(&ft) == 0 &&
            FT_New_Face(ft, ttfPath->c_str(), 0, &face) == 0 &&
            FT_Set_Pixel_Sizes(face, 0, (FT_UInt)pixelSize) == 0;
        if (generated) {
            std::vector<unsigned char> distance((size_t)atlasW * atlasH, 0);
            constexpr double kSpread = 4.0;
            for (const auto& c : page["characters"].GetArray()) {
                if (!c.HasMember("charCode") || !c["charCode"].IsInt() ||
                    !c.HasMember("u0") || !c["u0"].IsNumber() ||
                    !c.HasMember("v0") || !c["v0"].IsNumber() ||
                    !c.HasMember("u1") || !c["u1"].IsNumber() ||
                    !c.HasMember("v1") || !c["v1"].IsNumber()) continue;
                const int x0 = c["u0"].GetInt(), y0 = c["v0"].GetInt();
                const int width = c["u1"].GetInt() - x0;
                const int height = c["v1"].GetInt() - y0;
                if (width <= 0 || height <= 0 || x0 < 0 || y0 < 0 ||
                    x0 + width > atlasW || y0 + height > atlasH ||
                    FT_Load_Char(face, (FT_ULong)c["charCode"].GetInt(), FT_LOAD_RENDER) != 0)
                    continue;
                const FT_Bitmap& source = face->glyph->bitmap;
                const int sourceW = (int)source.width, sourceH = (int)source.rows;
                if (sourceW <= 0 || sourceH <= 0 || !source.buffer ||
                    source.pixel_mode != FT_PIXEL_MODE_GRAY) continue;

                // Resample the high-resolution FreeType coverage into the
                // exact compact glyph cell recorded by the retail JSON.
                std::vector<unsigned char> mask((size_t)width * height, 0);
                for (int y = 0; y < height; ++y) {
                    const double sy = (y + 0.5) * sourceH / height - 0.5;
                    const int yA = std::clamp((int)std::floor(sy), 0, sourceH - 1);
                    const int yB = std::min(yA + 1, sourceH - 1);
                    const double fy = std::clamp(sy - std::floor(sy), 0.0, 1.0);
                    const int sourcePitch = source.pitch;
                    auto sourceRow = [&](int row) {
                        const int physicalRow = sourcePitch >= 0 ? row : sourceH - 1 - row;
                        return source.buffer + (ptrdiff_t)physicalRow * std::abs(sourcePitch);
                    };
                    for (int x = 0; x < width; ++x) {
                        const double sx = (x + 0.5) * sourceW / width - 0.5;
                        const int xA = std::clamp((int)std::floor(sx), 0, sourceW - 1);
                        const int xB = std::min(xA + 1, sourceW - 1);
                        const double fx = std::clamp(sx - std::floor(sx), 0.0, 1.0);
                        const double top = sourceRow(yA)[xA] * (1.0 - fx) + sourceRow(yA)[xB] * fx;
                        const double bottom = sourceRow(yB)[xA] * (1.0 - fx) + sourceRow(yB)[xB] * fx;
                        mask[(size_t)y * width + x] = (unsigned char)std::clamp(
                            (int)std::lround(top * (1.0 - fy) + bottom * fy), 0, 255);
                    }
                }

                // Encode signed pixel distance around the 0.5 contour. A
                // one-texel virtual border supplies the outside samples that
                // lie beyond the glyph cell; the renderer smooths this field
                // at the final on-screen scale.
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        const bool inside = mask[(size_t)y * width + x] >= 128;
                        double nearestSquared = kSpread * kSpread;
                        for (int oy = -1; oy <= height; ++oy) {
                            for (int ox = -1; ox <= width; ++ox) {
                                const bool otherInside = ox >= 0 && ox < width &&
                                    oy >= 0 && oy < height &&
                                    mask[(size_t)oy * width + ox] >= 128;
                                if (otherInside == inside) continue;
                                const double dx = (double)ox - x;
                                const double dy = (double)oy - y;
                                nearestSquared = std::min(nearestSquared, dx * dx + dy * dy);
                            }
                        }
                        const double signedDistance = (inside ? 1.0 : -1.0) *
                            std::sqrt(nearestSquared);
                        const unsigned char value = (unsigned char)std::clamp(
                            (int)std::lround((0.5 + signedDistance / (2.0 * kSpread)) * 255.0),
                            0, 255);
                        distance[(size_t)(y0 + y) * atlasW + x0 + x] = value;
                    }
                }
            }

            std::vector<unsigned char> rgba((size_t)atlasW * atlasH * 4, 255);
            for (size_t i = 0; i < distance.size(); ++i)
                rgba[i * 4 + 3] = distance[i];
            m_textureHandle = a_renderer.CreateTextureFromRGBA(rgba.data(), atlasW, atlasH);
        }
        if (face) FT_Done_Face(face);
        if (ft) FT_Done_FreeType(ft);
        if (!m_textureHandle) {
            std::fprintf(stderr,
                "BitmapFont::Load: could not build retail Courier atlas from courierFont.ttf\n");
            return false;
        }
    } else if (m_textureHandle) {
        // Renderer only hands back a GL handle, so re-probe dimensions directly
        // (cheap: stbi_info reads just the header, no decode).
        stbi_info(imagePath.c_str(), &atlasW, &atlasH, &atlasChannels);
    }
    if (atlasW == 0 || atlasH == 0) {
        std::fprintf(stderr, "BitmapFont::Load: could not probe atlas dimensions for '%s'\n", imagePath.c_str());
        return false;
    }

    if (!page.HasMember("characters") || !page["characters"].IsArray()) return false;

    m_glyphs.clear();
    m_hasRetailFallbackGlyph = false;
    for (const auto& c : page["characters"].GetArray()) {
        if (!c.HasMember("charCode")) continue;
        Glyph g;
        g.width = c["width"].GetFloat();
        g.height = c["height"].GetFloat();
        g.advance = c["advance"].GetFloat();
        g.yOfs = c["yOfs"].GetFloat();
        g.bearingX = c["bearingX"].GetFloat();
        float u0px = c["u0"].GetFloat();
        float v0px = c["v0"].GetFloat();
        float u1px = c["u1"].GetFloat();
        float v1px = c["v1"].GetFloat();
        g.u0 = u0px / atlasW;
        g.u1 = u1px / atlasW;
        g.v0 = v0px / atlasH;
        g.v1 = v1px / atlasH;
        if (!m_hasRetailFallbackGlyph) {
            m_retailFallbackGlyph = g;
            m_hasRetailFallbackGlyph = true;
        }
        m_glyphs[c["charCode"].GetInt()] = g;
    }

    return true;
}

bool BitmapFont::LoadTTF(const std::string& a_ttfPath, float a_pixelHeight, Renderer& a_renderer) {
    FT_Library ft = nullptr;
    if (FT_Init_FreeType(&ft) != 0) {
        std::fprintf(stderr, "BitmapFont::LoadTTF: FT_Init_FreeType failed\n");
        return false;
    }
    FT_Face face = nullptr;
    if (FT_New_Face(ft, a_ttfPath.c_str(), 0, &face) != 0) {
        std::fprintf(stderr, "BitmapFont::LoadTTF: could not open '%s'\n", a_ttfPath.c_str());
        FT_Done_FreeType(ft);
        return false;
    }
    // Integer pixel size, matching the pixel-height contract the JSON path and
    // DrawText both assume.
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)(a_pixelHeight + 0.5f)) != 0) {
        std::fprintf(stderr, "BitmapFont::LoadTTF: FT_Set_Pixel_Sizes(%.0f) failed for '%s'\n",
                     a_pixelHeight, a_ttfPath.c_str());
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return false;
    }

    const int atlasW = 1024, atlasH = 1024;
    std::vector<unsigned char> bitmap((size_t)atlasW * atlasH, 0);

    // Simple left-to-right, top-to-bottom shelf packing with a 1px gutter, so
    // linear filtering at a glyph edge cannot bleed in a neighbour.
    const int kPad = 1;
    int penX = kPad, penY = kPad, rowH = 0;

    struct Packed { int x, y, w, h, left, top, advance; bool ok; };
    std::vector<Packed> packed(95);

    for (int i = 0; i < 95; ++i) {
        const int ch = 32 + i;
        Packed& p = packed[i];
        p = Packed{0, 0, 0, 0, 0, 0, 0, false};
        if (FT_Load_Char(face, (FT_ULong)ch, FT_LOAD_RENDER) != 0) continue;
        FT_GlyphSlot gs = face->glyph;
        const int gw = (int)gs->bitmap.width;
        const int gh = (int)gs->bitmap.rows;
        p.advance = (int)(gs->advance.x >> 6);
        p.left = gs->bitmap_left;
        p.top = gs->bitmap_top;
        p.ok = true;
        if (gw <= 0 || gh <= 0) continue; // space and friends: advance only

        if (penX + gw + kPad > atlasW) { // next shelf
            penX = kPad;
            penY += rowH + kPad;
            rowH = 0;
        }
        if (penY + gh + kPad > atlasH) {
            std::fprintf(stderr, "BitmapFont::LoadTTF: '%s' does not fit a %dx%d atlas at %.0fpx\n",
                         a_ttfPath.c_str(), atlasW, atlasH, a_pixelHeight);
            FT_Done_Face(face);
            FT_Done_FreeType(ft);
            return false;
        }
        for (int y = 0; y < gh; ++y) {
            const unsigned char* src = gs->bitmap.buffer + (size_t)y * gs->bitmap.pitch;
            unsigned char* dst = &bitmap[(size_t)(penY + y) * atlasW + penX];
            for (int x = 0; x < gw; ++x) dst[x] = src[x];
        }
        p.x = penX; p.y = penY; p.w = gw; p.h = gh;
        penX += gw + kPad;
        if (gh > rowH) rowH = gh;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Single-channel coverage -> RGBA. Replicated across RGB and alpha so the
    // atlas works under fixed-function GL_MODULATE (glColor4f supplies the
    // text colour, the atlas supplies coverage in alpha) -- see
    // Renderer::DrawGlyphQuad.
    std::vector<unsigned char> rgba((size_t)atlasW * atlasH * 4);
    for (size_t i = 0; i < bitmap.size(); ++i) {
        const unsigned char v = bitmap[i];
        rgba[i * 4 + 0] = v;
        rgba[i * 4 + 1] = v;
        rgba[i * 4 + 2] = v;
        rgba[i * 4 + 3] = v;
    }
    m_textureHandle = a_renderer.CreateTextureFromRGBA(rgba.data(), atlasW, atlasH);
    if (!m_textureHandle) return false;

    m_glyphs.clear();
    m_hasRetailFallbackGlyph = false;
    for (int i = 0; i < 95; ++i) {
        const Packed& p = packed[i];
        if (!p.ok) continue;
        Glyph g;
        g.advance = (float)p.advance / a_pixelHeight;
        if (!m_hasRetailFallbackGlyph) {
            m_retailFallbackGlyph = g;
            m_hasRetailFallbackGlyph = true;
        }
        if (p.w <= 0 || p.h <= 0) {
            m_glyphs[32 + i] = g;
            continue;
        }
        g.width = (float)p.w / a_pixelHeight;
        g.height = (float)p.h / a_pixelHeight;
        g.bearingX = (float)p.left / a_pixelHeight;
        g.yOfs = (float)(p.h - p.top) / a_pixelHeight;
        g.u0 = (float)p.x / atlasW;
        g.u1 = (float)(p.x + p.w) / atlasW;
        // CreateTextureFromRGBA -> LoadFromMemoryRGBA row-flips on upload
        // (matching stbi_set_flip_vertically_on_load(true) for the JSON+PNG
        // path). The rows packed above are PRE-flip, so they must be
        // converted to post-flip rows or sampling reads blank atlas space --
        // which is why glyphs once rendered fully invisible despite a
        // correctly baked atlas.
        g.v0 = (float)(atlasH - (p.y + p.h)) / atlasH;
        g.v1 = (float)(atlasH - p.y) / atlasH;
        m_glyphs[32 + i] = g;
    }

    return true;
}

const Glyph* BitmapFont::FindGlyph(int a_charCode) const {
    auto it = m_glyphs.find(a_charCode);
    return it == m_glyphs.end() ? nullptr : &it->second;
}

float BitmapFont::GetRetailGlyphAdvance(uint32_t a_codepoint) const {
    const Glyph* glyph = FindGlyph(static_cast<int>(a_codepoint));
    return glyph ? glyph->advance : (m_hasRetailFallbackGlyph ? m_retailFallbackGlyph.advance : 0.0f);
}

float BitmapFont::MeasureRetailTextWidth(const std::string& a_text, double a_scale) const {
    auto glyphForByte = [this](unsigned char a_byte) -> const Glyph* {
        const int signedCode = static_cast<int>(static_cast<int8_t>(a_byte));
        const Glyph* glyph = FindGlyph(signedCode);
        return glyph ? glyph : (m_hasRetailFallbackGlyph ? &m_retailFallbackGlyph : nullptr);
    };
    auto scaled = [a_scale](float a_value) -> float {
        return static_cast<float>(static_cast<double>(a_value) * a_scale);
    };

    float width = 0.0f;
    for (size_t i = 0; i < a_text.size(); ++i) {
        const Glyph* glyph = glyphForByte(static_cast<unsigned char>(a_text[i]));
        if (!glyph) continue;

        float accumulated = width;
        if (i == 0) {
            // First byte: advance minus its bearing.
            width = static_cast<float>(static_cast<double>(glyph->advance) * a_scale -
                                       static_cast<double>(scaled(glyph->bearingX)));
        } else if (i == a_text.size() - 1) {
            // Last byte: include the right ink edge, not the advance.
            accumulated = static_cast<float>(static_cast<double>(glyph->bearingX) * a_scale +
                                             static_cast<double>(scaled(glyph->width)));
            width = static_cast<float>(static_cast<double>(width) + accumulated);
        } else {
            width = static_cast<float>(static_cast<double>(width) +
                                       static_cast<double>(scaled(glyph->advance)));
        }
    }
    return width;
}

float BitmapFont::MeasureWidth(const std::string& a_text, float a_scale) const {
    float width = 0.0f;
    for (const FontToken& t : Tokenize(a_text)) {
        if (t.isIcon) {
            width += kIconAdvance * a_scale;
            continue;
        }
        const Glyph* g = FindGlyph((int)t.codepoint);
        if (!g) continue;
        width += g->advance * a_scale;
    }
    return width;
}

namespace {
// Same math as draw_context.h's DC_TransformPoint, duplicated here rather
// than pulled in via an include (this file has no other reason to know
// about DrawContext::Mat -- see the DrawText header comment). Takes a
// LOCAL corner point and the caller's full 2x3 affine matrix.
inline void TransformCorner(float a_matA, float a_matB, float a_matC, float a_matD,
                             float a_matE, float a_matF, float a_localX, float a_localY,
                             float& a_outX, float& a_outY) {
    a_outX = a_matA * a_localX + a_matC * a_localY + a_matE;
    a_outY = a_matB * a_localX + a_matD * a_localY + a_matF;
}
} // namespace

void BitmapFont::DrawText(Renderer& a_renderer, const std::string& a_text, float a_x, float a_y,
                           float a_scale, int a_align, float a_edge, float a_width,
                           float a_matA, float a_matB, float a_matC, float a_matD,
                           float a_matE, float a_matF, float a_z) const {
    float totalWidth = MeasureWidth(a_text, a_scale);
    float cursorX = a_x;
    if (a_align == kAlignCenter) cursorX -= totalWidth * 0.5f;
    else if (a_align == kAlignRight) cursorX -= totalWidth;

    for (const FontToken& t : Tokenize(a_text)) {
        if (t.isIcon) {
            unsigned int iconHandle = GetIconTexture(t.iconToken, a_renderer);
            if (iconHandle) {
                float lx0 = cursorX + kIconBearingX * a_scale;
                float lx1 = lx0 + kIconWidth * a_scale;
                float ly0 = a_y - kIconYOfs * a_scale;
                float ly1 = ly0 + kIconHeight * a_scale;
                float x0, y0, x1, y1, x2, y2, x3, y3;
                TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx0, ly0, x0, y0);
                TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx1, ly0, x1, y1);
                TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx1, ly1, x2, y2);
                TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx0, ly1, x3, y3);
                a_renderer.DrawTexturedQuad(x0, y0, x1, y1, x2, y2, x3, y3, iconHandle, a_z);
            }
            cursorX += kIconAdvance * a_scale;
            continue;
        }

        const Glyph* g = FindGlyph((int)t.codepoint);
        if (!g) continue;

        float lx0 = cursorX + g->bearingX * a_scale;
        float lx1 = lx0 + g->width * a_scale;
        float lyBottom = a_y - g->yOfs * a_scale;
        float lyTop = lyBottom + g->height * a_scale;

        float x0, y0, x1, y1, x2, y2, x3, y3;
        TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx0, lyBottom, x0, y0);
        TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx1, lyBottom, x1, y1);
        TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx1, lyTop, x2, y2);
        TransformCorner(a_matA, a_matB, a_matC, a_matD, a_matE, a_matF, lx0, lyTop, x3, y3);

        a_renderer.DrawGlyphQuad(x0, y0, x1, y1, x2, y2, x3, y3,
                                  m_textureHandle, g->u0, g->v0, g->u1, g->v1,
                                  a_edge, a_width, a_z);

        cursorX += g->advance * a_scale;
    }
}
