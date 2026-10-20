# Sequencer checks

The core `probe.gm` and the edge probes run unchanged on both engines. They
print named values as `OEP__case__value.gm` load errors; no probe files need to
exist. Risky inputs have separate scripts so a retail crash cannot hide others.
`OEPV__` marks clock dependent values shown in the logs but excluded from scores.

To refresh the retail baseline from your own extracted game installation:

```sh
python3 tests/capture_sequencer_suite.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN
```

Each run restores `Main.gm`. Every baseline records its script hash; changing
a probe requires a new retail capture. Logs stay in `tests/_results/sequencer/`.

From this repo root:

```sh
python3 tests/compare_probe.py --retail-log /path/to/TTY.log
```

The command runs OpenEgg headlessly, compares exact output strings, runs the local
assertion scripts, and writes `TEST_REPORT.md`. Missing or duplicate readings
fail the comparison. `--strict-parity` also fails while coverage limits or
known gaps remain. `python3 tests/run.py` runs only the
local assertions; section scripts stay separate so cases are easy to edit.

The checked-in baselines let the default command work without RPCS3.
`planned.json` maps the 19 edge groups and records what each still misses.
Matching output supports parity for tested inputs and visible results; hidden
side effects and untested behavior remain open.

`unsafe_inputs/` holds isolated retail crash probes. See `retail-crashes.md`
before using those calls in scripts that must run on PS3.
