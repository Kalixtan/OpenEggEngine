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

int GM_CDECL gmf_STRINGHELPER_ConvertHexStringToInt(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    a_thread->PushInt((gmint)std::strtol(str, nullptr, 16));
    return GM_OK;
}

int GM_CDECL gmf_STRINGHELPER_GetFirstOccurrence(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(needle, 0, "");
    GM_STRING_PARAM(haystack, 1, "");
    if (!needle || !*needle) { a_thread->PushInt(0); return GM_OK; }
    const char* found = std::strstr(haystack, needle);
    a_thread->PushInt(found ? (gmint)((found - haystack) + 1) : 0);
    return GM_OK;
}

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

int GM_CDECL gmf_STRINGHELPER_GetLength(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    a_thread->PushInt((gmint)std::strlen(str));
    return GM_OK;
}

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

int GM_CDECL gmf_MakeGMScriptFilename(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(typeName, 0, "");
    std::string path = typeName;
    path += ".gm";
    for (char& c : path) if (c == '_') c = '/';
    a_thread->PushNewString(path.c_str());
    return GM_OK;
}

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

int GM_CDECL gmf_STRINGHELPER_ConvertToReal(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    char* end = nullptr;
    float value = std::strtof(str, &end);
    if (end == str) { a_thread->PushNull(); return GM_OK; }
    a_thread->PushFloat(value);
    return GM_OK;
}

int GM_CDECL gmf_STRINGHELPER_ConvertMSStringToSecondInt(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    char* end = nullptr;
    long long ms = std::strtoll(str, &end, 10);
    if (end == str) { a_thread->PushNull(); return GM_OK; }
    a_thread->PushInt((gmint)(ms / 1000));
    return GM_OK;
}

int GM_CDECL gmf_STRINGHELPER_MakeHexString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(value, 0, 0);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%X", (unsigned int)value);
    a_thread->PushNewString(buf);
    return GM_OK;
}

int GM_CDECL gmf_STRINGHELPER_BeginsWith(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(str, 0, "");
    GM_STRING_PARAM(prefix, 1, "");
    size_t plen = std::strlen(prefix);
    a_thread->PushInt((std::strlen(str) >= plen && std::strncmp(str, prefix, plen) == 0) ? 1 : 0);
    return GM_OK;
}
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

int GM_CDECL gmf_STRINGHELPER_IsEntireStringWhitespace(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    bool allSpace = true;
    for (const char* p = str; *p; ++p) { if (!std::isspace((unsigned char)*p)) { allSpace = false; break; } }
    a_thread->PushInt(allSpace ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_STRINGHELPER_StringCompare(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(a, 0, "");
    GM_STRING_PARAM(b, 1, "");
    a_thread->PushInt((gmint)std::strcmp(a, b));
    return GM_OK;
}

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
