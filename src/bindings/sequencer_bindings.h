#pragma once
#include "gmMachine.h"

// SEQUENCERACTION (three own methods plus inherited field operators; see
// A real, functional step sequencer (real BPM-driven timing, triggers that
// actually fire during playback), not just stubs -- MainSequencer (the
// real retail global that game_data/Main.gm drives its whole music/gameplay
// loop through) is created from this and set as a global before Main.gm
// loads, since no .gm script anywhere creates it -- confirmed by grepping
// the whole corpus for "MainSequencer =", zero hits. It has to come from
// the native engine bootstrap, exactly like retail's must.
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
