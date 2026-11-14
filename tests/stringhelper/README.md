# String probes

Run headless with `python3 tests/run.py --suite stringhelper --report reports/STRINGHELPER.md`.
The suite covers all listed GameMonkey string methods and 29 of 30 StringHelper
symbols. Retail `STRINGHELPER_Char` reads an uninitialized byte, so its usable
contract is unknown.

Checks run against OpenEgg; they are not retail log comparisons.
