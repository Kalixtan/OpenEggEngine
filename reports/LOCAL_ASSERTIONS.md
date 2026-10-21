# Test report

Generated: 2026-10-21 17:27 UTC
Suite: sequencer
Engine: `/home/leia/Documents/GitHub/OpenEggEngine/build/OpenEgg`
Result: **49/49 checks passed (100%); 0 known gaps**

Scores count passing test cases. They do not measure total engine completeness. One integration case may count for several functions.

| System | Section | Passed | Total | Rate |
| --- | --- | ---: | ---: | ---: |
| Sequencer | Core | 4 | 4 | 100% |
| Sequencer | State | 5 | 5 | 100% |
| Sequencer | Manager | 4 | 4 | 100% |
| Sequencer | Timers | 3 | 3 | 100% |
| Sequencer | Loops | 2 | 2 | 100% |
| Sequencer | Safety | 3 | 3 | 100% |
| Sequencer | Trigger forms | 13 | 13 | 100% |
| Sequencer | Actions | 6 | 6 | 100% |
| Sequencer | Sequence conversion | 9 | 9 | 100% |

## Functions

| Function | Passed | Cases | Score | Status |
| --- | ---: | ---: | ---: | --- |
| `SEQUENCER` | 2 | 2 | 100% | PASS |
| `SEQUENCERACTION` | 3 | 3 | 100% | PASS |
| `SEQUENCER_GetAllSequencers` | 1 | 1 | 100% | PASS |
| `SEQUENCER_OnNoteCollect` | 0 | 0 | — | untested |
| `SEQUENCER_PauseAllSequencers` | 1 | 1 | 100% | PASS |
| `SEQUENCER_SetMasterPulse` | 0 | 0 | — | untested |
| `SEQUENCER_UnpauseAllSequencers` | 1 | 1 | 100% | PASS |
| `SEQUENCE_FromString` | 4 | 4 | 100% | PASS |
| `SEQUENCE_FromTable` | 3 | 3 | 100% | PASS |
| `SEQUENCE_ToString` | 3 | 3 | 100% | PASS |
| `SEQUENCE_ToTable` | 3 | 3 | 100% | PASS |
| `Sequencer.clearTriggers` | 2 | 2 | 100% | PASS |
| `Sequencer.getAllTriggers` | 1 | 1 | 100% | PASS |
| `Sequencer.getBPM` | 2 | 2 | 100% | PASS |
| `Sequencer.getCurrentPattern` | 5 | 5 | 100% | PASS |
| `Sequencer.getCurrentStep` | 5 | 5 | 100% | PASS |
| `Sequencer.getCurrentStep16` | 0 | 0 | — | untested |
| `Sequencer.getMSPerStep` | 4 | 4 | 100% | PASS |
| `Sequencer.getMSTillNextNoteStep` | 0 | 0 | — | untested |
| `Sequencer.getMSTillNextStep` | 4 | 4 | 100% | PASS |
| `Sequencer.getMSTillNextStep16` | 1 | 1 | 100% | PASS |
| `Sequencer.getMSTillNextStepAccurate` | 1 | 1 | 100% | PASS |
| `Sequencer.getNextNotePattern` | 2 | 2 | 100% | PASS |
| `Sequencer.getNextNoteStep` | 2 | 2 | 100% | PASS |
| `Sequencer.getNoteTime` | 1 | 1 | 100% | PASS |
| `Sequencer.getNumStepsPerNote` | 1 | 1 | 100% | PASS |
| `Sequencer.getNumStepsPerPattern` | 3 | 3 | 100% | PASS |
| `Sequencer.getNumTriggers` | 6 | 6 | 100% | PASS |
| `Sequencer.getObjectID` | 1 | 1 | 100% | PASS |
| `Sequencer.getWrappedStep` | 0 | 0 | — | untested |
| `Sequencer.gotoStep` | 1 | 1 | 100% | PASS |
| `Sequencer.isPlaying` | 3 | 3 | 100% | PASS |
| `Sequencer.moveTriggerToFront` | 2 | 2 | 100% | PASS |
| `Sequencer.restart` | 1 | 1 | 100% | PASS |
| `Sequencer.setBPM` | 3 | 3 | 100% | PASS |
| `Sequencer.setCurrentStep` | 1 | 1 | 100% | PASS |
| `Sequencer.setLoopLen` | 1 | 1 | 100% | PASS |
| `Sequencer.setNumStepsPerPattern` | 2 | 2 | 100% | PASS |
| `Sequencer.setPan` | 0 | 0 | — | untested |
| `Sequencer.setTrigger` | 9 | 9 | 100% | PASS |
| `Sequencer.setTrigger16` | 2 | 2 | 100% | PASS |
| `Sequencer.setTriggerOnNext` | 1 | 1 | 100% | PASS |
| `Sequencer.setVolume` | 0 | 0 | — | untested |
| `Sequencer.start` | 4 | 4 | 100% | PASS |
| `Sequencer.stop` | 1 | 1 | 100% | PASS |
| `Sequencer.test` | 0 | 0 | — | untested |
| `SequencerAction.isTaggedForRemoval` | 2 | 2 | 100% | PASS |
| `SequencerAction.setTable` | 2 | 2 | 100% | PASS |
| `SequencerAction.tagForRemoval` | 3 | 3 | 100% | PASS |
| `SequencerListRef.get` | 3 | 3 | 100% | PASS |
| `SequencerListRef.goToNext` | 2 | 2 | 100% | PASS |
| `SequencerListRef.isValid` | 3 | 3 | 100% | PASS |
| `SequencerListRef.reset` | 1 | 1 | 100% | PASS |
| `SequencerListRef.size` | 1 | 1 | 100% | PASS |
| `SequencerTrigger.getAction` | 1 | 1 | 100% | PASS |
| `SequencerTrigger.getLooped` | 7 | 7 | 100% | PASS |
| `SequencerTrigger.getPattern` | 8 | 8 | 100% | PASS |
| `SequencerTrigger.getStep` | 8 | 8 | 100% | PASS |
| `SequencerTriggerList.get` | 4 | 4 | 100% | PASS |
| `SequencerTriggerList.goToNext` | 2 | 2 | 100% | PASS |
| `SequencerTriggerList.isValid` | 4 | 4 | 100% | PASS |
| `SequencerTriggerList.reset` | 2 | 2 | 100% | PASS |
| `SequencerTriggerList.size` | 3 | 3 | 100% | PASS |

## Checks

| System / section | Test | Result | Retail basis |
| --- | --- | --- | --- |
| Sequencer / Core | SEQ conversion | PASS | local check |
| Sequencer / Core | SEQ order | PASS | paired probe |
| Sequencer / Core | SEQ removal | PASS | paired probe |
| Sequencer / Core | SEQ timing | PASS | paired probe |
| Sequencer / State | SEQ initial state | PASS | paired probe |
| Sequencer / State | SEQ tempo and notes | PASS | paired probe |
| Sequencer / State | SEQ next note | PASS | paired probe |
| Sequencer / State | SEQ raw step | PASS | paired probe |
| Sequencer / State | SEQ playback state | PASS | paired probe |
| Sequencer / Manager | SEQ global walk | PASS | paired probe |
| Sequencer / Manager | SEQ global wrap | PASS | paired probe |
| Sequencer / Manager | SEQ global reset | PASS | paired probe |
| Sequencer / Manager | SEQ global pause | PASS | paired probe |
| Sequencer / Timers | SEQ timer keep | PASS | paired probe |
| Sequencer / Timers | SEQ pattern timer reset | PASS | paired probe |
| Sequencer / Timers | SEQ BPM timer reset | PASS | paired probe |
| Sequencer / Loops | SEQ loop first | PASS | paired probe |
| Sequencer / Loops | SEQ loop second | PASS | paired probe |
| Sequencer / Safety | SEQ cleared cursor safe | PASS | safe difference |
| Sequencer / Safety | SEQ cursor reset | PASS | safe difference |
| Sequencer / Safety | SEQ tail move safe | PASS | safe difference |
| Sequencer / Trigger forms | SEQ trigger implicit | PASS | paired probe |
| Sequencer / Trigger forms | SEQ action wrapper identity | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger implicit flag | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger explicit | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger explicit flag | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger16 implicit | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger16 explicit | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger next | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger count | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger list walk | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger list reset | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger wrapper identity | PASS | paired probe |
| Sequencer / Trigger forms | SEQ trigger clear | PASS | paired probe |
| Sequencer / Actions | SEQ action fields | PASS | paired probe |
| Sequencer / Actions | SEQ action set table | PASS | paired probe |
| Sequencer / Actions | SEQ action table constructor | PASS | paired probe |
| Sequencer / Actions | SEQ action tag | PASS | paired probe |
| Sequencer / Actions | SEQ action GC table | PASS | paired probe |
| Sequencer / Actions | SEQ action stopped removal | PASS | paired probe |
| Sequencer / Sequence conversion | SEQ pack word | PASS | local check |
| Sequencer / Sequence conversion | SEQ unpack word | PASS | local check |
| Sequencer / Sequence conversion | SEQ encode string | PASS | local check |
| Sequencer / Sequence conversion | SEQ decode string | PASS | local check |
| Sequencer / Sequence conversion | SEQ invalid prefix | PASS | local check |
| Sequencer / Sequence conversion | SEQ raw character | PASS | local check |
| Sequencer / Sequence conversion | SEQ state three stop | PASS | local check |
| Sequencer / Sequence conversion | SEQ partial word | PASS | local check |
| Sequencer / Sequence conversion | SEQ empty packed | PASS | local check |

Known safety differences: retail crashes when moving the final trigger and leaves a stale cursor after `clearTriggers()`; OpenEgg handles both safely.
