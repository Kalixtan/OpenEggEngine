# Test report

Generated: 2026-11-14 18:22 UTC  
Suite: stringhelper  
Engine: `OpenEgg`  
Result: **7/8 checks passed (88%); 1 known gaps**

Scores count passing test cases. They do not measure total engine completeness. One integration case may count for several functions.

| System | Section | Passed | Total | Rate |
| --- | --- | ---: | ---: | ---: |
| Stringhelper | Formatting helpers | 7 | 8 | 88% |

## Functions

| Function | Passed | Cases | Score | Status |
| --- | ---: | ---: | ---: | --- |
| `STRINGHELPER_Backspace` | 0 | 0 | — | untested |
| `STRINGHELPER_BeginsWith` | 0 | 0 | — | untested |
| `STRINGHELPER_BeginsWithSkipSpaceNoCase` | 0 | 0 | — | untested |
| `STRINGHELPER_Char` | 0 | 0 | — | untested |
| `STRINGHELPER_ConvertHexStringToInt` | 0 | 0 | — | untested |
| `STRINGHELPER_ConvertMSStringToSecondInt` | 0 | 0 | — | untested |
| `STRINGHELPER_ConvertToInt` | 0 | 0 | — | untested |
| `STRINGHELPER_ConvertToReal` | 0 | 0 | — | untested |
| `STRINGHELPER_CopyAndSetChar` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_FormatLevelName` | 0 | 0 | — | untested |
| `STRINGHELPER_GetChar` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_GetFirstOccurrence` | 0 | 0 | — | untested |
| `STRINGHELPER_GetLastOccurrence` | 0 | 0 | — | untested |
| `STRINGHELPER_GetLength` | 0 | 0 | — | untested |
| `STRINGHELPER_GetSubString` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsAlpha` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsAlphanumeric` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsBackspace` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsEntireStringWhitespace` | 0 | 0 | — | untested |
| `STRINGHELPER_IsLowercase` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsNumeric` | 0 | 0 | — | untested |
| `STRINGHELPER_IsUppercase` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsWhitespace` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_MakeHexString` | 0 | 0 | — | untested |
| `STRINGHELPER_MakeLowercase` | 0 | 0 | — | untested |
| `STRINGHELPER_MakeTimeString` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_MakeTimeTable` | 0 | 1 | 0% | GAP |
| `STRINGHELPER_MakeUppercase` | 0 | 0 | — | untested |
| `STRINGHELPER_SortAscending` | 2 | 2 | 100% | PASS |
| `STRINGHELPER_StringCompare` | 0 | 0 | — | untested |

## Checks

| System / section | Test | Result | Retail basis |
| --- | --- | --- | --- |
| Stringhelper / Formatting helpers | MakeTimeString handles seconds and fractional milliseconds | PASS | local check |
| Stringhelper / Formatting helpers | MakeTimeTable returns the retail table type | GAP | local check |
| Stringhelper / Formatting helpers | SortAscending sorts string keys or values | PASS | local check |
| Stringhelper / Formatting helpers | SortAscending reads selector only for exactly two integer arguments | PASS | local check |
| Stringhelper / Formatting helpers | CopyAndSetChar uses zero-based byte indexes | PASS | local check |
| Stringhelper / Formatting helpers | GetChar and GetSubString use raw integer argument slots | PASS | local check |
| Stringhelper / Formatting helpers | IsAlpha requires a one-byte string | PASS | local check |
| Stringhelper / Formatting helpers | Character predicates use retail byte arguments | PASS | local check |

## Gaps and failures

- **MakeTimeTable returns the retail table type**: Retail decomp returns a GM table; OpenEgg still returns a minutes:seconds string because the retail table schema is unresolved. (log: `tests/_results/stringhelper/probe.log`)

The decomp confirms `STRINGHELPER_Char` exists in retail, but its saved wrapper returns one byte from an uninitialized local buffer. OpenEgg has no binding because that does not establish a usable contract. The `MakeTimeTable` schema and `MakeTimeString` formatter helper remain unresolved. Ghidra shows `IsNumeric` passes the full string to an opaque predicate, while OpenEgg currently checks one character. Only twelve of the thirty StringHelper symbols have assertions. These are local checks, not retail output comparisons.
