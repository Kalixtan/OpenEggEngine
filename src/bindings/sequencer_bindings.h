#pragma once
#include "gmMachine.h"

void RegisterSequencerBindings(gmMachine& a_machine);

// Advances every live SEQUENCER instance's playback clock and fires any
// triggers crossed since the last call. Call once per frame alongside
// GmVm::Tick.
void UpdateAllSequencers(gmMachine& a_machine, float a_deltaMs);

// Creates a new SEQUENCER instance the same way the real SEQUENCER()
// constructor does, without going through GM script -- used to create the
// real MainSequencer global from main.cpp's bootstrap.
gmVariable CreateSequencerInstance(gmMachine& a_machine, int a_initialBpm);

// Cached period used by retail MainSequencer. EntityRegistration's native
// music callback retimes its ground pulse from this value every eighth step.
float GetMainSequencerMsPerStep(gmMachine& a_machine);

// Stops the bootstrapped MainSequencer when the native process manager retires
// Platformer_Main, matching that level process's retail teardown call.
void StopMainSequencerForProcessListReset(gmMachine& a_machine);
