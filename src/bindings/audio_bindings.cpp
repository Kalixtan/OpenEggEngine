#include "gmThread.h"
#include "audio_bindings.h"
#include "gmTableObject.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
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
#include <memory>
#include <optional>
#include <vector>

// ---------------------------------------------------------------------
// citations on each binding below) plus recovered method-table wrappers for
// Audio.getVoiceVolume @ 0x0008603c and Audio.setVoiceVolume @ 0x00086124.
// EntityBase also has separate voice-volume methods at 0x001c5f50/0x001c5fe0;
// those operate on receiver-owned voice lists and are not these Audio APIs.
//
// The platform audio service plays decoded WAV samples from active voice
// handles. Retail effects, priority, and exact mixer routing
// remain to be ported; the script-facing voice registry stays authoritative.
// ---------------------------------------------------------------------

namespace {

constexpr int kOutputRate = 44100;
struct DecodedSound {
    std::vector<float> stereo;
};
struct DeferredEffectCleanup {
    int voiceId = 0;
    gmVariable object;
};
PlatformFrontend* g_audioFrontend = nullptr;
bool g_audioOutputStarted = false;
double g_fractionalOutputFrames = 0.0;
std::unordered_map<std::string, std::weak_ptr<DecodedSound>> g_decodedSounds;
std::unordered_map<std::string, std::shared_ptr<DecodedSound>> g_loadedSoundSamples;
std::unordered_map<std::string, int> g_loadedSoundModes;
std::unordered_map<std::string, int> g_loadedSoundResourceKinds;

struct VoiceState {
    std::string soundFile;
    float volume = 1.0f;
    float pan = 0.0f;
    int referenceNote = 48;
    float speed = 1.0f;
    float targetSpeed = 1.0f;
    double speedRampRemainingMs = 0.0;
    double positionMs = 0.0;
    double gateDurationMs = 0.0;
    double gateElapsedMs = 0.0;
    double fadeRemainingMs = 0.0;
    double fadeDurationMs = 0.0;
    double stopRemainingMs = 0.0;
    bool stopping = false;
    bool paused = false;
    bool looping = false;
    int loopCount = 0;
    std::shared_ptr<DecodedSound> sample;
    bool sampleLoadAttempted = false;
};

struct AudioState {
    std::unordered_map<int, VoiceState> voices;
    std::vector<DeferredEffectCleanup> effectChainsWaitingForVoiceEnd;
    std::vector<DeferredEffectCleanup> effectsWaitingForVoiceEnd;
    std::unordered_set<std::string> loadedSounds;
    std::unordered_map<std::string, int> soundLengthMs;
    int nextVoiceId = 1;
    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    bool enabled = true; // Retail gates playback/Slicker paths at manager +0x2c3c
    float frequencyMultiplier = 1.0f; // constructor initializes +0x30f0 to 1.0
    gmMachine* machine = nullptr;
};
AudioState& State() {
    static AudioState s;
    return s;
}

struct AudioGMRootState { unsigned uses = 0; bool addedRoot = false; };
std::unordered_map<gmObject*, AudioGMRootState> g_audioGMRoots;
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
void ProcessDeferredEffectCleanup(gmMachine* a_machine,
                                 std::vector<DeferredEffectCleanup>& a_pending,
                                 bool a_chain);

// RETAIL BYTECODE: FUN_0001b718/FUN_0001b768/FUN_0001b7c8 clamp the incoming
// double to [0,1] with the PPC fsub/fsel sequence before storing an f32.
float RetailClampAudioVolume(float a_value) {
    const double value = static_cast<double>(a_value);
    const double lowerTest = 0.0 - value;
    const double lowerClamped = std::signbit(lowerTest) ? value : 0.0;
    const double upperTest = 1.0 - lowerClamped;
    const double result = std::signbit(upperTest) ? 1.0 : lowerClamped;
    return static_cast<float>(result);
}

VoiceState* FindVoice(int a_id) {
    auto it = State().voices.find(a_id);
    return it == State().voices.end() ? nullptr : &it->second;
}

void SetVoiceSpeedTarget(VoiceState& a_voice, float a_target,
                         float a_durationMs) {
    if (!std::isfinite(a_target)) return;
    a_voice.targetSpeed = a_target;
    if (std::isfinite(a_durationMs) && a_durationMs > 0.0f) {
        a_voice.speedRampRemainingMs = a_durationMs;
    } else {
        a_voice.speed = a_target;
        a_voice.speedRampRemainingMs = 0.0;
    }
}

// Integrate the retail speed target's linear ramp through a partial frame.
// The current voice.speed is the value at this frame's start.
double PlaybackAdvanceMs(const VoiceState& a_voice, double a_elapsedMs) {
    if (a_voice.speedRampRemainingMs <= 0.0)
        return a_elapsedMs * a_voice.speed;
    const double rampMs = std::min(a_elapsedMs, a_voice.speedRampRemainingMs);
    const double slope = (a_voice.targetSpeed - a_voice.speed) /
                         a_voice.speedRampRemainingMs;
    return rampMs * a_voice.speed + 0.5 * slope * rampMs * rampMs +
           (a_elapsedMs - rampMs) * a_voice.targetSpeed;
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
    std::ifstream stream(*resolved, std::ios::binary);
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

std::shared_ptr<DecodedSound> DecodeSound(const std::string& a_path) {
    auto cached = g_decodedSounds.find(a_path);
    if (cached != g_decodedSounds.end()) {
        if (auto sample = cached->second.lock()) return sample;
    }
    FileHelper* files = GetEngineContext().fileHelper;
    if (!files) return {};
    std::optional<std::string> resolved;
    if (a_path.size() > 4 &&
        a_path.compare(a_path.size() - 4, 4, ".vag") == 0) {
        // Most shipped Sony VAG assets also have a RIFF sibling. The active
        // frontend decodes that sibling without a second ADPCM decoder.
        std::string wavePath = a_path.substr(0, a_path.size() - 4) + ".wav";
        resolved = files->Resolve(wavePath);
    }
    if (!resolved) resolved = files->Resolve(a_path);
    if (!resolved) return {};
    auto sample = std::make_shared<DecodedSound>();
    if (!g_audioFrontend || !g_audioFrontend->DecodeWave(*resolved, kOutputRate, sample->stereo))
        return {};
    g_decodedSounds[a_path] = sample;
    return sample;
}

void QueueMixedVoices(float a_deltaMs) {
    if (!g_audioOutputStarted || !g_audioFrontend) return;
    const double wanted = a_deltaMs * kOutputRate / 1000.0 + g_fractionalOutputFrames;
    int frameCount = (int)std::min(8192.0, std::floor(wanted));
    g_fractionalOutputFrames = wanted - std::floor(wanted);
    if (frameCount <= 0) return;
    std::vector<float> mix((size_t)frameCount * 2, 0.0f);
    // Retail's +0x2c3c flag gates playback/resource entry paths. It is not
    // read in the retail per-frame voice update, so this mixer must not treat
    // it as a global mute. The binding-side creation gate remains under audit.
    // FUN_0001b050 sends requested voice gain multiplied by manager SFX and
    // master volumes to the backend.
    const float voiceGainScale = State().masterVolume * State().sfxVolume;
    for (auto& entry : State().voices) {
        VoiceState& voice = entry.second;
        if (voice.paused ||
            (voice.speed == 0.0f && voice.speedRampRemainingMs <= 0.0) ||
            !std::isfinite(voice.speed)) continue;
        if (!voice.sampleLoadAttempted) {
            voice.sample = DecodeSound(voice.soundFile);
            voice.sampleLoadAttempted = true;
        }
        if (!voice.sample || voice.sample->stereo.size() < 2) continue;
        const size_t soundFrames = voice.sample->stereo.size() / 2;
        const float pan = std::clamp(voice.pan, -1.0f, 1.0f);
        const float gain = std::max(0.0f, voice.volume * voiceGainScale);
        const float leftPan = pan > 0.0f ? 1.0f - pan : 1.0f;
        const float rightPan = pan < 0.0f ? 1.0f + pan : 1.0f;
        for (int frame = 0; frame < frameCount; ++frame) {
            const double frameMs = (double)frame * 1000.0 / kOutputRate;
            if (voice.stopping && voice.fadeDurationMs > 0.0 &&
                voice.fadeRemainingMs <= frameMs) continue;
            if (voice.stopping && voice.stopRemainingMs > 0.0 &&
                voice.stopRemainingMs <= frameMs) continue;
            float fade = voice.fadeDurationMs > 0.0
                ? (float)std::clamp((voice.fadeRemainingMs - frameMs) /
                                    voice.fadeDurationMs, 0.0, 1.0)
                : 1.0f;
            if (!voice.stopping && voice.gateDurationMs > 0.0 &&
                voice.gateElapsedMs + frameMs >= voice.gateDurationMs) {
                const double sinceGateMs = voice.gateElapsedMs + frameMs -
                                           voice.gateDurationMs;
                fade = std::min(fade, (float)std::clamp(
                    (60.0 - sinceGateMs) / 60.0, 0.0, 1.0));
            }
            const double positionMs = voice.positionMs +
                PlaybackAdvanceMs(voice, frameMs);
            double sourceFrame = positionMs * kOutputRate / 1000.0;
            if (voice.looping || voice.loopCount > 1) {
                sourceFrame = std::fmod(sourceFrame, (double)soundFrames);
                if (sourceFrame < 0.0) sourceFrame += soundFrames;
            }
            if (sourceFrame < 0.0 || sourceFrame >= soundFrames) continue;
            const size_t i = (size_t)sourceFrame;
            const size_t next = i + 1 < soundFrames ? i + 1 : i;
            const float blend = (float)(sourceFrame - i);
            mix[(size_t)frame * 2] +=
                (voice.sample->stereo[i * 2] * (1.0f - blend) +
                 voice.sample->stereo[next * 2] * blend) * gain * fade * leftPan;
            mix[(size_t)frame * 2 + 1] +=
                (voice.sample->stereo[i * 2 + 1] * (1.0f - blend) +
                 voice.sample->stereo[next * 2 + 1] * blend) * gain * fade * rightPan;
        }
    }
    for (float& sample : mix) sample = std::clamp(sample, -1.0f, 1.0f);
    const size_t maxQueuedBytes = (size_t)(kOutputRate * 2 * sizeof(float) / 4);
    if (g_audioFrontend->QueuedAudioBytes() > maxQueuedBytes)
        g_audioFrontend->ClearAudioQueue();
    g_audioFrontend->QueueAudioSamples(mix.data(), mix.size());
}

int CreateVoiceState(const std::string& a_path, float a_pan, float a_volume,
                     int a_note = 96, bool a_applyFrequencyMultiplier = false) {
    const int id = State().nextVoiceId++;
    VoiceState voice;
    voice.soundFile = a_path;
    voice.volume = a_volume;
    voice.pan = a_pan;
    // Retail audio-manager constructor at 0x00022528 initializes its
    // default base note (+0x24) to C4=48. FUN_000236bc then calls
    // FUN_0001ad48 with the requested note when starting a voice;
    // 96 is its special "play at the sample's base speed" value.
    voice.speed = a_note == 96 ? 1.0f :
        std::pow(2.0f, (float)(a_note - voice.referenceNote) / 12.0f);
    // Retail FUN_000236bc multiplies the newly-created backend voice's base
    // frequency by Audio manager +0x30f0. Every accepted playSound overload
    // reaches this helper directly or through FUN_0001a878/1a958/1cdd0.
    if (a_applyFrequencyMultiplier)
        voice.speed *= State().frequencyMultiplier;
    voice.targetSpeed = voice.speed;
    State().voices[id] = std::move(voice);
    return id;
}

// --- loading ---------------------------------------------------------

// @ 0x00084984: exactly 1 or 2 args; returns whether the native load
// produced a handle (not the handle itself). The campaign menu branches on
// failure to select a fallback preview, so an unresolved file must fail.
int GM_CDECL gmf_loadSound(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 2) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    if (count == 2) { GM_INT_PARAM(area, 1, 0); (void)area; }
    FileHelper* files = GetEngineContext().fileHelper;
    const bool found = files && files->Resolve(path).has_value();
    if (found) State().loadedSounds.insert(path);
    a_thread->PushInt(found ? 1 : 0);
    return GM_OK;
}
// Audio.unloadSound(path) -- confirmed real, 0x00084ac4: 1 arg (path
// string, not a handle), no return value.
int GM_CDECL gmf_unloadSound(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    State().loadedSounds.erase(path);
    return GM_OK;
}
// Audio.unloadAllSounds() -- confirmed real body is a NO-OP in retail
// itself: the decompiled function contains the literal string "WARNING:
// disabled Audio.unloadAllSounds for now because it may crash dirac" and
// skips the actual unload, only doing unrelated bookkeeping around it
// (0x00084be8). A genuine "shipped disabled" retail function, not a gap.
int GM_CDECL gmf_unloadAllSounds(gmThread* a_thread) { return GM_OK; }
// Audio.loadSampleList(path) -- confirmed real, 0x00084b28: 1 arg, returns
// a value typed the same way loadSound's success flag is (type=1/INT).
int GM_CDECL gmf_loadSampleList(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, ""); (void)path;
    a_thread->PushInt(1);
    return GM_OK;
}

// --- playback ----------------------------------------------------------

// Retail AUDIO.playSound is the function at EBOOT.elf 0x00085174, reached
// through AUDIO table entry 0x00421460. Its arity dispatch accepts 1, 3,
// and 5-8 arguments. The three-argument branch passes two numeric values
// to a separate playback overload; shipped menu calls use (path, 0, volume),
// so these are pan and volume, not note and gate. The 5-8 argument branches
// use (path, note, gate, pan, volume, [priority, effectChain, position]).
// Rejecting 2 and 4 args matches the retail wrapper's error branch.
int GM_CDECL gmf_playSound(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 3 && (count < 5 || count > 8)) return GM_EXCEPTION;
    GM_STRING_PARAM(path, 0, "");
    float pan = 0.0f, vol = 1.0f;
    float gate = 0.0f;
    int note = 96;
    if (count == 3) {
        pan = a_thread->ParamFloatOrInt(1, 0.0f);
        vol = a_thread->ParamFloatOrInt(2, 1.0f);
    } else if (count >= 5) {
        note = (int)a_thread->ParamFloatOrInt(1, 0.0f);
        gate = a_thread->ParamFloatOrInt(2, 0.0f);
        pan = a_thread->ParamFloatOrInt(3, 0.0f);
        vol = a_thread->ParamFloatOrInt(4, 1.0f);
    }

    // FUN_000236bc checks manager +0x2c3c before completing voice creation.
    // Its disabled result is DAT_00432dac, a zero-initialized BSS word.
    if (!State().enabled) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const int id = CreateVoiceState(path, pan, vol, note, true);
    State().voices[id].gateDurationMs = gate;
    a_thread->PushInt(id);
    return GM_OK;
}
// Verification: bytecode checked against Audio.c at EBOOT 0x84dc8. Retail
// accepts exactly five arguments; its native resource/backend path through
// FUN_002b6af4, FUN_002d2de8, and FUN_00024928 is not ported, so valid calls
// are surfaced by StrictCalls instead of fabricating a silent voice handle.
int GM_CDECL gmf_spawnSoundSlicker(gmThread* a_thread) {
    if (a_thread->GetNumParams() != 5) return GM_EXCEPTION;
    return StrictCalls::Report(a_thread, "Audio.spawnSoundSlicker");
}

// Verification: bytecode checked against Audio.c at EBOOT 0x85008. Retail
// accepts exactly four arguments and operates on the existing voice handle
// in argument 0 through FUN_000244b8; it does not create a new voice. That
// existing-handle/backend operation is not ported yet, so report valid calls.
int GM_CDECL gmf_playSoundSlicker(gmThread* a_thread) {
    if (a_thread->GetNumParams() != 4) return GM_EXCEPTION;
    return StrictCalls::Report(a_thread, "Audio.playSoundSlicker");
}

// Verification: bytecode checked against Audio.c at EBOOT 0x85940. Retail
// dispatches 1, 3, and 6 argument overloads to distinct stream helpers and
// returns an exception for every other arity. The resource/stream backend is
// not ported yet, so accepted calls are logged as a known missing function.
int GM_CDECL gmf_playStream(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 3 && count != 6) return GM_EXCEPTION;
    return StrictCalls::Report(a_thread, "Audio.playStream");
}
// Audio.loadSoundSlicker() -- confirmed real body is LITERALLY `loadSound();
// return;` in retail (0x00084da8) -- forwards to loadSound with NO
// arguments (hits loadSound's own 0-arg exception branch and does
// nothing useful), a genuine retail no-op/dead-code call, not a gap here.
int GM_CDECL gmf_loadSoundSlicker(gmThread* a_thread) { (void)a_thread; return GM_OK; }

// --- voice control -------------------------------------------------------

void StartVoiceFade(VoiceState& a_voice, double a_durationMs) {
    a_voice.stopping = true;
    a_voice.fadeDurationMs = a_durationMs;
    a_voice.fadeRemainingMs = a_durationMs;
    a_voice.stopRemainingMs = 0.0;
}

// Audio.stopVoice(voice, [timeMs]) -- 0x00086b20 accepts 1-2 args.
// Retail FUN_0001d6f0 schedules a timed stop for a positive value. For
// zero/negative values it calls FUN_0001d59c with a literal 60 ms fade.
int GM_CDECL gmf_stopVoice(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 1 && count != 2) return GM_EXCEPTION;
    const int voice = (int)a_thread->ParamFloatOrInt(0, 0.0f);
    const float timeMs = count == 2 ? a_thread->ParamFloatOrInt(1, 0.0f) : 0.0f;
    if (VoiceState* v = FindVoice(voice)) {
        if (std::isfinite(timeMs) && timeMs > 0.0f) {
            v->stopping = true;
            v->stopRemainingMs = timeMs;
            v->fadeRemainingMs = v->fadeDurationMs = 0.0;
        } else {
            StartVoiceFade(*v, 60.0);
        }
    }
    return GM_OK;
}
// Bytecode verified against Audio.stopAllVoices 0x86c20 -> FUN_0001dd08:
// snapshot each active voice ID, then invoke FUN_0001d6f0 with duration 0.
// That helper maps nonpositive durations to FUN_0001d59c with DAT_0001d6e8
// (60 ms), so voices fade out and remain valid until their fade completes.
int GM_CDECL gmf_stopAllVoices(gmThread* a_thread) {
    (void)a_thread;
    for (auto& entry : State().voices)
        StartVoiceFade(entry.second, 60.0);
    return GM_OK;
}
// Audio.fadeOutVoice(voice, fadeMs) forwards the duration to FUN_0001d59c
// (retail wrapper 0x00086994). Keep the voice live while its gain falls.
int GM_CDECL gmf_fadeOutVoice(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    const float fadeMs = a_thread->ParamFloatOrInt(1, 0.0f);
    if (VoiceState* v = FindVoice(voice)) {
        if (std::isfinite(fadeMs) && fadeMs > 0.0f)
            StartVoiceFade(*v, fadeMs);
        else {
            State().voices.erase(voice);
        }
    }
    return GM_OK;
}
// Audio.isVoiceValid(voice) -- confirmed real, 0x00085bc4.
int GM_CDECL gmf_isVoiceValid(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    a_thread->PushInt(FindVoice(voice) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_isVoicePaused(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    VoiceState* v = FindVoice(voice);
    a_thread->PushInt((v && v->paused) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setVoicePause(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_INT_PARAM(pause, 1, 0);
    if (VoiceState* v = FindVoice(voice)) v->paused = (pause != 0);
    return GM_OK;
}
// Audio.setVoiceVolume(voice, volume) -- retail wrapper 0x00086124 calls
// FUN_0001b050, which suppresses the gain mutation while the voice gate is
// expired. The timer is reset by the retail update after gate expiry.
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
    GM_INT_PARAM(voice, 0, 0);
    VoiceState* v = FindVoice(voice);
    // FUN_0001d330 reads raw backend gain (+0xac), divides by master*SFX,
    // and returns zero when that product is zero. Retail applies fades in the
    // output path (FUN_00209a3c), separately from this base-gain getter.
    const float voiceGainScale = State().masterVolume * State().sfxVolume;
    a_thread->PushFloat(v && voiceGainScale != 0.0f ? v->volume : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_setVoicePan(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_FLOAT_OR_INT_PARAM(pan, 1, 0.0f);
    if (VoiceState* v = FindVoice(voice)) v->pan = pan;
    return GM_OK;
}
// Audio.setVoicePitchBend(voice, targetNote, durationMs) -- wrapper
// 0x00086354 forwards both values to FUN_0001d3d0. That helper targets
// baseSpeed * pow(2, (targetNote - referenceNote) / 12), then interpolates
// playback speed linearly over the supplied duration.
int GM_CDECL gmf_setVoicePitchBend(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(voice, 0, 0);
    const int targetNote = (int)a_thread->ParamFloatOrInt(1, 0.0f);
    const float durationMs = a_thread->ParamFloatOrInt(2, 0.0f);
    if (VoiceState* v = FindVoice(voice)) {
        const float targetSpeed = std::pow(2.0f,
            (float)(targetNote - v->referenceNote) / 12.0f);
        SetVoiceSpeedTarget(*v, targetSpeed, durationMs);
    }
    return GM_OK;
}
// FUN_0001abf4 stores the new gate duration without resetting its elapsed
// timer. The update at 0x00022e0c schedules a default stop when it expires.
int GM_CDECL gmf_setVoiceGateTime(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    const float gateMs = a_thread->ParamFloatOrInt(1, 0.0f);
    if (VoiceState* v = FindVoice(voice)) v->gateDurationMs = gateMs;
    return GM_OK;
}
// FUN_0001ad48 sets the voice's pitch immediately. Note 96 is a special
// request for the sample's base speed; other notes use the same semitone
// ratio as pitch bend, relative to the voice's reference note.
int GM_CDECL gmf_setVoiceNote(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    const int note = (int)a_thread->ParamFloatOrInt(1, 0.0f);
    if (VoiceState* v = FindVoice(voice)) {
        const float speed = note == 96 ? 1.0f :
            std::pow(2.0f, (float)(note - v->referenceNote) / 12.0f);
        SetVoiceSpeedTarget(*v, speed, 0.0f);
    }
    return GM_OK;
}
// FUN_0001d8c8 uses zero duration for an immediate set and otherwise
// interpolates toward baseSpeed * requested multiplier. The getter at
// 0x00086620 returns the current multiplier (or zero for a missing voice).
int GM_CDECL gmf_setVoiceSpeed(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 2 && count != 3) return GM_EXCEPTION;
    GM_INT_PARAM(voice, 0, 0);
    GM_FLOAT_OR_INT_PARAM(speed, 1, 1.0f);
    const float durationMs = count == 3
        ? a_thread->ParamFloatOrInt(2, 0.0f) : 0.0f;
    if (VoiceState* v = FindVoice(voice))
        SetVoiceSpeedTarget(*v, speed, durationMs);
    return GM_OK;
}
int GM_CDECL gmf_getVoiceSpeed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    VoiceState* v = FindVoice(voice);
    a_thread->PushFloat(v ? v->speed : 0.0f);
    return GM_OK;
}
// Audio.setVoicePosition(voice, msPosition)/getVoicePosition(voice) --
// confirmed real, 0x000866d8/0x00086764: position is a playback cursor in
// MILLISECONDS (arg1 read as a plain int, not a spatial coordinate --
// confirmed both by the decompiled body and by every real call site, e.g.
// Platformer's sequencer helpers: `Audio.setVoicePosition(vid,
// step*msPerStep)`), not a stereo/3D position despite the name.
int GM_CDECL gmf_setVoicePosition(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    // Lenient, not GM_INT_PARAM: real call sites compute this as
    // `step*MainSequencer.getMSPerStep()` (Platformer/MainMenu5/
    // HomeScreenBase.gm:278/292/315 etc.) -- msPerStep is a float, so this
    // is a GM_FLOAT the overwhelming majority of the time, and the strict
    // int-only macro threw a blank-message exception on every single one
    // of those real calls.
    int ms = (int)a_thread->ParamFloatOrInt(1, 0.0f);
    if (VoiceState* v = FindVoice(voice)) v->positionMs = ms;
    return GM_OK;
}
int GM_CDECL gmf_getVoicePosition(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    VoiceState* v = FindVoice(voice);
    double position = v ? v->positionMs : 0.0;
    if (position > std::numeric_limits<int>::max()) position = std::numeric_limits<int>::max();
    if (position < std::numeric_limits<int>::min()) position = std::numeric_limits<int>::min();
    a_thread->PushInt((int)position);
    return GM_OK;
}
int GM_CDECL gmf_getVoiceLength(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
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
    // RETAIL BYTECODE: float-tagged payload is read as float; all other tags
    // are sign-extended from the 32-bit payload and converted to float.
    if (a_thread->GetNumParams() > 0) {
        const gmVariable& value = a_thread->Param(0);
        State().frequencyMultiplier = value.m_type == GM_FLOAT
            ? value.m_value.m_float
            : static_cast<float>(value.m_value.m_int);
    }
    return GM_OK;
}
int GM_CDECL gmf_hackSetEnabled(gmThread* a_thread) {
    // RETAIL BYTECODE: stores (raw argument payload != 0) at manager +0x2c3c.
    State().enabled = a_thread->GetNumParams() > 0 &&
                      a_thread->Param(0).m_value.m_int != 0;
    return GM_OK;
}
int GM_CDECL gmf_setVoiceLoop(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_INT_PARAM(loop, 1, 0);
    if (VoiceState* v = FindVoice(voice)) v->looping = (loop != 0);
    return GM_OK;
}
int GM_CDECL gmf_setVoiceLoopCount(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_INT_PARAM(count, 1, 0);
    if (VoiceState* v = FindVoice(voice)) v->loopCount = count;
    return GM_OK;
}
// Audio.getAllVoices() -- confirmed real, 0x00086c70: pushes a value
// type-tagged DAT_0043c3a8, the same ref-container type tag Sequencer.
// getAllTriggers uses (TYPES.md's INTLISTREFCONTAINER family) -- walked
// with isValid()/get()/goToNext(), NOT foreach/tableCount. Confirmed live:
// PauseProcess.gm's pause-menu constructor (`for(it=Audio.getAllVoices();
// it.isValid();it.goToNext()){local vid=it.get();
// if(!Audio.isVoicePaused(vid)){this.voiceList.pushBack(vid);
// Audio.setVoicePause(vid,true);}}`) pauses every active voice when the
// game pauses and restores them on unpause -- a plain indexed table (this
// engine's previous return value, no isValid/get/goToNext at all) null-
// called on the very first isValid() check, so pausing the game silently
// paused NO voices and the restore list on unpause stayed empty.
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

// Audio.setMasterVolume/getMasterVolume -- confirmed real, 0x00085c88/
// 0x00085d18.
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
    // FUN_0001b7ac returns musicVolume * masterVolume with fmuls.
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
    // FUN_0001b03c returns sfxVolume * masterVolume with fmuls.
    a_thread->PushFloat(State().sfxVolume * State().masterVolume);
    return GM_OK;
}

// --- effect chains / effects: real objects, not stubs ---------------------
// Confirmed real: createEffectChain/getMasterEffectChain both push a return
// on the success path), not a plain int. Confirmed by real usage too --
// EntityBasePS3.gm/EntityBaseNGP.gm (`t.loopTimeStretchChain.addEffect(
// SOUNDEFFECTTYPE_TIMESTRETCH)`) and Main.gm (`Audio.getMasterEffectChain().
// freeAllChildEffects();`, `this.globalEffectsChain.addEffect(
// SOUNDEFFECTTYPE_LOWPASS,FILTER_CUTOFF,300,FILTER_RES,3.3)`) all call
// .addEffect()/.freeAllChildEffects()/.freeAllChildEffectChains() on the
// object createEffectChain()/getMasterEffectChain() returned, and
// .setParam()/.setBypass() on the object addEffect() returned -- method-call
// syntax a plain int (this engine's previous return value) cannot support,
// silently null-calling and dropping every real effect-chain/timestretch
// setup in the corpus. No real DSP backend exists (see file header), so
// these are genuine GM table objects carrying effect type, parameter, and
// ownership state, but are not wired to actual DSP processing.
bool RetailEffectTypeHasParameters(int a_type) {
    return (a_type >= 3 && a_type <= 12) || (a_type >= 15 && a_type <= 20) ||
           a_type == 0x10000;
}
gmVariable RetailEffectParameterValue(const gmVariable& a_value, int a_type, int a_paramId) {
    // FUN_0031a774 interprets GM_INT as signed integer and every other tag as
    // a float payload. The type-specific adapters then apply these exact
    // float-rounded unit conversions before calling the DSP setter.
    double value = a_value.m_type == GM_INT
        ? (double)a_value.m_value.m_int : (double)a_value.m_value.m_float;
    bool scaled = false;
    if (a_type == 15 && (a_paramId == 0 || a_paramId == 1)) {
        value = (double)(float)(value * 100.0); // DAT_0001e9c0 = 0x42c80000
        scaled = true;
    } else if (a_type == 17 && (a_paramId == 4 || a_paramId == 7 || a_paramId == 9)) {
        value = (double)(float)(value * 1000.0); // DAT_0001f200 = 0x447a0000
        scaled = true;
    } else if (a_type == 17 && (a_paramId == 10 || a_paramId == 11)) {
        value = (double)(float)(value * 100.0); // DAT_0001f204 = 0x42c80000
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
// Confirmed real: 1+ args (effectType, [paramId, value]*) -- 0 args is a
// "Needs at least EFFECT_TYPE" and returns null). The (paramId, value)
// pairs starting at arg index 1 are applied immediately, matching every
// real call site (Main.gm's `addEffect(SOUNDEFFECTTYPE_LOWPASS,
// FILTER_CUTOFF,300,FILTER_RES,3.3)`).
int GM_CDECL gmf_chain_addEffect(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    int n = a_thread->GetNumParams();
    if (n < 1) { a_thread->PushNull(); return GM_OK; }
    // FUN_00024c18 scans the receiver chain's eight +0x4c effect slots and
    // returns null when all are occupied. Model those slots on the script
    // table so add/free/freeAll have the same observable capacity and reuse.
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
    ReleaseEffectSlots(a_machine, a_chain);
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
        // Retail FUN_00022c78 erases completed records by replacing each with
        // the vector's last pair, then shrinking the end pointer.
        a_pending[i] = a_pending.back();
        a_pending.pop_back();
    }
}
// Confirmed real 0-arg method; EBOOT helper 0x1e2a4 releases all eight
// receiver-owned effect slots. The table model also marks retained handles
// freed and makes their slots available for reuse.
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
// Confirmed real one-argument method. Retail helper 0x19a10 reparents the
// child chain and links it in a 16-slot list; local tables mirror that
// ownership while the unavailable backend channel-group routing stays open.
int GM_CDECL gmf_chain_addEffectChain(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine* machine = a_thread->GetMachine();
    gmVariable value = a_thread->Param(0);
    if (value.m_type != GM_TABLE) return GM_OK;
    gmTableObject* child = (gmTableObject*)machine->GetObject(value.m_value.m_ref);
    if (!child || child == self || child->Get(machine, "__freed").m_type != GM_NULL)
        return GM_OK;
    // FUN_00019a10 moves a child from its prior parent before linking it here.
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
    (void)self;
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
// DESKTOP APPROXIMATION: retail FUN_0001e508/FUN_0001e63c gate callback-object
// scheduling on manager mode, large voice IDs, high-bit native handles, and a
// reentrancy counter, then append records to manager vectors at +0x2c44/+0x2c54.
// Retail FUN_00022c78 scans each vector after voice updates and releases its
// object only when the referenced voice ID has left the live-voice map. Locally,
// mirror that manager-level deferred lifetime with rooted GM tables; the native
// callback scheduling gates/objects and handle representation remain absent.
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
    gmTableObject* t = a_machine.AllocTableObject();
    t->Set(&a_machine, "__name", gmVariable(a_machine.AllocStringObject(a_name.c_str())));
    t->Set(&a_machine, "__freed", gmVariable::s_null);
    if (a_source) t->Set(&a_machine, "__source", *a_source);
    t->Set(&a_machine, "addEffect", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addEffect)));
    t->Set(&a_machine, "freeAllChildEffects", gmVariable(a_machine.AllocFunctionObject(gmf_chain_freeAllChildEffects)));
    t->Set(&a_machine, "freeAllChildEffectChains", gmVariable(a_machine.AllocFunctionObject(gmf_chain_freeAllChildEffectChains)));
    t->Set(&a_machine, "addEffectChain", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addEffectChain)));
    t->Set(&a_machine, "addVoice", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addVoice)));
    return gmVariable(t);
}
int GM_CDECL gmf_createEffectChain(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    // RETAIL BYTECODE: only one and two argument branches call chain factory
    // helpers. Zero args and 3+ args push GM_NULL.
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
    if (source && source->m_type != GM_TABLE && source->m_type != GM_USER) {
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->Push(MakeEffectChainTable(*a_thread->GetMachine(), name, source));
    return GM_OK;
}
// Confirmed real: 0 args, returns the same kind of object createEffectChain
// does (same DAT_0043c3e0 type tag) -- a single persistent master chain,
// not a fresh one per call.
int GM_CDECL gmf_getMasterEffectChain(gmThread* a_thread) {
    static gmVariable s_master = gmVariable::s_null;
    if (s_master.m_type != GM_TABLE) {
        s_master = MakeEffectChainTable(*a_thread->GetMachine(), "master");
        gmTableObject* master = (gmTableObject*)a_thread->GetMachine()->GetObject(s_master.m_value.m_ref);
        master->Set(a_thread->GetMachine(), "__protected", gmVariable((gmint)1));
    }
    a_thread->Push(s_master);
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
int GM_CDECL gmf_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
// Audio manager constructor 0x00022528 stores 48 at +0x24; the retail
// getDefaultBaseNote wrapper 0x00084d28 returns that exact field.
int GM_CDECL gmf_getDefaultBaseNote(gmThread* a_thread) {
    a_thread->PushInt(48);
    return GM_OK;
}
// Audio.checkDiracPoolIntegrity() -- confirmed real: pushes type tag 1
// 0x00087f80), not the plain no-return gmf_noop this was previously mapped
// to. No real DSP pool exists here to corrupt, so "integrity ok" (true)
// is the honest default rather than a fabricated failure.
int GM_CDECL gmf_trueBool(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }

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
    // Retail wrappers 0x0008681c/0x000868d4 return integer milliseconds.
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

    // Confirmed real: createEffectChain/getMasterEffectChain are genuinely
    // flat Audio.* calls (this = the Audio singleton, standard pattern).
    // addEffect/addEffectChain/addVoice/freeAllChildEffect(Chains)/setParam/
    // setBypass all read "this" from a distinct +0x74 offset instead (see
    // MakeEffectChainTable/MakeEffectTable above) -- they're real methods on
    // the chain/effect objects those two return, not flat Audio.* calls, so
    // they're deliberately NOT registered here (a flat `Audio.addEffect(...)`
    // has zero real call sites -- confirmed by grep).
    {"createEffectChain", gmf_createEffectChain, nullptr},
    {"getMasterEffectChain", gmf_getMasterEffectChain, nullptr},
    {"freeEffectChain", gmf_freeEffectChain, nullptr},
    {"freeSoundEffect", gmf_freeSoundEffect, nullptr},
    {"onVoiceEndFreeEffectChain", gmf_onVoiceEndFreeEffectChain, nullptr},
    {"onVoiceEndFreeSoundEffect", gmf_onVoiceEndFreeSoundEffect, nullptr},

    // Script recording and save/load have live shipped callers (Main.gm
    // records audio while saving user levels and calls saveScriptToFile for
    // the .jef payload). The desktop FunctionObjectRecorder/SAVEREQUEST path
    // is not implemented yet, so these remain explicit no-op/false stubs;
    // they are known retail parity gaps, not unused retail APIs.
    {"startRecording", gmf_noop, nullptr},
    {"stopRecording", gmf_noop, nullptr},
    {"isRecording", gmf_falseBool, nullptr},
    {"startRecordingScript", gmf_noop, nullptr},
    {"stopRecordingScript", gmf_noop, nullptr},
    {"isRecordingScript", gmf_falseBool, nullptr},
    {"startPlayingScript", gmf_noop, nullptr},
    {"stopPlayingScript", gmf_noop, nullptr},
    {"isPlayingScript", gmf_falseBool, nullptr},
    {"saveScriptToFile", gmf_noop, nullptr},
    {"loadScriptFromFile", gmf_noop, nullptr},
    {"startRetriggering", gmf_noop, nullptr},
    {"stopRetriggering", gmf_noop, nullptr},
    {"isRetriggering", gmf_falseBool, nullptr},
    // Audio.c 0x00086ee4).
    {"getRetriggerTime", gmf_zeroInt, nullptr},
    {"setRetriggerTime", gmf_noop, nullptr},

    {"hack_setEnabled", gmf_hackSetEnabled, nullptr},
    {"setFrequencyMultiplier", gmf_setFrequencyMultiplier, nullptr},
    {"getFrequencyMultiplier", gmf_getFrequencyMultiplier, nullptr},
    {"getFreeMemory", gmf_zeroInt, nullptr},
    {"getDefaultBaseNote", gmf_getDefaultBaseNote, nullptr},
    {"getNumDiracDSPsFree", gmf_zeroInt, nullptr},
    {"getNumDiracDSPsUsed", gmf_zeroInt, nullptr},
    {"getNumDiracDSPChannelsUsed", gmf_zeroInt, nullptr},
    {"getNumDiracDSPs", gmf_zeroInt, nullptr},
    {"checkDiracPoolIntegrity", gmf_trueBool, nullptr},
};

} // namespace

void CleanupAudioManagerTrackedResources() {
    // Retail AudioManagerClearTrackedResources releases manager-owned resource
    // references without stopping active voices. Shared sample references held
    // by live voices therefore survive this clear.
    State().loadedSounds.clear();
    State().soundLengthMs.clear();
    g_loadedSoundSamples.clear();
    g_loadedSoundModes.clear();
    g_loadedSoundResourceKinds.clear();
    g_decodedSounds.clear();
}

int CreateVoiceNative(const std::string& a_path, float a_pan, float a_volume,
                      int a_note) {
    return CreateVoiceState(a_path, a_pan, a_volume, a_note);
}

void AdvanceAudioVoices(float a_deltaMs) {
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
        if (voice.paused) {
            ++it;
            continue;
        }
        bool gateTriggered = false;
        if (!voice.stopping && voice.gateDurationMs > 0.0) {
            voice.gateElapsedMs += a_deltaMs;
            if (voice.gateElapsedMs >= voice.gateDurationMs) {
                const double overshootMs = voice.gateElapsedMs -
                                           voice.gateDurationMs;
                StartVoiceFade(voice, 60.0);
                // FUN_00022c78 hard-resets the embedded gate Alarm after
                // scheduling its zero-duration stop (FUN_000c47e0).
                voice.gateElapsedMs = 0.0;
                voice.fadeRemainingMs = 60.0 - overshootMs;
                if (voice.fadeRemainingMs <= 0.0) {
                    it = voices.erase(it);
                    continue;
                }
                gateTriggered = true;
            }
        }
        if (voice.stopping && !gateTriggered) {
            if (voice.fadeDurationMs > 0.0) {
                voice.fadeRemainingMs -= a_deltaMs;
                if (voice.fadeRemainingMs <= 0.0) {
                    it = voices.erase(it);
                    continue;
                }
            } else {
                voice.stopRemainingMs -= a_deltaMs;
                if (voice.stopRemainingMs <= 0.0) {
                    it = voices.erase(it);
                    continue;
                }
            }
        }
        if (!std::isfinite(voice.speed)) {
            ++it;
            continue;
        }
        voice.positionMs += PlaybackAdvanceMs(voice, a_deltaMs);
        if (voice.speedRampRemainingMs > 0.0) {
            const double rampMs = std::min((double)a_deltaMs,
                                           voice.speedRampRemainingMs);
            const double fraction = rampMs / voice.speedRampRemainingMs;
            voice.speed += (float)((voice.targetSpeed - voice.speed) * fraction);
            voice.speedRampRemainingMs -= rampMs;
            if (voice.speedRampRemainingMs <= 0.0)
                voice.speed = voice.targetSpeed;
        }
        const int length = SoundLengthMs(voice.soundFile);
        // A missing or unsupported asset has no known end marker. Keep its
        // handle available for explicit stop, while still advancing its
        // script-visible cursor.
        if (length <= 0) { ++it; continue; }
        if (voice.positionMs >= 0.0 && voice.positionMs < length) {
            ++it;
            continue;
        }
        if (voice.looping || voice.loopCount > 1) {
            if (voice.loopCount > 1) --voice.loopCount;
            voice.positionMs = std::fmod(voice.positionMs, (double)length);
            if (voice.positionMs < 0.0) voice.positionMs += length;
            ++it;
        } else {
            it = voices.erase(it);
        }
    }
    ProcessDeferredEffectCleanup(State().machine,
                                 State().effectsWaitingForVoiceEnd, false);
    ProcessDeferredEffectCleanup(State().machine,
                                 State().effectChainsWaitingForVoiceEnd, true);
}

void InitializeAudioOutput() {
    if (std::getenv("GE3_HEADLESS") && !std::getenv("GE3_AUDIO_OUTPUT")) return;
    g_audioOutputStarted = g_audioFrontend && g_audioFrontend->StartAudioOutput(kOutputRate);
}

void ShutdownAudioOutput() {
    if (g_audioOutputStarted && g_audioFrontend) g_audioFrontend->StopAudioOutput();
    g_audioOutputStarted = false;
    g_decodedSounds.clear();
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
    g_audioFrontend = &a_frontend;
    State().machine = &a_machine;
    a_machine.RegisterLibrary(s_audioLib, sizeof(s_audioLib) / sizeof(s_audioLib[0]), "Audio");
}
