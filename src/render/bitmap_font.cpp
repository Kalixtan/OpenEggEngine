#include "bitmap_font.h"
#include "../core/file_helper.h"
#include "rapidjson/document.h"
#include "stb_image.h" // declarations only -- implementation lives in texture.cpp
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../core/engine_context.h"
#include "../bindings/animation_bindings.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {
// Confirmed live on real RPCS3/retail via game_data_testcart's TEST
// 104/105/106 -- NOT the intuitive 0/1/2 order. Must match
// constants_bindings.cpp's TEXTALIGN_LEFT/CENTER/RIGHT.
enum { kAlignLeft = 0, kAlignRight = 1, kAlignCenter = 2 };

// `{TOKEN}` inline icon substitution -- confirmed real via
// game_data/Platformer/extFont/how_to_use.txt (left-in Queasy Games
// internal doc): "You can also add an entry in FontSpecialChars.cpp...
// {EXAMPLE} will use EXAMPLE.png". Real retail scripts embed these tokens
// directly in localized strings (ResizeScreenProcess.gm's "Use {LSY} to
// fit the outer corners...", PauseProcessBase's button-hint text) and
// expect the native font renderer to splice in the button-icon image
// in-place, matching the surrounding text's colour/height/baseline.
//
// {ACCEPT}/{CANCEL} are the two real semantic (not literal-filename)
// tokens -- retail maps these to the current platform's actual
// accept/cancel button, which is region-dependent (US convention: Cross
// accepts, Circle cancels; this reimplementation targets NPUA80543/US so
// that's hardcoded here rather than threaded through SystemConfig).
// {CROSS_BUTTON} appears once in the corpus (commented-out
// TouchEditProcessPS3.gm tip) as an alt spelling of {CROSS}.
const char* ResolveIconAlias(const std::string& a_token) {
    if (a_token == "ACCEPT") return "CROSS";
    if (a_token == "CANCEL") return "CIRCLE";
    if (a_token == "CROSS_BUTTON") return "CROSS";
    return nullptr;
}

// Retail FUN_00044540 maps this exact codepoint interval to the name table
// at EBOOT.elf 0x0041f81c. The table is the order shown below.
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

// Match FUN_0004387c's UTF-8 leading-byte length and continuation-bit
// assembly. Retail accepts legacy five/six-byte forms and does not validate
// continuation bytes; normal valid UTF-8 therefore maps identically, while
// malformed inputs avoid reading past the C++ string buffer.
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

// FUN_00041e40 writes the same five metrics as a normal font glyph:
// width=.98*(imageWidth/imageHeight), height=.98, advance=width+.08,
// yOfs=.12446, bearingX=.04. extFont's own instructions require square
// icons, making the retail width .98 and advance 1.06 for the supported
// assets. yOfs/bearingX use DrawText's ordinary glyph baseline formula.
constexpr float kIconWidth = 0.98f;
constexpr float kIconHeight = 0.98f;
constexpr float kIconAdvance = 1.06f;
constexpr float kIconYOfs = 0.12446f;
constexpr float kIconBearingX = 0.04f;

std::vector<FontToken> Tokenize(const std::string& a_text) {
    std::vector<FontToken> out;
    size_t i = 0;
    while (i < a_text.size()) {
        if (a_text[i] == '{') {
            size_t close = a_text.find('}', i + 1);
            if (close != std::string::npos && close - i <= 24 && close - i >= 2) {
                std::string name = a_text.substr(i + 1, close - i - 1);
                bool valid = !name.empty();
                for (char c : name) {
                    if (!(std::isupper((unsigned char)c) || c == '_')) { valid = false; break; }
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
    std::ifstream f(a_jsonPath, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "BitmapFont::Load: could not open '%s'\n", a_jsonPath.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string text = ss.str();

    // kParseTrailingCommasFlag: the retail bitmap font atlases
    // (HelveticaNeueLTCom-*.json etc, loaded via
    // GameMonkeyScript.loadTableFromJsonFile in the real engine) contain
    // trailing commas that a strict JSON parser rejects -- confirmed
    // necessary, not a stylistic choice.
    rapidjson::Document root;
    if (root.Parse<rapidjson::kParseTrailingCommasFlag>(text.c_str()).HasParseError() || !root.IsObject()) {
        std::fprintf(stderr, "BitmapFont::Load: JSON parse failed for '%s'\n", a_jsonPath.c_str());
        return false;
    }

    if (!root.HasMember("pages") || !root["pages"].IsArray() || root["pages"].Empty()) {
        std::fprintf(stderr, "BitmapFont::Load: no 'pages' in '%s'\n", a_jsonPath.c_str());
        return false;
    }
    const auto& page = root["pages"][0]; // single-page atlas only, matches confirmed data

    if (!page.HasMember("image") || !page["image"].IsString()) return false;
    std::string imagePath = a_imageDir + "/" + page["image"].GetString();
    if (a_fileHelper) {
        if (auto resolvedImage = a_fileHelper->Resolve(page["image"].GetString()))
            imagePath = *resolvedImage;
    }
    m_textureHandle = a_renderer.LoadTexture(imagePath);
    if (!m_textureHandle) return false;

    // Renderer only hands back a GL handle, so re-probe dimensions directly
    // (cheap: stbi_info reads just the header, no decode).
    int atlasW = 0, atlasH = 0, atlasChannels = 0;
    stbi_info(imagePath.c_str(), &atlasW, &atlasH, &atlasChannels);
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

// FoE PC's font isn't a pre-baked retail atlas (see header comment) -- this
// rasterises grayscale-bold.ttf into the same normalized Glyph shape Load()
// produces, so DrawText/DrawGlyphQuad need no separate code path.
//
// RETAIL MATCH: retail rasterises its text with **FreeType2** -- its own
// assert paths name `D:\projects\pekkoAndJon\code\FreeType2TestProcess.cpp`
// (see reveng_pc/RETAIL_GL_GROUND_TRUTH.md's module-architecture section).
// This used stb_truetype, which is a different rasteriser: it produces
// different coverage values for the same outline, so glyph pixels differed
// from retail's even when geometry and blending were identical. Using
// FreeType2 here removes that class of difference.
//
// Metrics mapping from FreeType to this codebase's normalized units (all
// divided by a_pixelHeight to get fraction-of-em, matching what the JSON
// atlas path already stores):
//
//   advance   = (glyph->advance.x >> 6)        / pixelHeight   [26.6 fixed]
//   width     = bitmap.width                   / pixelHeight
//   height    = bitmap.rows                    / pixelHeight
//   bearingX  = glyph->bitmap_left             / pixelHeight
//   yOfs      = (rows - bitmap_top)            / pixelHeight
//
// The yOfs line is the one worth care. FreeType's `bitmap_top` is the
// distance from the baseline UP to the glyph's top row. This codebase's yOfs
// is a y-DOWN signed offset from the baseline to the glyph's BOTTOM edge
// (DrawText does `lyBottom = a_y - yOfs*scale`, so a negative yOfs pushes the
// bottom edge above the baseline, which is the normal case for a
// non-descending glyph). Converting: top edge y-down is -bitmap_top, and the
// bottom edge is that plus the glyph height, giving (rows - bitmap_top).
// That is exactly the quantity stb_truetype expressed as (yoff + glyphH).
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
            // Space etc: zero-size ink box, advance only. MUST still be
            // inserted -- FindGlyph returning null makes DrawText's
            // `if (!g) continue;` skip advancing the cursor too, which is how
            // "flowers of error" once rendered as "flowersoferror".
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
    // Retail Video.getTextWidth -> FUN_00052c20 uses strlen and looks up each
    // signed byte separately via FUN_000441c4. That helper selects a glyph
    // range or falls back to the font's first glyph record.
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

        // Retail formula, decompiled from the real glyph-quad builder
        // (FUN_0003f4b8, called from DrawList.print's line-layout routine
        // function computes one glyph edge as `baselineY - yOfs*scale`
        // and the other as that same edge offset by `height * (k*scale)`
        // where k is a per-font sign/scale constant read from the current
        // font object (FUN_00052c0c) -- i.e. yOfs anchors DIRECTLY to the
        // baseline, no line-top/ascent reference glyph involved at all
        // (the earlier m_baselineFontY-subtraction approach was a wrong
        // model). Confirmed numerically against every glyph in
        // HelveticaNeueLTCom-Lt.json using bottom=baseline-yOfs,
        // top=bottom+height: caps/digits/most-lowercase all give a
        // consistent ~0.777-0.793 cap-height with a small ~0.06-0.08
        // baseline overshoot (expected optical correction on round/flat
        // letters like O/U/S), descenders (g/j/p/q/y) all give a
        // consistent ~0.25-0.27 hang below baseline with x-height (or,
        // for j, full cap-height) tops, and '.' gives a small ~0.17-tall
        // box sitting almost exactly on the baseline -- exactly matching
        // real typographic proportions for this typeface.
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
