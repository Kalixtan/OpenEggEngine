# File API checks

Run with `python3 tests/run.py --suite files`. Checks cover the FileHelper areas, read/write return rules, copy/list behavior, data-root modes, and the `File` object API.

These are OpenEgg headless assertions, not a full retail log comparison. Ghidra traces show that retail area 1 listing searches mounted layers, while `LoadString` formats a direct `/<path>` open. The PS3 run found ROMs under `game_data/games` but tried `/games`, then crashed when the missing-file result reached its string wrapper. Retail also returns a null buffer for a missing `LoadString`; OpenEgg returns an empty string to avoid that crash.
