#include "gmThread.h"
#include "gmStringObject.h"
#include "stringhelper_bindings.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

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
}
