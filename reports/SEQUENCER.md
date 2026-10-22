# Sequencer test report

Generated: 2026-10-22 00:34 UTC
Retail: `tests/sequencer/retail-baseline.tsv` and `tests/sequencer/edge-baselines/` | OpenEgg: `tests/_results/sequencer/`

Matched: **204/204 declared readings (100%)** | Known gaps: **0** | Failures: **0** | Uncaptured: **0** | Edge groups run: **20** | Clock samples: **4** | Undefined retail outputs: **3**

Local assertions: **49/49 passed (100%)**

Verdict: **all captured readings match**. Coverage is limited to the readings below.

One row is one exact string comparison from the same probe file on both engines. A missing reading or duplicate fails the run. Scores measure tested cases only; they are not total implementation progress. Integration checks can count for several functions. Clock samples stay visible but outside the score. Retail's sequencer reads elapsed wall time, so frame-count samples vary between runs.

Retail marks final packed words in 512-step `SEQUENCE_ToTable` cases undefined for scoring: recovered routine clears only first 16-word scratch block, then ORs into remaining uninitialized stack words. Captured values stay listed below.

## Local assertion sections

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

| Function | Paired score | Local checks | Edge groups | Status |
| --- | ---: | ---: | ---: | --- |
| `SEQUENCER` | 2/2 (100%) | 2/2 | 1 | LIMITED |
| `SEQUENCERACTION` | 3/3 (100%) | 3/3 | 1 | LIMITED |
| `SEQUENCER_GetAllSequencers` | 3/3 (100%) | 1/1 | 1 | LIMITED |
| `SEQUENCER_OnNoteCollect` | 1/1 (100%) | 0/0 | 0 | PASS |
| `SEQUENCER_PauseAllSequencers` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SEQUENCER_SetMasterPulse` | 2/2 (100%) | 0/0 | 1 | LIMITED |
| `SEQUENCER_UnpauseAllSequencers` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SEQUENCE_FromString` | 9/9 (100%) | 4/4 | 1 | LIMITED |
| `SEQUENCE_FromTable` | 9/9 (100%) | 3/3 | 1 | LIMITED |
| `SEQUENCE_ToString` | 8/8 (100%) | 3/3 | 1 | LIMITED |
| `SEQUENCE_ToTable` | 10/10 (100%) | 3/3 | 1 | LIMITED |
| `Sequencer.clearTriggers` | 4/4 (100%) | 2/2 | 1 | LIMITED |
| `Sequencer.getAllTriggers` | 34/34 (100%) | 1/1 | 2 | LIMITED |
| `Sequencer.getBPM` | 7/7 (100%) | 2/2 | 1 | LIMITED |
| `Sequencer.getCurrentPattern` | 6/6 (100%) | 5/5 | 1 | LIMITED |
| `Sequencer.getCurrentStep` | 10/10 (100%) | 5/5 | 0 | PASS |
| `Sequencer.getCurrentStep16` | 1/1 (100%) | 0/0 | 0 | PASS |
| `Sequencer.getMSPerStep` | 6/6 (100%) | 4/4 | 1 | LIMITED |
| `Sequencer.getMSTillNextNoteStep` | 1/1 (100%) | 0/0 | 0 | PASS |
| `Sequencer.getMSTillNextStep` | 4/4 (100%) | 4/4 | 1 | LIMITED |
| `Sequencer.getMSTillNextStep16` | 4/4 (100%) | 1/1 | 1 | LIMITED |
| `Sequencer.getMSTillNextStepAccurate` | 4/4 (100%) | 1/1 | 1 | LIMITED |
| `Sequencer.getNextNotePattern` | 12/12 (100%) | 2/2 | 1 | LIMITED |
| `Sequencer.getNextNoteStep` | 12/12 (100%) | 2/2 | 1 | LIMITED |
| `Sequencer.getNoteTime` | 2/2 (100%) | 1/1 | 0 | PASS |
| `Sequencer.getNumStepsPerNote` | 3/3 (100%) | 1/1 | 0 | PASS |
| `Sequencer.getNumStepsPerPattern` | 2/2 (100%) | 3/3 | 0 | PASS |
| `Sequencer.getNumTriggers` | 3/3 (100%) | 6/6 | 0 | PASS |
| `Sequencer.getObjectID` | 2/2 (100%) | 1/1 | 1 | LIMITED |
| `Sequencer.getWrappedStep` | 25/25 (100%) | 0/0 | 1 | LIMITED |
| `Sequencer.gotoStep` | 1/1 (100%) | 1/1 | 0 | PASS |
| `Sequencer.isPlaying` | 5/5 (100%) | 3/3 | 0 | PASS |
| `Sequencer.moveTriggerToFront` | 19/19 (100%) | 2/2 | 2 | LIMITED |
| `Sequencer.restart` | 5/5 (100%) | 1/1 | 1 | LIMITED |
| `Sequencer.setBPM` | 13/13 (100%) | 3/3 | 2 | LIMITED |
| `Sequencer.setCurrentStep` | 1/1 (100%) | 1/1 | 0 | PASS |
| `Sequencer.setLoopLen` | 5/5 (100%) | 1/1 | 1 | LIMITED |
| `Sequencer.setNumStepsPerPattern` | 26/26 (100%) | 2/2 | 1 | LIMITED |
| `Sequencer.setPan` | 3/3 (100%) | 0/0 | 1 | LIMITED |
| `Sequencer.setTrigger` | 21/21 (100%) | 9/9 | 6 | LIMITED |
| `Sequencer.setTrigger16` | 6/6 (100%) | 2/2 | 1 | LIMITED |
| `Sequencer.setTriggerOnNext` | 7/7 (100%) | 1/1 | 1 | LIMITED |
| `Sequencer.setVolume` | 3/3 (100%) | 0/0 | 1 | LIMITED |
| `Sequencer.start` | 24/24 (100%) | 4/4 | 1 | LIMITED |
| `Sequencer.stop` | 1/1 (100%) | 1/1 | 0 | PASS |
| `Sequencer.test` | 1/1 (100%) | 0/0 | 0 | PASS |
| `SequencerAction.isTaggedForRemoval` | 2/2 (100%) | 2/2 | 0 | PASS |
| `SequencerAction.setTable` | 5/5 (100%) | 2/2 | 2 | LIMITED |
| `SequencerAction.tagForRemoval` | 7/7 (100%) | 3/3 | 2 | LIMITED |
| `SequencerListRef.get` | 3/3 (100%) | 3/3 | 1 | LIMITED |
| `SequencerListRef.goToNext` | 3/3 (100%) | 2/2 | 1 | LIMITED |
| `SequencerListRef.isValid` | 3/3 (100%) | 3/3 | 0 | PASS |
| `SequencerListRef.reset` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SequencerListRef.size` | 1/1 (100%) | 1/1 | 0 | PASS |
| `SequencerTrigger.getAction` | 7/7 (100%) | 1/1 | 1 | LIMITED |
| `SequencerTrigger.getLooped` | 1/1 (100%) | 7/7 | 0 | PASS |
| `SequencerTrigger.getPattern` | 1/1 (100%) | 8/8 | 0 | PASS |
| `SequencerTrigger.getStep` | 1/1 (100%) | 8/8 | 0 | PASS |
| `SequencerTriggerList.get` | 22/22 (100%) | 4/4 | 1 | LIMITED |
| `SequencerTriggerList.goToNext` | 34/34 (100%) | 2/2 | 2 | LIMITED |
| `SequencerTriggerList.isValid` | 1/1 (100%) | 4/4 | 0 | PASS |
| `SequencerTriggerList.reset` | 1/1 (100%) | 2/2 | 0 | PASS |
| `SequencerTriggerList.size` | 1/1 (100%) | 3/3 | 0 | PASS |

## Edge cases

| Case | Matched | Limits |
| --- | ---: | --- |
| BPM zero and negative | 7/7 | Zero and negative BPM sampled through 12 updates; longer runs are open. |
| BPM float edge conversion | 5/5 | NaN is not covered; fractional truncation edges are captured. |
| Pattern length zero and negative | 25/25 | Tick wrap sampled through 12 updates; longer runs are open. |
| Loop length zero and negative | 4/4 | Observed after 12 updates. |
| Note type outside 0 to 5 | 11/11 | Covers 6 and -1. |
| Time getter tick boundaries | 3/3 | Raw clock values vary and are excluded. |
| Volume and pan effect | 2/2 | Return values only; audio output is not measured. |
| Master pulse consumer | 1/1 | Return value only; no consumer effect measured. |
| Trigger overload type matrix | 5/5 | Supported forms only; bad types and arity are not covered. |
| Duplicate trigger coordinates | 3/3 | Moving the last of multiple triggers crashes retail; see retail-crashes.md. |
| Restart at future step | 4/4 | Two same-step actions after restart are covered; other callback orders are open. |
| Callback mutation | 3/3 | One append, tag, and clear path. |
| Action wrapper lifetime | 3/3 | Clear and explicit GC covered. |
| Trigger cursor during mutation | 18/18 | Insertion, moving, and clearing after cursor advance are covered. Retail keeps a cleared cursor valid; OpenEgg invalidates it. Single-node deletion is not exposed. |
| Move trigger ordering | 15/15 | Moving the last of multiple triggers crashes retail; see retail-crashes.md. |
| Sequencer cursor during mutation | 2/2 | Creation only; collection is not covered. |
| Callback and GC interleave | 1/1 | One explicit GC before callback. |
| Malformed packed sequence | 9/9 | Empty, short, sparse, float, and oversized inputs are captured; retail's final packed word at 512 steps differs from OpenEgg. |
| Malformed sequence string | 7/7 | Invalid prefix, bad digit, long text, and embedded NUL are captured. |
| Object ID allocation order | 1/1 | One allocation interval. |

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
| `Sequencer.getMSTillNextStep` | edge.time.positive | `1` | `1` | PASS |
| `Sequencer.getMSTillNextStep` | edge.time.accurate | `1` | `1` | PASS |
| `Sequencer.getMSTillNextStep` | edge.time.sixteenth | `1` | `1` | PASS |
| `Sequencer.setVolume` | edge.volume.return | `null` | `null` | PASS |
| `Sequencer.setVolume` | edge.pan.return | `null` | `null` | PASS |
| `SEQUENCER_SetMasterPulse` | edge.master.return | `null` | `null` | PASS |
| `Sequencer.setTrigger` | edge.overload.two | `1` | `1` | PASS |
| `Sequencer.setTrigger` | edge.overload.three | `0` | `0` | PASS |
| `Sequencer.setTrigger` | edge.overload.four | `2` | `2` | PASS |
| `Sequencer.setTrigger` | edge.overload.sixteen | `2` | `2` | PASS |
| `Sequencer.setTrigger` | edge.overload.next | `2` | `2` | PASS |
| `Sequencer.setTrigger` | edge.duplicate.first | `2` | `2` | PASS |
| `Sequencer.setTrigger` | edge.duplicate.second | `1` | `1` | PASS |
| `Sequencer.setTrigger` | edge.duplicate.afterMove | `1` | `1` | PASS |
| `SequencerTrigger.getAction` | edge.lifetime.value | `12` | `12` | PASS |
| `SequencerTrigger.getAction` | edge.lifetime.table | `13` | `13` | PASS |
| `SequencerTrigger.getAction` | edge.lifetime.tag | `1` | `1` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerCursor.size | `2` | `2` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerCursor.step | `0` | `0` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerCursor.next | `1` | `1` | PASS |
| `SEQUENCER_GetAllSequencers` | edge.sequencerCursor.grew | `1` | `1` | PASS |
| `SEQUENCER_GetAllSequencers` | edge.sequencerCursor.valid | `1` | `1` | PASS |
| `Sequencer.restart` | edge.restart.step | `4` | `4` | PASS |
| `Sequencer.restart` | edge.restart.scheduled | `2` | `2` | PASS |
| `Sequencer.clearTriggers` | edge.mutation.added | `1` | `1` | PASS |
| `Sequencer.clearTriggers` | edge.mutation.tagged | `1` | `1` | PASS |
| `Sequencer.clearTriggers` | edge.mutation.cleared | `0` | `0` | PASS |
| `SEQUENCERACTION` | edge.gc.callback | `0` | `0` | PASS |
| `Sequencer.getObjectID` | edge.objectID.delta | `2` | `2` | PASS |
| `Sequencer.setBPM` | edge.bpm.zero | `0` | `0` | PASS |
| `Sequencer.setBPM` | edge.bpm.zeroPeriod | `inf` | `inf` | PASS |
| `Sequencer.setBPM` | edge.bpm.negative | `-1` | `-1` | PASS |
| `Sequencer.setBPM` | edge.bpm.negativePeriod | `-7499.999512` | `-7499.999512` | PASS |
| `Sequencer.setBPM` | edge.bpmFloat.positiveFraction | `0` | `0` | PASS |
| `Sequencer.setBPM` | edge.bpmFloat.negativeFraction | `0` | `0` | PASS |
| `Sequencer.setBPM` | edge.bpmFloat.positive | `1` | `1` | PASS |
| `Sequencer.setBPM` | edge.bpmFloat.negative | `-1` | `-1` | PASS |
| `Sequencer.setBPM` | edge.bpmFloat.overflow | `2147483647` | `2147483647` | PASS |
| `Sequencer.getCurrentStep` | edge.bpm.zeroTickInitial | `-1` | `-1` | PASS |
| `Sequencer.getCurrentStep` | edge.bpm.negativeTickInitial | `-1` | `-1` | PASS |
| `Sequencer.getCurrentStep` | edge.bpm.zeroTickAfter | `0` | `0` | PASS |
| `Sequencer.getCurrentStep` | edge.bpm.negativeTickAfter | `11` | `12` | CLOCK |
| `Sequencer.getAllTriggers` | edge.cursorMove.beforeFirst | `0` | `0` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.beforeSecond | `1` | `1` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.beforeThird | `2` | `2` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.before | `1` | `1` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.freshFirst | `1` | `1` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.freshSecond | `0` | `0` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.freshThird | `2` | `2` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.valid | `1` | `1` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.after | `1` | `1` | PASS |
| `Sequencer.getAllTriggers` | edge.cursorMove.next | `0` | `0` | PASS |
| `Sequencer.setLoopLen` | edge.loop.zeroInitial | `0` | `0` | PASS |
| `Sequencer.setLoopLen` | edge.loop.negativeInitial | `0` | `0` | PASS |
| `Sequencer.setLoopLen` | edge.loop.zeroAfter | `0` | `0` | PASS |
| `Sequencer.setLoopLen` | edge.loop.negativeAfter | `0` | `0` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.mixed0 | `1` | `1` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.mixed1 | `0` | `0` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.mixed2 | `1` | `1` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.mixed3 | `2` | `2` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.repeat0 | `1` | `1` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.repeat1 | `2` | `2` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.repeat2 | `0` | `0` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.repeat3 | `3` | `3` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.head0 | `0` | `0` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.head1 | `1` | `1` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveMatrix.head2 | `2` | `2` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveOrder.four.0 | `0` | `0` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveOrder.four.1 | `2` | `2` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveOrder.four.2 | `1` | `1` | PASS |
| `Sequencer.moveTriggerToFront` | edge.moveOrder.four.3 | `3` | `3` | PASS |
| `Sequencer.getNextNoteStep` | edge.note.before | `1` | `1` | PASS |
| `Sequencer.getNextNoteStep` | edge.note.nextStep | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.note.nextPattern | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.startSixStep | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.startSixPattern | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.fiveSixStep | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.fiveSixPattern | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.fiveMinusStep | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.fiveMinusPattern | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.endSixStep | `0` | `0` | PASS |
| `Sequencer.getNextNoteStep` | edge.noteMatrix.endSixPattern | `0` | `0` | PASS |
| `SEQUENCE_FromTable` | edge.packed.empty | `null` | `null` | PASS |
| `SEQUENCE_FromTable` | edge.packed.shortSize | `16` | `16` | PASS |
| `SEQUENCE_FromTable` | edge.packed.sparseWord | `null` | `null` | PASS |
| `SEQUENCE_FromTable` | edge.packed.floatWord | `null` | `null` | PASS |
| `SEQUENCE_FromTable` | edge.packed.oversizedDecodedSize | `256` | `256` | PASS |
| `SEQUENCE_FromTable` | edge.packed.oversizedLastState | `1` | `1` | PASS |
| `SEQUENCE_FromTable` | edge.packed.oversizedIgnoredState | `null` | `null` | PASS |
| `SEQUENCE_ToTable` | edge.packed.oversizedSourceLast | `1` | `1` | PASS |
| `SEQUENCE_ToTable` | edge.packed.oversizedLastWord | `1440077661` | `1431655765` | UNDEFINED |
| `SEQUENCE_ToTable` | edge.packed.oversizedIgnoredWord | `null` | `null` | PASS |
| `SEQUENCE_ToTable` | edge.packed.exactLimitWord | `1440077661` | `1431655765` | UNDEFINED |
| `SEQUENCE_ToTable` | edge.packed.exactLimitWordRepeat | `1440077661` | `1431655765` | UNDEFINED |
| `Sequencer.setNumStepsPerPattern` | edge.pattern.zero | `0` | `0` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.pattern.zeroWrap | `-1` | `-1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.pattern.negative | `-2` | `-2` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.pattern.negativeWrap | `-3` | `-3` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.pattern.started | `1` | `1` | PASS |
| `Sequencer.getCurrentStep` | edge.pattern.zeroTickInitial | `-1` | `-1` | PASS |
| `Sequencer.getCurrentStep` | edge.pattern.negativeTickInitial | `-1` | `-1` | PASS |
| `Sequencer.getCurrentStep` | edge.pattern.zeroTickAfter | `8` | `12` | CLOCK |
| `Sequencer.getCurrentStep` | edge.pattern.negativeTickAfter | `8` | `12` | CLOCK |
| `Sequencer.getCurrentStep` | edge.restart.initialStep | `-1` | `-1` | PASS |
| `Sequencer.setTrigger` | edge.restart.actionOrder | `123` | `123` | PASS |
| `Sequencer.getCurrentStep` | edge.restart.currentStep | `3` | `4` | CLOCK |
| `SEQUENCE_FromString` | edge.string.prefix | `null` | `null` | PASS |
| `SEQUENCE_FromString` | edge.string.badDigit | `72` | `72` | PASS |
| `SEQUENCE_FromString` | edge.string.longSize | `128` | `128` | PASS |
| `SEQUENCE_FromString` | edge.string.nulResult | `1` | `1` | PASS |
| `SEQUENCE_FromString` | edge.string.nulSize | `2` | `2` | PASS |
| `SEQUENCE_FromString` | edge.string.nulLastValue | `2` | `2` | PASS |
| `SEQUENCE_FromString` | edge.string.nulSuffixIgnored | `null` | `null` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerClear.currentStep | `2` | `2` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerClear.size | `0` | `0` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerClear.valid | `1` | `1` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerClear.getNull | `0` | `0` | PASS |
| `Sequencer.getAllTriggers` | edge.triggerClear.resetInvalid | `1` | `1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.zero.n5 | `-5` | `-5` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.zero.n1 | `-1` | `-1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.zero.z | `0` | `0` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.zero.p1 | `1` | `1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.zero.p5 | `5` | `5` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.neg.n5 | `-3` | `-3` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.neg.n1 | `-3` | `-3` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.neg.z | `0` | `0` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.neg.p1 | `1` | `1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.neg.p5 | `1` | `1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.n8 | `1` | `1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.n4 | `2` | `2` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.n1 | `2` | `2` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.z | `0` | `0` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.p1 | `1` | `1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.p3 | `0` | `0` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.p4 | `1` | `1` | PASS |
| `Sequencer.setNumStepsPerPattern` | edge.wrap.pos.p8 | `2` | `2` | PASS |

## Volatile readings (excluded from score)

These samples vary with runtime timing and are retained for review.

| Reading | OpenEgg |
| --- | --- |
| `Sequencer.getMSTillNextStep.raw` | `45.833336` |
| `Sequencer.getMSTillNextStep16.raw` | `91.666672` |
| `Sequencer.getMSTillNextStepAccurate.raw` | `45.833336` |
| `edge.time.raw` | `45.833336` |

Retail crash warning: moving the last of multiple triggers faulted on PS3. See `tests/sequencer/retail-crashes.md`.

Local assertion details: `reports/LOCAL_ASSERTIONS.md`.
