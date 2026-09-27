#include "animation_bindings.h"
#include "gmThread.h"
#include "../render/fsa_bci.h"
#include "../render/draw_context.h"
#include "../render/draw_list.h"
#include "../core/engine_context.h"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

gmType g_animType = -1;

// See fsa_bci.h's header comment for why this palette model (index 0
// transparent, index 1 == the fill colour, ramping toward the outline
// colour as the index rises) is the confirmed-plausible real scheme --
// visually verified against a decoded real frame (spawnRecordPlayerIdle,
// the player-spawner "record" animation), not guessed from nothing.
struct AnimationData {
    FsaAsset asset;

    // Playback: real API is playTime (total ms for one full pass through
    // every frame, confirmed via SpawnPoint.gm's `this.idleAnim.
    // setPlayTime(250);`), a wall-clock start time, loop vs one-shot, and
    // direction -- computed on demand (getCurrentFrame/isDone/draw) from
    // elapsed real time rather than needing an explicit per-frame update()
    // call, since nothing in the corpus calls one.
    float playTimeMs = 500.0f;
    float playStartMs = 0.0f;
    bool playing = false;
    bool looping = false;
    bool reversed = false;
    int pausedFrame = 0;

    bool haveTint = false;
    float outR = 1, outG = 1, outB = 1;
    float fillR = 1, fillG = 1, fillB = 1;

    // One generated RGBA texture per source frame, valid only while
    // texValid is true -- regenerated wholesale the first time a tint is
    // set and whenever it changes (SetAnimationPaletteColour). Small
    // sprites (tens x tens of pixels), so redoing every frame's texture on
    // a tint change is cheap; per-instance tints rarely change at all in
    // practice (each entity type sets its colour names once).
    std::vector<unsigned int> frameTex;
    bool texValid = false;
    float lastOutR = -1, lastOutG = -1, lastOutB = -1;
    float lastFillR = -1, lastFillG = -1, lastFillB = -1;
};

AnimationData* ThisAnim(gmThread* a_thread) {
    return (AnimationData*)a_thread->ThisUserCheckType(g_animType);
}

int NumFrames(const AnimationData* a) { return (int)a->asset.frames.size(); }

int ComputeFrameIndex(const AnimationData* a) {
    int n = NumFrames(a);
    if (n <= 0) return 0;
    if (!a->playing) return std::clamp(a->pausedFrame, 0, n - 1);
    float now = GetEngineContext().gameSystem.GetMsAlive();
    float elapsed = now - a->playStartMs;
    if (elapsed < 0.0f) elapsed = 0.0f;
    float perFrame = a->playTimeMs / (float)n;
    if (perFrame <= 0.0001f) perFrame = 1.0f;
    int idx = (int)(elapsed / perFrame);
    if (a->looping) idx = idx % n;
    else if (idx >= n) idx = n - 1;
    if (a->reversed) idx = n - 1 - idx;
    return idx;
}

bool ComputeIsDone(const AnimationData* a) {
    if (NumFrames(a) <= 0) return true;
    // NOT "if (!a->playing) return true" -- a freshly-loaded/never-played
    // clip also has playing=false (its default), and treating that as
    // "done" made isDone() true before playOnce() had ever run. Real
    // callers (e.g. SpawnPoint.gm's TRIGGER state: `playOnce(); ... if
    // (triggerAnim.isDone()) changeState(...)`) call playOnce() then check
    // isDone() in the same or a later update() -- with the old logic this
    // fired true on the very first frame (before/without ever actually
    // playing), skipping the whole animation and its state straight to the
    // next one. A clip that hasn't started (or was pause()'d mid-way) isn't
    // "finished" -- only "played non-looping to completion" is.
    if (!a->playing) return false;
    if (a->looping) return false;
    float now = GetEngineContext().gameSystem.GetMsAlive();
    return (now - a->playStartMs) >= a->playTimeMs;
}

void FreeTextures(AnimationData* a) {
    Renderer* renderer = GetEngineContext().renderer;
    if (renderer) {
        for (unsigned int h : a->frameTex) {
            if (h) renderer->UnloadTexture(h);
        }
    }
    a->frameTex.clear();
    a->texValid = false;
}

void RebuildTextures(AnimationData* a) {
    FreeTextures(a);
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer || !a->haveTint) return;

    a->frameTex.assign(a->asset.frames.size(), 0);
    std::vector<unsigned char> rgba;
    for (size_t f = 0; f < a->asset.frames.size(); ++f) {
        const BciFrame& frame = a->asset.frames[f];
        if (frame.width <= 0 || frame.height <= 0) continue;
        size_t count = (size_t)frame.width * frame.height;
        rgba.assign(count * 4, 0);
        for (size_t i = 0; i < frame.indices.size() && i < count; ++i) {
            uint8_t idx = frame.indices[i];
            float r, g, b, al;
            if (idx == 0) {
                r = g = b = al = 0.0f;
            } else {
                float t = (float)(idx - 1) / 14.0f;
                r = a->fillR + (a->outR - a->fillR) * t;
                g = a->fillG + (a->outG - a->fillG) * t;
                b = a->fillB + (a->outB - a->fillB) * t;
                al = 1.0f;
            }
            rgba[i * 4 + 0] = (unsigned char)std::clamp(r * 255.0f, 0.0f, 255.0f);
            rgba[i * 4 + 1] = (unsigned char)std::clamp(g * 255.0f, 0.0f, 255.0f);
            rgba[i * 4 + 2] = (unsigned char)std::clamp(b * 255.0f, 0.0f, 255.0f);
            rgba[i * 4 + 3] = (unsigned char)std::clamp(al * 255.0f, 0.0f, 255.0f);
        }
        a->frameTex[f] = renderer->CreateTextureFromRGBA(rgba.data(), frame.width, frame.height);
    }
    a->texValid = true;
    a->lastOutR = a->outR; a->lastOutG = a->outG; a->lastOutB = a->outB;
    a->lastFillR = a->fillR; a->lastFillG = a->fillG; a->lastFillB = a->fillB;
}

int GM_CDECL gmf_anim_finishLoad(gmThread* a_thread) { return GM_OK; } // loaded synchronously already

int GM_CDECL gmf_anim_pause(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a->pausedFrame = ComputeFrameIndex(a);
    a->playing = false;
    return GM_OK;
}

int GM_CDECL gmf_anim_play(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a->looping = true;
    a->playing = true;
    a->playStartMs = GetEngineContext().gameSystem.GetMsAlive();
    return GM_OK;
}

// playOnce([resetToStart]) -- confirmed real 0-1 arg (SpawnPoint.gm calls
// both `parent.idleAnim.playOnce();` and `parent.triggerAnim.playOnce(true);`).
// Always restarts from frame 0 -- no real usage relies on resuming a
// paused one-shot mid-way, and every call site treats it as "start this
// one-shot now."
int GM_CDECL gmf_anim_playOnce(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a->looping = false;
    a->reversed = false;
    a->playing = true;
    a->playStartMs = GetEngineContext().gameSystem.GetMsAlive();
    return GM_OK;
}

int GM_CDECL gmf_anim_setCurrentFrame(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(frame, 0, 0);
    a->pausedFrame = frame;
    a->playing = false;
    return GM_OK;
}

int GM_CDECL gmf_anim_setIsReversed(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    a->reversed = a_thread->Param(0).m_type == GM_INT && a_thread->Param(0).m_value.m_int != 0;
    return GM_OK;
}

int GM_CDECL gmf_anim_setPaletteBank(gmThread* a_thread) { return GM_OK; } // no multi-bank palette system modelled

// 0x000a118c): takes either a float or int argument (both converted to the
// same double "position" value) and repositions playback, distinct from
// setCurrentFrame's discrete frame index -- every real call site (e.g.
// BlockCannon.gm:64-83, SwayingTree.gm's `this.headAni.setPlayPosition(0);
// this.headAni.playOnce();`) passes 0.0 immediately after setPlayTime() and
// before the clip is ever played, to rewind a freshly-loaded/reused clip to
// its start. Modelled the same way as this engine's existing playStartMs/
// pausedFrame split: while playing, shift playStartMs so elapsed time
// equals the requested position (keeps playing); while stopped, convert the
// ms position to the equivalent paused frame (mirrors setCurrentFrame),
// since ComputeFrameIndex only reads pausedFrame in that state.
int GM_CDECL gmf_anim_setPlayPosition(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(posMs, 0, 0.0f);
    if (a->playing) {
        a->playStartMs = GetEngineContext().gameSystem.GetMsAlive() - posMs;
    } else {
        int n = NumFrames(a);
        if (n > 0) {
            float perFrame = a->playTimeMs / (float)n;
            if (perFrame <= 0.0001f) perFrame = 1.0f;
            a->pausedFrame = std::clamp((int)(posMs / perFrame), 0, n - 1);
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_anim_setPlayTime(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(ms, 0, 500.0f);
    if (ms > 0.0f) a->playTimeMs = ms;
    return GM_OK;
}

int GM_CDECL gmf_anim_setShape(gmThread* a_thread) { return GM_OK; } // unclear real semantics, not load-bearing for any confirmed call site

int GM_CDECL gmf_anim_getCurrentFrame(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(ComputeFrameIndex(a));
    return GM_OK;
}
int GM_CDECL gmf_anim_getNumFrames(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(NumFrames(a));
    return GM_OK;
}
int GM_CDECL gmf_anim_getSourcePixelWidth(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(NumFrames(a) > 0 ? a->asset.frames[0].width : 0);
    return GM_OK;
}
int GM_CDECL gmf_anim_getSourcePixelHeight(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(NumFrames(a) > 0 ? a->asset.frames[0].height : 0);
    return GM_OK;
}
int GM_CDECL gmf_anim_getTextureExt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }

int GM_CDECL gmf_anim_isDone(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(ComputeIsDone(a) ? 1 : 0);
    return GM_OK;
}
// isPaused() -- real, load-bearing (36 real .gm call sites, always as
// `xxxAni.isDone()||xxxAni.isPaused()` right after a matching
// `xxxAni.setPlayTime(n); xxxAni.pause();` at setup), but genuinely
// isDone(), only the unrelated isDoneTransitioning) -- a known reveng gap,
// not evidence it's not real. Was entirely unimplemented/unregistered
// (silently resolving to null in every caller, always falsy), which
// combined with ComputeIsDone's own "!playing -> not done" logic meant
// `isDone()||isPaused()` could never become true for a clip that was only
// ever pause()'d and never played -- exactly the setup/idle-check pattern
// above -- permanently locking every such entity onto its "still
// mid-transition" draw branch instead of ever reaching its idle one.
// Simplest reading consistent with every real call site: paused is just
// "not currently playing" -- the direct complement of isPlaying() above.
int GM_CDECL gmf_anim_isPaused(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(!a->playing ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_anim_isPlaying(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt((a->playing && !ComputeIsDone(a)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_anim_isReversed(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(a->reversed ? 1 : 0);
    return GM_OK;
}

gmFunctionEntry s_animLib[] = {
    {"finishLoad", gmf_anim_finishLoad, nullptr},
    {"pause", gmf_anim_pause, nullptr},
    {"play", gmf_anim_play, nullptr},
    {"playOnce", gmf_anim_playOnce, nullptr},
    {"setCurrentFrame", gmf_anim_setCurrentFrame, nullptr},
    {"setIsReversed", gmf_anim_setIsReversed, nullptr},
    {"setPaletteBank", gmf_anim_setPaletteBank, nullptr},
    {"setPlayTime", gmf_anim_setPlayTime, nullptr},
    {"setPlayPosition", gmf_anim_setPlayPosition, nullptr},
    {"setShape", gmf_anim_setShape, nullptr},
    {"getCurrentFrame", gmf_anim_getCurrentFrame, nullptr},
    {"getNumFrames", gmf_anim_getNumFrames, nullptr},
    {"getSourcePixelWidth", gmf_anim_getSourcePixelWidth, nullptr},
    {"getSourcePixelHeight", gmf_anim_getSourcePixelHeight, nullptr},
    {"getTextureExt", gmf_anim_getTextureExt, nullptr},
    {"isDone", gmf_anim_isDone, nullptr},
    {"isPaused", gmf_anim_isPaused, nullptr},
    {"isPlaying", gmf_anim_isPlaying, nullptr},
    {"isReversed", gmf_anim_isReversed, nullptr},
};

void GM_CDECL AnimGCDestruct(gmMachine*, gmUserObject* a_object) {
    AnimationData* a = (AnimationData*)a_object->m_user;
    FreeTextures(a);
    delete a;
}

} // namespace

void RegisterAnimationBindings(gmMachine& a_machine) {
    g_animType = a_machine.CreateUserType("Animation");
    a_machine.RegisterTypeLibrary(g_animType, s_animLib, sizeof(s_animLib) / sizeof(s_animLib[0]));
    a_machine.RegisterUserCallbacks(g_animType, nullptr, AnimGCDestruct, nullptr);
}

void PushNewAnimation(gmThread* a_thread, const std::string& a_resolvedPath) {
    AnimationData* data = new AnimationData();
    if (!a_resolvedPath.empty()) {
        if (!data->asset.LoadFromFile(a_resolvedPath)) {
            std::fprintf(stderr, "Entity.loadAnimation: failed to parse '%s'\n", a_resolvedPath.c_str());
        }
    }
    a_thread->PushNewUser(data, g_animType);
}

bool SetAnimationPaletteColour(gmMachine& a_machine, const gmVariable& a_animVar,
                                float a_outR, float a_outG, float a_outB,
                                float a_fillR, float a_fillG, float a_fillB) {
    if (a_animVar.m_type != g_animType) return false;
    AnimationData* a = (AnimationData*)((gmUserObject*)a_machine.GetObject(a_animVar.m_value.m_ref))->m_user;
    if (!a) return false;
    if (a->haveTint && a->outR == a_outR && a->outG == a_outG && a->outB == a_outB &&
        a->fillR == a_fillR && a->fillG == a_fillG && a->fillB == a_fillB && a->texValid) {
        return true; // unchanged -- skip regenerating every frame's texture
    }
    a->outR = a_outR; a->outG = a_outG; a->outB = a_outB;
    a->fillR = a_fillR; a->fillG = a_fillG; a->fillB = a_fillB;
    a->haveTint = true;
    RebuildTextures(a);
    return true;
}

int GM_CDECL DC_gmf_drawAnimation(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable animVar = a_thread->Param(0);
    if (animVar.m_type != g_animType) return GM_OK; // lenient -- matches drawShape's null-arg tolerance
    AnimationData* a = (AnimationData*)((gmUserObject*)a_thread->GetMachine()->GetObject(animVar.m_value.m_ref))->m_user;
    if (!a) return GM_OK;

    // Quad spans LOCAL (0,0)-(1,1) -- bottom-left anchored, NOT centred on
    // the local origin. Confirmed by the two shared real helpers every
    // animated entity draws through (Platformer/EntityBase.gm, 104 real
    // .gm files call one of these): both
    // `ENTITYBASE_DrawAnimationCentered(x,y,z,w,h,anim,...)` and
    // `ENTITYBASE_DrawAnimationCenteredUsingBottomLeftMethod(x,y,z,w,h,lw,
    // lh,anim,...)` do the identical `dl.translate(x-w/2.0,y-h/2.0,z);
    // dl.scale(w,h); dl.drawAnimation(anim);` -- translating to what would
    // be the bottom-left corner of a (0,0)-(1,1) quad, then scaling by the
    // FULL width/height, is only self-consistent (and only actually lands
    // the sprite centred on (x,y), matching both functions' names) if
    // drawAnimation's own quad is (0,0)-(1,1). A centred (-0.5,-0.5)-
    // (0.5,0.5) quad here would double up with that translate, landing
    // every animated entity in the game a further half-width/half-height
    // down-left of where it should be.
    //
    // Frame index (ComputeFrameIndex) and transform are both resolved at
    // DRAW time (inside the deferred lambda, if this was called via a real
    // drawList instance -- see draw_list.h's RecordingTarget), not here:
    // an animation's current frame is time-based, so freezing it at
    // whatever it was when a cached sub-list was last rebuilt would be
    // wrong the moment that rebuild isn't happening every single frame.
    return DC_RecordOrRun(a_thread, [a] {
        if (!a->texValid) return;
        int idx = ComputeFrameIndex(a);
        if (idx < 0 || idx >= (int)a->frameTex.size()) return;
        unsigned int handle = a->frameTex[idx];
        if (!handle) return;
        const DrawContext::Mat& m = GetDrawContext().Top();
        float x0, y0, x1, y1, x2, y2, x3, y3;
        DC_TransformPoint(m, 0.0f, 0.0f, x0, y0);
        DC_TransformPoint(m, 1.0f, 0.0f, x1, y1);
        DC_TransformPoint(m, 1.0f, 1.0f, x2, y2);
        DC_TransformPoint(m, 0.0f, 1.0f, x3, y3);
        // RebuildTextures already bakes the final fill/outline colours
        // into this frame's RGBA pixels (the whole point of the
        // fill->outline palette-index ramp) -- the plain quad shader still
        // multiplies every texture draw by the drawList's current
        // SetColour state though (kQuadFS: texture * uColour), so leftover
        // colour from an earlier same-frame SetColour call (e.g. a stem/
        // leaf outline drawn with COLOURSCHEME_SetColour("bg_out",...)
        // right before this same entity's head, MarshmallowTree.gm:185)
        // silently re-tints the already-correct texture a second time.
        // Same bug class as the SauceBase cloud tint fix (entity_bindings.
        // cpp's DrawSauceBaseEntityNative) -- force white RGB (keep alpha,
        // for real fade in/out effects) around this draw, then restore.
        Renderer* renderer = GetEngineContext().renderer;
        float pr, pg, pb, pa;
        renderer->GetColour(pr, pg, pb, pa);
        renderer->SetColour(1.0f, 1.0f, 1.0f, pa);
        renderer->DrawTexturedQuad(x0, y0, x1, y1, x2, y2, x3, y3, handle, m.z);
        renderer->SetColour(pr, pg, pb, pa);
    });
}

bool IsAnimationPlayingNow(gmMachine& a_machine, const gmVariable& a_animVar) {
    if (a_animVar.m_type != g_animType) return false;
    gmObject* obj = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!obj) return false;
    AnimationData* a = (AnimationData*)((gmUserObject*)obj)->m_user;
    if (!a) return false;
    return a->playing && !ComputeIsDone(a);
}

void DrawAnimationWithTransform(gmMachine& a_machine, const gmVariable& a_animVar,
                                 float a_a, float a_b, float a_c, float a_d, float a_e, float a_f, float a_z) {
    if (a_animVar.m_type != g_animType) return;
    gmObject* obj = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!obj) return;
    AnimationData* a = (AnimationData*)((gmUserObject*)obj)->m_user;
    if (!a || !a->texValid) return;
    int idx = ComputeFrameIndex(a);
    if (idx < 0 || idx >= (int)a->frameTex.size()) return;
    unsigned int handle = a->frameTex[idx];
    if (!handle) return;
    DrawContext::Mat m; m.a = a_a; m.b = a_b; m.c = a_c; m.d = a_d; m.e = a_e; m.f = a_f; m.z = a_z;
    float x0, y0, x1, y1, x2, y2, x3, y3;
    DC_TransformPoint(m, 0.0f, 0.0f, x0, y0);
    DC_TransformPoint(m, 1.0f, 0.0f, x1, y1);
    DC_TransformPoint(m, 1.0f, 1.0f, x2, y2);
    DC_TransformPoint(m, 0.0f, 1.0f, x3, y3);
    // See DC_gmf_drawAnimation's comment -- same double-tint hazard.
    Renderer* renderer = GetEngineContext().renderer;
    float pr, pg, pb, pa;
    renderer->GetColour(pr, pg, pb, pa);
    renderer->SetColour(1.0f, 1.0f, 1.0f, pa);
    renderer->DrawTexturedQuad(x0, y0, x1, y1, x2, y2, x3, y3, handle, m.z);
    renderer->SetColour(pr, pg, pb, pa);
}
