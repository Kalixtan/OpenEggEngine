#include "draw_context.h"
#include "draw_list.h"
#include "../core/engine_context.h"
#include "../core/system_config.h"
#include "renderer.h"
#include "../core/save_data.h"
#include "../core/strict_calls.h"
#include "../bindings/colour_bindings.h"
#include "../bindings/texture_bindings.h"
#include "../bindings/video_bindings.h"
#include "../bindings/vector2d_bindings.h"
#include "../bindings/matrix4_bindings.h"
#include "../bindings/shape_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <functional>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>

DrawContext& GetDrawContext() {
    static DrawContext ctx;
    return ctx;
}

int DC_RecordOrRun(gmThread* a_thread, std::function<void()> a_apply) {
    if (DrawListData* rec = RecordingTarget(a_thread)) { rec->commandBuffer->commands.push_back(std::move(a_apply)); return GM_OK; }
    a_apply();
    return GM_OK;
}

static std::optional<std::string> ResolveDrawTexturePath(const char* a_path, int a_area) {
    EngineContext& ctx = GetEngineContext();
    if (a_area == 1 && ctx.fileHelper) return ResolveTextureAssetPath(a_path, a_area);
    std::string fullPath;
    if (a_area == 0) fullPath = SaveData::Dir() + "/" + a_path;
    else if (a_area == 2) fullPath = a_path;
    else return std::nullopt;
    std::error_code ec;
    if (!std::filesystem::is_regular_file(std::filesystem::u8path(fullPath), ec) || ec) return std::nullopt;
    return fullPath;
}

static double RetailFloatOrIntAsDouble(gmThread* a_thread, int a_index) {
    const gmVariable& value = a_thread->Param(a_index);
    return value.m_type == GM_FLOAT
        ? static_cast<double>(value.m_value.m_float)
        : static_cast<double>(value.m_value.m_int);
}

static float RetailSmoothLevelFromArgs(double a_first, double a_second) {
    return static_cast<float>(a_second + std::fabs(a_first)) * 0.5f;
}

int GM_CDECL DC_gmf_pushAttributes(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        ctx.attrStack.push_back({ctx.depthTest, ctx.depthWrite, ctx.colourMask,
                                  ctx.stencilFunc, ctx.stencilRef,
                                  ctx.stencilSfail, ctx.stencilDpfail, ctx.stencilDppass,
                                  ctx.culling});
    });
}

int GM_CDECL DC_gmf_popAttributes(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        if (ctx.attrStack.empty()) return; // unbalanced pop -- leniently ignored, matches this codebase's convention
        DrawContext::Attrs a = ctx.attrStack.back();
        ctx.attrStack.pop_back();

        ctx.depthTest = a.depthTest;
        ctx.depthWrite = a.depthWrite;
        ctx.colourMask = a.colourMask;
        ctx.stencilFunc = a.stencilFunc;
        ctx.stencilRef = a.stencilRef;
        ctx.stencilSfail = a.stencilSfail;
        ctx.stencilDpfail = a.stencilDpfail;
        ctx.stencilDppass = a.stencilDppass;
        ctx.culling = a.culling;

        Renderer* renderer = GetEngineContext().renderer;
        renderer->SetDepthTest(ctx.depthTest);
        renderer->SetDepthWrite(ctx.depthWrite);
        renderer->SetColourMask(ctx.colourMask);
        renderer->SetStencilFunc(ctx.stencilFunc, ctx.stencilRef);
        renderer->SetStencilOp(ctx.stencilSfail, ctx.stencilDpfail, ctx.stencilDppass);
        renderer->SetCulling(ctx.culling);
    });
}

int GM_CDECL DC_gmf_setAttrDepthTest(gmThread* a_thread) {
    GM_INT_PARAM(enabled, 0, 0);
    return DC_RecordOrRun(a_thread, [enabled] {
        DrawContext& ctx = GetDrawContext();
        ctx.depthTest = (enabled != 0);
        GetEngineContext().renderer->SetDepthTest(ctx.depthTest);
    });
}

int GM_CDECL DC_gmf_setAttrDepthWrite(gmThread* a_thread) {
    GM_INT_PARAM(enabled, 0, 1);
    return DC_RecordOrRun(a_thread, [enabled] {
        DrawContext& ctx = GetDrawContext();
        ctx.depthWrite = (enabled != 0);
        GetEngineContext().renderer->SetDepthWrite(ctx.depthWrite);
    });
}

int GM_CDECL DC_gmf_setAttrCulling(gmThread* a_thread) {
    GM_INT_PARAM(mode, 0, 0);
    return DC_RecordOrRun(a_thread, [mode] {
        DrawContext& ctx = GetDrawContext();
        ctx.culling = mode;
        GetEngineContext().renderer->SetCulling(ctx.culling);
    });
}

int GM_CDECL DC_gmf_setOrderTransparent(gmThread* a_thread) {
    DrawListData* list = RecordingTarget(a_thread);
    if (!list) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(z, 0, 0.0f);
    const int quantized = std::clamp((int)(z * 33.333335876464844f) + 0xff00, 0, 0xffff);
    list->orderKey = 0x30000000u | (unsigned int)quantized;
    return GM_OK;
}

int GM_CDECL DC_gmf_setOrderAlphaTest(gmThread* a_thread) {
    DrawListData* list = RecordingTarget(a_thread);
    if (!list) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(z, 0, 0.0f);
    const int quantized = std::clamp(0xff - (int)(z * 33.333335876464844f), 0, 0xffff);
    list->orderKey = 0x20000000u | (unsigned int)quantized;
    return GM_OK;
}

int GM_CDECL DC_gmf_setOrderLast(gmThread* a_thread) {
    DrawListData* list = RecordingTarget(a_thread);
    if (!list) return GM_EXCEPTION;
    list->orderKey = (list->orderKey & 0xcfffffffu) | 0x30000000u;
    return GM_OK;
}

int GM_CDECL DC_gmf_setDistanceTextureRenderAttributes(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const double nearValue = RetailFloatOrIntAsDouble(a_thread, 0);
    const double farValue = RetailFloatOrIntAsDouble(a_thread, 1);
    const double retailFirst = -nearValue;
    const float discardLevel = retailFirst < 0.0 ? 0.25f : 0.5f;
    const float smoothLevel = RetailSmoothLevelFromArgs(retailFirst, farValue);
    return DC_RecordOrRun(a_thread, [discardLevel, smoothLevel] {
        DrawContext& ctx = GetDrawContext();
        ctx.distanceTextureEnabled = true;
        ctx.distanceTextureDiscardLevel = discardLevel;
        ctx.distanceTextureSmoothLevel = smoothLevel;
    });
}

int GM_CDECL DC_gmf_setColourMask(gmThread* a_thread) {
    GM_INT_PARAM(enabled, 0, 1);
    return DC_RecordOrRun(a_thread, [enabled] {
        DrawContext& ctx = GetDrawContext();
        ctx.colourMask = (enabled != 0);
        GetEngineContext().renderer->SetColourMask(ctx.colourMask);
    });
}

int GM_CDECL DC_gmf_setAttrStencil(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(func, 0, 8 /* VIDEOPARAMS_DISABLE */);
    int sfail = 100, dpfail = 100, dppass, ref;
    if (n >= 5) {
        GM_INT_PARAM(sfail_, 1, 100); GM_INT_PARAM(dpfail_, 2, 100);
        GM_INT_PARAM(dppass_, 3, 100); GM_INT_PARAM(ref_, 4, 0);
        sfail = sfail_; dpfail = dpfail_; dppass = dppass_; ref = ref_;
    } else if (n == 3) {
        // Single op argument -> all three op fields (see header comment).
        GM_INT_PARAM(op_, 1, 100); GM_INT_PARAM(ref_, 2, 0);
        sfail = dpfail = dppass = op_; ref = ref_;
    } else {
        // Retail returns -1 for any count other than 3 or 5.
        return GM_EXCEPTION;
    }
    return DC_RecordOrRun(a_thread, [func, sfail, dpfail, dppass, ref] {
        DrawContext& ctx = GetDrawContext();
        ctx.stencilFunc = func; ctx.stencilRef = ref;
        ctx.stencilSfail = sfail; ctx.stencilDpfail = dpfail; ctx.stencilDppass = dppass;
        Renderer* renderer = GetEngineContext().renderer;
        renderer->SetStencilFunc(ctx.stencilFunc, ctx.stencilRef);
        renderer->SetStencilOp(ctx.stencilSfail, ctx.stencilDpfail, ctx.stencilDppass);
    });
}

int GM_CDECL DC_gmf_setAttrDefaultStencil(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        ctx.stencilFunc = 8; ctx.stencilRef = 0;
        ctx.stencilSfail = ctx.stencilDpfail = ctx.stencilDppass = 100;
        GetEngineContext().renderer->SetDefaultStencil();
    });
}

static float RetailSetColourScalar(gmThread* a_thread, int a_index) {
    const gmVariable& value = a_thread->Param(a_index);
    return value.m_type == GM_FLOAT
        ? value.m_value.m_float
        : static_cast<float>(value.m_value.m_int);
}

int GM_CDECL gmf_DC_setColour_drawList(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    float r, g, b, alpha = 1.0f;
    if (count == 1 || count == 2) {
        if (!ResolveColourRGB(a_thread, a_thread->Param(0), r, g, b))
            return GM_EXCEPTION;
        if (count == 2) alpha = RetailSetColourScalar(a_thread, 1);
    } else if (count == 3 || count == 4) {
        r = RetailSetColourScalar(a_thread, 0);
        g = RetailSetColourScalar(a_thread, 1);
        b = RetailSetColourScalar(a_thread, 2);
        if (count == 4) alpha = RetailSetColourScalar(a_thread, 3);
    } else {
        return GM_EXCEPTION;
    }
    return DC_RecordOrRun(a_thread, [r, g, b, alpha] {
        GetEngineContext().renderer->SetColour(r, g, b, alpha);
    });
}
// Verification: DrawList.setColourRef wrapper 0x7ebac accepts exactly 1/2
// args. Arg 0 resolves a live int-ID, string-name, or COLOUR reference; arg 1
// when present becomes alpha (float payload, otherwise signed integer payload).
// Each valid call appends opcode 0x0c with {RGB pointer, alpha}; replay reads
// the referenced RGB then. COLOUR references are rooted on the drawList until
// reset/destruction so their data remains live for every replay.
int GM_CDECL gmf_DC_setColourRef(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 2) return GM_EXCEPTION;

    const gmVariable& colourRef = a_thread->Param(0);
    const float alpha = count == 2 ? RetailSetColourScalar(a_thread, 1) : 1.0f;
    DrawListData* target = RecordingTarget(a_thread);
    if (!target) return GM_EXCEPTION;

    if (colourRef.m_type == GM_INT) {
        const int id = colourRef.m_value.m_int;
        const void* reference = GetColourManagerReferenceByID(id);
        if (!reference) return GM_EXCEPTION;
        target->commandBuffer->commands.emplace_back([reference, alpha] {
            float r, g, b;
            if (ReadColourManagerReference(reference, r, g, b))
                GetEngineContext().renderer->SetColour(r, g, b, alpha);
        });
        return GM_OK;
    }

    if (colourRef.m_type == GM_STRING) {
        auto* stringObject = static_cast<gmStringObject*>(
            a_thread->GetMachine()->GetObject(colourRef.m_value.m_ref));
        if (!stringObject) return GM_EXCEPTION;
        const std::string name = stringObject->GetString();
        const void* reference = GetColourManagerReferenceByName(name);
        if (!reference) return GM_EXCEPTION;
        target->commandBuffer->commands.emplace_back([reference, alpha] {
            float r, g, b;
            if (ReadColourManagerReference(reference, r, g, b))
                GetEngineContext().renderer->SetColour(r, g, b, alpha);
        });
        return GM_OK;
    }

    if (colourRef.m_type == g_colourType) {
        gmObject* object = a_thread->GetMachine()->GetObject(colourRef.m_value.m_ref);
        float r, g, b;
        if (!GetColourObjectRGB(object, r, g, b)) return GM_EXCEPTION;
        target->commandBuffer->commands.emplace_back([object, alpha] {
            float r, g, b;
            if (GetColourObjectRGB(object, r, g, b))
                GetEngineContext().renderer->SetColour(r, g, b, alpha);
        });
        target->commandBuffer->retainedObjects.push_back(object);
        return GM_OK;
    }

    return GM_EXCEPTION;
}

int GM_CDECL DC_gmf_setFontRenderAttributes(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const double edge = RetailFloatOrIntAsDouble(a_thread, 0);
    const double width = RetailFloatOrIntAsDouble(a_thread, 1);
    const float storedEdge = edge < 0.0 ? 0.25f : 0.5f;
    const float storedWidth = RetailSmoothLevelFromArgs(edge, width);
    return DC_RecordOrRun(a_thread, [storedEdge, storedWidth] {
        DrawContext& ctx = GetDrawContext();
        ctx.fontEdge = storedEdge;
        ctx.fontWidth = storedWidth;
    });
}

int GM_CDECL DC_gmf_resetFontRenderAttributes(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        ctx.fontEdge = 0.5f;
        ctx.fontWidth = 0.45f;
        ctx.distanceTextureEnabled = false;
        ctx.distanceTextureDiscardLevel = 0.5f;
        ctx.distanceTextureSmoothLevel = 0.45f;
    });
}

namespace {

std::string ResolveFormatArg(gmMachine* a_machine, const gmVariable& a_arg) {
    char buf[256];
    if (a_arg.m_type == GM_STRING) {
        return a_arg.AsString(a_machine, buf, sizeof(buf));
    }
    if (a_arg.m_type == GM_INT || a_arg.m_type == GM_FLOAT) {
        return a_arg.AsString(a_machine, buf, sizeof(buf));
    }
    if (a_arg.m_type == GM_TABLE) {
        gmTableObject* t = (gmTableObject*)a_machine->GetObject(a_arg.m_value.m_ref);
        gmVariable inner = t->Get(a_machine, "untranslated");
        if (inner.m_type != GM_STRING) inner = t->Get((gmint)0);
        if (inner.m_type == GM_STRING) return inner.AsString(a_machine, buf, sizeof(buf));
        if (inner.m_type == GM_INT || inner.m_type == GM_FLOAT) return inner.AsString(a_machine, buf, sizeof(buf));
    }
    return std::string();
}

std::string SubstituteFormatArgs(const std::string& a_fmt, const std::vector<std::string>& a_args) {
    std::string out;
    out.reserve(a_fmt.size());
    for (size_t i = 0; i < a_fmt.size(); ++i) {
        if (a_fmt[i] == '%' && i + 1 < a_fmt.size() && a_fmt[i + 1] >= '1' && a_fmt[i + 1] <= '9') {
            size_t argIdx = (size_t)(a_fmt[i + 1] - '1');
            if (argIdx < a_args.size()) {
                out += a_args[argIdx];
                ++i;
                continue;
            }
        }
        out += a_fmt[i];
    }
    return out;
}

std::vector<std::string> WrapText(const BitmapFont& a_font, const std::string& a_text, float a_scale, float a_wrapWidth) {
    std::vector<std::string> lines;
    std::string line, word;
    auto commitWord = [&]() {
        if (word.empty()) return;
        std::string trial = line.empty() ? word : line + " " + word;
        if (!line.empty() && a_font.MeasureWidth(trial, a_scale) > a_wrapWidth) {
            lines.push_back(line);
            line = word;
        } else {
            line = trial;
        }
        word.clear();
    };
    for (char c : a_text) {
        if (c == '\n') { commitWord(); lines.push_back(line); line.clear(); }
        else if (c == ' ') commitWord();
        else word += c;
    }
    commitWord();
    if (!line.empty() || lines.empty()) lines.push_back(line);
    return lines;
}

int PrintImpl(gmThread* a_thread, bool a_translate) {
    int n = a_thread->GetNumParams();
    GM_CHECK_NUM_PARAMS(4);

    auto isPosition = [](const gmVariable& v) { return v.m_type == GM_INT || v.m_type == GM_FLOAT; };
    int numLeading = 0;
    while (numLeading < n && isPosition(a_thread->Param(numLeading))) ++numLeading;
    if (numLeading < 2 || numLeading > 4 || numLeading >= n) {
        GM_EXCEPTION_MSG("print: expecting a string text argument");
        return GM_EXCEPTION;
    }

    int idx = 0;
    float leftOrX = a_thread->ParamFloatOrInt(idx++, 0.0f);
    bool hasBounds = (numLeading == 4);
    float rightBound = 0.0f;
    if (hasBounds) rightBound = a_thread->ParamFloatOrInt(idx++, 0.0f);
    float y = a_thread->ParamFloatOrInt(idx++, 0.0f);
    float localZ = 0.0f;
    if (numLeading >= 3) localZ = a_thread->ParamFloatOrInt(idx++, 0.0f);

    int textIdx = idx++;
    std::string text;
    const gmVariable& textParam = a_thread->Param(textIdx);
    if (textParam.m_type == GM_STRING) {
        char textBuf[512];
        text = textParam.AsString(a_thread->GetMachine(), textBuf, sizeof(textBuf));
    } else if (textParam.m_type == GM_TABLE) {
        gmTableObject* textTable = (gmTableObject*)a_thread->GetMachine()->GetObject(textParam.m_value.m_ref);
        gmVariable inner = textTable->Get(a_thread->GetMachine(), "untranslated");
        bool isUntranslatedWrapper = (inner.m_type == GM_STRING);
        if (!isUntranslatedWrapper) inner = textTable->Get((gmint)0);
        if (inner.m_type == GM_STRING) {
            char textBuf[512];
            text = inner.AsString(a_thread->GetMachine(), textBuf, sizeof(textBuf));
        }
        // {fmt, arg1, arg2, ...} substitution table -- see
        // SubstituteFormatArgs's header comment. Only the bare-positional
        // form carries args this way (an {untranslated=str} wrapper is
        // always a single resolved value, never a format string), and
        // only when there's actually a %N in the text to fill.
        if (!isUntranslatedWrapper && text.find('%') != std::string::npos) {
            std::vector<std::string> args;
            for (gmint i = 1; ; ++i) {
                gmVariable argVar = textTable->Get(i);
                if (argVar.m_type == GM_NULL) break;
                args.push_back(ResolveFormatArg(a_thread->GetMachine(), argVar));
            }
            if (!args.empty()) text = SubstituteFormatArgs(text, args);
        }
    } else {
        GM_EXCEPTION_MSG("print: expecting a string text argument");
        return GM_EXCEPTION;
    }
    float scale = (idx < n) ? a_thread->ParamFloatOrInt(idx++, 1.0f) : 1.0f;
    int align = (idx < n) ? a_thread->ParamInt(idx++, 0) : 0;
    gmVariable printInfo = (idx < n) ? a_thread->Param(idx++) : gmVariable::s_null;

    // PRINTINFO.shouldDraw=false is the script API's measurement-only mode
    // (e.g. DrawBubble measures its button caption before positioning it).
    // Keep text metrics and the PRINTINFO out-parameters below, but do not
    // enqueue glyphs for this pass. Without this check, the measuring print
    // leaks a duplicate button icon/text at the current matrix origin.
    bool shouldDrawText = true;
    if (printInfo.m_type == GM_TABLE) {
        gmTableObject* table = static_cast<gmTableObject*>(
            a_thread->GetMachine()->GetObject(printInfo.m_value.m_ref));
        const gmVariable drawFlag = table->Get(a_thread->GetMachine(), "shouldDraw");
        if (drawFlag.m_type == GM_INT && drawFlag.m_value.m_int == 0) {
            shouldDrawText = false;
        } else if (drawFlag.m_type == GM_FLOAT && drawFlag.m_value.m_float == 0.0f) {
            shouldDrawText = false;
        }
    }

    // Resolve the (left, right, y, z) bounds form into a single anchor x
    // now that align is known -- see this function's leading-numerics
    // comment above. The 2/3-numeric direct (x,[y],[z]) forms already hand
    // us the correct alignment-relative anchor as-is (the caller picked x
    // themselves), so only the 4-numeric bounds form needs this.
    float x = leftOrX;
    float wrapWidth = 0.0f;
    bool hasWrapWidth = hasBounds;
    if (hasBounds) {
        wrapWidth = rightBound - leftOrX;
        if (align == 2) x = (leftOrX + rightBound) * 0.5f;      // TEXTALIGN_CENTER
        else if (align == 1) x = rightBound;                     // TEXTALIGN_RIGHT
        // TEXTALIGN_LEFT (0) or unrecognised: x stays leftOrX, already correct.
    }

    // x/y stay in LOCAL (pre-transform) space here -- BitmapFont::DrawText
    // now does the pushMatrix/translate/rotate/scale transform itself, per
    // glyph corner, so text drawn inside a rotateZ() block actually
    // rotates instead of only its anchor point moving (see DrawText's
    // header comment; this used to call DC_TransformPoint once here and
    // hand DrawText an already-world-space x,y, which is exactly the
    // "anchor moves, glyphs stay axis-aligned" bug). z is unaffected by
    // rotation -- still accumulated from the transform stack the same way
    // DC_gmf_translate does. Read at DRAW time (inside the deferred lambda
    // below), not here -- see this function's DC_RecordOrRun call for why:
    // a recorded print() must see whatever transform is ambient when it's
    // actually replayed, not whatever it was when the list was built.

    constexpr float kLineSpacing = 1.0f; // see WrapText's header comment
    float heightConsumed = 0.0f;

    EngineContext& engineCtx = GetEngineContext();
    if (engineCtx.currentFont) {
        const std::string& drawn = (a_translate && engineCtx.localization)
            ? engineCtx.localization->Translate(text)
            : std::string(text);

        std::vector<std::string> lines = (hasWrapWidth && wrapWidth > 0.0f)
            ? WrapText(*engineCtx.currentFont, drawn, scale, wrapWidth)
            : std::vector<std::string>{drawn};
        heightConsumed = (float)lines.size() * scale * kLineSpacing;

        // The actual glyph draws -- deferred if this print() was called on
        // a real drawList instance (see DC_RecordOrRun), so a cached
        // sub-list's text redraws every time it's replayed instead of only
        // once at build time.
        BitmapFont* font = engineCtx.currentFont;
        Renderer* renderer = engineCtx.renderer;
        if (shouldDrawText) {
            DC_RecordOrRun(a_thread, [font, renderer, lines, x, y, localZ, scale, align] {
                DrawContext& ctx2 = GetDrawContext();
                const DrawContext::Mat& top = ctx2.Top();
                float z = top.z + localZ;
                // GE3_TEXT_DIAG=1 -- report every glyph draw with the world
                // position it actually lands at. Diagnostic for "no text on
                // screen": distinguishes "print never runs" from "print runs
                // but draws off-camera" (e.g. an entity left at the world
                // origin), which look identical on a screenshot.
                if (getenv("GE3_TEXT_DIAG")) {
                    float wx, wy;
                    DC_TransformPoint(top, x, y, wx, wy);
                    Renderer* r = GetEngineContext().renderer;
                    std::fprintf(stderr, "[text] '%s' world(%.2f,%.2f) z=%.2f scale=%.3f cam(%.2f,%.2f)\n",
                                 lines.empty() ? "" : lines[0].c_str(), wx, wy, z, scale,
                                 r ? r->GetCameraX() : 0.0f, r ? r->GetCameraY() : 0.0f);
                }
                float lineY = y;
                for (const std::string& line : lines) {
                    font->DrawText(*renderer, line, x, lineY, scale, align, ctx2.fontEdge, ctx2.fontWidth,
                                    top.a, top.b, top.c, top.d, top.e, top.f, z);
                    lineY -= scale * kLineSpacing; // LOCAL space: each subsequent line sits lower before rotation is applied
                }
            });
        }

        float maxHalfWidth = 0.0f;
        for (const std::string& line : lines) {
            maxHalfWidth = std::max(maxHalfWidth, font->MeasureWidth(line, scale) * 0.5f);
        }

        // PRINTINFO out-param: approximate bounding box (top/bottom/left/
        // right) spanning every drawn line -- real callers use it purely
        // for layout math (e.g. `this.titleHeight += pInfo.top - pInfo.bottom;`),
        // not pixel-exact bounds, so a scale-derived box is enough. See
        // printinfo_bindings.h.
        if (printInfo.m_type == GM_TABLE) {
            gmTableObject* table = (gmTableObject*)a_thread->GetMachine()->GetObject(printInfo.m_value.m_ref);
            float halfWidth = maxHalfWidth;
            float left = x, right = x;
            if (align == 2 /* TEXTALIGN_CENTER, see constants_bindings.cpp */) { left = x - halfWidth; right = x + halfWidth; }
            else if (align == 1 /* TEXTALIGN_RIGHT */) { left = x - halfWidth * 2.0f; right = x; }
            else { left = x; right = x + halfWidth * 2.0f; }
            // Equivalent to the old code's `lineY + scale*kLineSpacing -
            // scale*0.5f` read right after the draw loop exited (lineY there
            // == y - lines.size()*spacing) -- computed directly here since
            // that loop is now deferred (see the DC_RecordOrRun call above).
            float bottomOfLastLine = y - (float)(lines.size() - 1) * scale * kLineSpacing - scale * 0.5f;
            table->Set(a_thread->GetMachine(), "top", gmVariable(y + scale * 0.5f));
            table->Set(a_thread->GetMachine(), "bottom", gmVariable(bottomOfLastLine));
            table->Set(a_thread->GetMachine(), "left", gmVariable(left));
            table->Set(a_thread->GetMachine(), "right", gmVariable(right));
            table->Set(a_thread->GetMachine(), "numLines", gmVariable((gmint)lines.size()));
        }
    }
    a_thread->PushFloat(heightConsumed);
    return GM_OK;
}

int PrintArrayImpl(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    GM_CHECK_NUM_PARAMS(5);

    auto isPosition = [](const gmVariable& v) { return v.m_type == GM_INT || v.m_type == GM_FLOAT; };
    int numLeading = 0;
    while (numLeading < n && isPosition(a_thread->Param(numLeading))) ++numLeading;
    if (numLeading < 2 || numLeading > 4 || numLeading >= n) {
        GM_EXCEPTION_MSG("printArray: expecting a table argument after the leading position numbers");
        return GM_EXCEPTION;
    }

    int idx = 0;
    float leftOrX = a_thread->ParamFloatOrInt(idx++, 0.0f);
    bool hasBounds = (numLeading == 4);
    float rightBound = 0.0f;
    if (hasBounds) rightBound = a_thread->ParamFloatOrInt(idx++, 0.0f);
    float y = a_thread->ParamFloatOrInt(idx++, 0.0f);
    float localZ = 0.0f;
    if (numLeading >= 3) localZ = a_thread->ParamFloatOrInt(idx++, 0.0f);

    gmVariable arrayParam = a_thread->Param(idx++);
    std::vector<std::string> lines;
    if (arrayParam.m_type == GM_TABLE) {
        gmTableObject* arrTable = (gmTableObject*)a_thread->GetMachine()->GetObject(arrayParam.m_value.m_ref);
        for (gmint i = 0; ; ++i) {
            gmVariable elem = arrTable->Get(i);
            if (elem.m_type == GM_NULL) break;
            if (elem.m_type == GM_STRING) {
                char buf[256];
                lines.push_back(elem.AsString(a_thread->GetMachine(), buf, sizeof(buf)));
            } else {
                lines.push_back(std::string());
            }
        }
    }

    float scale = (idx < n) ? a_thread->ParamFloatOrInt(idx++, 1.0f) : 1.0f;
    int align = (idx < n) ? a_thread->ParamInt(idx++, 0) : 0;
    gmVariable printInfo = (idx < n) ? a_thread->Param(idx++) : gmVariable::s_null;

    float x = leftOrX;
    if (hasBounds) {
        if (align == 2) x = (leftOrX + rightBound) * 0.5f;
        else if (align == 1) x = rightBound;
    }

    constexpr float kLineSpacing = 1.15f;
    float heightConsumed = (float)lines.size() * scale * kLineSpacing;

    EngineContext& engineCtx = GetEngineContext();
    if (engineCtx.currentFont && !lines.empty()) {
        BitmapFont* font = engineCtx.currentFont;
        Renderer* renderer = engineCtx.renderer;
        DC_RecordOrRun(a_thread, [font, renderer, lines, x, y, localZ, scale, align] {
            DrawContext& ctx2 = GetDrawContext();
            const DrawContext::Mat& top = ctx2.Top();
            float z = top.z + localZ;
            // GE3_TEXT_DIAG=1 -- report every glyph draw with the world
            // position it actually lands at. Diagnostic for "no text on
            // screen": distinguishes "print never runs" from "print runs
            // but draws off-camera" (e.g. an entity left at the world
            // origin), which look identical on a screenshot.
            if (getenv("GE3_TEXT_DIAG")) {
                float wx, wy;
                DC_TransformPoint(top, x, y, wx, wy);
                Renderer* r = GetEngineContext().renderer;
                std::fprintf(stderr, "[text] '%s' world(%.2f,%.2f) z=%.2f scale=%.3f cam(%.2f,%.2f)\n",
                             lines.empty() ? "" : lines[0].c_str(), wx, wy, z, scale,
                             r ? r->GetCameraX() : 0.0f, r ? r->GetCameraY() : 0.0f);
            }
            float lineY = y;
            for (const std::string& line : lines) {
                font->DrawText(*renderer, line, x, lineY, scale, align, ctx2.fontEdge, ctx2.fontWidth,
                                top.a, top.b, top.c, top.d, top.e, top.f, z);
                lineY -= scale * kLineSpacing;
            }
        });

        if (printInfo.m_type == GM_TABLE) {
            float maxHalfWidth = 0.0f;
            for (const std::string& line : lines) {
                maxHalfWidth = std::max(maxHalfWidth, font->MeasureWidth(line, scale) * 0.5f);
            }
            gmTableObject* table = (gmTableObject*)a_thread->GetMachine()->GetObject(printInfo.m_value.m_ref);
            float left = x, right = x;
            if (align == 2) { left = x - maxHalfWidth; right = x + maxHalfWidth; }
            else if (align == 1) { left = x - maxHalfWidth * 2.0f; right = x; }
            else { left = x; right = x + maxHalfWidth * 2.0f; }
            float bottomOfLastLine = y - (float)(lines.size() - 1) * scale * kLineSpacing - scale * 0.5f;
            table->Set(a_thread->GetMachine(), "top", gmVariable(y + scale * 0.5f));
            table->Set(a_thread->GetMachine(), "bottom", gmVariable(bottomOfLastLine));
            table->Set(a_thread->GetMachine(), "left", gmVariable(left));
            table->Set(a_thread->GetMachine(), "right", gmVariable(right));
            table->Set(a_thread->GetMachine(), "numLines", gmVariable((gmint)lines.size()));
        }
    }
    a_thread->PushFloat(heightConsumed);
    return GM_OK;
}

} // namespace

int GM_CDECL DC_gmf_print(gmThread* a_thread) { return PrintImpl(a_thread, true); }
int GM_CDECL DC_gmf_printTranslated(gmThread* a_thread) { return PrintImpl(a_thread, true); }
int GM_CDECL DC_gmf_printUntranslated(gmThread* a_thread) { return PrintImpl(a_thread, false); }
int GM_CDECL DC_gmf_printArray(gmThread* a_thread) { return PrintArrayImpl(a_thread); }

int GM_CDECL DC_gmf_setupWorldView(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 0 || count > 1) return GM_EXCEPTION;
    const bool cameraRotationEnabled = count == 1 &&
        a_thread->Param(0).m_value.m_int != 0;
    return DC_RecordOrRun(a_thread, [cameraRotationEnabled] {
        GetEngineContext().renderer->SetWorldView(cameraRotationEnabled);
    });
}

int GM_CDECL DC_gmf_setupWorldOrthoView(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        GetEngineContext().renderer->SetViewMode(Renderer::ViewMode::WorldOrtho);
    });
}

int GM_CDECL DC_gmf_setupOrthoView(gmThread* a_thread) {
    const int n = GM_NUM_PARAMS;
    if (n != 0 && n != 4) return GM_EXCEPTION;
    if (n == 0) {
        return DC_RecordOrRun(a_thread, [] {
            GetEngineContext().renderer->SetViewMode(Renderer::ViewMode::UnitOrtho);
        });
    }
    const float l = a_thread->ParamFloatOrInt(0, 0.0f);
    const float r = a_thread->ParamFloatOrInt(1, 1.0f);
    const float b = a_thread->ParamFloatOrInt(2, 0.0f);
    const float t = a_thread->ParamFloatOrInt(3, 1.0f);
    return DC_RecordOrRun(a_thread, [l, r, b, t] {
        GetEngineContext().renderer->SetCustomOrtho(l, r, b, t);
    });
}
int GM_CDECL DC_gmf_setupAspectRatioOrthoView(gmThread* a_thread) {
    const auto& engine = GetEngineContext();
    const float aspect = !SystemConfig::Get().letterbox && engine.renderer
        ? engine.renderer->GetAspect() : engine.video.aspectRatio;
    return DC_RecordOrRun(a_thread, [aspect] {
        GetEngineContext().renderer->SetAspectRatioOrtho(aspect);
    });
}

int GM_CDECL gmf_DC_pushMatrix(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        ctx.stack.push_back(ctx.Top());
    });
}

int GM_CDECL gmf_DC_popMatrix(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        if (ctx.stack.size() > 1) ctx.stack.pop_back();
    });
}

int GM_CDECL gmf_DC_translate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    float x = 0.0f, y = 0.0f, z = 0.0f;
    gmVariable p0 = a_thread->Param(0);
    if (p0.m_type == g_vector2DType) {
        GetVector2D(*a_thread->GetMachine(), p0, x, y);
        if (a_thread->GetNumParams() >= 2) z = a_thread->ParamFloatOrInt(1, 0.0f);
        // GE3_TEXT_DIAG=1 -- RECORD-time translate values. Compare against
        // the [text] lines' replay-time world positions: if a translate is
        // recorded as (0,0) the body really was at the origin when draw()
        // ran, which means draw() ran before configure() positioned it and
        // the cached draw list has been replaying that stale transform
        // ever since. If it records real values but [text] still reports
        // world(0,0), the transform is being lost between record and
        // replay instead.
        if (getenv("GE3_TEXT_DIAG")) {
            std::fprintf(stderr, "[xlate-rec] (%.2f,%.2f) z=%.2f\n", x, y, z);
        }
    } else if (p0.m_type == GM_NULL) {
        // GE3_TEXT_DIAG=1 -- a null vector reaching translate keeps z but
        // zeroes x/y, which is exactly the "(0,0) with correct z" signature
        // seen on every tutorial text draw. Almost always means the caller
        // did `translate(this.mainpb.getPos(), z)` with a NULL mainpb.
        if (getenv("GE3_TEXT_DIAG")) {
            std::fprintf(stderr, "[xlate-null] null vector, z kept\n");
        }
        if (a_thread->GetNumParams() >= 2) z = a_thread->ParamFloatOrInt(1, 0.0f);
    } else {
        GM_CHECK_NUM_PARAMS(2);
        x = a_thread->ParamFloatOrInt(0, 0.0f);
        y = a_thread->ParamFloatOrInt(1, 0.0f);
        if (a_thread->GetNumParams() >= 3) z = a_thread->ParamFloatOrInt(2, 0.0f);
    }
    return DC_RecordOrRun(a_thread, [x, y, z] {
        DrawContext& ctx = GetDrawContext();
        DrawContext::Mat t;
        t.e = x; t.f = y; t.z = z;
        ctx.Top() = DC_Multiply(ctx.Top(), t);
        // GE3_TEXT_DIAG=1 -- REPLAY-time translate. This is the one that
        // can be compared against the [text] lines (also replay-time); the
        // [xlate-rec] line above fires at RECORD time and interleaves
        // differently, so the two must not be read against each other.
        if (getenv("GE3_TEXT_DIAG")) {
            std::fprintf(stderr, "[xlate-run] (%.2f,%.2f) z=%.2f -> top(%.2f,%.2f)\n",
                         x, y, z, ctx.Top().e, ctx.Top().f);
        }
    });
}

int GM_CDECL gmf_DC_rotate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable p0 = a_thread->Param(0);
    float angle = 0.0f;
    if (p0.m_type == GM_FLOAT) angle = p0.m_value.m_float;
    else if (p0.m_type == GM_INT) angle = (float)p0.m_value.m_int;
    else if (p0.m_type == g_vector2DType) {
        float vx = 0.0f, vy = 0.0f;
        GetVector2D(*a_thread->GetMachine(), p0, vx, vy);
        if (vx != 0.0f || vy != 0.0f) angle = std::atan2(vy, vx);
    }
    return DC_RecordOrRun(a_thread, [angle] {
        DrawContext& ctx = GetDrawContext();
        float s = std::sin(angle), c = std::cos(angle);
        DrawContext::Mat r;
        r.a = c; r.b = s; r.c = -s; r.d = c;
        ctx.Top() = DC_Multiply(ctx.Top(), r);
    });
}

int GM_CDECL gmf_DC_scale(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    float sx = a_thread->ParamFloatOrInt(0, 1.0f);
    float sy = sx;
    if (a_thread->GetNumParams() >= 2) sy = a_thread->ParamFloatOrInt(1, 1.0f);
    return DC_RecordOrRun(a_thread, [sx, sy] {
        DrawContext& ctx = GetDrawContext();
        DrawContext::Mat s;
        s.a = sx; s.d = sy;
        ctx.Top() = DC_Multiply(ctx.Top(), s);
    });
}

int GM_CDECL gmf_DC_multMatrix(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    Matrix4Handle matrix = GetMatrix4Handle(*a_thread->GetMachine(),
                                            a_thread->Param(0));
    if (!matrix) return GM_EXCEPTION;
    return DC_RecordOrRun(a_thread, [matrix] {
        DrawContext& ctx = GetDrawContext();
        ctx.Top() = DC_Multiply(ctx.Top(), *matrix);
    });
}

int GM_CDECL DC_gmf_clear(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n > 1) return GM_EXCEPTION;
    // Retail copies the raw GM value word for the optional mode argument; it
    // does not interpret it as a colour or alpha value.
    const uint32_t mode = n == 0 ? 9u :
        static_cast<uint32_t>(a_thread->Param(0).m_value.m_int);
    return DC_RecordOrRun(a_thread, [mode] {
        if (Renderer* renderer = GetEngineContext().renderer)
            renderer->DrawFullscreenOverlay(GetDrawContext().Top().z, mode);
    });
}

int GM_CDECL DC_gmf_setTextureByHandle(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    auto resourceLease = ResolveTextureResourceLease(*a_thread->GetMachine(), a_thread->Param(0));
    return DC_RecordOrRun(a_thread, [handle, resourceLease] {
        GetDrawContext().currentTextureHandle = handle;
    });
}
int GM_CDECL DC_gmf_setTexture(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    EngineContext& ctx = GetEngineContext();
    unsigned int handle = 0;
    std::shared_ptr<void> resourceLease;
    auto resolved = ctx.fileHelper ? ctx.fileHelper->Resolve(path) : std::nullopt;
    if (resolved && ctx.renderer) {
        handle = ctx.renderer->LoadTexture(*resolved, std::string("1:") + path);
        resourceLease = MakeTextureResourceLease(handle, 1, path);
    }
    return DC_RecordOrRun(a_thread, [handle, resourceLease = std::move(resourceLease)] {
        GetDrawContext().currentTextureHandle = handle;
    });
}
int GM_CDECL DC_gmf_clearTexture(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        GetDrawContext().currentTextureHandle = 0;
    });
}

int GM_CDECL DC_gmf_drawModTexture(gmThread* a_thread) {
    DrawListData* target = RecordingTarget(a_thread);
    if (!target || a_thread->GetNumParams() < 1) return GM_EXCEPTION;

    const gmVariable& value = a_thread->Param(0);
    if (value.m_type != GM_NULL) {
        // The retail wrapper assumes a native user object and dereferences
        // its payload directly without checking the GM tag. Avoid treating a
        // string/table/value payload as that pointer on the desktop.
        if (value.m_type < GM_USER) return GM_EXCEPTION;
        gmObject* object = a_thread->GetMachine()->GetObject(value.m_value.m_ref);
        if (!object) return GM_EXCEPTION;
        target->commandBuffer->retainedObjects.push_back(object);
    }

    // The command is intentionally empty: the retail opcode is ignored by
    // the dispatcher. Recording it preserves its place among list entries.
    target->commandBuffer->commands.emplace_back([] {});
    return GM_OK;
}

int GM_CDECL DC_gmf_setTextureSubImg(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    auto resourceLease = ResolveTextureResourceLease(*a_thread->GetMachine(), a_thread->Param(0));
    // Real signature takes (texture, palette). COLOURSCHEME_
    // SetGeneratedPaletteColour(palette,...) can stash pending fill/outline
    // colours on the palette-backed TEXTURE (which has no GL image handle);
    // this is where real usage pairs it with the image resource.
    std::shared_ptr<PaletteData> palette;
    if (handle != 0 && a_thread->GetNumParams() >= 2) {
        // Retail draw opcode 0xf retains the palette object and consumes its
        // current entries during replay. Hold shared palette storage rather
        // than snapshotting at record time: game scripts commonly set palette
        // entries after setTextureSubImg() but before drawing.
        palette = ResolvePaletteDataLease(*a_thread->GetMachine(), a_thread->Param(1));
    }
    return DC_RecordOrRun(a_thread, [handle, palette, resourceLease] {
        BindTextureSubTexture(handle, palette);
        const bool indexedPaletteApplied = handle != 0 && palette &&
            GetEngineContext().renderer->SetTexturePalette(handle, palette->rgba.data(), palette->rgba.size());
        if (handle != 0 && !palette)
            GetEngineContext().renderer->ResetTexturePalette(handle);
        if (handle != 0 && palette && palette->hasPendingTint && !indexedPaletteApplied) {
            GetEngineContext().renderer->SetTextureGeneratedTint(
                handle, palette->fillR, palette->fillG, palette->fillB,
                palette->outlineR, palette->outlineG, palette->outlineB);
        }
        GetDrawContext().currentTextureHandle = handle;
    });
}

int GM_CDECL DC_gmf_drawTextureByHandle(gmThread* a_thread) {
    // RETAIL PPC: wrapper 0x7d510 dispatches only counts 9, 10, 11 and 13;
    // every other count returns -1. The 9-arg form is corners+texture, 10
    // adds Z, 11 adds a caller draw-buffer object, and 13 supplies Z plus UV
    // scales, texture, and a caller draw-buffer object.
    const int count = a_thread->GetNumParams();
    if (count != 9 && count != 10 && count != 11 && count != 13)
        return GM_EXCEPTION;
    GM_FLOAT_OR_INT_PARAM(x0, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y0, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x1, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y1, 3, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x2, 4, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y2, 5, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x3, 6, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y3, 7, 0.0f);
    float extraZ = 0.0f;
    float uvScaleU = 1.0f;
    float uvScaleV = 1.0f;
    int handleParamIdx = 8;
    int extraObjectParamIdx = -1;
    if (count == 10 || count == 11 || count == 13) {
        extraZ = a_thread->ParamFloatOrInt(8, 0.0f);
        handleParamIdx = 9;
    }
    if (count == 11) {
        extraObjectParamIdx = 10;
    } else if (count == 13) {
        uvScaleU = a_thread->ParamFloatOrInt(9, 1.0f);
        uvScaleV = a_thread->ParamFloatOrInt(10, 1.0f);
        handleParamIdx = 11;
        extraObjectParamIdx = 12;
    }
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(handleParamIdx));
    auto resourceLease = ResolveTextureResourceLease(*a_thread->GetMachine(), a_thread->Param(handleParamIdx));
    Renderer* renderer = GetEngineContext().renderer;
    const int textureShaderType = renderer ? renderer->GetTextureShaderType(handle) : 0;
    const unsigned int shapePrimitiveMode = extraObjectParamIdx >= 0
        ? GetShapePrimitiveMode(*a_thread->GetMachine(),
                                a_thread->Param(extraObjectParamIdx))
        : 0;

    // The retail wrapper retains this extra native object during recording.
    // Keep it alive through deferred desktop replay as well; its retained
    // primitive selects the native quad vertex/UV order below.
    if (extraObjectParamIdx >= 0) {
        const gmVariable& value = a_thread->Param(extraObjectParamIdx);
        if (value.m_type >= GM_USER) {
            gmObject* object = a_thread->GetMachine()->GetObject(value.m_value.m_ref);
            if (object) {
                if (DrawListData* target = RecordingTarget(a_thread))
                    target->commandBuffer->retainedObjects.push_back(object);
            }
        }
    }

    return DC_RecordOrRun(a_thread, [x0, y0, x1, y1, x2, y2, x3, y3, handle, extraZ,
                                     uvScaleU, uvScaleV,
                                     textureShaderType, shapePrimitiveMode,
                                     resourceLease] {
        if (handle == 0) return;
        DrawContext& drawContext = GetDrawContext();
        drawContext.currentTextureHandle = handle;
        const DrawContext::Mat& m = GetDrawContext().Top();
        Renderer* renderer = GetEngineContext().renderer;
        if (std::getenv("GE3_TRACE_UI_QUADS")) {
            const std::string& sourcePath = renderer
                ? renderer->GetTextureSourcePath(handle) : std::string{};
            std::string normalizedSourcePath = sourcePath;
            std::replace(normalizedSourcePath.begin(), normalizedSourcePath.end(), '\\', '/');
            if (normalizedSourcePath.find("/loadingScreens/") != std::string::npos) {
                std::fprintf(stderr,
                    "[ui-quad-source] path=%s tex=%u raw0=(%.5f,%.5f) raw1=(%.5f,%.5f) "
                    "mat=(%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f)\n",
                    sourcePath.c_str(), handle, x0, y0, x1, y1,
                    m.a, m.b, m.c, m.d, m.e, m.f, m.z);
            }
        }
        float tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3;
        DC_TransformPoint(m, x0, y0, tx0, ty0);
        DC_TransformPoint(m, x1, y1, tx1, ty1);
        DC_TransformPoint(m, x2, y2, tx2, ty2);
        DC_TransformPoint(m, x3, y3, tx3, ty3);
        const DrawContext& ctx = GetDrawContext();
        // Generated texture palettes and animation palettes already contain
        // their final RGB. drawAnimation applies white modulation for this
        // reason; without the same rule static generated sprites inherit a
        // preceding COLOURSCHEME_SetColour (SwayingTree sets bg_out while
        // drawing its stem immediately before its head).
        float pr, pg, pb, pa;
        const bool generatedPalette = renderer->UsesGeneratedTexturePalette(handle);
        if (generatedPalette) {
            renderer->GetColour(pr, pg, pb, pa);
            renderer->SetColour(1.0f, 1.0f, 1.0f, pa);
        }
        renderer->DrawTexturedQuad(
            tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3, handle, m.z + extraZ,
            ctx.distanceTextureEnabled, ctx.distanceTextureDiscardLevel,
            ctx.distanceTextureSmoothLevel, textureShaderType, uvScaleU, uvScaleV,
            shapePrimitiveMode);
        if (generatedPalette) {
            renderer->SetColour(pr, pg, pb, pa);
        }
        drawContext.currentTextureHandle = 0;
    });
}

int GM_CDECL DC_gmf_drawTexture(gmThread* a_thread) {
    const int paramCount = a_thread->GetNumParams();
    if (paramCount != 9 && paramCount != 10 && paramCount != 11 &&
        paramCount != 12 && paramCount != 14)
        return GM_EXCEPTION;
    GM_FLOAT_OR_INT_PARAM(x0, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y0, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x1, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y1, 3, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x2, 4, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y2, 5, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x3, 6, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y3, 7, 0.0f);
    float extraZ = 0.0f;
    float uvScaleU = 1.0f;
    float uvScaleV = 1.0f;
    int pathParamIdx = 8;
    int areaParamIdx = -1;
    int extraObjectParamIdx = -1;
    if (paramCount == 10) {
        areaParamIdx = 9;
    } else if (paramCount == 11) {
        extraZ = a_thread->ParamFloatOrInt(8, 0.0f);
        pathParamIdx = 9;
        areaParamIdx = 10;
    } else if (paramCount == 12) {
        extraZ = a_thread->ParamFloatOrInt(8, 0.0f);
        pathParamIdx = 9;
        extraObjectParamIdx = 10;
        areaParamIdx = 11;
    } else if (paramCount == 14) {
        extraZ = a_thread->ParamFloatOrInt(8, 0.0f);
        uvScaleU = a_thread->ParamFloatOrInt(9, 1.0f);
        uvScaleV = a_thread->ParamFloatOrInt(10, 1.0f);
        pathParamIdx = 11;
        extraObjectParamIdx = 12;
        areaParamIdx = 13;
    }
    GM_STRING_PARAM(path, pathParamIdx, "");
    int area = areaParamIdx >= 0
        ? a_thread->Param(areaParamIdx).m_value.m_int
        : 1; // Compatibility form defaults to FILEHELPER_AREA_GAMEDATA.

    unsigned int handle = 0;
    EngineContext& ctx = GetEngineContext();
    auto resolved = ResolveDrawTexturePath(path, area);
    if (resolved) {
        const std::string cacheKey = std::to_string(area) + ":" + path;
        handle = ctx.renderer->LoadTexture(*resolved, cacheKey);
    }
    const int textureShaderType = ctx.renderer->GetTextureShaderType(handle);
    const unsigned int shapePrimitiveMode = extraObjectParamIdx >= 0
        ? GetShapePrimitiveMode(*a_thread->GetMachine(),
                                a_thread->Param(extraObjectParamIdx))
        : 0;

    // Retail retains the caller object across helper execution. The desktop
    // textured quad already uses its explicit four corners; root the object
    // through deferred playback while the exact custom-buffer behavior is
    // being carried forward.
    if (extraObjectParamIdx >= 0) {
        const gmVariable& value = a_thread->Param(extraObjectParamIdx);
        if (value.m_type >= GM_USER) {
            gmObject* object = a_thread->GetMachine()->GetObject(value.m_value.m_ref);
            if (object) {
                if (DrawListData* target = RecordingTarget(a_thread))
                    target->commandBuffer->retainedObjects.push_back(object);
            }
        }
    }

    return DC_RecordOrRun(a_thread, [x0, y0, x1, y1, x2, y2, x3, y3, handle,
                                     extraZ, uvScaleU, uvScaleV, textureShaderType,
                                     shapePrimitiveMode] {
        if (handle == 0) return;
        DrawContext& drawContext = GetDrawContext();
        drawContext.currentTextureHandle = handle;
        const DrawContext::Mat& m = GetDrawContext().Top();
        float tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3;
        DC_TransformPoint(m, x0, y0, tx0, ty0);
        DC_TransformPoint(m, x1, y1, tx1, ty1);
        DC_TransformPoint(m, x2, y2, tx2, ty2);
        DC_TransformPoint(m, x3, y3, tx3, ty3);
        GetEngineContext().renderer->DrawTexturedQuad(
            tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3, handle, m.z + extraZ,
            drawContext.distanceTextureEnabled,
            drawContext.distanceTextureDiscardLevel,
            drawContext.distanceTextureSmoothLevel,
            textureShaderType, uvScaleU, uvScaleV, shapePrimitiveMode);
        drawContext.currentTextureHandle = 0;
    });
}
