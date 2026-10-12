#include "gmThread.h"
#include "colour_bindings.h"
#include "animation_bindings.h"
#include "texture_bindings.h"
#include "gmTableObject.h"
#include "../core/engine_context.h"
#include "../render/draw_list.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

gmType g_colourType = -1;

namespace {

// Confirmed real: Main.gm:113 `global DEFAULT_COLOUR_SCHEME_ID=9;`, used
// whenever a level's own save data doesn't specify one (Main.gm:7743). Moved
// up from its previous spot further down this file so LookupRealColoursDat
// (below) can read it before its own declaration point.
int g_currentColourSchemeID = 9;

struct ColourData {
    float r = 0.0f, g = 0.0f, b = 0.0f; // confirmed real: no alpha channel on this type
};

ColourData* ThisColour(gmThread* a_thread) {
    return (ColourData*)a_thread->ThisUserCheckType(g_colourType);
}

// Defined below (with the real evidence/derivation for each colour) --
// forward-declared here so gmf_CM_getColour can share the exact same
// known-name table COLOURSCHEME_SetColour already falls back to.
bool LookupColourSchemeDefault(const std::string& a_name, float& a_r, float& a_g, float& a_b);

gmVariable PushNewColourVar(gmMachine& a_machine, const ColourData& a_c) {
    ColourData* data = new ColourData(a_c);
    return gmVariable(a_machine.AllocUserObject(data, g_colourType));
}

float Clamp01(float a_v) { return std::max(0.0f, std::min(1.0f, a_v)); }

// --- COLOUR ---

// Confirmed real: 0, 1, or 3 args -- 0 = black, 1 = copy-construct from
// another COLOUR, 3 = (r,g,b).
int GM_CDECL gmf_COLOUR(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    ColourData c;
    if (n == 0) {
        // c already black
    } else if (n == 1) {
        ColourData* src = (ColourData*)a_thread->ParamUserCheckType(0, g_colourType);
        if (!src) { GM_EXCEPTION_MSG("COLOUR: expecting a COLOUR for the 1-arg form"); return GM_EXCEPTION; }
        c = *src;
    } else if (n == 3) {
        c.r = a_thread->ParamFloatOrInt(0, 0.0f);
        c.g = a_thread->ParamFloatOrInt(1, 0.0f);
        c.b = a_thread->ParamFloatOrInt(2, 0.0f);
    } else {
        GM_EXCEPTION_MSG("COLOUR: expecting 0, 1 or 3 params");
        return GM_EXCEPTION;
    }
    a_thread->PushNewUser(new ColourData(c), g_colourType);
    return GM_OK;
}

int GM_CDECL gmf_getRed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; a_thread->PushFloat(c->r); return GM_OK; }
int GM_CDECL gmf_getGreen(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; a_thread->PushFloat(c->g); return GM_OK; }
int GM_CDECL gmf_getBlue(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; a_thread->PushFloat(c->b); return GM_OK; }
int GM_CDECL gmf_getRedIndexed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; a_thread->PushInt((gmint)(c->r * 255.0f)); return GM_OK; }
int GM_CDECL gmf_getGreenIndexed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; a_thread->PushInt((gmint)(c->g * 255.0f)); return GM_OK; }
int GM_CDECL gmf_getBlueIndexed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; a_thread->PushInt((gmint)(c->b * 255.0f)); return GM_OK; }
int GM_CDECL gmf_setRed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f); c->r = Clamp01(v); return GM_OK; }
int GM_CDECL gmf_setGreen(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f); c->g = Clamp01(v); return GM_OK; }
int GM_CDECL gmf_setBlue(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f); c->b = Clamp01(v); return GM_OK; }
int GM_CDECL gmf_setRedIndexed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0); c->r = Clamp01(v / 255.0f); return GM_OK; }
int GM_CDECL gmf_setGreenIndexed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0); c->g = Clamp01(v / 255.0f); return GM_OK; }
int GM_CDECL gmf_setBlueIndexed(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 0); c->b = Clamp01(v / 255.0f); return GM_OK; }

// a magnitude, not an average; can exceed 1.0. Kept exactly as decompiled
// even though that looks surprising.
float GrayscaleMagnitude(const ColourData& c) { return std::sqrt(c.r * c.r + c.g * c.g + c.b * c.b); }

int GM_CDECL gmf_makeGrayscale(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    float m = GrayscaleMagnitude(*c);
    c->r = c->g = c->b = m;
    return GM_OK;
}
int GM_CDECL gmf_makeAsGrayscale(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    float m = GrayscaleMagnitude(*c);
    a_thread->PushNewUser(new ColourData{m, m, m}, g_colourType);
    return GM_OK;
}

// Confirmed real: scales rgb by a scalar multiplier (no clamping in the
// decompiled body).
int GM_CDECL gmf_setIntensity(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    c->r *= v; c->g *= v; c->b *= v;
    return GM_OK;
}
int GM_CDECL gmf_makeWithIntensity(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f);
    a_thread->PushNewUser(new ColourData{c->r * v, c->g * v, c->b * v}, g_colourType);
    return GM_OK;
}

// getBrightness: CONFIRMED against the decompiled retail binary (Ghidra,
// EBOOT.elf, FUN_000c4e78 @ 0x000c4e78, referenced by Colour.getBrightness
// normalization constant, clamped to [min,max]. The normalization constant
// and clamp bounds are DAT_ literals whose exact float bits weren't pulled
// out of the binary -- 1/sqrt(3) (so pure white (1,1,1) maps to exactly
// 1.0) and a [0,1] clamp are the structurally obvious values and used
// here, but are inferred, not byte-confirmed.
float Brightness(const ColourData& c) {
    const float kNorm = 1.0f / 1.7320508f; // 1/sqrt(3), see comment above
    return Clamp01(GrayscaleMagnitude(c) * kNorm);
}

int GM_CDECL gmf_getBrightness(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    a_thread->PushFloat(Brightness(*c));
    return GM_OK;
}

// shade/makeWithShade: CONFIRMED against the decompiled retail binary
// (Ghidra, EBOOT.elf, FUN_000c4ed0 @ 0x000c4ed0 for shade,
// FUN_000c4fb8 @ 0x000c4fb8 for makeWithShade -- the latter just copies
// rgb into a new COLOUR then calls the former on it). Real formula,
// brightness-adaptive (NOT a flat blend toward black as originally
// guessed before checking):
//   - if this colour's brightness (see above) is already <= a threshold
//     (dark colour): lerp each channel TOWARD a target value by t
//     (lightens dark colours).
//   - otherwise (colour already bright): scale each channel down by
//     (target - t) (darkens bright colours).
//   - result is clamped to [minBound, target] per-channel.
// Threshold/target/minBound are DAT_ literals not byte-read from the
// binary; target reused as both the lerp target AND the clamp max is
// confirmed structurally, and 1.0/0.5/0.0 are the obvious values for
// "target=white, threshold=brightness midpoint, min=black" given
// brightness is normalized to [0,1] -- inferred, not byte-confirmed.
void ApplyShade(float t, ColourData& c) {
    const float kTarget = 1.0f;
    const float kThreshold = 0.5f;
    const float kMin = 0.0f;
    if (Brightness(c) <= kThreshold) {
        c.r = (kTarget - c.r) * t + c.r;
        c.g = (kTarget - c.g) * t + c.g;
        c.b = (kTarget - c.b) * t + c.b;
    } else {
        float k = kTarget - t;
        c.r *= k; c.g *= k; c.b *= k;
    }
    c.r = std::min(kTarget, std::max(kMin, c.r));
    c.g = std::min(kTarget, std::max(kMin, c.g));
    c.b = std::min(kTarget, std::max(kMin, c.b));
}

int GM_CDECL gmf_shade(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(t, 0, 0.0f);
    ApplyShade(t, *c);
    return GM_OK;
}
int GM_CDECL gmf_makeWithShade(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(t, 0, 0.0f);
    ColourData result = *c;
    ApplyShade(t, result);
    a_thread->PushNewUser(new ColourData(result), g_colourType);
    return GM_OK;
}

float RetailFsel(float selector, float nonnegative, float negative) {
    return std::signbit(selector) ? negative : nonnegative;
}

// Bytecode translation of EBOOT FUN_000c49f8. The native result is normalized
// H/S/L: hue degrees are multiplied by 1/360 and lightness is (max+min)/2.
// Preserve the PPC comparison choices, exact achromatic check, single-
// precision divisions, and fused hue multiply-adds; callers include COLOUR
// getters/interpolation and DrawList lightness shifting.
void RgbToHsl(const ColourData& c, float& h, float& s, float& l) {
    constexpr float kHueScale = 0.0027777778450399637f; // 1/360 from 0xc49ec
    constexpr float kHueSector = 60.0f;                 // 0xc49dc
    constexpr float kFullTurn = 360.0f;                 // 0xc49e0
    constexpr float kGreenOffset = 120.0f;              // 0xc49e4
    constexpr float kBlueOffset = 240.0f;               // 0xc49e8
    constexpr float kHalf = 0.5f;                       // 0xc49f0
    constexpr float kTwo = 2.0f;                        // 0xc49f4

    const float greenMinusBlue = c.g - c.b;
    const float maxGreenBlue = RetailFsel(greenMinusBlue, c.g, c.b);
    const float minGreenBlue = RetailFsel(greenMinusBlue, c.b, c.g);
    const float redMinusMax = c.r - maxGreenBlue;
    const float redMinusMin = c.r - minGreenBlue;
    const float mx = RetailFsel(redMinusMax, c.r, maxGreenBlue);
    const float mn = RetailFsel(redMinusMin, minGreenBlue, c.r);
    const float delta = mx - mn;

    float hueDegrees = 0.0f;
    float saturation = 0.0f;
    if (mx != mn) {
        if (c.g > c.r || c.b > c.r) {
            if (c.g < c.r || c.b > c.g) {
                if (c.b >= c.r && c.b >= c.g) {
                    hueDegrees = std::fma((c.r - c.g) / delta,
                                          kHueSector, kBlueOffset);
                }
            } else {
                hueDegrees = std::fma((c.b - c.r) / delta,
                                      kHueSector, kGreenOffset);
            }
        } else {
            const float redHue = std::fma((c.g - c.b) / delta,
                                          kHueSector, kFullTurn);
            hueDegrees = std::fmod(redHue, kFullTurn);
        }
    }

    h = hueDegrees * kHueScale;
    l = (mx + mn) * kHalf;
    if (mx != mn) {
        const float twiceLightness = l * kTwo;
        const float denominator = l > kHalf
            ? kTwo - twiceLightness : twiceLightness;
        saturation = delta / denominator;
    }
    s = saturation;
}
// Bytecode translated from EBOOT FUN_000c4c08/FUN_000c4b80. Keep the
// single-precision operation order and the PPC fused multiply-add points;
// FUN_000c4c08 wraps hue samples below zero and above one before calling
// FUN_000c4b80.
float RetailHueToRgb(float p, float q, float t) {
    constexpr float oneSixth = 0.1666666716337204f;
    constexpr float half = 0.5f;
    constexpr float twoThirds = 0.6666666865348816f;
    const float scaledDelta = (q - p) * 6.0f;
    if (t < oneSixth) return std::fma(scaledDelta, t, p);
    if (t < half) return q;
    if (t >= twoThirds) return p;
    return std::fma(scaledDelta, twoThirds - t, p);
}

void HslToRgb(float h, float s, float l, ColourData& out) {
    constexpr float oneThird = 0.3333333432674408f;
    constexpr float half = 0.5f;
    const float highSum = l + s;
    const float qHigh = std::fma(-l, s, highSum); // PPC fnmsubs
    const float qLow = l * (s + 1.0f);
    const float q = RetailFsel(l - half, qHigh, qLow);
    const float p = std::fma(l, 2.0f, -q); // PPC fmsubs

    float hueRed = h + oneThird;
    float hueGreen = h;
    float hueBlue = h - oneThird;
    hueRed = RetailFsel(hueRed, hueRed, hueRed + 1.0f);
    hueGreen = RetailFsel(hueGreen, hueGreen, hueGreen + 1.0f);
    hueBlue = RetailFsel(hueBlue, hueBlue, hueBlue + 1.0f);
    hueRed = RetailFsel(1.0f - hueRed, hueRed, hueRed - 1.0f);
    hueGreen = RetailFsel(1.0f - hueGreen, hueGreen, hueGreen - 1.0f);
    hueBlue = RetailFsel(1.0f - hueBlue, hueBlue, hueBlue - 1.0f);
    out.r = RetailHueToRgb(p, q, hueRed);
    out.g = RetailHueToRgb(p, q, hueGreen);
    out.b = RetailHueToRgb(p, q, hueBlue);
}

ColourData InterpolateHsl(const ColourData& a_from, const ColourData& a_to,
                          float a_amount) {
    const float t = Clamp01(a_amount);
    float ha, sa, la, hb, sb, lb;
    RgbToHsl(a_from, ha, sa, la);
    RgbToHsl(a_to, hb, sb, lb);

    float hue;
    if (std::fabs(hb - ha) >= 0.5f) {
        const float adjustedHa = (ha < 0.5f) ? ha + 1.0f : ha - 1.0f;
        hue = (adjustedHa - hb) * t + hb;
        hue -= std::floor(hue);
    } else {
        hue = (ha - hb) * t + hb;
    }

    ColourData result;
    HslToRgb(hue, (sa - sb) * t + sb, (la - lb) * t + lb, result);
    return result;
}

int GM_CDECL gmf_getHue(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; float h,s,l; RgbToHsl(*c,h,s,l); a_thread->PushFloat(h); return GM_OK; }
int GM_CDECL gmf_getSaturation(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; float h,s,l; RgbToHsl(*c,h,s,l); a_thread->PushFloat(s); return GM_OK; }
int GM_CDECL gmf_getLightness(gmThread* a_thread) { ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION; float h,s,l; RgbToHsl(*c,h,s,l); a_thread->PushFloat(l); return GM_OK; }
int GM_CDECL gmf_setHSL(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(h, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(s, 1, 0.0f); GM_FLOAT_OR_INT_PARAM(l, 2, 0.0f);
    HslToRgb(h, s, l, *c);
    return GM_OK;
}

gmFunctionEntry s_colourLib[] = {
    {"getRed", gmf_getRed, nullptr}, {"getGreen", gmf_getGreen, nullptr}, {"getBlue", gmf_getBlue, nullptr},
    {"getRedIndexed", gmf_getRedIndexed, nullptr}, {"getGreenIndexed", gmf_getGreenIndexed, nullptr}, {"getBlueIndexed", gmf_getBlueIndexed, nullptr},
    {"setRed", gmf_setRed, nullptr}, {"setGreen", gmf_setGreen, nullptr}, {"setBlue", gmf_setBlue, nullptr},
    {"setRedIndexed", gmf_setRedIndexed, nullptr}, {"setGreenIndexed", gmf_setGreenIndexed, nullptr}, {"setBlueIndexed", gmf_setBlueIndexed, nullptr},
    {"makeGrayscale", gmf_makeGrayscale, nullptr}, {"makeAsGrayscale", gmf_makeAsGrayscale, nullptr},
    {"setIntensity", gmf_setIntensity, nullptr}, {"makeWithIntensity", gmf_makeWithIntensity, nullptr},
    {"shade", gmf_shade, nullptr}, {"makeWithShade", gmf_makeWithShade, nullptr},
    {"getBrightness", gmf_getBrightness, nullptr},
    {"getHue", gmf_getHue, nullptr}, {"getSaturation", gmf_getSaturation, nullptr}, {"getLightness", gmf_getLightness, nullptr},
    {"setHSL", gmf_setHSL, nullptr},
};

void GM_CDECL ColourGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (ColourData*)a_object->m_user;
}

// --- ColourManager (see header comment -- no decompiled evidence, built
// from usage) ---

struct ColourManagerState {
    // Retail drawList.setColourRef stores pointers into its manager's packed
    // RGB records. Keep record addresses stable as new colors are appended.
    std::deque<std::pair<std::string, ColourData>> colours; // ID == index
    std::unordered_map<std::string, int> nameToIndex;
    std::string lastFilename;
};

ColourManagerState& GetColourManagerState() {
    static ColourManagerState state;
    return state;
}

// Resolves a name (string), an ID (int index), or an existing COLOUR
// object to a ColourData -- see header comment on why all three are
// accepted at every ColourManager call site.
bool ResolveColour(gmThread* a_thread, const gmVariable& a_var, ColourData& a_out) {
    ColourManagerState& state = GetColourManagerState();
    if (a_var.m_type == GM_STRING) {
        const char* name = ((gmStringObject*)a_thread->GetMachine()->GetObject(a_var.m_value.m_ref))->GetString();
        auto it = state.nameToIndex.find(name);
        if (it == state.nameToIndex.end()) return false;
        a_out = state.colours[it->second].second;
        return true;
    }
    if (a_var.m_type == GM_INT) {
        int id = a_var.m_value.m_int;
        if (id < 0 || id >= (int)state.colours.size()) return false;
        a_out = state.colours[id].second;
        return true;
    }
    if (a_var.m_type == g_colourType) {
        a_out = *(ColourData*)((gmUserObject*)a_thread->GetMachine()->GetObject(a_var.m_value.m_ref))->m_user;
        return true;
    }
    return false;
}

int FindOrAddIndex(const std::string& a_name) {
    ColourManagerState& state = GetColourManagerState();
    auto it = state.nameToIndex.find(a_name);
    if (it != state.nameToIndex.end()) return it->second;
    int idx = (int)state.colours.size();
    state.colours.push_back({a_name, ColourData{}});
    state.nameToIndex[a_name] = idx;
    return idx;
}

int GM_CDECL gmf_CM_load(gmThread* a_thread) {
    ColourManagerState& state = GetColourManagerState();
    std::string path = state.lastFilename;
    if (a_thread->GetNumParams() >= 1) { GM_STRING_PARAM(p, 0, ""); path = p; }
    if (path.empty()) { GM_EXCEPTION_MSG("ColourManager.load: no filename given and none loaded previously"); return GM_EXCEPTION; }

    std::string text;
    if (!GetEngineContext().fileHelper->ReadTextFile(path, text)) {
        std::fprintf(stderr, "ColourManager.load: '%s' not found\n", path.c_str());
        a_thread->PushInt(0);
        return GM_OK;
    }

    state.colours.clear();
    state.nameToIndex.clear();
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        std::istringstream ls(line);
        std::string name; float r, g, b;
        if (!(ls >> name >> r >> g >> b)) continue;
        int idx = (int)state.colours.size();
        state.colours.push_back({name, ColourData{r, g, b}});
        state.nameToIndex[name] = idx;
    }
    state.lastFilename = path;
    a_thread->PushInt(1);
    return GM_OK;
}

// Doesn't actually write to disk -- game_data is this project's reference
// corpus, not a live save slot, and this is a rare (3 call sites, all
// inside an interactive colour-editing debug tool) function that isn't on
// any path this milestone needs. Logs instead of silently doing nothing.
int GM_CDECL gmf_CM_save(gmThread* a_thread) {
    ColourManagerState& state = GetColourManagerState();
    std::string path = state.lastFilename;
    if (a_thread->GetNumParams() >= 1) { GM_STRING_PARAM(p, 0, ""); path = p; }
    std::fprintf(stderr, "ColourManager.save: not writing to disk ('%s', %d colours) -- see colour_bindings.cpp\n",
                 path.c_str(), (int)state.colours.size());
    return GM_OK;
}

int GM_CDECL gmf_CM_getCurrentFilename(gmThread* a_thread) {
    a_thread->PushNewString(GetColourManagerState().lastFilename.c_str());
    return GM_OK;
}

// Was a straight ResolveColour-or-null -- a REAL, load-bearing gap: this
// engine's ColourManager registry starts completely empty (no real
// colour-scheme asset files exist anywhere in this corpus to load, see
// LookupColourSchemeDefault's header comment), so
// `ColourManager.getColour("clear")` -- confirmed real, Platformer/
// Main.gm: `this.clearColour=ColourManager.getColour("clear");`, cached
// once and reused every preDraw() -- silently returned null forever.
// That null then flowed into `ColourManager.setColour("clear",
// this.clearColour)` right before the actual screen clear, which is a
// genuine "not a valid colour" case, and the resulting fallback/failure
// there is exactly why the tutorial level's real background painted
// black instead of the intended colour every single frame. Falls back to
// the same known-name table COLOURSCHEME_SetColour already uses before
// giving up to null, so a name with a real confirmed default (clear,
// player_fill, ground_fill, ...) resolves consistently everywhere it's
// looked up, not just through COLOURSCHEME_SetColour's own direct path.
int GM_CDECL gmf_CM_getColour(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    ColourData c;
    gmVariable arg0 = a_thread->Param(0);
    // See gmf_COLOURSCHEME_SetColour's header comment -- plain string names
    // bypass the registry (stale-scheme shadowing) and go straight to the
    // live, scheme-aware lookup.
    bool ok = (arg0.m_type != GM_STRING) && ResolveColour(a_thread, arg0, c);
    // See gmf_COLOURSCHEME_SetColour's comment -- a resolved-but-exactly-
    // black hit is treated the same as a miss.
    if (ok && c.r == 0.0f && c.g == 0.0f && c.b == 0.0f) ok = false;
    if (!ok) {
        bool gotDefault = false;
        if (arg0.m_type == GM_STRING) {
            const char* name = ((gmStringObject*)a_thread->GetMachine()->GetObject(arg0.m_value.m_ref))->GetString();
            gotDefault = GetNamedColourRGB(name, c.r, c.g, c.b);
        }
        if (!gotDefault) { a_thread->PushNull(); return GM_OK; }
    }
    a_thread->PushNewUser(new ColourData(c), g_colourType);
    return GM_OK;
}

int GM_CDECL gmf_CM_setColour(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmVariable keyVar = a_thread->Param(0);
    ColourManagerState& state = GetColourManagerState();
    int idx;
    if (keyVar.m_type == GM_STRING) {
        idx = FindOrAddIndex(((gmStringObject*)a_thread->GetMachine()->GetObject(keyVar.m_value.m_ref))->GetString());
    } else if (keyVar.m_type == GM_INT) {
        idx = keyVar.m_value.m_int;
        if (idx < 0 || idx >= (int)state.colours.size()) { GM_EXCEPTION_MSG("ColourManager.setColour: bad colour ID"); return GM_EXCEPTION; }
    } else {
        GM_EXCEPTION_MSG("ColourManager.setColour: expecting a name or ID for param 0");
        return GM_EXCEPTION;
    }
    ColourData c;
    if (!ResolveColour(a_thread, a_thread->Param(1), c)) { GM_EXCEPTION_MSG("ColourManager.setColour: param 1 isn't a colour"); return GM_EXCEPTION; }
    state.colours[idx].second = c;
    return GM_OK;
}

// Confirmed real: 3 or 4 args (perc, colourA, colourB, [outColour]) --
// colourA/colourB accept name/ID/COLOUR interchangeably (see
// ResolveColour). If outColour (an existing COLOUR object) is given, the
// result is written into it in place and it's returned; matches real
// usage `ColourManager.setColour(name, ColourManager.makeInterpolatedColour(
// perc,a,b,existingColourObj))` patterns where reusing an existing COLOUR
// avoids an allocation every frame.
//
// CONFIRMED against the decompiled retail binary (Ghidra, EBOOT.elf,
// FUN_000c4d20 @ 0x000c4d20, the native helper backing
// Colour.COLOUR_MakeInterpolatedColour and, structurally, this function
// too): interpolation happens in HSL space, NOT plain RGB lerp as
// originally implemented here -- each colour is converted via the same
// RGB->HSL transform Colour.getHue/getSaturation/getLightness use (see
// RgbToHsl above, itself already structurally confirmed against
// FUN_000c49f8), hue is lerped with shortest-path wraparound (going the
// "short way" around the hue wheel rather than always increasing), and the
// result is converted back with the same HSL->RGB transform setHSL uses
// (HslToRgb above). Also confirmed: the lerp direction is
// `t=0 -> colourB, t=1 -> colourA` (the SECOND colour argument at t=0),
// not the more intuitive `t=0 -> colourA` -- verified against the real
// call site game_data/Main.gm:667's `perc=this.selectionPulsePerc.
// getValue()` (0 at rest) landing on "cory_selected_base" (arg2, the
// baseline) rather than "cory_selected_pulse" (arg1, the flash colour),
// which matches "at rest show the base colour" far better than the
// reverse would.
int GM_CDECL gmf_CM_makeInterpolatedColour(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n < 3 || n > 4) { GM_EXCEPTION_MSG("ColourManager.makeInterpolatedColour: expecting 3-4 params"); return GM_EXCEPTION; }
    GM_FLOAT_OR_INT_PARAM(perc, 0, 0.0f);
    ColourData a, b;
    if (!ResolveColour(a_thread, a_thread->Param(1), a) || !ResolveColour(a_thread, a_thread->Param(2), b)) {
        GM_EXCEPTION_MSG("ColourManager.makeInterpolatedColour: param 1/2 isn't a resolvable colour");
        return GM_EXCEPTION;
    }
    float t = Clamp01(perc);

    float ha, sa, la, hb, sb, lb;
    RgbToHsl(a, ha, sa, la);
    RgbToHsl(b, hb, sb, lb);

    float resultH;
    if (std::fabs(hb - ha) >= 0.5f) {
        float adjustedHa = (ha < 0.5f) ? ha + 1.0f : ha - 1.0f; // shortest-path wraparound
        resultH = (adjustedHa - hb) * t + hb;
        resultH -= std::floor(resultH); // wrap into [0,1)
    } else {
        resultH = (ha - hb) * t + hb;
    }
    float resultS = (sa - sb) * t + sb;
    float resultL = (la - lb) * t + lb;

    ColourData result;
    HslToRgb(resultH, resultS, resultL, result);

    if (n == 4 && a_thread->Param(3).m_type == g_colourType) {
        ColourData* out = (ColourData*)((gmUserObject*)a_thread->GetMachine()->GetObject(a_thread->Param(3).m_value.m_ref))->m_user;
        *out = result;
        a_thread->Push(a_thread->Param(3));
        return GM_OK;
    }
    a_thread->PushNewUser(new ColourData(result), g_colourType);
    return GM_OK;
}

int GM_CDECL gmf_CM_getColourID(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_STRING_PARAM(name, 0, "");
    ColourManagerState& state = GetColourManagerState();
    auto it = state.nameToIndex.find(name);
    if (it == state.nameToIndex.end()) { a_thread->PushInt(-1); return GM_OK; }
    a_thread->PushInt(it->second);
    return GM_OK;
}

int GM_CDECL gmf_CM_getColourNames(gmThread* a_thread) {
    ColourManagerState& state = GetColourManagerState();
    gmTableObject* table = a_thread->GetMachine()->AllocTableObject();
    for (size_t i = 0; i < state.colours.size(); ++i) {
        gmStringObject* s = a_thread->GetMachine()->AllocStringObject(state.colours[i].first.c_str());
        table->Set(a_thread->GetMachine(), (int)i, gmVariable(s));
    }
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_CM_removeColour(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); GM_STRING_PARAM(name, 0, "");
    ColourManagerState& state = GetColourManagerState();
    auto it = state.nameToIndex.find(name);
    if (it == state.nameToIndex.end()) return GM_OK;
    // Erase without disturbing other entries' IDs (index-based) -- clear
    // the slot in place rather than std::vector::erase (which would shift
    // every later ID, breaking any getColourID() result cached by a
    // script). Real removeColour semantics for already-cached IDs aren't
    // recoverable from usage alone; leaving a "hole" is the safer choice.
    int idx = it->second;
    state.colours[idx].second = ColourData{};
    state.nameToIndex.erase(it);
    return GM_OK;
}

// COLOURSCHEME_SetColour(name, [alpha], target) / COLOURSCHEME_SetColourScheme(schemeId, [force])
// -- confirmed real (widespread: Platformer/Main.gm's own preDraw() calls
// `COLOURSCHEME_SetColour("clear",drawList);` before drawing the level
// background) but entirely unregistered until now. This was a real,
// process-crashing-adjacent gap: null-call leniency made every call a
// silent no-op, so the shared draw colour (see draw_context.h -- Video/
// drawList/shape all share ONE current-colour value) never got reset
// away from whatever a PREVIOUS frame's "still loading" branch left it
// at (`drawList.setColour(0,0,0,1)`, i.e. black) -- once the level
// finished loading and this SHOULD have picked a real background colour,
// it silently stayed black, painting over any real content drawn later
// in the same preDraw() pass.
//
// No real colour-scheme asset files exist anywhere in this corpus (only
// scripts that reference the concept) to source real RGB values from, so
// this resolves through the same ColourManager registry every other real
// colour lookup uses (in case something has actually populated it by
// name), falling back to a small table of plausible colours for the
// handful of names confirmed load-bearing for basic visibility (the
// background/"clear" colour above all), and a stable per-name-hash
// fallback colour for anything else -- deterministic (same name always
// gets the same colour) rather than an arbitrary/flickering guess.
// COLOURSCHEME_SetColourScheme itself (selecting WHICH scheme's colours
// apply) has no real effect here since there's no scheme data to select
// between -- consumed and ignored rather than crashing.
// tutorial_welcome_screen.png and tutorial_gameplay_moving.png, both actual
// frames of THIS exact tutorial level): background/platform/player colours
// read off the real rendered frame at known pixel coordinates. Deliberately
// checked BEFORE the real colours.dat scheme data (LookupRealColoursDat)
// below -- colours.dat's per-scheme entries are only correct if
// g_currentColourSchemeID happens to match whatever scheme THIS level
// actually uses, which isn't tracked yet (defaults to
// DEFAULT_COLOUR_SCHEME_ID=9 whether or not that's right); scheme 9's own
// "ground_fill"/"player_fill" turned out NOT to match this level when tried
// (gray ground, purple/green player instead of the confirmed dark-brown/
// cream), so pixel-sampled ground truth for the exact names/scenario we
// have it for wins over a guessed scheme ID every time.
bool LookupSampledDefault(const std::string& a_name, float& a_r, float& a_g, float& a_b) {
    static const std::unordered_map<std::string, ColourData> kKnown = {
        {"clear", {0.804f, 0.698f, 0.259f}},    // (205,178,66) sampled background
        {"bg_fill", {0.804f, 0.698f, 0.259f}},
        {"bg_out", {0.165f, 0.129f, 0.059f}},
        {"player_fill", {0.824f, 0.745f, 0.6f}}, // (210,190,153) sampled player body/spawner cup
        {"player_out", {1.0f, 1.0f, 1.0f}},       // (255,255,255) sampled player outline ring
        {"ground_fill", {0.165f, 0.129f, 0.059f}}, // (42,33,15) sampled platform/floor
        {"ground_out", {0.165f, 0.129f, 0.059f}},
        // "ground_outline" -- confirmed real, distinct string from
        // "ground_out" (Platformer/GroundProcessNEW.gm's installNoClimbGround
        // and the diagonal-hatch overlay in drawGrounds() both use this
        // exact spelling, while installNormalGround uses "ground_out") --
        // could be a genuinely different colour in a real scheme, but with
        // no sampled evidence either way, matching ground_out's value is
        // the more defensible default (same visual role: a dark ground
        // outline/border colour) than an arbitrary hash-fallback colour.
        {"ground_outline", {0.165f, 0.129f, 0.059f}},
    };
    auto it = kKnown.find(a_name);
    if (it == kKnown.end()) return false;
    a_r = it->second.r; a_g = it->second.g; a_b = it->second.b;
    return true;
}

// Last-resort guesses for names with neither pixel-sampled ground truth nor
// a real colours.dat hit (e.g. colours.dat failed to load) -- "danger_fill"/
// "danger_out" values are the real "s9_danger_fill"/"s9_danger_out" entries
// (scheme 9 == DEFAULT_COLOUR_SCHEME_ID, Main.gm:113) copied here as a
// backstop.
bool LookupColourSchemeDefault(const std::string& a_name, float& a_r, float& a_g, float& a_b) {
    // ENGINE PATCH (2026-08-17): "fire" (ground_renderer.cpp's DrawFire,
    // the jagged fire-ground edge) used to be a hardcoded flame-orange
    // guess (0.95,0.45,0.1) with, by this file's own prior comment, "no
    // sampled or real-data evidence" -- confirmed WRONG by direct live
    // testing: colours.dat has no "fire"/"sN_fire" entry under any scheme,
    // so every fire-ground edge always rendered that one fixed orange
    // regardless of colour scheme, and it visibly clashed against level
    // shows a thin, muted coral-red dashed accent in roughly this same
    // location -- not a saturated flame orange). "danger_fill" is the
    // real, scheme-aware, colours.dat-backed colour for "hazard/danger"
    // elements elsewhere in this same file (DrawDanger's original
    // reasoning) and visually matches that sampled dashed accent far
    // better -- delegating here (rather than hand-copying its RGB) means
    // "fire" automatically tracks whatever "danger_fill" resolves to per
    // scheme, same as every other name that goes through the real
    // colours.dat lookup, instead of being frozen at one guessed value.
    if (a_name == "fire") return GetNamedColourRGB("danger_fill", a_r, a_g, a_b);

    static const std::unordered_map<std::string, ColourData> kGuessed = {
        {"danger_fill", {0.862745f, 0.345098f, 0.321569f}},
        {"danger_out", {0.431373f, 0.176471f, 0.160784f}},
    };
    auto it = kGuessed.find(a_name);
    if (it != kGuessed.end()) { a_r = it->second.r; a_g = it->second.g; a_b = it->second.b; return true; }
    // Stable fallback: hash the name into a mid-range, never-black colour
    // so different unknown scheme names are at least visually distinct
    // (and never invisible-on-black) instead of all defaulting identically.
    std::hash<std::string> hasher;
    size_t h = hasher(a_name);
    a_r = 0.35f + 0.5f * ((h & 0xFF) / 255.0f);
    a_g = 0.35f + 0.5f * (((h >> 8) & 0xFF) / 255.0f);
    a_b = 0.35f + 0.5f * (((h >> 16) & 0xFF) / 255.0f);
    return true;
}

// ENGINE PATCH (2026-08-16): real, shipped per-scheme colour data. Confirmed
// present at game_data/Platformer/colours.dat -- a real, ~11700-line, plain
// text "name r g b" table Queasy Games left in the corpus, NOT loaded by
// anything in this reimplementation until now (LookupColourSchemeDefault
// above was a hand-picked guess table built before this file's existence was
// noticed). Two real facts confirmed by reading it directly: (1) almost none
// of the bare names real scripts use ("ground_fill", "player_fill",
// "danger_fill", ...) exist unprefixed -- only a scheme-specific "sN_" form
// does (e.g. "s9_ground_fill", "s9_danger_fill") -- confirming colour
// resolution is genuinely scheme-ID-scoped, matching g_currentColourSchemeID
// existing at all; (2) the ONE bare exception, "clear", is (0,0,0) black in
// the base table -- so scheme-prefixed lookup has to come first, or every
// "clear" reference (including the real per-frame background wash,
// Main.gm's preDraw()) would resolve to black instead of a real scheme's
// actual background tone. Lookup order: "s{currentSchemeId}_{name}", then
// bare "{name}", then this file's own state.nameToIndex (script SetColour
// overrides), then the hand-picked LookupColourSchemeDefault table as a
// last resort for names that exist in neither.
std::unordered_map<std::string, ColourData>& RealColoursDatTable() {
    static std::unordered_map<std::string, ColourData> table;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        std::string text;
        FileHelper* fh = GetEngineContext().fileHelper;
        if (fh && fh->ReadTextFile("Platformer/colours.dat", text)) {
            std::istringstream all(text);
            std::string line;
            while (std::getline(all, line)) {
                std::istringstream ls(line);
                std::string name;
                float r, g, b;
                if (ls >> name >> r >> g >> b) {
                    table[name] = ColourData{r, g, b};
                }
            }
            std::fprintf(stderr, "colour_bindings: loaded %zu real colours from Platformer/colours.dat\n", table.size());
        } else {
            std::fprintf(stderr, "colour_bindings: Platformer/colours.dat not found, using guessed defaults only\n");
        }
    }
    return table;
}

bool LookupRealColoursDat(const std::string& a_name, float& a_r, float& a_g, float& a_b) {
    auto& table = RealColoursDatTable();
    auto it = table.find("s" + std::to_string(g_currentColourSchemeID) + "_" + a_name);
    if (it == table.end()) it = table.find(a_name);
    if (it != table.end()) { a_r = it->second.r; a_g = it->second.g; a_b = it->second.b; return true; }
    return false;
}

} // namespace

bool GetNamedColourRGB(const std::string& a_name, float& a_r, float& a_g, float& a_b) {
    // ENGINE PATCH (2026-08-18): the registry check this function used to
    // do first (an exact mirror of gmf_COLOURSCHEME_SetColour's own
    // registry-first check) is gone entirely now, not just bypassed for
    // "clear" -- see that function's header comment for the full, now-
    // generalized diagnosis (live logging proved EVERY plain string colour
    // name gets permanently stuck on whatever scheme was active the first
    // time it was ever looked up, not just "clear"). Every caller that
    // reaches this function with a plain name string wants the CURRENT
    // scheme's real colour, which only the colours.dat-backed lookup below
    // can give reliably; callers that need genuine runtime-registered
    // overrides (a ColourManager colour ID int, an existing COLOUR object)
    // resolve those directly via ResolveColour before ever falling back to
    // this function, so removing this dead-weight (worse: actively
    // harmful) registry check here doesn't lose real functionality.
    // ENGINE PATCH (2026-08-17): real per-scheme colours.dat data now wins
    // over the hardcoded pixel-sampled guesses below. That sampling was done
    // back when g_currentColourSchemeID was believed untracked/stuck at the
    // DEFAULT_COLOUR_SCHEME_ID=9 fallback -- since confirmed WRONG: live
    // logging of gmf_COLOURSCHEME_SetColourScheme showed the real tutorial
    // level genuinely calls COLOURSCHEME_SetColourScheme(8) once gameplay
    // starts (a menu/loading-screen scheme 9/152 call happens first, then
    // 8 for the actual level), and colours.dat's real "s8_ground_fill"
    // (0.890,0.867,0.455 -- khaki/olive) / "s8_bg_fill" (0.978,0.927,0.766
    // -- warm khaki sky) / "s8_creature_fill" (0.443,0.827,0.608 -- teal)
    // match the real retail screenshot exactly, while the old sampled
    // "ground_fill" guess (0.165,0.129,0.059 -- near-black dark brown) was
    // sampled from a run where the scheme was never correctly switched --
    // it was rendering the WRONG scheme's near-black seed data, not a real
    // ground colour, and this file's own guess table then baked that mistake
    // in as if it were ground truth. LookupSampledDefault now only matters
    // as a fallback for names colours.dat doesn't have an entry for at all.
    if (LookupRealColoursDat(a_name, a_r, a_g, a_b)) return true;
    if (LookupSampledDefault(a_name, a_r, a_g, a_b)) return true;
    return LookupColourSchemeDefault(a_name, a_r, a_g, a_b);
}

bool GetInterpolatedNamedColourRGB(float a_amount, const std::string& a_from,
                                   const std::string& a_to,
                                   float& a_r, float& a_g, float& a_b) {
    ColourData from, to;
    if (!GetNamedColourRGB(a_from, from.r, from.g, from.b) ||
        !GetNamedColourRGB(a_to, to.r, to.g, to.b))
        return false;

    const ColourData result = InterpolateHsl(from, to, a_amount);
    a_r = result.r;
    a_g = result.g;
    a_b = result.b;
    return true;
}

namespace {

int GM_CDECL gmf_COLOURSCHEME_SetColour(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    // Lenient read, same reasoning as gmf_DC_scale/gmf_DC_translate:
    // GM_STRING_PARAM expands to the STRICT variant, which throws a
    // blank-message GM_EXCEPTION when the parameter exists but isn't a
    // string -- and that aborts the entire calling draw(), not just this
    // statement. EntityPacks/BrianMover.gm:726 calls
    // `COLOURSCHEME_SetColour(this.fillcn, this.drawList)` every frame with
    // `fillcn` still null on entities whose configure hasn't assigned it,
    // which threw ~1100 times over one tutorial run and discarded all of
    // those entities' remaining draw calls. Falling back to the same "clear"
    // default the strict macro already declared keeps the draw alive.
    const char* name = a_thread->ParamString(0, "clear");
    if (!name) name = "clear";

    ColourData c;
    float r, g, b;
    // ENGINE PATCH (2026-08-18): ANY plain string colour name bypasses the
    // registry and always uses the live, scheme-aware GetNamedColourRGB
    // resolution -- generalized from an earlier same-day fix that only
    // special-cased "clear" (that one's own diagnosis, kept below for the
    // "clear"-specific evidence trail, turned out to be the first
    // discovered instance of a much broader bug: live logging showed EVERY
    // string-keyed colour name resolved through this engine's animation-
    // palette path -- struct_fill, creature_fill, interact_fill, player_out,
    // etc, not just "clear" -- permanently stuck on scheme 9's values
    // despite real gameplay correctly running under scheme 8, because
    // something writes bare (unprefixed) registry entries per-name, early,
    // at whatever scheme is active the first time each name is ever looked
    // up, and nothing in the corpus ever refreshes them afterward). Original
    // "clear" evidence: Main.gm's `this.clearColour=ColourManager.
    // getColour("clear")` (Main.gm:1456) runs exactly ONCE ever, before this
    // level's real COLOURSCHEME_SetColourScheme(8) call has fired, then
    // preDraw() writes that stale value BACK into the registry every frame
    // (`ColourManager.setColour("clear",this.clearColour)`, Main.gm:4331) --
    // sampling that scheme 8's real "clear" (mint-green) was the correct
    // target, not scheme 9's stale pale pink. Registry lookup is preserved
    // for non-string args (a ColourManager colour ID int, or an existing
    // COLOUR object) since those represent real, intentional runtime state
    // that genuinely should round-trip through the registry.
    bool isStringName = a_thread->Param(0).m_type == GM_STRING;
    bool resolved = !isStringName && ResolveColour(a_thread, a_thread->Param(0), c);
    // A resolved-but-exactly-(0,0,0) hit is treated the same as
    // unresolved: nothing in this engine's own known-name table
    // (LookupColourSchemeDefault) is ever pure black (ground_fill/
    // ground_out are deliberately near-black, not exactly 0,0,0), so a
    // real registry hit of exactly (0,0,0) is far more likely a
    // default-constructed/never-actually-set ColourData than a real
    // intentional black background.
    bool isBlack = resolved && c.r == 0.0f && c.g == 0.0f && c.b == 0.0f;
    if (resolved && !isBlack) { r = c.r; g = c.g; b = c.b; }
    else GetNamedColourRGB(name, r, g, b);

    // ENGINE PATCH (2026-08-17): real call shape is
    // `COLOURSCHEME_SetColour(name, drawList)` (SafeVine.gm:286's
    // `COLOURSCHEME_SetColour(this.stemColour,this.drawList);`) -- a bare
    // GLOBAL function taking an explicit drawList as its 2nd positional
    // arg, a THIRD calling convention this file's existing Video-vs-
    // drawList dual-mode split (see draw_list.h's own header comment)
    // doesn't cover: it's not called as `someList.method()` (so
    // RecordingTarget's receiver-based check finds nothing) and it isn't
    // the bare `Video.*` immediate form either (a real drawList target was
    // explicitly passed as data, not implied). Previously always applied
    // immediately (a raw renderer->SetColour call, un-deferred) even when
    // a drawList was passed -- since every actual DRAW call in the same
    // script sequence (drawShape/drawTextureByHandle/...) IS properly
    // deferred (queued, replayed later, right after the whole draw()
    // function finishes recording), the immediate colour mutation applied
    // during RECORDING was long gone by REPLAY TIME, overwritten by
    // whatever ELSE mutated the single shared Renderer::m_colour in
    // between recording and replay -- confirmed live, root cause of
    // DangerVine's petals rendering solid black instead of the correct
    // "danger_fill" red fallback this function computes correctly just
    // above. Now explicitly detects a drawList in param(1) and, if found,
    // pushes the colour-set as a recorded command onto THAT list instead
    // of applying it immediately -- matching every other drawList
    // primitive's own record/replay contract. Falls back to the old
    // immediate behaviour when no drawList is passed (rare/no confirmed
    // real call site, but a safe default).
    float alpha = 1.0f;
    DrawListData* target = nullptr;
    if (a_thread->GetNumParams() >= 2) {
        gmVariable p1 = a_thread->Param(1);
        if (p1.m_type == GetDrawListType()) {
            gmUserObject* obj = (gmUserObject*)a_thread->GetMachine()->GetObject(p1.m_value.m_ref);
            if (obj) target = (DrawListData*)obj->m_user;
        } else {
            alpha = a_thread->ParamFloatOrInt(1, 1.0f);
        }
    }
    if (a_thread->GetNumParams() >= 3) alpha = a_thread->ParamFloatOrInt(2, 1.0f);

    if (target) {
        target->commandBuffer->commands.push_back([r, g, b, alpha] { GetEngineContext().renderer->SetColour(r, g, b, alpha); });
    } else {
        GetEngineContext().renderer->SetColour(r, g, b, alpha);
    }
    return GM_OK;
}

// COLOURSCHEME_SetInterpolatedColour(t, colourName1, colourName2, drawList)
// Real call site (BrianMover.gm:738): `COLOURSCHEME_SetInterpolatedColour(
// mina+(1-mina)*MATH_POW(...), this.fillcn, this.outcn, this.drawList);`
// right alongside two COLOURSCHEME_SetColour(name, drawList) calls in the
// same draw() -- same "bare global taking an explicit drawList" shape as
// SetColour above, and the same drawList-deferred contract applies for the
// same reason (an immediate renderer->SetColour during recording would be
// overwritten by the time the deferred drawShape() commands actually
// replay). Resolves both colour names the same way SetColour does (string
// names bypass the registry -- see that function's header comment for why)
// and blends them in HSL via the same RgbToHsl/HslToRgb path
// ColourManager.makeInterpolatedColour already uses, for the same "shortest
// hue path" reasoning.
int GM_CDECL gmf_COLOURSCHEME_SetInterpolatedColour(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(perc, 0, 0.0f);
    float t = Clamp01(perc);

    auto resolveName = [a_thread](int a_paramIdx, ColourData& a_out) {
        gmVariable v = a_thread->Param(a_paramIdx);
        if (v.m_type == GM_STRING) {
            const char* name = ((gmStringObject*)a_thread->GetMachine()->GetObject(v.m_value.m_ref))->GetString();
            GetNamedColourRGB(name, a_out.r, a_out.g, a_out.b);
        } else if (!ResolveColour(a_thread, v, a_out)) {
            a_out.r = a_out.g = a_out.b = 1.0f;
        }
    };
    ColourData a, b;
    resolveName(1, a);
    resolveName(2, b);

    float ha, sa, la, hb, sb, lb;
    RgbToHsl(a, ha, sa, la);
    RgbToHsl(b, hb, sb, lb);
    float resultH;
    if (std::fabs(hb - ha) >= 0.5f) {
        float adjustedHa = (ha < 0.5f) ? ha + 1.0f : ha - 1.0f;
        resultH = (adjustedHa - hb) * t + hb;
        resultH -= std::floor(resultH);
    } else {
        resultH = (ha - hb) * t + hb;
    }
    float resultS = (sa - sb) * t + sb;
    float resultL = (la - lb) * t + lb;
    ColourData result;
    HslToRgb(resultH, resultS, resultL, result);

    DrawListData* target = nullptr;
    if (a_thread->GetNumParams() >= 4) {
        gmVariable p3 = a_thread->Param(3);
        if (p3.m_type == GetDrawListType()) {
            gmUserObject* obj = (gmUserObject*)a_thread->GetMachine()->GetObject(p3.m_value.m_ref);
            if (obj) target = (DrawListData*)obj->m_user;
        }
    }
    if (!target) target = RecordingTarget(a_thread);
    float r = result.r, g = result.g, bl = result.b;
    if (target) {
        target->commandBuffer->commands.push_back([r, g, bl] { GetEngineContext().renderer->SetColour(r, g, bl, 1.0f); });
    } else {
        GetEngineContext().renderer->SetColour(r, g, bl, 1.0f);
    }
    return GM_OK;
}

// g_currentColourSchemeID / COLOURSCHEME_GetCurrentSchemeID -- this getter
// was entirely missing (not stubbed, not registered at all -- a bare-
// global call to it null-call-leniently returns null every time). Real,
// widespread usage as a per-frame CACHE KEY (e.g. EntityPacks/SafeVine.gm's
// `draw()`: `local sid=COLOURSCHEME_GetCurrentSchemeID(); if
// (?this.drawListSetSID&&this.drawListSetSID==sid) { return; }` --
// rebuilds its drawList geometry once per scheme change, otherwise a cheap
// early-out) -- with the getter always returning null, `?this.
// drawListSetSID` (set to that same null on the previous call) is never
// true, so the early-out never fires and the FULL expensive geometry
// rebuild ran unconditionally every single frame. Confirmed as a real,
// severe perf bug this way: profiling a live tutorial-level run showed
// SafeVine/DangerVine's draw() alone eating 1.2+ seconds out of every 2
// real seconds (dropping the game from a solid 60fps to ~25-30fps),
// entirely from this one missing getter defeating its own cache guard.
// SetColourScheme's own real call sites (Main.gm) always pass a real,
// already-known scheme id (`COLOURSCHEME_SetColourScheme(scr.
// colourSchemeID,force)`), so the getter just needs to echo back the last
// id actually set -- no separate ID-assignment scheme to invent.
// (g_currentColourSchemeID itself now declared near the top of this file,
// initialized to 9 -- see that declaration's comment.)
int GM_CDECL gmf_COLOURSCHEME_SetColourScheme(gmThread* a_thread) {
    if (std::getenv("GE3_DIAG_COL")) {
        static int n = 0;
        if (n++ < 30) std::fprintf(stderr, "[scheme] SetColourScheme(%d)\n",
                                   a_thread->GetNumParams() >= 1 ? (int)a_thread->ParamInt(0, -1) : -1);
    }
    if (a_thread->GetNumParams() >= 1) g_currentColourSchemeID = a_thread->ParamInt(0, 0);
    return GM_OK;
}
int GM_CDECL gmf_COLOURSCHEME_GetCurrentSchemeID(gmThread* a_thread) {
    a_thread->PushInt(g_currentColourSchemeID);
    return GM_OK;
}

// ColourScheme.c 0x001d26f0): reads the arg string, calls an internal
// name->id lookup, returns a GM_INT. Real call site (TouchEditProcessBaseOld
// .gm:1817, editor selected-entity highlight): `local colID=
// COLOURSCHEME_GetCOLOURSCHEMEID("edit_hint"); ... drawList.setColour(colID,
// hintAlpha);` -- so the returned int must round-trip through drawList.
// setColour's own GM_INT dispatch (ResolveColour's by-index branch, see
// above), which is exactly what this file's own name<->index ColourManager
// registry (FindOrAddIndex/state.colours) already provides for
// COLOURSCHEME_SetColour by name. Newly-registered names default to
// (0,0,0); seeded once here from the same real-data/sampled/guessed chain
// GetNamedColourRGB uses (colours.dat's real "s{scheme}_edit_hint" entries
// are, in fact, genuinely (0,0,0) in every scheme -- an editor-only subtle
// darken-overlay by design, not a missing-data placeholder) so the ID's
// backing colour matches what COLOURSCHEME_SetColour("edit_hint",...) would
// have produced instead of silently defaulting to unset black for other
// (non-edit_hint) names that might resolve to something real.
int GM_CDECL gmf_COLOURSCHEME_GetCOLOURSCHEMEID(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(name, 0, "");
    ColourManagerState& state = GetColourManagerState();
    int idx = FindOrAddIndex(name);
    ColourData& c = state.colours[idx].second;
    if (c.r == 0.0f && c.g == 0.0f && c.b == 0.0f) {
        float r, g, b;
        if (GetNamedColourRGB(name, r, g, b)) { c.r = r; c.g = g; c.b = b; }
    }
    a_thread->PushInt(idx);
    return GM_OK;
}

// COLOURSCHEME_SetAnimationPaletteColour(anim, outColourName, fillColourName)
// -- confirmed real (70+ call sites, e.g. EntityPacks/SpawnPoint.gm's
// `COLOURSCHEME_SetAnimationPaletteColour(anim,cn1,cn2)` called via
// ENTITYBASE_DrawAnimationCenteredUsingBottomLeftMethod right before every
// `dl.drawAnimation(anim)`) but entirely unregistered -- every animated
// entity's sprite tint call silently no-op'd, so even once .fsa/.bci
// loading and drawAnimation itself worked, nothing had ever told the
// Animation what colours to render its palette-index frames with (see
// fsa_bci.h's header comment for why the format needs external tinting at
// all). Resolves both names through the exact same ResolveColourRGB path
// COLOURSCHEME_SetColour already uses, so a real colour-scheme name
// resolves identically everywhere it's looked up.
// ENGINE PATCH (2026-08-18): plain string names now bypass the
// ColourManager registry entirely and resolve straight through
// GetNamedColourRGB (live, scheme-aware colours.dat lookup) -- confirmed
// real, systemic bug, not specific to any one name (see
// gmf_COLOURSCHEME_SetColour's "clear" bypass, which turned out to be only
// the FIRST discovered instance of this same class of bug): live logging
// of every (out,fill) name pair passed through this function during actual
// level-1 gameplay showed EVERY one resolving to scheme 9's values
// (e.g. "struct_fill" -> exactly s9_struct_fill' (0.964706,0.670588,
// 0.384314), "creature_fill"/"interact_fill" -> exactly s9_creature_fill/
// s9_interact_fill's shared (0.380392,0.737255,0.803922)) despite the real
// level correctly switching to scheme 8 for actual gameplay (confirmed via
// a separate SetColourScheme trace) -- something writes bare (unprefixed)
// registry entries for colour names generically, early, at whatever scheme
// happens to be active at that point (matching the exact same "Main.gm's
// this.clearColour cached once, too early" timing bug, just for many names
// at once rather than one), and since nothing in the corpus ever refreshes
// those bare entries once the real per-level scheme applies, EVERY string-
// keyed colour name via this path was permanently stuck on whatever scheme
// happened to be active the first time each name was ever looked up --
// confirmed live to be the actual cause of level 1's flower head rendering
// the wrong (scheme 9, not scheme 8) colour. Registry lookup is preserved
// for non-string args (a ColourManager colour ID int, or an existing
// COLOUR object) since those represent real, intentional runtime state
// (e.g. a player-customized colour) that genuinely should round-trip
// through the registry, not colours.dat.
void ResolveAnimColour(gmThread* a_thread, const gmVariable& a_var, float& a_r, float& a_g, float& a_b) {
    if (a_var.m_type == GM_STRING) {
        const char* name = ((gmStringObject*)a_thread->GetMachine()->GetObject(a_var.m_value.m_ref))->GetString();
        if (GetNamedColourRGB(name, a_r, a_g, a_b)) return;
        a_r = a_g = a_b = 1.0f;
        return;
    }
    ColourData c;
    if (ResolveColour(a_thread, a_var, c) && !(c.r == 0.0f && c.g == 0.0f && c.b == 0.0f)) {
        a_r = c.r; a_g = c.g; a_b = c.b;
        return;
    }
    a_r = a_g = a_b = 1.0f;
}

int GM_CDECL gmf_COLOURSCHEME_SetAnimationPaletteColour(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    float outR, outG, outB, fillR, fillG, fillB;
    ResolveAnimColour(a_thread, a_thread->Param(1), outR, outG, outB);
    ResolveAnimColour(a_thread, a_thread->Param(2), fillR, fillG, fillB);
    SetAnimationPaletteColour(*a_thread->GetMachine(), a_thread->Param(0), outR, outG, outB, fillR, fillG, fillB);
    return GM_OK;
}

// COLOURSCHEME_SetGeneratedPaletteColour(texture, colour1Name, colour2Name)
// an argument-marshalling stub calling an undecompiled internal (FUN_
// addresses with no recovered body) -- the actual recolour math on real
// hardware isn't available to read. What IS confirmed: the real static-PNG
// call shape (EntityPacks/SwayingTree.gm:493's `COLOURSCHEME_
// SetGeneratedPaletteColour(this.headTexture,this.colour1,this.colour2)`,
// this.headTexture a plain Video.getTexture(...) handle, not an animation)
// and this engine's own direct pixel inspection of the textures these calls
// actually target (see Texture::isChromaKeyMask/hasOutlineMask's header
// comments): a background chroma key plus one or two further "pure"
// template colours meant to be replaced outright by colour1 (fill) /
// colour2 (outline) at draw time. Same behavioural-evidence methodology
// analogous .fsa/.bci animated case ("not yet proven -- would need actual
// native-body decompilation", still true here). No drawList parameter
// exists to defer onto (unlike COLOURSCHEME_SetColour), so this recolours
// the TEXTURE itself, persistently, via Renderer::SetTextureGeneratedTint
// -- see that function's header comment for why that's immune to the
// record/replay timing bug the drawList-based natives have to work around.
int GM_CDECL gmf_COLOURSCHEME_SetGeneratedPaletteColour(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    gmVariable texVar = a_thread->Param(0);
    float fillR, fillG, fillB, outR, outG, outB;
    ResolveAnimColour(a_thread, a_thread->Param(1), fillR, fillG, fillB);
    ResolveAnimColour(a_thread, a_thread->Param(2), outR, outG, outB);

    unsigned int handle = 0;
    if (texVar.m_type == GM_STRING) {
        const char* path = ((gmStringObject*)a_thread->GetMachine()->GetObject(texVar.m_value.m_ref))->GetString();
        // String call sites name a game-data asset (FlareBuddy.gm passes
        // `Platformer/EntityPacks/FlareBuddy/face.png`). Match Video's
        // texture path handling instead of handing a relative path to the
        // renderer, whose working directory is the build folder.
        auto resolved = GetEngineContext().fileHelper->Resolve(path);
        if (resolved) handle = GetEngineContext().renderer->LoadTexture(*resolved);
    } else {
        // The normal case: a real TEXTURE object -- see texture_bindings.h's
        // header comment for why TEXTURE is its own gm type here. Every
        // other texture-consuming native (drawTextureByHandle,
        // unloadTexture, ...) already goes through ResolveTextureHandle;
        // this one didn't, so it silently no-opped for every real caller
        // (SwayingTree.gm's flower head recolour among them).
        handle = ResolveTextureHandle(*a_thread->GetMachine(), texVar);
    }
    if (handle != 0) {
        // Indexed image files also expose a synthesized subtexture palette
        // in this port. That child is not the argument's identity: retail's
        // generated-palette native distinguishes a normal image from a
        // palette-only TEXTURE (Video.createPalette()). Applying the tint
        // to the image here preserves that distinction and fixes static
        // generated sprites such as SwayingTree's head.
        GetEngineContext().renderer->SetTextureGeneratedTint(
            handle, fillR, fillG, fillB, outR, outG, outB);
        return GM_OK;
    }

    // Video.createPalette() returns a palette-only TEXTURE with no image
    // handle. Retain its generated colours until setTextureSubImg pairs it
    // with an image resource.
    PaletteData* pal = ResolvePaletteData(*a_thread->GetMachine(), texVar);
    if (!pal) return GM_OK;
    SetGeneratedPaletteColours(*pal, fillR, fillG, fillB, outR, outG, outB);
    return GM_OK;
}

gmFunctionEntry s_colourManagerLib[] = {
    {"load", gmf_CM_load, nullptr},
    {"save", gmf_CM_save, nullptr},
    {"getCurrentFilename", gmf_CM_getCurrentFilename, nullptr},
    {"getColour", gmf_CM_getColour, nullptr},
    {"setColour", gmf_CM_setColour, nullptr},
    {"makeInterpolatedColour", gmf_CM_makeInterpolatedColour, nullptr},
    {"getColourID", gmf_CM_getColourID, nullptr},
    {"getColourNames", gmf_CM_getColourNames, nullptr},
    {"removeColour", gmf_CM_removeColour, nullptr},
};

} // namespace

int GM_CDECL DrawList_SetInterpolatedColour(gmThread* a_thread) {
    return gmf_COLOURSCHEME_SetInterpolatedColour(a_thread);
}

// Bytecode verified against VMDRAWLIST.setColourHSL at EBOOT 0x7f420: it
// accepts 1/2 color-reference arguments or 3/4 numeric HSL[A] arguments,
// then records opcode 0x0b with RGB and alpha via 0x3d8e4/0x3ddb4->0x3da60.
// HslToRgb translates the retail PPC operation order and fused operations
// from FUN_000c4c08/FUN_000c4b80.
int GM_CDECL DrawList_SetColourHSL(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    float r, g, b, alpha = 1.0f;
    if (count == 1 || count == 2) {
        if (!ResolveColourRGB(a_thread, a_thread->Param(0), r, g, b))
            return GM_EXCEPTION;
        if (count == 2) {
            const gmVariable& value = a_thread->Param(1);
            alpha = value.m_type == GM_FLOAT
                ? value.m_value.m_float : static_cast<float>(value.m_value.m_int);
        }
    } else if (count == 3 || count == 4) {
        auto scalar = [a_thread](int index) {
            const gmVariable& value = a_thread->Param(index);
            return value.m_type == GM_FLOAT
                ? value.m_value.m_float : static_cast<float>(value.m_value.m_int);
        };
        ColourData converted;
        HslToRgb(scalar(0), scalar(1), scalar(2), converted);
        r = converted.r;
        g = converted.g;
        b = converted.b;
        if (count == 4) alpha = scalar(3);
    } else {
        return GM_EXCEPTION;
    }
    return DC_RecordOrRun(a_thread, [r, g, b, alpha] {
        GetEngineContext().renderer->SetColour(r, g, b, alpha);
    });
}

// Bytecode verified against VMDRAWLIST.setColourWithLightnessShift at
// EBOOT 0x7f6a0. It accepts (shift, colour) or (shift, alpha, colour), where
// colour must be a string name or COLOUR object. Retail converts RGB to HSL,
// adds shift to lightness, clamps lightness to [0,1] with PPC fsel branches,
// converts back to RGB, then appends opcode 0x0b with RGBA through 0x3da60.
// RgbToHsl's arithmetic is still a candidate for instruction-order audit.
int GM_CDECL DrawList_SetColourWithLightnessShift(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 2 && count != 3) return GM_EXCEPTION;

    const gmVariable& colour = a_thread->Param(count - 1);
    if (colour.m_type != GM_STRING && colour.m_type != g_colourType)
        return GM_EXCEPTION;

    auto scalar = [a_thread](int index) {
        const gmVariable& value = a_thread->Param(index);
        return value.m_type == GM_FLOAT
            ? value.m_value.m_float : static_cast<float>(value.m_value.m_int);
    };
    float r, g, b;
    if (!ResolveColourRGB(a_thread, colour, r, g, b)) return GM_EXCEPTION;

    ColourData rgb{r, g, b};
    float h, s, l;
    RgbToHsl(rgb, h, s, l);
    const float shifted = l + scalar(0);
    l = shifted < 0.0f ? 0.0f : shifted;
    l = l > 1.0f ? 1.0f : l;
    ColourData converted;
    HslToRgb(h, s, l, converted);
    const float alpha = count == 3 ? scalar(1) : 1.0f;
    return DC_RecordOrRun(a_thread, [converted, alpha] {
        GetEngineContext().renderer->SetColour(converted.r, converted.g,
                                               converted.b, alpha);
    });
}

bool ResolveColourRGB(gmThread* a_thread, const gmVariable& a_var, float& a_r, float& a_g, float& a_b) {
    ColourData c;
    if (!ResolveColour(a_thread, a_var, c)) return false;
    a_r = c.r; a_g = c.g; a_b = c.b;
    return true;
}

const void* GetColourManagerReferenceByID(int a_id) {
    const ColourManagerState& state = GetColourManagerState();
    if (a_id < 0 || a_id >= static_cast<int>(state.colours.size())) return nullptr;
    return &state.colours[static_cast<size_t>(a_id)].second;
}

const void* GetColourManagerReferenceByName(const std::string& a_name) {
    const ColourManagerState& state = GetColourManagerState();
    const auto it = state.nameToIndex.find(a_name);
    if (it == state.nameToIndex.end()) return nullptr;
    return &state.colours[static_cast<size_t>(it->second)].second;
}

bool ReadColourManagerReference(const void* a_reference,
                                float& a_r, float& a_g, float& a_b) {
    if (!a_reference) return false;
    const auto* colour = static_cast<const ColourData*>(a_reference);
    a_r = colour->r;
    a_g = colour->g;
    a_b = colour->b;
    return true;
}

bool GetColourObjectRGB(gmObject* a_object, float& a_r, float& a_g, float& a_b) {
    if (!a_object || a_object->GetType() != g_colourType) return false;
    const gmUserObject* userObject = static_cast<const gmUserObject*>(a_object);
    const ColourData* colour = static_cast<const ColourData*>(userObject->m_user);
    if (!colour) return false;
    a_r = colour->r; a_g = colour->g; a_b = colour->b;
    return true;
}

void RegisterColourBindings(gmMachine& a_machine) {
    g_colourType = a_machine.CreateUserType("COLOUR");
    a_machine.RegisterTypeLibrary(g_colourType, s_colourLib, sizeof(s_colourLib) / sizeof(s_colourLib[0]));
    a_machine.RegisterUserCallbacks(g_colourType, nullptr, ColourGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("COLOUR", gmf_COLOUR);

    a_machine.RegisterLibrary(s_colourManagerLib, sizeof(s_colourManagerLib) / sizeof(s_colourManagerLib[0]), "ColourManager");

    a_machine.RegisterLibraryFunction("COLOURSCHEME_SetColour", gmf_COLOURSCHEME_SetColour);
    a_machine.RegisterLibraryFunction("COLOURSCHEME_SetInterpolatedColour", gmf_COLOURSCHEME_SetInterpolatedColour);
    a_machine.RegisterLibraryFunction("COLOURSCHEME_GetCOLOURSCHEMEID", gmf_COLOURSCHEME_GetCOLOURSCHEMEID);
    a_machine.RegisterLibraryFunction("COLOURSCHEME_SetColourScheme", gmf_COLOURSCHEME_SetColourScheme);
    a_machine.RegisterLibraryFunction("COLOURSCHEME_GetCurrentSchemeID", gmf_COLOURSCHEME_GetCurrentSchemeID);
    a_machine.RegisterLibraryFunction("COLOURSCHEME_SetAnimationPaletteColour", gmf_COLOURSCHEME_SetAnimationPaletteColour);
    a_machine.RegisterLibraryFunction("COLOURSCHEME_SetGeneratedPaletteColour", gmf_COLOURSCHEME_SetGeneratedPaletteColour);
}

int GetCurrentColourSchemeID() { return g_currentColourSchemeID; }
