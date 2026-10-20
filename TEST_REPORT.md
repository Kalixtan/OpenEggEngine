# Sequencer test report

Generated: 2026-10-20 16:40 UTC
Retail: `tests/sequencer/retail-baseline.tsv` | OpenEgg: `tests/_results/sequencer/probe.log`

Matched: **75/75 declared readings (100%)** | Known gaps: **0** | Uncaptured: **0** | Planned: **19**

Local assertions: **49/49 passed (100%)**

Verdict: **all captured readings match**. **19 edge cases still need paired retail runs.**

One row is one exact string comparison from the same `probe.gm` on both engines. A missing reading or duplicate fails the run. Scores measure tested cases only; they are not total implementation progress. Integration checks can count for several functions. `OEPV__` clock readings stay outside the score.

## Sections

| Section | Passed | Cases | Score |
| --- | ---: | ---: | ---: |
| Core | 4 | 4 | 100% |
| State | 5 | 5 | 100% |
| Manager | 4 | 4 | 100% |
| Timers | 3 | 3 | 100% |
| Loops | 2 | 2 | 100% |
| Safety | 3 | 3 | 100% |
| Trigger forms | 13 | 13 | 100% |
| Actions | 6 | 6 | 100% |
| Sequence conversion | 9 | 9 | 100% |

## Per function

| Function | Paired score | Local checks | Planned | Status |
| --- | ---: | ---: | ---: | --- |
| `SEQUENCER` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `SEQUENCERACTION` | 2/2 (100%) | 3/3 | 1 | PENDING |
| `SEQUENCER_GetAllSequencers` | 1/1 (100%) | 1/1 | 1 | PENDING |
| `SEQUENCER_OnNoteCollect` | 1/1 (100%) | 0/0 | 0 | PASS |
| `SEQUENCER_PauseAllSequencers` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SEQUENCER_SetMasterPulse` | 1/1 (100%) | 0/0 | 1 | PENDING |
| `SEQUENCER_UnpauseAllSequencers` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SEQUENCE_FromString` | 2/2 (100%) | 4/4 | 1 | PENDING |
| `SEQUENCE_FromTable` | 2/2 (100%) | 3/3 | 1 | PENDING |
| `SEQUENCE_ToString` | 1/1 (100%) | 3/3 | 1 | PENDING |
| `SEQUENCE_ToTable` | 1/1 (100%) | 3/3 | 1 | PENDING |
| `Sequencer.clearTriggers` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `Sequencer.getAllTriggers` | 1/1 (100%) | 1/1 | 1 | PENDING |
| `Sequencer.getBPM` | 2/2 (100%) | 2/2 | 1 | PENDING |
| `Sequencer.getCurrentPattern` | 2/2 (100%) | 5/5 | 1 | PENDING |
| `Sequencer.getCurrentStep` | 4/4 (100%) | 5/5 | 0 | PASS |
| `Sequencer.getCurrentStep16` | 1/1 (100%) | 0/0 | 0 | PASS |
| `Sequencer.getMSPerStep` | 2/2 (100%) | 4/4 | 1 | PENDING |
| `Sequencer.getMSTillNextNoteStep` | 1/1 (100%) | 0/0 | 0 | PASS |
| `Sequencer.getMSTillNextStep` | 1/1 (100%) | 4/4 | 1 | PENDING |
| `Sequencer.getMSTillNextStep16` | 1/1 (100%) | 1/1 | 1 | PENDING |
| `Sequencer.getMSTillNextStepAccurate` | 1/1 (100%) | 1/1 | 1 | PENDING |
| `Sequencer.getNextNotePattern` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `Sequencer.getNextNoteStep` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `Sequencer.getNoteTime` | 2/2 (100%) | 1/1 | 0 | PASS |
| `Sequencer.getNumStepsPerNote` | 3/3 (100%) | 1/1 | 0 | PASS |
| `Sequencer.getNumStepsPerPattern` | 2/2 (100%) | 3/3 | 0 | PASS |
| `Sequencer.getNumTriggers` | 3/3 (100%) | 6/6 | 0 | PASS |
| `Sequencer.getObjectID` | 1/1 (100%) | 1/1 | 1 | PENDING |
| `Sequencer.getWrappedStep` | 2/2 (100%) | 0/0 | 1 | PENDING |
| `Sequencer.gotoStep` | 1/1 (100%) | 1/1 | 0 | PASS |
| `Sequencer.isPlaying` | 5/5 (100%) | 3/3 | 0 | PASS |
| `Sequencer.moveTriggerToFront` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `Sequencer.restart` | 1/1 (100%) | 1/1 | 1 | PENDING |
| `Sequencer.setBPM` | 1/1 (100%) | 3/3 | 2 | PENDING |
| `Sequencer.setCurrentStep` | 1/1 (100%) | 1/1 | 0 | PASS |
| `Sequencer.setLoopLen` | 1/1 (100%) | 1/1 | 1 | PENDING |
| `Sequencer.setNumStepsPerPattern` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `Sequencer.setPan` | 1/1 (100%) | 0/0 | 1 | PENDING |
| `Sequencer.setTrigger` | 5/5 (100%) | 9/9 | 6 | PENDING |
| `Sequencer.setTrigger16` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `Sequencer.setTriggerOnNext` | 2/2 (100%) | 1/1 | 1 | PENDING |
| `Sequencer.setVolume` | 1/1 (100%) | 0/0 | 1 | PENDING |
| `Sequencer.start` | 1/1 (100%) | 4/4 | 1 | PENDING |
| `Sequencer.stop` | 1/1 (100%) | 1/1 | 0 | PASS |
| `Sequencer.test` | 1/1 (100%) | 0/0 | 0 | PASS |
| `SequencerAction.isTaggedForRemoval` | 2/2 (100%) | 2/2 | 0 | PASS |
| `SequencerAction.setTable` | 1/1 (100%) | 2/2 | 2 | PENDING |
| `SequencerAction.tagForRemoval` | 1/1 (100%) | 3/3 | 2 | PENDING |
| `SequencerListRef.get` | 1/1 (100%) | 3/3 | 1 | PENDING |
| `SequencerListRef.goToNext` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `SequencerListRef.isValid` | 3/3 (100%) | 3/3 | 0 | PASS |
| `SequencerListRef.reset` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SequencerListRef.size` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SequencerTrigger.getAction` | 4/4 (100%) | 1/1 | 1 | PENDING |
| `SequencerTrigger.getLooped` | 1/1 (100%) | 7/7 | 0 | PASS |
| `SequencerTrigger.getPattern` | 1/1 (100%) | 8/8 | 0 | PASS |
| `SequencerTrigger.getStep` | 1/1 (100%) | 8/8 | 0 | PASS |
| `SequencerTriggerList.get` | 4/4 (100%) | 4/4 | 1 | PENDING |
| `SequencerTriggerList.goToNext` | 1/1 (100%) | 2/2 | 1 | PENDING |
| `SequencerTriggerList.isValid` | 1/1 (100%) | 4/4 | 0 | PASS |
| `SequencerTriggerList.reset` | 1/1 (100%) | 2/2 | 0 | PASS |
| `SequencerTriggerList.size` | 1/1 (100%) | 3/3 | 0 | PASS |

## Planned cases

| Case | Functions | Why |
| --- | --- | --- |
| BPM zero and negative | `Sequencer.setBPM`, `Sequencer.getMSPerStep` | Capture native float and tick behavior with a disposable sequencer. |
| BPM float edge conversion | `Sequencer.setBPM`, `Sequencer.getBPM` | Check fractions, NaN, and values outside signed int range. |
| Pattern length zero and negative | `Sequencer.setNumStepsPerPattern`, `Sequencer.getWrappedStep`, `Sequencer.start` | Compare invalid lengths without using the main game sequencer. |
| Loop length zero and negative | `Sequencer.setLoopLen`, `Sequencer.getCurrentPattern` | Check wrap at the first boundary. |
| Note type outside 0 to 5 | `Sequencer.getNextNoteStep`, `Sequencer.getNextNotePattern` | Retail can divide by zero; record as an unsafe case. |
| Time getter tick boundaries | `Sequencer.getMSTillNextStep`, `Sequencer.getMSTillNextStepAccurate`, `Sequencer.getMSTillNextStep16` | Use controlled clock steps so exact values are repeatable. |
| Volume and pan effect | `Sequencer.setVolume`, `Sequencer.setPan` | Compare audible output or a recovered native state reader; return values alone are shallow. |
| Master pulse consumer | `SEQUENCER_SetMasterPulse` | Trace reads of the retail global and compare the affected behavior. |
| Trigger overload type matrix | `Sequencer.setTrigger`, `Sequencer.setTrigger16`, `Sequencer.setTriggerOnNext` | Cover supported int, float, bool, null, and wrong arity combinations. |
| Duplicate trigger coordinates | `Sequencer.setTrigger`, `Sequencer.moveTriggerToFront` | Check stable order after insertion and moving first, middle, and tail. |
| Restart at future step | `Sequencer.restart`, `Sequencer.setTrigger` | Compare same-step callback ordering and one-shot removal. |
| Callback mutation | `Sequencer.clearTriggers`, `Sequencer.setTrigger`, `SequencerAction.tagForRemoval` | Append, tag, and clear inside a callback; check staged dispatch. |
| Action wrapper lifetime | `SequencerTrigger.getAction`, `SequencerAction.setTable`, `SequencerAction.tagForRemoval` | Verify alias state after GC, trigger removal, and table replacement. |
| Trigger cursor during mutation | `Sequencer.getAllTriggers`, `SequencerTriggerList.get`, `SequencerTriggerList.goToNext` | Check insertion and deletion while a cursor is held; avoid retail stale-pointer crash. |
| Sequencer cursor during mutation | `SEQUENCER_GetAllSequencers`, `SequencerListRef.get`, `SequencerListRef.goToNext` | Compare visibility and ownership when a sequencer is created or collected. |
| Callback and GC interleave | `SEQUENCERACTION`, `SequencerAction.setTable`, `Sequencer.setTrigger` | Collect between trigger scheduling and dispatch. |
| Malformed packed sequence | `SEQUENCE_FromTable`, `SEQUENCE_ToTable` | Check short, sparse, oversized, and noninteger tables. |
| Malformed sequence string | `SEQUENCE_FromString`, `SEQUENCE_ToString` | Check invalid prefix, invalid digits, embedded null, and long strings. |
| Object ID allocation order | `Sequencer.getObjectID`, `SEQUENCER`, `Sequencer.setTrigger` | Compare ID consumption by sequencers and trigger handles. |

Documented sequencer APIs missing from bindings: **0**.

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
| `Sequencer.setVolume` | Sequencer.setVolume.return | `null` | `null` | PASS |
| `Sequencer.setPan` | Sequencer.setPan.return | `null` | `null` | PASS |
| `Sequencer.test` | Sequencer.test.return | `null` | `null` | PASS |
| `SEQUENCER_OnNoteCollect` | SEQUENCER_OnNoteCollect.return | `null` | `null` | PASS |
| `SEQUENCER_SetMasterPulse` | SEQUENCER_SetMasterPulse.return | `null` | `null` | PASS |
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
| `SequencerTrigger.getAction` | SequencerTrigger.getAction.fresh | `1` | `1` | PASS |
| `SequencerTrigger.getAction` | SequencerTrigger.getAction.repeat | `1` | `1` | PASS |
| `SequencerTrigger.getAction` | SequencerTrigger.getAction.sharedTable | `11` | `11` | PASS |
| `SequencerTrigger.getAction` | SequencerTrigger.getAction.sharedTag | `1` | `1` | PASS |
| `Sequencer.setTrigger16` | Sequencer.setTrigger16.step | `2` | `2` | PASS |
| `Sequencer.setTriggerOnNext` | Sequencer.setTriggerOnNext.pattern | `0` | `0` | PASS |
| `Sequencer.setTriggerOnNext` | Sequencer.setTriggerOnNext.step | `8` | `8` | PASS |
| `Sequencer.getNumTriggers` | Sequencer.getNumTriggers.three | `3` | `3` | PASS |
| `SequencerTriggerList.size` | SequencerTriggerList.size.three | `3` | `3` | PASS |
| `SequencerTriggerList.isValid` | SequencerTriggerList.isValid.first | `1` | `1` | PASS |
| `SequencerTriggerList.get` | SequencerTriggerList.get.fresh | `1` | `1` | PASS |
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
| `Sequencer.getObjectID` | Sequencer.getObjectID.unique | `1` | `1` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.started | `1` | `1` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.paused | `0` | `0` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.unpaused | `1` | `1` | PASS |
| `Sequencer.isPlaying` | Sequencer.isPlaying.stopped | `0` | `0` | PASS |
| `Sequencer.getCurrentStep` | Sequencer.getCurrentStep.restarted | `-1` | `-1` | PASS |
| `Sequencer.moveTriggerToFront` | Sequencer.moveTriggerToFront.first | `2` | `2` | PASS |
| `Sequencer.setTrigger` | Sequencer.callback.step | `0` | `0` | PASS |
| `Sequencer.getMSTillNextStep` | Sequencer.getMSTillNextStep.positive | `1` | `1` | PASS |
| `Sequencer.getMSTillNextStepAccurate` | Sequencer.getMSTillNextStepAccurate.same | `1` | `1` | PASS |
| `Sequencer.getMSTillNextStep16` | Sequencer.getMSTillNextStep16.double | `1` | `1` | PASS |
| `Sequencer.setTrigger` | Sequencer.callback.count | `1` | `1` | PASS |
| `Sequencer.getNumTriggers` | Sequencer.getNumTriggers.afterCallback | `0` | `0` | PASS |
| `SequencerListRef.size` | SequencerListRef.size.initial | `3` | `3` | PASS |
| `SequencerListRef.isValid` | SequencerListRef.isValid.initial | `1` | `1` | PASS |
| `SequencerListRef.get` | SequencerListRef.get.initial | `1` | `1` | PASS |
| `SequencerListRef.isValid` | SequencerListRef.isValid.next | `1` | `1` | PASS |
| `SequencerListRef.isValid` | SequencerListRef.isValid.reset | `1` | `1` | PASS |

Local assertion details: `tests/_results/assertions.md`.
