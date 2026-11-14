# String helper probes

Run headless with `python3 tests/run.py --suite stringhelper --report reports/STRINGHELPER.md`.
The suite has eight checks over twelve functions. `MakeTimeTable` still has a known
return-type/schema gap. The retail symbol `STRINGHELPER_Char` has no OpenEgg
binding; its saved decomp shows a one-byte string return from an uninitialized
local buffer, so its usable script contract is not established. Most of the 30
StringHelper symbols still have no direct assertions.

Other open gaps: `MakeTimeString`'s formatter helper is opaque, and Ghidra shows
`IsNumeric` passes the whole string to an unresolved predicate while OpenEgg
currently tests one byte. The suite is not a retail log comparison.
