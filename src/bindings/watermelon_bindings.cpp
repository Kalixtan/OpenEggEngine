#include "gmThread.h"
#include "watermelon_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <string>

// See watermelon_bindings.h for the overall stance. Constants recovered from
// EBOOT.elf .rodata @ 0x39be00-0x39be60: "levelId", "uploadId", "key",
// "value", "base", "%llu", "tokens", "time", "type", "/", "~identity",
// "/~identity", "~level" -- these pin the table field names and the score
// pack format below.

namespace {

// =====================================================================
// LOCAL helpers -- real semantics, used in single-player paths too.
// =====================================================================

// Watermelon_GetLevelObject(levelId) -> string. Confirmed exact
// (0x001b8794): sprintf("~level:%s", levelId).
int GM_CDECL gmf_GetLevelObject(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(levelId, 0, "");
    char buf[500];
    std::snprintf(buf, sizeof(buf), "~level:%s", levelId);
    a_thread->PushNewString(buf);
    return GM_OK;
}

// Watermelon_ParseLevelObjectString(versionID, table) -> table. Confirmed
// (0x001b88a8) to split the string into two <=100-char parts and store them
// as table.levelId / table.uploadId ("levelId"/"uploadId" field names are
// byte-confirmed @ 0x39be04/0x39be0c). The exact split routine
// (FUN_001d3640) isn't in the decompile set; a "/" separator is the only
// separator constant in this rodata block and matches the composite
// "levelId/uploadId" versionID format the corpus round-trips through
// Watermelon_Record_Post(..., results.levelId, ...). Splits at the first
// '/'; no '/' means the whole string is the levelId and uploadId is empty.
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

// Watermelon_BaseKeyValue(id, table) -> table with .key/.value/.base
// (field names byte-confirmed @ 0x39be18/0x39be20/0x39be28). Confirmed
// (0x001b8a18) to split the id into a <=200-char "base"+"key" pair via a
// helper not in the decompile set, plus a UTF-8-safe truncated remainder.
// Server ids in this rodata block look like "/~identity/<value>" --
// base = the "~..." type prefix, value = the trailing id component. Every
// real corpus call site reads only `.value` (e.g. WatermelonHelperNew.gm's
// Watermelon_BaseKeyValue(item.author.id,{}).value), so: value = text after
// the last '/', base = everything before it, key = the full input.
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

// Watermelon_PackScore(tokens, playTimeSeconds) -> string. Confirmed exact
// (0x001b909c): sprintf("%llu", (u64)(0x7fff - min(tokens, 0x7ffe)) << 32
//                               | (u32)playTime).
// Tokens are inverted so that MORE tokens pack to a numerically SMALLER
// u64 -- the server sorts packed scores ascending, and
// Watermelon_IsScoreGreaterThan below compares the same way.
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

// Watermelon_UnpackScore(packedString, table) -> table with .tokens/.time
// (field names byte-confirmed @ 0x39be38/0x39be40; "%llu" parse format
// byte-confirmed @ 0x39be30). Exact inverse of Watermelon_PackScore --
// WinScreenProcessBase.gm's (commented) debug path round-trips
// PackScore -> UnpackScore, which pins these semantics.
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

// Watermelon_IsScoreGreaterThan(scoreA, scoreB) -> bool. Confirmed exact
// (0x001b92f0): strtoull(a,0,0) < strtoull(b,0,0). Because PackScore
// inverts tokens, "numerically less" means "better score", so this answers
// "is scoreA a better score than scoreB".
int GM_CDECL gmf_IsScoreGreaterThan(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(a, 0, "");
    GM_STRING_PARAM(b, 1, "");
    a_thread->PushInt(std::strtoull(a, nullptr, 0) < std::strtoull(b, nullptr, 0) ? 1 : 0);
    return GM_OK;
}

// Watermelon_UrlEncode(str) -> string. Confirmed (0x001b8f60) to
// percent-encode into a 512-byte buffer (encoder body FUN_000149e0 not in
// the decompile set; standard RFC 3986 unreserved-set encoding used here).
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

// Watermelon_GameMonkey2QueryString(table) -> "k=v&k2=v2" string (2000-byte
// buffer in retail, 0x001b8d18). Serializes int/float/string members;
// values are NOT url-encoded here in retail either (the corpus calls
// Watermelon_UrlEncode separately where needed).
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

// Watermelon_QueryString2GameMonkey(str[, table]) -> table. Confirmed
// (0x001b8e28) 1-or-2-arg: with no table argument a fresh one is
// allocated. Parses "k=v&k2=v2" into string-valued members.
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

// Watermelon_GetUserIdentityId() -> string. Retail (0x001b94a0) returns the
// signed-in user's server identity id (a "/~identity/<n>" style handle from
// the Watermelon session). No server session here; a fixed local handle
// keeps every "is this my level?" comparison (authorID ==
// Watermelon_GetUserIdentityId()) consistent -- locally-authored levels
// will match, downloaded/community levels won't, which is correct offline.
int GM_CDECL gmf_GetUserIdentityId(gmThread* a_thread) {
    a_thread->PushNewString("/~identity/local");
    return GM_OK;
}

// Watermelon_IsOnline() -> bool. Permanently offline, matching the
// OnlineCoordinator_* stance.
int GM_CDECL gmf_IsOnline(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }

// Watermelon_Purge(handle) / Watermelon_ForceCachePurgeByFilter(filter) --
// confirmed real cache-management actions, no return value. Nothing is
// ever cached here; no-ops.
int GM_CDECL gmf_NoOp(gmThread*) { return GM_OK; }

// WatermelonGlobals_GetFeaturedSearches() -> table of featured-search
// entries pushed by the server with the session handshake. Empty table
// offline (CommunityScreenNavBar.gm iterates it to build "more" buttons;
// zero entries is handled).
int GM_CDECL gmf_Globals_GetFeaturedSearches(gmThread* a_thread) {
    a_thread->PushTable(a_thread->GetMachine()->AllocTableObject());
    return GM_OK;
}

// WatermelonGlobals_GetNews() -> array of news-item tables from the server
// handshake. CommunityScreenSidePanel.gm indexes `[0]` unconditionally, so
// offline this returns one empty news table rather than an empty array.
int GM_CDECL gmf_Globals_GetNews(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* arr = m.AllocTableObject();
    arr->Set(&m, gmVariable((gmint)0), gmVariable(m.AllocTableObject()));
    a_thread->PushTable(arr);
    return GM_OK;
}

// =====================================================================
// SERVER requests -- permanently-offline stubs.
//
// Retail shape (uniform across the whole family, confirmed from the
// decompiled bodies): Watermelon_X(callback, callbackData, requestBucket,
// <endpoint args...>[, queryString]) formats an HTTP request against the
// Watermelon community server, queues it on the given bucket, and returns
// bool "request started". On completion the engine calls
// callback(resultTable, callbackData) on a GM thread; resultTable carries
// the parsed JSON response.
//
// Pseudocode of what a revived online layer must do per family:
//   PublishLevel(cb, data, bucket, levelTable, ...)   -> POST /levels
//   UnpublishLevel(cb, data, bucket, levelId)         -> DELETE /levels/:id
//   GetLevelDescription/Thumbnail/Data/Versions/
//   GetSnapshot/GetSoundData/GetContentImage          -> GET level assets
//   SaveLevelAndAssetsToDisk(cb, data, bucket, id)    -> GET + write cache
//   Index_Page(cb, data, bucket, url, query)          -> GET paged index
//   Favourite_/Metadata_/Rating_/LevelHearts_/
//   Following_/Followers_/Queued_/Albums_/Album_/
//   ActivityStream_/FeaturedLevel_/Leaderboard_ *     -> REST CRUD on the
//        matching resource (All/Page/Get = GET, Put/Post = write, Delete)
//   Leaderboard_Near(cb, data, bucket, url, identity,
//                    query)                           -> GET scores around
//        the given identity's entry
//   Record_Post(cb, data, bucket, levelId, record)    -> POST score record
//   Grief_Post(cb, data, bucket, reportTable)         -> POST grief report
//        (+ the prepared screenshot from Watermelon_Prepare_Screenshot)
//   Get_Friends(cb, data, bucket)                     -> GET friends list
//   GetEula/GetEulaVersion/
//   GetBetaParticipationAgreement                     -> GET legal docs
//   GetCloudSave/PostCloudSave                        -> cloud save-data
//        sync (single-player saves live in SaveData_* locally, so nothing
//        is lost offline)
//   GetUserIdentityId/SignIn handshake                -> session setup
//
// Offline behavior: return 0 ("request not started") and never invoke the
// callback. Every real corpus call site either ignores the return value
// (fire-and-forget posts) or takes its no-network branch on false (e.g.
// WatermelonHelperNew.gm: if (Watermelon_GetLevelThumbnail(...)) {...}).
// Never invoking the callback is retail-accurate for an unstarted request.
// =====================================================================

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
