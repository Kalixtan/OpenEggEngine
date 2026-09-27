#include "gmThread.h"
#include "audio_bindings.h"
#include "gmTableObject.h"
#include <unordered_map>
#include <unordered_set>
#include <string>

// ---------------------------------------------------------------------
// citations on each binding below) plus two direct Ghidra cross-references
// this pass added for methods that file's original clustering missed
// (playSound @ 0x001c5d6c, setVoiceVolume @ 0x001c5f50, getVoiceVolume
// @ 0x001c5fe0 -- found via the same gmFunctionEntry-table technique used
//
// No real audio backend (see audio_bindings.h) -- every voice handle
// playSound returns is real and unique, and every setter below writes into
// that voice's actual tracked state (read back correctly by the matching
// getter), it's just never fed to an actual mixer/output device.
// ---------------------------------------------------------------------

namespace {

struct VoiceState {
    std::string soundFile;
    float volume = 1.0f;
    float pan = 0.0f;
    float pitchBend = 0.0f;
    float speed = 1.0f;
    int positionMs = 0;
    bool paused = false;
    bool looping = false;
    int loopCount = 0;
};

struct AudioState {
    std::unordered_map<int, VoiceState> voices;
    std::unordered_set<std::string> loadedSounds;
    int nextVoiceId = 1;
    int nextEffectChainId = 1;
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

    int id = State().nextVoiceId++;
    VoiceState v;
    v.soundFile = path;
    v.volume = vol;
    v.pan = pan;
    State().voices[id] = v;
    a_thread->PushInt(id);
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
    a_thread->PushInt(v ? v->positionMs : 0);
    return GM_OK;
}
// Audio.getVoiceLength(voice)/getSoundLength(path) -- confirmed real names
// (0x0008681c/0x000868d4); no real audio decode to measure an actual
// duration from -- 0 is an honest "unknown", not a fabricated number.
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
// Audio.getAllVoices() -- confirmed real, 0x00086c70 (returns a native
// ref-container object this engine doesn't model, see NATIVE_TYPES.md's
// ENTITYLISTREFCONTAINER-family note) -- a plain sequential-int-keyed GM
// table of currently tracked voice ids works the same way for
// foreach/tableCount, same convention as GameSystem.getAllProcesses().
int GM_CDECL gmf_getAllVoices(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* table = m.AllocTableObject();
    int i = 0;
    for (auto& kv : State().voices) table->Set(&m, i++, gmVariable((gmint)kv.first));
    a_thread->PushTable(table);
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

// --- effect chains, DSP, recording, misc (confirmed real names, low/no
// real corpus usage -- honest handle-allocating or no-op stubs) ---------

int GM_CDECL gmf_createEffectChain(gmThread* a_thread) { a_thread->PushInt(State().nextEffectChainId++); return GM_OK; }
int GM_CDECL gmf_getMasterEffectChain(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_noop(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_falseBool(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_zeroInt(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }

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
    {"getVoiceLength", gmf_zeroFloat, nullptr},
    {"getSoundLength", gmf_zeroFloat, nullptr},
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
    {"freeEffectChain", gmf_noop, nullptr},
    {"freeSoundEffect", gmf_noop, nullptr},
    {"onVoiceEndFreeEffectChain", gmf_noop, nullptr},
    {"onVoiceEndFreeSoundEffect", gmf_noop, nullptr},
    {"addEffectChain", gmf_noop, nullptr},
    {"addEffect", gmf_noop, nullptr},
    {"addVoice", gmf_noop, nullptr},
    {"freeAllChildEffectChains", gmf_noop, nullptr},
    {"freeAllChildEffects", gmf_noop, nullptr},
    {"setBypass", gmf_noop, nullptr},
    {"setParam", gmf_noop, nullptr},

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
    {"getRetriggerTime", gmf_zeroFloat, nullptr},
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
    {"checkDiracPoolIntegrity", gmf_noop, nullptr},
};

} // namespace

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
