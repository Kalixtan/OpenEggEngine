#include "draw_context.h"
#include "draw_list.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "../bindings/colour_bindings.h"
#include "../bindings/texture_bindings.h"
#include "../bindings/vector2d_bindings.h"
#include "gmTableObject.h"
#include <functional>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>

DrawContext& GetDrawContext() {
    static DrawContext ctx;
    return ctx;
}

int DC_RecordOrRun(gmThread* a_thread, std::function<void()> a_apply) {
    if (DrawListData* rec = RecordingTarget(a_thread)) { rec->commands.push_back(std::move(a_apply)); return GM_OK; }
    a_apply();
    return GM_OK;
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

// 0x00082b0c): sets a transparent-pass draw-order/sort key, distinct from
// the opaque depth-test Z this engine already models (see Renderer's
// GL_EQUAL depth-layering comment). Real call site (BaseNotePlant.gm:1025/
// 1029: `drawList.setOrderTransparent(WIZARD_MODE_NOTE_Z);`, right after
// `drawList.translate(notePos, z)` already placed the note at that exact
// Z) -- this engine draws everything in a single painter's-algorithm pass
// with no separate transparent-object sort bucket to feed a second sort
// key into, so genuine no-op (the translate() call already did the real
// positioning work).
int GM_CDECL DC_gmf_setOrderTransparent(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// sets a 2-bit draw-order/tier field on the DrawList's internal struct to
// its maximum value (`& 0xcfffffff | 0x30000000`), the same "which sort
// bucket does this belong to" mechanic as setOrderTransparent just above,
// just picking the opposite end of the tier range. Real call site
// (TutorialMessage.gm:95, right before `pushMatrix()`/`translate(pos,z)`)
// already picks an explicit Z from PLAYER_MODE_NOTE_Z/FRONT_Z (this
// engine's existing single-pass GL_EQUAL depth-layering already places it
// correctly) -- setOrderLast is a same-Z tie-breaker for retail's
// multi-bucket sorted renderer, which this engine's single painter's-
// algorithm pass has no equivalent bucket system for. Same architecture
// gap and same no-op resolution as setOrderTransparent.
int GM_CDECL DC_gmf_setOrderLast(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// setDistanceTextureRenderAttributes(near, far) -- confirmed real
// texture fade/blend range (a shader uniform pair). Real call site
// (BaseNotePlant.gm:1054: `drawList.setDistanceTextureRenderAttributes(
// 0.5f-u, 0.55f+u);`, right before drawTextureByHandle) -- this engine's
// textured-quad renderer has no per-draw distance-fade shader stage,
// genuine no-op.
int GM_CDECL DC_gmf_setDistanceTextureRenderAttributes(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
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

// setColour -- now DECOMPILED-CONFIRMED via live Ghidra cross-reference
// matrixstack_setColour.txt): Video's own OPD-resolved body is
// FUN_00076578 @ 0x00076578, drawList's is FUN_0007e6c8 @ 0x0007e6c8 --
// TWO SEPARATE native functions (not one shared implementation the way
// this engine's DC_gmf_setColour singleton assumed), differing in how they
// STORE the result (Video appends a colour-set opcode to its deferred
// command stream; drawList appends into its own growable float buffer)
// but IDENTICAL in dispatch contract, confirmed byte-for-byte across both
// bodies:
//   1 arg:  arg0 type-dispatch -- GM_INT(1)=colour ID, GM_STRING(0x25)=
//           colour name, COLOUR-type=read r/g/b off the object directly.
//           Alpha defaults (a per-function rodata float constant, exact
//           value not pinned down, but matches this engine's existing 1.0
//           default -- nothing in the corpus distinguishes it from 1.0).
//   2 args: same arg0 dispatch, alpha = explicit arg1.
//   3 args: pure numeric (r,g,b), alpha implicit (same default).
//   4 args: pure numeric (r,g,b,a), alpha explicit.
// This is EXACTLY the shape this file's previous (corpus-inferred)
// implementation already used -- confirms rather than changes that
// dispatch logic. The three GM_INT/GM_STRING/COLOUR-type branches also
// match colour_bindings.cpp's ResolveColour() precisely (by-name registry
// lookup / by-ID array lookup / direct object read), so ResolveColourRGB
// below is reused as-is. What's NOT replicated is retail's actual storage
// (opcode stream / growable buffer) -- this engine's SetColour() applies
// the result immediately to the current draw colour instead, matching
// this project's stated principle for the renderer layer (docs/
// ARCHITECTURE.md: "this is the one place where 'matches retail' means
// matching visible output, not matching implementation") since backing
// that exact storage mechanism would require replicating retail's whole
// deferred-command-stream renderer, not just this one call.
int GM_CDECL gmf_DC_setColour_video(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable arg0 = a_thread->Param(0);
    float r, g, b;
    bool tryNamedForm = (a_thread->GetNumParams() < 3);
    if (tryNamedForm && ResolveColourRGB(a_thread, arg0, r, g, b)) {
        float a = 1.0f;
        if (a_thread->GetNumParams() >= 2) { GM_FLOAT_OR_INT_PARAM(alpha, 1, 1.0f); a = alpha; }
        return DC_RecordOrRun(a_thread, [r, g, b, a] { GetEngineContext().renderer->SetColour(r, g, b, a); });
    }

    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(rr, 0, 1.0f);
    GM_FLOAT_OR_INT_PARAM(gg, 1, 1.0f);
    GM_FLOAT_OR_INT_PARAM(bb, 2, 1.0f);
    GM_FLOAT_OR_INT_PARAM(aa, 3, 1.0f);
    return DC_RecordOrRun(a_thread, [rr, gg, bb, aa] { GetEngineContext().renderer->SetColour(rr, gg, bb, aa); });
}
// drawList.setColour -- same confirmed dispatch, see comment above
// (FUN_0007e6c8's body is byte-for-byte the same shape as Video's
// FUN_00076578, just storing into a different buffer natively).
int GM_CDECL gmf_DC_setColour_drawList(gmThread* a_thread) {
    return gmf_DC_setColour_video(a_thread);
}
// drawList.setColourRef -- confirmed real (EBOOT.elf, function named
// "setColourRef" already in Ghidra's own symbol table @ 0x0007ebac) but a
// GENUINELY DIFFERENT function from setColour, not an alias -- takes a
// name/id-shaped first argument feeding a lookup-and-store helper
// (FUN_002b6af4 among others), not the r/g/b dispatch above. Not yet
// decompiled/understood well enough to implement -- stays trapped.
GE3_REMOVED(gmf_DC_setColourRef, "drawList.setColourRef")

int GM_CDECL DC_gmf_setFontRenderAttributes(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(edge, 0, 0.5f);
    GM_FLOAT_OR_INT_PARAM(width, 1, 0.5f);
    return DC_RecordOrRun(a_thread, [edge, width] {
        DrawContext& ctx = GetDrawContext();
        ctx.fontEdge = edge;
        ctx.fontWidth = width;
    });
}

int GM_CDECL DC_gmf_resetFontRenderAttributes(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        DrawContext& ctx = GetDrawContext();
        ctx.fontEdge = 0.5f;
        ctx.fontWidth = 0.5f;
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
int GM_CDECL DC_gmf_printUntranslated(gmThread* a_thread) { return PrintImpl(a_thread, false); }
int GM_CDECL DC_gmf_printArray(gmThread* a_thread) { return PrintArrayImpl(a_thread); }

// SOUND SHAPES: takes an OPTIONAL trailing boolean. Confirmed from its own
// it records FUN_0003e198(list, 0), with 1 it records FUN_0003e198(list,
// arg != 0), and any other count returns -1. What the flag selects is NOT
// recoverable from the recorder alone -- it is consumed downstream in the
// RSX command path (FUN_0004cea0, unanalysed in the committed Ghidra
// project) -- and all 55 corpus call sites use the bare 0-argument form, so
// nothing here can currently distinguish the two states. Accepted and
// ignored rather than guessed at; flagged so the flag is not mistaken for
// an argument retail does not have.
int GM_CDECL DC_gmf_setupWorldView(gmThread* a_thread) {
    // Establishes the world->screen projection for subsequent draws this
    // frame -- our Renderer recomputes this per-draw-call already (fixed
    // ortho, see Renderer::kWorldHalfHeight), so this used to be a pure
    // no-op marker call. Now also flips on camera-relative projection
    // (Renderer::SetUseCameraView) -- setupWorldView() is real retail's
    // signal that subsequent draws are camera-relative gameplay content,
    // as opposed to setupOrthoView()'s screen-fixed UI content, confirmed
    // by real usage (Platformer/Main.gm's preDraw() calls
    // Video.setCameraPos(...) then relies on world-space draws tracking
    // it, while menu screens never move the camera at all).
    return DC_RecordOrRun(a_thread, [] {
        GetEngineContext().renderer->SetViewMode(Renderer::ViewMode::WorldPerspective);
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
    return DC_RecordOrRun(a_thread, [] {
        GetEngineContext().renderer->SetViewMode(Renderer::ViewMode::AspectRatioOrtho);
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

// scale -- drawList's own candidate body (FUN_00083280) genuinely can't be
// decompiled with this toolchain: it hits an unrecognized PowerPC opcode
// (0x7c43340e @ 0x000832d0, confirmed via raw disassembly -- not a
// disassembly-alignment mistake, a real instruction Ghidra's stock
// PowerPC processor module doesn't have a constructor for, almost
// certainly a Cell PPU paired-single/VMX float op the installed processor
// spec doesn't model; the installed "Ps3GhidraScripts" extension only adds
// syscall/NID tables, not a fuller Cell processor definition). Confirmed
// NOT an artifact of -noanalysis: reran with a full targeted analysis
// pass first (AnalyzeAndDecompile.py) and the same address still fails
// the same way.
// Video's OWN scale (FUN_000802d8 @ 0x000802d8, opcode-stream based, see
// this cluster's other comments for why that storage mechanism itself
// isn't replicated) DID decompile cleanly, and gives the confirmed
// argument contract both share: 1 arg = uniform scale (x=y=arg0), 2+ args
// = non-uniform (x=arg0, y=arg1). Applied here the same way translate/
// rotateZ are -- composed into this engine's existing affine matrix stack.
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

// Was assumed to be a no-op like Video.clearDepthBuffer/clearColourBuffer
// confirmed via direct RPCS3 comparison: a script that does
// `setColour(1,0,1,1); clear(VIDEO_ALLSURFACES);` shows a solid magenta
// background in real retail, not whatever was drawn before. Clears to the
// current draw colour, same as every other draw call.
// Confirmed real dual use: Main.gm's preDraw() calls
// `COLOURSCHEME_SetColour("clear",drawList); drawList.clear();` once per
// frame as the opaque background wash (wants a real full overwrite,
// current alpha always 1), but also mid-frame for fade transitions
// (`drawList.setColour(0,0,0,this.niceExitData.fadePerc); drawList.
// clear();`, ScreenTransitionProcess-style code) where alpha is meant to
// blend translucently over whatever's already drawn. A hardware glClear
// ignores alpha entirely and always fully overwrites the framebuffer, so
// every fade-out snapped straight to a solid black screen the instant
// fadePerc left 0 instead of ramping smoothly -- exactly the "screens of
// full black" symptom. Below-opaque alpha now draws a blended full-screen
// quad (Renderer::DrawFullscreenOverlay) instead of clearing; alpha>=1
// keeps the real hardware clear (fast path, matches the original opaque
// use unchanged).
// ENGINE PATCH (FoE compat): FoE's own preDraw() convention passes the
// colour straight into clear() -- `Video.clear("clear", local0 +
// this.transitionPerc*(1-local0))` (PekkoAndJon/Main.gm) -- rather than
// Sound Shapes' separate `COLOURSCHEME_SetColour("clear",drawList);
// drawList.clear();` two-call idiom this function was originally built
// for (see header comment above). With 0 args this still reads whatever
// setColour last left current (Sound Shapes path, unchanged); with 1+
// args it resolves them the SAME way gmf_DC_setColour_video does (name/
// COLOUR-object first, falling back to bare r,g,b,[a]) and applies that
// AS the clear colour, mirroring setColour's own persistent side effect
// so a later bare clear() still sees it. Without this FoE's title-screen
// background wash (meant to fade from black to a cream "clear" colour as
// transitionPerc ramps up) silently no-op'd on its own 2-arg call,
// leaving every frame cleared to Renderer::BeginFrame's hardcoded black.
int GM_CDECL DC_gmf_clear(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n >= 1) {
        gmVariable arg0 = a_thread->Param(0);
        float r, g, b, a = 1.0f;
        bool tryNamedForm = (n < 3);
        if (tryNamedForm && ResolveColourRGB(a_thread, arg0, r, g, b)) {
            if (n >= 2) { GM_FLOAT_OR_INT_PARAM(alpha, 1, 1.0f); a = alpha; }
            return DC_RecordOrRun(a_thread, [r, g, b, a] {
                GetEngineContext().renderer->SetColour(r, g, b, a);
                if (a >= 1.0f) GetEngineContext().renderer->ClearScreen();
                else GetEngineContext().renderer->DrawFullscreenOverlay(GetDrawContext().Top().z);
            });
        }
        if (n >= 3) {
            GM_FLOAT_OR_INT_PARAM(rr, 0, 1.0f);
            GM_FLOAT_OR_INT_PARAM(gg, 1, 1.0f);
            GM_FLOAT_OR_INT_PARAM(bb, 2, 1.0f);
            GM_FLOAT_OR_INT_PARAM(aa, 3, 1.0f);
            return DC_RecordOrRun(a_thread, [rr, gg, bb, aa] {
                GetEngineContext().renderer->SetColour(rr, gg, bb, aa);
                if (aa >= 1.0f) GetEngineContext().renderer->ClearScreen();
                else GetEngineContext().renderer->DrawFullscreenOverlay(GetDrawContext().Top().z);
            });
        }
    }
    return DC_RecordOrRun(a_thread, [] {
        Renderer* renderer = GetEngineContext().renderer;
        float r, g, b, a;
        renderer->GetColour(r, g, b, a);
        if (a >= 1.0f) renderer->ClearScreen();
        else renderer->DrawFullscreenOverlay(GetDrawContext().Top().z);
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
    return DC_RecordOrRun(a_thread, [handle] {
        GetDrawContext().currentTextureHandle = handle;
    });
}
int GM_CDECL DC_gmf_clearTexture(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        GetDrawContext().currentTextureHandle = 0;
    });
}

// setTextureSubImg(texture, palette) -- confirmed real (Ghidra decompile,
// from setTextureByHandle's (0xd), taking a texture AND a palette handle --
// real usage always pairs it immediately before setTextureByHandle for the
// SAME texture (SafeVine.gm:290-291: `setTextureSubImg(thornTexture,
// thornPalette); setTextureByHandle(thornTexture);`), suggesting a palette-
// remap/sub-region bind this renderer's plain whole-texture model has no
// equivalent for (no texture-atlas or palette-swap support exists here).
// Approximated as an ordinary whole-texture bind, same as setTextureByHandle
// (ignoring the palette arg) -- was entirely unbound on drawList instances,
// a HALTING crash (GE3_STRICT defaults to halt, not survey) reached before
// setTextureByHandle's own fix even mattered, since this is called first in
// every real sequence.
int GM_CDECL DC_gmf_setTextureSubImg(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(0));
    // Real signature takes (texture, palette) -- see texture_bindings.h's
    // PALETTE comment. COLOURSCHEME_SetGeneratedPaletteColour(palette,...)
    // can only stash pending fill/outline colours on the palette object
    // (it has no texture yet); this is the point real usage always pairs
    // a palette with its texture, so forward the tint on to the real
    // texture handle now.
    if (handle != 0 && a_thread->GetNumParams() >= 2) {
        PaletteData* pal = ResolvePaletteData(*a_thread->GetMachine(), a_thread->Param(1));
        if (pal && pal->hasPendingTint) {
            GetEngineContext().renderer->SetTextureGeneratedTint(
                handle, pal->fillR, pal->fillG, pal->fillB, pal->outlineR, pal->outlineG, pal->outlineB);
        }
    }
    return DC_RecordOrRun(a_thread, [handle] {
        GetDrawContext().currentTextureHandle = handle;
    });
}

int GM_CDECL DC_gmf_drawTextureByHandle(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(9);
    GM_FLOAT_OR_INT_PARAM(x0, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y0, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x1, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y1, 3, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x2, 4, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y2, 5, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x3, 6, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y3, 7, 0.0f);
    // Two real call shapes confirmed by corpus usage: the plain 9-arg form
    // (corners + handle), and an 11-arg form with an extra Z depth and a
    // trailing "textureQuad" arg between the corners and the handle
    // (PauseProcessBase.gm's clock/note-coin icons: `drawTextureByHandle(
    // ...corners..., 0, this.clockTexture, this.parent.textureQuad)`) --
    // textureQuad is just the same reusable unit-square Shape every other
    // panel background uses (`VIDEOHELPER_DrawFilledPolygon`), not texture
    // UV data, so it doesn't affect the draw and is safe to ignore; the
    // real bug this fixes is that param(8) being treated unconditionally
    // as the texture handle meant the 11-arg form resolved the literal
    // int 0 (the Z arg) as a texture handle instead -- always drawing a
    // solid black quad (a zero/invalid handle) regardless of the real
    // texture two args later. Detected by type: a real handle is never a
    // plain number.
    float extraZ = 0.0f;
    int handleParamIdx = 8;
    const gmVariable& p8 = a_thread->Param(8);
    if ((p8.m_type == GM_INT || p8.m_type == GM_FLOAT) && a_thread->GetNumParams() >= 10) {
        extraZ = a_thread->ParamFloatOrInt(8, 0.0f);
        handleParamIdx = 9;
    }
    unsigned int handle = ResolveTextureHandle(*a_thread->GetMachine(), a_thread->Param(handleParamIdx));

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
    return DC_RecordOrRun(a_thread, [x0, y0, x1, y1, x2, y2, x3, y3, handle, extraZ] {
        const DrawContext::Mat& m = GetDrawContext().Top();
        float tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3;
        DC_TransformPoint(m, x0, y0, tx0, ty0);
        DC_TransformPoint(m, x1, y1, tx1, ty1);
        DC_TransformPoint(m, x2, y2, tx2, ty2);
        DC_TransformPoint(m, x3, y3, tx3, ty3);
        GetEngineContext().renderer->DrawTexturedQuad(tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3, handle, m.z + extraZ);
    });
}

// drawTexture(x0,y0,x1,y1,x2,y2,x3,y3,filename,[area]) -- confirmed real,
// distinct from drawTextureByHandle: takes a raw filename + FILEHELPER_AREA_*
// instead of a pre-resolved handle (EntityPacks/TutorialGraphicsEntity.gm's
// in-level tutorial-hint graphics -- the "Welcome to Sound Shapes!"/move-
// jump-icon overlays confirmed via the real RPCS3 reference screenshots --
// `this.drawList.drawTexture(-hw,0,hw,0,hw,this.height,-hw,this.height,
// this.ad.filename,FILEHELPER_AREA_GAMEDATA)`), entirely unregistered.
// Renderer::LoadTexture already caches by resolved path, so resolving +
// loading fresh every call is cheap after the first frame (same as
// Video.getTexture already relies on for its own every-call-site pattern).
int GM_CDECL DC_gmf_drawTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(9);
    GM_FLOAT_OR_INT_PARAM(x0, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y0, 1, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x1, 2, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y1, 3, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x2, 4, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y2, 5, 0.0f);
    GM_FLOAT_OR_INT_PARAM(x3, 6, 0.0f);
    GM_FLOAT_OR_INT_PARAM(y3, 7, 0.0f);
    GM_STRING_PARAM(path, 8, "");
    // param 9 (FILEHELPER_AREA_*) -- only GAMEDATA matters for now, same as
    // Video.getTexture's own area handling.

    unsigned int handle = 0;
    EngineContext& ctx = GetEngineContext();
    auto resolved = ctx.fileHelper->Resolve(path);
    if (resolved) handle = ctx.renderer->LoadTexture(*resolved);

    return DC_RecordOrRun(a_thread, [x0, y0, x1, y1, x2, y2, x3, y3, handle] {
        const DrawContext::Mat& m = GetDrawContext().Top();
        float tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3;
        DC_TransformPoint(m, x0, y0, tx0, ty0);
        DC_TransformPoint(m, x1, y1, tx1, ty1);
        DC_TransformPoint(m, x2, y2, tx2, ty2);
        DC_TransformPoint(m, x3, y3, tx3, ty3);
        GetEngineContext().renderer->DrawTexturedQuad(tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3, handle, m.z);
    });
}
