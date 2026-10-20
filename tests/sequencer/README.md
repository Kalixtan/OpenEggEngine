# Sequencer checks

`probe.gm` is the same script to load in both engines. It prints named values
as `OEP__function.case__value.gm` load errors; no probe files need to exist.
Each reading sets up its own sequencer state first.
`OEPV__` marks clock dependent values shown in the logs but excluded from scores.

To refresh the retail baseline from your own extracted game installation:

```sh
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN
```

The command restores `Main.gm` after RPCS3 exits. The baseline records a hash
of `probe.gm`; changing the probe requires a new retail capture.

From this repo root:

```sh
python3 tests/compare_probe.py --retail-log /path/to/TTY.log
```

The command runs OpenEgg headlessly, compares exact output strings, runs the local
assertion scripts, and writes `TEST_REPORT.md`. Missing or duplicate readings
fail the comparison. `--strict-parity` also fails while planned paired cases
remain or known gaps exist. `python3 tests/run.py` runs only the
local assertions; section scripts stay separate so cases are easy to edit.

The checked-in `retail-baseline.tsv` contains captured readings, so the
default command works without RPCS3. `planned.json` lists untested edge
cases. Matching output supports parity for the inputs and results this probe
actually covers; it does not prove untested behavior or hidden side effects.
