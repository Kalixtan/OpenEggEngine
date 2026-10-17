#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

class FileHelper;
#include "renderer.h"

struct Glyph {
    float width = 0, height = 0, advance = 0, yOfs = 0, bearingX = 0;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0; // normalized [0,1] atlas UVs
};

class BitmapFont {
public:
    bool Load(const std::string& a_jsonPath, const std::string& a_imageDir, Renderer& a_renderer,
              FileHelper* a_fileHelper = nullptr);

    bool LoadTTF(const std::string& a_ttfPath, float a_pixelHeight, Renderer& a_renderer);

    unsigned int GetTextureHandle() const { return m_textureHandle; }
    const Glyph* FindGlyph(int a_charCode) const;
    float GetRetailGlyphAdvance(uint32_t a_codepoint) const;
    // Video.getTextWidth's retail helper scans signed string bytes and uses
    // glyph advance/bearing/width fields rather than rendered-character
    // tokenization. This is intentionally separate from MeasureWidth().
    float MeasureRetailTextWidth(const std::string& a_text, double a_scale) const;

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
