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

int g_currentColourSchemeID = 9;

struct ColourData {
    float r = 0.0f, g = 0.0f, b = 0.0f;
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

float Brightness(const ColourData& c) {
    const float kNorm = 1.0f / 1.7320508f; // 1/sqrt(3), see comment above
    return Clamp01(GrayscaleMagnitude(c) * kNorm);
}

int GM_CDECL gmf_getBrightness(gmThread* a_thread) {
    ColourData* c = ThisColour(a_thread); if (!c) return GM_EXCEPTION;
    a_thread->PushFloat(Brightness(*c));
    return GM_OK;
}

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

int GM_CDECL gmf_CM_makeInterpolatedColour(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n < 3 || n > 4) { GM_EXCEPTION_MSG("ColourManager.makeInterpolatedColour: expecting 3-4 params"); return GM_EXCEPTION; }
    GM_FLOAT_OR_INT_PARAM(perc, 0, 0.0f);
    ColourData a, b;
    if (!ResolveColour(a_thread, a_thread->Param(1), a) || !ResolveColour(a_thread, a_thread->Param(2), b)) {
        GM_EXCEPTION_MSG("ColourManager.makeInterpolatedColour: param 1/2 isn't a resolvable colour");
        return GM_EXCEPTION;
    }
    ColourData result = InterpolateHsl(a, b, perc);

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
    a_thread->PushInt(FindOrAddIndex(name));
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

bool LookupSampledDefault(const std::string& a_name, float& a_r, float& a_g, float& a_b) {
    static const std::unordered_map<std::string, ColourData> kKnown = {
        {"clear", {0.804f, 0.698f, 0.259f}},    // (205,178,66) sampled background
        {"bg_fill", {0.804f, 0.698f, 0.259f}},
        {"bg_out", {0.165f, 0.129f, 0.059f}},
        {"player_fill", {0.824f, 0.745f, 0.6f}}, // (210,190,153) sampled player body/spawner cup
        {"player_out", {1.0f, 1.0f, 1.0f}},       // (255,255,255) sampled player outline ring
        {"ground_fill", {0.165f, 0.129f, 0.059f}}, // (42,33,15) sampled platform/floor
        {"ground_out", {0.165f, 0.129f, 0.059f}},
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

    float alpha = 1.0f;
    DrawListData* target = nullptr;
    auto readTarget = [a_thread](int index) -> DrawListData* {
        const gmVariable& value = a_thread->Param(index);
        if (value.m_type != GetDrawListType()) return nullptr;
        gmUserObject* object = static_cast<gmUserObject*>(
            a_thread->GetMachine()->GetObject(value.m_value.m_ref));
        return object ? static_cast<DrawListData*>(object->m_user) : nullptr;
    };

    // Retail call sites use both (name, drawList) and
    // (name, alpha, drawList). The latter is used by Main.gm's win flash and
    // several level fades. Treating arg 2 as alpha when it is a drawList
    // object both leaks a bogus alpha into the renderer and fails to record
    // the colour command; the later deferred clear then replays with the
    // colour left by an unrelated draw list. Resolve the optional destination
    // first, and only read numeric alpha from the other optional argument.
    if (a_thread->GetNumParams() >= 2) {
        target = readTarget(1);
        if (!target) alpha = a_thread->ParamFloatOrInt(1, 1.0f);
    }
    if (a_thread->GetNumParams() >= 3) {
        DrawListData* trailingTarget = readTarget(2);
        if (trailingTarget) target = trailingTarget;
        else alpha = a_thread->ParamFloatOrInt(2, 1.0f);
    }

    // Paired with GE3_TRACE_SWAYING_TREE_COLOUR in shape_bindings.cpp: this
    // records which scheme and resolved bg_out value were queued for the
    // stem, without modifying the deferred draw-list state.
    if (std::strcmp(name, "bg_out") == 0 &&
        std::getenv("GE3_TRACE_SWAYING_TREE_COLOUR")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 64) {
            std::fprintf(stderr,
                "[sway-tree-colour] record scheme=%d rgb=(%.4f,%.4f,%.4f) alpha=%.4f target=%d commands=%zu\n",
                g_currentColourSchemeID, r, g, b, alpha, target != nullptr,
                target && target->commandBuffer
                    ? target->commandBuffer->commands.size() : 0u);
        }
    }

    if (target) {
        target->commandBuffer->commands.push_back(
            [r, g, b, alpha] { GetEngineContext().renderer->SetColour(r, g, b, alpha); });
    } else {
        GetEngineContext().renderer->SetColour(r, g, b, alpha);
    }
    return GM_OK;
}

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

    const ColourData result = InterpolateHsl(a, b, t);

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
