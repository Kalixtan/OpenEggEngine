# Sequencer test report

Generated: 2026-10-20 16:07 UTC  
Retail: `tests/sequencer/retail-baseline.tsv`  
OpenEgg: `tests/_results/sequencer/probe.log`  
Matched: **57/60 retail readings (95%)**  
Known gaps: **3**  Uncaptured retail readings: **6**

Local assertions: **47/49 passed (96%)**

One row is one function reading from the same `probe.gm` on both engines. Scores measure tested cases only; they are not total implementation progress. Integration checks can count for several functions. Missing retail callback readings stay outside the paired score.

## Sections

| Section | Passed | Cases | Score |
| --- | ---: | ---: | ---: |
| Core | 4 | 4 | 100% |
| State | 5 | 5 | 100% |
| Manager | 4 | 4 | 100% |
| Timers | 3 | 3 | 100% |
| Loops | 2 | 2 | 100% |
| Safety | 3 | 3 | 100% |
| Trigger forms | 11 | 13 | 85% |
| Actions | 6 | 6 | 100% |
| Sequence conversion | 9 | 9 | 100% |

## Per function

| Function | Retail | Local checks | Combined score | Status |
| --- | ---: | ---: | ---: | --- |
| `SEQUENCER` | 0/0 | 2/2 | 2/2 (100%) | NO RETAIL |
| `SEQUENCERACTION` | 2/2 | 3/3 | 5/5 (100%) | PASS |
| `SEQUENCER_GetAllSequencers` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `SEQUENCER_OnNoteCollect` | 0/0 | 0/0 | 0/0 (—) | untested |
| `SEQUENCER_PauseAllSequencers` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `SEQUENCER_SetMasterPulse` | 0/0 | 0/0 | 0/0 (—) | untested |
| `SEQUENCER_UnpauseAllSequencers` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `SEQUENCE_FromString` | 2/2 | 4/4 | 6/6 (100%) | PASS |
| `SEQUENCE_FromTable` | 2/2 | 3/3 | 5/5 (100%) | PASS |
| `SEQUENCE_ToString` | 1/1 | 3/3 | 4/4 (100%) | PASS |
| `SEQUENCE_ToTable` | 1/1 | 3/3 | 4/4 (100%) | PASS |
| `Sequencer.clearTriggers` | 0/0 | 2/2 | 2/2 (100%) | NO RETAIL |
| `Sequencer.getAllTriggers` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.getBPM` | 2/2 | 2/2 | 4/4 (100%) | PASS |
| `Sequencer.getCurrentPattern` | 2/2 | 5/5 | 7/7 (100%) | PASS |
| `Sequencer.getCurrentStep` | 4/4 | 5/5 | 9/9 (100%) | PASS |
| `Sequencer.getCurrentStep16` | 1/1 | 0/0 | 1/1 (100%) | PASS |
| `Sequencer.getMSPerStep` | 2/2 | 4/4 | 6/6 (100%) | PASS |
| `Sequencer.getMSTillNextNoteStep` | 1/1 | 0/0 | 1/1 (100%) | PASS |
| `Sequencer.getMSTillNextStep` | 0/0 | 4/4 | 4/4 (100%) | NO RETAIL |
| `Sequencer.getMSTillNextStep16` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.getMSTillNextStepAccurate` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.getNextNotePattern` | 1/1 | 2/2 | 3/3 (100%) | PASS |
| `Sequencer.getNextNoteStep` | 1/1 | 2/2 | 3/3 (100%) | PASS |
| `Sequencer.getNoteTime` | 2/2 | 1/1 | 3/3 (100%) | PASS |
| `Sequencer.getNumStepsPerNote` | 3/3 | 1/1 | 4/4 (100%) | PASS |
| `Sequencer.getNumStepsPerPattern` | 2/2 | 3/3 | 5/5 (100%) | PASS |
| `Sequencer.getNumTriggers` | 2/2 | 6/6 | 8/8 (100%) | PARTIAL |
| `Sequencer.getObjectID` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.getWrappedStep` | 2/2 | 0/0 | 2/2 (100%) | PASS |
| `Sequencer.gotoStep` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.isPlaying` | 5/5 | 3/3 | 8/8 (100%) | PASS |
| `Sequencer.moveTriggerToFront` | 0/0 | 2/2 | 2/2 (100%) | NO RETAIL |
| `Sequencer.restart` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.setBPM` | 0/0 | 3/3 | 3/3 (100%) | NO RETAIL |
| `Sequencer.setCurrentStep` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.setLoopLen` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.setNumStepsPerPattern` | 0/0 | 2/2 | 2/2 (100%) | NO RETAIL |
| `Sequencer.setPan` | 0/0 | 0/0 | 0/0 (—) | untested |
| `Sequencer.setTrigger` | 3/3 | 9/9 | 12/12 (100%) | PARTIAL |
| `Sequencer.setTrigger16` | 1/1 | 2/2 | 3/3 (100%) | PASS |
| `Sequencer.setTriggerOnNext` | 2/2 | 1/1 | 3/3 (100%) | PASS |
| `Sequencer.setVolume` | 0/0 | 0/0 | 0/0 (—) | untested |
| `Sequencer.start` | 0/0 | 4/4 | 4/4 (100%) | NO RETAIL |
| `Sequencer.stop` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `Sequencer.test` | 0/0 | 0/0 | 0/0 (—) | untested |
| `SequencerAction.isTaggedForRemoval` | 2/2 | 2/2 | 4/4 (100%) | PASS |
| `SequencerAction.setTable` | 1/1 | 2/2 | 3/3 (100%) | PASS |
| `SequencerAction.tagForRemoval` | 0/0 | 3/3 | 3/3 (100%) | NO RETAIL |
| `SequencerListRef.get` | 1/1 | 3/3 | 4/4 (100%) | PASS |
| `SequencerListRef.goToNext` | 0/0 | 2/2 | 2/2 (100%) | NO RETAIL |
| `SequencerListRef.isValid` | 3/3 | 3/3 | 6/6 (100%) | PASS |
| `SequencerListRef.reset` | 0/0 | 1/1 | 1/1 (100%) | NO RETAIL |
| `SequencerListRef.size` | 1/1 | 1/1 | 2/2 (100%) | PASS |
| `SequencerTrigger.getAction` | 0/2 | 0/1 | 0/3 (0%) | GAP |
| `SequencerTrigger.getLooped` | 0/0 | 7/7 | 7/7 (100%) | NO RETAIL |
| `SequencerTrigger.getPattern` | 0/0 | 8/8 | 8/8 (100%) | NO RETAIL |
| `SequencerTrigger.getStep` | 0/0 | 8/8 | 8/8 (100%) | NO RETAIL |
| `SequencerTriggerList.get` | 3/4 | 3/4 | 6/8 (75%) | GAP |
| `SequencerTriggerList.goToNext` | 0/0 | 2/2 | 2/2 (100%) | NO RETAIL |
| `SequencerTriggerList.isValid` | 1/1 | 4/4 | 5/5 (100%) | PASS |
| `SequencerTriggerList.reset` | 0/0 | 2/2 | 2/2 (100%) | NO RETAIL |
| `SequencerTriggerList.size` | 1/1 | 3/3 | 4/4 (100%) | PASS |

## Readings

| Function | Case | Retail | OpenEgg | Result |
| --- | --- | --- | --- | --- |
| `Sequencer.getBPM` | Sequencer.getBPM.default | `98` | `98` | PASS |
| `Sequencer.getNumStepsPerPattern` | Sequencer.getNumStepsPerPattern.default | `32` | `32` | PASS |
| `Sequencer.getCurrentStep` | Sequencer.getCurrentStep.default | `-1` | `-1` | PASS |
| `Sequencer.getCurrentPattern` | Sequencer.getCurrentPattern.default | `0` | `0` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.default | `0` | `0` | PASS |
| `Sequencer.getMSPerStep` | Sequencer.getMSPerStep.default | `76.530609` | `76.530609` | PASS |
| `Sequencer.getMSTillNextNoteStep` | Sequencer.getMSTillNextNoteStep.default | `0.000000` | `0.000000` | PASS |
| `Sequencer.getBPM` | Sequencer.getBPM.changed | `120` | `120` | PASS |
| `Sequencer.getMSPerStep` | Sequencer.getMSPerStep.changed | `62.500000` | `62.500000` | PASS |
| `Sequencer.getNumStepsPerPattern` | Sequencer.getNumStepsPerPattern.float | `16` | `16` | PASS |
| `Sequencer.getWrappedStep` | Sequencer.getWrappedStep.negative | `15` | `15` | PASS |
| `Sequencer.getWrappedStep` | Sequencer.getWrappedStep.overflow | `1` | `1` | PASS |
| `Sequencer.getNumStepsPerNote` | Sequencer.getNumStepsPerNote.zero | `1` | `1` | PASS |
| `Sequencer.getNumStepsPerNote` | Sequencer.getNumStepsPerNote.five | `32` | `32` | PASS |
| `Sequencer.getNumStepsPerNote` | Sequencer.getNumStepsPerNote.six | `0` | `0` | PASS |
| `Sequencer.getNoteTime` | Sequencer.getNoteTime.zero | `62.500000` | `62.500000` | PASS |
| `Sequencer.getNoteTime` | Sequencer.getNoteTime.five | `2000.000000` | `2000.000000` | PASS |
| `Sequencer.getNextNotePattern` | Sequencer.getNextNotePattern.wrap | `1` | `1` | PASS |
| `Sequencer.getNextNoteStep` | Sequencer.getNextNoteStep.wrap | `0` | `0` | PASS |
| `Sequencer.getCurrentStep` | Sequencer.getCurrentStep.raw | `-1` | `-1` | PASS |
| `Sequencer.getCurrentPattern` | Sequencer.getCurrentPattern.raw | `0` | `0` | PASS |
| `Sequencer.getCurrentStep16` | Sequencer.getCurrentStep16.five | `2` | `2` | PASS |
| `Sequencer.getCurrentStep` | Sequencer.getCurrentStep.set | `5` | `5` | PASS |
| `SEQUENCERACTION` | SequencerAction.field.default | `7` | `7` | PASS |
| `SequencerAction.isTaggedForRemoval` | SequencerAction.isTaggedForRemoval.default | `0` | `0` | PASS |
| `SequencerAction.setTable` | SequencerAction.setTable.value | `9` | `9` | PASS |
| `SEQUENCERACTION` | SEQUENCERACTION.table.value | `4` | `4` | PASS |
| `SequencerAction.isTaggedForRemoval` | SequencerAction.isTaggedForRemoval.tagged | `1` | `1` | PASS |
| `Sequencer.setTrigger` | Sequencer.setTrigger.implicit.pattern | `0` | `0` | PASS |
| `Sequencer.setTrigger` | Sequencer.setTrigger.implicit.step | `15` | `15` | PASS |
| `Sequencer.setTrigger` | Sequencer.setTrigger.implicit.looped | `1` | `1` | PASS |
| `SequencerTrigger.getAction` | SequencerTrigger.getAction.fresh | `1` | `0` | GAP |
| `SequencerTrigger.getAction` | SequencerTrigger.getAction.repeat | `1` | `0` | GAP |
| `Sequencer.setTrigger16` | Sequencer.setTrigger16.step | `2` | `2` | PASS |
| `Sequencer.setTriggerOnNext` | Sequencer.setTriggerOnNext.pattern | `0` | `0` | PASS |
| `Sequencer.setTriggerOnNext` | Sequencer.setTriggerOnNext.step | `8` | `8` | PASS |
| `Sequencer.getNumTriggers` | Sequencer.getNumTriggers.three | `3` | `3` | PASS |
| `SequencerTriggerList.size` | SequencerTriggerList.size.three | `3` | `3` | PASS |
| `SequencerTriggerList.isValid` | SequencerTriggerList.isValid.first | `1` | `1` | PASS |
| `SequencerTriggerList.get` | SequencerTriggerList.get.fresh | `1` | `0` | GAP |
| `SequencerTriggerList.get` | SequencerTriggerList.get.firstStep | `2` | `2` | PASS |
| `SequencerTriggerList.get` | SequencerTriggerList.get.nextStep | `8` | `8` | PASS |
| `SequencerTriggerList.get` | SequencerTriggerList.get.resetStep | `2` | `2` | PASS |
| `Sequencer.getNumTriggers` | Sequencer.getNumTriggers.cleared | `0` | `0` | PASS |
| `SEQUENCE_ToTable` | SEQUENCE_ToTable.word | `613566756` | `613566756` | PASS |
| `SEQUENCE_FromTable` | SEQUENCE_FromTable.size | `16` | `16` | PASS |
| `SEQUENCE_FromTable` | SEQUENCE_FromTable.last | `0` | `0` | PASS |
| `SEQUENCE_ToString` | SEQUENCE_ToString.word | `z0120120120120120` | `z0120120120120120` | PASS |
| `SEQUENCE_FromString` | SEQUENCE_FromString.size | `16` | `16` | PASS |
| `SEQUENCE_FromString` | SEQUENCE_FromString.second | `1` | `1` | PASS |
| `SequencerListRef.size` | SequencerListRef.size.initial | `3` | `3` | PASS |
| `SequencerListRef.isValid` | SequencerListRef.isValid.initial | `1` | `1` | PASS |
| `SequencerListRef.get` | SequencerListRef.get.initial | `1` | `1` | PASS |
| `SequencerListRef.isValid` | SequencerListRef.isValid.next | `1` | `1` | PASS |
| `SequencerListRef.isValid` | SequencerListRef.isValid.reset | `1` | `1` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.started | `1` | `1` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.paused | `0` | `0` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.unpaused | `1` | `1` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.stopped | `0` | `0` | PASS |
| `Sequencer.getCurrentStep` | Sequencer.getCurrentStep.restarted | `-1` | `-1` | PASS |
| `Sequencer.setTrigger` | Sequencer.callback.step | `—` | `0` | NO RETAIL |
| `Sequencer.getMSTillNextStep` | Sequencer.getMSTillNextStep.callback | `—` | `45.833336` | NO RETAIL |
| `Sequencer.getMSTillNextStepAccurate` | Sequencer.getMSTillNextStepAccurate.callback | `—` | `45.833336` | NO RETAIL |
| `Sequencer.getMSTillNextStep16` | Sequencer.getMSTillNextStep16.callback | `—` | `91.666672` | NO RETAIL |
| `Sequencer.setTrigger` | Sequencer.callback.count | `—` | `1` | NO RETAIL |
| `Sequencer.getNumTriggers` | Sequencer.getNumTriggers.afterCallback | `—` | `0` | NO RETAIL |

## Known gaps

- `SequencerTrigger.getAction.fresh`: retail creates a new action wrapper.
- `SequencerTrigger.getAction.repeat`: retail creates a wrapper on each call.
- `SequencerTriggerList.get.fresh`: retail creates a new trigger wrapper.

The retail capture has no callback readings; the bundled run faulted before those results appeared. They are marked NO RETAIL.

Local assertion details: `tests/_results/assertions.md`.
