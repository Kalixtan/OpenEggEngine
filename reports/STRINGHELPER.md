# Test report

Generated: 2026-11-14 18:01 UTC
Suite: stringhelper
Engine: `OpenEgg`
Result: **3/4 checks passed (75%); 1 known gaps**

Scores count passing test cases. They do not measure total engine completeness. One integration case may count for several functions.

| System | Section | Passed | Total | Rate |
| --- | --- | ---: | ---: | ---: |
| Stringhelper | Formatting helpers | 3 | 4 | 75% |

## Functions

| Function | Passed | Cases | Score | Status |
| --- | ---: | ---: | ---: | --- |
| `STRINGHELPER_MakeTimeString` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_MakeTimeTable` | 0 | 1 | 0% | GAP |
| `STRINGHELPER_SortAscending` | 2 | 2 | 100% | PASS |

## Checks

| System / section | Test | Result | Retail basis |
| --- | --- | --- | --- |
| Stringhelper / Formatting helpers | MakeTimeString handles seconds and fractional milliseconds | PASS | local check |
| Stringhelper / Formatting helpers | MakeTimeTable returns the retail table type | GAP | local check |
| Stringhelper / Formatting helpers | SortAscending sorts string keys or values | PASS | local check |
| Stringhelper / Formatting helpers | SortAscending reads selector only for exactly two integer arguments | PASS | local check |

## Gaps and failures

- **MakeTimeTable returns the retail table type**: Retail decomp returns a GM table; OpenEgg still returns a minutes:seconds string because the retail table schema is unresolved. (log: `tests/_results/stringhelper/probe.log`)
