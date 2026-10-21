# Test reports

| Report | Contents |
| --- | --- |
| `SEQUENCER.md` | Sequencer output comparison and local assertions |
| `PHYSICS.md` | Physics and entity output comparison |
| `AUDIO.md` | Script-visible audio comparisons |
| `AUDIO_WAVEFORM.md` | Basic waveform comparisons |
| `AUDIO_CONTROLS_WAVEFORM.md` | Audio control waveform comparisons |
| `ANIMATION.md` | Animation resource and playback checks |
| `ANIMATION_EXTENDED.md` | FSA bounds, negative indices, entity clock |
| `LOCAL_ASSERTIONS.md` | Local sequencer assertion details |

Refresh paired reports with `python3 tests/compare_all.py`. Run waveform and
animation probes using the commands in their test READMEs. Raw logs and captured
waveforms stay under ignored `tests/_results/`.

Scores cover captured cases only; they do not measure total engine completion.
