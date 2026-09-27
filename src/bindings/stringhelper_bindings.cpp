#include "gmThread.h"
#include "gmStringObject.h"
#include "gmTableObject.h"
#include "stringhelper_bindings.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>

namespace {

// Reads either an int char code or a length-1 string as a single character.
// functions read a raw int char code (IsBackspace/IsLowercase/IsUppercase/
// IsAlphanumeric) while others (IsAlpha/IsWhitespace) read a full string and
// require length==1. Every real corpus call site for any of them passes a
// length-1 string (STRINGHELPER_GetChar's own return shape), so both forms
// are accepted here uniformly rather than picking one and rejecting the
// other's real callers.
gmType g_stringStreamType = -1;
struct StringStreamData { std::string text; };

bool ReadSingleChar(gmThread* a_thread, int a_param, char& a_out) {
    gmVariable v = a_thread->Param(a_param);
    if (v.m_type == GM_INT) { a_out = (char)v.m_value.m_int; return true; }
    if (v.m_type == GM_STRING) {
        gmStringObject* s = (gmStringObject*)a_thread->GetMachine()->GetObject(v.m_value.m_ref);
        if (s->GetLength() != 1) return false;
        a_out = s->GetString()[0];
        return true;
    }
    return false;
}

// StringHelper.c, 0x000a97b8): the decompiled body sscanf's the string
// against a numeric format, and returns GM_NULL on a failed/empty parse
// (iVar3 == 0 || iVar3 == -1, i.e. sscanf's "0 items matched" or
// "input error" returns) or the parsed int otherwise -- matched here with
// strtol so leading whitespace/sign are handled the same way sscanf("%d")
// would, and "no digits consumed" (endptr == str) is the null case.
int GM_CDECL gmf_STRINGHELPER_ConvertToInt(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    char* end = nullptr;
    long value = std::strtol(str, &end, 10);
    if (end == str) {
        a_thread->PushNull();
    } else {
        a_thread->PushInt((gmint)value);
    }
    return GM_OK;
}

// function's real return-slot tag is 0x26 (table-like), not 0x25 (string)
// -- SortAscending and FileHelper.GetFileListing above both also return
// 0x26, and both of those really are index-keyed collections, so 0x26 most
// likely means "table" in retail's tag scheme (distinct from tag 5, which
// StringStream.STRINGSTREAM_Create confirms is the actual StringStream
// type). But every real call site (WinScreenProcessBase.gm:1084's
// `drawList.print(0,0,0,this.parent.elapsedTimeString,...)`) feeds the
// result straight into a function that needs a plain string, and the
// helper building the real table's contents (FUN_000c9920) isn't
// independently decompiled, so its exact field layout can't be recovered.
// Deliberately kept as a plain formatted string rather than guessing a
// table schema that would make every real call site's print() crash
// outright -- the pragmatic choice given the ambiguity, not a claim that
// this matches retail's internal representation.
int GM_CDECL gmf_STRINGHELPER_MakeTimeTable(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    long totalSeconds = (long)(ms / 1000.0f);
    if (totalSeconds < 0) totalSeconds = 0;
    long minutes = totalSeconds / 60;
    long seconds = totalSeconds % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%ld:%02ld", minutes, seconds);
    a_thread->PushNewString(buf);
    return GM_OK;
}

// StringHelper.c, 0x000a9a70). Unlike ConvertToInt, always returns an int
// (the decompiled body unconditionally writes type tag 1, no null branch),
// so a failed/empty parse is 0, not null. strtol base 16 handles an
// optional leading "0x"/"0X" the same way real hex-string parsing would
// (confirmed call site: CampaignScreenRecordPlayer.gm:367 passes "0x80").
int GM_CDECL gmf_STRINGHELPER_ConvertHexStringToInt(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    a_thread->PushInt((gmint)std::strtol(str, nullptr, 16));
    return GM_OK;
}

// STRINGHELPER_GetFirstOccurrence(needle, haystack) -- confirmed real
// GM_INT. Real call site (TouchEditProcessBase.gm:1460's
// `while(STRINGHELPER_GetFirstOccurrence("_", fileName) > 0) { fileName =
// STRINGHELPER_CopyAndSetChar(STRINGHELPER_GetFirstOccurrence("_",
// fileName), "/", fileName); }`) checks the result `> 0` and feeds it
// straight into CopyAndSetChar's own index param with no adjustment --
// consistent only with 1-based positions and a 0 "not found" sentinel
// (the common convention for this shape of API, e.g. Lua's string.find/
// BASIC's INSTR), not 0-based indexing (which would treat a real match at
// the very first character as indistinguishable from "not found").
int GM_CDECL gmf_STRINGHELPER_GetFirstOccurrence(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(needle, 0, "");
    GM_STRING_PARAM(haystack, 1, "");
    if (!needle || !*needle) { a_thread->PushInt(0); return GM_OK; }
    const char* found = std::strstr(haystack, needle);
    a_thread->PushInt(found ? (gmint)((found - haystack) + 1) : 0);
    return GM_OK;
}

// STRINGHELPER_CopyAndSetChar(index, newChar, sourceString) -- confirmed
// (copy of sourceString, source itself untouched -- matches the "Copy"
// half of the name), overwrites a single character at `index`, returns
// the copy. Paired 1-based indexing with GetFirstOccurrence above (real
// usage passes GetFirstOccurrence's result straight through with no -1/+1
// adjustment). Out-of-range index (matching the decompiled body's own
// `-1 < iVar1 && iVar1 < strlen` bounds check) leaves the string
// unmodified rather than throwing, same "tolerate a bad index" leniency
// as the rest of this file's confirmed-real functions.
int GM_CDECL gmf_STRINGHELPER_CopyAndSetChar(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(index1Based, 0, 0);
    GM_STRING_PARAM(newChar, 1, "");
    GM_STRING_PARAM(source, 2, "");
    std::string out = source;
    int i = index1Based - 1;
    if (i >= 0 && i < (int)out.size() && newChar && *newChar) {
        out[(size_t)i] = newChar[0];
    }
    a_thread->PushNewString(out.c_str());
    return GM_OK;
}

// STRINGHELPER_GetLength(str) -- confirmed real (Ghidra: retail
// FUN_00443b10 @ 0x00443b10, a bare strlen wrapper). Real call sites:
// PekkoAndJon/Main.gm:150,344,416,418 and Point.gm (typewriter score-text
// effect, compares `g_currentCharIndex >= STRINGHELPER_GetLength(text)` to
// detect the end of the string). Was entirely unbound.
int GM_CDECL gmf_STRINGHELPER_GetLength(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    a_thread->PushInt((gmint)std::strlen(str));
    return GM_OK;
}

// STRINGHELPER_GetChar(index, str) -- confirmed real (Ghidra: retail
// FUN_00444070 @ 0x00444070), 0-based index (real caller Main.gm:149
// increments `g_currentCharIndex` from 0 and compares
// `>= STRINGHELPER_GetLength(...)`) -- returns a length-1 string, not a
// char/int. Was entirely unbound. Out-of-range index returns an empty
// string rather than throwing, matching this file's existing
// tolerate-a-bad-index convention (see STRINGHELPER_CopyAndSetChar above).
int GM_CDECL gmf_STRINGHELPER_GetChar(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(index, 0, 0);
    GM_STRING_PARAM(str, 1, "");
    char buf[2] = {0, 0};
    size_t len = std::strlen(str);
    if (index >= 0 && (size_t)index < len) buf[0] = str[index];
    a_thread->PushNewString(buf);
    return GM_OK;
}

// StringStream.c, address obscured by an unreachable-block Ghidra
// warning, but the confirmed 1-arg-string-in/string-out shape plus real
// call site TouchEditProcessBase.gm:1469 (`GameMonkeyScript.loadScript(
// MakeGMScriptFilename(entType));`) pin the contract exactly: this
// function must do internally what the SAME script does manually two
// lines above it for an identical purpose (`local fileName=entType+".gm";
// while(STRINGHELPER_GetFirstOccurrence("_",fileName)>0){fileName=
// STRINGHELPER_CopyAndSetChar(...,"/",fileName);}`) -- append ".gm", then
// replace every "_" with "/" to turn an entity type name
// ("Platformer_EntityPacks_SpawnPoint") into its real script file path
// ("Platformer/EntityPacks/SpawnPoint.gm").
int GM_CDECL gmf_MakeGMScriptFilename(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(typeName, 0, "");
    std::string path = typeName;
    path += ".gm";
    for (char& c : path) if (c == '_') c = '/';
    a_thread->PushNewString(path.c_str());
    return GM_OK;
}

// STRINGHELPER_GetLastOccurrence(needle, haystack) -- confirmed real
// contract as GetFirstOccurrence above (sibling function, same shape).
int GM_CDECL gmf_STRINGHELPER_GetLastOccurrence(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(needle, 0, "");
    GM_STRING_PARAM(haystack, 1, "");
    if (!needle || !*needle) { a_thread->PushInt(0); return GM_OK; }
    std::string hay(haystack);
    size_t pos = hay.rfind(needle);
    a_thread->PushInt(pos == std::string::npos ? 0 : (gmint)(pos + 1));
    return GM_OK;
}

// STRINGHELPER_FormatLevelName(name) -- confirmed real, exact algorithm
// @ 0xa7f70): replaces each of '^' '\\' '/' '\n' '{' '}' with a space, then
// trims leading/trailing spaces and tabs.
int GM_CDECL gmf_STRINGHELPER_FormatLevelName(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(name, 0, "");
    std::string s(name);
    for (char& c : s) {
        if (c == '^' || c == '\\' || c == '/' || c == '\n' || c == '{' || c == '}') c = ' ';
    }
    size_t begin = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    a_thread->PushNewString(begin == std::string::npos ? "" : s.substr(begin, end - begin + 1).c_str());
    return GM_OK;
}

// StringHelper.c @ 0xa8260): returns a copy of str with its last character
// removed. Removes a whole UTF-8 codepoint (not just the last byte), since
// this backs real text-entry backspacing (login username, level titles)
// that can contain non-ASCII characters.
int GM_CDECL gmf_STRINGHELPER_Backspace(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    std::string s(str);
    if (!s.empty()) {
        size_t cut = s.size() - 1;
        while (cut > 0 && ((unsigned char)s[cut] & 0xC0) == 0x80) --cut; // skip UTF-8 continuation bytes
        s.erase(cut);
    }
    a_thread->PushNewString(s.c_str());
    return GM_OK;
}

// StringHelper.c @ 0xa839c), returns a plain string (return-slot tag 0x25),
// unlike its sibling STRINGHELPER_MakeTimeTable which returns a table-like
// object (tag 0x26, see that function's own comment above for why it's
// implemented as a plain string here too). No corpus call site exercises
// anything but feeding the result straight into drawList.printUntranslated
// (Main.gm:548/659), so the exact internal format (colon placement,
// zero-padding) can't be independently confirmed beyond "some human-
// readable elapsed time" -- matches the minutes:seconds format already
// used by MakeTimeTable's own (also unconfirmed-exact) implementation.
int GM_CDECL gmf_STRINGHELPER_MakeTimeString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    long totalSeconds = (long)(ms / 1000.0f);
    if (totalSeconds < 0) totalSeconds = 0;
    long minutes = totalSeconds / 60;
    long seconds = totalSeconds % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%ld:%02ld", minutes, seconds);
    a_thread->PushNewString(buf);
    return GM_OK;
}

// STRINGHELPER_Is{Backspace,Lowercase,Uppercase,Alpha,Whitespace,Numeric,
// thin wrappers over standard C character classification (ctype.h-shaped
// helper calls in the decompiled body: FUN_000c9764/9ad8/9af8/9b40/9b18/
// 9b74/9bf4). IsAlpha/IsWhitespace/IsNumeric's decompiled bodies read a
// full string and require it be exactly 1 character (false otherwise, per
// IsAlpha's own confirmed `iVar3==1` length check) -- matches every real
// call site, which always passes STRINGHELPER_GetChar's length-1 result.
int GM_CDECL gmf_STRINGHELPER_IsBackspace(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); char c;
    a_thread->PushInt((ReadSingleChar(a_thread, 0, c) && (c == 8 || c == 127)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_STRINGHELPER_IsLowercase(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); char c;
    a_thread->PushInt((ReadSingleChar(a_thread, 0, c) && std::islower((unsigned char)c)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_STRINGHELPER_IsUppercase(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); char c;
    a_thread->PushInt((ReadSingleChar(a_thread, 0, c) && std::isupper((unsigned char)c)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_STRINGHELPER_IsAlpha(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); char c;
    a_thread->PushInt((ReadSingleChar(a_thread, 0, c) && std::isalpha((unsigned char)c)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_STRINGHELPER_IsWhitespace(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); char c;
    a_thread->PushInt((ReadSingleChar(a_thread, 0, c) && std::isspace((unsigned char)c)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_STRINGHELPER_IsNumeric(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); char c;
    a_thread->PushInt((ReadSingleChar(a_thread, 0, c) && std::isdigit((unsigned char)c)) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_STRINGHELPER_IsAlphanumeric(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1); char c;
    a_thread->PushInt((ReadSingleChar(a_thread, 0, c) && std::isalnum((unsigned char)c)) ? 1 : 0);
    return GM_OK;
}

// classes/StringHelper.c @ 0xa8e80/0xa9048), full-string case conversion.
int GM_CDECL gmf_STRINGHELPER_MakeLowercase(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    std::string s(str);
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    a_thread->PushNewString(s.c_str());
    return GM_OK;
}
int GM_CDECL gmf_STRINGHELPER_MakeUppercase(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    std::string s(str);
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    a_thread->PushNewString(s.c_str());
    return GM_OK;
}

// STRINGHELPER_GetSubString(start, end, str) -- confirmed real, INCLUSIVE
// (not an exception/null) when end<start, start<0, end<0, or end>=length
// -- all confirmed straight from the decompiled bounds check.
int GM_CDECL gmf_STRINGHELPER_GetSubString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_INT_PARAM(start, 0, 0);
    GM_INT_PARAM(end, 1, 0);
    GM_STRING_PARAM(str, 2, "");
    int len = (int)std::strlen(str);
    if (end < start || start < 0 || end < 0 || end >= len) {
        a_thread->PushNewString("");
        return GM_OK;
    }
    a_thread->PushNewString(std::string(str + start, (size_t)(end - start + 1)).c_str());
    return GM_OK;
}

// StringHelper.c @ 0xa96dc): sscanf(str,"%f",...) (format string confirmed
// via direct EBOOT.elf extraction), null on a failed/empty parse (mirrors
// ConvertToInt's own confirmed null-on-failure contract above).
int GM_CDECL gmf_STRINGHELPER_ConvertToReal(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    char* end = nullptr;
    float value = std::strtof(str, &end);
    if (end == str) { a_thread->PushNull(); return GM_OK; }
    a_thread->PushFloat(value);
    return GM_OK;
}

// classes/StringHelper.c @ 0xa9894): parses the string as a 64-bit integer
// (format confirmed "%lld" via EBOOT.elf extraction) then divides by 1000,
// truncating toward zero -- the decompiled body's fixed-point reciprocal-
// multiply (DAT_0036cb28, confirmed via EBOOT.elf extraction to be the
// standard magic constant for unsigned division by 1000) is just an
// optimized encoding of exactly that division. Null on a failed/empty
// parse, matching every other STRINGHELPER_ConvertXxx above.
int GM_CDECL gmf_STRINGHELPER_ConvertMSStringToSecondInt(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    char* end = nullptr;
    long long ms = std::strtoll(str, &end, 10);
    if (end == str) { a_thread->PushNull(); return GM_OK; }
    a_thread->PushInt((gmint)(ms / 1000));
    return GM_OK;
}

// STRINGHELPER_MakeHexString(value) -- confirmed real 1-int-arg -> string
// (FUN_000ca5b4) isn't independently decompiled, so exact digit-padding
// can't be confirmed, but every real call site (ColourEditorProcess.gm's
// packed colour values, Main.gm's debug step counters) manually prepends
// its own "0x" before calling this, confirming no prefix and no fixed
// width is baked in here -- a plain uppercase hex digit string, the
// standard "%X" shape.
int GM_CDECL gmf_STRINGHELPER_MakeHexString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(value, 0, 0);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%X", (unsigned int)value);
    a_thread->PushNewString(buf);
    return GM_OK;
}

// StringHelper.c @ 0xa9b1c). Real call site (Main.gm:955
// `!STRINGHELPER_BeginsWith(item.entityType,"SchemeMockUp_")`) confirms
// (haystack, prefix) argument order.
int GM_CDECL gmf_STRINGHELPER_BeginsWith(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(str, 0, "");
    GM_STRING_PARAM(prefix, 1, "");
    size_t plen = std::strlen(prefix);
    a_thread->PushInt((std::strlen(str) >= plen && std::strncmp(str, prefix, plen) == 0) ? 1 : 0);
    return GM_OK;
}
// STRINGHELPER_BeginsWithSkipSpaceNoCase(str, prefix) -- confirmed real
// skips leading whitespace on `str` and compares case-insensitively (real
// call site: ConsoleProcess.gm checking console output lines for "error"
// regardless of leading indentation/case).
int GM_CDECL gmf_STRINGHELPER_BeginsWithSkipSpaceNoCase(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(str, 0, "");
    GM_STRING_PARAM(prefix, 1, "");
    const char* s = str;
    while (*s == ' ' || *s == '\t') ++s;
    size_t plen = std::strlen(prefix);
    bool ok = std::strlen(s) >= plen;
    for (size_t i = 0; ok && i < plen; ++i) {
        ok = std::tolower((unsigned char)s[i]) == std::tolower((unsigned char)prefix[i]);
    }
    a_thread->PushInt(ok ? 1 : 0);
    return GM_OK;
}

// classes/StringHelper.c @ 0xa9cb4). Empty string counts as "entirely
// whitespace" (real call sites use this to reject blank level titles --
// EditScreenInfoPopupBase.gm, EditorStartMenu.gm -- where an empty string
// must count as invalid input same as one full of spaces).
int GM_CDECL gmf_STRINGHELPER_IsEntireStringWhitespace(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    bool allSpace = true;
    for (const char* p = str; *p; ++p) { if (!std::isspace((unsigned char)*p)) { allSpace = false; break; } }
    a_thread->PushInt(allSpace ? 1 : 0);
    return GM_OK;
}

// StringHelper.c @ 0xa9d6c), plain int-returning comparator. Real call site
// (Mixer.gm:441 `if(STRINGHELPER_StringCompare(a.name,b.name)<0)`) only
// checks the sign, matching strcmp's contract exactly.
int GM_CDECL gmf_STRINGHELPER_StringCompare(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(a, 0, "");
    GM_STRING_PARAM(b, 1, "");
    a_thread->PushInt((gmint)std::strcmp(a, b));
    return GM_OK;
}

// STRINGHELPER_SortAscending(table, sortByValue=1) -- confirmed real
// a ~300-line custom linked-list merge sort that resisted a line-by-line
// port, but its CONTRACT is fully recoverable from the arg-count dispatch
// plus real call-site shapes: iterates the input table's key/value pairs,
// collects the KEY string when sortByValue==0 or the VALUE string when
// sortByValue!=0 (the default, confirmed by User.gm's 1-arg call sorting a
// plain array's own elements, i.e. its values) for every pair where that
// field actually is a string, sorts those strings ascending, and returns
// them as a table with 0-based integer keys AND a `.size` field --
// confirmed exactly by EntityBase.gm's `local sortedKeys=
// STRINGHELPER_SortAscending(t,0); for(i=0;i<sortedKeys.size;i+=1){local
// key=sortedKeys[i]; local value=t[key];}`.
int GM_CDECL gmf_STRINGHELPER_SortAscending(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_TABLE_PARAM(table, 0);
    bool sortByValue = true;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(sbv, 1, 1); sortByValue = (sbv != 0); }

    gmMachine& m = *a_thread->GetMachine();
    std::vector<std::string> strings;
    gmTableIterator it;
    for (gmTableNode* n = table->GetFirst(it); n; n = table->GetNext(it)) {
        const gmVariable& field = sortByValue ? n->m_value : n->m_key;
        if (field.m_type == GM_STRING) {
            strings.emplace_back(((gmStringObject*)m.GetObject(field.m_value.m_ref))->GetString());
        }
    }
    std::sort(strings.begin(), strings.end());

    gmTableObject* out = m.AllocTableObject();
    for (size_t i = 0; i < strings.size(); ++i) {
        out->Set(&m, (int)i, gmVariable(m.AllocStringObject(strings[i].c_str())));
    }
    out->Set(&m, "size", gmVariable((gmint)strings.size()));
    a_thread->PushTable(out);
    return GM_OK;
}

// ---------------------------------------------------------------------
// StringStream.c) with 4 confirmed functions (Create/Append/GetString/
// Delete, all bare globals taking the stream as an explicit first arg,
// e.g. `STRINGSTREAM_Append(ss, value)`, not `ss.append(value)` -- matches
// every real call site). Was entirely unimplemented despite 76 real
// STRINGSTREAM_Append call sites (script/XML/debug-text generation across
// the corpus, several feeding straight into the File type this session
// also implemented, e.g. Main.gm:3556-3571's
// `STRINGSTREAM_Append(ss,...); file.write(STRINGSTREAM_GetString(ss));`).
// @ 0xae6c4): int/float/string values are all appended via their natural
// text formatting (no explicit format string decompiled for the int/float
// paths, so "%d"/"%g"-shaped via to_string/snprintf, matching every other
// plain-value-to-text helper elsewhere in this codebase); anything else
// (a table, another StringStream, etc.) throws, matching the confirmed
// decompiled exception-sentinel return for an unrecognized type tag.
// Delete is a manual free in retail (no GC there); this engine's
// StringStream is GC-managed like every other user type here, so Delete
// is a safe no-op rather than an early double-free risk.
// ---------------------------------------------------------------------

StringStreamData* ThisStringStream(gmThread* a_thread, int a_param) {
    gmVariable v = a_thread->Param(a_param);
    if (v.m_type != g_stringStreamType) return nullptr;
    return (StringStreamData*)((gmUserObject*)a_thread->GetMachine()->GetObject(v.m_value.m_ref))->m_user;
}

int GM_CDECL gmf_STRINGSTREAM_Create(gmThread* a_thread) {
    (void)a_thread;
    StringStreamData* data = new StringStreamData();
    gmUserObject* obj = a_thread->GetMachine()->AllocUserObject(data, g_stringStreamType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}
int GM_CDECL gmf_STRINGSTREAM_Append(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    StringStreamData* ss = ThisStringStream(a_thread, 0);
    if (!ss) { GM_EXCEPTION_MSG("STRINGSTREAM_Append: expecting a StringStream as param 0"); return GM_EXCEPTION; }
    gmVariable v = a_thread->Param(1);
    if (v.m_type == GM_INT) {
        ss->text += std::to_string(v.m_value.m_int);
    } else if (v.m_type == GM_FLOAT) {
        char buf[64]; std::snprintf(buf, sizeof(buf), "%g", v.m_value.m_float);
        ss->text += buf;
    } else if (v.m_type == GM_STRING) {
        ss->text += ((gmStringObject*)a_thread->GetMachine()->GetObject(v.m_value.m_ref))->GetString();
    } else {
        GM_EXCEPTION_MSG("STRINGSTREAM_Append: expecting an int, float, or string value");
        return GM_EXCEPTION;
    }
    return GM_OK;
}
int GM_CDECL gmf_STRINGSTREAM_GetString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    StringStreamData* ss = ThisStringStream(a_thread, 0);
    if (!ss) { GM_EXCEPTION_MSG("STRINGSTREAM_GetString: expecting a StringStream as param 0"); return GM_EXCEPTION; }
    a_thread->PushNewString(ss->text.c_str());
    return GM_OK;
}
int GM_CDECL gmf_STRINGSTREAM_Delete(gmThread* a_thread) {
    (void)a_thread; // no-op, see header comment above
    return GM_OK;
}

void GM_CDECL StringStreamGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    delete (StringStreamData*)a_object->m_user;
}

} // namespace

void RegisterStringHelperBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("STRINGHELPER_MakeTimeTable", gmf_STRINGHELPER_MakeTimeTable);
    a_machine.RegisterLibraryFunction("STRINGHELPER_ConvertToInt", gmf_STRINGHELPER_ConvertToInt);
    a_machine.RegisterLibraryFunction("STRINGHELPER_ConvertHexStringToInt", gmf_STRINGHELPER_ConvertHexStringToInt);
    a_machine.RegisterLibraryFunction("STRINGHELPER_GetFirstOccurrence", gmf_STRINGHELPER_GetFirstOccurrence);
    a_machine.RegisterLibraryFunction("STRINGHELPER_CopyAndSetChar", gmf_STRINGHELPER_CopyAndSetChar);
    a_machine.RegisterLibraryFunction("MakeGMScriptFilename", gmf_MakeGMScriptFilename);
    a_machine.RegisterLibraryFunction("STRINGHELPER_GetLength", gmf_STRINGHELPER_GetLength);
    a_machine.RegisterLibraryFunction("STRINGHELPER_GetChar", gmf_STRINGHELPER_GetChar);
    a_machine.RegisterLibraryFunction("STRINGHELPER_GetLastOccurrence", gmf_STRINGHELPER_GetLastOccurrence);
    a_machine.RegisterLibraryFunction("STRINGHELPER_FormatLevelName", gmf_STRINGHELPER_FormatLevelName);
    a_machine.RegisterLibraryFunction("STRINGHELPER_Backspace", gmf_STRINGHELPER_Backspace);
    a_machine.RegisterLibraryFunction("STRINGHELPER_MakeTimeString", gmf_STRINGHELPER_MakeTimeString);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsBackspace", gmf_STRINGHELPER_IsBackspace);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsLowercase", gmf_STRINGHELPER_IsLowercase);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsUppercase", gmf_STRINGHELPER_IsUppercase);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsAlpha", gmf_STRINGHELPER_IsAlpha);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsWhitespace", gmf_STRINGHELPER_IsWhitespace);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsNumeric", gmf_STRINGHELPER_IsNumeric);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsAlphanumeric", gmf_STRINGHELPER_IsAlphanumeric);
    a_machine.RegisterLibraryFunction("STRINGHELPER_MakeLowercase", gmf_STRINGHELPER_MakeLowercase);
    a_machine.RegisterLibraryFunction("STRINGHELPER_MakeUppercase", gmf_STRINGHELPER_MakeUppercase);
    a_machine.RegisterLibraryFunction("STRINGHELPER_GetSubString", gmf_STRINGHELPER_GetSubString);
    a_machine.RegisterLibraryFunction("STRINGHELPER_ConvertToReal", gmf_STRINGHELPER_ConvertToReal);
    a_machine.RegisterLibraryFunction("STRINGHELPER_ConvertMSStringToSecondInt", gmf_STRINGHELPER_ConvertMSStringToSecondInt);
    a_machine.RegisterLibraryFunction("STRINGHELPER_MakeHexString", gmf_STRINGHELPER_MakeHexString);
    a_machine.RegisterLibraryFunction("STRINGHELPER_BeginsWith", gmf_STRINGHELPER_BeginsWith);
    a_machine.RegisterLibraryFunction("STRINGHELPER_BeginsWithSkipSpaceNoCase", gmf_STRINGHELPER_BeginsWithSkipSpaceNoCase);
    a_machine.RegisterLibraryFunction("STRINGHELPER_IsEntireStringWhitespace", gmf_STRINGHELPER_IsEntireStringWhitespace);
    a_machine.RegisterLibraryFunction("STRINGHELPER_StringCompare", gmf_STRINGHELPER_StringCompare);
    a_machine.RegisterLibraryFunction("STRINGHELPER_SortAscending", gmf_STRINGHELPER_SortAscending);

    g_stringStreamType = a_machine.CreateUserType("StringStream");
    a_machine.RegisterUserCallbacks(g_stringStreamType, nullptr, StringStreamGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("STRINGSTREAM_Create", gmf_STRINGSTREAM_Create);
    a_machine.RegisterLibraryFunction("STRINGSTREAM_Append", gmf_STRINGSTREAM_Append);
    a_machine.RegisterLibraryFunction("STRINGSTREAM_GetString", gmf_STRINGSTREAM_GetString);
    a_machine.RegisterLibraryFunction("STRINGSTREAM_Delete", gmf_STRINGSTREAM_Delete);
}
