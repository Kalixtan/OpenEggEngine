# String helper probes

Run headless with `python3 tests/run.py --suite stringhelper --report reports/STRINGHELPER.md`.
The suite checks fourteen functions. `MakeTimeTable` table shape and time edges are
covered from the PS3 wrapper trace. Retail symbol `STRINGHELPER_Char` has no OpenEgg
binding; its saved decomp shows a one-byte string return from an uninitialized
local buffer, so its usable script contract is not established. Most of the 30
StringHelper symbols still have no direct assertions.

The suite is not a retail log comparison.
