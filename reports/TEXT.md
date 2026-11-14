# Test report

Generated: 2026-11-14 17:53 UTC
Suite: text
Engine: `OpenEgg`
Result: **9/9 checks passed (100%); 0 known gaps**

Scores count passing test cases. They do not measure total engine completeness. One integration case may count for several functions.

| System | Section | Passed | Total | Rate |
| --- | --- | ---: | ---: | ---: |
| Text | Text layout | 9 | 9 | 100% |

## Functions

| Function | Passed | Cases | Score | Status |
| --- | ---: | ---: | ---: | --- |
| `PRINTINFO` | 5 | 5 | 100% | PASS |
| `Video.createDrawList` | 0 | 0 | — | untested |
| `Video.getTextWidth` | 1 | 1 | 100% | PASS |
| `drawList.print` | 4 | 4 | 100% | PASS |
| `drawList.printArray` | 1 | 1 | 100% | PASS |
| `drawList.splitString` | 5 | 5 | 100% | PASS |

## Checks

| System / section | Test | Result | Retail basis |
| --- | --- | --- | --- |
| Text / Text layout | Split ranges use UTF-8 byte offsets | PASS | local check |
| Text / Text layout | Integer bounds keep retail float-tag defaults | PASS | local check |
| Text / Text layout | CRLF and caret create line breaks | PASS | local check |
| Text / Text layout | Named icon stays atomic when wrapping | PASS | local check |
| Text / Text layout | Print keeps explicit breaks without bounds | PASS | local check |
| Text / Text layout | Print array joins items before drawing | PASS | local check |
| Text / Text layout | Print keeps strings longer than 512 bytes | PASS | local check |
| Text / Text layout | Print uses splitter line count | PASS | local check |
| Text / Text layout | Credit name changes before layout | PASS | local check |

These checks are local assertions, not retail output comparisons. Exact font raster/shader output, indexed FSA palettes, animated RGB transparency and timing, and region-specific accept/cancel mapping remain unverified.
