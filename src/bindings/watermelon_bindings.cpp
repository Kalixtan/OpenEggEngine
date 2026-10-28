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

std::string EncodeRetailUrl(const std::string& a_text, size_t a_capacity = 0x200) {
    char buffer[0x200];
    if (a_capacity > sizeof(buffer)) a_capacity = sizeof(buffer);
    size_t out = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(a_text.c_str());
         *p && out + 4 < a_capacity; ++p) {
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

int QueueMetadata(gmThread* a_thread, int a_operation) {
    const int minimum = a_operation == 0 ? 4 : (a_operation == 1 ? 5 : 6);
    if (a_thread->GetNumParams() < minimum) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base, key, value;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, base) ||
        (a_operation != 0 && !ReadOptionalString(a_thread, 4, key)) ||
        (a_operation == 2 && !ReadOptionalString(a_thread, 5, value))) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += "~metadata:" + (a_operation == 0 ? std::string("*") : EncodeRetailUrl(key)) +
            (a_operation == 2 ? ".put" : ".get");
    int id;
    if (a_operation == 2 && !value.empty()) {
        id = g_networkClient->QueueRetailPost(path, value, "text/plain", a_thread->Param(0),
            a_thread->Param(1), a_thread->Param(2), 0, 0);
    } else {
        id = g_networkClient->QueueRetailGet(path, a_thread->Param(0), a_thread->Param(1),
            a_thread->Param(2), 0, a_operation == 1 ? 1 : 0);
    }
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_MetadataAll(gmThread* a_thread) { return QueueMetadata(a_thread, 0); }
int GM_CDECL gmf_MetadataGet(gmThread* a_thread) { return QueueMetadata(a_thread, 1); }
int GM_CDECL gmf_MetadataPut(gmThread* a_thread) { return QueueMetadata(a_thread, 2); }

int QueueRating(gmThread* a_thread, bool a_put) {
    if (a_thread->GetNumParams() < (a_put ? 5 : 4)) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base;
    if (!g_networkClient || g_networkClient->UserId().empty() ||
        !ReadOptionalString(a_thread, 3, base) ||
        (a_put && a_thread->Param(4).m_type != GM_INT)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += "~rating:" + EncodeRetailUrl("/~identity:" + g_networkClient->UserId()) +
            (a_put ? ".put" : ".get");
    const int id = a_put
        ? g_networkClient->QueueRetailPost(path,
            "value=" + std::to_string(a_thread->Param(4).m_value.m_int), "",
            a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 0)
        : g_networkClient->QueueRetailGet(path, a_thread->Param(0),
            a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_RatingGet(gmThread* a_thread) { return QueueRating(a_thread, false); }
int GM_CDECL gmf_RatingPut(gmThread* a_thread) { return QueueRating(a_thread, true); }

int QueueFavourite(gmThread* a_thread, const char* a_operation, bool a_all) {
    if (a_thread->GetNumParams() < (a_all ? 4 : 5)) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base, key;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, base) ||
        (!a_all && !ReadOptionalString(a_thread, 4, key))) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += "~favourite:" + (a_all ? std::string("*") : EncodeRetailUrl(key)) +
            "." + a_operation;
    const int id = g_networkClient->QueueRetailGet(path, a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_FavouriteAll(gmThread* a_thread) { return QueueFavourite(a_thread, "all", true); }
int GM_CDECL gmf_FavouritePut(gmThread* a_thread) { return QueueFavourite(a_thread, "put", false); }
int GM_CDECL gmf_FavouriteDelete(gmThread* a_thread) { return QueueFavourite(a_thread, "delete", false); }

int GM_CDECL gmf_LeaderboardPage(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 4 && count != 5) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, base)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += "~leaderboard.page";
    if (count == 5) {
        std::string query;
        if (ReadOptionalString(a_thread, 4, query) && !query.empty())
            path += "?" + query;
    }
    const int id = g_networkClient->QueueRetailGet(path, a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 2);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int QueueLeaderboardRead(gmThread* a_thread, const char* a_operation) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, base)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += std::string("~leaderboard.") + a_operation;
    const int id = g_networkClient->QueueRetailGet(path, a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 2);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_LeaderboardGet(gmThread* a_thread) {
    return QueueLeaderboardRead(a_thread, "get");
}

int GM_CDECL gmf_LeaderboardAll(gmThread* a_thread) {
    return QueueLeaderboardRead(a_thread, "all");
}

int GM_CDECL gmf_LeaderboardPost(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 5) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base, score;
    const gmVariable value = a_thread->Param(4);
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, base)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (value.m_type == GM_INT) score = std::to_string(value.m_value.m_int);
    else if (value.m_type != GM_STRING || !ReadOptionalString(a_thread, 4, score)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += "~leaderboard.post";
    const int id = g_networkClient->QueueRetailPost(path, "score=" + score, "",
        a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 2);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_LeaderboardNear(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    if (count != 5 && count != 6) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base, entrant;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, base) ||
        !ReadOptionalString(a_thread, 4, entrant)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += "~leaderboard.near?entrant=" + EncodeRetailUrl(entrant);
    if (count == 6) {
        std::string query;
        if (ReadOptionalString(a_thread, 5, query) && !query.empty()) path += "&" + query;
    }
    const int id = g_networkClient->QueueRetailGet(path, a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 2);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
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

int QueueUserPage(gmThread* a_thread, const char* a_suffix) {
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
    path += a_suffix;
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
    return QueueUserPage(a_thread, "/~follow:*.page");
}

int GM_CDECL gmf_FollowingAll(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 3) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    if (!g_networkClient || g_networkClient->UserId().empty()) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const std::string path = "/otg/~identity:" + g_networkClient->UserId() + "/~follow:*.all";
    const int id = g_networkClient->QueueRetailGet(path, a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_FollowersPage(gmThread* a_thread) {
    return QueueUserPage(a_thread, "/~followers.page");
}

int GM_CDECL gmf_LevelHeartsPage(gmThread* a_thread) {
    return QueueUserPage(a_thread, "/~like:*.page");
}

int QueueLevelHearts(gmThread* a_thread, const char* a_operation, bool a_delete) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string levelId;
    if (!g_networkClient || g_networkClient->UserId().empty() ||
        !ReadOptionalString(a_thread, 3, levelId)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const std::string level = "/~level:" + levelId;
    const std::string path = "/otg/~identity:" + g_networkClient->UserId() +
        "/~like:" + EncodeRetailUrl(level, 0x80) + "." + a_operation;
    const int id = g_networkClient->QueueRetailGet(path, a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 0);
    if (id && a_delete) {
        const std::string rating = "/otg" + level + "/~rating:" +
            EncodeRetailUrl("/~identity:" + g_networkClient->UserId()) + ".delete";
        g_networkClient->QueueRetailGet(rating, gmVariable::s_null, gmVariable::s_null,
                                       gmVariable::s_null, 0, 0);
    }
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_LevelHeartsGet(gmThread* a_thread) { return QueueLevelHearts(a_thread, "get", false); }
int GM_CDECL gmf_LevelHeartsPost(gmThread* a_thread) { return QueueLevelHearts(a_thread, "put", false); }
int GM_CDECL gmf_LevelHeartsDelete(gmThread* a_thread) { return QueueLevelHearts(a_thread, "delete", true); }

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

int GM_CDECL gmf_ActivityStreamPost(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 5) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string base, component;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, base) ||
        !ReadOptionalString(a_thread, 4, component)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    if (!base.empty() && base.front() == '/') base.erase(0, 1);
    std::string path = "/otg/" + base;
    if (!base.empty() && base.back() != '/') path += '/';
    path += "~activity:*";
    if (component.size() > 1 && component.front() != '/') path += '/';
    path += component + ".post";
    const int id = g_networkClient->QueueRetailPost(path, "", "", a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
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

int GM_CDECL gmf_FeaturedLevelPost(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string levelId;
    if (!g_networkClient || g_networkClient->UserId().empty() ||
        !ReadOptionalString(a_thread, 3, levelId)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const std::string path = "/otg/~identity:" + g_networkClient->UserId() +
                             "/~metadata:featuredlevel.put";
    const int id = g_networkClient->QueueRetailPost(path, "/~level:" + levelId,
        "text/plain", a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_RecordPost(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 5) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string levelId;
    gmTableObject* table = a_thread->ParamTable(4);
    if (!g_networkClient || g_networkClient->UserId().empty() || !table ||
        !ReadOptionalString(a_thread, 3, levelId)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    gmMachine& machine = *a_thread->GetMachine();
    std::string body;
    gmTableIterator it;
    for (gmTableNode* node = table->GetFirst(it); !table->IsNull(it); node = table->GetNext(it)) {
        char keyBuffer[256], valueBuffer[256];
        const char* key = node->m_key.AsString(&machine, keyBuffer, sizeof(keyBuffer));
        std::string value;
        if (!key) { a_thread->PushInt(0); return GM_OK; }
        if (node->m_value.m_type == GM_INT)
            value = std::to_string(node->m_value.m_value.m_int);
        else if (node->m_value.m_type == GM_FLOAT) {
            std::snprintf(valueBuffer, sizeof(valueBuffer), "%f", node->m_value.m_value.m_float);
            value = valueBuffer;
        } else if (node->m_value.m_type == GM_STRING) {
            const char* text = node->m_value.AsString(&machine, valueBuffer, sizeof(valueBuffer));
            if (!text) { a_thread->PushInt(0); return GM_OK; }
            value = text;
        } else { a_thread->PushInt(0); return GM_OK; }
        if (!body.empty()) body += '&';
        body += key;
        body += '=';
        body += value;
        if (body.size() >= 2000) { a_thread->PushInt(0); return GM_OK; }
    }
    const std::string path = "/otg/~identity:" + g_networkClient->UserId() +
        "/~record:" + EncodeRetailUrl("/~level:" + levelId) + ".post";
    const int id = g_networkClient->QueueRetailPost(path, body, "www-form-urlencoded",
        a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_UnpublishLevel(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string levelId;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, levelId)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    const int id = g_networkClient->QueueRetailGet("/otg/~level:" + levelId + ".delete",
        a_thread->Param(0), a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int QueueLevelLookup(gmThread* a_thread, bool a_versions) {
    if (a_thread->GetNumParams() < 4) {
        a_thread->PushInt(-1);
        return GM_OK;
    }
    std::string levelId;
    if (!g_networkClient || !ReadOptionalString(a_thread, 3, levelId)) {
        a_thread->PushInt(0);
        return GM_OK;
    }
    std::string path = "/otg/~level:" + levelId;
    path += a_versions ? "/~version:*.get" : ".latest";
    const int id = g_networkClient->QueueRetailGet(path, a_thread->Param(0),
        a_thread->Param(1), a_thread->Param(2), 0, 0);
    a_thread->PushInt(id != 0 ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_GetSnapshot(gmThread* a_thread) { return QueueLevelLookup(a_thread, false); }
int GM_CDECL gmf_GetLevelVersions(gmThread* a_thread) { return QueueLevelLookup(a_thread, true); }

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
    const std::string path(str);
    const size_t lastColon = path.rfind(':');
    std::string levelId, uploadId;
    if (lastColon != std::string::npos) {
        const size_t previous = lastColon == 0 ? std::string::npos : path.rfind(':', lastColon - 1);
        const size_t start = previous != std::string::npos ? previous + 1 : (lastColon == 0 ? 0 : 1);
        const size_t slash = path.find('/', start);
        levelId = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start).substr(0, 99);
        uploadId = path.substr(lastColon + 1, 99);
    }
    table->Set(&m, "levelId", gmVariable(m.AllocStringObject(levelId.c_str())));
    table->Set(&m, "uploadId", gmVariable(m.AllocStringObject(uploadId.c_str())));
    a_thread->PushTable(table);
    return GM_OK;
}

std::string TruncateKeyComponent(const std::string& a_text) {
    size_t pos = 0;
    while (pos < a_text.size()) {
        const auto c = static_cast<unsigned char>(a_text[pos]);
        const size_t width = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 :
                             c < 0xf8 ? 4 : c < 0xfc ? 5 : 6;
        if (pos + width > 199 || pos + width > a_text.size()) break;
        pos += width;
    }
    return a_text.substr(0, pos);
}

int GM_CDECL gmf_BaseKeyValue(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(str, 0, "");
    GM_TABLE_PARAM(table, 1);
    gmMachine& m = *a_thread->GetMachine();
    const std::string path(str);
    const size_t slash = path.rfind('/');
    const size_t colon = path.rfind(':');
    const size_t start = slash == std::string::npos ? 0 : slash + 1;
    const bool hasValue = colon != std::string::npos && colon >= start;
    const std::string base = slash == std::string::npos ? "" : TruncateKeyComponent(path.substr(0, slash));
    const std::string key = TruncateKeyComponent(path.substr(start, hasValue ? colon - start : std::string::npos));
    const std::string value = hasValue ? TruncateKeyComponent(path.substr(colon + 1)) : "";
    table->Set(&m, "key", gmVariable(m.AllocStringObject(key.c_str())));
    table->Set(&m, "value", gmVariable(m.AllocStringObject(value.c_str())));
    table->Set(&m, "base", gmVariable(m.AllocStringObject(base.c_str())));
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_PackScore(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(tokens, 0, 0);
    GM_INT_PARAM(playTime, 1, 0);
    const int32_t inputTokens = static_cast<int32_t>(tokens);
    int64_t t = inputTokens;
    if (t > 0x7ffe) t = 0x7ffe;
    unsigned long long packed =
        ((unsigned long long)(0x7fff - t) << 32) |
        static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(playTime)));
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
    table->Set(&m, "tokens", gmVariable((gmint)(0x7fff - ((packed >> 32) & 0x7fff))));
    table->Set(&m, "time", gmVariable((gmint)(int32_t)(uint32_t)packed));
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
        const char* v;
        if (n->m_value.m_type == GM_FLOAT) {
            std::snprintf(vbuf, sizeof(vbuf), "%f", n->m_value.m_value.m_float);
            v = vbuf;
        } else {
            v = n->m_value.AsString(&m, vbuf, sizeof(vbuf));
        }
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
        } else {
            table->Set(&m, p, gmVariable(static_cast<gmint>(1)));
        }
        p += seglen + (amp ? 1 : 0);
    }
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_GetUserIdentityId(gmThread* a_thread) {
    a_thread->PushNewString(g_networkClient ? g_networkClient->UserId().c_str() : "");
    return GM_OK;
}

int GM_CDECL gmf_GetEula(gmThread* a_thread) {
    if (g_networkClient && g_networkClient->HasEula())
        a_thread->PushNewString(g_networkClient->EulaText().c_str());
    else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL gmf_GetEulaVersion(gmThread* a_thread) {
    a_thread->PushNewString(g_networkClient ? g_networkClient->EulaVersion().c_str() : "");
    return GM_OK;
}

int GM_CDECL gmf_GetFriends(gmThread* a_thread) {
    if (!g_networkClient || !g_networkClient->FriendsReady()) return GM_OK;
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* friends = machine.AllocTableObject();
    int index = 0;
    for (const auto& entry : g_networkClient->Friends()) {
        gmTableObject* row = machine.AllocTableObject();
        row->Set(&machine, "identity", gmVariable(machine.AllocStringObject(entry.identity.c_str())));
        row->Set(&machine, "displayName", gmVariable(machine.AllocStringObject(entry.displayName.c_str())));
        friends->Set(&machine, index++, gmVariable(row));
    }
    a_thread->PushTable(friends);
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
    a_machine.RegisterLibraryFunction("Watermelon_GetEula", gmf_GetEula);
    a_machine.RegisterLibraryFunction("Watermelon_GetEulaVersion", gmf_GetEulaVersion);
    a_machine.RegisterLibraryFunction("Watermelon_Get_Friends", gmf_GetFriends);
    a_machine.RegisterLibraryFunction("Watermelon_IsOnline", gmf_IsOnline);
    a_machine.RegisterLibraryFunction("Watermelon_Purge", gmf_Purge);
    a_machine.RegisterLibraryFunction("Watermelon_ForceCachePurgeByFilter", gmf_NoOp);
    a_machine.RegisterLibraryFunction("WatermelonGlobals_GetFeaturedSearches", gmf_Globals_GetFeaturedSearches);
    a_machine.RegisterLibraryFunction("WatermelonGlobals_GetNews", gmf_Globals_GetNews);
    a_machine.RegisterLibraryFunction("Watermelon_Index_Page", gmf_IndexPage);
    a_machine.RegisterLibraryFunction("Watermelon_Metadata_All", gmf_MetadataAll);
    a_machine.RegisterLibraryFunction("Watermelon_Metadata_Get", gmf_MetadataGet);
    a_machine.RegisterLibraryFunction("Watermelon_Metadata_Put", gmf_MetadataPut);
    a_machine.RegisterLibraryFunction("Watermelon_Rating_Get", gmf_RatingGet);
    a_machine.RegisterLibraryFunction("Watermelon_Rating_Put", gmf_RatingPut);
    a_machine.RegisterLibraryFunction("Watermelon_Favourite_All", gmf_FavouriteAll);
    a_machine.RegisterLibraryFunction("Watermelon_Favourite_Put", gmf_FavouritePut);
    a_machine.RegisterLibraryFunction("Watermelon_Favourite_Delete", gmf_FavouriteDelete);
    a_machine.RegisterLibraryFunction("Watermelon_Leaderboard_Page", gmf_LeaderboardPage);
    a_machine.RegisterLibraryFunction("Watermelon_Leaderboard_Post", gmf_LeaderboardPost);
    a_machine.RegisterLibraryFunction("Watermelon_Leaderboard_Get", gmf_LeaderboardGet);
    a_machine.RegisterLibraryFunction("Watermelon_Leaderboard_All", gmf_LeaderboardAll);
    a_machine.RegisterLibraryFunction("Watermelon_Leaderboard_Near", gmf_LeaderboardNear);
    a_machine.RegisterLibraryFunction("Watermelon_Albums_Page", gmf_AlbumsPage);
    a_machine.RegisterLibraryFunction("Watermelon_Album_Page", gmf_AlbumPage);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Page", gmf_FollowingPage);
    a_machine.RegisterLibraryFunction("Watermelon_Following_All", gmf_FollowingAll);
    a_machine.RegisterLibraryFunction("Watermelon_Followers_Page", gmf_FollowersPage);
    a_machine.RegisterLibraryFunction("Watermelon_LevelHearts_Page", gmf_LevelHeartsPage);
    a_machine.RegisterLibraryFunction("Watermelon_LevelHearts_Get", gmf_LevelHeartsGet);
    a_machine.RegisterLibraryFunction("Watermelon_LevelHearts_Post", gmf_LevelHeartsPost);
    a_machine.RegisterLibraryFunction("Watermelon_LevelHearts_Delete", gmf_LevelHeartsDelete);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Get", gmf_FollowingGet);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Put", gmf_FollowingPut);
    a_machine.RegisterLibraryFunction("Watermelon_Following_Delete", gmf_FollowingDelete);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Page", gmf_QueuedPage);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Put", gmf_QueuedPut);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Get", gmf_QueuedGet);
    a_machine.RegisterLibraryFunction("Watermelon_Queued_Delete", gmf_QueuedDelete);
    a_machine.RegisterLibraryFunction("Watermelon_ActivityStream_Page", gmf_ActivityStreamPage);
    a_machine.RegisterLibraryFunction("Watermelon_ActivityStream_Post", gmf_ActivityStreamPost);
    a_machine.RegisterLibraryFunction("Watermelon_FeaturedLevel_Get", gmf_FeaturedLevelGet);
    a_machine.RegisterLibraryFunction("Watermelon_FeaturedLevel_Post", gmf_FeaturedLevelPost);
    a_machine.RegisterLibraryFunction("Watermelon_Record_Post", gmf_RecordPost);
    a_machine.RegisterLibraryFunction("Watermelon_UnpublishLevel", gmf_UnpublishLevel);
    a_machine.RegisterLibraryFunction("Watermelon_GetSnapshot", gmf_GetSnapshot);
    a_machine.RegisterLibraryFunction("Watermelon_GetLevelVersions", gmf_GetLevelVersions);
    // Local test hook. Not part of the retail GameMonkey API.
    a_machine.RegisterLibraryFunction("OpenEgg_NetworkGet", gmf_OpenEggNetworkGet);
    a_machine.RegisterLibraryFunction("OpenEgg_NetworkPost", gmf_OpenEggNetworkPost);

    // Server-request family -- see the offline-stub block comment above.
    static const char* kOfflineRequests[] = {
        "Watermelon_PublishLevel",
        "Watermelon_GetLevelDescription",
        "Watermelon_GetLevelThumbnail",
        "Watermelon_GetLevelData",
        "Watermelon_SaveLevelAndAssetsToDisk",
        "Watermelon_GetSoundData",
        "Watermelon_Grief_Post",
        "Watermelon_GetContentImage",
        "Watermelon_GetBetaParticipationAgreement",
        "Watermelon_GetCloudSave",
        "Watermelon_PostCloudSave",
    };
    for (const char* name : kOfflineRequests)
        a_machine.RegisterLibraryFunction(name, gmf_OfflineRequest);
}
