# Animation probes

Run playback checks with extracted game data:

```sh
python3 tests/animation/run.py --game-data /path/to/game_data
```

This copies one FSA to a temporary data root. No retail files enter Git. Results
go to `reports/ANIMATION.md`.

Capture matching readings from RPCS3:

```sh
python3 tests/capture_retail_probe.py \
  --main /path/to/installed/game_data/Main.gm \
  --eboot /path/to/installed/USRDIR/EBOOT.BIN \
  --probe tests/animation/probe.gm \
  --baseline tests/animation/retail-baseline.tsv
```

The retail probe uses the same extracted FSA path and must be run with the
matching retail game data installed. A capture that produces no complete TTY
readings is inconclusive; it is not a parity failure.

Missing-resource cases are separate because retail faults when the FSA is
absent. Capture them on their own with `--probe tests/animation/missing_resource.gm`
or `--probe tests/animation/entity_missing_resource.gm`. OpenEgg returns an
empty animation safely and warns; see `retail-crashes.md`.
