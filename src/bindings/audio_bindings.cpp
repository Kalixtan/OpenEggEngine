#include "gmThread.h"
#include "audio_bindings.h"
#include "gmTableObject.h"
#include "../core/engine_context.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdio>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <optional>
#include <vector>
#include <SDL2/SDL.h>

// ---------------------------------------------------------------------
// citations on each binding below) plus two direct Ghidra cross-references
// this pass added for methods that file's original clustering missed
// (playSound @ 0x001c5d6c, setVoiceVolume @ 0x001c5f50, getVoiceVolume
// @ 0x001c5fe0 -- found via the same gmFunctionEntry-table technique used
//
// SDL's queued audio device now plays decoded WAV samples from active voice
// handles. Retail effects, gate shaping, priority, and exact mixer routing
// remain to be ported; the script-facing voice registry stays authoritative.
// ---------------------------------------------------------------------

namespace {

constexpr int kOutputRate = 44100;
struct DecodedSound {
    std::vector<float> stereo;
};
SDL_AudioDeviceID g_audioDevice = 0;
double g_fractionalOutputFrames = 0.0;
std::unordered_map<std::string, std::weak_ptr<DecodedSound>> g_decodedSounds;

struct VoiceState {
    std::string soundFile;
    float volume = 1.0f;
    float pan = 0.0f;
    float pitchBend = 0.0f;
    float speed = 1.0f;
    double positionMs = 0.0;
    bool paused = false;
    bool looping = false;
    int loopCount = 0;
    std::shared_ptr<DecodedSound> sample;
    bool sampleLoadAttempted = false;
};

struct AudioState {
    std::unordered_map<int, VoiceState> voices;
    std::unordered_set<std::string> loadedSounds;
    std::unordered_map<std::string, int> soundLengthMs;
    int nextVoiceId = 1;
    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
};
AudioState& State() {
    static AudioState s;
    return s;
}

VoiceState* FindVoice(int a_id) {
    auto it = State().voices.find(a_id);
    return it == State().voices.end() ? nullptr : &it->second;
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
        // Most shipped Sony VAG assets also have a RIFF sibling. SDL can
        // decode that sibling without carrying a second ADPCM decoder.
        std::string wavePath = a_path.substr(0, a_path.size() - 4) + ".wav";
        resolved = files->Resolve(wavePath);
    }
    if (!resolved) resolved = files->Resolve(a_path);
    if (!resolved) return {};
    SDL_AudioSpec source{};
    Uint8* bytes = nullptr;
    Uint32 byteCount = 0;
    if (!SDL_LoadWAV(resolved->c_str(), &source, &bytes, &byteCount)) return {};
    SDL_AudioCVT cvt{};
    if (SDL_BuildAudioCVT(&cvt, source.format, source.channels, source.freq,
                          AUDIO_F32SYS, 2, kOutputRate) < 0) {
        SDL_FreeWAV(bytes);
        return {};
    }
    if (byteCount > (size_t)std::numeric_limits<int>::max() / (size_t)cvt.len_mult) {
        SDL_FreeWAV(bytes);
        return {};
    }
    cvt.len = (int)byteCount;
    cvt.buf = (Uint8*)SDL_malloc((size_t)cvt.len * cvt.len_mult);
    if (!cvt.buf) { SDL_FreeWAV(bytes); return {}; }
    std::memcpy(cvt.buf, bytes, byteCount);
    SDL_FreeWAV(bytes);
    if (SDL_ConvertAudio(&cvt) != 0) { SDL_free(cvt.buf); return {}; }
    auto sample = std::make_shared<DecodedSound>();
    const float* floats = (const float*)cvt.buf;
    sample->stereo.assign(floats, floats + cvt.len_cvt / sizeof(float));
    SDL_free(cvt.buf);
    g_decodedSounds[a_path] = sample;
    return sample;
}

void QueueMixedVoices(float a_deltaMs) {
    if (!g_audioDevice) return;
    const double wanted = a_deltaMs * kOutputRate / 1000.0 + g_fractionalOutputFrames;
    int frameCount = (int)std::min(8192.0, std::floor(wanted));
    g_fractionalOutputFrames = wanted - std::floor(wanted);
    if (frameCount <= 0) return;
    std::vector<float> mix((size_t)frameCount * 2, 0.0f);
    const float master = State().masterVolume;
    for (auto& entry : State().voices) {
        VoiceState& voice = entry.second;
        if (voice.paused || voice.speed == 0.0f || !std::isfinite(voice.speed)) continue;
        if (!voice.sampleLoadAttempted) {
            voice.sample = DecodeSound(voice.soundFile);
            voice.sampleLoadAttempted = true;
        }
        if (!voice.sample || voice.sample->stereo.size() < 2) continue;
        const size_t soundFrames = voice.sample->stereo.size() / 2;
        const float pan = std::clamp(voice.pan, -1.0f, 1.0f);
        const float gain = std::max(0.0f, voice.volume * master);
        const float leftGain = gain * (pan > 0.0f ? 1.0f - pan : 1.0f);
        const float rightGain = gain * (pan < 0.0f ? 1.0f + pan : 1.0f);
        for (int frame = 0; frame < frameCount; ++frame) {
            const double positionMs = voice.positionMs +
                (double)frame * 1000.0 / kOutputRate * voice.speed;
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
                 voice.sample->stereo[next * 2] * blend) * leftGain;
            mix[(size_t)frame * 2 + 1] +=
                (voice.sample->stereo[i * 2 + 1] * (1.0f - blend) +
                 voice.sample->stereo[next * 2 + 1] * blend) * rightGain;
        }
    }
    for (float& sample : mix) sample = std::clamp(sample, -1.0f, 1.0f);
    const Uint32 maxQueuedBytes = (Uint32)(kOutputRate * 2 * sizeof(float) / 4);
    if (SDL_GetQueuedAudioSize(g_audioDevice) > maxQueuedBytes)
        SDL_ClearQueuedAudio(g_audioDevice);
    SDL_QueueAudio(g_audioDevice, mix.data(), (Uint32)(mix.size() * sizeof(float)));
}

int CreateVoiceState(const std::string& a_path, float a_pan, float a_volume) {
    const int id = State().nextVoiceId++;
    VoiceState voice;
    voice.soundFile = a_path;
    voice.volume = a_volume;
    voice.pan = a_pan;
    State().voices[id] = std::move(voice);
    return id;
}

// --- loading ---------------------------------------------------------

// @ 0x00084984: 1 or 2 args (path, [area]); returns a BOOLEAN success flag
// (the decompiled body computes it via `LZCOUNT(handle)>>5^1`, the
// compiler's branchless "is this pointer non-null" idiom -- i.e. the
// return value is success/failure, NOT a handle to keep). Real handles
// only ever come back from playSound.
int GM_CDECL gmf_loadSound(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    if (a_thread->GetNumParams() >= 2) { GM_INT_PARAM(area, 1, 0); (void)area; }
    State().loadedSounds.insert(path);
    a_thread->PushInt(1);
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

// Audio.playSound(path, note, gate, pan, vol, [.., ..]) -- confirmed real
// via direct Ghidra cross-reference (0x001c5d6c, not originally clustered
// into Audio.c): accepts 5-7 args, only the first 5 are ever read
// (note kept as a raw int -- MIDI-style base note -- gate/pan/vol as
// floats); returns an INT voice handle (return slot type=1). Most real
// call sites do pass all 5 (`Audio.playSound(this.soundfile,this.note,
// this.gate,this.pan,this.vol)`, SequencerHelper.gm) -- but
// Platformer/MainMenu5/HomeScreenBase.gm calls it with only 3
// (`Audio.playSound(this.loopPath,0,(1.0-winVol)*this.volume)`, several
// call sites around line 277-314), so the original "matches every real
// call site" claim in this comment was wrong: pan/vol must have real
// defaults, not a hard 5-arg minimum. A strict GM_CHECK_NUM_PARAMS(5) here
// threw on every one of those (an uncaught thread exception whose blank-
// message form -- gmThread.h's GM_INT_PARAM etc. return bare GM_EXCEPTION
// with no GM_EXCEPTION_MSG -- made it easy to miss in logs), aborting
// HomeScreenBase's onMusicTrigger() every single time the main menu's
// music loop tried to (re)start.
int GM_CDECL gmf_playSound(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    GM_INT_PARAM(note, 1, 0); (void)note;
    float gate = a_thread->ParamFloatOrInt(2, 0.0f); (void)gate;
    float pan = a_thread->ParamFloatOrInt(3, 0.0f);
    float vol = a_thread->ParamFloatOrInt(4, 1.0f);

    a_thread->PushInt(CreateVoiceState(path, pan, vol));
    return GM_OK;
}
// Audio.playSoundSlicker/spawnSoundSlicker/playStream -- confirmed real
// but distinct helper natives (0x00084dc8/0x00085008/0x00085940) layered
// on top of the same playback path with slicing/pitch-shift/streaming
// params this pass hasn't traced in detail (a handful of real call sites,
// mostly dev tooling) -- delegate to playSound's handle-allocation
// contract so a real voice handle still comes back, matching the "handle
// you can call stopVoice/isVoiceValid on" contract every consumer
// actually depends on.
int GM_CDECL gmf_spawnOrPlaySoundSlicker(gmThread* a_thread) {
    int id = State().nextVoiceId++;
    VoiceState v;
    if (a_thread->GetNumParams() >= 1 && a_thread->Param(0).m_type == GM_STRING) {
        char buf[256];
        v.soundFile = a_thread->Param(0).AsString(a_thread->GetMachine(), buf, sizeof(buf));
    }
    State().voices[id] = v;
    a_thread->PushInt(id);
    return GM_OK;
}
// Audio.loadSoundSlicker() -- confirmed real body is LITERALLY `loadSound();
// return;` in retail (0x00084da8) -- forwards to loadSound with NO
// arguments (hits loadSound's own 0-arg exception branch and does
// nothing useful), a genuine retail no-op/dead-code call, not a gap here.
int GM_CDECL gmf_loadSoundSlicker(gmThread* a_thread) { (void)a_thread; return GM_OK; }

// --- voice control -------------------------------------------------------

// Audio.stopVoice(voice, [fadeMs]) -- confirmed real, 0x00086b20: 1-2 args.
int GM_CDECL gmf_stopVoice(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    if (a_thread->GetNumParams() >= 2) { GM_FLOAT_OR_INT_PARAM(fadeMs, 1, 0.0f); (void)fadeMs; }
    State().voices.erase(voice);
    return GM_OK;
}
// Audio.stopAllVoices() -- confirmed real, 0x00086c20: 0 args.
int GM_CDECL gmf_stopAllVoices(gmThread* a_thread) {
    (void)a_thread;
    State().voices.clear();
    return GM_OK;
}
// Audio.fadeOutVoice(voice, fadeMs) -- confirmed real by usage (name +
// corpus shape); no real call site fades and re-queries a specific voice's
// volume afterward, so an immediate stop satisfies every observed use.
int GM_CDECL gmf_fadeOutVoice(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    State().voices.erase(voice);
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
// Audio.setVoiceVolume(voice, volume)/getVoiceVolume(voice) -- confirmed
// real via direct Ghidra cross-reference (0x001c5f50/0x001c5fe0): setter
// reads a float volume argument; getter returns a float.
int GM_CDECL gmf_setVoiceVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_FLOAT_OR_INT_PARAM(vol, 1, 1.0f);
    if (VoiceState* v = FindVoice(voice)) v->volume = vol;
    return GM_OK;
}
int GM_CDECL gmf_getVoiceVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    VoiceState* v = FindVoice(voice);
    a_thread->PushFloat(v ? v->volume : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_setVoicePan(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_FLOAT_OR_INT_PARAM(pan, 1, 0.0f);
    if (VoiceState* v = FindVoice(voice)) v->pan = pan;
    return GM_OK;
}
// Audio.setVoicePitchBend(voice, bend) -- confirmed real, 0x00086354.
int GM_CDECL gmf_setVoicePitchBend(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_FLOAT_OR_INT_PARAM(bend, 1, 0.0f);
    if (VoiceState* v = FindVoice(voice)) v->pitchBend = bend;
    return GM_OK;
}
// Audio.setVoiceGateTime/setVoiceNote -- confirmed real names, real corpus
// shape (voice, value), no return -- same tracked-but-unplayed pattern as
// pitchBend above; not surfaced through a dedicated getter anywhere in the
// corpus, so no VoiceState field needed beyond accepting the call.
int GM_CDECL gmf_setVoiceIntParamNoop(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    return GM_OK;
}
// Audio.setVoiceSpeed(voice, speed)/getVoiceSpeed(voice) -- confirmed real,
// 0x00086550/0x00086620.
int GM_CDECL gmf_setVoiceSpeed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(voice, 0, 0);
    GM_FLOAT_OR_INT_PARAM(speed, 1, 1.0f);
    if (VoiceState* v = FindVoice(voice)) v->speed = speed;
    return GM_OK;
}
int GM_CDECL gmf_getVoiceSpeed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(voice, 0, 0);
    VoiceState* v = FindVoice(voice);
    a_thread->PushFloat(v ? v->speed : 1.0f);
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
// Audio.getFrequencyMultiplier() -- no real audio decode/mixer to read an
// actual value from -- 0 is an honest "unknown", not a fabricated number.
int GM_CDECL gmf_zeroFloat(gmThread* a_thread) { a_thread->PushFloat(0.0f); return GM_OK; }
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
    State().masterVolume = vol;
    return GM_OK;
}
int GM_CDECL gmf_getMasterVolume(gmThread* a_thread) { a_thread->PushFloat(State().masterVolume); return GM_OK; }
int GM_CDECL gmf_setMusicVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(vol, 0, 1.0f);
    State().musicVolume = vol;
    return GM_OK;
}
int GM_CDECL gmf_getMusicVolume(gmThread* a_thread) { a_thread->PushFloat(State().musicVolume); return GM_OK; }
int GM_CDECL gmf_setSFXVolume(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(vol, 0, 1.0f);
    State().sfxVolume = vol;
    return GM_OK;
}
int GM_CDECL gmf_getSFXVolume(gmThread* a_thread) { a_thread->PushFloat(State().sfxVolume); return GM_OK; }

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
// these are genuine GM table objects carrying real per-effect state (so a
// later getter would see exactly what was set), just not wired to any
// actual audio processing -- same "real state, no output" contract as
// VoiceState above.
gmVariable MakeEffectTable(gmMachine& a_machine, int a_effectType) {
    gmTableObject* t = a_machine.AllocTableObject();
    t->Set(&a_machine, "__type", gmVariable((gmint)a_effectType));
    t->Set(&a_machine, "__bypass", gmVariable((gmint)0));
    t->Set(&a_machine, "setBypass", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(b, 0, 0);
            self->Set(a_thread->GetMachine(), "__bypass", gmVariable((gmint)(b != 0)));
            return GM_OK;
        })));
    // addEffect applies these same (id, value) pairs immediately at
    // creation via a per-effect-type dispatch this pass hasn't traced (no
    // real DSP backend to apply them to); stored keyed by id so a repeat
    // setParam(id, value) overwrites, matching that dispatch's shape.
    t->Set(&a_machine, "setParam", gmVariable(a_machine.AllocFunctionObject(
        [](gmThread* a_thread) -> int {
            gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
            GM_CHECK_NUM_PARAMS(2); GM_INT_PARAM(paramId, 0, 0);
            char key[32];
            snprintf(key, sizeof(key), "__p%d", paramId);
            self->Set(a_thread->GetMachine(), key, a_thread->Param(1));
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
    (void)self;
    int n = a_thread->GetNumParams();
    if (n < 1) { a_thread->PushNull(); return GM_OK; }
    GM_INT_PARAM(effectType, 0, 0);
    gmVariable effectVar = MakeEffectTable(*a_thread->GetMachine(), effectType);
    gmTableObject* effect = (gmTableObject*)a_thread->GetMachine()->GetObject(effectVar.m_value.m_ref);
    for (int i = 1; i + 1 < n; i += 2) {
        int paramId = a_thread->ParamInt(i, 0);
        char key[32];
        snprintf(key, sizeof(key), "__p%d", paramId);
        effect->Set(a_thread->GetMachine(), key, a_thread->Param(i + 1));
    }
    a_thread->Push(effectVar);
    return GM_OK;
}
// Confirmed real names/0-arg method-call shape (Main.gm's
// `Audio.getMasterEffectChain().freeAllChildEffects();`); no real per-chain
// child tracking is needed since nothing in the corpus reads it back after.
int GM_CDECL gmf_chain_freeAllChildEffects(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    (void)self;
    return GM_OK;
}
int GM_CDECL gmf_chain_freeAllChildEffectChains(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    (void)self;
    return GM_OK;
}
// Confirmed real names/1-arg method-call shape (same +0x74 "this" pattern
// as addEffect/freeAllChildEffect(Chains) above) but zero real corpus call
// sites for either -- honest no-ops with the confirmed arity, not a
// fabricated nested-chain/voice-routing pipeline.
int GM_CDECL gmf_chain_addEffectChain(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    (void)self;
    return GM_OK;
}
int GM_CDECL gmf_chain_addVoice(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable(); if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    (void)self;
    return GM_OK;
}
gmVariable MakeEffectChainTable(gmMachine& a_machine, const std::string& a_name) {
    gmTableObject* t = a_machine.AllocTableObject();
    t->Set(&a_machine, "__name", gmVariable(a_machine.AllocStringObject(a_name.c_str())));
    t->Set(&a_machine, "addEffect", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addEffect)));
    t->Set(&a_machine, "freeAllChildEffects", gmVariable(a_machine.AllocFunctionObject(gmf_chain_freeAllChildEffects)));
    t->Set(&a_machine, "freeAllChildEffectChains", gmVariable(a_machine.AllocFunctionObject(gmf_chain_freeAllChildEffectChains)));
    t->Set(&a_machine, "addEffectChain", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addEffectChain)));
    t->Set(&a_machine, "addVoice", gmVariable(a_machine.AllocFunctionObject(gmf_chain_addVoice)));
    return gmVariable(t);
}
int GM_CDECL gmf_createEffectChain(gmThread* a_thread) {
    std::string name;
    if (a_thread->GetNumParams() >= 1 && a_thread->Param(0).m_type == GM_STRING) {
        char buf[256];
        name = a_thread->Param(0).AsString(a_thread->GetMachine(), buf, sizeof(buf));
    }
    a_thread->Push(MakeEffectChainTable(*a_thread->GetMachine(), name));
    return GM_OK;
}
// Confirmed real: 0 args, returns the same kind of object createEffectChain
// does (same DAT_0043c3e0 type tag) -- a single persistent master chain,
// not a fresh one per call.
int GM_CDECL gmf_getMasterEffectChain(gmThread* a_thread) {
    static gmVariable s_master = gmVariable::s_null;
    if (s_master.m_type != GM_TABLE) s_master = MakeEffectChainTable(*a_thread->GetMachine(), "master");
    a_thread->Push(s_master);
    return GM_OK;
}
int GM_CDECL gmf_noop(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_falseBool(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
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
    {"playSoundSlicker", gmf_spawnOrPlaySoundSlicker, nullptr},
    {"spawnSoundSlicker", gmf_spawnOrPlaySoundSlicker, nullptr},
    {"loadSoundSlicker", gmf_loadSoundSlicker, nullptr},
    {"playStream", gmf_spawnOrPlaySoundSlicker, nullptr},

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
    {"setVoiceGateTime", gmf_setVoiceIntParamNoop, nullptr},
    {"setVoiceNote", gmf_setVoiceIntParamNoop, nullptr},
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
    {"freeEffectChain", gmf_noop, nullptr},
    {"freeSoundEffect", gmf_noop, nullptr},
    {"onVoiceEndFreeEffectChain", gmf_noop, nullptr},
    {"onVoiceEndFreeSoundEffect", gmf_noop, nullptr},

    // Recording/scripting/retriggering -- confirmed real names, no real
    // corpus call sites that read back a result (grepped): honest
    // false/no-op rather than a fabricated recording pipeline.
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

    {"hack_setEnabled", gmf_noop, nullptr},
    {"setFrequencyMultiplier", gmf_noop, nullptr},
    {"getFrequencyMultiplier", gmf_zeroFloat, nullptr},
    {"getFreeMemory", gmf_zeroInt, nullptr},
    {"getDefaultBaseNote", gmf_zeroInt, nullptr},
    {"getNumDiracDSPsFree", gmf_zeroInt, nullptr},
    {"getNumDiracDSPsUsed", gmf_zeroInt, nullptr},
    {"getNumDiracDSPChannelsUsed", gmf_zeroInt, nullptr},
    {"getNumDiracDSPs", gmf_zeroInt, nullptr},
    {"checkDiracPoolIntegrity", gmf_trueBool, nullptr},
};

} // namespace

int CreateVoiceNative(const std::string& a_path, float a_pan, float a_volume) {
    return CreateVoiceState(a_path, a_pan, a_volume);
}

void AdvanceAudioVoices(float a_deltaMs) {
    if (!(a_deltaMs > 0.0f) || !std::isfinite(a_deltaMs)) return;
    QueueMixedVoices(a_deltaMs);
    auto& voices = State().voices;
    for (auto it = voices.begin(); it != voices.end();) {
        VoiceState& voice = it->second;
        if (voice.paused || !std::isfinite(voice.speed) || voice.speed == 0.0f) {
            ++it;
            continue;
        }
        voice.positionMs += (double)a_deltaMs * voice.speed;
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
}

void InitializeAudioOutput() {
    if (std::getenv("GE3_HEADLESS") && !std::getenv("GE3_AUDIO_OUTPUT")) return;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "Audio output unavailable: %s\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec wanted{};
    wanted.freq = kOutputRate;
    wanted.format = AUDIO_F32SYS;
    wanted.channels = 2;
    wanted.samples = 1024;
    g_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
    if (!g_audioDevice) {
        std::fprintf(stderr, "Audio output unavailable: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }
    SDL_PauseAudioDevice(g_audioDevice, 0);
}

void ShutdownAudioOutput() {
    if (g_audioDevice) {
        SDL_CloseAudioDevice(g_audioDevice);
        g_audioDevice = 0;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
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

void RegisterAudioBindings(gmMachine& a_machine) {
    a_machine.RegisterLibrary(s_audioLib, sizeof(s_audioLib) / sizeof(s_audioLib[0]), "Audio");
}
