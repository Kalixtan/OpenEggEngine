# Test report

Generated: 2026-11-14 21:08 UTC
Suite: stringhelper
Engine: `OpenEgg`
Result: **13/13 checks passed (100%); 0 known gaps**

Scores count passing test cases. They do not measure total engine completeness. One integration case may count for several functions.

| System | Section | Passed | Total | Rate |
| --- | --- | ---: | ---: | ---: |
| Stringhelper | Formatting helpers | 13 | 13 | 100% |

## Functions

| Function | Passed | Cases | Score | Status |
| --- | ---: | ---: | ---: | --- |
| `STRINGHELPER_Backspace` | 1 | 1 | 100% | PASS |
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
| `STRINGHELPER_IsNumeric` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsUppercase` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_IsWhitespace` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_MakeHexString` | 0 | 0 | — | untested |
| `STRINGHELPER_MakeLowercase` | 0 | 0 | — | untested |
| `STRINGHELPER_MakeTimeString` | 1 | 1 | 100% | PASS |
| `STRINGHELPER_MakeTimeTable` | 3 | 3 | 100% | PASS |
| `STRINGHELPER_MakeUppercase` | 0 | 0 | — | untested |
| `STRINGHELPER_SortAscending` | 2 | 2 | 100% | PASS |
| `STRINGHELPER_StringCompare` | 0 | 0 | — | untested |
| `String.Float` | 1 | 1 | 100% | PASS |
| `String.Int` | 1 | 1 | 100% | PASS |

## Checks

| System / section | Test | Result | Retail basis |
| --- | --- | --- | --- |
| Stringhelper / Formatting helpers | Server difficulty metadata converts through String.Int | PASS | local check |
| Stringhelper / Formatting helpers | MakeTimeString uses the retail format and clamps time | PASS | local check |
| Stringhelper / Formatting helpers | MakeTimeTable returns retail table and string types | PASS | local check |
| Stringhelper / Formatting helpers | MakeTimeTable formats minute second and hundredth slots | PASS | local check |
| Stringhelper / Formatting helpers | MakeTimeTable handles zero boundaries clamp and sign | PASS | local check |
| Stringhelper / Formatting helpers | IsNumeric accepts only nonempty decimal digit strings | PASS | local check |
| Stringhelper / Formatting helpers | Backspace removes one byte and handles empty strings | PASS | local check |
| Stringhelper / Formatting helpers | SortAscending sorts string keys or values | PASS | local check |
| Stringhelper / Formatting helpers | SortAscending reads selector only for exactly two integer arguments | PASS | local check |
| Stringhelper / Formatting helpers | CopyAndSetChar uses zero-based byte indexes | PASS | local check |
| Stringhelper / Formatting helpers | GetChar and GetSubString use raw integer argument slots | PASS | local check |
| Stringhelper / Formatting helpers | IsAlpha requires a one-byte string | PASS | local check |
| Stringhelper / Formatting helpers | Character predicates use retail byte arguments | PASS | local check |

The decomp confirms `STRINGHELPER_Char` exists in retail, but its saved wrapper returns one byte from an uninitialized local buffer. OpenEgg has no binding because that does not establish a usable contract. Formatter, numeric predicate, table layout, and byte-removal helper were traced from the PS3 disassembly. Only fourteen of the thirty StringHelper symbols have assertions. These are local checks, not retail output comparisons.
