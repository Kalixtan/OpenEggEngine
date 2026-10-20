# Audio test report

Generated: 2026-10-20 23:00 UTC

Matched: **94/98 (96%)** script readings. Effect-chain calls without a getter only check completion. This does not test emitted audio or DSP parity.

Asset probes: tested with user supplied sound. Functions with readings: **54/68**.

## Per function

| Function | Matched | Cases | Status |
| --- | ---: | ---: | --- |
| `Audio.checkDiracPoolIntegrity` | 2 | 2 | PASS |
| `Audio.createEffectChain` | 2 | 2 | PASS |
| `Audio.fadeOutVoice` | 1 | 1 | PASS |
| `Audio.freeEffectChain` | 1 | 1 | PASS |
| `Audio.freeSoundEffect` | 1 | 1 | PASS |
| `Audio.getAllVoices` | 2 | 2 | PASS |
| `Audio.getDefaultBaseNote` | 1 | 1 | PASS |
| `Audio.getFreeMemory` | 0 | 0 | untested |
| `Audio.getFrequencyMultiplier` | 2 | 2 | PASS |
| `Audio.getMasterEffectChain` | 1 | 1 | PASS |
| `Audio.getMasterVolume` | 4 | 4 | PASS |
| `Audio.getMusicVolume` | 2 | 2 | PASS |
| `Audio.getNumDiracDSPChannelsUsed` | 2 | 2 | PASS |
| `Audio.getNumDiracDSPs` | 1 | 1 | PASS |
| `Audio.getNumDiracDSPsFree` | 2 | 2 | PASS |
| `Audio.getNumDiracDSPsUsed` | 2 | 2 | PASS |
| `Audio.getRetriggerTime` | 1 | 1 | PASS |
| `Audio.getSFXVolume` | 2 | 2 | PASS |
| `Audio.getSoundLength` | 2 | 2 | PASS |
| `Audio.getVoiceLength` | 2 | 2 | PASS |
| `Audio.getVoicePosition` | 2 | 2 | PASS |
| `Audio.getVoiceSpeed` | 5 | 5 | PASS |
| `Audio.getVoiceVolume` | 4 | 4 | PASS |
| `Audio.hack_setEnabled` | 1 | 1 | PASS |
| `Audio.isPlayingScript` | 1 | 1 | PASS |
| `Audio.isRecording` | 1 | 1 | PASS |
| `Audio.isRecordingScript` | 1 | 1 | PASS |
| `Audio.isRetriggering` | 1 | 1 | PASS |
| `Audio.isVoicePaused` | 4 | 4 | PASS |
| `Audio.isVoiceValid` | 5 | 5 | PASS |
| `Audio.loadSampleList` | 0 | 0 | untested |
| `Audio.loadScriptFromFile` | 0 | 0 | untested |
| `Audio.loadSound` | 2 | 2 | PASS |
| `Audio.loadSoundSlicker` | 2 | 2 | PASS |
| `Audio.onVoiceEndFreeEffectChain` | 1 | 1 | PASS |
| `Audio.onVoiceEndFreeSoundEffect` | 0 | 0 | untested |
| `Audio.playSound` | 5 | 5 | PASS |
| `Audio.playSoundSlicker` | 0 | 2 | FAIL |
| `Audio.playStream` | 2 | 4 | FAIL |
| `Audio.saveScriptToFile` | 0 | 0 | untested |
| `Audio.setFrequencyMultiplier` | 0 | 0 | untested |
| `Audio.setMasterVolume` | 0 | 0 | untested |
| `Audio.setMusicVolume` | 0 | 0 | untested |
| `Audio.setRetriggerTime` | 1 | 1 | PASS |
| `Audio.setSFXVolume` | 0 | 0 | untested |
| `Audio.setVoiceGateTime` | 0 | 0 | untested |
| `Audio.setVoiceLoop` | 1 | 1 | PASS |
| `Audio.setVoiceLoopCount` | 0 | 0 | untested |
| `Audio.setVoiceNote` | 1 | 1 | PASS |
| `Audio.setVoicePan` | 0 | 0 | untested |
| `Audio.setVoicePause` | 2 | 2 | PASS |
| `Audio.setVoicePitchBend` | 3 | 3 | PASS |
| `Audio.setVoicePosition` | 1 | 1 | PASS |
| `Audio.setVoiceSpeed` | 2 | 2 | PASS |
| `Audio.setVoiceVolume` | 0 | 0 | untested |
| `Audio.spawnSoundSlicker` | 1 | 1 | PASS |
| `Audio.startPlayingScript` | 1 | 1 | PASS |
| `Audio.startRecording` | 1 | 1 | PASS |
| `Audio.startRecordingScript` | 1 | 1 | PASS |
| `Audio.startRetriggering` | 1 | 1 | PASS |
| `Audio.stopAllVoices` | 1 | 1 | PASS |
| `Audio.stopPlayingScript` | 1 | 1 | PASS |
| `Audio.stopRecording` | 1 | 1 | PASS |
| `Audio.stopRecordingScript` | 1 | 1 | PASS |
| `Audio.stopRetriggering` | 1 | 1 | PASS |
| `Audio.stopVoice` | 2 | 2 | PASS |
| `Audio.unloadAllSounds` | 0 | 0 | untested |
| `Audio.unloadSound` | 1 | 1 | PASS |

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
| `Audio.getMasterVolume` | Audio.getMasterVolume.high | `1.000000` | `1.000000` | PASS |
| `Audio.getMasterVolume` | Audio.getMasterVolume.low | `0.000000` | `0.000000` | PASS |
| `Audio.getFrequencyMultiplier` | Audio.getFrequencyMultiplier.changed | `0.750000` | `0.750000` | PASS |
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
| `Audio.playStream` | Audio.playStream.samplePositive | `0` | `1` | FAIL |
| `Audio.playStream` | Audio.playStream.sampleLength | `0` | `2400` | FAIL |
| `Audio.unloadSound` | Audio.unloadSound.length | `87` | `87` | PASS |
| `Audio.setVoicePitchBend` | Audio.setVoicePitchBend.beforeUpdate | `1.000000` | `1.000000` | PASS |
| `Audio.setVoicePitchBend` | Audio.setVoicePitchBend.afterUpdate | `2.000000` | `2.000000` | PASS |
| `Audio.loadSoundSlicker` | Audio.loadSoundSlicker.ready | `1` | `1` | PASS |
| `Audio.spawnSoundSlicker` | Audio.spawnSoundSlicker.positive | `1` | `1` | PASS |
| `Audio.getNumDiracDSPsUsed` | Audio.getNumDiracDSPsUsed.spawned | `1` | `1` | PASS |
| `Audio.getNumDiracDSPChannelsUsed` | Audio.getNumDiracDSPChannelsUsed.spawned | `1` | `1` | PASS |
| `Audio.getNumDiracDSPsFree` | Audio.getNumDiracDSPsFree.spawned | `6` | `6` | PASS |
| `Audio.checkDiracPoolIntegrity` | Audio.checkDiracPoolIntegrity.spawned | `0` | `0` | PASS |
| `Audio.playSoundSlicker` | Audio.playSoundSlicker.sameVoice | `0` | `1` | FAIL |
| `Audio.playSoundSlicker` | Audio.playSoundSlicker.volume | `0.000000` | `1.000000` | FAIL |
| `Audio.fadeOutVoice` | Audio.fadeOutVoice.pending | `1` | `1` | PASS |
| `Audio.stopVoice` | Audio.stopVoice.timedPending | `1` | `1` | PASS |
| `Audio.onVoiceEndFreeEffectChain` | Audio.onVoiceEndFreeEffectChain.active | `1` | `1` | PASS |
| `Audio.stopAllVoices` | Audio.stopAllVoices.fadePending | `1` | `1` | PASS |
