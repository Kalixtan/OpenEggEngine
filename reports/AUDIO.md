# Audio test report

Generated: 2026-10-22 11:04 UTC

Matched: **132/132 (100%)** comparable script readings; 4 skipped after retail setup failures; 24 clock-dependent readings excluded. Effect-chain calls without a getter only check completion. This does not test emitted audio or DSP parity.

Asset probes: tested with user supplied sound. Functions with readings: **61/68**.

## Retail setup warnings

- **Warning:** Retail Slicker setup failed: RPCS3 logged an FMOD invalid-seek error. The captured play call returned 0 with voice volume 0; these readings do not describe successful Slicker playback.
- **Warning:** Retail playStream setup failed for the tested WAV: preload returned 1, but playStream returned 0 and no voice length. Playback parity cannot be judged from this failed setup.

## Per function

| Function | Matched | Compared | Setup skip | Clock | Status |
| --- | ---: | ---: | ---: | ---: | --- |
| `Audio.checkDiracPoolIntegrity` | 2 | 2 | 0 | 0 | PASS |
| `Audio.createEffectChain` | 2 | 2 | 0 | 0 | PASS |
| `Audio.fadeOutVoice` | 3 | 3 | 0 | 0 | PASS |
| `Audio.freeEffectChain` | 1 | 1 | 0 | 0 | PASS |
| `Audio.freeSoundEffect` | 1 | 1 | 0 | 0 | PASS |
| `Audio.getAllVoices` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getDefaultBaseNote` | 1 | 1 | 0 | 0 | PASS |
| `Audio.getFreeMemory` | 0 | 0 | 0 | 0 | untested |
| `Audio.getFrequencyMultiplier` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getMasterEffectChain` | 1 | 1 | 0 | 0 | PASS |
| `Audio.getMasterVolume` | 4 | 4 | 0 | 0 | PASS |
| `Audio.getMusicVolume` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getNumDiracDSPChannelsUsed` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getNumDiracDSPs` | 1 | 1 | 0 | 0 | PASS |
| `Audio.getNumDiracDSPsFree` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getNumDiracDSPsUsed` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getRetriggerTime` | 1 | 1 | 0 | 0 | PASS |
| `Audio.getSFXVolume` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getSoundLength` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getVoiceLength` | 2 | 2 | 0 | 0 | PASS |
| `Audio.getVoicePosition` | 4 | 4 | 0 | 1 | PASS |
| `Audio.getVoiceSpeed` | 5 | 5 | 0 | 0 | PASS |
| `Audio.getVoiceVolume` | 4 | 4 | 0 | 0 | PASS |
| `Audio.hack_setEnabled` | 1 | 1 | 0 | 0 | PASS |
| `Audio.isPlayingScript` | 1 | 1 | 0 | 0 | PASS |
| `Audio.isRecording` | 1 | 1 | 0 | 0 | PASS |
| `Audio.isRecordingScript` | 1 | 1 | 0 | 0 | PASS |
| `Audio.isRetriggering` | 1 | 1 | 0 | 0 | PASS |
| `Audio.isVoicePaused` | 4 | 4 | 0 | 0 | PASS |
| `Audio.isVoiceValid` | 5 | 5 | 0 | 0 | PASS |
| `Audio.loadSampleList` | 0 | 0 | 0 | 0 | untested |
| `Audio.loadScriptFromFile` | 0 | 0 | 0 | 0 | untested |
| `Audio.loadSound` | 2 | 2 | 0 | 0 | PASS |
| `Audio.loadSoundSlicker` | 2 | 2 | 0 | 0 | PASS |
| `Audio.onVoiceEndFreeEffectChain` | 2 | 2 | 0 | 0 | PASS |
| `Audio.onVoiceEndFreeSoundEffect` | 0 | 0 | 0 | 0 | untested |
| `Audio.playSound` | 7 | 7 | 0 | 11 | PASS |
| `Audio.playSoundSlicker` | 0 | 0 | 2 | 0 | SKIP |
| `Audio.playStream` | 2 | 2 | 2 | 0 | PASS |
| `Audio.saveScriptToFile` | 0 | 0 | 0 | 0 | untested |
| `Audio.setFrequencyMultiplier` | 1 | 1 | 0 | 0 | PASS |
| `Audio.setMasterVolume` | 3 | 3 | 0 | 0 | PASS |
| `Audio.setMusicVolume` | 1 | 1 | 0 | 0 | PASS |
| `Audio.setRetriggerTime` | 1 | 1 | 0 | 0 | PASS |
| `Audio.setSFXVolume` | 1 | 1 | 0 | 0 | PASS |
| `Audio.setVoiceGateTime` | 7 | 7 | 0 | 0 | PASS |
| `Audio.setVoiceLoop` | 1 | 1 | 0 | 0 | PASS |
| `Audio.setVoiceLoopCount` | 2 | 2 | 0 | 12 | PASS |
| `Audio.setVoiceNote` | 1 | 1 | 0 | 0 | PASS |
| `Audio.setVoicePan` | 0 | 0 | 0 | 0 | untested |
| `Audio.setVoicePause` | 4 | 4 | 0 | 0 | PASS |
| `Audio.setVoicePitchBend` | 3 | 3 | 0 | 0 | PASS |
| `Audio.setVoicePosition` | 1 | 1 | 0 | 0 | PASS |
| `Audio.setVoiceSpeed` | 2 | 2 | 0 | 0 | PASS |
| `Audio.setVoiceVolume` | 3 | 3 | 0 | 0 | PASS |
| `Audio.spawnSoundSlicker` | 1 | 1 | 0 | 0 | PASS |
| `Audio.startPlayingScript` | 1 | 1 | 0 | 0 | PASS |
| `Audio.startRecording` | 1 | 1 | 0 | 0 | PASS |
| `Audio.startRecordingScript` | 1 | 1 | 0 | 0 | PASS |
| `Audio.startRetriggering` | 1 | 1 | 0 | 0 | PASS |
| `Audio.stopAllVoices` | 5 | 5 | 0 | 0 | PASS |
| `Audio.stopPlayingScript` | 1 | 1 | 0 | 0 | PASS |
| `Audio.stopRecording` | 1 | 1 | 0 | 0 | PASS |
| `Audio.stopRecordingScript` | 1 | 1 | 0 | 0 | PASS |
| `Audio.stopRetriggering` | 1 | 1 | 0 | 0 | PASS |
| `Audio.stopVoice` | 7 | 7 | 0 | 0 | PASS |
| `Audio.unloadAllSounds` | 0 | 0 | 0 | 0 | untested |
| `Audio.unloadSound` | 3 | 3 | 0 | 0 | PASS |

## Readings

| Function | Case | Retail | OpenEgg | Result |
| --- | --- | --- | --- | --- |
| `Audio.getMasterVolume` | Audio.getMasterVolume.default | `1.000000` | `1.000000` | PASS |
| `Audio.getMusicVolume` | Audio.getMusicVolume.default | `1.000000` | `1.000000` | PASS |
| `Audio.getSFXVolume` | Audio.getSFXVolume.default | `1.000000` | `1.000000` | PASS |
| `Audio.getFrequencyMultiplier` | Audio.getFrequencyMultiplier.default | `1.000000` | `1.000000` | PASS |
| `Audio.getDefaultBaseNote` | Audio.getDefaultBaseNote.default | `48` | `48` | PASS |
| `Audio.getNumDiracDSPs` | Audio.getNumDiracDSPs.default | `7` | `7` | PASS |
| `Audio.getNumDiracDSPsFree` | Audio.getNumDiracDSPsFree.default | `7` | `7` | PASS |
| `Audio.getNumDiracDSPsUsed` | Audio.getNumDiracDSPsUsed.default | `0` | `0` | PASS |
| `Audio.getNumDiracDSPChannelsUsed` | Audio.getNumDiracDSPChannelsUsed.default | `0` | `0` | PASS |
| `Audio.checkDiracPoolIntegrity` | Audio.checkDiracPoolIntegrity.default | `1` | `1` | PASS |
| `Audio.getMasterVolume` | Audio.getMasterVolume.half | `0.500000` | `0.500000` | PASS |
| `Audio.getMusicVolume` | Audio.getMusicVolume.scaled | `0.125000` | `0.125000` | PASS |
| `Audio.getSFXVolume` | Audio.getSFXVolume.scaled | `0.375000` | `0.375000` | PASS |
| `Audio.setMasterVolume` | Audio.setMasterVolume.readback | `0.500000` | `0.500000` | PASS |
| `Audio.setMusicVolume` | Audio.setMusicVolume.readback | `0.125000` | `0.125000` | PASS |
| `Audio.setSFXVolume` | Audio.setSFXVolume.readback | `0.375000` | `0.375000` | PASS |
| `Audio.getMasterVolume` | Audio.getMasterVolume.high | `1.000000` | `1.000000` | PASS |
| `Audio.setMasterVolume` | Audio.setMasterVolume.high | `1.000000` | `1.000000` | PASS |
| `Audio.getMasterVolume` | Audio.getMasterVolume.low | `0.000000` | `0.000000` | PASS |
| `Audio.setMasterVolume` | Audio.setMasterVolume.low | `0.000000` | `0.000000` | PASS |
| `Audio.getFrequencyMultiplier` | Audio.getFrequencyMultiplier.changed | `0.750000` | `0.750000` | PASS |
| `Audio.setFrequencyMultiplier` | Audio.setFrequencyMultiplier.readback | `0.750000` | `0.750000` | PASS |
| `Audio.loadSound` | Audio.loadSound.missing | `0` | `0` | PASS |
| `Audio.getSoundLength` | Audio.getSoundLength.missing | `0` | `0` | PASS |
| `Audio.playSound` | Audio.playSound.missing | `0` | `0` | PASS |
| `Audio.playStream` | Audio.playStream.missing | `0` | `0` | PASS |
| `Audio.isVoiceValid` | Audio.isVoiceValid.zero | `0` | `0` | PASS |
| `Audio.isVoiceValid` | Audio.isVoiceValid.unknown | `0` | `0` | PASS |
| `Audio.getVoiceLength` | Audio.getVoiceLength.unknown | `0` | `0` | PASS |
| `Audio.getVoicePosition` | Audio.getVoicePosition.unknown | `0` | `0` | PASS |
| `Audio.getVoiceSpeed` | Audio.getVoiceSpeed.unknown | `0.000000` | `0.000000` | PASS |
| `Audio.getVoiceVolume` | Audio.getVoiceVolume.unknown | `0.000000` | `0.000000` | PASS |
| `Audio.isVoicePaused` | Audio.isVoicePaused.unknown | `0` | `0` | PASS |
| `Audio.isRecordingScript` | Audio.isRecordingScript.default | `0` | `0` | PASS |
| `Audio.isPlayingScript` | Audio.isPlayingScript.default | `0` | `0` | PASS |
| `Audio.isRetriggering` | Audio.isRetriggering.default | `0` | `0` | PASS |
| `Audio.getRetriggerTime` | Audio.getRetriggerTime.default | `0` | `0` | PASS |
| `Audio.isRecording` | Audio.isRecording.default | `0` | `0` | PASS |
| `Audio.getAllVoices` | Audio.getAllVoices.empty | `0` | `0` | PASS |
| `Audio.startRecordingScript` | Audio.startRecordingScript.active | `1` | `1` | PASS |
| `Audio.stopRecordingScript` | Audio.stopRecordingScript.inactive | `0` | `0` | PASS |
| `Audio.startRecording` | Audio.startRecording.active | `0` | `0` | PASS |
| `Audio.stopRecording` | Audio.stopRecording.inactive | `0` | `0` | PASS |
| `Audio.startRetriggering` | Audio.startRetriggering.active | `0` | `0` | PASS |
| `Audio.setRetriggerTime` | Audio.setRetriggerTime.changed | `0` | `0` | PASS |
| `Audio.stopRetriggering` | Audio.stopRetriggering.inactive | `0` | `0` | PASS |
| `Audio.hack_setEnabled` | Audio.hack_setEnabled.disabled | `0` | `0` | PASS |
| `Audio.getMasterEffectChain` | Audio.getMasterEffectChain.clear | `1` | `1` | PASS |
| `Audio.createEffectChain` | Audio.createEffectChain.child | `1` | `1` | PASS |
| `Audio.createEffectChain` | Audio.createEffectChain.effectControls | `1` | `1` | PASS |
| `Audio.freeSoundEffect` | Audio.freeSoundEffect.slotReused | `1` | `1` | PASS |
| `Audio.freeEffectChain` | Audio.freeEffectChain.released | `1` | `1` | PASS |
| `Audio.startPlayingScript` | Audio.startPlayingScript.active | `1` | `1` | PASS |
| `Audio.stopPlayingScript` | Audio.stopPlayingScript.inactive | `0` | `0` | PASS |
| `Audio.loadSound` | Audio.loadSound.sample | `1` | `1` | PASS |
| `Audio.getSoundLength` | Audio.getSoundLength.sample | `87` | `87` | PASS |
| `Audio.playSound` | Audio.playSound.onePositive | `1` | `1` | PASS |
| `Audio.isVoiceValid` | Audio.isVoiceValid.one | `1` | `1` | PASS |
| `Audio.getVoiceLength` | Audio.getVoiceLength.one | `87` | `87` | PASS |
| `Audio.getVoiceSpeed` | Audio.getVoiceSpeed.one | `1.000000` | `1.000000` | PASS |
| `Audio.getVoiceVolume` | Audio.getVoiceVolume.one | `1.000000` | `1.000000` | PASS |
| `Audio.isVoicePaused` | Audio.isVoicePaused.one | `0` | `0` | PASS |
| `Audio.playSound` | Audio.playSound.threePositive | `1` | `1` | PASS |
| `Audio.isVoiceValid` | Audio.isVoiceValid.three | `1` | `1` | PASS |
| `Audio.getVoiceVolume` | Audio.getVoiceVolume.three | `0.250000` | `0.250000` | PASS |
| `Audio.playSound` | Audio.playSound.fivePositive | `1` | `1` | PASS |
| `Audio.isVoiceValid` | Audio.isVoiceValid.five | `1` | `1` | PASS |
| `Audio.getVoiceSpeed` | Audio.getVoiceSpeed.five | `1.000000` | `1.000000` | PASS |
| `Audio.getVoiceVolume` | Audio.getVoiceVolume.changed | `0.200000` | `0.200000` | PASS |
| `Audio.getVoiceSpeed` | Audio.getVoiceSpeed.changed | `1.250000` | `1.250000` | PASS |
| `Audio.getVoiceSpeed` | Audio.getVoiceSpeed.note | `1.000000` | `1.000000` | PASS |
| `Audio.isVoicePaused` | Audio.isVoicePaused.paused | `1` | `1` | PASS |
| `Audio.isVoicePaused` | Audio.isVoicePaused.resumed | `0` | `0` | PASS |
| `Audio.getVoicePosition` | Audio.getVoicePosition.set | `20` | `20` | PASS |
| `Audio.getAllVoices` | Audio.getAllVoices.hasVoice | `1` | `1` | PASS |
| `Audio.loadSoundSlicker` | Audio.loadSoundSlicker.sample | `1` | `1` | PASS |
| `Audio.playSound` | Audio.playSound.controlPositive | `1` | `1` | PASS |
| `Audio.setVoiceVolume` | Audio.setVoiceVolume.quarter | `0.250000` | `0.250000` | PASS |
| `Audio.setVoiceVolume` | Audio.setVoiceVolume.zero | `0.000000` | `0.000000` | PASS |
| `Audio.setVoiceVolume` | Audio.setVoiceVolume.full | `1.000000` | `1.000000` | PASS |
| `Audio.setVoiceSpeed` | Audio.setVoiceSpeed.immediate | `1.500000` | `1.500000` | PASS |
| `Audio.setVoiceSpeed` | Audio.setVoiceSpeed.rampStart | `1.500000` | `1.500000` | PASS |
| `Audio.setVoiceNote` | Audio.setVoiceNote.base | `1.000000` | `1.000000` | PASS |
| `Audio.setVoicePitchBend` | Audio.setVoicePitchBend.immediate | `1.000000` | `1.000000` | PASS |
| `Audio.setVoicePosition` | Audio.setVoicePosition.changed | `25` | `25` | PASS |
| `Audio.setVoicePause` | Audio.setVoicePause.paused | `1` | `1` | PASS |
| `Audio.setVoicePause` | Audio.setVoicePause.resumed | `0` | `0` | PASS |
| `Audio.setVoiceLoop` | Audio.setVoiceLoop.valid | `1` | `1` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.pending | `1` | `1` | PASS |
| `Audio.playStream` | Audio.playStream.preloaded | `1` | `1` | PASS |
| `Audio.playStream` | Audio.playStream.samplePositive | `0` | `1` | SETUP SKIP |
| `Audio.playStream` | Audio.playStream.sampleLength | `0` | `2400` | SETUP SKIP |
| `Audio.unloadSound` | Audio.unloadSound.length | `87` | `87` | PASS |
| `Audio.setVoicePitchBend` | Audio.setVoicePitchBend.beforeUpdate | `1.000000` | `1.000000` | PASS |
| `Audio.setVoicePitchBend` | Audio.setVoicePitchBend.afterUpdate | `2.000000` | `2.000000` | PASS |
| `Audio.loadSoundSlicker` | Audio.loadSoundSlicker.ready | `1` | `1` | PASS |
| `Audio.spawnSoundSlicker` | Audio.spawnSoundSlicker.positive | `1` | `1` | PASS |
| `Audio.getNumDiracDSPsUsed` | Audio.getNumDiracDSPsUsed.spawned | `1` | `1` | PASS |
| `Audio.getNumDiracDSPChannelsUsed` | Audio.getNumDiracDSPChannelsUsed.spawned | `1` | `1` | PASS |
| `Audio.getNumDiracDSPsFree` | Audio.getNumDiracDSPsFree.spawned | `6` | `6` | PASS |
| `Audio.checkDiracPoolIntegrity` | Audio.checkDiracPoolIntegrity.spawned | `0` | `0` | PASS |
| `Audio.playSoundSlicker` | Audio.playSoundSlicker.sameVoice | `0` | `1` | SETUP SKIP |
| `Audio.playSoundSlicker` | Audio.playSoundSlicker.volume | `0.000000` | `1.000000` | SETUP SKIP |
| `Audio.fadeOutVoice` | Audio.fadeOutVoice.active | `1` | `1` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.timedActive | `1` | `1` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.fadeActive | `1` | `1` | PASS |
| `Audio.playSound` | Audio.playSound.naturalStart | `1` | `1` | PASS |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.active | `1` | `1` | PASS |
| `Audio.setVoicePause` | Audio.setVoicePause.active | `1` | `1` | PASS |
| `Audio.onVoiceEndFreeEffectChain` | Audio.onVoiceEndFreeEffectChain.pending | `1` | `1` | PASS |
| `Audio.unloadSound` | Audio.unloadSound.activeVoice | `1` | `1` | PASS |
| `Audio.fadeOutVoice` | Audio.fadeOutVoice.after1 | `1` | `1` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.timedAfter1 | `1` | `1` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.fadeAfter1 | `1` | `1` | PASS |
| `Audio.getVoicePosition` | Audio.getVoicePosition.pausedInitial | `0` | `0` | PASS |
| `Audio.playSound` | Audio.playSound.natural.position1 | `0` | `16` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.valid1 | `0` | `1` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.position1 | `0` | `16` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.valid1 | `0` | `1` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.position2 | `0` | `33` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.valid2 | `0` | `1` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.position2 | `0` | `33` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.valid2 | `0` | `1` | CLOCK |
| `Audio.getVoicePosition` | Audio.getVoicePosition.pausedHeld | `0` | `0` | PASS |
| `Audio.setVoicePause` | Audio.setVoicePause.resumed | `0` | `0` | PASS |
| `Audio.playSound` | Audio.playSound.natural.position3 | `0` | `49` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.valid3 | `0` | `1` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.position3 | `0` | `49` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.valid3 | `0` | `1` | CLOCK |
| `Audio.getVoicePosition` | Audio.getVoicePosition.resumed | `15` | `16` | CLOCK |
| `Audio.playSound` | Audio.playSound.naturalBeforeEnd | `0` | `1` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.position4 | `0` | `66` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.position4 | `0` | `66` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.valid4 | `0` | `1` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.position5 | `0` | `83` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.valid5 | `0` | `1` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.position5 | `0` | `83` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.valid5 | `0` | `1` | CLOCK |
| `Audio.playSound` | Audio.playSound.naturalEnd | `0` | `0` | PASS |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.secondPass | `0` | `1` | CLOCK |
| `Audio.playSound` | Audio.playSound.natural.position6 | `0` | `0` | CLOCK |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.position6 | `0` | `12` | CLOCK |
| `Audio.fadeOutVoice` | Audio.fadeOutVoice.complete | `0` | `0` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.timedComplete | `0` | `0` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.fadeComplete | `0` | `0` | PASS |
| `Audio.onVoiceEndFreeEffectChain` | Audio.onVoiceEndFreeEffectChain.voiceEnded | `1` | `1` | PASS |
| `Audio.unloadSound` | Audio.unloadSound.voiceStillActive | `0` | `0` | PASS |
| `Audio.setVoiceLoopCount` | Audio.setVoiceLoopCount.complete | `0` | `0` | PASS |
| `Audio.stopAllVoices` | Audio.stopAllVoices.firstPending | `1` | `1` | PASS |
| `Audio.stopAllVoices` | Audio.stopAllVoices.secondPending | `1` | `1` | PASS |
| `Audio.stopAllVoices` | Audio.stopAllVoices.beforeFadeEnd | `1` | `1` | PASS |
| `Audio.stopAllVoices` | Audio.stopAllVoices.afterFadeEnd | `0` | `0` | PASS |
| `Audio.stopAllVoices` | Audio.stopAllVoices.noVoicesLeft | `1` | `1` | PASS |
| `Audio.setVoiceGateTime` | Audio.setVoiceGateTime.active | `1` | `1` | PASS |
| `Audio.setVoiceGateTime` | Audio.setVoiceGateTime.afterOneTick | `1` | `1` | PASS |
| `Audio.setVoiceGateTime` | Audio.setVoiceGateTime.afterFourTicks | `1` | `1` | PASS |
| `Audio.setVoiceGateTime` | Audio.setVoiceGateTime.afterEightTicks | `1` | `1` | PASS |
| `Audio.setVoiceGateTime` | Audio.setVoiceGateTime.afterTwentyTicks | `1` | `1` | PASS |
| `Audio.setVoiceGateTime` | Audio.setVoiceGateTime.afterSixtyTicks | `1` | `1` | PASS |
| `Audio.setVoiceGateTime` | Audio.setVoiceGateTime.afterHundredTicks | `1` | `1` | PASS |
