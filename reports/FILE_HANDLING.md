# Test report

Generated: 2026-11-15 17:54 UTC
Suite: files
Engine: `OpenEgg`
Result: **21/21 checks passed (100%); 0 known gaps**

Scores count passing test cases. They do not measure total engine completeness. One integration case may count for several functions.

| System | Section | Passed | Total | Rate |
| --- | --- | ---: | ---: | ---: |
| Files | File handling | 19 | 19 | 100% |
| Files | Forced root | 1 | 1 | 100% |
| Files | Solo root | 1 | 1 | 100% |

## Functions

| Function | Passed | Cases | Score | Status |
| --- | ---: | ---: | ---: | --- |
| `FILE` | 1 | 1 | 100% | PASS |
| `FILEHELPER_CopyFile` | 3 | 3 | 100% | PASS |
| `FILEHELPER_DeleteFile` | 4 | 4 | 100% | PASS |
| `FILEHELPER_EnsurePathExists` | 2 | 2 | 100% | PASS |
| `FILEHELPER_GetFileListing` | 5 | 5 | 100% | PASS |
| `FILEHELPER_LoadCompressedString` | 1 | 1 | 100% | PASS |
| `FILEHELPER_LoadString` | 12 | 12 | 100% | PASS |
| `FILEHELPER_VirtualSpaceLeft` | 1 | 1 | 100% | PASS |
| `FILEHELPER_WriteCompressedString` | 1 | 1 | 100% | PASS |
| `FILEHELPER_WriteString` | 2 | 2 | 100% | PASS |
| `FILE_Append` | 1 | 1 | 100% | PASS |
| `FILE_IsExists` | 5 | 5 | 100% | PASS |
| `FILE_Write` | 1 | 1 | 100% | PASS |
| `File.close` | 6 | 6 | 100% | PASS |
| `File.openForAppend` | 1 | 1 | 100% | PASS |
| `File.openForRead` | 6 | 6 | 100% | PASS |
| `File.openForWrite` | 2 | 2 | 100% | PASS |
| `File.readAll` | 6 | 6 | 100% | PASS |
| `File.write` | 3 | 3 | 100% | PASS |

## Checks

| System / section | Test | Result | Retail basis |
| --- | --- | --- | --- |
| Files / File handling | Area constants and default arguments | PASS | Ghidra path helper traces; missing LoadString is a known retail crash |
| Files / File handling | Game data reads and listing | PASS | Ghidra path helper traces; missing LoadString is a known retail crash |
| Files / File handling | Host area paths | PASS | Ghidra path helper traces; missing LoadString is a known retail crash |
| Files / File handling | Save data write read delete lifecycle | PASS | Ghidra path helper traces; missing LoadString is a known retail crash |
| Files / File handling | Missing and unsupported area behavior | PASS | Ghidra path helper traces; missing LoadString is a known retail crash |
| Files / File handling | Virtual space constant | PASS | Ghidra path helper traces; missing LoadString is a known retail crash |
| Files / File handling | Copy data between supported areas | PASS | Ghidra helper traces plus local assertions |
| Files / File handling | Copy missing and empty sources | PASS | Ghidra helper traces plus local assertions |
| Files / File handling | WriteString area return behavior | PASS | Ghidra helper traces plus local assertions |
| Files / File handling | Compressed string roundtrip and invalid data | PASS | Ghidra helper traces plus local assertions |
| Files / File handling | EnsurePathExists nested and existing paths | PASS | Ghidra helper traces plus local assertions |
| Files / File handling | File write, read, and close | PASS | Ghidra File class and bare helper traces |
| Files / File handling | File append modes and byte content | PASS | Ghidra File class and bare helper traces |
| Files / File handling | File missing path and closed operations | PASS | Ghidra File class and bare helper traces |
| Files / File handling | Bare FILE write and append formatting | PASS | Ghidra File class and bare helper traces |
| Files / File handling | Empty and embedded NUL text files | PASS | Ghidra LoadString wrapper uses NUL-terminated conversion |
| Files / File handling | Host copy and SaveData deletion | PASS | Ghidra area helpers plus local assertions |
| Files / File handling | File reopen truncates and text mode reads | PASS | Ghidra File class plus local assertions |
| Files / File handling | File empty reads | PASS | Ghidra File class plus local assertions |
| Files / Forced root | Forced data root wins | PASS | local check |
| Files / Solo root | Solo data root isolates sources | PASS | local check |

These are local headless assertions, not a retail differential run. Ghidra shows `GetFileListing` walking data overlays, but `LoadString` formats area 1 as `/app_home/<path>`. The PS3 trace found ROMs under `USRDIR/game_data/games` and then tried `USRDIR/games/<name>`. The open failed; retail returned a null buffer which its string wrapper did not guard, causing the crash. OpenEgg resolves through its overlay and returns an empty string for a missing file. `WriteString` on SaveData returns success without writing, and `EnsurePathExists` attempts mkdir but returns false. Layered PSARC precedence still needs a retail log comparison.
