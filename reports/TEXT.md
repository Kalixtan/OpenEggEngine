# Test report

Generated: 2026-11-14 17:37 UTC  
Suite: text  
Engine: `OpenEgg`  
Result: **5/5 checks passed (100%); 0 known gaps**

Scores count passing test cases. They do not measure total engine completeness. One integration case may count for several functions.

| System | Section | Passed | Total | Rate |
| --- | --- | ---: | ---: | ---: |
| Text | Text layout | 5 | 5 | 100% |

## Functions

| Function | Passed | Cases | Score | Status |
| --- | ---: | ---: | ---: | --- |
| `PRINTINFO` | 2 | 2 | 100% | PASS |
| `Video.createDrawList` | 0 | 0 | — | untested |
| `Video.getTextWidth` | 1 | 1 | 100% | PASS |
| `drawList.print` | 2 | 2 | 100% | PASS |
| `drawList.splitString` | 4 | 4 | 100% | PASS |

## Checks

| System / section | Test | Result | Retail basis |
| --- | --- | --- | --- |
| Text / Text layout | Split ranges use UTF-8 byte offsets | PASS | local check |
| Text / Text layout | Integer bounds keep retail float-tag defaults | PASS | local check |
| Text / Text layout | CRLF and caret create line breaks | PASS | local check |
| Text / Text layout | Print uses splitter line count | PASS | local check |
| Text / Text layout | Credit name changes before layout | PASS | local check |