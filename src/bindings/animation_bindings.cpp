#include "animation_bindings.h"
#include "video_bindings.h"
#include "gmThread.h"
#include "../render/fsa_bci.h"
#include "../render/draw_context.h"
#include "../render/draw_list.h"
#include "../render/texture.h"
#include "../core/engine_context.h"
#include "vector2d_bindings.h"
#include "shape_bindings.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace {

gmType g_animType = -1;
struct AnimationData;
std::unordered_map<std::string, std::shared_ptr<FsaAsset>> s_animationResources;
std::vector<AnimationData*> s_animationInstances;
// Detached GM Animation objects remain alive when another script or retained
// draw command references them, but they no longer belong to manager ticking.
std::vector<AnimationData*> s_detachedAnimationInstances;

struct FsaGlobalShape {
    std::weak_ptr<FsaAsset> asset;
    gmMachine* machine = nullptr;
    gmVariable shape = gmVariable::s_null;
};
std::unordered_map<FsaAsset*, FsaGlobalShape> s_fsaGlobalShapes;

struct AnimationPaletteBank {
    bool haveTint = false;
    float outR = 1, outG = 1, outB = 1;
    float fillR = 1, fillG = 1, fillB = 1;
    std::vector<unsigned int> frameTex;
    bool texValid = false;
};

struct FsaPaletteResources {
    std::weak_ptr<FsaAsset> asset;
    // Retail keeps a contiguous pointer vector at FSA resource +0x118.
    // Selecting a high index allocates all intermediate banks too.
    std::vector<AnimationPaletteBank> banks;

    ~FsaPaletteResources() {
        Renderer* renderer = GetEngineContext().renderer;
        if (!renderer) return;
        for (auto& bank : banks) {
            for (unsigned int handle : bank.frameTex) {
                if (handle) renderer->UnloadTexture(handle);
            }
        }
    }
};
std::unordered_map<FsaAsset*, std::weak_ptr<FsaPaletteResources>> s_fsaPaletteResources;

AnimationPaletteBank* EnsurePaletteBank(FsaPaletteResources& resources,
                                        unsigned int index) {
    const size_t requestedSize = (size_t)index + 1;
    if (requestedSize > resources.banks.max_size()) return nullptr;
    try {
        if (resources.banks.size() < requestedSize)
            resources.banks.resize(requestedSize);
    } catch (const std::bad_alloc&) {
        return nullptr;
    } catch (const std::length_error&) {
        return nullptr;
    }
    return &resources.banks[index];
}

std::shared_ptr<FsaPaletteResources> PaletteResourcesFor(
        const std::shared_ptr<FsaAsset>& asset) {
    if (!asset) return nullptr;
    auto& resources = s_fsaPaletteResources;
    auto found = resources.find(asset.get());
    if (found != resources.end()) {
        if (auto existing = found->second.lock()) return existing;
        resources.erase(found);
    }
    auto created = std::make_shared<FsaPaletteResources>();
    created->asset = asset;
    resources.emplace(asset.get(), created);
    return created;
}

struct AnimationData {
    // The retail animation manager owns cached FSA resource references while
    // each Animation object owns a separate reference to its selected data.
    std::shared_ptr<FsaAsset> asset;
    std::shared_ptr<FsaPaletteResources> paletteResources;

    float recordDurationMs = 1000.0f;
    float playTimeSpeed = 1.0f;
    // Retail stores the live cursor in seconds at animation +0x38. Advance it
    // once per manager tick to preserve its float rounding and mode behavior.
    float playPositionSeconds = 0.0f;
    bool playing = false;
    // Retail manager removal clears Animation+0x4d and stops manager ticks,
    // while leaving the script-visible playback mode and resources intact.
    bool managerAttached = true;
    bool looping = false;
    bool defaultPlayback = false;
    bool hideAfterOneShot = false;
    bool hidden = false; // retail Animation field at +0x4e
    bool reversed = false;
    int selectedFrame = -1;
    int defaultRecord = 0;
    int selectedRecord = 0;
    bool playToFrame = false;
    int stopFrame = 0;
    AnimationData* overlay = nullptr;
    // Keep an attached GM animation object alive while a retained primary
    // draw command follows this native pointer during opcode-9 replay.
    gmObject* overlayObject = nullptr;
    bool ownsOverlayObjectRoot = false;
    gmVariable shape = gmVariable::s_null;
    unsigned int deferredDrawRefs = 0;
    bool unloadPending = false;
    bool unloaded = false;

    // Retail stores this selector per Animation, while the selected bank's
    // texture resource belongs to the shared FSA resource.
    unsigned int selectedPaletteBank = 0;
};

std::unordered_map<std::string, std::unique_ptr<AnimationData>> s_fontSpecialAnimations;

AnimationData* ThisAnim(gmThread* a_thread) {
    return (AnimationData*)a_thread->ThisUserCheckType(g_animType);
}

struct ObjectRoot { unsigned int uses = 0; bool addedRoot = false; };
std::unordered_map<gmObject*, ObjectRoot> s_objectRoots;

void RetainObject(gmMachine& machine, gmObject* object) {
    if (!object) return;
    auto& root = s_objectRoots[object];
    if (root.uses++ == 0 && !machine.IsCPPOwnedGMObject(object)) {
        machine.AddCPPOwnedGMObject(object);
        root.addedRoot = true;
    }
}

void ReleaseObject(gmMachine& machine, gmObject* object) {
    auto it = s_objectRoots.find(object);
    if (it == s_objectRoots.end()) return;
    if (it->second.uses > 0 && --it->second.uses == 0) {
        if (it->second.addedRoot && machine.IsCPPOwnedGMObject(object))
            machine.RemoveCPPOwnedGMObject(object);
        s_objectRoots.erase(it);
    }
}

struct ObjectRootLease {
    gmMachine* machine;
    gmObject* object;
    ObjectRootLease(gmMachine& m, gmObject* o) : machine(&m), object(o) {
        RetainObject(m, o);
    }
    ~ObjectRootLease() {
        if (machine && object) ReleaseObject(*machine, object);
    }
};

void FinalizeAnimationUnload(gmMachine& machine, AnimationData* animation);

struct AnimationDrawLease {
    ObjectRootLease objectRoot;
    AnimationData* animation;

    AnimationDrawLease(gmMachine& machine, gmObject* object, AnimationData* data)
        : objectRoot(machine, object), animation(data) {
        ++animation->deferredDrawRefs;
    }

    ~AnimationDrawLease() {
        // The VM destroys AnimationData and recorded draw-list closures in
        // arbitrary order during ResetAndFreeMemory. The VM is discarding the
        // deferred references as a whole, so do not inspect the raw payload.
        if (objectRoot.machine && objectRoot.machine->IsResetting()) return;
        if (!animation || animation->deferredDrawRefs == 0) return;
        --animation->deferredDrawRefs;
        if (animation->deferredDrawRefs == 0 && animation->unloadPending)
            FinalizeAnimationUnload(*objectRoot.machine, animation);
    }
};

void ReleaseShape(gmMachine& machine, AnimationData* animation) {
    if (!animation->shape.IsReference()) return;
    gmObject* object = machine.GetObject(animation->shape.m_value.m_ref);
    ReleaseObject(machine, object);
    animation->shape = gmVariable::s_null;
}

void ClearAnimationOverlay(gmMachine& machine, AnimationData* animation) {
    if (!animation) return;
    if (animation->ownsOverlayObjectRoot && animation->overlayObject)
        ReleaseObject(machine, animation->overlayObject);
    animation->overlay = nullptr;
    animation->overlayObject = nullptr;
    animation->ownsOverlayObjectRoot = false;
}

void RetainShape(gmMachine& machine, AnimationData* animation, const gmVariable& shape) {
    if (animation->shape.m_type == shape.m_type &&
        animation->shape.m_value.m_ref == shape.m_value.m_ref) return;
    ReleaseShape(machine, animation);
    gmObject* object = machine.GetObject(shape.m_value.m_ref);
    RetainObject(machine, object);
    animation->shape = shape;
}

void ReleaseGlobalShape(FsaGlobalShape& globalShape) {
    if (globalShape.machine && globalShape.shape.IsReference()) {
        gmObject* object = globalShape.machine->GetObject(globalShape.shape.m_value.m_ref);
        ReleaseObject(*globalShape.machine, object);
    }
    globalShape.shape = gmVariable::s_null;
}

void PurgeExpiredGlobalShapes() {
    for (auto it = s_fsaGlobalShapes.begin(); it != s_fsaGlobalShapes.end();) {
        if (it->second.asset.expired()) {
            ReleaseGlobalShape(it->second);
            it = s_fsaGlobalShapes.erase(it);
        } else {
            ++it;
        }
    }
}

void RetainGlobalShape(gmMachine& machine, AnimationData* animation,
                       const gmVariable& shape) {
    if (!animation->asset) return;
    FsaAsset* asset = animation->asset.get();
    auto it = s_fsaGlobalShapes.find(asset);
    if (it != s_fsaGlobalShapes.end() &&
        it->second.shape.m_type == shape.m_type &&
        it->second.shape.m_value.m_ref == shape.m_value.m_ref) return;
    if (it != s_fsaGlobalShapes.end()) {
        ReleaseGlobalShape(it->second);
        s_fsaGlobalShapes.erase(it);
    }
    FsaGlobalShape globalShape;
    globalShape.asset = animation->asset;
    globalShape.machine = &machine;
    globalShape.shape = shape;
    RetainObject(machine, machine.GetObject(shape.m_value.m_ref));
    s_fsaGlobalShapes.emplace(asset, std::move(globalShape));
}

void BuildFsaDefaultShape(gmMachine& machine,
                          const std::shared_ptr<FsaAsset>& asset,
                          bool centeredLayout) {
    if (!asset || asset->frames.empty()) return;
    const BciFrame& image = asset->frames.front();
    if (image.width <= 0 || image.height <= 0) return;

    const float u = (float)image.width / (float)NextPowerOfTwo(image.width);
    const float v = (float)image.height / (float)NextPowerOfTwo(image.height);
    float xy[10];
    float uv[10];
    int count;
    if (!centeredLayout) {
        constexpr float kXY[8] = {0, 0, 1, 0, 1, 1, 0, 1};
        const float kUV[8] = {0, 0, u, 0, u, v, 0, v};
        std::copy(std::begin(kXY), std::end(kXY), xy);
        std::copy(std::begin(kUV), std::end(kUV), uv);
        count = 4;
    } else {
        constexpr float kXY[10] = {0, 1, -0.5f, 1, -0.5f, 0,
                                    0.5f, 0, 0.5f, 1};
        const float kUV[10] = {u * 0.5f, v, 0, v, 0, 0,
                                u * 0.5f, 0, u, v};
        std::copy(std::begin(kXY), std::end(kXY), xy);
        std::copy(std::begin(kUV), std::end(kUV), uv);
        count = 5;
    }

    gmVariable shape = CreateShapeFromVertsAndUVs(
        machine, 6, xy, uv, count, 0.0f);
    if (!shape.IsReference()) return;
    auto it = s_fsaGlobalShapes.find(asset.get());
    if (it != s_fsaGlobalShapes.end()) {
        ReleaseGlobalShape(it->second);
        s_fsaGlobalShapes.erase(it);
    }
    FsaGlobalShape globalShape;
    globalShape.asset = asset;
    globalShape.machine = &machine;
    globalShape.shape = shape;
    RetainObject(machine, machine.GetObject(shape.m_value.m_ref));
    s_fsaGlobalShapes.emplace(asset.get(), std::move(globalShape));
}

gmVariable ShapeForAnimation(const AnimationData* animation) {
    return animation->shape.IsReference() ? animation->shape : gmVariable::s_null;
}

int NumFrames(const AnimationData* a);
int NumImages(const AnimationData* a);
int ApplyReverseFrame(const AnimationData* a, int frame);
float TotalDurationMs(const AnimationData* a);
const std::vector<std::pair<int, float>>& CustomSequence(const AnimationData* a);

const std::vector<std::pair<int, float>>& CustomSequence(const AnimationData* a) {
    static const std::vector<std::pair<int, float>> empty;
    return a->asset ? a->asset->customSequence : empty;
}

float TotalDurationMs(const AnimationData* a) {
    const int count = NumFrames(a);
    if (count <= 0) return 0.0f;
    return a->recordDurationMs;
}

float ElapsedPlaybackMs(const AnimationData* a) {
    return a->playPositionSeconds * 1000.0f;
}

void StartPlaybackAt(AnimationData* a, float positionMs) {
    a->playPositionSeconds = positionMs * 0.001f;
}

float ComputePlayPosition(const AnimationData* a) {
    return ElapsedPlaybackMs(a);
}

int NumFrames(const AnimationData* a) {
    const auto& sequence = CustomSequence(a);
    return !sequence.empty() ? (int)sequence.size() : NumImages(a);
}

int NumImages(const AnimationData* a) {
    return a->asset ? (int)a->asset->frames.size() : 0;
}

int ApplyReverseFrame(const AnimationData* a, int frame) {
    return a->reversed ? NumFrames(a) - frame - 1 : frame;
}

int ComputeFrameIndex(const AnimationData* a) {
    int n = NumFrames(a);
    if (n <= 0) return 0;
    const float durationSeconds = TotalDurationMs(a) * 0.001f;
    const float normalized = (float)((double)a->playPositionSeconds /
                                     (double)durationSeconds);
    const auto& sequence = CustomSequence(a);
    if (!sequence.empty()) {
        float end = 0.0f;
        for (const auto& segment : sequence) {
            end += segment.second;
            if (normalized < end) return ApplyReverseFrame(a, segment.first);
        }
        return ApplyReverseFrame(a, sequence.back().first);
    }
    // The ordinary retail path multiplies frame count by the normalized
    // cursor, converts the result to a signed integer (truncate toward zero),
    // then stores it as uint before clamping only the upper edge. Thus a
    // negative fractional cursor can select frame 0, while a sufficiently
    // negative cursor wraps to a large uint and selects the last frame.
    const float scaledFrame = (float)n * normalized;
    // PPC fctidz saturates positive overflow to INT64_MAX and maps negative
    // overflow/NaN to INT64_MIN. The following stfd/lwz path reads the low
    // word, so these become UINT32_MAX and 0 respectively. Avoid relying on
    // C++'s undefined float-to-integer conversion for out-of-range values.
    const double scaled = (double)scaledFrame;
    uint32_t frameIndex = 0;
    if (std::isnan(scaled) ||
        scaled < (double)std::numeric_limits<int64_t>::min()) {
        frameIndex = 0;  // low word of INT64_MIN
    } else if (scaled >= 0x1p63) {
        frameIndex = UINT32_MAX;  // low word of INT64_MAX
    } else {
        frameIndex = (uint32_t)(int64_t)scaled;
    }
    int idx = frameIndex < (uint32_t)(n - 1) ? (int)frameIndex : n - 1;
    return ApplyReverseFrame(a, idx);
}

void RefreshSelectedFrame(AnimationData* a) {
    a->selectedFrame = NumImages(a) > 0 ? ComputeFrameIndex(a) : -1;
}

bool ComputeIsDone(const AnimationData* a) {
    if (NumImages(a) <= 0) return true;
    const bool oneShotMode = a->playing && !a->looping && !a->defaultPlayback &&
                             !a->playToFrame && !a->hideAfterOneShot;
    return oneShotMode && TotalDurationMs(a) <= ElapsedPlaybackMs(a);
}

void FreeTextures(AnimationData* a) {
    if (a) a->paletteResources.reset();
}

void FinalizeAnimationUnload(gmMachine& machine, AnimationData* animation) {
    ReleaseShape(machine, animation);
    FreeTextures(animation);
    animation->asset.reset();
    PurgeExpiredGlobalShapes();
    animation->recordDurationMs = 0.0f;
    animation->playing = false;
    animation->defaultPlayback = false;
    animation->unloadPending = false;
    animation->unloaded = true;
}

void RebuildTextures(AnimationData* a, AnimationPaletteBank& bank) {
    Renderer* renderer = GetEngineContext().renderer;
    if (renderer) {
        for (unsigned int h : bank.frameTex) {
            if (h) renderer->UnloadTexture(h);
        }
    }
    bank.frameTex.clear();
    bank.texValid = false;
    if (!renderer || !bank.haveTint) return;

    unsigned char palette[16][4] = {};
    auto byte = [](float value) -> unsigned char {
        const float scaled = value * 255.0f;
        if (!std::isfinite(scaled) || scaled >= 0x1p63f || scaled < -0x1p63f)
            return 0;
        const int64_t converted = (int64_t)scaled; // truncates toward zero
        return (unsigned char)((uint64_t)converted & 0xffu);
    };
    const unsigned char fill[3] = {byte(bank.fillR), byte(bank.fillG), byte(bank.fillB)};
    const unsigned char outline[3] = {byte(bank.outR), byte(bank.outG), byte(bank.outB)};
    constexpr unsigned char alpha[5] = {255, 212, 169, 127, 84};
    for (int i = 0; i < 5; ++i) {
        for (int channel = 0; channel < 3; ++channel) {
            palette[i + 1][channel] = fill[channel];
            palette[i + 6][channel] = outline[channel];
        }
        palette[i + 1][3] = alpha[i];
        palette[i + 6][3] = alpha[i];
        const float t = (float)(i + 1) / 6.0f;
        for (int channel = 0; channel < 3; ++channel) {
            float blended = (float)fill[channel] +
                ((float)outline[channel] - (float)fill[channel]) * t;
            palette[i + 11][channel] = (unsigned char)std::clamp(
                blended + 0.5f, 0.0f, 255.0f);
        }
        palette[i + 11][3] = 255;
    }

    if (!a->asset) return;
    bank.frameTex.assign(a->asset->frames.size(), 0);
    std::vector<unsigned char> rgba;
    for (size_t f = 0; f < a->asset->frames.size(); ++f) {
        const BciFrame& frame = a->asset->frames[f];
        if (frame.width <= 0 || frame.height <= 0) continue;
        size_t count = (size_t)frame.width * frame.height;
        if (!frame.rgba.empty()) {
            if (frame.rgba.size() != count * 4) continue;
            bank.frameTex[f] = renderer->CreateTextureFromRGBA(
                frame.rgba.data(), frame.width, frame.height);
            continue;
        }
        rgba.assign(count * 4, 0);
        for (size_t i = 0; i < frame.indices.size() && i < count; ++i) {
            const unsigned char* colour = palette[frame.indices[i] & 15];
            for (int channel = 0; channel < 4; ++channel)
                rgba[i * 4 + channel] = colour[channel];
        }
        bank.frameTex[f] = renderer->CreateTextureFromRGBA(
            rgba.data(), frame.width, frame.height, /*nearestFilter=*/true);
    }
    bank.texValid = true;
}

int GM_CDECL gmf_anim_finishLoad(gmThread* a_thread) { return GM_OK; }

int GM_CDECL gmf_anim_pause(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a->playing = false;
    return GM_OK;
}

int GM_CDECL gmf_anim_play(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    if (a_thread->GetNumParams() > 1) return GM_EXCEPTION;
    const bool reset = a_thread->GetNumParams() == 1 &&
        a_thread->Param(0).m_value.m_int != 0;
    const float position = reset ? 0.0f : ComputePlayPosition(a);
    a->looping = true;
    a->defaultPlayback = false;
    a->playToFrame = false;
    a->hideAfterOneShot = false;
    a->hidden = false;
    a->playing = true;
    StartPlaybackAt(a, position);
    if (reset) RefreshSelectedFrame(a);
    return GM_OK;
}

int GM_CDECL gmf_anim_playOnce(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    if (a_thread->GetNumParams() > 1) return GM_EXCEPTION;
    const bool reset = a_thread->GetNumParams() == 1 &&
        a_thread->Param(0).m_value.m_int != 0;
    const float position = NumImages(a) < 2 ? TotalDurationMs(a)
        : reset ? 0.0f : ComputePlayPosition(a);
    a->looping = false;
    a->defaultPlayback = false;
    a->playToFrame = false;
    a->hideAfterOneShot = false;
    a->hidden = false;
    a->playing = true;
    StartPlaybackAt(a, position);
    if (NumImages(a) >= 2 && reset) RefreshSelectedFrame(a);
    return GM_OK;
}

int GM_CDECL gmf_anim_playOnceHide(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a->looping = false;
    a->defaultPlayback = false;
    a->playToFrame = false;
    a->hideAfterOneShot = true;
    a->hidden = false;
    a->playing = true;
    StartPlaybackAt(a, 0.0f);
    RefreshSelectedFrame(a);
    return GM_OK;
}

int GM_CDECL gmf_anim_setCurrentFrame(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(frame, 0, 0);
    const int count = NumFrames(a);
    if (count <= 0) return GM_OK;
    if (frame >= count)
        frame = count - 1;
    // A negative index is not handled safely by retail: its signed compare
    // lets it through, then its unsigned loop count walks outside the vector.
    // Fail explicitly instead of reproducing an uncontrolled memory walk.
    if (frame < 0) return GM_EXCEPTION;
    const auto& sequence = CustomSequence(a);
    float normalizedPosition = 0.0f;
    for (int i = 0; i < frame; ++i)
        normalizedPosition += sequence.empty()
            ? 1.0f / static_cast<float>(count)
            : sequence[i].second;
    const float frameWeight = sequence.empty()
        ? 1.0f / static_cast<float>(count)
        : sequence[frame].second;
    normalizedPosition += static_cast<float>(static_cast<double>(frameWeight) * 0.01);
    a->playPositionSeconds = normalizedPosition;
    return GM_OK;
}

int GM_CDECL gmf_anim_playAnimToFrame(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(record, 0, 0);
    GM_INT_PARAM(stop, 1, 0);
    const int recordCount = NumImages(a) > 0 ? 1 : 0;
    if (record < 0 || record >= recordCount) return GM_OK;
    a->selectedRecord = record;
    a->stopFrame = stop;
    a->playToFrame = true;
    a->defaultPlayback = false;
    a->looping = false;
    a->hideAfterOneShot = false;
    a->hidden = false;
    a->playing = true;
    StartPlaybackAt(a, 0.0f);
    RefreshSelectedFrame(a);
    return GM_OK;
}

int GM_CDECL gmf_anim_attachOverlayAnim(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    AnimationData* overlay = (AnimationData*)a_thread->ParamUserCheckType(0, g_animType);
    if (!overlay) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    const gmVariable& overlayVar = a_thread->Param(0);
    gmObject* overlayObject = overlayVar.m_type >= GM_USER
        ? machine.GetObject(overlayVar.m_value.m_ref) : nullptr;
    gmUserObject* ownerObject = a_thread->ThisUserObject();
    if (!overlayObject || !ownerObject) return GM_EXCEPTION;
    if (a->overlay == overlay && a->overlayObject == overlayObject) return GM_OK;
    ClearAnimationOverlay(machine, a);
    a->overlay = overlay;
    a->overlayObject = overlayObject;
    if (overlayObject != ownerObject) {
        RetainObject(machine, overlayObject);
        a->ownsOverlayObjectRoot = true;
    }
    return GM_OK;
}

int GM_CDECL gmf_anim_setAnimSequence(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(sequence, 0, 0);
    if (sequence != 0 || a_thread->Param(1).m_type != GM_TABLE) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    gmTableObject* table = (gmTableObject*)machine->GetObject(a_thread->Param(1).m_value.m_ref);
    if (!table || table->Count() == 0 || (table->Count() & 1)) return GM_EXCEPTION;
    std::vector<std::pair<int, float>> entries;
    float total = 0.0f;
    for (int i = 0; i < table->Count(); i += 2) {
        gmVariable frame = table->Get(i);
        gmVariable weight = table->Get(i + 1);
        if ((frame.m_type != GM_INT && frame.m_type != GM_FLOAT) ||
            (weight.m_type != GM_INT && weight.m_type != GM_FLOAT)) return GM_EXCEPTION;
        int frameIndex = frame.m_type == GM_INT ? frame.m_value.m_int : (int)frame.m_value.m_float;
        float duration = weight.m_type == GM_INT ? (float)weight.m_value.m_int : weight.m_value.m_float;
        // Retail rejects only image indices above getNumImages(); negative
        // indices and non-finite weights pass through its wrapper unchanged.
        if (frameIndex > NumImages(a))
            return GM_EXCEPTION;
        entries.emplace_back(frameIndex, duration);
        total = static_cast<float>((double)total + (double)duration);
    }
    a->recordDurationMs = total;
    // Retail rounds the reciprocal to float once, then multiplies each
    // float weight by that rounded reciprocal in double before rounding the
    // product back to float. Dividing each weight directly can differ by ULP.
    const float inverseTotal = static_cast<float>(1.0 / (double)total);
    for (auto& entry : entries) {
        entry.second = static_cast<float>(
            (double)entry.second * (double)inverseTotal);
    }
    if (a->asset) a->asset->customSequence = std::move(entries);
    return GM_OK;
}

int GM_CDECL gmf_anim_setIsReversed(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    a->reversed = a_thread->Param(0).m_type == GM_INT && a_thread->Param(0).m_value.m_int != 0;
    return GM_OK;
}

int GM_CDECL gmf_anim_setDefaultAnim(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(record, 0, 0);
    const int recordCount = NumImages(a) > 0 ? 1 : 0;
    if (record >= 0 && record < recordCount) a->defaultRecord = record;
    return GM_OK;
}

int GM_CDECL gmf_anim_playAnimDefault(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(record, 0, 0);
    const int recordCount = NumImages(a) > 0 ? 1 : 0;
    if (record < 0 || record >= recordCount) return GM_OK;
    a->looping = true;
    a->defaultPlayback = true;
    a->playToFrame = false;
    a->hideAfterOneShot = false;
    a->hidden = false;
    a->playing = true;
    a->selectedRecord = record;
    StartPlaybackAt(a, 0.0f);
    RefreshSelectedFrame(a);
    return GM_OK;
}

int GM_CDECL gmf_anim_resumeAnimDefault(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    if (a_thread->GetNumParams() > 1) return GM_OK; // retail wrapper ignores extras
    const bool reverse = a_thread->GetNumParams() == 1 &&
        a_thread->Param(0).m_type == GM_INT && a_thread->Param(0).m_value.m_int != 0;
    if (NumImages(a) <= 0) return GM_OK;
    float position = ComputePlayPosition(a);
    if (reverse) {
        position = TotalDurationMs(a) - position;
        a->reversed = true;
    }
    a->looping = true;
    a->defaultPlayback = true;
    a->playToFrame = false;
    a->hideAfterOneShot = false;
    a->hidden = false;
    a->playing = true;
    StartPlaybackAt(a, position);
    return GM_OK;
}

int GM_CDECL gmf_anim_setPaletteBank(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(bank, 0, 0);
    // Retail stores the argument as an unsigned index at animation +0x20.
    a->selectedPaletteBank = (unsigned int)bank;
    if (a->asset) {
        a->paletteResources = PaletteResourcesFor(a->asset);
        if (a->paletteResources)
            EnsurePaletteBank(*a->paletteResources, a->selectedPaletteBank);
    }
    return GM_OK;
}
int GM_CDECL gmf_anim_isHidden(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(a->hidden ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_anim_setIsHidden(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(hidden, 0, 0);
    a->hidden = hidden != 0;
    return GM_OK;
}

int GM_CDECL gmf_anim_setPlayPosition(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(posMs, 0, 0.0f);
    StartPlaybackAt(a, posMs);
    RefreshSelectedFrame(a);
    return GM_OK;
}

int GM_CDECL gmf_anim_getPlayPosition(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushFloat(ComputePlayPosition(a));
    return GM_OK;
}

int GM_CDECL gmf_anim_setPlayTime(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    if (a_thread->GetNumParams() < 1 || a_thread->GetNumParams() > 2)
        return GM_EXCEPTION;
    GM_FLOAT_OR_INT_PARAM(ms, a_thread->GetNumParams() == 1 ? 0 : 1, 500.0f);
    const int recordCount = NumImages(a) > 0 ? 1 : 0; // current loader exposes one FSA record
    if (a_thread->GetNumParams() == 1) {
        if (recordCount > 0) {
            a->recordDurationMs = ms;
        }
    } else {
        GM_INT_PARAM(record, 0, -1);
        if (record >= 0 && record < recordCount) {
            a->recordDurationMs = ms;
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_anim_getPlayTime(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushFloat(NumImages(a) > 0 ? TotalDurationMs(a) : 1000.0f);
    return GM_OK;
}

int GM_CDECL gmf_anim_setPlayTimeSpeed(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(speed, 0, 1.0f);
    a->playTimeSpeed = speed;
    return GM_OK;
}

int GM_CDECL gmf_anim_setShape(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmVariable shape = a_thread->Param(0);
    gmMachine& machine = *a_thread->GetMachine();
    if (!AsShapeObject(machine, shape)) return GM_EXCEPTION;
    RetainShape(machine, a, shape);
    return GM_OK;
}

int GM_CDECL gmf_anim_setGlobalShape(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmVariable shape = a_thread->Param(0);
    gmMachine& machine = *a_thread->GetMachine();
    if (!AsShapeObject(machine, shape)) return GM_EXCEPTION;
    RetainShape(machine, a, shape);
    RetainGlobalShape(machine, a, shape);
    return GM_OK;
}

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
int GM_CDECL gmf_anim_getNumImages(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    // A standard FSA is represented locally as one flattened resource record:
    // each decoded BCI is one image in that record. Multi-image records and
    // record selection remain a known loader-model gap (audit document).
    a_thread->PushInt(NumImages(a));
    return GM_OK;
}
int GM_CDECL gmf_anim_getSourcePixelWidth(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushFloat(NumImages(a) > 0 ? (float)a->asset->frames[0].width : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_anim_getSourcePixelHeight(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushFloat(NumImages(a) > 0 ? (float)a->asset->frames[0].height : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_anim_getTextureExt(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    float u = 1.0f, v = 1.0f;
    int frame = a->selectedFrame;
    if (frame >= 0 && frame < NumImages(a)) {
        const BciFrame& image = a->asset->frames[frame];
        if (image.width > 0 && image.height > 0) {
            u = (float)image.width / (float)NextPowerOfTwo(image.width);
            v = (float)image.height / (float)NextPowerOfTwo(image.height);
        }
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), u, v));
    return GM_OK;
}

int GM_CDECL gmf_anim_isDone(gmThread* a_thread) {
    AnimationData* a = ThisAnim(a_thread);
    if (!a) return GM_EXCEPTION;
    a_thread->PushInt(ComputeIsDone(a) ? 1 : 0);
    return GM_OK;
}
// This predicate was not included in the recovered Animation.c method list.
// Its registration-table wrapper and helper are now recovered: retail tests
// whether animation mode +0x50 is zero, exactly represented by !playing.
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

int GM_CDECL gmf_animationModuleUnloadAll(gmThread* a_thread);

gmFunctionEntry s_animLib[] = {
    {"attachOverlayAnim", gmf_anim_attachOverlayAnim, nullptr},
    {"setAnimSequence", gmf_anim_setAnimSequence, nullptr},
    {"finishLoad", gmf_anim_finishLoad, nullptr},
    {"pause", gmf_anim_pause, nullptr},
    {"play", gmf_anim_play, nullptr},
    {"playOnce", gmf_anim_playOnce, nullptr},
    {"playOnceHide", gmf_anim_playOnceHide, nullptr},
    {"playAnimToFrame", gmf_anim_playAnimToFrame, nullptr},
    {"setCurrentFrame", gmf_anim_setCurrentFrame, nullptr},
    {"setIsReversed", gmf_anim_setIsReversed, nullptr},
    {"setIsHidden", gmf_anim_setIsHidden, nullptr},
    {"setDefaultAnim", gmf_anim_setDefaultAnim, nullptr},
    {"playAnimDefault", gmf_anim_playAnimDefault, nullptr},
    {"resumeAnimDefault", gmf_anim_resumeAnimDefault, nullptr},
    {"setPaletteBank", gmf_anim_setPaletteBank, nullptr},
    {"setPlayTime", gmf_anim_setPlayTime, nullptr},
    {"setPlayTimeSpeed", gmf_anim_setPlayTimeSpeed, nullptr},
    {"getPlayTime", gmf_anim_getPlayTime, nullptr},
    {"setPlayPosition", gmf_anim_setPlayPosition, nullptr},
    {"getPlayPosition", gmf_anim_getPlayPosition, nullptr},
    {"setShape", gmf_anim_setShape, nullptr},
    {"setGlobalShape", gmf_anim_setGlobalShape, nullptr},
    {"getCurrentFrame", gmf_anim_getCurrentFrame, nullptr},
    {"getNumFrames", gmf_anim_getNumFrames, nullptr},
    {"getNumImages", gmf_anim_getNumImages, nullptr},
    {"getSourcePixelWidth", gmf_anim_getSourcePixelWidth, nullptr},
    {"getSourcePixelHeight", gmf_anim_getSourcePixelHeight, nullptr},
    {"getTextureExt", gmf_anim_getTextureExt, nullptr},
    {"isDone", gmf_anim_isDone, nullptr},
    {"isPaused", gmf_anim_isPaused, nullptr},
    {"isPlaying", gmf_anim_isPlaying, nullptr},
    {"isReversed", gmf_anim_isReversed, nullptr},
    {"isHidden", gmf_anim_isHidden, nullptr},
    {"unloadAll", gmf_animationModuleUnloadAll, nullptr},
};

int GM_CDECL gmf_animationModuleLoad(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 1 || a_thread->GetNumParams() > 2)
        return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    const bool centeredLayout = a_thread->GetNumParams() == 2 &&
        a_thread->Param(1).m_value.m_int != 0;
    std::string resolved;
    if (auto* fileHelper = GetEngineContext().fileHelper) {
        auto result = ResolvePlatformResourcePath(path, 1);
        if (result) resolved = *result;
    }
    if (resolved.empty())
        ReportMissingAnimationResource("AnimationModule.loadAnimation", path);
    // Standalone module loads are not attached to the entity animation clock.
    PushNewAnimation(a_thread, resolved, false, centeredLayout ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_animationModuleUnload(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    // The retail wrapper leaves both the live object and shared cache intact.
    return GM_OK;
}

int GM_CDECL gmf_animationModuleUnloadAll(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(0);
    s_animationResources.clear();
    PurgeExpiredGlobalShapes();
    return GM_OK;
}

gmFunctionEntry s_animationModuleLib[] = {
    {"loadAnimation", gmf_animationModuleLoad, nullptr},
    {"unloadAnimation", gmf_animationModuleUnload, nullptr},
    {"unloadAll", gmf_animationModuleUnloadAll, nullptr},
};

void GM_CDECL AnimGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    AnimationData* a = (AnimationData*)a_object->m_user;
    s_animationInstances.erase(
        std::remove(s_animationInstances.begin(), s_animationInstances.end(), a),
        s_animationInstances.end());
    s_detachedAnimationInstances.erase(
        std::remove(s_detachedAnimationInstances.begin(), s_detachedAnimationInstances.end(), a),
        s_detachedAnimationInstances.end());
    if (a_machine) {
        ClearAnimationOverlay(*a_machine, a);
        ReleaseShape(*a_machine, a);
    }
    FreeTextures(a);
    delete a;
    if (a_machine) PurgeExpiredGlobalShapes();
}

} // namespace

void ReportMissingAnimationResource(const char* a_api, const char* a_path) {
    std::fprintf(stderr,
        "WARNING: %s('%s') would fault retail (observed VM access violation reading 0x48); "
        "OpenEgg returns an empty Animation.\n",
        a_api ? a_api : "animation loader", a_path ? a_path : "");
}

void AdvanceAnimationClock(float a_msElapsed) {
    for (AnimationData* animation : s_animationInstances) {
        if (!animation || animation->unloaded || !animation->managerAttached) continue;
        if (animation->playing) {
            // Retail uses fmadds(deltaMs, 0.001f, cursorSeconds).
            animation->playPositionSeconds = std::fma(
                a_msElapsed, 0.001f, animation->playPositionSeconds);
            const float durationSeconds = TotalDurationMs(animation) * 0.001f;

            // Mode 1 adds delta and clamps at the selected record duration.
            if (!animation->looping && !animation->defaultPlayback &&
                !animation->playToFrame && !animation->hideAfterOneShot) {
                animation->playPositionSeconds = std::min(
                    animation->playPositionSeconds, durationSeconds);
            } else if (animation->looping && !animation->defaultPlayback) {
                animation->playPositionSeconds = std::fmod(
                    animation->playPositionSeconds, durationSeconds);
            // Mode 3 discards overshoot, selects the default record, clears
            // reverse, and changes to mode 2 at the duration boundary.
            } else if (animation->defaultPlayback &&
                       !(animation->playPositionSeconds < durationSeconds)) {
                animation->playPositionSeconds = 0.0f;
                animation->reversed = false;
                animation->selectedRecord = animation->defaultRecord;
                animation->defaultPlayback = false;
                animation->looping = true;
            // Mode 4 compares the ordinary resolved record frame to its stop.
            } else if (animation->playToFrame &&
                       ComputeFrameIndex(animation) == animation->stopFrame) {
                animation->playing = false;
                animation->playToFrame = false;
            // Mode 5 stops and hides only when the cursor strictly exceeds
            // the duration. This transition occurs for off-screen clips too.
            } else if (animation->hideAfterOneShot &&
                       !(animation->playPositionSeconds <= durationSeconds)) {
                animation->playing = false;
                animation->hidden = true;
                animation->hideAfterOneShot = false;
            }
        }
        RefreshSelectedFrame(animation);
    }
}

void DetachAnimationFromManager(gmMachine& a_machine, const gmVariable& a_animVar) {
    if (a_animVar.m_type < GM_USER) return;
    gmObject* object = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!object || object->GetType() != g_animType) return;
    auto* animation = (AnimationData*)((gmUserObject*)object)->m_user;
    if (!animation) return;

    ClearAnimationOverlay(a_machine, animation);
    animation->managerAttached = false;
    if (std::find(s_detachedAnimationInstances.begin(), s_detachedAnimationInstances.end(),
                  animation) == s_detachedAnimationInstances.end())
        s_detachedAnimationInstances.push_back(animation);
    s_animationInstances.erase(
        std::remove(s_animationInstances.begin(), s_animationInstances.end(), animation),
        s_animationInstances.end());
}

void SetAnimationManagerAttached(gmMachine& a_machine, const gmVariable& a_animVar,
                                 bool a_attached) {
    if (a_animVar.m_type < GM_USER) return;
    gmObject* object = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!object || object->GetType() != g_animType) return;
    auto* animation = (AnimationData*)((gmUserObject*)object)->m_user;
    if (!animation || animation->unloaded || animation->unloadPending) return;

    if (a_attached) {
        if (animation->managerAttached) return;
        if (NumImages(animation) <= 1) return;
        animation->managerAttached = true;
        s_detachedAnimationInstances.erase(
            std::remove(s_detachedAnimationInstances.begin(),
                        s_detachedAnimationInstances.end(), animation),
            s_detachedAnimationInstances.end());
        if (std::find(s_animationInstances.begin(), s_animationInstances.end(), animation) ==
            s_animationInstances.end())
            s_animationInstances.push_back(animation);
    } else {
        if (!animation->managerAttached) return;
        animation->managerAttached = false;
        if (std::find(s_detachedAnimationInstances.begin(),
                      s_detachedAnimationInstances.end(), animation) ==
            s_detachedAnimationInstances.end())
            s_detachedAnimationInstances.push_back(animation);
        s_animationInstances.erase(
            std::remove(s_animationInstances.begin(), s_animationInstances.end(), animation),
            s_animationInstances.end());
    }
}

std::size_t GetLiveAnimationResourceCount() {
    std::unordered_set<AnimationData*> fontInstances;
    std::unordered_set<FsaAsset*> fontAssets;
    std::unordered_set<FsaAsset*> liveAssets;
    for (const auto& [path, animation] : s_fontSpecialAnimations) {
        (void)path;
        if (!animation) continue;
        fontInstances.insert(animation.get());
        if (animation->asset) fontAssets.insert(animation->asset.get());
    }
    for (AnimationData* animation : s_animationInstances) {
        if (!animation || fontInstances.count(animation) || !animation->asset) continue;
        liveAssets.insert(animation->asset.get());
    }
    for (AnimationData* animation : s_detachedAnimationInstances) {
        if (!animation || fontInstances.count(animation) || !animation->asset) continue;
        liveAssets.insert(animation->asset.get());
    }
    // The shared manager cache itself owns resource references. Font-special
    // assets are engine-lifetime resources in this port; count a matching
    // cached asset only when a non-font Animation instance also uses it.
    for (const auto& [path, asset] : s_animationResources) {
        (void)path;
        if (!asset) continue;
        if (!fontAssets.count(asset.get()) || liveAssets.count(asset.get()))
            liveAssets.insert(asset.get());
    }
    return liveAssets.size();
}

void RegisterAnimationBindings(gmMachine& a_machine) {
    g_animType = a_machine.CreateUserType("Animation");
    a_machine.RegisterTypeLibrary(g_animType, s_animLib, sizeof(s_animLib) / sizeof(s_animLib[0]));
    a_machine.RegisterUserCallbacks(g_animType, nullptr, AnimGCDestruct, nullptr);
    a_machine.RegisterLibrary(s_animationModuleLib,
        sizeof(s_animationModuleLib) / sizeof(s_animationModuleLib[0]),
        "AnimationModule", false);
}

void PushNewAnimation(gmThread* a_thread, const std::string& a_resolvedPath,
                      bool a_attachToManager, int a_fsaShapeLayout) {
    AnimationData* data = new AnimationData();
    if (!a_resolvedPath.empty()) {
        auto it = s_animationResources.find(a_resolvedPath);
        if (it != s_animationResources.end()) {
            data->asset = it->second;
        } else {
            auto asset = std::make_shared<FsaAsset>();
            if (asset->LoadFromFile(a_resolvedPath)) {
                data->asset = asset;
                s_animationResources.emplace(a_resolvedPath, std::move(asset));
            } else {
                std::fprintf(stderr, "Entity.loadAnimation: failed to parse '%s'\n",
                             a_resolvedPath.c_str());
            }
        }
    }
    if (data->asset && a_fsaShapeLayout >= 0)
        BuildFsaDefaultShape(*a_thread->GetMachine(), data->asset,
                             a_fsaShapeLayout != 0);
    if (data->asset) {
        auto sharedShape = s_fsaGlobalShapes.find(data->asset.get());
        if (sharedShape != s_fsaGlobalShapes.end() &&
            sharedShape->second.shape.IsReference()) {
            RetainShape(*a_thread->GetMachine(), data, sharedShape->second.shape);
        }
    }
    data->paletteResources = PaletteResourcesFor(data->asset);
    data->managerAttached = a_attachToManager && NumImages(data) > 1;
    if (NumImages(data) == 1) data->selectedFrame = 0;
    a_thread->PushNewUser(data, g_animType);
    if (data->managerAttached) {
        s_animationInstances.push_back(data);
    } else {
        // Keep the GC-owned instance visible to resource-lifetime accounting
        // even when retail does not tick it (disabled owner or <=1 image).
        s_detachedAnimationInstances.push_back(data);
    }
}

bool GetFontAnimationFrame(const std::string& a_resolvedPath,
                           unsigned int& a_texture, int& a_width, int& a_height) {
    a_texture = 0;
    a_width = a_height = 0;
    if (a_resolvedPath.empty()) return false;

    auto it = s_fontSpecialAnimations.find(a_resolvedPath);
    AnimationData* animation = nullptr;
    if (it == s_fontSpecialAnimations.end()) {
        std::shared_ptr<FsaAsset> asset;
        auto resource = s_animationResources.find(a_resolvedPath);
        if (resource != s_animationResources.end()) {
            asset = resource->second;
        } else {
            asset = std::make_shared<FsaAsset>();
            if (!asset->LoadFromFile(a_resolvedPath)) {
                std::fprintf(stderr,
                    "BitmapFont: failed to parse FSA special character '%s'\n",
                    a_resolvedPath.c_str());
                return false;
            }
            s_animationResources.emplace(a_resolvedPath, asset);
        }

        if (!asset || asset->frames.empty()) return false;
        // FontSpecialChars can use FSA files, but the font-special loader's
        // palette selection contract is not the entity animation tint API.
        // Only direct-colour frames are safe to upload here; indexed frames
        // stay visible as unsupported rather than receiving guessed colours.
        for (const BciFrame& frame : asset->frames) {
            const size_t pixels = (size_t)frame.width * (size_t)frame.height;
            if (frame.width <= 0 || frame.height <= 0 ||
                frame.rgba.size() != pixels * 4 || !frame.indices.empty()) {
                std::fprintf(stderr,
                    "BitmapFont: indexed/non-RGBA frame in FSA special character '%s'\n",
                    a_resolvedPath.c_str());
                return false;
            }
        }

        auto created = std::make_unique<AnimationData>();
        created->asset = std::move(asset);
        created->paletteResources = PaletteResourcesFor(created->asset);
        created->playing = true;
        created->looping = true;
        created->selectedFrame = 0; // loader's selected first image
        AnimationPaletteBank* bankPtr = EnsurePaletteBank(*created->paletteResources, 0);
        if (!bankPtr) return false;
        AnimationPaletteBank& bank = *bankPtr;
        bank.haveTint = true; // direct RGBA ignores generated palette values
        RebuildTextures(created.get(), bank);
        if (!bank.texValid || bank.frameTex.empty() || !bank.frameTex[0]) {
            std::fprintf(stderr,
                "BitmapFont: could not upload FSA special character '%s'\n",
                a_resolvedPath.c_str());
            return false;
        }
        animation = created.get();
        s_animationInstances.push_back(animation);
        s_fontSpecialAnimations.emplace(a_resolvedPath, std::move(created));
    } else {
        animation = it->second.get();
    }

    if (!animation || animation->selectedFrame < 0 || !animation->asset) return false;
    if (!animation->paletteResources) return false;
    const int frame = animation->selectedFrame;
    if (animation->paletteResources->banks.empty()) return false;
    AnimationPaletteBank& bank = animation->paletteResources->banks[0];
    if (frame >= (int)bank.frameTex.size() ||
        frame >= (int)animation->asset->frames.size())
        return false;
    a_texture = bank.frameTex[frame];
    a_width = animation->asset->frames[frame].width;
    a_height = animation->asset->frames[frame].height;
    return a_texture != 0;
}

void UnloadAnimation(gmMachine& a_machine, const gmVariable& a_animVar) {
    if (a_animVar.m_type != g_animType) return;
    gmObject* obj = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!obj) return;
    auto* anim = (AnimationData*)((gmUserObject*)obj)->m_user;
    if (!anim) return;
    if (anim->unloaded || anim->unloadPending) return;
    if (anim->deferredDrawRefs != 0) {
        anim->unloadPending = true;
        return;
    }
    FinalizeAnimationUnload(a_machine, anim);
}

bool SetAnimationPaletteBank(gmMachine& a_machine, const gmVariable& a_animVar, int a_bank) {
    if (a_animVar.m_type != g_animType) return false;
    gmObject* object = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!object) return false;
    AnimationData* animation = (AnimationData*)((gmUserObject*)object)->m_user;
    if (!animation) return false;
    // Mirrors gmf_anim_setPaletteBank: store the argument as an unsigned bank
    // index and ensure the shared FSA resource owns that bank.
    animation->selectedPaletteBank = (unsigned int)a_bank;
    if (animation->asset) {
        animation->paletteResources = PaletteResourcesFor(animation->asset);
        if (animation->paletteResources)
            EnsurePaletteBank(*animation->paletteResources, animation->selectedPaletteBank);
    }
    return true;
}

bool SetAnimationPaletteColour(gmMachine& a_machine, const gmVariable& a_animVar,
                                float a_outR, float a_outG, float a_outB,
                                float a_fillR, float a_fillG, float a_fillB) {
    if (a_animVar.m_type != g_animType) return false;
    gmObject* object = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!object) return false;
    AnimationData* a = (AnimationData*)((gmUserObject*)object)->m_user;
    if (!a) return false;
    if (a->unloaded || a->unloadPending) return false;
    if (!a->asset) return false;
    if (!a->paletteResources) a->paletteResources = PaletteResourcesFor(a->asset);
    if (!a->paletteResources) return false;
    AnimationPaletteBank* selectedBank = EnsurePaletteBank(
        *a->paletteResources, a->selectedPaletteBank);
    if (!selectedBank) return false;
    AnimationPaletteBank& bank = *selectedBank;
    if (bank.haveTint && bank.outR == a_outR && bank.outG == a_outG && bank.outB == a_outB &&
        bank.fillR == a_fillR && bank.fillG == a_fillG && bank.fillB == a_fillB && bank.texValid) {
        return true; // unchanged -- skip regenerating every frame's texture
    }
    bank.outR = a_outR; bank.outG = a_outG; bank.outB = a_outB;
    bank.fillR = a_fillR; bank.fillG = a_fillG; bank.fillB = a_fillB;
    bank.haveTint = true;
    RebuildTextures(a, bank);
    return true;
}

bool DrawAnimationOverlayWithTransform(gmMachine& a_machine,
                                       const gmVariable& a_animVar,
                                       float a_a, float a_b, float a_c,
                                       float a_d, float a_e, float a_f,
                                       float a_z);

int GM_CDECL DC_gmf_drawAnimation(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable animVar = a_thread->Param(0);
    if (animVar.m_type != g_animType) return GM_OK; // lenient -- matches drawShape's null-arg tolerance
    gmMachine* machine = a_thread->GetMachine();
    gmObject* animationObject = machine->GetObject(animVar.m_value.m_ref);
    if (!animationObject) return GM_OK;
    AnimationData* a = (AnimationData*)((gmUserObject*)animationObject)->m_user;
    if (!a) return GM_OK;
    if (a->unloaded || a->unloadPending) return GM_OK;

    const bool pairedAnimation = a->overlay != nullptr;
    const unsigned int paletteBankIndex = a->selectedPaletteBank;
    auto keepAlive = machine
        ? std::make_shared<AnimationDrawLease>(*machine, animationObject, a)
        : std::shared_ptr<AnimationDrawLease>{};
    return DC_RecordOrRun(a_thread, [a, animVar, pairedAnimation,
                                     paletteBankIndex, machine, keepAlive] {
        // Retail opcode 8 checks Animation+0x4e, but opcode 9 (the attached
        // animation path selected by Animation+0x28) does not check either
        // animation's hidden byte. Route paired commands through the same
        // compositor used by the native entity path, preserving that opcode
        // distinction in generic DrawList.drawAnimation calls too.
        if (pairedAnimation) {
            if (!machine) return;
            const DrawContext::Mat& pairedMat = GetDrawContext().Top();
            DrawAnimationOverlayWithTransform(*machine, animVar,
                pairedMat.a, pairedMat.b, pairedMat.c, pairedMat.d,
                pairedMat.e, pairedMat.f, pairedMat.z);
            return;
        }
        if (a->hidden) return;
        if (!a->paletteResources) return;
        if ((size_t)paletteBankIndex >= a->paletteResources->banks.size()) return;
        const AnimationPaletteBank& bank = a->paletteResources->banks[paletteBankIndex];
        if (!bank.texValid) return;
        int idx = a->selectedFrame;
        if (idx < 0 || idx >= (int)bank.frameTex.size()) return;
        unsigned int handle = bank.frameTex[idx];
        if (!handle) return;
        const DrawContext::Mat& m = GetDrawContext().Top();
        float x0, y0, x1, y1, x2, y2, x3, y3;
        DC_TransformPoint(m, 0.0f, 0.0f, x0, y0);
        DC_TransformPoint(m, 1.0f, 0.0f, x1, y1);
        DC_TransformPoint(m, 1.0f, 1.0f, x2, y2);
        DC_TransformPoint(m, 0.0f, 1.0f, x3, y3);
        Renderer* renderer = GetEngineContext().renderer;
        float pr, pg, pb, pa;
        renderer->GetColour(pr, pg, pb, pa);
        renderer->SetColour(1.0f, 1.0f, 1.0f, pa);
        if (!machine || !DrawAnimationShapeWithTransform(*machine, ShapeForAnimation(a), handle, pa,
                m.a, m.b, m.c, m.d, m.e, m.f, m.z))
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
    if (!a) return;
    if (a->unloaded || a->unloadPending) return;
    if (a->hidden) return;
    if (!a->paletteResources) return;
    if ((size_t)a->selectedPaletteBank >= a->paletteResources->banks.size()) return;
    const AnimationPaletteBank& bank = a->paletteResources->banks[a->selectedPaletteBank];
    if (!bank.texValid) return;
    int idx = a->selectedFrame;
    if (idx < 0 || idx >= (int)bank.frameTex.size()) return;
    unsigned int handle = bank.frameTex[idx];
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
    if (!DrawAnimationShapeWithTransform(a_machine, ShapeForAnimation(a), handle, pa,
            m.a, m.b, m.c, m.d, m.e, m.f, m.z))
        renderer->DrawTexturedQuad(x0, y0, x1, y1, x2, y2, x3, y3, handle, m.z);
    renderer->SetColour(pr, pg, pb, pa);
}

bool DrawAnimationOverlayWithTransform(gmMachine& a_machine, const gmVariable& a_animVar,
                                       float a_a, float a_b, float a_c, float a_d,
                                       float a_e, float a_f, float a_z) {
    if (a_animVar.m_type != g_animType) return false;
    gmObject* object = a_machine.GetObject(a_animVar.m_value.m_ref);
    if (!object) return false;
    AnimationData* back = (AnimationData*)((gmUserObject*)object)->m_user;
    if (!back || !back->overlay || back->unloaded || back->unloadPending) return false;
    AnimationData* front = back->overlay;
    if (front->unloaded || front->unloadPending || !back->asset || !front->asset ||
        !back->paletteResources || !front->paletteResources) return false;
    if (back->selectedPaletteBank >= back->paletteResources->banks.size() ||
        front->selectedPaletteBank >= front->paletteResources->banks.size()) return false;
    const AnimationPaletteBank& backBank = back->paletteResources->banks[back->selectedPaletteBank];
    const AnimationPaletteBank& frontBank = front->paletteResources->banks[front->selectedPaletteBank];
    if (!backBank.texValid || !frontBank.texValid) return false;
    const int backIndex = back->selectedFrame;
    const int frontIndex = front->selectedFrame;
    if (backIndex < 0 || frontIndex < 0 ||
        backIndex >= (int)back->asset->frames.size() ||
        frontIndex >= (int)front->asset->frames.size() ||
        backIndex >= (int)backBank.frameTex.size() ||
        frontIndex >= (int)frontBank.frameTex.size()) return false;
    const unsigned int backTexture = backBank.frameTex[backIndex];
    const unsigned int frontTexture = frontBank.frameTex[frontIndex];
    if (!backTexture || !frontTexture) return false;

    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer) return false;
    int backWidth, backHeight, frontWidth, frontHeight;
    renderer->GetTextureDimensions(backTexture, backWidth, backHeight);
    renderer->GetTextureDimensions(frontTexture, frontWidth, frontHeight);
    if (backWidth <= 0 || backHeight <= 0 || frontWidth <= 0 || frontHeight <= 0)
        return false;

    // Retail type 11 uploads each texture resource's logical extent to c467;
    // the vertex shader divides the primary extent by the attached extent.
    const float frontScaleX = (float)backWidth / (float)frontWidth;
    const float frontScaleY = (float)backHeight / (float)frontHeight;
    float pr, pg, pb, pa;
    renderer->GetColour(pr, pg, pb, pa);
    renderer->SetColour(1.0f, 1.0f, 1.0f, pa);
    const bool drawn = DrawAnimationShapeOverlayWithTransform(a_machine,
        ShapeForAnimation(back), backTexture, frontTexture,
        frontScaleX, frontScaleY, pa,
        a_a, a_b, a_c, a_d, a_e, a_f, a_z);
    renderer->SetColour(pr, pg, pb, pa);
    return drawn;
}
