# String helper probes

Run headless with `python3 tests/run.py --suite stringhelper --report reports/STRINGHELPER.md`.
The tests cover the time-string wrapper and `SortAscending` argument selection.
The retail `MakeTimeTable` wrapper is confirmed to return a table, but its
format-table schema remains unknown; that mismatch is recorded as a known gap.
