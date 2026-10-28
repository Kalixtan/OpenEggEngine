#include "gmThread.h"
#include "watermelon_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "../core/network_client.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <string>


namespace {

NetworkClient* g_networkClient = nullptr;

std::string EncodeRetailUrl(const std::string& a_text) {
    char buffer[0x200];
    size_t out = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(a_text.c_str());
         *p && out + 4 < sizeof(buffer); ++p) {
        const unsigned char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            buffer[out++] = static_cast<char>(c);
        } else {
            out += static_cast<size_t>(std::snprintf(buffer + out, sizeof(buffer) - out,
                                                     "%%%02X", c));
        }
    }
    buffer[out] = '\0';
    return buffer;
}

int GM_CDECL gmf_IndexPage(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 3 || a_thread->GetNumParams() > 4)
        return GM_EXCEPTION;

    const gmVariable callback = a_thread->Param(0);
    const gmVariable callbackData = a_thread->Param(1);
    const gmVariable tag = a_thread->Param(2);
    std::string query;
    if (a_thread->GetNumParams() == 4 && a_thread->Param(3).m_type == GM_STRING) {
        char buffer[2048] = {};
        const char* value = a_thread->Param(3).AsString(
            a_thread->GetMachine(), buffer, sizeof(buffer));
        if (value) query.assign(value);
    }

    std::string type = "*";
    size_t pos = 0;
    while (pos < query.size()) {
        const size_t end = query.find('&', pos);
        const size_t pairEnd = end == std::string::npos ? query.size() : end;
        if (query.compare(pos, 5, "type=") == 0) {
            type = query.substr(pos + 5, pairEnd - pos - 5);
            break;
        }
        if (end == std::string::npos) break;
        pos = end + 1;
    }

    std::string path = "/otg/~index:" + type + ".page";
    if (!query.empty()) path += "?" + query;
    const int requestId = g_networkClient
        ? g_networkClient->QueueRetailGet(path, callback, callbackData, tag, 0, 0)
        : 0;
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

bool ReadOptionalString(gmThread* a_thread, int a_index, std::string& a_value) {
    if (a_thread->Param(a_index).m_type != GM_STRING) return false;
    char buffer[2048] = {};
    const char* value = a_thread->Param(a_index).AsString(
        a_thread->GetMachine(), buffer, sizeof(buffer));
    if (!value) return false;
    a_value.assign(value);
    return true;
}

int QueueAlbumPage(gmThread* a_thread, bool a_singleAlbum) {
    const int count = a_thread->GetNumParams();
    const int minimum = a_singleAlbum ? 4 : 3;
    const int maximum = minimum + 1;
    if (count < minimum || count > maximum) {
        a_thread->PushInt(-1);
        return GM_OK;
    }

    std::string path = a_singleAlbum ? "/otg/~album:" : "/otg/~albums";
    if (a_singleAlbum) {
        std::string albumId;
        if (!ReadOptionalString(a_thread, 3, albumId) || albumId.empty()) {
            a_thread->PushInt(0);
            return GM_OK;
        }
        path += albumId;
    }
    path += "/~link:*.page";

    const int queryIndex = minimum;
    if (count == maximum) {
        std::string query;
        if (ReadOptionalString(a_thread, queryIndex, query) && !query.empty())
            path += "?" + query;
    }

    const int requestId = g_networkClient
        ? g_networkClient->QueueRetailGet(path, a_thread->Param(0), a_thread->Param(1),
                                          a_thread->Param(2), 0, 0)
        : 0;
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_AlbumsPage(gmThread* a_thread) {
    return QueueAlbumPage(a_thread, false);
}

int GM_CDECL gmf_AlbumPage(gmThread* a_thread) {
    return QueueAlbumPage(a_thread, true);
}

int QueueUserPage(gmThread* a_thread, bool a_followers) {
    const int count = a_thread->GetNumParams();
    if (count != 4 && count != 5) {
        a_thread->PushInt(-1);
        return GM_OK;
    }

    std::string identity;
    if (!ReadOptionalString(a_thread, 3, identity) || identity.empty()) {
        if (!g_networkClient || g_networkClient->UserId().empty()) {
            a_thread->PushInt(0);
            return GM_OK;
        }
        identity = g_networkClient->UserId();
    }

    std::string path = "/otg/~identity:" + identity;
    path += a_followers ? "/~followers.page" : "/~follow:*.page";
    if (count == 5) {
        std::string query;
        if (ReadOptionalString(a_thread, 4, query) && !query.empty())
            path += "?" + query;
    }

    const int requestId = g_networkClient
        ? g_networkClient->QueueRetailGet(path, a_thread->Param(0), a_thread->Param(1),
                                          a_thread->Param(2), 0, 0)
        : 0;
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_FollowingPage(gmThread* a_thread) {
    return QueueUserPage(a_thread, false);
}

int GM_CDECL gmf_FollowersPage(gmThread* a_thread) {
    return QueueUserPage(a_thread, true);
}

int QueueFollowingAction(gmThread* a_thread, const char* a_operation,
                         bool a_blockSelfFollow) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string target;
    if (!ReadOptionalString(a_thread, 3, target) || !g_networkClient ||
        g_networkClient->UserId().empty()) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (a_blockSelfFollow && target == g_networkClient->UserId()) {
        a_thread->PushInt(0);
        return GM_OK;
    }

    const std::string path = "/otg/~identity:" + g_networkClient->UserId() +
        "/~follow:" + EncodeRetailUrl("/~identity:" + target) + "." + a_operation;
    const int requestId = g_networkClient->QueueRetailGet(
        path, a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_FollowingGet(gmThread* a_thread) {
    return QueueFollowingAction(a_thread, "get", false);
}

int GM_CDECL gmf_FollowingPut(gmThread* a_thread) {
    return QueueFollowingAction(a_thread, "put", true);
}

int GM_CDECL gmf_FollowingDelete(gmThread* a_thread) {
    return QueueFollowingAction(a_thread, "delete", false);
}

int GM_CDECL gmf_QueuedPage(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 3 && count != 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    if (!g_networkClient || g_networkClient->UserId().empty()) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    std::string path = "/otg/~identity:" + g_networkClient->UserId() +
                       "/~queued:*.page";
    if (count == 4) {
        std::string query;
        if (ReadOptionalString(a_thread, 3, query) && !query.empty())
            path += "?" + query;
    }
    const int requestId = g_networkClient->QueueRetailGet(
        path, a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

int QueueQueuedAction(gmThread* a_thread, const char* a_operation) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string levelId;
    if (!ReadOptionalString(a_thread, 3, levelId) || !g_networkClient ||
        g_networkClient->UserId().empty()) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const std::string path = "/otg/~identity:" + g_networkClient->UserId() +
        "/~queued:" + EncodeRetailUrl("/~level:" + levelId) + "." + a_operation;
    const int requestId = g_networkClient->QueueRetailGet(
        path, a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_QueuedPut(gmThread* a_thread) {
    return QueueQueuedAction(a_thread, "put");
}

int GM_CDECL gmf_QueuedGet(gmThread* a_thread) {
    return QueueQueuedAction(a_thread, "get");
}

int GM_CDECL gmf_QueuedDelete(gmThread* a_thread) {
    return QueueQueuedAction(a_thread, "delete");
}

int GM_CDECL gmf_ActivityStreamPage(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 5 && count != 6) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string identity;
    std::string stream;
    if (!ReadOptionalString(a_thread, 3, identity) ||
        !ReadOptionalString(a_thread, 4, stream) || identity.empty() || stream.empty()) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    std::string path = "/otg/~identity:" + identity + "/~stream:" + stream + ".page";
    if (count == 6) {
        std::string query;
        if (ReadOptionalString(a_thread, 5, query) && !query.empty())
            path += "?" + query;
    }
    const int requestId = g_networkClient
        ? g_networkClient->QueueRetailGet(path, a_thread->Param(0), a_thread->Param(1),
                                          a_thread->Param(2), 0, 0)
        : 0;
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_FeaturedLevelGet(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string authorId;
    if (!ReadOptionalString(a_thread, 3, authorId) || authorId.empty()) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const std::string path = "/otg/~identity:" + authorId +
                             "/~metadata:featuredlevel.get";
    const int requestId = g_networkClient
        ? g_networkClient->QueueRetailGet(path, a_thread->Param(0), a_thread->Param(1),
                                          a_thread->Param(2), 0, 1)
        : 0;
    a_thread->PushInt(requestId != 0 ? 1 : 0);
    return GM_OK;
}

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
    const std::string encoded = EncodeRetailUrl(str);
    a_thread->PushNewString(encoded.c_str());
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

int GM_CDECL gmf_Purge(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(bucket, 0, 0);
    if (g_networkClient) g_networkClient->Purge(bucket);
    return GM_OK;
}

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

int GM_CDECL gmf_OpenEggNetworkGet(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_STRING_PARAM(path, 0, "");
    if (!g_networkClient) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    a_thread->PushInt(g_networkClient->QueueGet(path, a_thread->Param(1), a_thread->Param(2)));
    return GM_OK;
}

int GM_CDECL gmf_OpenEggNetworkPost(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(5);
    GM_STRING_PARAM(path, 0, "");
    GM_STRING_PARAM(body, 1, "");
    GM_STRING_PARAM(contentType, 2, "");
    if (!g_networkClient) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    a_thread->PushInt(g_networkClient->QueueRetailPost(
        path, body, contentType, a_thread->Param(3), a_thread->Param(4),
        gmVariable::s_null, 0, -1));
    return GM_OK;
}

} // namespace

void RegisterWatermelonBindings(gmMachine& a_machine, NetworkClient& a_networkClient) {
    g_networkClient = &a_networkClient;
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
    a_machine.RegisterLibraryFunction("Watermelon_Purge", gmf_Purge);
    a_machine.RegisterLibraryFunction("Watermelon_ForceCachePurgeByFilter", gmf_NoOp);
    a_machine.RegisterLibraryFunction("WatermelonGlobals_GetFeaturedSearches", gmf_Globals_GetFeaturedSearches);
    a_machine.RegisterLibraryFunction("WatermelonGlobals_GetNews", gmf_Globals_GetNews);
    a_machine.RegisterLibraryFunction("Watermelon_Index_Page", gmf_IndexPage);
    a_machine.RegisterLibraryFunction("Watermelon_Albums_Page", gmf_AlbumsPage);
    a_machine.RegisterLibraryFunction("Watermelon_Album_Page", gmf_AlbumPage);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Page", gmf_FollowingPage);
    a_machine.RegisterLibraryFunction("Watermelon_Followers_Page", gmf_FollowersPage);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Get", gmf_FollowingGet);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Put", gmf_FollowingPut);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Delete", gmf_FollowingDelete);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Page", gmf_QueuedPage);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Put", gmf_QueuedPut);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Get", gmf_QueuedGet);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Delete", gmf_QueuedDelete);
    a_machine.RegisterLibraryFunction("Watermelon_ActivityStream_Page", gmf_ActivityStreamPage);
    a_machine.RegisterLibraryFunction("Watermelon_FeaturedLevel_Get", gmf_FeaturedLevelGet);
    // Local test hook. Not part of the retail GameMonkey API.
    a_machine.RegisterLibraryFunction("OpenEgg_NetworkGet", gmf_OpenEggNetworkGet);
    a_machine.RegisterLibraryFunction("OpenEgg_NetworkPost", gmf_OpenEggNetworkPost);

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
        "Watermelon_ActivityStream_Post",
        "Watermelon_FeaturedLevel_Post",
        "Watermelon_GetContentImage",
        "Watermelon_GetBetaParticipationAgreement",
        "Watermelon_GetCloudSave",
        "Watermelon_PostCloudSave",
    };
    for (const char* name : kOfflineRequests)
        a_machine.RegisterLibraryFunction(name, gmf_OfflineRequest);
}
