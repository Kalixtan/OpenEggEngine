# Sequencer checks

`probe.gm` is the single script to load in both engines. It prints named values
as `OEP__function.case__value.gm` load errors; no probe files need to exist.
Each reading sets up its own sequencer state first.
`OEPV__` marks clock dependent values shown in the logs but excluded from scores.

To refresh the retail baseline, append `probe.gm` to a **copy** of the game's
extracted `Main.gm`, temporarily install that copy, run the PS3 build in RPCS3,
then save `~/.cache/rpcs3/TTY.log`. Restore the original `Main.gm` afterward.

From this repo root:

```sh
python3 tests/compare_probe.py --retail-log /path/to/TTY.log
```

The command runs OpenEgg headlessly, compares both logs, runs the local
assertion scripts, and writes `TEST_REPORT.md`. Use `--strict-parity` to
return an error for known gaps too. `python3 tests/run.py` runs only the
local assertions; section scripts stay separate so cases are easy to edit.

The checked-in `retail-baseline.tsv` contains captured readings, so the
default command works without RPCS3. `planned.json` lists untested edge
cases and keeps a 100% score from being mistaken for complete parity.
