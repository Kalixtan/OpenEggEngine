# String probes

Run headless with `python3 tests/run.py --suite stringhelper --report reports/STRINGHELPER.md`.
The suite covers GameMonkey string methods and StringHelper functions. Fourteen of
the thirty StringHelper symbols have assertions; retail `STRINGHELPER_Char` reads
an uninitialized byte, so its usable contract is unknown.

Checks run against OpenEgg; they are not retail log comparisons.
