#include "gmThread.h"
#include "watermelon_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <string>


namespace {

// =====================================================================
// LOCAL helpers -- real semantics, used in single-player paths too.
// =====================================================================

int GM_CDECL gmf_GetLevelObject(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(levelId, 0, "");
    char buf[500];
    std::snprintf(buf, sizeof(buf), "~level:%s", levelId);
    a_thread->PushNewString(buf);
    return GM_OK;
}

int GM_CDECL gmf_ParseLevelObjectString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(str, 0, "");
    GM_TABLE_PARAM(table, 1);
    gmMachine& m = *a_thread->GetMachine();
    char levelId[101] = {0};
    char uploadId[101] = {0};
    const char* slash = std::strchr(str, '/');
    if (slash) {
        std::snprintf(levelId, sizeof(levelId), "%.*s", (int)(slash - str), str);
        std::snprintf(uploadId, sizeof(uploadId), "%.100s", slash + 1);
    } else {
        std::snprintf(levelId, sizeof(levelId), "%.100s", str);
    }
    table->Set(&m, "levelId", gmVariable(m.AllocStringObject(levelId)));
    table->Set(&m, "uploadId", gmVariable(m.AllocStringObject(uploadId)));
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_BaseKeyValue(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(str, 0, "");
    GM_TABLE_PARAM(table, 1);
    gmMachine& m = *a_thread->GetMachine();
    char base[201] = {0};
    char value[201] = {0};
    const char* slash = std::strrchr(str, '/');
    if (slash) {
        std::snprintf(base, sizeof(base), "%.*s", (int)(slash - str), str);
        std::snprintf(value, sizeof(value), "%.200s", slash + 1);
    } else {
        std::snprintf(value, sizeof(value), "%.200s", str);
    }
    table->Set(&m, "key", gmVariable(m.AllocStringObject(str)));
    table->Set(&m, "value", gmVariable(m.AllocStringObject(value)));
    table->Set(&m, "base", gmVariable(m.AllocStringObject(base)));
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_PackScore(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(tokens, 0, 0);
    GM_INT_PARAM(playTime, 1, 0);
    long long t = tokens;
    if (t > 0x7ffe) t = 0x7ffe;
    unsigned long long packed =
        ((unsigned long long)(0x7fff - t) << 32) | (uint32_t)playTime;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%llu", packed);
    a_thread->PushNewString(buf);
    return GM_OK;
}

int GM_CDECL gmf_UnpackScore(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(str, 0, "");
    GM_TABLE_PARAM(table, 1);
    gmMachine& m = *a_thread->GetMachine();
    unsigned long long packed = std::strtoull(str, nullptr, 10);
    table->Set(&m, "tokens", gmVariable((gmint)(0x7fff - (gmint)(packed >> 32))));
    table->Set(&m, "time", gmVariable((gmint)(uint32_t)(packed & 0xffffffffu)));
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_IsScoreGreaterThan(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(a, 0, "");
    GM_STRING_PARAM(b, 1, "");
    a_thread->PushInt(std::strtoull(a, nullptr, 0) < std::strtoull(b, nullptr, 0) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_UrlEncode(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    char buf[0x200];
    size_t o = 0;
    for (const unsigned char* p = (const unsigned char*)str; *p && o + 4 < sizeof(buf); ++p) {
        unsigned char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            buf[o++] = (char)c;
        } else {
            o += std::snprintf(buf + o, sizeof(buf) - o, "%%%02X", c);
        }
    }
    buf[o] = 0;
    a_thread->PushNewString(buf);
    return GM_OK;
}

int GM_CDECL gmf_GameMonkey2QueryString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_TABLE_PARAM(table, 0);
    gmMachine& m = *a_thread->GetMachine();
    std::string out;
    gmTableIterator it;
    for (gmTableNode* n = table->GetFirst(it); !table->IsNull(it); n = table->GetNext(it)) {
        char kbuf[64], vbuf[256];
        const char* k = n->m_key.AsString(&m, kbuf, sizeof(kbuf));
        const char* v = n->m_value.AsString(&m, vbuf, sizeof(vbuf));
        if (!k || !v) continue;
        if (!out.empty()) out += '&';
        out += k;
        out += '=';
        out += v;
        if (out.size() >= 2000) break;
    }
    a_thread->PushNewString(out.c_str());
    return GM_OK;
}

int GM_CDECL gmf_QueryString2GameMonkey(gmThread* a_thread) {
    if (GM_NUM_PARAMS < 1 || GM_NUM_PARAMS > 2) return GM_EXCEPTION;
    GM_STRING_PARAM(str, 0, "");
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* table = nullptr;
    if (GM_NUM_PARAMS == 2) table = a_thread->ParamTable(1);
    if (!table) table = m.AllocTableObject();
    const char* p = str;
    while (*p) {
        const char* amp = std::strchr(p, '&');
        size_t seglen = amp ? (size_t)(amp - p) : std::strlen(p);
        const char* eq = (const char*)std::memchr(p, '=', seglen);
        if (eq) {
            std::string k(p, eq);
            std::string v(eq + 1, p + seglen);
            table->Set(&m, k.c_str(), gmVariable(m.AllocStringObject(v.c_str())));
        }
        p += seglen + (amp ? 1 : 0);
    }
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_GetUserIdentityId(gmThread* a_thread) {
    a_thread->PushNewString("/~identity/local");
    return GM_OK;
}

// Watermelon_IsOnline() -> bool. Permanently offline, matching the
// OnlineCoordinator_* stance.
int GM_CDECL gmf_IsOnline(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }

int GM_CDECL gmf_NoOp(gmThread*) { return GM_OK; }

int GM_CDECL gmf_Globals_GetFeaturedSearches(gmThread* a_thread) {
    a_thread->PushTable(a_thread->GetMachine()->AllocTableObject());
    return GM_OK;
}

int GM_CDECL gmf_Globals_GetNews(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* arr = m.AllocTableObject();
    arr->Set(&m, gmVariable((gmint)0), gmVariable(m.AllocTableObject()));
    a_thread->PushTable(arr);
    return GM_OK;
}


int GM_CDECL gmf_OfflineRequest(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(0);
    return GM_OK;
}

} // namespace

void RegisterWatermelonBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("Watermelon_GetLevelObject", gmf_GetLevelObject);
    a_machine.RegisterLibraryFunction("Watermelon_ParseLevelObjectString", gmf_ParseLevelObjectString);
    a_machine.RegisterLibraryFunction("Watermelon_BaseKeyValue", gmf_BaseKeyValue);
    a_machine.RegisterLibraryFunction("Watermelon_GameMonkey2QueryString", gmf_GameMonkey2QueryString);
    a_machine.RegisterLibraryFunction("Watermelon_QueryString2GameMonkey", gmf_QueryString2GameMonkey);
    a_machine.RegisterLibraryFunction("Watermelon_UrlEncode", gmf_UrlEncode);
    a_machine.RegisterLibraryFunction("Watermelon_PackScore", gmf_PackScore);
    a_machine.RegisterLibraryFunction("Watermelon_UnpackScore", gmf_UnpackScore);
    a_machine.RegisterLibraryFunction("Watermelon_IsScoreGreaterThan", gmf_IsScoreGreaterThan);
    a_machine.RegisterLibraryFunction("Watermelon_GetUserIdentityId", gmf_GetUserIdentityId);
    a_machine.RegisterLibraryFunction("Watermelon_IsOnline", gmf_IsOnline);
    a_machine.RegisterLibraryFunction("Watermelon_Purge", gmf_NoOp);
    a_machine.RegisterLibraryFunction("Watermelon_ForceCachePurgeByFilter", gmf_NoOp);
    a_machine.RegisterLibraryFunction("WatermelonGlobals_GetFeaturedSearches", gmf_Globals_GetFeaturedSearches);
    a_machine.RegisterLibraryFunction("WatermelonGlobals_GetNews", gmf_Globals_GetNews);

    // Server-request family -- see the offline-stub block comment above.
    static const char* kOfflineRequests[] = {
        "Watermelon_GetEula",
        "Watermelon_GetEulaVersion",
        "Watermelon_PublishLevel",
        "Watermelon_UnpublishLevel",
        "Watermelon_GetLevelDescription",
        "Watermelon_GetLevelThumbnail",
        "Watermelon_GetLevelData",
        "Watermelon_SaveLevelAndAssetsToDisk",
        "Watermelon_GetLevelVersions",
        "Watermelon_GetSnapshot",
        "Watermelon_GetSoundData",
        "Watermelon_Index_Page",
        "Watermelon_Favourite_All",
        "Watermelon_Favourite_Put",
        "Watermelon_Favourite_Delete",
        "Watermelon_Metadata_All",
        "Watermelon_Metadata_Get",
        "Watermelon_Metadata_Put",
        "Watermelon_Rating_Get",
        "Watermelon_Rating_Put",
        "Watermelon_Leaderboard_Get",
        "Watermelon_Leaderboard_Post",
        "Watermelon_Leaderboard_All",
        "Watermelon_Leaderboard_Page",
        "Watermelon_Leaderboard_Near",
        "Watermelon_Record_Post",
        "Watermelon_Grief_Post",
        "Watermelon_LevelHearts_Post",
        "Watermelon_LevelHearts_Get",
        "Watermelon_LevelHearts_Delete",
        "Watermelon_LevelHearts_Page",
        "Watermelon_Get_Friends",
        "Watermelon_Following_All",
        "Watermelon_Following_Page",
        "Watermelon_Following_Put",
        "Watermelon_Following_Delete",
        "Watermelon_Following_Get",
        "Watermelon_Followers_Page",
        "Watermelon_ActivityStream_Post",
        "Watermelon_ActivityStream_Page",
        "Watermelon_FeaturedLevel_Post",
        "Watermelon_FeaturedLevel_Get",
        "Watermelon_Queued_Page",
        "Watermelon_Queued_Put",
        "Watermelon_Queued_Get",
        "Watermelon_Queued_Delete",
        "Watermelon_Albums_Page",
        "Watermelon_Album_Page",
        "Watermelon_GetContentImage",
        "Watermelon_GetBetaParticipationAgreement",
        "Watermelon_GetCloudSave",
        "Watermelon_PostCloudSave",
    };
    for (const char* name : kOfflineRequests)
        a_machine.RegisterLibraryFunction(name, gmf_OfflineRequest);
}
