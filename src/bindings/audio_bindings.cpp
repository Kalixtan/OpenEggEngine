#include "gmThread.h"
#include "audio_bindings.h"
#include "audio_recorder_signatures.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "../core/engine_context.h"
#include "../core/save_data.h"
#include "../core/strict_calls.h"
#include "../ported_native/ground_math.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdio>
#include <array>
#include <fstream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <vector>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <map>
#include <zlib.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "../../vendor/minimp3/minimp3_ex.h"
#if defined(_WIN32) && defined(GetObject)
// minimp3_ex.h includes windows.h for its file mapping support. Windows' GDI
// GetObject macro would rewrite gmMachine::GetObject calls throughout this
// translation unit, so remove the macro after the third-party include.
#undef GetObject
#endif


namespace {

constexpr int kOutputRate = 48000;
struct DecodedSound {
    std::vector<float> stereo;
    int sourceChannels = 2;
};
struct DeferredEffectCleanup {
    int voiceId = 0;
    gmVariable object;
};
struct RecordedPlaySoundEvent {
    float timeMs = 0.0f;
    std::string path;
    int note = 96;
    float gate = 0.0f;
    float pan = 0.0f;
    float volume = 1.0f;
    int priority = 0x80;
    bool chainRoute = false;
    gmVariable effectChain = gmVariable::s_null;
    uint32_t effectChainHandle = 0xffffffffu;
    int startPositionMs = 0;
};
PlatformFrontend* g_audioFrontend = nullptr;
bool g_audioOutputStarted = false;
std::FILE* g_audioCaptureFile = nullptr;
double g_fractionalOutputFrames = 0.0;
std::unordered_map<std::string, std::weak_ptr<DecodedSound>> g_decodedSounds;
struct TimeStretchCacheEntry {
    std::shared_ptr<DecodedSound> sample;
    uint64_t lastUse = 0;
};
std::unordered_map<std::string, TimeStretchCacheEntry> g_timeStretchedSounds;
uint64_t g_timeStretchCacheClock = 0;
// Audio.loadSound creates a manager-owned sample resource in retail. Keep a
// strong desktop reference for explicit loads so successful loading actually
// decodes and retains the sample until unloadSound or shutdown.
std::unordered_map<std::string, std::shared_ptr<DecodedSound>> g_loadedSoundSamples;
std::unordered_map<std::string, int> g_loadedSoundModes;
std::unordered_map<std::string, int> g_loadedSoundResourceKinds;

struct VoiceState {
    std::string soundFile;
    float volume = 1.0f;
    float pan = 0.0f;
    // Native playSound forwards this to the backend voice allocator. The
    // software mixer currently has no voice-stealing policy, but retain the
    // exact requested priority for diagnostics and future allocation parity.
    int priority = 0x80;
    // Keep the selected explicit or manager-default script chain alive and
    // associated with the voice. The mixer still lacks the retail DSP route.
    gmVariable effectChain = gmVariable::s_null;
    uint32_t effectChainHandle = 0xffffffffu;
    int referenceNote = 48;
    // Normalized source base frequency stored at retail voice payload +0x10.
    // getVoiceSpeed divides the live backend frequency by this value. Note,
    // pitch-bend, and setVoiceSpeed targets are also based on this unscaled
    // value; playSound applies the global multiplier to the initial backend
    // frequency separately.
    float sourceBaseSpeed = 1.0f;
    bool pitchBendPending = false;
    float pendingPitchSpeed = 1.0f;
    float pendingPitchDurationMs = 0.0f;
    float speed = 1.0f;
    float targetSpeed = 1.0f;
    // spawnSoundSlicker's time factor configures the retail Slicker/Dirac
    // stretch object. It changes duration while preserving the note pitch;
    // it must not be folded into `speed`, which is the sample playback rate.
    // The SDL fallback applies this with a cached WSOLA sample transform;
    // retail's exact FMOD/Dirac processing remains unported.
    float timeStretchFactor = 1.0f;
    float speedRampRate = 0.0f;
    float loopGainScale = 1.0f;
    double positionMs = 0.0;
    double gateDurationMs = 0.0;
    double gateElapsedMs = 0.0;
    bool gateFading = false;
    double fadeRemainingMs = 0.0;
    double fadeDurationMs = 0.0;
    float fadeGain = 1.0f;
    double stopRemainingMs = 0.0;
    bool stopping = false;
    bool paused = false;
    bool looping = false;
    int loopCount = 0;
    bool ownsDiracPoolEntry = false;
    std::shared_ptr<DecodedSound> sample;
    std::shared_ptr<DecodedSound> timeStretchedSample;
    bool sampleLoadAttempted = false;
};

struct AudioState {
    std::unordered_map<int, VoiceState> voices;
    // Observed PS3 pool total after audio setup. The retail total comes from
    // its backend allocator, so this may differ on other game versions.
    static constexpr int kDiracPoolCapacity = 7;
    size_t diracPoolEntriesUsed = 0;
    std::vector<DeferredEffectCleanup> effectChainsWaitingForVoiceEnd;
    std::vector<DeferredEffectCleanup> effectsWaitingForVoiceEnd;
    std::unordered_set<std::string> loadedSounds;
    // FunctionObjectRecorderV4's interned voice-resource-name set. Retail
    // adds names before the playSound enabled gate and resets it when script
    // recording starts; saveScriptToFile will consume this state later.
    std::set<std::string> recordedSoundResources;
    // Retail queues ordinary and 7/8-argument playSound callbacks as
    // different timed event classes. Keep the raw callback arguments for
    // the later FunctionObjectRecorderV4 serializer.
    std::vector<RecordedPlaySoundEvent> recordedPlaySounds;
    std::chrono::steady_clock::time_point recordingStart{};
    float recordingStopMs = 0.0f;
    // The desktop allocator uses ordinary voice IDs; this set mirrors the
    // reserved-ID range owned by retail script playback.
    std::unordered_set<int> scriptPlaybackVoiceIds;
    size_t playbackEventIndex = 0;
    float playbackElapsedMs = 0.0f;
    std::unordered_map<std::string, int> soundLengthMs;
    int nextVoiceId = 1;
    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    bool enabled = true; // Retail gates playback/Slicker paths at manager +0x2c3c
    float frequencyMultiplier = 1.0f; // constructor initializes +0x30f0 to 1.0
    // Retail FunctionObjectRecorder scheduler mode byte at +0x31.
    bool playingScript = false;
    // The adjacent recorder mode byte at scheduler +0x30.
    bool recordingScript = false;
    gmMachine* machine = nullptr;
};
AudioState& State() {
    static AudioState s;
    return s;
}

void ReleaseDiracPoolEntry(VoiceState& a_voice) {
    if (!a_voice.ownsDiracPoolEntry) return;
    a_voice.ownsDiracPoolEntry = false;
    if (State().diracPoolEntriesUsed > 0) --State().diracPoolEntriesUsed;
}

class EffectChainHandlePool {
public:
    EffectChainHandlePool() { Reset(); }

    void Reset() {
        for (uint32_t slot = 0; slot < kSlotCount; ++slot)
            m_nextFree[slot] = slot + 1 < kSlotCount ? slot + 1 : kInvalidSlot;
        m_liveHandle.fill(kInvalidHandle);
        m_freeHead = 0;
        m_generation = 0;
    }

    uint32_t Allocate() {
        if (m_freeHead == kInvalidSlot) return kInvalidHandle;
        const uint32_t slot = m_freeHead;
        m_freeHead = m_nextFree[slot];
        uint32_t handle = 0;
        do {
            handle = ((m_generation & kGenerationMask) << 8) | slot;
            m_generation = (m_generation + 1) & kGenerationMask;
        } while (handle == kInvalidHandle);
        m_liveHandle[slot] = handle;
        return handle;
    }

    bool IsLive(uint32_t a_handle) const {
        if (a_handle == kInvalidHandle) return false;
        const uint32_t slot = a_handle & 0xffu;
        return m_liveHandle[slot] == a_handle;
    }

    bool Release(uint32_t a_handle) {
        if (!IsLive(a_handle)) return false;
        const uint32_t slot = a_handle & 0xffu;
        m_liveHandle[slot] = kInvalidHandle;
        m_nextFree[slot] = m_freeHead;
        m_freeHead = slot;
        return true;
    }

    static constexpr uint32_t kInvalidHandle = 0xffffffffu;

private:
    static constexpr uint32_t kSlotCount = 256;
    static constexpr uint32_t kInvalidSlot = 0xffffffffu;
    static constexpr uint32_t kGenerationMask = 0x000fffffu;
    std::array<uint32_t, kSlotCount> m_nextFree{};
    std::array<uint32_t, kSlotCount> m_liveHandle = [] {
        std::array<uint32_t, kSlotCount> values{};
        values.fill(kInvalidHandle);
        return values;
    }();
    uint32_t m_freeHead = 0;
    uint32_t m_generation = 0;
};
EffectChainHandlePool& ChainHandles() {
    static EffectChainHandlePool pool;
    return pool;
}
gmVariable GetOrCreateMasterEffectChain(gmMachine& a_machine);

struct AudioGMRootState { unsigned uses = 0; bool addedRoot = false; };
std::unordered_map<gmObject*, AudioGMRootState> g_audioGMRoots;
// Retail links each backend voice into exactly one channel group. Keep the
// same one-group membership invariant even though this frontend has no FMOD
// channel-group object or DSP graph to attach.
std::unordered_map<gmObject*, std::unordered_set<int>> g_effectChainVoiceIds;
void RetainAudioGMVariable(gmMachine& a_machine, const gmVariable& a_value) {
    if (!a_value.IsReference()) return;
    gmObject* object = a_machine.GetObject(a_value.m_value.m_ref);
    if (!object) return;
    auto& root = g_audioGMRoots[object];
    if (root.uses++ == 0 && !a_machine.IsCPPOwnedGMObject(object)) {
        a_machine.AddCPPOwnedGMObject(object);
        root.addedRoot = true;
    }
}
void ReleaseAudioGMVariable(gmMachine& a_machine, const gmVariable& a_value) {
    if (!a_value.IsReference()) return;
    gmObject* object = a_machine.GetObject(a_value.m_value.m_ref);
    auto it = g_audioGMRoots.find(object);
    if (!object || it == g_audioGMRoots.end()) return;
    if (it->second.uses > 0 && --it->second.uses == 0) {
        if (it->second.addedRoot && a_machine.IsCPPOwnedGMObject(object))
            a_machine.RemoveCPPOwnedGMObject(object);
        g_audioGMRoots.erase(it);
    }
}
bool IsLocalEffectChain(gmMachine& a_machine, const gmVariable& a_value) {
    if (a_value.m_type != GM_TABLE) return false;
    auto* table = static_cast<gmTableObject*>(
        a_machine.GetObject(a_value.m_value.m_ref));
    if (!table || table->Get(&a_machine, "__freed").m_type != GM_NULL ||
        table->Get(&a_machine, "__name").m_type != GM_STRING ||
        table->Get(&a_machine, "addEffect").m_type != GM_FUNCTION)
        return false;
    const gmVariable handle = table->Get(&a_machine, "__nativeHandle");
    return handle.m_type == GM_INT &&
           ChainHandles().IsLive(static_cast<uint32_t>(handle.m_value.m_int));
}
std::unordered_map<uint32_t, gmVariable> g_effectChainsByHandle;
gmVariable FindEffectChainByHandle(gmMachine& a_machine, uint32_t a_handle) {
    const auto it = g_effectChainsByHandle.find(a_handle);
    if (it == g_effectChainsByHandle.end() || !ChainHandles().IsLive(a_handle))
        return gmVariable::s_null;
    auto* table = static_cast<gmTableObject*>(
        a_machine.GetObject(it->second.m_value.m_ref));
    if (!table || !IsLocalEffectChain(a_machine, it->second))
        return gmVariable::s_null;
    const gmVariable currentHandle = table->Get(&a_machine, "__nativeHandle");
    if (currentHandle.m_type != GM_INT ||
        static_cast<uint32_t>(currentHandle.m_value.m_int) != a_handle)
        return gmVariable::s_null;
    return it->second;
}
float RecordedAudioElapsedMs() {
    const auto elapsed = std::chrono::steady_clock::now() - State().recordingStart;
    return std::chrono::duration<float, std::milli>(elapsed).count();
}
void ClearRecordedPlaySounds(gmMachine& a_machine) {
    for (const auto& event : State().recordedPlaySounds) {
        if (event.effectChain.IsReference())
            ReleaseAudioGMVariable(a_machine, event.effectChain);
    }
    State().recordedPlaySounds.clear();
}
void DetachVoiceFromEffectChain(gmMachine& a_machine, int a_voiceId,
                                VoiceState& a_voice) {
    if (a_voice.effectChain.IsReference()) {
        gmObject* object = a_machine.GetObject(a_voice.effectChain.m_value.m_ref);
        auto membership = g_effectChainVoiceIds.find(object);
        if (membership != g_effectChainVoiceIds.end()) {
            membership->second.erase(a_voiceId);
            if (membership->second.empty()) g_effectChainVoiceIds.erase(membership);
        }
        ReleaseAudioGMVariable(a_machine, a_voice.effectChain);
    }
    a_voice.effectChain = gmVariable::s_null;
    a_voice.effectChainHandle = 0xffffffffu;
}
void AttachEffectChainToVoice(gmMachine& a_machine, int a_voiceId,
                              VoiceState& a_voice, const gmVariable& a_chain) {
    if (a_voice.effectChain.m_type == a_chain.m_type &&
        (!a_chain.IsReference() ||
         a_voice.effectChain.m_value.m_ref == a_chain.m_value.m_ref)) {
        if (a_chain.IsReference()) {
            gmObject* object = a_machine.GetObject(a_chain.m_value.m_ref);
            if (object) g_effectChainVoiceIds[object].insert(a_voiceId);
        }
        return;
    }
    DetachVoiceFromEffectChain(a_machine, a_voiceId, a_voice);
    a_voice.effectChain = a_chain;
    if (a_voice.effectChain.IsReference()) {
        RetainAudioGMVariable(a_machine, a_voice.effectChain);
        gmObject* object = a_machine.GetObject(a_voice.effectChain.m_value.m_ref);
        if (object) g_effectChainVoiceIds[object].insert(a_voiceId);
        if (a_chain.m_type == GM_TABLE) {
            auto* table = static_cast<gmTableObject*>(object);
            const gmVariable handle = table
                ? table->Get(&a_machine, "__nativeHandle") : gmVariable::s_null;
            if (handle.m_type == GM_INT)
                a_voice.effectChainHandle =
                    static_cast<uint32_t>(handle.m_value.m_int);
        }
    }
}
void StopRecordedPlaybackVoices() {
    AudioState& state = State();
    if (state.machine) {
        for (const int id : state.scriptPlaybackVoiceIds) {
            const auto voice = state.voices.find(id);
            if (voice == state.voices.end()) continue;
            DetachVoiceFromEffectChain(*state.machine, id, voice->second);
            ReleaseDiracPoolEntry(voice->second);
            state.voices.erase(voice);
        }
    }
    state.scriptPlaybackVoiceIds.clear();
}
void ProcessDeferredEffectCleanup(gmMachine* a_machine,
                                 std::vector<DeferredEffectCleanup>& a_pending,
                                 bool a_chain);

float RetailClampAudioVolume(float a_value) {
    const double value = static_cast<double>(a_value);
    const double lowerTest = 0.0 - value;
    const double lowerClamped = std::signbit(lowerTest) ? value : 0.0;
    const double upperTest = 1.0 - lowerClamped;
    const double result = std::signbit(upperTest) ? 1.0 : lowerClamped;
    return static_cast<float>(result);
}

// PPC fsel selects the nonnegative operand unless its condition is less than
// zero. An unordered NaN comparison therefore takes the nonnegative branch.
float RetailFsel(float a_condition, float a_nonnegative, float a_negative) {
    return a_condition < 0.0f ? a_negative : a_nonnegative;
}

// Force an observable binary32 store/load between PPC `fmuls` stages. This
// prevents the host optimizer from contracting the retail multiply chain into
// a single fused operation in optimized builds.
float RetailF32(float a_value) {
    volatile float rounded = a_value;
    return rounded;
}

// PPC fctiwz truncates finite in-range floats toward zero. Invalid and
// out-of-range conversions produce the integer-indefinite bit pattern.
int RetailFctiwzToInt(float a_value) {
    const double value = static_cast<double>(a_value);
    const double minValue = static_cast<double>(std::numeric_limits<int>::min());
    const double maxExclusive =
        static_cast<double>(std::numeric_limits<int>::max()) + 1.0;
    if (!std::isfinite(value) || value < minValue || value >= maxExclusive)
        return std::numeric_limits<int>::min();
    return static_cast<int>(value);
}

VoiceState* FindVoice(int a_id) {
    auto it = State().voices.find(a_id);
    return it == State().voices.end() ? nullptr : &it->second;
}

bool CanQueryVoiceBackend(const VoiceState& a_voice) {
    // A decoded voice has a channel even when the headless runner has no
    // output device. Output setup only controls playback, not voice queries.
    // An unattempted decode also represents a channel until loading fails.
    return !a_voice.sampleLoadAttempted ||
        (a_voice.sample && a_voice.sample->stereo.size() >= 2);
}

void SetVoiceSpeedTarget(VoiceState& a_voice, float a_target,
                         float a_durationMs) {
    if (!std::isfinite(a_target)) return;
    a_voice.targetSpeed = a_target;
    if (std::isfinite(a_durationMs) && a_durationMs != 0.0f) {
        // Retail stores one fixed f32 frequency delta per millisecond, then
        // applies rate * elapsed time on each audio-manager update. A negative
        // nonzero duration therefore produces the same reverse-direction rate.
        a_voice.speedRampRate =
            (a_target - a_voice.speed) / a_durationMs;
    } else {
        a_voice.speed = a_target;
        a_voice.speedRampRate = 0.0f;
    }
}

// Integrate the retail speed target's linear ramp through a partial frame.
// The current voice.speed is the value at this frame's start.
double PlaybackAdvanceMs(const VoiceState& a_voice, double a_elapsedMs) {
    if (a_voice.speedRampRate == 0.0f)
        return a_elapsedMs * a_voice.speed;
    const double slope = a_voice.speedRampRate;
    const double timeToTarget =
        (a_voice.targetSpeed - a_voice.speed) / slope;
    const double rampMs = timeToTarget >= 0.0
        ? std::min(a_elapsedMs, timeToTarget) : a_elapsedMs;
    return rampMs * a_voice.speed + 0.5 * slope * rampMs * rampMs +
           (a_elapsedMs - rampMs) * a_voice.targetSpeed;
}

// Short windowed-sinc filter limits aliasing when playback rate changes.
using ResampleReader = float (*)(const void*, size_t, size_t);
float ReadStereoFloat(const void* a_samples, size_t a_frame, size_t a_channel) {
    const auto* samples = static_cast<const float*>(a_samples);
    return samples[a_frame * 2 + a_channel];
}
struct Pcm16Source {
    const int16_t* samples = nullptr;
    size_t channels = 1;
};
float ReadPcm16(const void* a_source, size_t a_frame, size_t a_channel) {
    const auto* source = static_cast<const Pcm16Source*>(a_source);
    const size_t channel = source->channels == 1 ? 0 : a_channel;
    return source->samples[a_frame * source->channels + channel] / 32768.0f;
}

float InterpolatedSample(const void* a_source, ResampleReader a_read,
                        size_t a_frameCount, size_t a_channel,
                        double a_sourceFrame, double a_sourceStep,
                        bool a_loop = false,
                        bool a_exactIntegerPlaybackFrame = false) {
    if (!a_source || !a_read || a_frameCount == 0) return 0.0f;
    const double integral = std::floor(a_sourceFrame);
    const int64_t base = static_cast<int64_t>(integral);
    const double fraction = std::clamp(a_sourceFrame - integral, 0.0, 1.0);
    auto resolveIndex = [a_frameCount, a_loop](int64_t index) {
        if (a_loop) {
            const int64_t length = static_cast<int64_t>(a_frameCount);
            index %= length;
            if (index < 0) index += length;
            return static_cast<size_t>(index);
        }
        return static_cast<size_t>(std::clamp<int64_t>(
            index, 0, static_cast<int64_t>(a_frameCount - 1)));
    };
    // Retail playback reads the exact PCM frame at integer positions; the
    // playback rate alone does not trigger the decode resampling filter.
    if (fraction == 0.0 &&
        (a_exactIntegerPlaybackFrame ||
         std::fabs(a_sourceStep - 1.0) < 1.0e-6))
        return a_read(a_source, resolveIndex(base), a_channel);

    constexpr int radius = 2;
    const double cutoff = std::min(1.0, 1.0 / std::max(1.0, a_sourceStep));
    double weighted = 0.0;
    double weightSum = 0.0;
    for (int tap = -radius + 1; tap <= radius; ++tap) {
        const double offset = fraction - tap;
        if (std::fabs(offset) >= radius) continue;
        const double scaled = offset * cutoff;
        const double sinc = std::fabs(scaled) < 1.0e-12
            ? 1.0 : std::sin(3.14159265358979323846 * scaled) /
                   (3.14159265358979323846 * scaled);
        const double window = 0.42 + 0.5 * std::cos(
            3.14159265358979323846 * offset / radius) +
            0.08 * std::cos(2.0 * 3.14159265358979323846 * offset / radius);
        const double weight = cutoff * sinc * window;
        weighted += a_read(a_source, resolveIndex(base + tap), a_channel) * weight;
        weightSum += weight;
    }
    return weightSum != 0.0 ? static_cast<float>(weighted / weightSum) : 0.0f;
}

// Retail Slicker stores tempo and pitch as independent controls and feeds the
// tempo value to its Dirac processor. The desktop SDL backend has no Dirac
// plugin, so use WSOLA to change clip duration without changing its pitch.
// This operates once when a Slicker voice is created; the real-time mixer then
// treats the result as an ordinary 48 kHz sample and applies note pitch via
// playback rate, as the retail path does after time stretching.
std::shared_ptr<DecodedSound> BuildTimeStretchedSample(
        const std::string& a_path, const std::shared_ptr<DecodedSound>& a_source,
        float a_factor) {
    if (!a_source || a_source->stereo.size() < 2 ||
        !std::isfinite(a_factor) || a_factor <= 0.0f ||
        std::fabs(a_factor - 1.0f) < 1.0e-6f)
        return {};

    std::ostringstream keyBuilder;
    keyBuilder.imbue(std::locale::classic());
    keyBuilder << a_path << '\n' << std::setprecision(9) << a_factor;
    const std::string key = keyBuilder.str();
    if (auto found = g_timeStretchedSounds.find(key);
        found != g_timeStretchedSounds.end()) {
        found->second.lastUse = ++g_timeStretchCacheClock;
        return found->second.sample;
    }

    const size_t sourceFrames = a_source->stereo.size() / 2;
    // The shipped scripts derive this factor from source BPM / sequencer BPM;
    // ordinary values stay close to 1. Avoid pathological allocations for
    // malformed external calls and leave those voices on their source sample.
    if (a_factor < 0.125f || a_factor > 8.0f) return {};
    const long double wantedFrames =
        static_cast<long double>(sourceFrames) * a_factor;
    if (wantedFrames < 1.0L || wantedFrames > 48000000.0L) return {};
    const size_t outputFrames = static_cast<size_t>(std::llround(wantedFrames));
    if (outputFrames == 0 || outputFrames >
        std::numeric_limits<size_t>::max() / (2 * sizeof(float))) return {};

    const size_t window = std::min<size_t>(2048, sourceFrames);
    if (window < 128) return {};
    const size_t inputHop = std::max<size_t>(32, window / 4);
    const size_t outputHop = std::max<size_t>(1,
        static_cast<size_t>(std::llround(inputHop * a_factor)));
    if (outputHop >= window) return {};
    const size_t overlap = window - outputHop;
    const size_t search = std::max<size_t>(8, window / 16);
    const size_t maxSourceStart = sourceFrames - window;

    auto stretched = std::make_shared<DecodedSound>();
    stretched->sourceChannels = a_source->sourceChannels;
    stretched->stereo.assign(outputFrames * 2, 0.0f);
    const size_t firstCopy = std::min(window, outputFrames);
    std::copy_n(a_source->stereo.begin(), firstCopy * 2,
                stretched->stereo.begin());
    size_t previousSourceStart = 0;

    for (size_t outputStart = outputHop; outputStart < outputFrames;
         outputStart += outputHop) {
        const size_t compareFrames = std::min(overlap, outputFrames - outputStart);
        if (compareFrames == 0) break;
        const size_t expected = std::min(maxSourceStart,
                                         previousSourceStart + inputHop);
        const size_t low = expected > search ? expected - search : 0;
        const size_t high = std::min(maxSourceStart, expected + search);
        auto alignmentScore = [&](size_t candidate, size_t sampleCount) {
            double dot = 0.0;
            double outputEnergy = 0.0;
            double sourceEnergy = 0.0;
            for (size_t sample = 0; sample < sampleCount; ++sample) {
                const size_t offset = sampleCount <= 1 ? 0 :
                    sample * (compareFrames - 1) / (sampleCount - 1);
                const size_t outputIndex = (outputStart + offset) * 2;
                const size_t sourceIndex = (candidate + offset) * 2;
                const double outMono = (stretched->stereo[outputIndex] +
                                        stretched->stereo[outputIndex + 1]) * 0.5;
                const double inMono = (a_source->stereo[sourceIndex] +
                                       a_source->stereo[sourceIndex + 1]) * 0.5;
                dot += outMono * inMono;
                outputEnergy += outMono * outMono;
                sourceEnergy += inMono * inMono;
            }
            const double denominator = std::sqrt(outputEnergy * sourceEnergy);
            return denominator > 1.0e-18 ? dot / denominator :
                -std::fabs(static_cast<double>(candidate) - expected);
        };

        // Long musical loops can contain hundreds of thousands of frames.
        // Exhaustively evaluating up to 257 offsets with 192 sample pairs for
        // every overlap made the first transform stall the simulation thread.
        // Search every second frame with half as many distributed samples,
        // then score the five neighboring offsets at full resolution. This
        // keeps the same correlation criterion and final full-resolution
        // alignment while reducing the dominant work by roughly 4x.
        constexpr size_t kCoarseCandidateStride = 2;
        const size_t coarseSampleCount = std::min<size_t>(96, compareFrames);
        size_t bestSourceStart = low;
        double bestScore = -std::numeric_limits<double>::infinity();
        for (size_t candidate = low; candidate <= high;) {
            const double score = alignmentScore(candidate, coarseSampleCount);
            if (score > bestScore) {
                bestScore = score;
                bestSourceStart = candidate;
            }
            if (high - candidate < kCoarseCandidateStride) break;
            candidate += kCoarseCandidateStride;
        }
        if (bestSourceStart != high && (high - low) % kCoarseCandidateStride != 0) {
            const double score = alignmentScore(high, coarseSampleCount);
            if (score > bestScore) {
                bestScore = score;
                bestSourceStart = high;
            }
        }
        const size_t refineLow = bestSourceStart > low + kCoarseCandidateStride
            ? bestSourceStart - kCoarseCandidateStride : low;
        const size_t refineHigh = std::min(high,
            bestSourceStart + kCoarseCandidateStride);
        const size_t fullSampleCount = std::min<size_t>(192, compareFrames);
        bestScore = -std::numeric_limits<double>::infinity();
        for (size_t candidate = refineLow; candidate <= refineHigh; ++candidate) {
            const double score = alignmentScore(candidate, fullSampleCount);
            if (score > bestScore) {
                bestScore = score;
                bestSourceStart = candidate;
            }
        }

        const size_t blendFrames = std::min(overlap, outputFrames - outputStart);
        for (size_t frame = 0; frame < blendFrames; ++frame) {
            const double t = static_cast<double>(frame + 1) /
                             static_cast<double>(blendFrames + 1);
            const float incoming = static_cast<float>(
                0.5 - 0.5 * std::cos(3.14159265358979323846 * t));
            const float outgoing = 1.0f - incoming;
            const size_t outIndex = (outputStart + frame) * 2;
            const size_t inIndex = (bestSourceStart + frame) * 2;
            for (size_t channel = 0; channel < 2; ++channel) {
                stretched->stereo[outIndex + channel] =
                    stretched->stereo[outIndex + channel] * outgoing +
                    a_source->stereo[inIndex + channel] * incoming;
            }
        }
        const size_t tailCount = std::min(outputHop,
            outputFrames - (outputStart + blendFrames));
        if (tailCount != 0) {
            const size_t sourceTail = bestSourceStart + blendFrames;
            const size_t destTail = outputStart + blendFrames;
            std::copy_n(a_source->stereo.begin() + sourceTail * 2,
                        tailCount * 2,
                        stretched->stereo.begin() + destTail * 2);
        }
        previousSourceStart = bestSourceStart;
    }

    g_timeStretchedSounds[key] = {stretched, ++g_timeStretchCacheClock};
    constexpr size_t kMaxCachedTimeStretches = 32;
    constexpr size_t kMaxCachedTimeStretchBytes = 128u * 1024u * 1024u;
    auto cachedBytes = [&]() {
        size_t bytes = 0;
        for (const auto& entry : g_timeStretchedSounds) {
            const size_t samples = entry.second.sample
                ? entry.second.sample->stereo.size() : 0;
            if (samples > (std::numeric_limits<size_t>::max() - bytes) /
                              sizeof(float))
                return std::numeric_limits<size_t>::max();
            bytes += samples * sizeof(float);
        }
        return bytes;
    };
    while (g_timeStretchedSounds.size() > kMaxCachedTimeStretches ||
           cachedBytes() > kMaxCachedTimeStretchBytes) {
        auto oldest = std::min_element(g_timeStretchedSounds.begin(),
            g_timeStretchedSounds.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.second.lastUse < rhs.second.lastUse;
            });
        if (oldest == g_timeStretchedSounds.end()) break;
        g_timeStretchedSounds.erase(oldest);
    }
    return stretched;
}

uint32_t ReadU32LE(const unsigned char* a_bytes) {
    return (uint32_t)a_bytes[0] | ((uint32_t)a_bytes[1] << 8) |
           ((uint32_t)a_bytes[2] << 16) | ((uint32_t)a_bytes[3] << 24);
}
uint32_t ReadU32BE(const unsigned char* a_bytes) {
    return ((uint32_t)a_bytes[0] << 24) | ((uint32_t)a_bytes[1] << 16) |
           ((uint32_t)a_bytes[2] << 8) | (uint32_t)a_bytes[3];
}

// Duration is script-visible even before desktop audio output exists:
// EntityBasePS3 uses getSoundLength to compute note gate times, and the
// campaign record player seeks relative to getVoiceLength. Read only the
// container headers, keeping memory bounded for long level recordings.
int ReadSoundLengthMs(const std::string& a_path) {
    FileHelper* files = GetEngineContext().fileHelper;
    if (!files) return 0;
    auto resolved = files->Resolve(a_path);
    if (!resolved) return 0;
    std::ifstream stream(std::filesystem::u8path(*resolved), std::ios::binary);
    if (!stream) return 0;
    std::array<unsigned char, 12> header{};
    if (!stream.read((char*)header.data(), header.size())) return 0;

    if (std::memcmp(header.data(), "VAGp", 4) == 0) {
        // Standard Sony VAG header: big-endian encoded byte count at +12,
        // sample rate at +16. Each 16-byte ADPCM block holds 28 samples.
        std::array<unsigned char, 8> fields{};
        if (!stream.read((char*)fields.data(), fields.size())) return 0;
        uint32_t bytes = ReadU32BE(fields.data());
        uint32_t rate = ReadU32BE(fields.data() + 4);
        if (!rate) return 0;
        uint64_t ms = ((uint64_t)bytes / 16) * 28 * 1000 / rate;
        return ms <= (uint64_t)std::numeric_limits<int>::max() ? (int)ms : 0;
    }

    if (std::memcmp(header.data(), "RIFF", 4) != 0 ||
        std::memcmp(header.data() + 8, "WAVE", 4) != 0) return 0;
    uint32_t byteRate = 0, dataBytes = 0;
    for (int i = 0; i < 128 && stream; ++i) {
        std::array<unsigned char, 8> chunk{};
        if (!stream.read((char*)chunk.data(), chunk.size())) break;
        uint32_t size = ReadU32LE(chunk.data() + 4);
        if (std::memcmp(chunk.data(), "fmt ", 4) == 0 && size >= 16) {
            std::array<unsigned char, 16> fmt{};
            if (!stream.read((char*)fmt.data(), fmt.size())) break;
            byteRate = ReadU32LE(fmt.data() + 8);
            stream.seekg((std::streamoff)size - 16, std::ios::cur);
        } else {
            if (std::memcmp(chunk.data(), "data", 4) == 0) dataBytes = size;
            stream.seekg((std::streamoff)size, std::ios::cur);
        }
        if (size & 1) stream.seekg(1, std::ios::cur);
        if (byteRate && dataBytes) break;
    }
    if (!byteRate) return 0;
    uint64_t ms = (uint64_t)dataBytes * 1000 / byteRate;
    return ms <= (uint64_t)std::numeric_limits<int>::max() ? (int)ms : 0;
}

int SoundLengthMs(const std::string& a_path) {
    auto& cache = State().soundLengthMs;
    auto it = cache.find(a_path);
    if (it != cache.end()) return it->second;
    return cache.emplace(a_path, ReadSoundLengthMs(a_path)).first->second;
}

bool DecodeVagFile(const std::string& a_filePath, std::vector<float>& a_stereo,
                   int& a_sourceChannels) {
    std::ifstream stream(std::filesystem::u8path(a_filePath), std::ios::binary);
    if (!stream) return false;
    std::array<unsigned char, 48> header{};
    if (!stream.read(reinterpret_cast<char*>(header.data()), header.size()) ||
        std::memcmp(header.data(), "VAG", 3) != 0) return false;

    const uint32_t payloadBytes = ReadU32BE(header.data() + 12);
    const uint32_t sourceRate = ReadU32BE(header.data() + 16);
    if (!sourceRate || sourceRate > 384000 || payloadBytes == 0 ||
        payloadBytes % 16 != 0) return false;
    stream.seekg(0, std::ios::end);
    const std::streamoff fileSize = stream.tellg();
    if (fileSize < 48 || payloadBytes > static_cast<uint64_t>(fileSize - 48)) return false;
    stream.seekg(48, std::ios::beg);

    const size_t blockCount = payloadBytes / 16;
    const size_t frameCount = blockCount * 28;
    if (!frameCount || frameCount > std::numeric_limits<size_t>::max() / 2) return false;
    a_sourceChannels = 1;
    std::vector<unsigned char> payload(payloadBytes);
    if (!stream.read(reinterpret_cast<char*>(payload.data()), payload.size())) return false;

    static constexpr uint32_t coefficientBits[16][2] = {
        {0x00000000, 0x00000000}, {0x3f700000, 0x00000000},
        {0x3fe60000, 0xbf500000}, {0x3fc40000, 0xbf5c0000},
        {0x3ff40000, 0xbf700000}, {0x00000001, 0x00000010},
        {0x800000aa, 0x00389b71}, {0x00000003, 0x00000010},
        {0x800000aa, 0x00389b71}, {0x00000000, 0x00000000},
        {0x00070008, 0x0009000a}, {0x000b000c, 0x000d000e},
        {0x00100011, 0x00130015}, {0x00170019, 0x001c001f},
        {0x00220025, 0x0029002d}, {0x00320037, 0x003c0042}
    };
    float previous1 = 0.0f;
    float previous2 = 0.0f;
    std::vector<int16_t> decoded(frameCount);
    for (size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
        const unsigned char* block = payload.data() + blockIndex * 16;
        const int shift = block[0] & 0x0f;
        const int filter = block[0] >> 4;
        float filter0, filter1;
        std::memcpy(&filter0, &coefficientBits[filter][0], sizeof(filter0));
        std::memcpy(&filter1, &coefficientBits[filter][1], sizeof(filter1));
        for (size_t sampleIndex = 0; sampleIndex < 28; ++sampleIndex) {
            const unsigned char packed = block[2 + sampleIndex / 2];
            int nibble = (sampleIndex & 1) ? (packed >> 4) : (packed & 0x0f);
            if (nibble >= 8) nibble -= 16;
            const int32_t unshifted = nibble * 4096;
            const int32_t divisor = 1 << shift;
            const int32_t residual = unshifted >= 0
                ? unshifted / divisor
                : -((-unshifted + divisor - 1) / divisor);
            // Retail uses two fused `fmadds` instructions for its predictor,
            // then a single-precision `fadds` of 0.5 followed by `fctiwz`.
            const float firstPredict = std::fma(previous1, filter0,
                                                static_cast<float>(residual));
            const float prediction = std::fma(previous2, filter1, firstPredict);
            const float rounded = RetailF32(prediction + 0.5f);
            const int32_t quantized = static_cast<int32_t>(rounded);
            const uint16_t wrapped = static_cast<uint16_t>(quantized);
            int16_t pcm;
            std::memcpy(&pcm, &wrapped, sizeof(pcm));
            decoded[blockIndex * 28 + sampleIndex] = pcm;
            previous2 = previous1;
            previous1 = prediction;
        }
    }

    const size_t outputFrames = sourceRate == kOutputRate ? frameCount :
        std::max<size_t>(1, static_cast<size_t>(
            (static_cast<uint64_t>(frameCount) * kOutputRate + sourceRate / 2) / sourceRate));
    a_stereo.resize(outputFrames * 2);
    const Pcm16Source source{decoded.data(), 1};
    const double sourceStep = static_cast<double>(sourceRate) / kOutputRate;
    for (size_t frame = 0; frame < outputFrames; ++frame) {
        const double sourcePosition = static_cast<double>(frame) * sourceRate / kOutputRate;
        const float mono = InterpolatedSample(&source, ReadPcm16, frameCount, 0,
                                             sourcePosition, sourceStep);
        a_stereo[frame * 2] = mono;
        a_stereo[frame * 2 + 1] = mono;
    }
    return true;
}

bool DecodeMp3File(const std::string& a_filePath, std::vector<float>& a_stereo,
                   int& a_sourceChannels) {
    mp3dec_t decoder{};
    mp3dec_init(&decoder);
    mp3dec_file_info_t info{};
    if (mp3dec_load(&decoder, a_filePath.c_str(), &info, nullptr, nullptr) != 0 ||
        !info.buffer || info.channels < 1 || info.channels > 2 || info.hz <= 0 ||
        info.samples < static_cast<size_t>(info.channels)) {
        std::free(info.buffer);
        return false;
    }

    const size_t sourceFrames = info.samples / static_cast<size_t>(info.channels);
    if (!sourceFrames || sourceFrames >
        std::numeric_limits<size_t>::max() / 2) {
        std::free(info.buffer);
        return false;
    }
    const size_t outputFrames = info.hz == kOutputRate ? sourceFrames :
        std::max<size_t>(1, static_cast<size_t>(
            (static_cast<uint64_t>(sourceFrames) * kOutputRate + info.hz / 2) / info.hz));
    if (outputFrames > std::numeric_limits<size_t>::max() / 2) {
        std::free(info.buffer);
        return false;
    }

    a_sourceChannels = info.channels;
    a_stereo.resize(outputFrames * 2);
    const Pcm16Source source{
        static_cast<const int16_t*>(info.buffer), static_cast<size_t>(info.channels)};
    const double sourceStep = static_cast<double>(info.hz) / kOutputRate;
    for (size_t frame = 0; frame < outputFrames; ++frame) {
        const double sourcePosition = static_cast<double>(frame) * info.hz / kOutputRate;
        for (size_t channel = 0; channel < 2; ++channel) {
            a_stereo[frame * 2 + channel] = InterpolatedSample(
                &source, ReadPcm16, sourceFrames, channel, sourcePosition,
                sourceStep);
        }
    }
    std::free(info.buffer);
    return true;
}

int RetailSoundResourceKind(const std::string& a_path) {
    int kind = a_path.find(".vag") != std::string::npos ? 1 : 0;
    if (a_path.find(".fsb") != std::string::npos) kind = 2;
    return kind;
}

std::shared_ptr<DecodedSound> DecodeSound(const std::string& a_path) {
    auto cached = g_decodedSounds.find(a_path);
    if (cached != g_decodedSounds.end()) {
        if (auto sample = cached->second.lock()) return sample;
    }
    FileHelper* files = GetEngineContext().fileHelper;
    if (!files) return {};
    std::optional<std::string> resolved = files->Resolve(a_path);
    if (!resolved) return {};
    auto sample = std::make_shared<DecodedSound>();
    const bool isVag = a_path.size() >= 4 &&
        a_path.compare(a_path.size() - 4, 4, ".vag") == 0 &&
        resolved->size() >= 4 && resolved->compare(resolved->size() - 4, 4, ".vag") == 0;
    const bool isMp3 = a_path.size() >= 4 &&
        a_path.compare(a_path.size() - 4, 4, ".mp3") == 0 &&
        resolved->size() >= 4 && resolved->compare(resolved->size() - 4, 4, ".mp3") == 0;
    const bool isFsb = a_path.size() >= 4 &&
        a_path.compare(a_path.size() - 4, 4, ".fsb") == 0;
    if (isVag) {
        if (!DecodeVagFile(*resolved, sample->stereo, sample->sourceChannels))
            return {};
    } else if (isMp3) {
        if (!DecodeMp3File(*resolved, sample->stereo, sample->sourceChannels))
            return {};
    } else if (isFsb) {
        // Retail has a distinct FSB loader path; do not pass an unsupported
        // bank to the WAV decoder and accidentally treat it as a default sample.
        return {};
    } else if (!g_audioFrontend || !g_audioFrontend->DecodeWave(
                   *resolved, kOutputRate, sample->stereo,
                   sample->sourceChannels)) {
        return {};
    }
    g_decodedSounds[a_path] = sample;
    return sample;
}

void QueueMixedVoices(float a_deltaMs) {
    if ((!g_audioOutputStarted || !g_audioFrontend) && !g_audioCaptureFile) return;
    const double wanted = a_deltaMs * kOutputRate / 1000.0 + g_fractionalOutputFrames;
    int frameCount = (int)std::min(8192.0, std::floor(wanted));
    g_fractionalOutputFrames = wanted - std::floor(wanted);
    if (frameCount <= 0) return;
    std::vector<float> mix((size_t)frameCount * 2, 0.0f);
    const float voiceGainScale = RetailF32(
        State().masterVolume * State().sfxVolume);
    for (auto& entry : State().voices) {
        VoiceState& voice = entry.second;
        if (voice.paused ||
            (voice.speed == 0.0f && voice.speedRampRate == 0.0f) ||
            !std::isfinite(voice.speed)) continue;
        if (!voice.sampleLoadAttempted) {
            voice.sample = DecodeSound(voice.soundFile);
            voice.sampleLoadAttempted = true;
        }
        const std::shared_ptr<DecodedSound>& playbackSample =
            voice.timeStretchedSample ? voice.timeStretchedSample : voice.sample;
        if (!playbackSample || playbackSample->stereo.size() < 2) continue;
        const size_t soundFrames = playbackSample->stereo.size() / 2;
        const float pan = std::clamp(voice.pan, -1.0f, 1.0f);
        // Captures show different backend scales for mono and stereo samples.
        const float channelGain = playbackSample->sourceChannels == 1
            ? (2.0f / 3.0f) : 0.70710678f;
        const float gain = std::max(0.0f, voice.volume * voiceGainScale) *
                           channelGain * voice.loopGainScale;
        // Retail captures show a small level change as playback rate moves
        // away from unity. Compensate for it in the software mixer.
        const float rateGain = voice.speed > 0.0f
            ? static_cast<float>(std::pow(voice.speed, -0.047)) : 1.0f;
        const float panAngle = (pan + 1.0f) * 0.7853981633974483f;
        const bool monoSource = playbackSample->sourceChannels == 1;
        const float leftPan = monoSource ? std::cos(panAngle)
            : (pan > 0.0f ? 1.0f - pan : 1.0f);
        const float rightPan = monoSource ? std::sin(panAngle)
            : (pan < 0.0f ? 1.0f + pan : 1.0f);
        for (int frame = 0; frame < frameCount; ++frame) {
            const double frameMs = (double)frame * 1000.0 / kOutputRate;
            const bool fadeActive = voice.stopping || voice.gateFading;
            if (fadeActive && voice.fadeDurationMs > 0.0 &&
                voice.fadeRemainingMs <= frameMs) continue;
            if (voice.stopping && voice.stopRemainingMs > 0.0 &&
                voice.stopRemainingMs <= frameMs) continue;
            // Retail updates backend gain once per audio-manager tick. Use
            // that tick's resulting gain for the whole queued block instead
            // of applying a smooth per-sample ramp. Gate expiry is handled
            // after this block by AdvanceAudioVoices, then fades next tick.
            const float fade = fadeActive && voice.fadeDurationMs > 0.0
                ? static_cast<float>(std::clamp(
                    static_cast<double>(voice.fadeGain) -
                        a_deltaMs / voice.fadeDurationMs,
                    0.0, 1.0))
                : fadeActive ? voice.fadeGain : 1.0f;
            const double positionMs = voice.positionMs +
                PlaybackAdvanceMs(voice, frameMs);
            double sourceFrame = positionMs * kOutputRate / 1000.0;
            if (voice.looping || voice.loopCount > 0) {
                sourceFrame = std::fmod(sourceFrame, (double)soundFrames);
                if (sourceFrame < 0.0) sourceFrame += soundFrames;
            }
            if (sourceFrame < 0.0 || sourceFrame >= soundFrames) continue;
            const double sourceStep = std::fabs(voice.speed +
                static_cast<double>(voice.speedRampRate) * frameMs);
            const float leftSample = InterpolatedSample(
                playbackSample->stereo.data(), ReadStereoFloat,
                soundFrames, 0, sourceFrame, sourceStep,
                voice.looping || voice.loopCount > 0, true);
            const float rightSample = InterpolatedSample(
                playbackSample->stereo.data(), ReadStereoFloat,
                soundFrames, 1, sourceFrame, sourceStep,
                voice.looping || voice.loopCount > 0, true);
            mix[(size_t)frame * 2] += leftSample * gain * rateGain * fade * leftPan;
            mix[(size_t)frame * 2 + 1] += rightSample * gain * rateGain * fade * rightPan;
        }
    }
    for (float& sample : mix) sample = std::clamp(sample, -1.0f, 1.0f);
    if (g_audioCaptureFile)
        std::fwrite(mix.data(), sizeof(float), mix.size(), g_audioCaptureFile);
    if (g_audioOutputStarted && g_audioFrontend)
        g_audioFrontend->QueueAudioSamples(mix.data(), mix.size());
}

int CreateVoiceState(const std::string& a_path, float a_pan, float a_volume,
                     int a_note = 96, bool a_applyFrequencyMultiplier = false,
                     int a_referenceNote = 48) {
    auto sample = DecodeSound(a_path);
    if (!sample || sample->stereo.empty()) return 0;

    const int id = State().nextVoiceId++;
    VoiceState voice;
    voice.soundFile = a_path;
    voice.sample = std::move(sample);
    voice.sampleLoadAttempted = true;
    voice.volume = a_volume;
    voice.pan = a_pan;
    voice.referenceNote = a_referenceNote;
    const float noteRatio = a_note == 96 ? 1.0f :
        GroundRenderMath::RetailAudioNotePitchRatio(a_note, voice.referenceNote);
    voice.speed = voice.sourceBaseSpeed * noteRatio;
    if (a_applyFrequencyMultiplier)
        voice.speed *= State().frequencyMultiplier;
    voice.targetSpeed = voice.speed;
    State().voices[id] = std::move(voice);
    return id;
}

// --- loading ---------------------------------------------------------

int GM_CDECL gmf_loadSound(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 2) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    GM_STRING_PARAM(path, 0, "");
    int mode = 0;
    if (count == 2) {
        const gmVariable& modeArg = a_thread->Param(1);
        mode = modeArg.IsFloat()
            ? static_cast<int>(modeArg.GetFloat())
            : modeArg.m_value.m_int;
    }
    if (!State().enabled) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    auto sample = DecodeSound(path);
    const bool loaded = sample && !sample->stereo.empty();
    if (loaded) {
        State().loadedSounds.insert(path);
        g_loadedSoundSamples[path] = std::move(sample);
        g_loadedSoundModes.try_emplace(path, mode);
        g_loadedSoundResourceKinds.try_emplace(path, RetailSoundResourceKind(path));
    }
    a_thread->PushInt(loaded ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_unloadSound(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    State().loadedSounds.erase(path);
    State().soundLengthMs.erase(path);
    g_loadedSoundSamples.erase(path);
    g_loadedSoundModes.erase(path);
    g_loadedSoundResourceKinds.erase(path);
    // The weak cache owns no decoded sample by itself. Erasing its entry
    // releases the cached decode once active VoiceState references are gone.
    g_decodedSounds.erase(path);
    return GM_OK;
}
int GM_CDECL gmf_unloadAllSounds(gmThread* a_thread) { return GM_OK; }
int GM_CDECL gmf_loadSampleList(gmThread* a_thread) {
    FileHelper* files = GetEngineContext().fileHelper;
    std::string contents;
    if (!files || !files->ReadTextFile("sampleList.txt", contents)) {
        a_thread->PushInt(0);
        return GM_OK;
    }

    int loadedCount = 0;
    std::istringstream entries(contents);
    std::string path;
    while (entries >> path) {
        if (!State().enabled) continue;
        auto sample = DecodeSound(path);
        if (!sample || sample->stereo.empty()) continue;
        State().loadedSounds.insert(path);
        g_loadedSoundSamples[path] = std::move(sample);
        g_loadedSoundModes.try_emplace(path, 0);
        g_loadedSoundResourceKinds.try_emplace(path, RetailSoundResourceKind(path));
        ++loadedCount;
    }
    a_thread->PushInt(loadedCount);
    return GM_OK;
}

// --- playback ----------------------------------------------------------

int GM_CDECL gmf_playSound(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 3 && (count < 5 || count > 8)) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    GM_STRING_PARAM(path, 0, "");
    // The retail 1-argument route supplies gate=0, pan=0, volume=1 from
    // constants at 0x1a870/0x1a874 and note=0x60. The 3-argument route uses
    // gate=0 from 0x1a950, then forwards its two numeric values as pan/volume;
    // both defaults are therefore represented before the arity overrides.
    float pan = 0.0f, vol = 1.0f;
    float gate = 0.0f;
    int note = 96;
    int priority = 0x80;
    int positionMs = 0;
    if (count == 3) {
        pan = a_thread->ParamFloatOrInt(1, 0.0f);
        vol = a_thread->ParamFloatOrInt(2, 1.0f);
    } else if (count >= 5) {
        note = (int)a_thread->ParamFloatOrInt(1, 0.0f);
        gate = a_thread->ParamFloatOrInt(2, 0.0f);
        pan = a_thread->ParamFloatOrInt(3, 0.0f);
        vol = a_thread->ParamFloatOrInt(4, 1.0f);
        if (count >= 6) {
            // The retail wrapper sign-extends this payload without a
            // GM_FLOAT conversion before forwarding backend priority. The
            // backend rejects values above 0x100 without changing its
            // creation default, which is 0x80 on the ordinary play route.
            const int requestedPriority = a_thread->Param(5).m_value.m_int;
            if (requestedPriority >= 0 && requestedPriority <= 0x100)
                priority = requestedPriority;
        }
        if (count == 8) {
            // PPC fctiwz conversion in the retail wrapper truncates a float
            // start position toward zero; other payloads are raw GM_INT.
            const gmVariable& positionArg = a_thread->Param(7);
            positionMs = positionArg.IsFloat()
                ? RetailFctiwzToInt(positionArg.GetFloat())
                : positionArg.m_value.m_int;
        }
    }

    if (State().recordingScript) {
        State().recordedSoundResources.insert(path);
        const bool chainCallback = count >= 7 &&
            IsLocalEffectChain(*a_thread->GetMachine(), a_thread->Param(6));
        if (count < 7 || chainCallback) {
            RecordedPlaySoundEvent event;
            event.timeMs = RecordedAudioElapsedMs();
            event.path = path;
            event.note = note;
            event.gate = gate;
            event.pan = pan;
            event.volume = vol;
            event.priority = count >= 6
                ? a_thread->Param(5).m_value.m_int : 0x80;
            event.chainRoute = count >= 7;
            event.startPositionMs = positionMs;
            if (event.chainRoute) {
                event.effectChain = a_thread->Param(6);
                gmTableObject* chain = static_cast<gmTableObject*>(
                    a_thread->GetMachine()->GetObject(
                        event.effectChain.m_value.m_ref));
                const gmVariable handle = chain
                    ? chain->Get(a_thread->GetMachine(), "__nativeHandle")
                    : gmVariable::s_null;
                if (handle.m_type == GM_INT)
                    event.effectChainHandle =
                        static_cast<uint32_t>(handle.m_value.m_int);
                RetainAudioGMVariable(*a_thread->GetMachine(), event.effectChain);
            }
            State().recordedPlaySounds.push_back(std::move(event));
        }
    }

    if (!State().enabled) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const int id = CreateVoiceState(path, pan, vol, note, true);
    if (id != 0) {
        VoiceState& voice = State().voices[id];
        voice.gateDurationMs = gate;
        voice.priority = priority;
        voice.positionMs = positionMs;
        gmMachine* machine = a_thread->GetMachine();
        gmVariable chain = GetOrCreateMasterEffectChain(*machine);
        if (count >= 7 && IsLocalEffectChain(*machine, a_thread->Param(6))) {
            const gmVariable& requested = a_thread->Param(6);
            gmTableObject* requestedTable = (gmTableObject*)machine->GetObject(
                requested.m_value.m_ref);
            if (requestedTable &&
                requestedTable->Get(machine, "__freed").m_type == GM_NULL)
                chain = requested;
        }
        AttachEffectChainToVoice(*machine, id, voice, chain);
    }
    a_thread->PushInt(id);
    return GM_OK;
}

void DispatchRecordedPlaySounds() {
    AudioState& state = State();
    if (!state.playingScript) return;
    constexpr float kRetailSchedulerStepMs = 16.6660003662109375f;
    state.playbackElapsedMs += kRetailSchedulerStepMs;
    while (state.playbackEventIndex < state.recordedPlaySounds.size() &&
           state.recordedPlaySounds[state.playbackEventIndex].timeMs <=
               state.playbackElapsedMs) {
        const RecordedPlaySoundEvent& event =
            state.recordedPlaySounds[state.playbackEventIndex++];
        const int id = CreateVoiceState(event.path, event.pan, event.volume,
                                        event.note, true);
        if (id == 0) continue;
        state.scriptPlaybackVoiceIds.insert(id);

        VoiceState& voice = state.voices[id];
        voice.gateDurationMs = event.gate;
        voice.priority = event.priority >= 0 && event.priority <= 0x100
            ? event.priority : 0x80;
        voice.positionMs = event.startPositionMs;
        if (state.machine) {
            gmVariable chain = GetOrCreateMasterEffectChain(*state.machine);
            if (event.chainRoute) {
                gmVariable requestedChain = IsLocalEffectChain(
                    *state.machine, event.effectChain)
                    ? event.effectChain
                    : FindEffectChainByHandle(*state.machine,
                                              event.effectChainHandle);
                gmTableObject* requested = requestedChain.m_type == GM_TABLE
                    ? static_cast<gmTableObject*>(state.machine->GetObject(
                          requestedChain.m_value.m_ref))
                    : nullptr;
                if (requested &&
                    requested->Get(state.machine, "__freed").m_type == GM_NULL)
                    chain = requestedChain;
            }
            AttachEffectChainToVoice(*state.machine, id, voice, chain);
        }
    }
    if (state.recordingStopMs <= state.playbackElapsedMs) {
        state.playingScript = false;
        StopRecordedPlaybackVoices();
    }
}

int GM_CDECL gmf_spawnSoundSlicker(gmThread* a_thread) {
    if (a_thread->GetNumParams() != 5) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    GM_STRING_PARAM(path, 0, "");
    const float gateMs = a_thread->ParamFloatOrInt(1, 0.0f);
    const float noteOffset = a_thread->ParamFloatOrInt(2, 0.0f);
    const float timeFactor = a_thread->ParamFloatOrInt(3, 1.0f);
    const gmVariable& positionArg = a_thread->Param(4);
    const int startPositionMs = positionArg.IsFloat()
        ? RetailFctiwzToInt(positionArg.GetFloat())
        : positionArg.m_value.m_int;

    if (!State().enabled || !std::isfinite(timeFactor)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    // Slicker voices are created muted and made audible on the next sequencer
    // step through playSoundSlicker. Preserve the wrapper's float note-offset
    // conversion here; unlike Audio.setVoiceNote, this argument reaches the
    // Slicker helper as a float/double rather than an integer note.
    const float pitchExponent = noteOffset *
        GroundRenderMath::FloatFromBits(0x3daaaaabu);
    const float pitchRatio = GroundRenderMath::RetailGroundPow(2.0f, pitchExponent);
    const int id = CreateVoiceState(path, 0.0f, 0.0f, 96, false);
    if (id != 0) {
        VoiceState& voice = State().voices[id];
        voice.ownsDiracPoolEntry = true;
        ++State().diracPoolEntriesUsed;
        voice.timeStretchFactor = timeFactor;
        voice.timeStretchedSample = BuildTimeStretchedSample(
            path, voice.sample, timeFactor);
        if (!voice.timeStretchedSample) voice.timeStretchFactor = 1.0f;
        voice.speed *= pitchRatio;
        voice.targetSpeed = voice.speed;
        voice.gateDurationMs = gateMs;
        const int sampleLengthMs = SoundLengthMs(path);
        uint32_t wrappedStartMs = static_cast<uint32_t>(startPositionMs);
        if (sampleLengthMs > 0) {
            wrappedStartMs %= static_cast<uint32_t>(sampleLengthMs);
        }
        // The retail cursor is wrapped in source milliseconds first. WSOLA's
        // output timeline is then stretched by the same tempo factor.
        voice.positionMs = static_cast<double>(wrappedStartMs) *
                           voice.timeStretchFactor;
    }
    a_thread->PushInt(id);
    return GM_OK;
}

int GM_CDECL gmf_playSoundSlicker(gmThread* a_thread) {
    if (a_thread->GetNumParams() != 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    const int voiceId = a_thread->Param(0).m_value.m_int;
    const float gateMs = a_thread->ParamFloatOrInt(1, 0.0f);
    const float pan = a_thread->ParamFloatOrInt(2, 0.0f);
    const float volume = a_thread->ParamFloatOrInt(3, 1.0f);
    if (!State().enabled) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    VoiceState* voice = FindVoice(voiceId);
    if (!voice) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    voice->gateDurationMs = gateMs;
    voice->pan = pan;
    if (voice->gateDurationMs <= 0.0 ||
        !(voice->gateElapsedMs >= voice->gateDurationMs))
        voice->volume = volume;
    voice->paused = false;
    a_thread->PushInt(voiceId);
    return GM_OK;
}

int GM_CDECL gmf_playStream(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 3 && count != 6) {
        // The retail wrapper returns integer -1 for every other arity.
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string path;
    float gate = 0.0f;
    float pan = 0.0f;
    float volume = 1.0f;
    int note = 96;
    int referenceNote = 48;

    if (count == 1) {
        GM_STRING_PARAM(resource, 0, "");
        path = resource;
    } else if (count == 3) {
        pan = a_thread->ParamFloatOrInt(0, 0.0f);
        volume = a_thread->ParamFloatOrInt(1, 1.0f);
        GM_STRING_PARAM(resource, 2, "");
        path = resource;
    } else {
        // The six-argument wrapper truncates float note values with PPC
        // fctiwz and passes non-floats through as raw integer payloads.
        const gmVariable& referenceNoteArg = a_thread->Param(0);
        referenceNote = referenceNoteArg.IsFloat()
            ? RetailFctiwzToInt(referenceNoteArg.GetFloat())
            : referenceNoteArg.m_value.m_int;
        const gmVariable& noteArg = a_thread->Param(1);
        note = noteArg.IsFloat()
            ? RetailFctiwzToInt(noteArg.GetFloat())
            : noteArg.m_value.m_int;
        gate = a_thread->ParamFloatOrInt(2, 0.0f);
        pan = a_thread->ParamFloatOrInt(3, 0.0f);
        volume = a_thread->ParamFloatOrInt(4, 1.0f);
        GM_STRING_PARAM(resource, 5, "");
        path = resource;
    }

    if (!State().enabled) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const int id = CreateVoiceState(path, pan, volume, note, false,
                                    referenceNote);
    if (id != 0) State().voices[id].gateDurationMs = gate;
    a_thread->PushInt(id);
    return GM_OK;
}
// Audio.loadSoundSlicker() forwards the same gmThread to the retail
// Audio.loadSound wrapper at 0x84984 (PPC 0x84db4 is a direct `bl`, with r3
// preserved). Preserve the caller's existing arguments and return behavior;
// path/resource work remains the same as the ordinary loadSound binding.
int GM_CDECL gmf_loadSoundSlicker(gmThread* a_thread) {
    return gmf_loadSound(a_thread);
}

// --- voice control -------------------------------------------------------

void StartVoiceFade(VoiceState& a_voice, double a_durationMs) {
    const float retailDurationMs = static_cast<float>(a_durationMs);
    a_voice.stopping = true;
    a_voice.fadeDurationMs = static_cast<double>(retailDurationMs);
    a_voice.fadeRemainingMs = a_voice.fadeDurationMs;
    a_voice.fadeGain = 1.0f;
    a_voice.stopRemainingMs = 0.0;
}

int GM_CDECL gmf_stopVoice(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 2) return GM_EXCEPTION;
    const gmVariable& voiceArg = a_thread->Param(0);
    const int voice = voiceArg.IsFloat()
        ? static_cast<int>(voiceArg.GetFloat())
        : (voiceArg.IsInt() ? static_cast<int>(voiceArg.GetInt()) : 0);
    double timeMs = 0.0;
    if (count == 2) {
        const gmVariable& durationArg = a_thread->Param(1);
        if (durationArg.IsFloat())
            timeMs = static_cast<double>(durationArg.GetFloat());
        else if (durationArg.IsInt())
            timeMs = static_cast<double>(durationArg.GetInt());
    }
    if (VoiceState* v = FindVoice(voice)) {
        if (timeMs > 0.0) {
            v->stopping = true;
            v->stopRemainingMs = timeMs;
            v->fadeRemainingMs = v->fadeDurationMs = 0.0;
            v->fadeGain = 1.0f;
        } else {
            StartVoiceFade(*v, 60.0);
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_stopAllVoices(gmThread* a_thread) {
    (void)a_thread;
    StopAllVoicesForProcessListReset();
    return GM_OK;
}
int GM_CDECL gmf_fadeOutVoice(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const int voice = a_thread->Param(0).m_value.m_int;
    const gmVariable& durationArg = a_thread->Param(1);
    const double fadeMs = durationArg.IsFloat()
        ? static_cast<double>(durationArg.GetFloat())
        : static_cast<double>(durationArg.m_value.m_int);
    if (VoiceState* v = FindVoice(voice))
        StartVoiceFade(*v, fadeMs);
    return GM_OK;
}
int GM_CDECL gmf_isVoiceValid(gmThread* a_thread) {
    // Retail forwards the supplied GM payload raw to the manager's voice map.
    const int voice = a_thread->Param(0).m_value.m_int;
    a_thread->PushInt(FindVoice(voice) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_isVoicePaused(gmThread* a_thread) {
    // Retail forwards the argument payload raw. Its helper returns false when
    // either voice lookup or the backend pause query fails.
    const int voice = a_thread->Param(0).m_value.m_int;
    VoiceState* v = FindVoice(voice);
    a_thread->PushInt((v && CanQueryVoiceBackend(*v) && v->paused) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setVoicePause(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    // The retail wrapper passes the voice ID payload and tests the pause
    // payload for nonzero without checking either GM type tag.
    const int voice = a_thread->Param(0).m_value.m_int;
    const bool pause = a_thread->Param(1).m_value.m_int != 0;
    if (VoiceState* v = FindVoice(voice)) v->paused = pause;
    return GM_OK;
}
int GM_CDECL gmf_setVoiceVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_FLOAT_OR_INT_PARAM(vol, 1, 1.0f);
    if (VoiceState* v = FindVoice(voice)) {
        if (v->gateDurationMs <= 0.0 ||
            !(v->gateElapsedMs >= v->gateDurationMs)) {
            v->volume = vol;
        }
    }
    return GM_OK;
}
int GM_CDECL gmf_getVoiceVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable& voiceArg = a_thread->Param(0);
    // Retail wrapper converts GM_FLOAT voice IDs by truncating toward zero;
    // other tags pass their raw payload as the ID.
    const int voiceId = voiceArg.IsFloat()
        ? RetailFctiwzToInt(voiceArg.GetFloat())
        : voiceArg.m_value.m_int;
    VoiceState* v = FindVoice(voiceId);
    if (!v || !CanQueryVoiceBackend(*v)) {
        a_thread->PushFloat(0.0f);
        return GM_OK;
    }
    const float voiceGainScale = RetailF32(
        State().masterVolume * State().sfxVolume);
    const float scaledVoiceGain = RetailF32(voiceGainScale * v->volume);
    const float backendBaseGain = RetailF32(scaledVoiceGain * v->fadeGain);
    const float normalizedGain = RetailF32(backendBaseGain / voiceGainScale);
    const float nonpositiveScale = 0.0f - voiceGainScale;
    const float nonnegativeScale = voiceGainScale - 0.0f;
    const float firstSelect = RetailFsel(nonpositiveScale, 0.0f, normalizedGain);
    a_thread->PushFloat(RetailFsel(nonnegativeScale, firstSelect, normalizedGain));
    return GM_OK;
}
int GM_CDECL gmf_setVoicePan(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const int voice = a_thread->Param(0).m_value.m_int;
    const gmVariable& panArg = a_thread->Param(1);
    const double panValue = panArg.IsFloat()
        ? static_cast<double>(panArg.GetFloat())
        : static_cast<double>(panArg.m_value.m_int);
    const float pan = static_cast<float>(panValue);
    if (VoiceState* v = FindVoice(voice)) v->pan = pan;
    return GM_OK;
}
int GM_CDECL gmf_setVoicePitchBend(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    // The wrapper passes the voice and target-note payloads through without
    // checking or converting their GM type tags.
    const int voice = a_thread->Param(0).m_value.m_int;
    const int targetNote = a_thread->Param(1).m_value.m_int;
    const float durationMs = a_thread->ParamFloatOrInt(2, 0.0f);
    if (VoiceState* v = FindVoice(voice)) {
        const float targetSpeed = v->sourceBaseSpeed *
            GroundRenderMath::RetailAudioNotePitchRatio(targetNote, v->referenceNote);
        // Retail queues pitch-bend work on the audio manager. A speed query in
        // the same script turn still sees the backend's old frequency.
        v->pendingPitchSpeed = targetSpeed;
        v->pendingPitchDurationMs = durationMs;
        v->pitchBendPending = true;
    }
    return GM_OK;
}
int GM_CDECL gmf_setVoiceGateTime(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const int voice = a_thread->Param(0).m_value.m_int;
    const gmVariable& gateArg = a_thread->Param(1);
    const double gateValue = gateArg.IsFloat()
        ? static_cast<double>(gateArg.GetFloat())
        : static_cast<double>(gateArg.m_value.m_int);
    const float gateMs = static_cast<float>(gateValue);
    if (VoiceState* v = FindVoice(voice)) {
        v->gateDurationMs = gateMs;
    }
    return GM_OK;
}
int GM_CDECL gmf_setVoiceNote(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const int voice = a_thread->Param(0).m_value.m_int;
    const gmVariable& noteArg = a_thread->Param(1);
    const int note = noteArg.IsFloat()
        ? static_cast<int>(noteArg.GetFloat())
        : static_cast<int>(noteArg.m_value.m_int);
    if (VoiceState* v = FindVoice(voice)) {
        const float speed = v->sourceBaseSpeed * (note == 96 ? 1.0f :
            GroundRenderMath::RetailAudioNotePitchRatio(note, v->referenceNote));
        SetVoiceSpeedTarget(*v, speed, 0.0f);
    }
    return GM_OK;
}
int GM_CDECL gmf_setVoiceSpeed(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 2) return GM_EXCEPTION;
    const int voice = a_thread->Param(0).m_value.m_int;
    const gmVariable& speedArg = a_thread->Param(1);
    const float speed = speedArg.IsFloat()
        ? speedArg.GetFloat()
        : static_cast<float>(speedArg.m_value.m_int);
    float durationMs = 0.0f;
    if (count > 2) {
        const gmVariable& durationArg = a_thread->Param(2);
        if (durationArg.IsFloat()) durationMs = durationArg.GetFloat();
    }
    if (VoiceState* v = FindVoice(voice))
        SetVoiceSpeedTarget(*v, v->sourceBaseSpeed * speed, durationMs);
    return GM_OK;
}
int GM_CDECL gmf_getVoiceSpeed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    const int voice = a_thread->Param(0).m_value.m_int;
    VoiceState* v = FindVoice(voice);
    a_thread->PushFloat(v && v->sourceBaseSpeed != 0.0f
        ? v->speed / v->sourceBaseSpeed : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_setVoicePosition(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    // Retail wrapper 0x866d8 forwards the voice ID's raw payload. The
    // position is a float truncated toward zero when GM_FLOAT; all other
    // tags pass their raw 32-bit payload as an int.
    const int voice = a_thread->Param(0).m_value.m_int;
    const gmVariable& positionArg = a_thread->Param(1);
    const int ms = positionArg.IsFloat()
        ? static_cast<int>(positionArg.GetFloat())
        : positionArg.m_value.m_int;
    if (VoiceState* v = FindVoice(voice)) v->positionMs = ms;
    return GM_OK;
}
int GM_CDECL gmf_getVoicePosition(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    // Retail wrapper 0x86764 passes the voice-ID payload without a type test.
    const int voice = a_thread->Param(0).m_value.m_int;
    VoiceState* v = FindVoice(voice);
    double position = v ? v->positionMs : 0.0;
    if (position > std::numeric_limits<int>::max()) position = std::numeric_limits<int>::max();
    if (position < std::numeric_limits<int>::min()) position = std::numeric_limits<int>::min();
    a_thread->PushInt((int)position);
    return GM_OK;
}
int GM_CDECL gmf_getVoiceLength(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    // Retail wrapper forwards the voice-ID payload without checking its type.
    const int voice = a_thread->Param(0).m_value.m_int;
    VoiceState* v = FindVoice(voice);
    a_thread->PushInt(v ? SoundLengthMs(v->soundFile) : 0);
    return GM_OK;
}
int GM_CDECL gmf_getSoundLength(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    a_thread->PushInt(SoundLengthMs(path));
    return GM_OK;
}
// Retail Audio manager state controls. The bytecode wrapper and manager
// constructor initialize these fields directly; exact consumer behavior for
// the frequency multiplier is still under audit.
int GM_CDECL gmf_getFrequencyMultiplier(gmThread* a_thread) {
    a_thread->PushFloat(State().frequencyMultiplier);
    return GM_OK;
}
int GM_CDECL gmf_setFrequencyMultiplier(gmThread* a_thread) {
    if (a_thread->GetNumParams() > 0) {
        const gmVariable& value = a_thread->Param(0);
        State().frequencyMultiplier = value.m_type == GM_FLOAT
            ? value.m_value.m_float
            : static_cast<float>(value.m_value.m_int);
    }
    return GM_OK;
}
int GM_CDECL gmf_hackSetEnabled(gmThread* a_thread) {
    // RETAIL PPC: reads parameter 0's raw payload without validating arity or
    // type, then stores (payload != 0) at AudioManager +0x2c3c.
    State().enabled = a_thread->Param(0).m_value.m_int != 0;
    return GM_OK;
}
int GM_CDECL gmf_setVoiceLoop(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const int voice = a_thread->Param(0).m_value.m_int;
    const bool loop = a_thread->Param(1).m_value.m_int != 0;
    if (VoiceState* v = FindVoice(voice)) {
        v->looping = loop;
        v->loopGainScale = loop || v->loopCount > 0 ? 1.05f : 1.0f;
    }
    return GM_OK;
}
int GM_CDECL gmf_setVoiceLoopCount(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    const int voice = a_thread->Param(0).m_value.m_int;
    const int count = a_thread->Param(1).m_value.m_int;
    if (VoiceState* v = FindVoice(voice)) {
        v->loopCount = std::max(0, count);
        // Repeated passes measure about 5% below retail FMOD in captures.
        v->loopGainScale = v->looping || v->loopCount > 0 ? 1.05f : 1.0f;
    }
    return GM_OK;
}
int GM_CDECL gmf_getAllVoices(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* list = m.AllocTableObject();
    int i = 0;
    for (auto& kv : State().voices) list->Set(&m, i++, gmVariable((gmint)kv.first));
    gmTableObject* it = m.AllocTableObject();
    it->Set(&m, "__list", gmVariable(list));
    it->Set(&m, "__idx", gmVariable((gmint)0));
    it->Set(&m, "isValid", gmVariable(m.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            gmMachine& mm = *a_thread->GetMachine();
            gmVariable listVar = self->Get(&mm, "__list");
            gmVariable idxVar = self->Get(&mm, "__idx");
            int idx = (idxVar.m_type == GM_INT) ? idxVar.m_value.m_int : 0;
            int count = (listVar.m_type == GM_TABLE) ? ((gmTableObject*)mm.GetObject(listVar.m_value.m_ref))->Count() : 0;
            a_thread->PushInt(idx < count ? 1 : 0);
            return GM_OK;
        })));
    it->Set(&m, "get", gmVariable(m.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            gmMachine& mm = *a_thread->GetMachine();
            gmVariable listVar = self->Get(&mm, "__list");
            gmVariable idxVar = self->Get(&mm, "__idx");
            int idx = (idxVar.m_type == GM_INT) ? idxVar.m_value.m_int : 0;
            if (listVar.m_type == GM_TABLE) {
                a_thread->Push(((gmTableObject*)mm.GetObject(listVar.m_value.m_ref))->Get((gmint)idx));
            } else {
                a_thread->PushNull();
            }
            return GM_OK;
        })));
    it->Set(&m, "goToNext", gmVariable(m.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            gmMachine& mm = *a_thread->GetMachine();
            gmVariable idxVar = self->Get(&mm, "__idx");
            int idx = (idxVar.m_type == GM_INT) ? idxVar.m_value.m_int : 0;
            self->Set(&mm, "__idx", gmVariable((gmint)(idx + 1)));
            return GM_OK;
        })));
    a_thread->PushTable(it);
    return GM_OK;
}

// --- global volume -------------------------------------------------------

int GM_CDECL gmf_setMasterVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(vol, 0, 1.0f);
    State().masterVolume = RetailClampAudioVolume(vol);
    return GM_OK;
}
int GM_CDECL gmf_getMasterVolume(gmThread* a_thread) { a_thread->PushFloat(State().masterVolume); return GM_OK; }
int GM_CDECL gmf_setMusicVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(vol, 0, 1.0f);
    State().musicVolume = RetailClampAudioVolume(vol);
    return GM_OK;
}
int GM_CDECL gmf_getMusicVolume(gmThread* a_thread) {
    a_thread->PushFloat(State().musicVolume * State().masterVolume);
    return GM_OK;
}
int GM_CDECL gmf_setSFXVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(vol, 0, 1.0f);
    State().sfxVolume = RetailClampAudioVolume(vol);
    return GM_OK;
}
int GM_CDECL gmf_getSFXVolume(gmThread* a_thread) {
    a_thread->PushFloat(State().sfxVolume * State().masterVolume);
    return GM_OK;
}

bool RetailEffectTypeHasParameters(int a_type) {
    return (a_type >= 3 && a_type <= 12) || (a_type >= 15 && a_type <= 20) ||
           a_type == 0x10000;
}
gmVariable RetailEffectParameterValue(const gmVariable& a_value, int a_type, int a_paramId) {
    double value = a_value.m_type == GM_INT
        ? (double)a_value.m_value.m_int : (double)a_value.m_value.m_float;
    bool scaled = false;
    if (a_type == 15 && (a_paramId == 0 || a_paramId == 1)) {
        value = (double)(float)(value * 100.0);
        scaled = true;
    } else if (a_type == 17 && (a_paramId == 4 || a_paramId == 7 || a_paramId == 9)) {
        value = (double)(float)(value * 1000.0);
        scaled = true;
    } else if (a_type == 17 && (a_paramId == 10 || a_paramId == 11)) {
        value = (double)(float)(value * 100.0);
        scaled = true;
    }
    if (a_value.m_type == GM_INT && !scaled)
        return gmVariable((gmint)value);
    return gmVariable((float)value);
}
void StoreRetailEffectParameter(gmMachine* a_machine, gmTableObject* a_effect,
                                int a_paramId, const gmVariable& a_value) {
    gmVariable type = a_effect->Get(a_machine, "__type");
    int effectType = type.m_type == GM_INT ? type.m_value.m_int : 0;
    if (!RetailEffectTypeHasParameters(effectType)) return;
    char key[32];
    snprintf(key, sizeof(key), "__p%d", a_paramId);
    a_effect->Set(a_machine, key, RetailEffectParameterValue(a_value, effectType, a_paramId));
}
void ReleaseEffectHandleTable(gmMachine* a_machine, gmTableObject* a_effect) {
    if (!a_effect || a_effect->Get(a_machine, "__freed").m_type != GM_NULL) return;
    gmVariable owner = a_effect->Get(a_machine, "__owner");
    gmVariable slot = a_effect->Get(a_machine, "__ownerSlot");
    if (owner.m_type == GM_TABLE && slot.m_type == GM_INT) {
        gmTableObject* chain = (gmTableObject*)a_machine->GetObject(owner.m_value.m_ref);
        if (chain) {
            char key[24];
            snprintf(key, sizeof(key), "__effectSlot%d", slot.m_value.m_int);
            chain->Set(a_machine, key, gmVariable::s_null);
        }
    }
    a_effect->Set(a_machine, "__freed", gmVariable((gmint)1));
}
gmVariable MakeEffectTable(gmMachine& a_machine, int a_effectType) {
    gmTableObject* t = a_machine.AllocTableObject();
    t->Set(&a_machine, "__type", gmVariable((gmint)a_effectType));
    t->Set(&a_machine, "__bypass", gmVariable((gmint)0));
    // SOUNDEFFECT.getType @ 0x88e9c reads the native object's +4 field,
    // initialized from effectType by addEffect. Preserve that observable
    // value through the local table object's corresponding field.
    t->Set(&a_machine, "getType", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            a_thread->Push(self->Get(a_thread->GetMachine(), "__type"));
            return GM_OK;
        })));
    t->Set(&a_machine, "setBypass", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(b, 0, 0);
            self->Set(a_thread->GetMachine(), "__bypass", gmVariable((gmint)(b != 0)));
            return GM_OK;
        })));
    // The public method and addEffect's initial pair path share the exact
    // recovered effect-type dispatch; retained state is the converted value
    // the native adapter would pass to the absent DSP backend.
    t->Set(&a_machine, "setParam", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            GM_CHECK_NUM_PARAMS(2);
            const gmVariable& id = a_thread->Param(0);
            const int paramId = id.m_type == GM_INT ? id.m_value.m_int : 0;
            StoreRetailEffectParameter(a_thread->GetMachine(), self, paramId,
                                       a_thread->Param(1));
            return GM_OK;
        })));
    return gmVariable(t);
}
int GM_CDECL gmf_chain_addEffect(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    int n = a_thread->GetNumParams();
    if (n < 1) { a_thread->PushNull(); return GM_OK; }
    int freeSlot = -1;
    char slotKey[24];
    gmMachine* machine = a_thread->GetMachine();
    for (int i = 0; i < 8; ++i) {
        snprintf(slotKey, sizeof(slotKey), "__effectSlot%d", i);
        if (self->Get(machine, slotKey).m_type == GM_NULL) { freeSlot = i; break; }
    }
    if (freeSlot < 0) { a_thread->PushNull(); return GM_OK; }
    const gmVariable& typeValue = a_thread->Param(0);
    const int effectType = typeValue.m_type == GM_INT ? typeValue.m_value.m_int : 0;
    gmVariable effectVar = MakeEffectTable(*machine, effectType);
    gmTableObject* effect = (gmTableObject*)machine->GetObject(effectVar.m_value.m_ref);
    effect->Set(machine, "__owner", gmVariable(self));
    effect->Set(machine, "__ownerSlot", gmVariable((gmint)freeSlot));
    for (int i = 1; i + 1 < n; i += 2) {
        const gmVariable& id = a_thread->Param(i);
        const int paramId = id.m_type == GM_INT ? id.m_value.m_int : 0;
        StoreRetailEffectParameter(machine, effect, paramId, a_thread->Param(i + 1));
    }
    snprintf(slotKey, sizeof(slotKey), "__effectSlot%d", freeSlot);
    self->Set(machine, slotKey, effectVar);
    a_thread->Push(effectVar);
    return GM_OK;
}
void ReleaseEffectSlots(gmMachine* a_machine, gmTableObject* a_chain) {
    char key[24];
    for (int i = 0; i < 8; ++i) {
        snprintf(key, sizeof(key), "__effectSlot%d", i);
        gmVariable value = a_chain->Get(a_machine, key);
        if (value.m_type == GM_TABLE) {
            gmTableObject* effect = (gmTableObject*)a_machine->GetObject(value.m_value.m_ref);
            ReleaseEffectHandleTable(a_machine, effect);
        }
        a_chain->Set(a_machine, key, gmVariable::s_null);
    }
}
void ReleaseEffectChainTable(gmMachine* a_machine, gmTableObject* a_chain) {
    if (!a_chain || a_chain->Get(a_machine, "__freed").m_type != GM_NULL ||
        a_chain->Get(a_machine, "__protected").m_type != GM_NULL) return;
    // Retail clears the chain's active byte before recursively releasing its
    // children; mark first so corrupt/cyclic script links cannot recurse forever.
    a_chain->Set(a_machine, "__freed", gmVariable((gmint)1));
    const gmVariable nativeHandle = a_chain->Get(a_machine, "__nativeHandle");
    char key[24];
    for (int i = 0; i < 16; ++i) {
        snprintf(key, sizeof(key), "__childChainSlot%d", i);
        gmVariable value = a_chain->Get(a_machine, key);
        if (value.m_type == GM_TABLE) {
            gmTableObject* child = (gmTableObject*)a_machine->GetObject(value.m_value.m_ref);
            ReleaseEffectChainTable(a_machine, child);
            if (child) child->Set(a_machine, "__parentChain", gmVariable::s_null);
        }
        a_chain->Set(a_machine, key, gmVariable::s_null);
    }
    gmVariable parent = a_chain->Get(a_machine, "__parentChain");
    if (parent.m_type == GM_TABLE) {
        gmTableObject* parentTable = (gmTableObject*)a_machine->GetObject(parent.m_value.m_ref);
        if (parentTable) {
            for (int i = 0; i < 16; ++i) {
                snprintf(key, sizeof(key), "__childChainSlot%d", i);
                gmVariable slot = parentTable->Get(a_machine, key);
                if (slot.m_type == GM_TABLE && slot.m_value.m_ref == a_chain->GetRef())
                    parentTable->Set(a_machine, key, gmVariable::s_null);
            }
        }
    }
    a_chain->Set(a_machine, "__parentChain", gmVariable::s_null);
    ReleaseEffectSlots(a_machine, a_chain);

    std::vector<int> attachedVoiceIds;
    if (const auto members = g_effectChainVoiceIds.find(a_chain);
        members != g_effectChainVoiceIds.end()) {
        attachedVoiceIds.assign(members->second.begin(), members->second.end());
    }
    const gmVariable masterChain = GetOrCreateMasterEffectChain(*a_machine);
    for (const int voiceId : attachedVoiceIds) {
        const auto voice = State().voices.find(voiceId);
        if (voice != State().voices.end()) {
            AttachEffectChainToVoice(*a_machine, voiceId, voice->second, masterChain);
        } else {
            auto members = g_effectChainVoiceIds.find(a_chain);
            if (members != g_effectChainVoiceIds.end()) members->second.erase(voiceId);
        }
    }
    g_effectChainVoiceIds.erase(a_chain);
    if (nativeHandle.m_type == GM_INT) {
        const uint32_t handle = static_cast<uint32_t>(nativeHandle.m_value.m_int);
        g_effectChainsByHandle.erase(handle);
        ChainHandles().Release(handle);
        a_chain->Set(a_machine, "__nativeHandle",
                     gmVariable(static_cast<gmint>(EffectChainHandlePool::kInvalidHandle)));
    }
}
void ProcessDeferredEffectCleanup(gmMachine* a_machine,
                                  std::vector<DeferredEffectCleanup>& a_pending,
                                  bool a_chain) {
    if (!a_machine) return;
    for (size_t i = 0; i < a_pending.size();) {
        const DeferredEffectCleanup cleanup = a_pending[i];
        if (State().voices.find(cleanup.voiceId) != State().voices.end()) {
            ++i;
            continue;
        }
        if (cleanup.object.m_type == GM_TABLE) {
            gmTableObject* object = (gmTableObject*)a_machine->GetObject(
                cleanup.object.m_value.m_ref);
            if (a_chain) ReleaseEffectChainTable(a_machine, object);
            else ReleaseEffectHandleTable(a_machine, object);
        }
        ReleaseAudioGMVariable(*a_machine, cleanup.object);
        a_pending[i] = a_pending.back();
        a_pending.pop_back();
    }
}
int GM_CDECL gmf_chain_freeAllChildEffects(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    ReleaseEffectSlots(a_thread->GetMachine(), self);
    return GM_OK;
}
int GM_CDECL gmf_chain_freeAllChildEffectChains(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    char key[24];
    for (int i = 0; i < 16; ++i) {
        snprintf(key, sizeof(key), "__childChainSlot%d", i);
        gmVariable value = self->Get(machine, key);
        if (value.m_type == GM_TABLE) {
            gmTableObject* child = (gmTableObject*)machine->GetObject(value.m_value.m_ref);
            if (child && child->Get(machine, "__protected").m_type == GM_NULL)
                ReleaseEffectChainTable(machine, child);
            else if (child)
                child->Set(machine, "__parentChain", gmVariable::s_null);
        }
        self->Set(machine, key, gmVariable::s_null);
    }
    return GM_OK;
}
int GM_CDECL gmf_chain_addEffectChain(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine* machine = a_thread->GetMachine();
    gmVariable value = a_thread->Param(0);
    if (value.m_type != GM_TABLE) return GM_OK;
    gmTableObject* child = (gmTableObject*)machine->GetObject(value.m_value.m_ref);
    if (!child || child == self || child->Get(machine, "__freed").m_type != GM_NULL)
        return GM_OK;
    gmVariable oldParent = child->Get(machine, "__parentChain");
    if (oldParent.m_type == GM_TABLE) {
        gmTableObject* previous = (gmTableObject*)machine->GetObject(oldParent.m_value.m_ref);
        if (previous) {
            char oldKey[24];
            for (int i = 0; i < 16; ++i) {
                snprintf(oldKey, sizeof(oldKey), "__childChainSlot%d", i);
                gmVariable slot = previous->Get(machine, oldKey);
                if (slot.m_type == GM_TABLE && slot.m_value.m_ref == value.m_value.m_ref)
                    previous->Set(machine, oldKey, gmVariable::s_null);
            }
        }
    }
    for (int i = 0; i < 16; ++i) {
        char key[24];
        snprintf(key, sizeof(key), "__childChainSlot%d", i);
        if (self->Get(machine, key).m_type == GM_NULL) {
            self->Set(machine, key, value);
            break;
        }
    }
    child->Set(machine, "__parentChain", gmVariable(self));
    return GM_OK;
}
int GM_CDECL gmf_chain_addVoice(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable& voiceIdValue = a_thread->Param(0);
    if (voiceIdValue.m_type != GM_INT) return GM_OK;
    auto voice = State().voices.find(voiceIdValue.m_value.m_int);
    if (voice == State().voices.end()) return GM_OK;
    gmMachine* machine = a_thread->GetMachine();
    gmVariable chain(self);
    if (self->Get(machine, "__freed").m_type != GM_NULL)
        chain = GetOrCreateMasterEffectChain(*machine);
    AttachEffectChainToVoice(*machine, voice->first, voice->second, chain);
    return GM_OK;
}

// AUDIO.freeSoundEffect @ 0x87790 releases the native effect handle. The
// local table backend has no DSP handle, but it must release the owning
// chain's slot so later addEffect calls can reuse it.
int GM_CDECL gmf_freeSoundEffect(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable value = a_thread->Param(0);
    if (value.m_type != GM_TABLE) return GM_OK;
    gmMachine* machine = a_thread->GetMachine();
    gmTableObject* effect = (gmTableObject*)machine->GetObject(value.m_value.m_ref);
    ReleaseEffectHandleTable(machine, effect);
    return GM_OK;
}
int DeferEffectReleaseUntilVoiceEnd(gmThread* a_thread, bool a_chain) {
    if (a_thread->GetNumParams() < 2) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    const int voiceId = a_thread->ParamInt(0, 0);
    const gmVariable& object = a_thread->Param(1);
    if (object.m_type != GM_TABLE) return GM_OK;
    RetainAudioGMVariable(*machine, object);
    DeferredEffectCleanup cleanup{voiceId, object};
    if (a_chain) State().effectChainsWaitingForVoiceEnd.push_back(cleanup);
    else State().effectsWaitingForVoiceEnd.push_back(cleanup);
    return GM_OK;
}
int GM_CDECL gmf_onVoiceEndFreeEffectChain(gmThread* a_thread) {
    return DeferEffectReleaseUntilVoiceEnd(a_thread, true);
}
int GM_CDECL gmf_onVoiceEndFreeSoundEffect(gmThread* a_thread) {
    return DeferEffectReleaseUntilVoiceEnd(a_thread, false);
}
gmVariable MakeEffectChainTable(gmMachine& a_machine, const std::string& a_name,
                                const gmVariable* a_source = nullptr) {
    const uint32_t nativeHandle = ChainHandles().Allocate();
    if (nativeHandle == EffectChainHandlePool::kInvalidHandle)
        return gmVariable::s_null;
    gmTableObject* t = a_machine.AllocTableObject();
    t->Set(&a_machine, "__name", gmVariable(a_machine.AllocStringObject(a_name.c_str())));
    t->Set(&a_machine, "__nativeHandle", gmVariable(static_cast<gmint>(nativeHandle)));
    t->Set(&a_machine, "__freed", gmVariable::s_null);
    if (a_source) t->Set(&a_machine, "__source", *a_source);
    t->Set(&a_machine, "addEffect", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addEffect)));
    t->Set(&a_machine, "freeAllChildEffects", gmVariable(a_machine.AllocFunctionObject(gmf_chain_freeAllChildEffects)));
    t->Set(&a_machine, "freeAllChildEffectChains", gmVariable(a_machine.AllocFunctionObject(gmf_chain_freeAllChildEffectChains)));
    t->Set(&a_machine, "addEffectChain", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addEffectChain)));
    t->Set(&a_machine, "addVoice", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addVoice)));
    gmVariable chain(t);
    g_effectChainsByHandle[nativeHandle] = chain;
    return chain;
}
gmVariable GetOrCreateMasterEffectChain(gmMachine& a_machine) {
    static gmVariable master = gmVariable::s_null;
    static gmMachine* owner = nullptr;
    if (owner != &a_machine) {
        master = gmVariable::s_null;
        owner = &a_machine;
    }
    if (master.m_type != GM_TABLE) {
        master = MakeEffectChainTable(a_machine, "master");
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(master.m_value.m_ref);
        if (table) {
            table->Set(&a_machine, "__protected", gmVariable((gmint)1));
            RetainAudioGMVariable(a_machine, master);
        }
    }
    return master;
}
int GM_CDECL gmf_createEffectChain(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count < 1 || count > 2) {
        a_thread->PushNull();
        return GM_OK;
    }
    std::string name;
    if (a_thread->Param(0).m_type == GM_STRING) {
        char buf[256];
        name = a_thread->Param(0).AsString(a_thread->GetMachine(), buf, sizeof(buf));
    } else {
        a_thread->PushNull();
        return GM_OK;
    }
    const gmVariable* source = count == 2 ? &a_thread->Param(1) : nullptr;
    if (source && !IsLocalEffectChain(*a_thread->GetMachine(), *source)) {
        a_thread->PushNull();
        return GM_OK;
    }
    gmMachine* machine = a_thread->GetMachine();
    gmVariable parentChain = source
        ? *source : GetOrCreateMasterEffectChain(*machine);
    if (parentChain.m_type != GM_TABLE) {
        a_thread->PushNull();
        return GM_OK;
    }
    gmVariable chain = MakeEffectChainTable(*machine, name, source);
    if (chain.m_type == GM_TABLE) {
        gmTableObject* parent = static_cast<gmTableObject*>(
            machine->GetObject(parentChain.m_value.m_ref));
        gmTableObject* child = static_cast<gmTableObject*>(
            machine->GetObject(chain.m_value.m_ref));
        if (parent && child) {
            gmVariable parentVar(parent);
            child->Set(machine, "__parentChain", parentVar);
            // Reuse the same slot/link rules as EffectChain.addEffectChain.
            for (int slot = 0; slot < 16; ++slot) {
                char key[24];
                snprintf(key, sizeof(key), "__childChainSlot%d", slot);
                if (parent->Get(machine, key).m_type == GM_NULL) {
                    parent->Set(machine, key, chain);
                    break;
                }
            }
        }
    }
    a_thread->Push(chain);
    return GM_OK;
}
int GM_CDECL gmf_getMasterEffectChain(gmThread* a_thread) {
    a_thread->Push(GetOrCreateMasterEffectChain(*a_thread->GetMachine()));
    return GM_OK;
}
int GM_CDECL gmf_freeEffectChain(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable value = a_thread->Param(0);
    if (value.m_type == GM_TABLE) {
        gmTableObject* chain = (gmTableObject*)a_thread->GetMachine()->GetObject(value.m_value.m_ref);
        if (chain && chain->Get(a_thread->GetMachine(), "__protected").m_type == GM_NULL)
            ReleaseEffectChainTable(a_thread->GetMachine(), chain);
    }
    return GM_OK;
}
int GM_CDECL gmf_noop(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_falseBool(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_startPlayingScript(gmThread* a_thread) {
    (void)a_thread;
    State().playbackEventIndex = 0;
    State().playbackElapsedMs = 0.0f;
    State().scriptPlaybackVoiceIds.clear();
    State().playingScript = true;
    return GM_OK;
}
int GM_CDECL gmf_stopPlayingScript(gmThread* a_thread) {
    (void)a_thread;
    State().playingScript = false;
    StopRecordedPlaybackVoices();
    return GM_OK;
}
int GM_CDECL gmf_isPlayingScript(gmThread* a_thread) {
    a_thread->PushInt(State().playingScript ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_startRecordingScript(gmThread* a_thread) {
    ClearRecordedPlaySounds(*a_thread->GetMachine());
    State().recordedSoundResources.clear();
    State().recordingStart = std::chrono::steady_clock::now();
    State().recordingStopMs = 0.0f;
    State().recordingScript = true;
    return GM_OK;
}
int GM_CDECL gmf_stopRecordingScript(gmThread* a_thread) {
    (void)a_thread;
    State().recordingStopMs = RecordedAudioElapsedMs();
    State().recordingScript = false;
    return GM_OK;
}
int GM_CDECL gmf_isRecordingScript(gmThread* a_thread) {
    a_thread->PushInt(State().recordingScript ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_audioSaveRequestGetSize(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    a_thread->PushInt(self ? self->Get(a_thread->GetMachine(), "size").m_value.m_int : 0);
    return GM_OK;
}

std::string AudioRecorderNumber(float a_value) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << a_value;
    return out.str();
}

bool BuildRecordedAudioV4(std::string& a_outText) {
    const auto& state = State();
    std::set<std::string> resources = state.recordedSoundResources;
    std::vector<std::string> localNames(resources.begin(), resources.end());
    std::unordered_map<std::string, size_t> localIndices;
    for (size_t i = 0; i < localNames.size(); ++i) localIndices.emplace(localNames[i], i);

    std::ostringstream body;
    body.imbue(std::locale::classic());
    body << resources.size() << '\n';
    for (const auto& path : resources) body << path << '\n' << 48 << '\n';

    for (const auto& event : state.recordedPlaySounds) {
        const size_t signatureIndex = event.chainRoute ? 2u : 32u;
        body << AudioRecorderNumber(event.timeMs) << '\n'
             << kAudioRecorderSignatures[signatureIndex] << '\n'
             << event.path << '\n'
             << event.note << '\n'
             << AudioRecorderNumber(event.gate) << '\n'
             << AudioRecorderNumber(event.pan) << '\n'
             << AudioRecorderNumber(event.volume) << '\n'
             << event.priority << '\n'
             << static_cast<int32_t>(event.chainRoute
                    ? event.effectChainHandle : 0u) << '\n'
             << event.startPositionMs << '\n';
    }

    std::string tokenized;
    const std::string raw = body.str();
    size_t cursor = 0;
    while (cursor < raw.size()) {
        const size_t end = raw.find('\n', cursor);
        const size_t count = (end == std::string::npos ? raw.size() : end) - cursor;
        const std::string field = raw.substr(cursor, count);
        std::string encoded = field;
        if (!field.empty() && field[0] != '-' && (field[0] < '0' || field[0] > '9')) {
            const auto shared = std::find(kAudioRecorderSignatures.begin(),
                                          kAudioRecorderSignatures.end(), field);
            if (shared != kAudioRecorderSignatures.end()) {
                encoded = "#" + std::to_string(shared - kAudioRecorderSignatures.begin());
            } else {
                auto local = localIndices.find(field);
                if (local == localIndices.end()) {
                    const size_t index = localNames.size();
                    localNames.push_back(field);
                    local = localIndices.emplace(field, index).first;
                }
                encoded = "$" + std::to_string(local->second);
            }
        }
        tokenized += encoded;
        tokenized.push_back('\n');
        if (end == std::string::npos) break;
        cursor = end + 1;
    }

    std::ostringstream file;
    file.imbue(std::locale::classic());
    file << "FunctionObjectRecorderV4\n" << kAudioRecorderSignatures.size() << '\n';
    for (const auto signature : kAudioRecorderSignatures) file << signature << '\n';
    file << localNames.size() << '\n';
    for (const auto& name : localNames) file << name << '\n';
    file << AudioRecorderNumber(state.recordingStopMs) << '\n' << tokenized;
    a_outText = file.str();
    return true;
}

bool InflateAudioRecorder(const std::string& a_compressed, std::string& a_out) {
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK) return false;
    stream.next_in = reinterpret_cast<Bytef*>(
        const_cast<char*>(a_compressed.data()));
    stream.avail_in = static_cast<uInt>(std::min<size_t>(
        a_compressed.size(), std::numeric_limits<uInt>::max()));
    std::array<char, 16384> chunk{};
    int result = Z_OK;
    a_out.clear();
    while (result == Z_OK) {
        stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
        stream.avail_out = static_cast<uInt>(chunk.size());
        result = inflate(&stream, Z_NO_FLUSH);
        const size_t produced = chunk.size() - stream.avail_out;
        if (a_out.size() + produced > 64u * 1024u * 1024u) {
            inflateEnd(&stream);
            return false;
        }
        a_out.append(chunk.data(), produced);
    }
    inflateEnd(&stream);
    return result == Z_STREAM_END;
}

bool ReadAudioRecorderFile(const std::string& a_path, int a_area,
                           std::string& a_out) {
    std::string compressed;
    bool read = false;
    if (a_area == 0) {
        read = SaveData::ReadFile(a_path, compressed);
    } else if (a_area == 1 || a_area == 2) {
        FileHelper* files = GetEngineContext().fileHelper;
        read = files && files->ReadTextFile(a_path, compressed);
    }
    return read && InflateAudioRecorder(compressed, a_out);
}

bool ParseAudioRecorderV4(const std::string& a_text,
                          std::set<std::string>& a_resources,
                          std::vector<RecordedPlaySoundEvent>& a_events,
                          float& a_stopMs) {
    std::vector<std::string> lines;
    std::istringstream input(a_text);
    input.imbue(std::locale::classic());
    for (std::string line; std::getline(input, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
    }
    size_t cursor = 0;
    auto next = [&](const std::string*& value) {
        if (cursor >= lines.size()) return false;
        value = &lines[cursor++];
        return true;
    };
    auto parseUnsigned = [](const std::string& value, uint32_t& out) {
        if (value.empty()) return false;
        char* end = nullptr;
        const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
        if (!end || *end != '\0' || parsed > UINT32_MAX) return false;
        out = static_cast<uint32_t>(parsed);
        return true;
    };
    auto parseHandle = [&](const std::string& value, uint32_t& out) {
        if (value.empty()) return false;
        char* end = nullptr;
        const long long parsed = std::strtoll(value.c_str(), &end, 10);
        if (!end || *end != '\0' || parsed < INT32_MIN || parsed > UINT32_MAX)
            return false;
        out = static_cast<uint32_t>(parsed);
        return true;
    };
    auto parseInt = [](const std::string& value, int& out) {
        if (value.empty()) return false;
        char* end = nullptr;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        if (!end || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) return false;
        out = static_cast<int>(parsed);
        return true;
    };
    auto parseFloat = [](const std::string& value, float& out) {
        std::istringstream number(value);
        number.imbue(std::locale::classic());
        number >> out;
        return number && number.peek() == std::char_traits<char>::eof() &&
               std::isfinite(out);
    };

    const std::string* line = nullptr;
    if (!next(line) || *line != "FunctionObjectRecorderV4") return false;
    uint32_t sharedCount = 0;
    if (!next(line) || !parseUnsigned(*line, sharedCount) ||
        sharedCount != kAudioRecorderSignatures.size()) return false;
    for (const auto signature : kAudioRecorderSignatures)
        if (!next(line) || *line != signature) return false;

    uint32_t localCount = 0;
    if (!next(line) || !parseUnsigned(*line, localCount) || localCount > 100000)
        return false;
    std::vector<std::string> localNames;
    localNames.reserve(localCount);
    for (uint32_t i = 0; i < localCount; ++i) {
        if (!next(line)) return false;
        localNames.push_back(*line);
    }
    if (!next(line) || !parseFloat(*line, a_stopMs) || a_stopMs < 0.0f)
        return false;

    // Retail expands #shared and $file-local tokens across the whole raw
    // recorder body before it parses the resource list and callback stream.
    std::vector<std::string> body;
    for (; cursor < lines.size(); ++cursor) {
        std::string field = lines[cursor];
        if (field.size() > 1 && (field[0] == '#' || field[0] == '$')) {
            uint32_t index = 0;
            if (!parseUnsigned(field.substr(1), index)) return false;
            if (field[0] == '#') {
                if (index >= kAudioRecorderSignatures.size()) return false;
                field = kAudioRecorderSignatures[index];
            } else {
                if (index >= localNames.size()) return false;
                field = localNames[index];
            }
        }
        body.push_back(std::move(field));
    }

    size_t bodyCursor = 0;
    auto take = [&](const std::string*& value) {
        if (bodyCursor >= body.size()) return false;
        value = &body[bodyCursor++];
        return true;
    };
    uint32_t resourceCount = 0;
    if (!take(line) || !parseUnsigned(*line, resourceCount) || resourceCount > 100000)
        return false;
    std::set<std::string> resources;
    for (uint32_t i = 0; i < resourceCount; ++i) {
        std::string path;
        int baseNote = 0;
        if (!take(line) || line->empty()) return false;
        path = *line;
        if (!take(line) || !parseInt(*line, baseNote)) return false;
        resources.insert(std::move(path));
    }

    std::vector<RecordedPlaySoundEvent> events;
    while (bodyCursor < body.size()) {
        RecordedPlaySoundEvent event;
        const std::string* signature = nullptr;
        if (!take(line) || !parseFloat(*line, event.timeMs) || event.timeMs < 0.0f ||
            !take(signature) || !take(line) || line->empty()) return false;
        const std::string& callbackSignature = *signature;
        if (callbackSignature == kAudioRecorderSignatures[32]) {
            event.chainRoute = false;
        } else if (callbackSignature == kAudioRecorderSignatures[2]) {
            event.chainRoute = true;
        } else {
            // Native V4 supports more callback classes than this port has
            // modeled. Reject instead of silently shifting their field data.
            return false;
        }
        event.path = *line;
        if (!take(line) || !parseInt(*line, event.note) ||
            !take(line) || !parseFloat(*line, event.gate) ||
            !take(line) || !parseFloat(*line, event.pan) ||
            !take(line) || !parseFloat(*line, event.volume) ||
            !take(line) || !parseInt(*line, event.priority) ||
            !take(line) || !parseHandle(*line, event.effectChainHandle) ||
            !take(line) || !parseInt(*line, event.startPositionMs)) return false;
        if (!event.chainRoute && event.effectChainHandle != 0) return false;
        resources.insert(event.path);
        events.push_back(std::move(event));
    }

    a_resources = std::move(resources);
    a_events = std::move(events);
    return true;
}

int GM_CDECL gmf_loadScriptFromFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    const int area = a_thread->ParamInt(1, 0);
    std::string text;
    std::set<std::string> resources;
    std::vector<RecordedPlaySoundEvent> events;
    float stopMs = 0.0f;
    const bool ok = ReadAudioRecorderFile(path, area, text) &&
        ParseAudioRecorderV4(text, resources, events, stopMs);
    gmMachine* machine = a_thread->GetMachine();
    ClearRecordedPlaySounds(*machine);
    State().recordedSoundResources.clear();
    State().playbackEventIndex = 0;
    State().playbackElapsedMs = 0.0f;
    State().recordingStopMs = 0.0f;
    if (ok) {
        for (auto& event : events) {
            if (!event.chainRoute) continue;
            event.effectChain = FindEffectChainByHandle(
                *machine, event.effectChainHandle);
            if (event.effectChain.IsReference())
                RetainAudioGMVariable(*machine, event.effectChain);
        }
        State().recordedSoundResources = std::move(resources);
        State().recordedPlaySounds = std::move(events);
        State().recordingStopMs = stopMs;
    }
    return GM_OK;
}

int GM_CDECL gmf_saveScriptToFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    const int area = a_thread->ParamInt(1, 0);
    std::string compressed;
    long long size = 0;
    bool ok = false;

    // User generated levels invoke this with FILEHELPER_AREA_SAVEDATA. Other
    // areas map to platform mount roots which this desktop port does not own.
    if (area == 0) {
        std::string serialized;
        if (BuildRecordedAudioV4(serialized) && serialized.size() <= ULONG_MAX) {
            uLongf capacity = compressBound(static_cast<uLong>(serialized.size()));
            compressed.resize(capacity);
            const int result = compress2(reinterpret_cast<Bytef*>(compressed.data()), &capacity,
                reinterpret_cast<const Bytef*>(serialized.data()),
                static_cast<uLong>(serialized.size()), Z_DEFAULT_COMPRESSION);
            if (result == Z_OK) {
                compressed.resize(capacity);
                ok = SaveData::WriteFile(path, compressed);
                if (ok) size = static_cast<long long>(compressed.size());
            }
        }
    }

    gmMachine* machine = a_thread->GetMachine();
    gmTableObject* ticket = machine->AllocTableObject();
    ticket->Set(machine, "size", gmVariable(static_cast<gmint>(size)));
    ticket->Set(machine, "error", gmVariable(static_cast<gmint>(ok ? 0 : 1)));
    ticket->Set(machine, "getSize",
                gmVariable(machine->AllocFunctionObject(gmf_audioSaveRequestGetSize)));
    a_thread->Push(gmVariable(ticket));
    return GM_OK;
}

int GM_CDECL gmf_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_getDefaultBaseNote(gmThread* a_thread) {
    a_thread->PushInt(48);
    return GM_OK;
}
int GM_CDECL gmf_checkDiracPoolIntegrity(gmThread* a_thread) {
    a_thread->PushInt(State().diracPoolEntriesUsed == 0 ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getDiracPoolEntriesUsed(gmThread* a_thread) {
    a_thread->PushInt(static_cast<gmint>(State().diracPoolEntriesUsed));
    return GM_OK;
}
int GM_CDECL gmf_getDiracPoolEntriesFree(gmThread* a_thread) {
    a_thread->PushInt(AudioState::kDiracPoolCapacity -
                      static_cast<gmint>(State().diracPoolEntriesUsed));
    return GM_OK;
}
int GM_CDECL gmf_getDiracPoolCapacity(gmThread* a_thread) {
    a_thread->PushInt(AudioState::kDiracPoolCapacity);
    return GM_OK;
}

gmFunctionEntry s_audioLib[] = {
    {"loadSound", gmf_loadSound, nullptr},
    {"unloadSound", gmf_unloadSound, nullptr},
    {"unloadAllSounds", gmf_unloadAllSounds, nullptr},
    {"loadSampleList", gmf_loadSampleList, nullptr},

    {"playSound", gmf_playSound, nullptr},
    {"playSoundSlicker", gmf_playSoundSlicker, nullptr},
    {"spawnSoundSlicker", gmf_spawnSoundSlicker, nullptr},
    {"loadSoundSlicker", gmf_loadSoundSlicker, nullptr},
    {"playStream", gmf_playStream, nullptr},

    {"stopVoice", gmf_stopVoice, nullptr},
    {"stopAllVoices", gmf_stopAllVoices, nullptr},
    {"fadeOutVoice", gmf_fadeOutVoice, nullptr},
    {"isVoiceValid", gmf_isVoiceValid, nullptr},
    {"isVoicePaused", gmf_isVoicePaused, nullptr},
    {"setVoicePause", gmf_setVoicePause, nullptr},
    {"setVoiceVolume", gmf_setVoiceVolume, nullptr},
    {"getVoiceVolume", gmf_getVoiceVolume, nullptr},
    {"setVoicePan", gmf_setVoicePan, nullptr},
    {"setVoicePitchBend", gmf_setVoicePitchBend, nullptr},
    {"setVoiceGateTime", gmf_setVoiceGateTime, nullptr},
    {"setVoiceNote", gmf_setVoiceNote, nullptr},
    {"setVoiceSpeed", gmf_setVoiceSpeed, nullptr},
    {"getVoiceSpeed", gmf_getVoiceSpeed, nullptr},
    {"setVoicePosition", gmf_setVoicePosition, nullptr},
    {"getVoicePosition", gmf_getVoicePosition, nullptr},
    {"getVoiceLength", gmf_getVoiceLength, nullptr},
    {"getSoundLength", gmf_getSoundLength, nullptr},
    {"setVoiceLoop", gmf_setVoiceLoop, nullptr},
    {"setVoiceLoopCount", gmf_setVoiceLoopCount, nullptr},
    {"getAllVoices", gmf_getAllVoices, nullptr},

    {"setMasterVolume", gmf_setMasterVolume, nullptr},
    {"getMasterVolume", gmf_getMasterVolume, nullptr},
    {"setMusicVolume", gmf_setMusicVolume, nullptr},
    {"getMusicVolume", gmf_getMusicVolume, nullptr},
    {"setSFXVolume", gmf_setSFXVolume, nullptr},
    {"getSFXVolume", gmf_getSFXVolume, nullptr},

    {"createEffectChain", gmf_createEffectChain, nullptr},
    {"getMasterEffectChain", gmf_getMasterEffectChain, nullptr},
    {"freeEffectChain", gmf_freeEffectChain, nullptr},
    {"freeSoundEffect", gmf_freeSoundEffect, nullptr},
    {"onVoiceEndFreeEffectChain", gmf_onVoiceEndFreeEffectChain, nullptr},
    {"onVoiceEndFreeSoundEffect", gmf_onVoiceEndFreeSoundEffect, nullptr},

    {"startRecording", gmf_noop, nullptr},
    {"stopRecording", gmf_noop, nullptr},
    {"isRecording", gmf_falseBool, nullptr},
    {"startRecordingScript", gmf_startRecordingScript, nullptr},
    {"stopRecordingScript", gmf_stopRecordingScript, nullptr},
    {"isRecordingScript", gmf_isRecordingScript, nullptr},
    {"startPlayingScript", gmf_startPlayingScript, nullptr},
    {"stopPlayingScript", gmf_stopPlayingScript, nullptr},
    {"isPlayingScript", gmf_isPlayingScript, nullptr},
    {"saveScriptToFile", gmf_saveScriptToFile, nullptr},
    {"loadScriptFromFile", gmf_loadScriptFromFile, nullptr},
    {"startRetriggering", gmf_noop, nullptr},
    {"stopRetriggering", gmf_noop, nullptr},
    {"isRetriggering", gmf_falseBool, nullptr},
    {"getRetriggerTime", gmf_zeroInt, nullptr},
    {"setRetriggerTime", gmf_noop, nullptr},

    {"hack_setEnabled", gmf_hackSetEnabled, nullptr},
    {"setFrequencyMultiplier", gmf_setFrequencyMultiplier, nullptr},
    {"getFrequencyMultiplier", gmf_getFrequencyMultiplier, nullptr},
    {"getFreeMemory", gmf_zeroInt, nullptr},
    {"getDefaultBaseNote", gmf_getDefaultBaseNote, nullptr},
    {"getNumDiracDSPsFree", gmf_getDiracPoolEntriesFree, nullptr},
    {"getNumDiracDSPsUsed", gmf_getDiracPoolEntriesUsed, nullptr},
    {"getNumDiracDSPChannelsUsed", gmf_getDiracPoolEntriesUsed, nullptr},
    {"getNumDiracDSPs", gmf_getDiracPoolCapacity, nullptr},
    {"checkDiracPoolIntegrity", gmf_checkDiracPoolIntegrity, nullptr},
};

} // namespace

void CleanupAudioManagerTrackedResources() {
    State().loadedSounds.clear();
    State().soundLengthMs.clear();
    g_loadedSoundSamples.clear();
    g_loadedSoundModes.clear();
    g_loadedSoundResourceKinds.clear();
    g_decodedSounds.clear();
}

void StopAllVoicesForProcessListReset() {
    for (auto& entry : State().voices)
        StartVoiceFade(entry.second, 60.0);
}

int CreateVoiceNative(const std::string& a_path, float a_pan, float a_volume,
                      int a_note) {
    return CreateVoiceState(a_path, a_pan, a_volume, a_note);
}

void AdvanceAudioVoices(float a_deltaMs) {
    // Retail AudioManager dispatches its recorder timer before updating the
    // backend, and the scheduler advances by a fixed 16.666 ms step rather
    // than using the voice-mixer delta. Do this even when the backend receives
    // a zero/invalid delta for this frame.
    DispatchRecordedPlaySounds();
    for (auto& entry : State().voices) {
        VoiceState& voice = entry.second;
        if (!voice.pitchBendPending) continue;
        SetVoiceSpeedTarget(voice, voice.pendingPitchSpeed,
                            voice.pendingPitchDurationMs);
        voice.pitchBendPending = false;
    }
    if (!(a_deltaMs > 0.0f) || !std::isfinite(a_deltaMs)) {
        ProcessDeferredEffectCleanup(State().machine,
                                     State().effectsWaitingForVoiceEnd, false);
        ProcessDeferredEffectCleanup(State().machine,
                                     State().effectChainsWaitingForVoiceEnd, true);
        return;
    }
    QueueMixedVoices(a_deltaMs);
    auto& voices = State().voices;
    for (auto it = voices.begin(); it != voices.end();) {
        VoiceState& voice = it->second;
        if (voice.paused && !voice.stopping) {
            ++it;
            continue;
        }
        bool gateTriggered = false;
        if (!voice.stopping && voice.gateDurationMs > 0.0) {
            voice.gateElapsedMs += a_deltaMs;
            if (voice.gateElapsedMs >= voice.gateDurationMs) {
                // Retail's gate alarm starts a fade without marking the voice
                // stopped. Short gates can retrigger before voice cleanup.
                voice.gateFading = true;
                voice.fadeDurationMs = 60.0;
                voice.fadeRemainingMs = 60.0;
                voice.fadeGain = 1.0f;
                voice.gateElapsedMs = 0.0;
                gateTriggered = true;
            }
        }
        if (voice.stopping) {
            if (voice.stopRemainingMs > 0.0) {
                voice.stopRemainingMs -= a_deltaMs;
                if (voice.stopRemainingMs <= 0.0) {
                    if (State().machine)
                        DetachVoiceFromEffectChain(*State().machine, it->first, voice);
                    ReleaseDiracPoolEntry(voice);
                    it = voices.erase(it);
                    continue;
                }
            } else if (voice.fadeDurationMs >= 0.0) {
                if (voice.fadeGain <= 0.0f) {
                    if (State().machine)
                        DetachVoiceFromEffectChain(*State().machine, it->first, voice);
                    ReleaseDiracPoolEntry(voice);
                    it = voices.erase(it);
                    continue;
                }
                const double fadeStep =
                    (1.0 / voice.fadeDurationMs) * a_deltaMs;
                voice.fadeGain = static_cast<float>(
                    - (fadeStep - static_cast<double>(voice.fadeGain)));
                if (voice.fadeGain <= 0.0f) voice.fadeGain = 0.0f;
                voice.fadeRemainingMs =
                    static_cast<double>(voice.fadeGain) * voice.fadeDurationMs;
            }
        } else if (voice.gateFading && !gateTriggered) {
            if (voice.fadeGain > 0.0f) {
                const double fadeStep = a_deltaMs / voice.fadeDurationMs;
                voice.fadeGain = static_cast<float>(std::max(
                    static_cast<double>(voice.fadeGain) - fadeStep, 0.0));
                voice.fadeRemainingMs =
                    static_cast<double>(voice.fadeGain) * voice.fadeDurationMs;
            }
        }
        if (voice.paused) {
            ++it;
            continue;
        }
        if (!std::isfinite(voice.speed)) {
            ++it;
            continue;
        }
        voice.positionMs += PlaybackAdvanceMs(voice, a_deltaMs);
        if (voice.speedRampRate != 0.0f) {
            const float elapsedSpeedDelta = static_cast<float>(
                static_cast<double>(voice.speedRampRate) * a_deltaMs);
            const float remainingSpeedDelta =
                voice.targetSpeed - voice.speed;
            if (std::fabs(elapsedSpeedDelta) <
                std::fabs(remainingSpeedDelta)) {
                voice.speed += elapsedSpeedDelta;
            } else {
                voice.speed = voice.targetSpeed;
                voice.speedRampRate = 0.0f;
            }
        }
        double length = SoundLengthMs(voice.soundFile);
        if (voice.timeStretchedSample &&
            voice.timeStretchedSample->stereo.size() >= 2) {
            length = static_cast<double>(
                voice.timeStretchedSample->stereo.size() / 2) *
                1000.0 / kOutputRate;
        } else if (voice.sample && voice.sample->stereo.size() >= 2) {
            // Advance the software cursor against decoded sample frames, not
            // the integer-millisecond query value. The mixer loops at this
            // exact boundary, so rounding here shifts the next pass.
            length = static_cast<double>(voice.sample->stereo.size() / 2) *
                     1000.0 / kOutputRate;
        }
        // A missing or unsupported asset has no known end marker. Keep its
        // handle available for explicit stop, while still advancing its
        // script-visible cursor.
        if (length <= 0) { ++it; continue; }
        if (voice.positionMs >= 0.0 && voice.positionMs < length) {
            ++it;
            continue;
        }
        if (voice.looping || voice.loopCount > 0) {
            if (voice.loopCount > 0) --voice.loopCount;
            voice.positionMs = std::fmod(voice.positionMs, length);
            if (voice.positionMs < 0.0) voice.positionMs += length;
            ++it;
        } else {
            if (State().machine)
                DetachVoiceFromEffectChain(*State().machine, it->first, voice);
            ReleaseDiracPoolEntry(voice);
            it = voices.erase(it);
        }
    }
    ProcessDeferredEffectCleanup(State().machine,
                                 State().effectsWaitingForVoiceEnd, false);
    ProcessDeferredEffectCleanup(State().machine,
                                 State().effectChainsWaitingForVoiceEnd, true);
}

void InitializeAudioOutput() {
    if (const char* capturePath = std::getenv("GE3_AUDIO_CAPTURE_RAW")) {
        g_audioCaptureFile = std::fopen(capturePath, "wb");
        if (!g_audioCaptureFile)
            std::fprintf(stderr, "Audio capture unavailable: %s\n", capturePath);
    }
    if (std::getenv("GE3_HEADLESS") && !std::getenv("GE3_AUDIO_OUTPUT")) return;
    g_audioOutputStarted = g_audioFrontend && g_audioFrontend->StartAudioOutput(kOutputRate);
}

void ShutdownAudioOutput() {
    if (g_audioCaptureFile) {
        std::fclose(g_audioCaptureFile);
        g_audioCaptureFile = nullptr;
    }
    if (g_audioOutputStarted && g_audioFrontend) g_audioFrontend->StopAudioOutput();
    g_audioOutputStarted = false;
    g_decodedSounds.clear();
    g_timeStretchedSounds.clear();
    g_loadedSoundSamples.clear();
    g_loadedSoundModes.clear();
    g_loadedSoundResourceKinds.clear();
}

void SetMasterVolumeNative(float a_vol) { State().masterVolume = a_vol; }
float GetMasterVolumeNative() { return State().masterVolume; }

// External-linkage forwarder so entity_bindings.cpp can reuse the exact
// same playSound implementation for EntityBase.playSound -- see
// audio_bindings.h. A distinct name from the anonymous-namespace
// gmf_playSound above (internal linkage, not visible from other
// translation units) -- this just calls straight through to it.
int GM_CDECL PlaySoundNative(gmThread* a_thread) { return gmf_playSound(a_thread); }

void RegisterAudioBindings(gmMachine& a_machine, PlatformFrontend& a_frontend) {
    if (State().machine != &a_machine) {
        ChainHandles().Reset();
        g_effectChainsByHandle.clear();
    }
    g_audioFrontend = &a_frontend;
    State().machine = &a_machine;
    // The retail scheduler constructor initializes its timer before the first
    // startRecordingScript call; stopRecordingScript can therefore snapshot
    // meaningful elapsed time even when called without an active recording.
    State().recordingStart = std::chrono::steady_clock::now();
    a_machine.RegisterLibrary(s_audioLib, sizeof(s_audioLib) / sizeof(s_audioLib[0]), "Audio");
}
