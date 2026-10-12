#pragma once
#include "gmMachine.h"
#include "gmThread.h"
#include "../platform/platform_window.h"
#include <string>

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
// ones' conventions. The frontend decodes WAVs and outputs the mixed voices.
// VAG paths use a WAV sibling when one exists. Effects and exact
// retail mixer behavior remain open. The voice-handle registry is backed
// by actual per-voice state (volume/pan/pitch/loop/
// position/pause), so isVoiceValid/stopVoice/setVoiceVolume/etc. operate
// on tracked state consistent with what playSound handed back. Voice
// positions and known-duration one-shots advance on the engine frame clock.
// Timed gates use the retail default stop fade; exact envelope curves,
// effects, and precise lifetime parity remain open.
void RegisterAudioBindings(gmMachine& a_machine, PlatformFrontend& a_frontend);

// Advance script-visible voice playback cursors once per engine frame.
void AdvanceAudioVoices(float a_deltaMs);
void InitializeAudioOutput();
void ShutdownAudioOutput();
// Mirror the manager-owned resource-list clear in retail
// AudioManagerClearTrackedResources. Active voices keep their own sample refs.
void CleanupAudioManagerTrackedResources();

// For process_handle_bindings.cpp's Process.setGlobalVolume -- confirmed
// real call site (PauseProcessBase.gm: `this.gameProcess.setGlobalVolume(gv)`)
// but a process-handle method, not an Audio.* one; delegates to the same
// master-volume state Audio.setMasterVolume/getMasterVolume already track,
// which is the only real "volume" state this engine has and matches the
// name's obvious intent.
void SetMasterVolumeNative(float a_vol);
float GetMasterVolumeNative();

// Shared voice allocation path for Audio.playSound and the EntityBase mixer
// wrappers. The frame mixer reads this same state for desktop output.
int CreateVoiceNative(const std::string& a_path, float a_pan, float a_volume,
                      int a_note = 96);

// For entity_bindings.cpp's per-entity EntityBase.playSound (confirmed
// real via direct Ghidra cross-reference, 0x001c5d6c-adjacent -- distinct
// address from Audio.playSound but byte-for-byte the same (path, note,
// gate, pan, vol) arg shape and same voice-handle-allocation contract) --
// reused directly rather than re-implemented, since it never reads
// ThisTable() at all.
int GM_CDECL PlaySoundNative(gmThread* a_thread);
