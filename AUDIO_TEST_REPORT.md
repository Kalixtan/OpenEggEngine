# Audio test report

Generated: 2026-10-20 21:50 UTC

Matched: **54/54 (100%)** script readings. This does not test emitted audio or DSP parity.

Voice probe: tested with user supplied sound. Functions with readings: **26/68**.

## Per function

| Function | Matched | Cases | Status |
| --- | ---: | ---: | --- |
| `Audio.checkDiracPoolIntegrity` | 1 | 1 | PASS |
| `Audio.createEffectChain` | 0 | 0 | untested |
| `Audio.fadeOutVoice` | 0 | 0 | untested |
| `Audio.freeEffectChain` | 0 | 0 | untested |
| `Audio.freeSoundEffect` | 0 | 0 | untested |
| `Audio.getAllVoices` | 2 | 2 | PASS |
| `Audio.getDefaultBaseNote` | 1 | 1 | PASS |
| `Audio.getFreeMemory` | 0 | 0 | untested |
| `Audio.getFrequencyMultiplier` | 2 | 2 | PASS |
| `Audio.getMasterEffectChain` | 0 | 0 | untested |
| `Audio.getMasterVolume` | 4 | 4 | PASS |
| `Audio.getMusicVolume` | 2 | 2 | PASS |
| `Audio.getNumDiracDSPChannelsUsed` | 1 | 1 | PASS |
| `Audio.getNumDiracDSPs` | 1 | 1 | PASS |
| `Audio.getNumDiracDSPsFree` | 1 | 1 | PASS |
| `Audio.getNumDiracDSPsUsed` | 1 | 1 | PASS |
| `Audio.getRetriggerTime` | 1 | 1 | PASS |
| `Audio.getSFXVolume` | 2 | 2 | PASS |
| `Audio.getSoundLength` | 2 | 2 | PASS |
| `Audio.getVoiceLength` | 2 | 2 | PASS |
| `Audio.getVoicePosition` | 2 | 2 | PASS |
| `Audio.getVoiceSpeed` | 5 | 5 | PASS |
| `Audio.getVoiceVolume` | 4 | 4 | PASS |
| `Audio.hack_setEnabled` | 0 | 0 | untested |
| `Audio.isPlayingScript` | 1 | 1 | PASS |
| `Audio.isRecording` | 1 | 1 | PASS |
| `Audio.isRecordingScript` | 1 | 1 | PASS |
| `Audio.isRetriggering` | 1 | 1 | PASS |
| `Audio.isVoicePaused` | 4 | 4 | PASS |
| `Audio.isVoiceValid` | 5 | 5 | PASS |
| `Audio.loadSampleList` | 0 | 0 | untested |
| `Audio.loadScriptFromFile` | 0 | 0 | untested |
| `Audio.loadSound` | 2 | 2 | PASS |
| `Audio.loadSoundSlicker` | 0 | 0 | untested |
| `Audio.onVoiceEndFreeEffectChain` | 0 | 0 | untested |
| `Audio.onVoiceEndFreeSoundEffect` | 0 | 0 | untested |
| `Audio.playSound` | 4 | 4 | PASS |
| `Audio.playSoundSlicker` | 0 | 0 | untested |
| `Audio.playStream` | 1 | 1 | PASS |
| `Audio.saveScriptToFile` | 0 | 0 | untested |
| `Audio.setFrequencyMultiplier` | 0 | 0 | untested |
| `Audio.setMasterVolume` | 0 | 0 | untested |
| `Audio.setMusicVolume` | 0 | 0 | untested |
| `Audio.setRetriggerTime` | 0 | 0 | untested |
| `Audio.setSFXVolume` | 0 | 0 | untested |
| `Audio.setVoiceGateTime` | 0 | 0 | untested |
| `Audio.setVoiceLoop` | 0 | 0 | untested |
| `Audio.setVoiceLoopCount` | 0 | 0 | untested |
| `Audio.setVoiceNote` | 0 | 0 | untested |
| `Audio.setVoicePan` | 0 | 0 | untested |
| `Audio.setVoicePause` | 0 | 0 | untested |
| `Audio.setVoicePitchBend` | 0 | 0 | untested |
| `Audio.setVoicePosition` | 0 | 0 | untested |
| `Audio.setVoiceSpeed` | 0 | 0 | untested |
| `Audio.setVoiceVolume` | 0 | 0 | untested |
| `Audio.spawnSoundSlicker` | 0 | 0 | untested |
| `Audio.startPlayingScript` | 0 | 0 | untested |
| `Audio.startRecording` | 0 | 0 | untested |
| `Audio.startRecordingScript` | 0 | 0 | untested |
| `Audio.startRetriggering` | 0 | 0 | untested |
| `Audio.stopAllVoices` | 0 | 0 | untested |
| `Audio.stopPlayingScript` | 0 | 0 | untested |
| `Audio.stopRecording` | 0 | 0 | untested |
| `Audio.stopRecordingScript` | 0 | 0 | untested |
| `Audio.stopRetriggering` | 0 | 0 | untested |
| `Audio.stopVoice` | 0 | 0 | untested |
| `Audio.unloadAllSounds` | 0 | 0 | untested |
| `Audio.unloadSound` | 0 | 0 | untested |

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
