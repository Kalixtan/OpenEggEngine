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

// The retail wrappers widen GM_FLOAT values to double and convert GM_INT
// values directly to double before calling FUN_0003e040. Keep that precision
// through its double addition/absolute-value operations; the helper then
// rounds once to float before multiplying by its 0.5f constant.
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

// drawList.setAttrCulling(mode) -- confirmed real, see Renderer::SetCulling's
// header comment for the mode semantics (taken straight from the one real
// call site's own comment, QuadShadowed.gm:36).
int GM_CDECL DC_gmf_setAttrCulling(gmThread* a_thread) {
    GM_INT_PARAM(mode, 0, 0);
    return DC_RecordOrRun(a_thread, [mode] {
        DrawContext& ctx = GetDrawContext();
        ctx.culling = mode;
        GetEngineContext().renderer->SetCulling(ctx.culling);
    });
}

// Retail FUN_00041ad0 quantizes transparent Z by float 0x42055556, adds 0xff00,
// clamps to 16 bits, and selects tier 3 in the draw-list order key.
int GM_CDECL DC_gmf_setOrderTransparent(gmThread* a_thread) {
    DrawListData* list = RecordingTarget(a_thread);
    if (!list) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(z, 0, 0.0f);
    const int quantized = std::clamp((int)(z * 33.333335876464844f) + 0xff00, 0, 0xffff);
    list->orderKey = 0x30000000u | (unsigned int)quantized;
    return GM_OK;
}

// Retail FUN_00041b38 uses a different tier and reverses Z within it.
int GM_CDECL DC_gmf_setOrderAlphaTest(gmThread* a_thread) {
    DrawListData* list = RecordingTarget(a_thread);
    if (!list) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(z, 0, 0.0f);
    const int quantized = std::clamp(0xff - (int)(z * 33.333335876464844f), 0, 0xffff);
    list->orderKey = 0x20000000u | (unsigned int)quantized;
    return GM_OK;
}

// Retail 0x00082bbc changes only the tier bits of native field +0x30.
int GM_CDECL DC_gmf_setOrderLast(gmThread* a_thread) {
    DrawListData* list = RecordingTarget(a_thread);
    if (!list) return GM_EXCEPTION;
    list->orderKey = (list->orderKey & 0xcfffffffu) | 0x30000000u;
    return GM_OK;
}

// Verification: retail PPC plus native config table and Cg reflection. Native
// config row IDs differ from Cg paramno: row 1 is aColor/category 5, row 2 is
// aDiscardLevel, row 3 is aSmoothLevel, and row 4 is aSize/category 10.
// setDistanceTextureRenderAttributes(near, far) -- retail records two
// 4-byte payloads through FUN_0003e040 (DrawList.c:0x000815b8). Cg reflection
// and FUN_0003e040 establish that those payloads update native IDs 2
// (aDiscardLevel) and 3 (aSmoothLevel). Preserve both until another setter,
// resetFontRenderAttributes, or popAttributes changes them.
int GM_CDECL DC_gmf_setDistanceTextureRenderAttributes(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const double nearValue = RetailFloatOrIntAsDouble(a_thread, 0);
    const double farValue = RetailFloatOrIntAsDouble(a_thread, 1);
    // FUN_0003e040 receives (-near, far) from the retail wrapper. PPC fsel
    // selects native ID 2 / aDiscardLevel (0.25 for positive `near`) while
    // ID 3 / aSmoothLevel receives (near + far) / 2.
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

// drawList.setColourMask(bool) -- confirmed real, see Renderer::SetColourMask.
int GM_CDECL DC_gmf_setColourMask(gmThread* a_thread) {
    GM_INT_PARAM(enabled, 0, 1);
    return DC_RecordOrRun(a_thread, [enabled] {
        DrawContext& ctx = GetDrawContext();
        ctx.colourMask = (enabled != 0);
        GetEngineContext().renderer->SetColourMask(ctx.colourMask);
    });
}

// drawList.setAttrStencil(func, op, ref) / (func, sfail, dpfail, dppass, ref)
//
// DECOMPILED EXACTLY (0x000804d0), closing the gap
// layout not decoded in this pass ... flagged as a gap, not guessed"). The
// body accepts ONLY 3 or 5 arguments (any other count returns -1) and
// marshals them into a 0x18-byte / six-field struct pushed as opcode 0x22:
//
//     struct { u32 func, sfail, dpfail, dppass, ref, mask; }
//
// 5-arg form (0x80604), reading GM args 0..4 at the 12-byte gmVariable
// stride (+0, +0xc, +0x18, +0x24, +0x30):
//     func=arg0  sfail=arg1  dpfail=arg2  dppass=arg3  ref=arg4  mask=-1
// which is what this engine already did.
//
// 3-arg form (0x8053c) -- and this is the correction:
//     r6 = arg0; r5 = arg1; r4 = arg2; r7 = -1
//     stw r6,0x88   stw r5,0x8c   stw r5,0x90   stw r5,0x94
//     stw r4,0x98   stw r7,0x9c
// i.e. func=arg0, ref=arg2, mask=-1, and the SINGLE op argument is stored
// into all three op fields: sfail = dpfail = dppass = arg1. This engine
// previously defaulted sfail/dpfail to KEEP and only applied arg1 to
// dppass, on the reasoning that the 3-arg call sites "never need to touch
// the stencil buffer on a failed test". Retail simply does not make that
// distinction -- with the stencil test disabled or always-passing the two
// readings coincide, which is why it went unnoticed, but they diverge the
// moment a 3-arg REPLACE runs against a failing stencil or depth test.
//
// mask is the literal -1 (0xFFFFFFFF) in both forms, not GL's default 0xFF.
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

// drawList.setAttrDefaultStencil() -- confirmed real, resets to "no
// stencil test", see Renderer::SetDefaultStencil.
int GM_CDECL DC_gmf_setAttrDefaultStencil(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        ctx.stencilFunc = 8; ctx.stencilRef = 0;
        ctx.stencilSfail = ctx.stencilDpfail = ctx.stencilDppass = 100;
        GetEngineContext().renderer->SetDefaultStencil();
    });
}

// Retail drawList.setColour wrapper at 0x7e6c8 appends a color command after
// converting scalar payloads (float payload for GM_FLOAT, otherwise signed
// integer payload). Its 1/2-argument forms accept color ID, name, or COLOUR;
// its 3/4-argument forms are RGB[A]. Invalid arities/types return -1.
// Verification: FUN_0007e6c8 uses float payloads only for GM_FLOAT and
// sign-extends/converts every other raw 32-bit payload to single precision.
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
    // Match FUN_0003e040's PPC fsel: a nonnegative first argument stores
    // 0.5, a negative one 0.25. Unlike the distance-texture wrapper, the
    // font wrapper passes its first argument unchanged.
    const float storedEdge = edge < 0.0 ? 0.25f : 0.5f;
    const float storedWidth = RetailSmoothLevelFromArgs(edge, width);
    return DC_RecordOrRun(a_thread, [storedEdge, storedWidth] {
        DrawContext& ctx = GetDrawContext();
        ctx.fontEdge = storedEdge;
        ctx.fontWidth = storedWidth;
    });
}

// Verification: decomp-confirmed native call and parameter writes; the native
// table row map distinguishes descriptor IDs from Cg paramno. Reset writes
// rows 2/3 (discard/smooth); row 4 (aSize) is not set by this path.
int GM_CDECL DC_gmf_resetFontRenderAttributes(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        ctx.fontEdge = 0.5f;
        ctx.fontWidth = 0.45f;
        ctx.distanceTextureEnabled = false;
        // FUN_0003e0fc writes native ID 2 (aDiscardLevel)=0.5 and
        // ID 3 (aSmoothLevel)=0.45. Cg paramno is one higher than these
        // native descriptor-row IDs for the corresponding uniforms.
        ctx.distanceTextureDiscardLevel = 0.5f;
        ctx.distanceTextureSmoothLevel = 0.45f;
    });
}

namespace {

// classes/DrawList.c's printTranslated decompiles to a helper
// (FUN_000816c8) that reads a variable number of stack slots by type, not
// a fixed positional signature. Reverse-engineered from corpus usage
// instead (every distinct arg count/shape grepped across game_data/*.gm):
// the leading run of NUMERIC args (before the first STRING arg) encodes
// the position, and its length alone disambiguates the shape --
// consistently either 2 (x,y), 3 (x,y,z), or 4 (x,wrapWidth,y,z) across
// every real call site found. Whatever follows the text arg is, in order,
// [height], [align], [PRINTINFO out-param] -- all optional, trailing.
//   2 leading nums: print(x,y,text,[height,[align,[pInfo]]])
//   3 leading nums: print(x,y,z,text,[height,[align,[pInfo]]])
//   4 leading nums: print(x,wrapWidth,y,z,text,[height,[align,[pInfo]]])
// wrapWidth greedily word-wraps into multiple lines (see WrapText below).
// Resolves one {fmt, arg1, arg2, ...} substitution argument to its display
// string. Args are confirmed real in 3 shapes (corpus grep of `%1`/`%2`
// call sites): a plain GM_STRING (KeyToStringMap[keyCode]), a plain number
// (HelpScreenPS3.gm's `{_("Page %1/%2"), this.currentPageIndex+1,
// this.numberOfPages}`), or a nested {untranslated=str} wrapper
// (PauseProcessBase.gm's `{_("Music by %1"), {untranslated=this.artist}}`,
// used so the artist/level name itself isn't run through the `_()`
// translation table like the surrounding format string is).
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

// Substitutes 1-indexed %1, %2, ... placeholders in a format string with
// the given resolved arg strings -- confirmed real via pervasive corpus
// usage (`{"%1 %2", KeyToStringMap[...], text}`, `{_("Page %1/%2"), ...}`,
// `{_("Music by %1"), {untranslated=...}}`). Left unsubstituted (printed
// literally, e.g. "Music by %1") wherever this was missing before.
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

// Greedy word-wrap for print()'s 4-leading-number form (x,wrapWidth,y,z,text,...
// -- confirmed real call shape via FontHelper.gm's
// `drawList.print(0,width,0,0,text,adjustedSize,TEXTALIGN_CENTER,pInfo);`).
// wrapWidth was previously read-and-discarded (BitmapFont had no
// line-wrapping at all), so any real multi-line text (tutorial messages,
// long popup copy) just ran off the edge of its box as one unbroken line.
// No decompiled evidence for the exact line-spacing ratio -- 1.15x scale is
// a plausible standard leading, not pinned to a confirmed constant.
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

    // Stop the leading-position scan at the first NON-NUMERIC arg, not
    // specifically the first STRING -- confirmed real usage (Platformer/
    // ScrollingString.gm:136 `drawList.print(0,0,0,this.string,...)`, where
    // `this.string` was itself built by Platformer/PauseProcessBase.gm:685
    // as `{this.parent.levelName}` or `{untranslated=this.parent.levelName}`,
    // real table-wrapped-string patterns) can legitimately pass a GM_TABLE
    // as the text argument, not just a GM_STRING -- scanning for "not a
    // string" specifically ran straight past it (a table isn't a string
    // either) and miscounted the whole leading run, always overflowing
    // past 4 and throwing "expecting a string text argument" on every
    // real call that does this (e.g. the pause menu's level-title display,
    // permanently broken every frame).
    auto isPosition = [](const gmVariable& v) { return v.m_type == GM_INT || v.m_type == GM_FLOAT; };
    int numLeading = 0;
    while (numLeading < n && isPosition(a_thread->Param(numLeading))) ++numLeading;
    if (numLeading < 2 || numLeading > 4 || numLeading >= n) {
        GM_EXCEPTION_MSG("print: expecting a string text argument");
        return GM_EXCEPTION;
    }

    // 4 leading numerics is a (left, right, y, z) BOUNDS form, not
    // (x, wrapWidth, y, z) -- confirmed by corpus usage: every real 4-arg
    // call site passes a genuine left/right pair, not an x/width pair
    // (Platformer/PopupProcessBase.gm:265 `this.preDrawList.print(
    // -this.bodyTable[i].textWidth/2.0, this.bodyTable[i].textWidth/2.0,
    // 0, 0, text, textSize, alignment, pInfo)`; PopUpScriptLine.gm's
    // `drawList.print(0, this.TEXT_WIDTH, 0, 0, text, size, TEXTALIGN_LEFT,
    // pInfo)` pattern, repeated ~30 times across the corpus). Treating the
    // first numeric as a literal x (as before) happened to look right for
    // TEXTALIGN_LEFT calls (anchor == left, correct by coincidence) but
    // put CENTER/RIGHT-aligned text at `left` instead of the bounds'
    // midpoint/right edge -- e.g. GenericOkDialogProcess's body text
    // rendered shifted a full half-textWidth off its intended center,
    // overflowing the popup's left edge. The actual anchor x (computed
    // below, once `align` is known) and wrapWidth (right-left, not a bare
    // `right`) are both alignment-dependent for this form.
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
        // See header comment above -- {untranslated=str} or a bare
        // positional {str} table, both confirmed real.
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

    // Return value -- CONFIRMED REAL and load-bearing, missed in the
    // (0x000819cc) ends by pushing a computed float onto the GM return slot
    // (`*(float*)(iVar1+iVar2*0xc+4) = (float)dVar4;` then bumping the
    // thread's top -- GM's own "push a return value" pattern, identical in
    // shape to every other function here that returns something), not
    // falling through to nothing the way a bare `return GM_OK;` with no
    // push implies. Confirmed independently by corpus usage -- 31 real call
    // sites consume it directly as a line-advance amount, e.g.
    // Platformer/PopUpScriptLine.gm: `y-=drawList.print(left,right,y,0,
    // this.title,yinctitle,TEXTALIGN_CENTER,pi);` (stacking multiple
    // print() calls downward by subtracting each one's return value from
    // the running y cursor) -- not a return value real scripts merely
    // tolerate being null, one they actively depend on for layout, on
    // every single-line AND multi-line print in the corpus. Previously
    // returning nothing meant every one of those 31 sites hit "operator
    // sub undefined for type null and float" the moment it ran.
    // FUN_00040650's exact inputs aren't recoverable (opaque helper, see
    // this function's own reveng comment), so the precise formula isn't
    // decompiled -- but the CONTRACT is unambiguous from usage: the total
    // vertical height just consumed, since every call site treats it as
    // "how far to move the cursor past what I just drew". Derived here as
    // (line count) * (line spacing), the same kLineSpacing this function
    // already draws multi-line text with, so a single print() and an
    // N-line wrapped print() both advance the cursor by exactly the space
    // their own output actually occupies.
    // Calibrated against FoE's real PC build (Wine screenshot of the title
    // screen vs GE3_SCREENSHOT_FRAME capture of the same script state):
    // measured line-to-line pitch for consecutive single-line print() calls
    // at a known scale gives kLineSpacing ~= 0.75, not 1.15 -- the 1.15
    // guess made every successive print() advance the cursor ~53% too far,
    // visibly compounding over the title screen's ~15 stacked print() calls
    // (text drifted down far enough to collide with the TOJAM logo).
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

// Video.c cluster), same leading-numerics (left,[right,]y,[z]) position
// form as print()/PrintImpl above (real call site TouchEditProcessPS3.gm
// :2263: `drawList.printArray(0,0,-10,0,tipTable,.03,TEXTALIGN_CENTER,
// this.pInfo);` -- 4 leading numerics, i.e. the same (left,right,y,z)
// bounds form print() uses) -- but the "text" argument is a plain array
// of already-formed strings (tipTable[0..n], TouchEditProcessPS3.gm:1315's
// getTipTable() builds it that way, one already-translated/composed
// string per line: spacerString/createString/cancelString/...), printed
// one line per array element top-to-bottom, rather than PrintImpl's
// single-string-with-%N-substitution/table-wrapper handling. Shares
// PrintImpl's bounds-to-anchor-x resolution, deferred-draw-via-
// DC_RecordOrRun mechanism, and PRINTINFO out-param contract exactly --
// only the "how do we get the list of lines" step differs.
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

// print() confirmed real; the distinct printUntranslated() confirmed real
// localization lookup and printUntranslated() is the explicit opt-out.
int GM_CDECL DC_gmf_print(gmThread* a_thread) { return PrintImpl(a_thread, true); }
// Bytecode verified against VMDRAWLIST.printTranslated at EBOOT 0x81b80:
// it shares FUN_000816c8's argument parser with printUntranslated, then
// formats through FUN_00010e28 with translated=1 and submits the resulting
// text via FUN_00040650. This is the same translated path as DrawList.print.
int GM_CDECL DC_gmf_printTranslated(gmThread* a_thread) { return PrintImpl(a_thread, true); }
int GM_CDECL DC_gmf_printUntranslated(gmThread* a_thread) { return PrintImpl(a_thread, false); }
int GM_CDECL DC_gmf_printArray(gmThread* a_thread) { return PrintArrayImpl(a_thread); }

// Verification: DrawList.c @ 0x7f874 confirms 0/1 argument arity and direct
// raw-payload != 0 conversion. PPC FUN_0004cea0 branches on that flag: false
// skips its axis-angle camera transform; true runs the VMX transform branch.
int GM_CDECL DC_gmf_setupWorldView(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 0 || count > 1) return GM_EXCEPTION;
    const bool cameraRotationEnabled = count == 1 &&
        a_thread->Param(0).m_value.m_int != 0;
    return DC_RecordOrRun(a_thread, [cameraRotationEnabled] {
        GetEngineContext().renderer->SetWorldView(cameraRotationEnabled);
    });
}

// Video.setupWorldOrthoView -- retail binding +0x3c -> device +0x60
// (FUN_004215a0), gluOrtho2D over the GameBoard's AABB. Was not bound at all
// previously. FoE's own content never calls it, but retail has it.
int GM_CDECL DC_gmf_setupWorldOrthoView(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        GetEngineContext().renderer->SetViewMode(Renderer::ViewMode::WorldOrtho);
    });
}

// setupAspectRatioOrthoView. Same reasoning as setupWorldView above -- our
// renderer already recomputes a fixed ortho projection per draw call;
// these two turn camera-relative projection back off (screen-fixed UI).
// Video.setupOrthoView -- retail binding +0x40 -> device +0x64
// (FUN_00421640), gluOrtho2D(0, 1, 0, 1). This previously used the
// aspect-ratio form, which is a different mode (see below).
// SOUND SHAPES: this takes EITHER no arguments OR exactly four, confirmed
// it reads the GM argument count and dispatches `if (count < 1) { ortho(
// DAT_0008132c, DAT_00081330, DAT_0008132c, DAT_00081330, list) } else if
// (count == 4) { ...read four float-or-int args... } else { return -1 }`.
// The 0-arg constants read 0.0f/1.0f out of EBOOT.elf, i.e. the 0..1 unit
// square -- so the 4-arg form is the same (left, right, bottom, top) tuple
// made explicit. Anything other than 0 or 4 arguments is a real error in
// retail, matched here.
//
// The four-argument form was previously ignored outright (the arguments were
// never even read), silently projecting any caller's custom bounds as the
// unit square. Stock game_data only calls the 0-arg form, so this is an
// accuracy fix rather than a live bug fix -- but a silent one is exactly
// what would make a future custom-bounds caller mysteriously misrender.
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
// Video.setupAspectRatioOrthoView -- retail binding +0x44 -> device +0x68
// (FUN_00421680), gluOrtho2D(-a/2, a/2, -0.5, 0.5). This is the one FoE's
// own UI actually uses.
int GM_CDECL DC_gmf_setupAspectRatioOrthoView(gmThread* a_thread) {
    // RETAIL PPC: FUN_0003e4a0 reads Video+0x81bc before building the matrix
    // command. Capture the same value now because DrawList execution is deferred.
    // The desktop full-window option intentionally substitutes the current
    // viewport ratio so UI and camera helpers adapt to arbitrary window shapes.
    const auto& engine = GetEngineContext();
    const float aspect = !SystemConfig::Get().letterbox && engine.renderer
        ? engine.renderer->GetAspect() : engine.video.aspectRatio;
    return DC_RecordOrRun(a_thread, [aspect] {
        GetEngineContext().renderer->SetAspectRatioOrtho(aspect);
    });
}

// pushMatrix/popMatrix/translate/rotateZ -- DECOMPILED-CONFIRMED via live
// beginShape_plotVertex_endShape_matrixstack_setColour.txt). drawList's
// real bodies (FUN_0007f990/FUN_0008443c/FUN_00084478/FUN_0008470c) turned
// out to be a materially different representation than assumed: not a full
// 2x3 affine matrix at all, but a stack of plain (x, y, z, angle) tuples
// -- pushMatrix appends a new zeroed tuple (fields at +0x10/+0x14/+0x18/
// +0x1c off a per-frame 0x10-byte stride); popMatrix just decrements the
// frame count (no restore needed, since a child frame's tuple starts at
// zero and never touches its parent's); translate(x,y,[z]) ADDS into the
// CURRENT top frame's x/y/z fields; rotateZ(angle) ADDS into its angle
// field. (Video's own pushMatrix/popMatrix are separate again -- they
// append opcodes 0x17/0x18 to a deferred command stream instead, per the
// same evidence file; not replicated here, see this cluster's other
// comments for why.)
//
// This engine's existing DrawContext::Mat/DC_Multiply (a full affine 2x3 +
// z) is a strict superset of that four-field tuple -- composing a pure
// translation and a pure Z-rotation the way this file already does
// produces the identical CUMULATIVE push/translate/rotate/pop behavior
// just confirmed, without needing a parallel, narrower stack representation
// alongside it. Every other draw primitive already reads ctx.Top() as a
// full matrix (drawTextureByHandle, print, drawShape, ...), so keeping one
// unified representation -- rather than bolting on retail's narrower one
// just for these four calls -- matches this project's stated renderer
// principle (docs/ARCHITECTURE.md: match visible output, not internal
// storage). scale is NOT part of this confirmed (x,y,z,angle) tuple at
// all (Video's scale opcodes into its own command stream; drawList's own
// scale body, FUN_00083280, failed to disassemble cleanly under
// -noanalysis and needs a proper full-analysis pass before it can be
// trusted) -- stays trapped rather than guessed at.
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
    // translate(x, y, [z]) numeric, or translate(vector2D, [z]) -- both
    // real call shapes across the corpus (BaseNotePlant.gm's
    // `drawList.translate(notePos, PLAYER_MODE_NOTE_Z)` uses the vector2D
    // form); the decompiled body's own arg-count dispatch (2 vs 3 args)
    // only distinguishes "with/without z", not which of these two shapes
    // arg 0 is, so this dual-form read is retained from before.
    //
    // Uses gmThread::ParamFloatOrInt's lenient (default-on-mismatch)
    // overload rather than the GM_FLOAT_OR_INT_PARAM macro -- that macro
    // expands to the vendored gmscript's STRICT variant (gmThread.h's
    // unconditional `#if 1` block), which throws a blank-message
    // GM_EXCEPTION the instant a param exists but isn't numeric, rather
    // than coercing/defaulting. Confirmed real, non-hypothetical call site
    // needing the lenient read: Platformer/MainMenu5/
    // CampaignScreenRecordPlayer.gm:1072's `VIDEOHELPER_DrawFilledDonut(
    // t.grooveShape,0,0,0,t.r,t.r,th,th,q)` -- 9 args against
    // VideoHelper.gm's 8-param VIDEOHELPER_DrawFilledDonut, so every
    // positional arg shifts by one and this function's `x` ends up bound
    // to `t.grooveShape` (a shape handle, not a number) on every frame
    // after the first. A strict throw here aborts the entire draw call
    // mid-frame (this project's draw lists are flat/sequential, no
    // per-call isolation), which was silently blanking out the whole
    // campaign menu screen behind the one widget that reached this path.
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
        // ENGINE PATCH: null-tolerant vector-form translate(null, [z]) --
        // confirmed real, live crash FlareBuddyCannon.gm:141: `this.
        // drawList.translate(this.animOffset);` where `this.animOffset` is
        // a real vector2D field that simply hasn't been assigned yet on an
        // entity this engine's own lazy-configure sweep (entity_bindings.cpp's
        // ConfigureUnconfiguredEntities) now reaches earlier in its
        // lifecycle than before -- previously-dormant code, not a newly
        // introduced null. Falling through to the strict 2-numeric-arg
        // branch below threw "expecting 2 param(s)" (a 1-arg vector call,
        // not a 2-arg numeric one), aborting draw() every frame. A null
        // translate is a no-op translate (x=y=0), matching every other
        // null-tolerant leniency this engine already applies elsewhere
        // (BC_GETDOT/BC_SETDOT, null arithmetic) rather than a hard abort.
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

// Confirmed radians: FUN_0008470c (drawList's real rotateZ) adds the raw
// float argument directly into the angle field with no unit conversion
// (no *pi/180 or similar), and every other angle-bearing native in this
// codebase (MathUtil, CameraHelper) already works in radians -- consistent,
// not independently reconfirmed here.
int GM_CDECL gmf_DC_rotate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    // Lenient rather than GM_FLOAT_OR_INT_PARAM's usual "present but wrong
    // type -> exception": confirmed real per-entity draw() hooks (e.g.
    // EntityPacks/GameStuff/Checkpoint.gm's `this.drawList.rotateZ(this.
    // mainpb.getDir());`) call this with a value that's null whenever the
    // entity's own physics-body setup didn't fully complete (a separate,
    // not-yet-root-caused gap upstream of this draw call) -- since a GM
    // exception aborts the ENTIRE calling draw() function, not just this
    // one statement, that one null angle was silently discarding every
    // other real, correct draw call later in the same entity's draw()
    // (same "one bad entity taking the rest down with it" failure mode
    // documented elsewhere in this engine, e.g. gmf_entity_setUIDData's
    // header comment) -- defaulting to no rotation here is a strictly
    // better fallback than losing the whole entity's visuals over it.
    // ENGINE PATCH (2026-08-17): vector2D form. Real call sites pass a
    // normalized direction vector, not a scalar angle -- Player.gm:5489/
    // 5704's `drawList.rotateZ(this.lastEyeDirDrawn)` /
    // `drawList.rotateZ(this.mainpb.getVel().getNormalized())` (both real
    // Vector2D values, confirmed via getNormalized()'s own binding
    // returning a Vector2D, not a float). FUN_0008470c's decompiled body
    // only shows a bare float-or-int read with no vector handling, but that
    // reflects the SCALAR overload (Checkpoint.gm's `rotateZ(this.mainpb.
    // getDir())`, where getDir() is confirmed to return a float) -- passing
    // a Vector2D through that same blind int-cast path would reinterpret
    // the object's reference bits as a numeric angle, producing an
    // effectively random rotation every frame, which is visibly wrong
    // (confirmed live: player sprite never rotated to face its movement
    // direction, staying flat regardless of slope/velocity) and not a
    // plausible real behaviour. Converting a direction vector to its
    // rotateZ angle via atan2(y,x) is the only interpretation consistent
    // with both call sites actually working.
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

// RETAIL BYTECODE AUDIT: DrawList.scale is FUN_00083280. Raw PPC shows it
// builds a transform and calls FUN_0031bdb4, the native matrix-compose
// kernel. That confirms scale participates in matrix composition. The
// desktop post-multiplies a diagonal 2D matrix, consistent with the
// verified current*new order used by the neighboring matrix operations.
// The scale-matrix assembly includes paired-vector instructions at 0x832d0,
// exact parameter replication (including one- versus two-argument behavior)
// The separate Video.scale function is not evidence for DrawList.scale's
// argument contract. Do not change the desktop matrix behavior from a visual
// guess alone.
//
// Runtime note: lenient numeric reads are retained because null arguments
// occur in live game calls; unlike the retail bytecode path, their fallback
// semantics have not been verified and remain an API compatibility question.
// Lenient argument reads, for exactly the reason gmf_DC_translate documents
// at length: GM_FLOAT_OR_INT_PARAM expands to the vendored gmscript's STRICT
// variant, which throws a blank-message GM_EXCEPTION the moment a parameter
// exists but isn't numeric -- and a GM exception aborts the ENTIRE calling
// draw() function, not just the offending statement.
//
// Confirmed live, not hypothetical: `this.drawList.scale(this.dirSign, 1)`
// in EntityPacks/GoombaCritterGuitar.gm:495 and EntityPacks/BlockCannon.gm:311
// runs every frame with `dirSign` still null on entities whose facing hasn't
// been resolved yet, and it threw roughly 1900 times over a single tutorial
// run -- silently discarding every other draw call those entities make. A
// null scale is a no-op scale (1,1), matching the null-tolerant handling
// translate and rotateZ already apply to the same class of not-yet-assigned
// entity field.
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

// Retail DrawList.multMatrix (0x00080958) records opcode 0x12 together with
// the native matrix pointer. Read that matrix when the command is replayed:
// scripts update a stored pulse matrix without rebuilding the draw list.
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

// Retail VMDRAWLIST.clear is wrapper 0x800e0. It appends opcode 4 with mode
// 9 when called without arguments, or the raw first argument when called
// with one. Replay draws the prebuilt clear-screen VMSHAPE at GameSystem
// +0x8154 via FUN_0030a278, then restores render state. This is a draw, not
// a framebuffer clear: opcode 4 applies mode-selected color-mask, depth-write,
// stencil-op, and render-config branches while drawing the clear shape. The
// old alpha>=1 glClear branch erased depth/stencil buffers without following
// those branches.
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

// setTextureByHandle(texture)/clearTexture() -- confirmed real (Ghidra
// recorded opcode into the draw list (0xd/0xe respectively), not an
// immediate GL state change -- same "record now, apply at replay" model
// this engine's own DrawListData already uses for every other primitive
// (see draw_list.h's header comment). Was entirely unbound on drawList
// instances (Video.setTextureByHandle exists but is a DIFFERENT, genuinely
// unimplemented global-table function -- see its own GE3_REMOVED marker --
// the real call sites are always `this.drawList.setTextureByHandle(...)`,
// a distinct per-instance method). Confirmed live, HALTING crash (this
// engine's default GE3_STRICT=halt mode aborts the whole process on any
// unbound native call, not just logging and continuing like survey mode):
// SafeVine.gm:291, reached on the very first level that places a vine --
// every real player hit this immediately on starting a level with one.
// Sets/clears DrawContext::currentTextureHandle (see its own header
// comment for why a plain field, not a stack, matches every real call
// site's flat bind/draw-N/unbind pattern); DC_gmf_drawShape (shape_
// bindings.cpp) checks it at REPLAY time to decide flat-colour vs
// textured-quad drawing.
int GM_CDECL DC_gmf_setTextureByHandle(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    auto resourceLease = ResolveTextureResourceLease(*a_thread->GetMachine(), a_thread->Param(0));
    return DC_RecordOrRun(a_thread, [handle, resourceLease] {
        GetDrawContext().currentTextureHandle = handle;
    });
}
// Bytecode verified: VMDRAWLIST.setTexture (EBOOT 0x7c2f4) delegates to
// FUN_0003d83c, which loads from FILEHELPER_AREA_GAMEDATA (area 1), retains
// the texture in the list's resource vector, and appends the same opcode 0xd
// used by setTextureByHandle. The desktop closure holds the matching resource
// lease through deferred replay.
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

// Bytecode verified against VMDRAWLIST.drawModTexture (EBOOT 0x7bea4): it
// retains arg 0 in the list's mod-texture resource vector and appends opcode
// 0x0a with that vector's index. Retail FUN_00044d4c skips opcode 0x0a without
// issuing a draw, so replay preserves ordering but has no visual operation.
// The DrawList root below supplies the lifetime retention that the native
// list's intrusive reference vector provides.
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

// setTextureSubImg(texture, palette) -- confirmed real (Ghidra decompile,
// from setTextureByHandle's (0xd). Retail retains both objects in the
// drawlist, then dispatcher opcode 0xf assigns palette to image +0x4c via
// FUN_0030a7e0. This relationship persists beyond the current draw call.
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

// RETAIL BYTECODE VERIFIED: the Video wrapper at EBOOT 0x7d510 accepts only
// 9/10/11/13 arguments and forwards the corners to FUN_0003e6f0. That helper
// submits a texture-backed shape, then clears the texture state with opcode
// 0x0e. Desktop now mirrors the valid-handle bind/draw/clear lifetime during
// deferred replay; exact native resource-vector identity remains unmodeled.
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
        // FUN_0003f1a4 forwards args 9/10 as UV extents. FUN_0003e6f0
        // multiplies these by TEXTURE +0x34/+0x38 respectively.
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

    // Was passing raw script-space corners straight to the renderer,
    // completely ignoring the current pushMatrix/translate/rotate/scale
    // stack every other draw primitive (shapes, text) already goes
    // through via DC_TransformPoint (see shape_bindings.cpp). Confirmed
    // real bug via the MainMenu5PS3 logo screen: HomeScreenBase.gm draws
    // its full-screen quad as `drawList.translate(scrWidth/2,scrHeight/2);
    // drawTextureByHandle(-scrWidth/2,-scrHeight/2, ...)`, and without
    // this transform the translate was silently dropped, leaving the quad
    // centred on the origin instead of the screen centre -- exactly the
    // "logo stuck in the bottom-left quadrant" symptom. Transform is
    // resolved at DRAW time (inside the deferred lambda), not here -- same
    // reasoning as PrintImpl's deferred z/matrix read above.
    return DC_RecordOrRun(a_thread, [x0, y0, x1, y1, x2, y2, x3, y3, handle, extraZ,
                                     uvScaleU, uvScaleV,
                                     textureShaderType, shapePrimitiveMode,
                                     resourceLease] {
        // RETAIL PPC: FUN_0003e6f0 attaches the texture, submits its shape,
        // then appends clearTexture (opcode 0x0e). A null texture returns
        // before emitting commands. Mirror that binding lifetime so later
        // plain drawShape calls cannot inherit an earlier texture.
        if (handle == 0) return;
        DrawContext& drawContext = GetDrawContext();
        drawContext.currentTextureHandle = handle;
        const DrawContext::Mat& m = GetDrawContext().Top();
        Renderer* renderer = GetEngineContext().renderer;
        // Opt-in diagnostic for the loading-screen matrix audit: record both
        // the script-space corners and the matrix active when the deferred
        // draw command executes. This separates Main.gm's dimension math from
        // DrawList matrix replay without changing either path.
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

// RETAIL PPC: VMDRAWLIST.drawTexture at 0x7c6d0 accepts counts 10, 11, 12,
// and 14. PPC/decomp confirm the 11/12-arg paths use corners, optional Z,
// filename, optional retained native object, and area. The 14-arg path reads
// scalar slots 8-10, retains slot 12, and calls FUN_0003f24c. LimiterWall.gm's
// commented 14-arg call labels these as Z, nx, ny, filename, VMSHAPE, area.
// PPC for FUN_0003f24c copies nx/ny into a 2-float pair passed to
// FUN_0003e6f0; that helper multiplies the texture width/height by the pair
// when writing UVs. Thus using nx/ny as UV scales is bytecode-confirmed. The
// native path queues its texture command and retains the supplied object in
// the draw-list resource table; desktop currently roots the object through
// deferred playback, but does not reproduce the retail resource-index record.
// The 9-argument game-data spelling is a desktop compatibility extension
// (retail returns -1); checked-in TouchEditProcess.gm uses it.
// RETAIL BYTECODE VERIFIED: Video.drawTexture at EBOOT 0x7c6d0 accepts
// 10/11/12/14 arguments; valid calls reach FUN_0003f24c/FUN_0003e6f0, which
// submits a texture-backed shape and appends clearTexture. The 9-argument
// game-data form is a documented compatibility extension. Desktop mirrors
// the valid-handle binding lifetime but retains its direct-quad renderer.
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
        // RETAIL PPC: this overload reaches the same FUN_0003e6f0 path and
        // clears the texture binding after submitting the quad. Null handles
        // exit before changing the draw-list state.
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
