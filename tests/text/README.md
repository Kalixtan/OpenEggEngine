# Text probes

Run headless with `python3 tests/run.py --suite text --report reports/TEXT.md`.
Checks cover UTF-8 byte ranges, hard breaks, wrapped icon tokens, `printArray`
joining, print layout, and the display-only credit edit. The parser and icon
rules are based on saved PS3 Ghidra traces. These are local assertions, not a
retail-vs-OpenEgg output comparison. Exact font raster/shader output, indexed
FSA palettes, animated RGB transparency/timing, region button mapping, and
matched pixel captures remain unverified.
