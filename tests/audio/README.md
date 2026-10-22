# Audio probes

`state.gm`, `manager.gm`, `effects.gm`, and `script_playback.gm` check
script-visible state without game files. Voice probes copy sound samples from
your extracted game data into a temporary folder. These probes check script
responses; `waveform.gm` checks emitted audio.
`voice_lifetime.gm` checks voice completion, fades, pause/resume, loops, unload,
and stop-all across process updates. `voice_gate.gm` isolates gate expiry and
voice validity. `OEPV__` marks playback-clock samples that
are printed for review but excluded from deterministic score comparisons.

Capture retail readings with your installed PS3 game:

```sh
for probe in state manager effects script_playback voice voice_controls slicker voice_lifetime voice_gate; do
  python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
    --eboot /path/to/EBOOT.BIN --probe "tests/audio/$probe.gm" \
    --baseline "tests/audio/$probe-retail.tsv"
done
```

Compare headlessly:

```sh
python3 tests/audio/compare.py
python3 tests/audio/compare.py --game-data /path/to/game_data
```

The asset runs make a temporary copy of the supplied sound. Reports go to
`reports/AUDIO.md`; raw logs stay in `tests/_results/audio/`.
The report flags failed retail setup when its captured readings or TTY log show
no voice was created. These warnings only inspect saved capture data; they do
not call audio functions or change playback state.

## Waveforms

`waveform.gm` plays one owned retail sound four ways: normal, half volume,
left, and right. Capture the PS3 output once, then reuse its WAVs:

```sh
python3 tests/audio/waveform.py --game-data /path/to/game_data \
  --capture-retail --main /path/to/installed/game_data/Main.gm \
  --eboot /path/to/installed/EBOOT.BIN
python3 tests/audio/waveform.py --game-data /path/to/game_data
```

`waveform_controls.gm` checks immediate and deferred speed changes, fade, loop,
pause/resume, and voice pan. It uses a
separate capture and report:

```sh
python3 tests/audio/waveform.py --suite controls --game-data /path/to/game_data \
  --capture-retail --main /path/to/installed/game_data/Main.gm \
  --eboot /path/to/installed/EBOOT.BIN
python3 tests/audio/waveform.py --suite controls --game-data /path/to/game_data
```

`waveform_looping.gm` compares loop counts 0 through 3, plus an indefinite loop
with a timed stop. It writes a separate report:

```sh
python3 tests/audio/waveform.py --suite looping --game-data /path/to/game_data \
  --capture-retail --main /path/to/installed/game_data/Main.gm \
  --eboot /path/to/installed/EBOOT.BIN
python3 tests/audio/waveform.py --suite looping --game-data /path/to/game_data
```

Needs RPCS3, `pactl`, `parec`, NumPy, SciPy, and SoundFile. The capture uses a
temporary RPCS3 config and virtual sink. It briefly appends the probe to the
installed `Main.gm`, then restores it. Retail and OpenEgg audio stays under
ignored `tests/_results/audio/waveforms/`; only the comparison report is tracked.
