#include "gmThread.h"
#include "gamemonkeyscript_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "gmIncGC.h"
#include "../core/engine_context.h"
#include "../core/save_data.h"
#include "../core/strict_calls.h"
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

namespace {

// Retail GameMonkeyScript.clean stores a pending per-frame GC pass count;
// FUN_0018ab1c services one pass and decrements it in the engine update.
int g_cleanPassesRemaining = 0;

// Level files (Platformer/levels/*.xml.z, *.json.z) are real zlib streams
// (confirmed: every ".z" file in the corpus starts with byte 0x78, zlib's
// standard header) -- retail's own fallback-filename dance
// (".xml.z" -> ".xml" -> ".json.z" -> ".json", all four tried per level,
// confirmed by the "LEVEL LOAD FAILED: try uncompressed filename" trace in
// Platformer/Main.gm) only makes sense if SOME of those names are meant to
// be read raw and others decompressed -- detecting by the zlib magic byte
// rather than trusting the ".z" suffix handles both without guessing which
// path a given level actually takes.
bool MaybeInflate(std::string& a_text) {
    if (a_text.size() < 2 || (unsigned char)a_text[0] != 0x78) return true; // not zlib, leave as-is
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

// Confirmed real, reveng: none (no decompiled evidence anywhere) --
// GameMonkeyScript.loadTableFromXMLFile is confirmed real purely by usage
// (game_data/Platformer/Mixer.gm, SystemHelper.gm, Platformer/User.gm).
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

// Whole numbers ("3", "-12", not "3.0"/"3e1") parse to GM_INT, not
// GM_FLOAT -- confirmed load-bearing: GM table indexing distinguishes int
// and float keys (gmTableObject.cpp's VarKeysEqual requires
// `a_varA.m_type == a_varB.m_type`, and GetAtHashPos hashes the raw
// `m_value` bit pattern, so `t[3]` and `t[3.0]` are DIFFERENT entries in
// stock GM). Level XML/JSON integer fields (uid, pngIndex, scaleIndex,
// ...) previously always parsed as GM_FLOAT here, so looking them up
// against tables populated with plain int keys (e.g. ArrayHelper's
// `pushBack`: `this[this.size]=thing;`, `size` always an int) silently
// found nothing -- e.g. `TutorialGraphics_PNGList[this.configureData.pngIndex]`
// always returned null even for a genuinely in-range pngIndex, crashing
// several lines later on the null result ("getdot failed").
bool IsWholeNumber(const char* a_value, size_t a_len) {
    size_t i = (a_len > 0 && (a_value[0] == '-' || a_value[0] == '+')) ? 1 : 0;
    if (i >= a_len) return false;
    for (; i < a_len; ++i) if (!std::isdigit((unsigned char)a_value[i])) return false;
    return true;
}

gmVariable ParseLeafValue(gmMachine& a_machine, const char* a_value) {
    if (!a_value) a_value = "";
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

// a_fallbackIndex: real level XML uses <tableElement key="entities"> for
// keyed/named tables but plain, key-less <tableElement>/<element> children
// for ARRAY-style tables (confirmed: campaignTutorial.xml.z's "entities"
// and "screenData" both hold a bare sequence of unkeyed <tableElement>
// items, one per array entry -- GM itself has no separate array type,
// just tables keyed 0..n-1, matching JsonToGmVariable's IsArray() case
// just below). Every such child used to be silently dropped entirely
// (`if (!key) return;`), which meant every real level's entities/screenData
// arrays loaded as consistently EMPTY tables -- level data parsed
// "successfully" (no XML error) but described a level with no ground, no
// entities, nothing to render.
void ParseXmlElementInto(gmMachine& a_machine, gmTableObject* a_table, const tinyxml2::XMLElement* a_elem, int a_fallbackIndex) {
    const char* key = a_elem->Attribute("key");

    if (std::strcmp(a_elem->Name(), "tableElement") == 0) {
        gmTableObject* child = a_machine.AllocTableObject();
        int childIndex = 0;
        for (const tinyxml2::XMLElement* c = a_elem->FirstChildElement(); c; c = c->NextSiblingElement()) {
            ParseXmlElementInto(a_machine, child, c, childIndex++);
        }
        if (key) SetTableKey(a_machine, a_table, key, gmVariable(child));
        else a_table->Set(&a_machine, a_fallbackIndex, gmVariable(child));
    } else if (std::strcmp(a_elem->Name(), "element") == 0) {
        gmVariable v = ParseLeafValue(a_machine, a_elem->Attribute("value"));
        if (key) SetTableKey(a_machine, a_table, key, v);
        else a_table->Set(&a_machine, a_fallbackIndex, v);
    }
    // Any other/unrecognised tag (e.g. <indexSize/>) is skipped -- not part
    // of the confirmed vocabulary above.
}

// Confirmed real, reveng: none -- GameMonkeyScript.loadTableFromJsonFile
// is confirmed real purely by usage (heavily used for level/UI data
// across the corpus, e.g. game_data/Platformer/Main.gm,
// Platformer/SaveDataHelper.gm). Maps JSON objects/arrays onto GM tables
// (array elements keyed 0..n-1, matching how GM itself has no separate
// array type), numbers to GM_FLOAT, strings to GM_STRING, null to
// GM_NULL. kParseTrailingCommasFlag for the same reason bitmap_font.cpp
// needs it -- real retail data files aren't strict JSON.
gmVariable JsonToGmVariable(gmMachine& a_machine, const rapidjson::Value& a_v) {
    if (a_v.IsObject()) {
        gmTableObject* table = a_machine.AllocTableObject();
        for (auto it = a_v.MemberBegin(); it != a_v.MemberEnd(); ++it) {
            SetTableKey(a_machine, table, it->name.GetString(), JsonToGmVariable(a_machine, it->value));
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
        // Log-and-continue (GM_EXCEPTION_MSG here is just LogEntry, not a
        // thread-killing exception -- returns GM_OK below) is confirmed
        // real retail behavior, not just a convenient shortcut: a real PS3
        // command-line capture shows retail hitting genuinely missing files
        // this exact way and continuing straight on to the splash screen
        // and main menu ("Error - Failed to load file
        // [Platformer/CommunityLevelInfo.gm] -2147416315" / "ERROR: Could
        // not load script ... because file does not exist.", followed by
        // normal boot continuing). Several of the files our own game_data
        // corpus is missing (CommunityLevelInfo.gm, RunStoreProcess.gm,
        // OfflineLevelsTOC.gm, PatchfileHelper.gm, CommonCommunityStuff.gm,
        // PersistentDataHelper.gm) are confirmed missing in retail too, by
        // that same log -- not gaps in our reveng corpus.
        GM_EXCEPTION_MSG("GameMonkeyScript.loadScript: could not find '%s'", path);
        a_thread->PushInt(0);
        return GM_OK;
    }

    int errors = a_thread->GetMachine()->ExecuteString(source.c_str(), nullptr, true, path);
    // Was a genuinely blind spot: gmMachine::ExecuteString reports both
    // compile errors (parse/codegen) AND runtime exceptions from the
    // synchronous Sys_Execute() it triggers into gmMachine::GetLog(), not
    // via any return value we were printing or GM_EXCEPTION -- so a script
    // with a real compile error, or one that throws a runtime exception
    // partway through its top-level code, failed silently with zero
    // visible indication anywhere. `errors` only reflects compile-time
    // failures, so we flush the log unconditionally to also surface
    // runtime exceptions that leave `errors == 0`.
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

int GM_CDECL gmf_GameMonkeyScript_loadTableFromXMLFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    if (a_thread->GetNumParams() > 3) return GM_EXCEPTION;

    int area = 1; // FILEHELPER_AREA_GAMEDATA
    if (a_thread->GetNumParams() >= 2) {
        GM_INT_PARAM(requestedArea, 1, 1);
        area = requestedArea;
    }
    if (area != 0 && area != 1)
        return StrictCalls::Report(a_thread, "GameMonkeyScript.loadTableFromXMLFile(area)");
    if (a_thread->GetNumParams() == 3 && a_thread->Param(2).m_value.m_int != 0)
        return StrictCalls::Report(a_thread, "GameMonkeyScript.loadTableFromXMLFile(parseFlag)");

    std::string text;
    const bool found = area == 0 ? SaveData::ReadFile(path, text)
                                 : GetEngineContext().fileHelper->ReadTextFile(path, text);
    if (!found) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromXMLFile: '%s' not found\n", path);
        a_thread->PushNull();
        return GM_OK;
    }
    if (!MaybeInflate(text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromXMLFile: zlib inflate failed for '%s'\n", path);
        a_thread->PushNull();
        return GM_OK;
    }

    tinyxml2::XMLDocument doc;
    if (doc.Parse(text.c_str(), text.size()) != tinyxml2::XML_SUCCESS) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromXMLFile: XML parse failed for '%s': %s\n",
                     path, doc.ErrorStr());
        a_thread->PushNull();
        return GM_OK;
    }

    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* table = machine.AllocTableObject();
    int topIndex = 0;
    for (const tinyxml2::XMLElement* elem = doc.FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
        ParseXmlElementInto(machine, table, elem, topIndex++);
    }
    a_thread->PushTable(table);
    return GM_OK;
}

// Retail 0x0006e184 accepts (path, [area], [parseFlag]), reads the file,
// decompresses it, then calls the same XML-to-GM table converter family as
// loadTableFromXMLFile. The shipped level corpus calls the two-argument
// GAMEDATA form; all its .xml.z files are zlib streams and the shared XML
// loader above already inflates them before parsing. Savedata area 0 uses
// the same per-title save root as the other FILEHELPER_AREA_SAVEDATA calls.
// The third flag's effect and host/null areas need separate evidence.
int GM_CDECL gmf_GameMonkeyScript_loadTableFromCompressedXMLFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    if (a_thread->GetNumParams() > 3) return GM_EXCEPTION;
    if (a_thread->GetNumParams() >= 2 && a_thread->Param(1).m_type != GM_INT)
        return GM_EXCEPTION;
    if (a_thread->GetNumParams() >= 2 && a_thread->Param(1).m_value.m_int != 0 &&
        a_thread->Param(1).m_value.m_int != 1) {
        return StrictCalls::Report(a_thread, "GameMonkeyScript.loadTableFromCompressedXMLFile(area)");
    }
    if (a_thread->GetNumParams() == 3 && a_thread->Param(2).m_value.m_int != 0) {
        return StrictCalls::Report(a_thread, "GameMonkeyScript.loadTableFromCompressedXMLFile(parseFlag)");
    }
    return gmf_GameMonkeyScript_loadTableFromXMLFile(a_thread);
}

// Confirmed real: 1 or 2 args (path, [area]) -- area follows the same
// FILEHELPER_AREA_* convention as the XML loader above.
int GM_CDECL gmf_GameMonkeyScript_loadTableFromJsonFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    if (a_thread->GetNumParams() > 2) return GM_EXCEPTION;
    int area = 1;
    if (a_thread->GetNumParams() == 2) {
        GM_INT_PARAM(requestedArea, 1, 1);
        area = requestedArea;
    }
    if (area != 0 && area != 1)
        return StrictCalls::Report(a_thread, "GameMonkeyScript.loadTableFromJsonFile(area)");

    std::string text;
    const bool found = area == 0 ? SaveData::ReadFile(path, text)
                                 : GetEngineContext().fileHelper->ReadTextFile(path, text);
    if (!found) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonFile: '%s' not found\n", path);
        a_thread->PushNull();
        return GM_OK;
    }
    if (!MaybeInflate(text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonFile: zlib inflate failed for '%s'\n", path);
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

// GameMonkeyScript.saveTableToJsonSaveRequest(filename, table) -- confirmed
// real (game_data/Platformer/SaveDataHelper.gm's SAVEDATAHELPER_SaveToFile,
// Platformer/MyLevelsTOC.gm's t.save() -- the two real save-write call
// sites). Retail's real body queues an async ticket into Sony's SaveData
// SaveData.c) that scripts never inspect beyond handing straight back to a
// loader -- nothing here needs to reproduce that protocol, only its
// script-visible contract. Writes the JSON immediately (this engine's save
// path is synchronous; SAVEDATA_doRequest below just confirms completion)
// to SaveData::Dir() -- a real "savedata/" folder next to the built
// executable, so it's a plain text file, not a save-data block -- and
// returns the relative filename itself as the "ticket": every real call
// site only ever forwards it to loadTableFromJsonSaveRequest() or discards
// it, so a filename satisfies the contract with nothing invented beyond it.
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

// GameMonkeyScript.loadTableFromJsonSaveRequest(ticket) -- confirmed real
// (Main.gm's InitGame(), MyLevelsTOC.gm's t.load()), the read-side
// counterpart to saveTableToJsonSaveRequest above: resolves a ticket (a
// relative filename string, per this engine's ticket design, see that
// function's comment) against SaveData::Dir() rather than FileHelper's
// game_data-rooted resolve, since save data intentionally lives outside the
// game_data tree entirely.
int GM_CDECL gmf_GameMonkeyScript_loadTableFromJsonSaveRequest(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    if (a_thread->Param(0).m_type != GM_STRING) { a_thread->PushNull(); return GM_OK; }
    GM_STRING_PARAM(ticket, 0, "");

    std::string text;
    if (!SaveData::ReadFile(ticket, text)) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonSaveRequest: '%s' not found in save dir\n", ticket);
        a_thread->PushNull();
        return GM_OK;
    }
    rapidjson::Document doc;
    if (doc.Parse<rapidjson::kParseTrailingCommasFlag>(text.c_str()).HasParseError()) {
        std::fprintf(stderr, "GameMonkeyScript.loadTableFromJsonSaveRequest: JSON parse failed for '%s'\n", ticket);
        a_thread->PushNull();
        return GM_OK;
    }
    a_thread->Push(JsonToGmVariable(*a_thread->GetMachine(), doc));
    return GM_OK;
}

// GameMonkeyScript.setIsCheckingInfiniteLoop(bool) -- confirmed real
// setter). Real call site (Main.gm:3226's setupLevelFromJsonFile():
// `GameMonkeyScript.setIsCheckingInfiniteLoop(false);`) toggles the GM VM's
// own infinite-loop watchdog off before a long synchronous level-load pass
// -- this reimplementation has no such watchdog to disable, so genuine
// no-op.
int GM_CDECL gmf_GameMonkeyScript_setIsCheckingInfiniteLoop(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// GameMonkeyScript.clean([n]) -- retail sets a request count (default 4,
// clamped to at least 1); FUN_0018ab1c consumes one request per frame.
// Retail's collector manager performs an update with its force flag set.
// The local VM exposes a full-collection operation rather than that manager,
// so service each request with its corresponding forced collection.
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

// Retail 0x0006f77c pushes the collector's m_numAllocated field at +0x60.
// The bundled GameMonkey collector already tracks that field under
// GM_GC_STATS; keep the existing counter enabled in Release builds too.
int GM_CDECL gmf_GameMonkeyScript_getNumObjectsInGC(gmThread* a_thread) {
    a_thread->PushInt(a_thread->GetMachine()->GetGC()->GetNumAllocated());
    return GM_OK;
}

gmFunctionEntry s_gameMonkeyScriptLib[] = {
    {"loadScript", gmf_GameMonkeyScript_loadScript, nullptr},
    {"setIsCheckingInfiniteLoop", gmf_GameMonkeyScript_setIsCheckingInfiniteLoop, nullptr},
    {"clean", gmf_GameMonkeyScript_clean, nullptr},
    {"isCollectingGarbage", gmf_GameMonkeyScript_isCollectingGarbage, nullptr},
    {"getNumObjectsInGC", gmf_GameMonkeyScript_getNumObjectsInGC, nullptr},
    {"loadTableFromXMLFile", gmf_GameMonkeyScript_loadTableFromXMLFile, nullptr},
    {"loadTableFromCompressedXMLFile", gmf_GameMonkeyScript_loadTableFromCompressedXMLFile, nullptr},
    {"loadTableFromJsonFile", gmf_GameMonkeyScript_loadTableFromJsonFile, nullptr},
    // GameMonkeyScript.c 0x0006e9c0), same 1-or-2-arg (path,[area]) shape.
    // Genuinely the same native entry point as loadTableFromJsonFile above
    // once MaybeInflate's magic-byte zlib detection is accounted for (both
    // handle compressed and uncompressed input identically) -- Main.gm's
    // own real level-load fallback dance calls whichever one matches the
    // filename it's trying (compressedAreaFilename vs areaFilename) but
    // both ultimately need to inflate-then-parse the same way.
    {"loadTableFromCompressedJsonFile", gmf_GameMonkeyScript_loadTableFromJsonFile, nullptr},
    {"saveTableToJsonSaveRequest", gmf_GameMonkeyScript_saveTableToJsonSaveRequest, nullptr},
    {"loadTableFromJsonSaveRequest", gmf_GameMonkeyScript_loadTableFromJsonSaveRequest, nullptr},
};

} // namespace

void ServiceGameMonkeyScriptCleanPass(gmMachine& a_machine) {
    if (g_cleanPassesRemaining <= 0) return;
    // Approximation only: retail advances a separately managed, budgeted
    // cleanup cycle here; the local VM exposes only a forced full collection.
    a_machine.CollectGarbage(true);
    --g_cleanPassesRemaining;
}

// Exposed for Debug.GameMonkey2JSON/JSON2GameMonkey (confirmed real bare
// around the private implementations above rather than moving them out of
// the anonymous namespace (which would touch everything else in this
// file) or duplicating the JSON<->gmVariable mapping in system_bindings.cpp,
// which would drift independently over time.
gmVariable JsonStringToGmVariable(gmMachine& a_machine, const std::string& a_json, const gmVariable* a_extra) {
    rapidjson::Document doc;
    doc.Parse(a_json.c_str());
    if (doc.HasParseError()) return gmVariable::s_null;
    if (a_extra && a_extra->m_type == GM_TABLE && doc.IsObject()) {
        // JSON2GameMonkey(jsonString, extraTable): confirmed real 2-arg
        // overload merges the decoded object into the caller-supplied
        // table rather than allocating a fresh one.
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(a_extra->m_value.m_ref);
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
            SetTableKey(a_machine, table, it->name.GetString(), JsonToGmVariable(a_machine, it->value));
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
