#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

class FileHelper;
#include "renderer.h"

// Loads the retail bitmap/SDF font format confirmed from
// diagnostic fixture's HelveticaNeueLTCom-{Hv,Lt}.json + matching _00.png atlas:
// a single JSON page with pixel-space glyph rects (u0/v0/u1/v1) and
// em-normalized metrics (width/height/advance/yOfs/bearingX, all fractions
// notes -- this one's fully known, no guessing involved.
struct Glyph {
    // yOfs is how far this glyph's bottom edge hangs BELOW the baseline
    // (small ~0.06-0.08 "optical overshoot" for round/flat-bottomed
    // letters, larger ~0.25-0.27 for true descenders g/j/p/q/y) --
    // confirmed via the real glyph-quad builder (see bitmap_font.cpp's
    // DrawText). height is the glyph's full ink-box height, so its top
    // edge is baseline + (height - yOfs).
    float width = 0, height = 0, advance = 0, yOfs = 0, bearingX = 0;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0; // normalized [0,1] atlas UVs
};

class BitmapFont {
public:
    bool Load(const std::string& a_jsonPath, const std::string& a_imageDir, Renderer& a_renderer,
              FileHelper* a_fileHelper = nullptr);

    // FoE's PC build ships a real TrueType font (grayscale-bold.ttf) instead
    // of a pre-baked JSON+PNG atlas -- there's no retail atlas to port, so
    // this bakes one ourselves (stb_truetype) into the SAME Glyph/UV shape
    // Load() produces, so DrawText/DrawGlyphQuad need no separate path.
    // a_pixelHeight is the em size baked into the atlas (bigger = crisper
    // at close zoom, matches this.transitionPerc-scaled title-screen text).
    bool LoadTTF(const std::string& a_ttfPath, float a_pixelHeight, Renderer& a_renderer);

    unsigned int GetTextureHandle() const { return m_textureHandle; }
    const Glyph* FindGlyph(int a_charCode) const;
    // Retail glyph lookup falls back to the first glyph record when a
    // codepoint is outside every charset range (FUN_00044074). This exposes
    // the matching advance for layout helpers without changing normal draws.
    float GetRetailGlyphAdvance(uint32_t a_codepoint) const;
    // Video.getTextWidth's retail helper scans signed string bytes and uses
    // glyph advance/bearing/width fields rather than rendered-character
    // tokenization. This is intentionally separate from MeasureWidth().
    float MeasureRetailTextWidth(const std::string& a_text, double a_scale) const;

    // Draws a_text centered/left/right-aligned at LOCAL point (a_x, a_y)
    // using a_scale world units per em -- matches drawList.print(x,y,z,
    // text,scale,align). a_edge/a_width forward to Renderer::DrawGlyphQuad
    // (set via drawList.setFontRenderAttributes).
    //
    // a_mat{A,B,C,D,E,F} is the caller's current full 2x3 draw-matrix
    // (DrawContext::Mat's a/b/c/d/e/f -- passed as loose floats, not that
    // type, to avoid a bitmap_font.h -> draw_context.h include this file
    // has no other reason for). Every glyph's LOCAL corner (computed from
    // a_x/a_y exactly as before) is transformed through it individually --
    // worldX = A*localX + C*localY + E, worldY = B*localX + D*localY + F,
    // identical to DC_TransformPoint -- rather than only moving a single
    // anchor point and leaving every glyph axis-aligned. Real text draws
    // inside a pushMatrix()/rotateZ() block expect the GLYPHS to rotate
    // with it (confirmed by CampaignScreenRecordPlayer.gm's "Home"/"Album
    // Select" labels, drawn sideways along the record's spine via a
    // rotateZ before print() -- previously only the anchor point moved,
    // every glyph still drew horizontal). Defaults to identity for the
    // rare caller that doesn't care.
    void DrawText(Renderer& a_renderer, const std::string& a_text, float a_x, float a_y,
                  float a_scale, int a_align, float a_edge, float a_width,
                  float a_matA = 1.0f, float a_matB = 0.0f, float a_matC = 0.0f, float a_matD = 1.0f,
                  float a_matE = 0.0f, float a_matF = 0.0f, float a_z = 0.0f) const;

    float MeasureWidth(const std::string& a_text, float a_scale) const;

private:
    unsigned int m_textureHandle = 0;
    Glyph m_retailFallbackGlyph;
    bool m_hasRetailFallbackGlyph = false;
    std::unordered_map<int, Glyph> m_glyphs;
};
