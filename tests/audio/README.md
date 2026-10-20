# Audio probes

`state.gm` checks script-visible values without game files. `voice.gm` uses
`Platformer/sounds/fx/button-high.wav` from your own extracted game data.
Neither probe checks the sound sent to the speakers.

Capture retail readings with your installed PS3 game:

```sh
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/audio/state.gm \
  --baseline tests/audio/state-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/audio/voice.gm \
  --baseline tests/audio/voice-retail.tsv
```

Compare headlessly:

```sh
python3 tests/audio/compare.py
python3 tests/audio/compare.py --game-data /path/to/game_data
```

The voice run makes a temporary copy of the supplied sound. Reports go to
`AUDIO_TEST_REPORT.md`; raw logs stay in `tests/_results/audio/`.

## Waveforms

`waveform.gm` plays one owned retail sound four ways: normal, half volume,
left, and right. Capture the PS3 output once, then reuse its WAVs:

```sh
python3 tests/audio/waveform.py --game-data /path/to/game_data \
  --capture-retail --main /path/to/installed/game_data/Main.gm \
  --eboot /path/to/installed/EBOOT.BIN
python3 tests/audio/waveform.py --game-data /path/to/game_data
```

Needs RPCS3, `pactl`, `parec`, NumPy, SciPy, and SoundFile. The capture uses a
temporary RPCS3 config and virtual sink. It briefly appends the probe to the
installed `Main.gm`, then restores it. Retail and OpenEgg audio stays under
ignored `tests/_results/audio/waveforms/`; only the comparison report is tracked.
