#pragma once
#include "gmMachine.h"
#include "gmThread.h"
#include "../platform/platform_window.h"
#include <string>

void RegisterAudioBindings(gmMachine& a_machine, PlatformFrontend& a_frontend);

// Advance script-visible voice playback cursors once per engine frame.
void AdvanceAudioVoices(float a_deltaMs);
void InitializeAudioOutput();
void ShutdownAudioOutput();
// Mirror the manager-owned resource-list clear in retail
// AudioManagerClearTrackedResources. Active voices keep their own sample refs.
void CleanupAudioManagerTrackedResources();

// Native process-list teardown for Platformer_Main. Retail's level destructor
// calls Audio.stopAllVoices(), whose default fade is 60 ms; keep that policy
// available to the engine lifecycle path as well as the script binding.
void StopAllVoicesForProcessListReset();

void SetMasterVolumeNative(float a_vol);
float GetMasterVolumeNative();

// Shared voice allocation path for Audio.playSound and the EntityBase mixer
// wrappers. The frame mixer reads this same state for desktop output.
int CreateVoiceNative(const std::string& a_path, float a_pan, float a_volume,
                      int a_note = 96);

int GM_CDECL PlaySoundNative(gmThread* a_thread);
