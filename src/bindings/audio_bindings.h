#pragma once
#include "gmMachine.h"
#include "gmThread.h"

// covering most of the corpus's real call sites -- see audio_bindings.cpp
// for the address-by-address evidence). Used 313 times across the real
// corpus; Platformer/MainMenu5.gm's very first two statements are
// `Audio.loadSound(...)` calls.
//
// Argument counts, argument types, and return types below are decompiled-
// cross-reference, for the handful of methods that file's clustering
// missed -- playSound/setVoiceVolume/getVoiceVolume) has a body; a small
// remainder (methods with zero real corpus call sites, per an exhaustive
// grep) fall back to a plausible-typed default matching the confirmed
// ones' conventions. No real audio *output* backend exists yet (no WAV
// decode, no mixing, no SDL_mixer/OpenAL) -- that's a separate, large
// undertaking outside a script-compatibility pass. What IS real: a voice-
// handle registry backed by actual per-voice state (volume/pan/pitch/loop/
// position/pause), so isVoiceValid/stopVoice/setVoiceVolume/etc. all
// operate on genuine tracked state consistent with what playSound handed
// back, rather than a fixed dummy value -- silent, but internally
// consistent and script-observable exactly as retail's contract requires.
void RegisterAudioBindings(gmMachine& a_machine);

// For process_handle_bindings.cpp's Process.setGlobalVolume -- confirmed
// real call site (PauseProcessBase.gm: `this.gameProcess.setGlobalVolume(gv)`)
// but a process-handle method, not an Audio.* one; delegates to the same
// master-volume state Audio.setMasterVolume/getMasterVolume already track,
// which is the only real "volume" state this engine has and matches the
// name's obvious intent.
void SetMasterVolumeNative(float a_vol);
float GetMasterVolumeNative();

// For entity_bindings.cpp's per-entity EntityBase.playSound (confirmed
// real via direct Ghidra cross-reference, 0x001c5d6c-adjacent -- distinct
// address from Audio.playSound but byte-for-byte the same (path, note,
// gate, pan, vol) arg shape and same voice-handle-allocation contract) --
// reused directly rather than re-implemented, since it never reads
// ThisTable() at all.
int GM_CDECL PlaySoundNative(gmThread* a_thread);
