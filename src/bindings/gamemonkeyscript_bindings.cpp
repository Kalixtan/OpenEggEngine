#include "gmThread.h"
#include "gamemonkeyscript_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "gmIncGC.h"
#include "../core/engine_context.h"
#include "../core/save_data.h"
#include "tinyxml2.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <algorithm>
#include <zlib.h>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

int g_cleanPassesRemaining = 0;

// Retail has separate raw and compressed table loaders. The compressed
// wrappers always enter the zlib path; do not infer compression from bytes.
bool InflateRequired(std::string& a_text) {
    uLongf destLen = (uLongf)a_text.size() * 8 + 1024;
    for (int attempt = 0; attempt < 6; ++attempt) {
        std::string out(destLen, '\0');
        uLongf outLen = destLen;
        int rc = uncompress((Bytef*)out.data(), &outLen, (const Bytef*)a_text.data(), (uLong)a_text.size());
        if (rc == Z_OK) { out.resize(outLen); a_text = std::move(out); return true; }
        if (rc != Z_BUF_ERROR) return false; // corrupt/wrong format, not just "buffer too small"
        destLen *= 4;
    }
    return false;
}

bool SaveTicketFilename(gmMachine& a_machine, const gmVariable& a_ticket, std::string& a_out) {
    gmVariable filename = a_ticket;
    if (a_ticket.m_type == GM_TABLE) {
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(a_ticket.m_value.m_ref);
        if (!table) return false;
        filename = table->Get(&a_machine, "filename");
    }
    if (filename.m_type != GM_STRING) return false;
    gmStringObject* string = (gmStringObject*)a_machine.GetObject(filename.m_value.m_ref);
    if (!string) return false;
    a_out = string->GetString();
    return true;
}

// Retail wrapper at 0x6ceb8 accepts path, optional raw file-area payload, and
// optional force-string flag. Its XML converter selects GM_INT/GM_FLOAT for
// numeric leaf values by default and GM_STRING when that flag is nonzero.
// Real data files (Platformer/mixer.dat) use a small, consistent tag
// vocabulary -- nested `<tableElement key="...">...</tableElement>` and
// leaf `<element key="..." value="..."/>`, no XML declaration, no single
// root (mixer.dat is a bare sibling sequence, which tinyxml2 tolerates:
// XMLDocument itself just acts as a node with multiple children rather
// than enforcing single-root well-formedness). Numeric-looking keys (e.g.
// "99") become GM_INT keys rather than strings, matching real Mixer.gm's
// `if (typeName(id)!="int")` check on exactly this data. Leaf values are
// parsed as a float when the whole string is numeric, else kept as a
// string. Originally a hand-rolled parser for just this tag vocabulary;
// request for a compatible, still-maintained library instead.
bool IsAllDigits(const std::string& a_s) {
    if (a_s.empty()) return false;
    size_t i = (a_s[0] == '-') ? 1 : 0;
    if (i >= a_s.size()) return false;
    for (; i < a_s.size(); ++i) if (!std::isdigit((unsigned char)a_s[i])) return false;
    return true;
}

void SetTableKey(gmMachine& a_machine, gmTableObject* a_table, const std::string& a_key, const gmVariable& a_value) {
    if (IsAllDigits(a_key)) a_table->Set(&a_machine, std::atoi(a_key.c_str()), a_value);
    else a_table->Set(&a_machine, a_key.c_str(), a_value);
}

// JSON object member names are always strings, including numeric-looking
// names such as DLC pack IDs "1" through "15". Arrays are handled separately
// by JsonToGmVariable and receive integer indexes. Do not use SetTableKey for
// JSON objects: its numeric-key coercion is only appropriate for the retail
// XML table format above and made persisted DLC flags unreadable on reload.
void SetJsonObjectKey(gmMachine& a_machine, gmTableObject* a_table,
                      const char* a_key, const gmVariable& a_value) {
    a_table->Set(&a_machine, a_key, a_value);
}

bool IsWholeNumber(const char* a_value, size_t a_len) {
    size_t i = (a_len > 0 && (a_value[0] == '-' || a_value[0] == '+')) ? 1 : 0;
    if (i >= a_len) return false;
    for (; i < a_len; ++i) if (!std::isdigit((unsigned char)a_value[i])) return false;
    return true;
}

gmVariable ParseLeafValue(gmMachine& a_machine, const char* a_value,
                          bool a_forceString) {
    if (!a_value) a_value = "";
    if (a_forceString) return gmVariable(a_machine.AllocStringObject(a_value));
    char* numEnd = nullptr;
    double d = std::strtod(a_value, &numEnd);
    if (numEnd != a_value && *numEnd == '\0') {
        if (IsWholeNumber(a_value, (size_t)(numEnd - a_value)) &&
            d >= (double)INT32_MIN && d <= (double)INT32_MAX) {
            return gmVariable((gmint)d);
        }
        return gmVariable((gmfloat)d);
    }
    return gmVariable(a_machine.AllocStringObject(a_value));
}

void ParseXmlElementInto(gmMachine& a_machine, gmTableObject* a_table,
                         const tinyxml2::XMLElement* a_elem, int a_fallbackIndex,
                         bool a_forceStringValues) {
    const char* key = a_elem->Attribute("key");

    if (std::strcmp(a_elem->Name(), "tableElement") == 0) {
        gmTableObject* child = a_machine.AllocTableObject();
        int childIndex = 0;
        for (const tinyxml2::XMLElement* c = a_elem->FirstChildElement(); c; c = c->NextSiblingElement()) {
            ParseXmlElementInto(a_machine, child, c, childIndex++,
                                a_forceStringValues);
        }
        if (key) SetTableKey(a_machine, a_table, key, gmVariable(child));
        else a_table->Set(&a_machine, a_fallbackIndex, gmVariable(child));
    } else if (std::strcmp(a_elem->Name(), "element") == 0) {
        gmVariable v = ParseLeafValue(a_machine, a_elem->Attribute("value"),
                                      a_forceStringValues);
        if (key) SetTableKey(a_machine, a_table, key, v);
        else a_table->Set(&a_machine, a_fallbackIndex, v);
    }
}

gmTableObject* ParseXmlTable(gmMachine& a_machine, const std::string& a_text,
                             bool a_forceStringValues) {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(a_text.c_str(), a_text.size()) != tinyxml2::XML_SUCCESS)
        return nullptr;
    gmTableObject* table = a_machine.AllocTableObject();
    int topIndex = 0;
    for (const tinyxml2::XMLElement* elem = doc.FirstChildElement(); elem;
         elem = elem->NextSiblingElement()) {
        ParseXmlElementInto(a_machine, table, elem, topIndex++, a_forceStringValues);
    }
    return table;
}

// GameMonkeyScript's retail XML and JSON loaders forward their area argument
// to the same FileHelper area conventions. The desktop maps area 2 to a
// literal host path, matching system_bindings.cpp's FILEHELPER_AREA_HOST path.
bool ReadGameMonkeyScriptFile(int a_area, const std::string& a_path,
                              std::string& a_out) {
    if (a_area == 0) return SaveData::ReadFile(a_path, a_out);
    if (a_area == 1) {
        FileHelper* files = GetEngineContext().fileHelper;
        return files && files->ReadTextFile(a_path, a_out);
    }
    if (a_area == 2) {
        std::ifstream file(std::filesystem::u8path(a_path), std::ios::binary);
        if (!file) return false;
        std::ostringstream contents;
        contents << file.rdbuf();
        a_out = contents.str();
        return true;
    }
    // Retail's area formatter has no path branch for values outside 0..2.
    return false;
}

gmVariable JsonToGmVariable(gmMachine& a_machine, const rapidjson::Value& a_v) {
    if (a_v.IsObject()) {
        gmTableObject* table = a_machine.AllocTableObject();
        for (auto it = a_v.MemberBegin(); it != a_v.MemberEnd(); ++it) {
            SetJsonObjectKey(a_machine, table, it->name.GetString(),
                             JsonToGmVariable(a_machine, it->value));
        }
        return gmVariable(table);
    }
    if (a_v.IsArray()) {
        gmTableObject* table = a_machine.AllocTableObject();
        int i = 0;
        for (const auto& elem : a_v.GetArray()) {
            table->Set(&a_machine, i++, JsonToGmVariable(a_machine, elem));
        }
        return gmVariable(table);
    }
    if (a_v.IsString()) return gmVariable(a_machine.AllocStringObject(a_v.GetString()));
    // Same int-vs-float table-key distinction as ParseLeafValue above --
    // rapidjson already tells us whether the literal was integral
    // (no decimal point/exponent in the source JSON), so trust it
    // directly rather than re-deriving it.
    if (a_v.IsInt()) return gmVariable((gmint)a_v.GetInt());
    if (a_v.IsNumber()) return gmVariable((gmfloat)a_v.GetDouble());
    if (a_v.IsBool()) return gmVariable((gmint)(a_v.GetBool() ? 1 : 0));
    return gmVariable::s_null;
}

int GM_CDECL gmf_GameMonkeyScript_loadScript(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");

    EngineContext& ctx = GetEngineContext();
    std::string source;
    if (!ctx.fileHelper->ReadTextFile(path, source)) {
        GM_EXCEPTION_MSG("GameMonkeyScript.loadScript: could not find '%s'", path);
        a_thread->PushInt(0);
        return GM_OK;
    }

    int errors = a_thread->GetMachine()->ExecuteString(source.c_str(), nullptr, true, path);
    // Runtime exceptions go to GetLog(), not the return value.
    // Flush it even when ExecuteString reports no compile errors.
    {
        bool first = true;
        const char* entry;
        while ((entry = a_thread->GetMachine()->GetLog().GetEntry(first)) != nullptr) {
            std::fprintf(stderr, "[gm] %s: %s\n", path, entry);
        }
        a_thread->GetMachine()->GetLog().Reset();
    }
    a_thread->PushInt(errors == 0 ? 1 : 0);
    return GM_OK;
}

int LoadTableFromXMLFile(gmThread* a_thread, bool a_compressed) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    if (a_thread->GetNumParams() > 3) return GM_EXCEPTION;

    int area = 1; // FILEHELPER_AREA_GAMEDATA
    if (a_thread->GetNumParams() >= 2) {
        // Retail forwards the argument's raw payload to FileHelper.
        area = a_thread->Param(1).m_value.m_int;
    }
    // Retail's third argument selects string values in XML leaf elements;
    // the native converter emits GM_STRING even when the value looks numeric.
    const bool forceStringValues = a_thread->GetNumParams() == 3 &&
                                   a_thread->Param(2).m_value.m_int != 0;

    std::string text;
    const bool found = ReadGameMonkeyScriptFile(area, path, text);
    if (!found) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromXMLFile: '%s' not found\n", path);
        a_thread->PushNull();
        return GM_OK;
    }
    if (a_compressed && !InflateRequired(text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromCompressedXMLFile: zlib inflate failed for '%s'\n", path);
        a_thread->PushNull();
        return GM_OK;
    }

    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* table = ParseXmlTable(machine, text, forceStringValues);
    if (!table) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromXMLFile: XML parse failed for '%s'\n",
                     path);
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromXMLFile(gmThread* a_thread) {
    return LoadTableFromXMLFile(a_thread, false);
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromCompressedXMLFile(gmThread* a_thread) {
    return LoadTableFromXMLFile(a_thread, true);
}

int LoadTableFromXMLSaveRequest(gmThread* a_thread, bool a_compressed) {
    GM_CHECK_NUM_PARAMS(1);
    if (a_thread->GetNumParams() > 2) return GM_EXCEPTION;
    std::string ticket;
    if (!SaveTicketFilename(*a_thread->GetMachine(), a_thread->Param(0), ticket)) {
        a_thread->PushNull();
        return GM_OK;
    }
    const bool forceStringValues = a_thread->GetNumParams() == 2 &&
                                   a_thread->Param(1).m_value.m_int != 0;
    std::string text;
    if (!SaveData::ReadFile(ticket, text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromXMLSaveRequest: '%s' not found in save dir\n",
                     ticket.c_str());
        a_thread->PushNull();
        return GM_OK;
    }
    if (a_compressed && !InflateRequired(text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromCompressedXMLSaveRequest: zlib inflate failed for '%s'\n",
                     ticket.c_str());
        a_thread->PushNull();
        return GM_OK;
    }
    gmTableObject* table = ParseXmlTable(*a_thread->GetMachine(), text,
                                         forceStringValues);
    if (!table) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromXMLSaveRequest: XML parse failed for '%s'\n",
                     ticket.c_str());
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromXMLSaveRequest(gmThread* a_thread) {
    return LoadTableFromXMLSaveRequest(a_thread, false);
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromCompressedXMLSaveRequest(
    gmThread* a_thread) {
    return LoadTableFromXMLSaveRequest(a_thread, true);
}

int LoadTableFromJsonFile(gmThread* a_thread, bool a_compressed) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    if (a_thread->GetNumParams() > (a_compressed ? 3 : 2)) return GM_EXCEPTION;
    int area = 1;
    if (a_thread->GetNumParams() >= 2) {
        area = a_thread->Param(1).m_value.m_int;
    }

    std::string text;
    const bool found = ReadGameMonkeyScriptFile(area, path, text);
    if (!found) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonFile: '%s' not found\n", path);
        a_thread->PushNull();
        return GM_OK;
    }
    if (a_compressed && !InflateRequired(text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromCompressedJsonFile: zlib inflate failed for '%s'\n", path);
        a_thread->PushNull();
        return GM_OK;
    }

    rapidjson::Document doc;
    if (doc.Parse<rapidjson::kParseTrailingCommasFlag>(text.c_str()).HasParseError()) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonFile: JSON parse failed for '%s'\n", path);
        a_thread->PushNull();
        return GM_OK;
    }

    a_thread->Push(JsonToGmVariable(*a_thread->GetMachine(), doc));
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromJsonFile(gmThread* a_thread) {
    return LoadTableFromJsonFile(a_thread, false);
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromCompressedJsonFile(gmThread* a_thread) {
    return LoadTableFromJsonFile(a_thread, true);
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromJsonString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(json, 0, "");
    rapidjson::Document doc;
    if (doc.Parse<rapidjson::kParseTrailingCommasFlag>(json).HasParseError()) {
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->Push(JsonToGmVariable(*a_thread->GetMachine(), doc));
    return GM_OK;
}

// --- save-data JSON read/write (GmVariableToJson / GmTableToJsonString) ---
//
// Reverse of JsonToGmVariable above -- needed for
// GameMonkeyScript.saveTableToJsonSaveRequest, the write half of the save
// path (see core/save_data.h for why save data is plain JSON on disk rather
// than retail's opaque PS3 SaveData block). Mirrors JsonToGmVariable's own
// array/object convention so a save/load round-trip is lossless: a table
// whose keys are exactly the ints 0..count-1 serializes as a JSON array
// (matching how JsonToGmVariable turns a JSON array back into exactly that
// shape of table); everything else -- string keys, sparse/non-zero-based int
// keys -- serializes as a JSON object with stringified keys.
bool IsArrayLikeTable(gmTableObject* a_table, int& a_outCount) {
    int count = 0;
    gmTableIterator countIt;
    for (gmTableNode* node = a_table->GetFirst(countIt); !a_table->IsNull(countIt); node = a_table->GetNext(countIt)) {
        if (node->m_key.m_type != GM_INT) return false;
        ++count;
    }
    // An empty table serializes as {} not [] -- GM tables are fundamentally
    // associative; [] is only earned by actually having 0..n-1 int keys.
    if (count == 0) return false;

    std::vector<bool> seen(count, false);
    gmTableIterator it;
    for (gmTableNode* node = a_table->GetFirst(it); !a_table->IsNull(it); node = a_table->GetNext(it)) {
        gmint k = node->m_key.m_value.m_int;
        if (k < 0 || k >= count || seen[(size_t)k]) return false;
        seen[(size_t)k] = true;
    }
    a_outCount = count;
    return true;
}

void GmVariableToJson(gmMachine& a_machine, const gmVariable& a_var, rapidjson::Value& a_out,
                       rapidjson::Document::AllocatorType& a_alloc) {
    switch (a_var.m_type) {
    case GM_INT:
        a_out.SetInt(a_var.m_value.m_int);
        return;
    case GM_FLOAT: {
        // GM_FLOAT is a 32-bit float; widening it straight to double (as
        // SetDouble((double)f) would) prints the double's full,
        // not-actually-more-precise expansion (2.7f -> "2.700000047683716"),
        // real noise a hand-editor has to wade through for no benefit -- the
        // value read back is bit-identical either way (JsonToGmVariable
        // narrows back to gmfloat on load). Try the shortest decimal
        // precision that still round-trips to the exact same float bit
        // pattern (6 sig figs covers most values; up to 9 is the IEEE 754
        // guarantee for the rare ones that need it) and use that instead --
        // "2.7", not "2.70000005" or "2.700000047683716".
        float original = a_var.m_value.m_float;
        char buf[32];
        for (int precision = 6; precision <= 9; ++precision) {
            std::snprintf(buf, sizeof(buf), "%.*g", precision, (double)original);
            if (std::strtof(buf, nullptr) == original) break;
        }
        a_out.SetDouble(std::strtod(buf, nullptr));
        return;
    }
    case GM_STRING: {
        char buf[1024];
        const char* s = a_var.AsString(&a_machine, buf, sizeof(buf));
        a_out.SetString(s ? s : "", a_alloc);
        return;
    }
    case GM_TABLE: {
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(a_var.m_value.m_ref);
        int count = 0;
        if (IsArrayLikeTable(table, count)) {
            std::vector<gmVariable> ordered((size_t)count);
            gmTableIterator it;
            for (gmTableNode* node = table->GetFirst(it); !table->IsNull(it); node = table->GetNext(it)) {
                ordered[(size_t)node->m_key.m_value.m_int] = node->m_value;
            }
            a_out.SetArray();
            a_out.Reserve((rapidjson::SizeType)count, a_alloc);
            for (const gmVariable& elem : ordered) {
                rapidjson::Value v;
                GmVariableToJson(a_machine, elem, v, a_alloc);
                a_out.PushBack(v, a_alloc);
            }
        } else {
            a_out.SetObject();
            gmTableIterator it;
            char intKeyBuf[32];
            for (gmTableNode* node = table->GetFirst(it); !table->IsNull(it); node = table->GetNext(it)) {
                std::string keyStr;
                if (node->m_key.m_type == GM_STRING) {
                    char buf[256];
                    const char* s = node->m_key.AsString(&a_machine, buf, sizeof(buf));
                    keyStr = s ? s : "";
                } else if (node->m_key.m_type == GM_INT) {
                    std::snprintf(intKeyBuf, sizeof(intKeyBuf), "%d", node->m_key.m_value.m_int);
                    keyStr = intKeyBuf;
                } else {
                    continue; // no observed real usage of a non-int/string table key
                }
                rapidjson::Value v;
                GmVariableToJson(a_machine, node->m_value, v, a_alloc);
                rapidjson::Value key(keyStr.c_str(), (rapidjson::SizeType)keyStr.size(), a_alloc);
                a_out.AddMember(key, v, a_alloc);
            }
        }
        return;
    }
    default:
        a_out.SetNull();
        return;
    }
}

std::string GmTableToJsonString(gmMachine& a_machine, const gmVariable& a_var) {
    rapidjson::Document doc;
    GmVariableToJson(a_machine, a_var, doc, doc.GetAllocator());
    rapidjson::StringBuffer buf;
    // PrettyWriter, not the compact Writer -- this file is meant to be
    // hand-editable (the whole point of moving off the PS3 SaveData block),
    // and pretty-printed JSON is what makes that actually pleasant.
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);
    return buf.GetString();
}

int GM_CDECL gmf_GameMonkeyScript_saveTableToJsonSaveRequest(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(filename, 0, "");
    gmVariable tableVar = a_thread->Param(1);

    std::string json = GmTableToJsonString(*a_thread->GetMachine(), tableVar);
    if (!SaveData::WriteFile(filename, json)) {
        std::fprintf(stderr, "GameMonkeyScript.saveTableToJsonSaveRequest: failed to write '%s'\n", filename);
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->PushNewString(filename);
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromJsonSaveRequest(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    std::string ticket;
    if (!SaveTicketFilename(*a_thread->GetMachine(), a_thread->Param(0), ticket)) {
        a_thread->PushNull();
        return GM_OK;
    }

    std::string text;
    if (!SaveData::ReadFile(ticket, text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonSaveRequest: '%s' not found in save dir\n", ticket.c_str());
        a_thread->PushNull();
        return GM_OK;
    }
    rapidjson::Document doc;
    if (doc.Parse<rapidjson::kParseTrailingCommasFlag>(text.c_str()).HasParseError()) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonSaveRequest: JSON parse failed for '%s'\n", ticket.c_str());
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->Push(JsonToGmVariable(*a_thread->GetMachine(), doc));
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_loadTableFromCompressedJsonSaveRequest(
    gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    std::string ticket;
    if (!SaveTicketFilename(*a_thread->GetMachine(), a_thread->Param(0), ticket)) {
        a_thread->PushNull();
        return GM_OK;
    }
    if (a_thread->GetNumParams() != 1) return GM_EXCEPTION;
    std::string text;
    if (!SaveData::ReadFile(ticket, text) || !InflateRequired(text)) {
        a_thread->PushNull();
        return GM_OK;
    }
    rapidjson::Document doc;
    if (doc.Parse<rapidjson::kParseTrailingCommasFlag>(text.c_str()).HasParseError()) {
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->Push(JsonToGmVariable(*a_thread->GetMachine(), doc));
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_setIsCheckingInfiniteLoop(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_clean(gmThread* a_thread) {
    const int argc = a_thread->GetNumParams();
    if (argc == 0) {
        g_cleanPassesRemaining = 4;
    } else if (argc == 1) {
        // Retail reads the raw 32-bit argument value, without GM type
        // conversion, then clamps nonpositive counts to one.
        g_cleanPassesRemaining = std::max(a_thread->Param(0).m_value.m_int, 1);
    }
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_isCollectingGarbage(gmThread* a_thread) {
    a_thread->PushInt(g_cleanPassesRemaining > 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_scriptCheckpointNoop(gmThread*) {
    return GM_OK;
}

int GM_CDECL gmf_GameMonkeyScript_getNumObjectsInGC(gmThread* a_thread) {
    a_thread->PushInt(a_thread->GetMachine()->GetGC()->GetNumAllocated());
    return GM_OK;
}

gmFunctionEntry s_gameMonkeyScriptLib[] = {
    {"loadScript", gmf_GameMonkeyScript_loadScript, nullptr},
    {"setIsCheckingInfiniteLoop", gmf_GameMonkeyScript_setIsCheckingInfiniteLoop, nullptr},
    {"setScriptCheckpoint", gmf_GameMonkeyScript_scriptCheckpointNoop, nullptr},
    {"restoreScriptCheckpoint", gmf_GameMonkeyScript_scriptCheckpointNoop, nullptr},
    {"clean", gmf_GameMonkeyScript_clean, nullptr},
    {"isCollectingGarbage", gmf_GameMonkeyScript_isCollectingGarbage, nullptr},
    {"getNumObjectsInGC", gmf_GameMonkeyScript_getNumObjectsInGC, nullptr},
    {"loadTableFromXMLFile", gmf_GameMonkeyScript_loadTableFromXMLFile, nullptr},
    {"loadTableFromCompressedXMLFile", gmf_GameMonkeyScript_loadTableFromCompressedXMLFile, nullptr},
    {"loadTableFromXMLSaveRequest", gmf_GameMonkeyScript_loadTableFromXMLSaveRequest, nullptr},
    {"loadTableFromCompressedXMLSaveRequest", gmf_GameMonkeyScript_loadTableFromCompressedXMLSaveRequest, nullptr},
    {"loadTableFromJsonFile", gmf_GameMonkeyScript_loadTableFromJsonFile, nullptr},
    {"loadTableFromCompressedJsonFile", gmf_GameMonkeyScript_loadTableFromCompressedJsonFile, nullptr},
    {"loadTableFromJsonString", gmf_GameMonkeyScript_loadTableFromJsonString, nullptr},
    {"saveTableToJsonSaveRequest", gmf_GameMonkeyScript_saveTableToJsonSaveRequest, nullptr},
    {"loadTableFromJsonSaveRequest", gmf_GameMonkeyScript_loadTableFromJsonSaveRequest, nullptr},
    {"loadTableFromCompressedJsonSaveRequest", gmf_GameMonkeyScript_loadTableFromCompressedJsonSaveRequest, nullptr},
};

} // namespace

void RequestGameMonkeyScriptCleanPasses(int a_passes) {
    g_cleanPassesRemaining = std::max(a_passes, 1);
}

int GetGameMonkeyScriptCleanPassesRemaining() {
    return g_cleanPassesRemaining;
}

void ServiceGameMonkeyScriptCleanPass(gmMachine& a_machine) {
    if (g_cleanPassesRemaining <= 0) return;
    // Approximation only: retail advances a separately managed, budgeted
    // cleanup cycle here; the local VM exposes only a forced full collection.
    a_machine.CollectGarbage(true);
    --g_cleanPassesRemaining;
}

gmVariable JsonStringToGmVariable(gmMachine& a_machine, const std::string& a_json, const gmVariable* a_extra) {
    rapidjson::Document doc;
    doc.Parse(a_json.c_str());
    if (doc.HasParseError()) return gmVariable::s_null;
    if (a_extra && a_extra->m_type == GM_TABLE && doc.IsObject()) {
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(a_extra->m_value.m_ref);
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
            SetJsonObjectKey(a_machine, table, it->name.GetString(),
                             JsonToGmVariable(a_machine, it->value));
        }
        return *a_extra;
    }
    return JsonToGmVariable(a_machine, doc);
}

std::string GmVariableToJsonString(gmMachine& a_machine, const gmVariable& a_var) {
    return GmTableToJsonString(a_machine, a_var);
}

void RegisterGameMonkeyScriptBindings(gmMachine& a_machine) {
    a_machine.RegisterLibrary(s_gameMonkeyScriptLib, sizeof(s_gameMonkeyScriptLib) / sizeof(s_gameMonkeyScriptLib[0]), "GameMonkeyScript");
}
