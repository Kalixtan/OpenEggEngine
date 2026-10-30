#include "network_client.h"

#include "gmCall.h"
#include "gmFunctionObject.h"
#include "gmMachine.h"
#include "gmStringObject.h"
#include "gmTableObject.h"
#include "system_config.h"
#include "sha1.h"
#include "network_translation.h"
#include "network_level_data.h"
#include "save_data.h"
#include "engine_context.h"
#include "../bindings/texture_bindings.h"
#include "../bindings/audio_bindings.h"
#include <rapidjson/document.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include "stb_image.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <string_view>
#include <utility>

#if defined(OPENEGG_HAS_CURL)
#include <curl/curl.h>
#endif

namespace {
constexpr const char* kRootTableName = "__OpenEggNetworkRequestRoots";

bool CachePatternMatches(std::string_view pattern, std::string_view key) {
    while (!pattern.empty() && !key.empty() && pattern.front() == key.front()) {
        pattern.remove_prefix(1);
        key.remove_prefix(1);
    }
    if (pattern.empty()) return key.empty();
    if (pattern.front() != '#') return false;
    pattern.remove_prefix(1);
    const bool crossesSlash = !pattern.empty() && pattern.front() == '#';
    if (crossesSlash) pattern.remove_prefix(1);
    // Retail accepts the remaining input immediately for a trailing wildcard.
    if (pattern.empty()) return true;
    while (!key.empty()) {
        if (CachePatternMatches(pattern, key)) return true;
        if (!crossesSlash && key.front() == '/') return false;
        key.remove_prefix(1);
    }
    return false;
}

bool SameOnlineId(const std::string& first, const std::string& second) {
    if (first.size() != second.size()) return false;
    for (size_t i = 0; i < first.size(); ++i) {
        unsigned char a = static_cast<unsigned char>(first[i]);
        unsigned char b = static_cast<unsigned char>(second[i]);
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}

std::string ContentCacheKey(const std::string& path, bool post, bool identity,
                            const std::string& resourceKey) {
    if (post) return {};
    std::string key;
    if (identity) {
        if (path.rfind("/otg/", 0) != 0 || path.size() < 9 ||
            path.compare(path.size() - 4, 4, ".get") != 0) return {};
        key = path.substr(5, path.size() - 9);
    } else {
        // The global image builder stores the supplied key, not its base URL.
        key = resourceKey;
    }
    for (const char* pattern : {"##/~content:image/data", "##/~content:level/data", "##/~content:sound/data"})
        if (CachePatternMatches(pattern, key)) return key;
    return {};
}

// Desktop ticket files stand in for the PSN online-ID query. This reads only
// the common identity fields from PS3/Vita ticket versions 2.1 and 3.0.
std::string TicketOnlineId(const std::string& ticket, std::string& country) {
    country.clear();
    if (ticket.size() < 12) return {};
    const auto* bytes = reinterpret_cast<const unsigned char*>(ticket.data());
    const bool version21 = (bytes[0] >> 4) == 2 && bytes[1] == 1;
    const bool version30 = (bytes[0] >> 4) == 3 && bytes[1] == 0;
    if (!version21 && !version30) return {};
    auto read16 = [&](size_t offset) { return (unsigned(bytes[offset]) << 8) | bytes[offset + 1]; };
    if (read16(6) != ticket.size() - 8 || bytes[8] != 0x30 || bytes[9] != 0) return {};
    const size_t bodyEnd = 12 + read16(10);
    if (bodyEnd > ticket.size()) return {};
    size_t pos = 12;
    const unsigned types[] = {8, 1, 7, 7, 2, 4};
    const unsigned fixedSizes[] = {0, 4, 8, 8, 8, 0};
    for (size_t i = 0; i < 6; ++i) {
        if (pos > bodyEnd || bodyEnd - pos < 4) return {};
        const unsigned type = read16(pos);
        const size_t length = read16(pos + 2);
        pos += 4;
        if (type != types[i] || length > bodyEnd - pos || (fixedSizes[i] && length != fixedSizes[i])) return {};
        if (i == 5) {
            std::string id = ticket.substr(pos, std::min<size_t>(length, 16));
            const size_t zero = id.find('\0');
            if (zero != std::string::npos) id.resize(zero);
            pos += length;
            if (bodyEnd - pos >= 4 && read16(pos) == 8) {
                const size_t countryLength = read16(pos + 2);
                pos += 4;
                if (countryLength <= bodyEnd - pos) {
                    country = ticket.substr(pos, countryLength);
                    const size_t terminator = country.find('\0');
                    if (terminator != std::string::npos) country.resize(terminator);
                }
            }
            return id;
        }
        pos += length;
    }
    return {};
}

bool IsLoopbackUrl(const std::string& url) {
    std::string normalized = url;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    const size_t start = normalized.rfind("http://", 0) == 0 ? 7 :
        normalized.rfind("https://", 0) == 0 ? 8 : std::string::npos;
    if (start == std::string::npos) return false;
    const size_t end = normalized.find_first_of("/?#", start);
    const std::string authority = normalized.substr(start,
        end == std::string::npos ? std::string::npos : end - start);
    if (authority.empty() || authority.find('@') != std::string::npos) return false;

    const auto isHost = [&authority](const std::string& host) {
        if (authority == host) return true;
        if (authority.size() <= host.size() + 1 || authority.compare(0, host.size(), host) != 0 ||
            authority[host.size()] != ':') return false;
        return std::all_of(authority.begin() + static_cast<std::ptrdiff_t>(host.size() + 1),
            authority.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
    };
    return isHost("localhost") || isHost("127.0.0.1") || isHost("[::1]");
}

bool AllowsTicketTransport(const std::string& url) {
    std::string normalized = url;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return normalized.rfind("https://", 0) == 0 || IsLoopbackUrl(normalized);
}

class PartialJsonHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, PartialJsonHandler> {
public:
    PartialJsonHandler(gmMachine& machine, int mode)
        : m_machine(machine), m_mode(mode), m_root(machine.AllocTableObject()) {}

    gmVariable Result() const { return gmVariable(m_root); }
    bool Null() { return Add(gmVariable::s_null); }
    bool Bool(bool value) { return Add(gmVariable(static_cast<gmint>(value))); }
    bool Double(double value) { return Add(gmVariable(static_cast<float>(value))); }
    bool String(const char* value, rapidjson::SizeType length, bool) {
        return Add(gmVariable(m_machine.AllocStringObject(value, static_cast<int>(length))));
    }
    bool RawNumber(const char* value, rapidjson::SizeType length, bool) {
        const std::string_view token(value, length);
        if (token.find_first_of(".eE") == std::string_view::npos) {
            int64_t integer = 0;
            const auto parsed = std::from_chars(token.data(), token.data() + token.size(), integer);
            if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) return false;
            if (m_mode == 2 && integer >= 0x100000000LL)
                return Add(gmVariable(m_machine.AllocStringObject(std::to_string(integer).c_str())));
            return Add(gmVariable(static_cast<gmint>(integer)));
        }

        const std::string number(token);
        char* end = nullptr;
        errno = 0;
        const double parsed = std::strtod(number.c_str(), &end);
        const bool rangeOverflow = errno == ERANGE &&
            (parsed == std::numeric_limits<double>::max() ||
             parsed == std::numeric_limits<double>::lowest());
        if (end != number.c_str() + number.size() || rangeOverflow || !std::isfinite(parsed)) return false;
        return Double(parsed);
    }
    bool Key(const char* value, rapidjson::SizeType length, bool) {
        if (m_stack.empty()) return false;
        m_stack.back().key.assign(value, length);
        return true;
    }
    bool StartObject() { return Start(false); }
    bool StartArray() { return Start(true); }
    bool EndObject(rapidjson::SizeType) { return End(); }
    bool EndArray(rapidjson::SizeType) { return End(); }

private:
    struct Frame {
        gmTableObject* table;
        bool array;
        int index = 0;
        std::string key;
    };
    bool Add(const gmVariable& value) {
        if (m_stack.empty()) return false;
        Frame& frame = m_stack.back();
        if (frame.array) frame.table->Set(&m_machine, frame.index++, value);
        else frame.table->Set(&m_machine, frame.key.c_str(), value);
        return true;
    }
    bool Start(bool array) {
        gmTableObject* table = m_root;
        if (!m_stack.empty()) {
            table = m_machine.AllocTableObject();
            if (!Add(gmVariable(table))) return false;
        }
        m_stack.push_back({table, array, 0, {}});
        return true;
    }
    bool End() {
        if (m_stack.empty()) return false;
        m_stack.pop_back();
        return true;
    }
    gmMachine& m_machine;
    int m_mode;
    gmTableObject* m_root;
    std::vector<Frame> m_stack;
};

gmVariable JsonToGm(gmMachine& a_machine, const rapidjson::Value& a_value, int a_mode) {
    if (a_value.IsNull()) return gmVariable::s_null;
    if (a_value.IsBool()) return gmVariable(static_cast<gmint>(a_value.GetBool()));
    if (a_value.IsInt64()) {
        const int64_t value = a_value.GetInt64();
        if (a_mode == 2 && value >= 0x100000000LL)
            return gmVariable(a_machine.AllocStringObject(std::to_string(value).c_str()));
        return gmVariable(static_cast<gmint>(value));
    }
    if (a_value.IsUint64()) {
        const uint64_t value = a_value.GetUint64();
        if (a_mode == 2 && value >= 0x100000000ULL)
            return gmVariable(a_machine.AllocStringObject(std::to_string(value).c_str()));
        return gmVariable(static_cast<gmint>(value));
    }
    if (a_value.IsNumber()) return gmVariable(static_cast<float>(a_value.GetDouble()));
    if (a_value.IsString())
        return gmVariable(a_machine.AllocStringObject(a_value.GetString(),
                                                       static_cast<int>(a_value.GetStringLength())));

    gmTableObject* table = a_machine.AllocTableObject();
    if (a_value.IsArray()) {
        int index = 0;
        for (const rapidjson::Value& element : a_value.GetArray())
            table->Set(&a_machine, index++, JsonToGm(a_machine, element, a_mode));
    } else if (a_value.IsObject()) {
        for (auto member = a_value.MemberBegin(); member != a_value.MemberEnd(); ++member) {
            const gmVariable key(a_machine.AllocStringObject(
                member->name.GetString(), static_cast<int>(member->name.GetStringLength())));
            table->Set(&a_machine, key, JsonToGm(a_machine, member->value, a_mode));
        }
    }
    return gmVariable(table);
}

gmVariable ParseRetailJson(gmMachine& a_machine, const std::string& a_body, int a_mode) {
    PartialJsonHandler partial(a_machine, a_mode);
    rapidjson::MemoryStream stream(a_body.data(), a_body.size());
    rapidjson::Reader reader;
    reader.Parse<rapidjson::kParseNumbersAsStringsFlag>(stream, partial);
    // Retail keeps values already written by parser callbacks when later input
    // fails. A root scalar has no table key, leaving this result empty.
    return partial.Result();
}

#if defined(OPENEGG_HAS_CURL)
struct HeaderContext {
    CURL* curl;
    bool rawResponse;
    bool followRedirects;
    bool restrictRedirects;
    bool hasLocation = false;
    bool safeLocation = false;
    bool skippedErrorBody = false;
};

size_t ReceiveHeaders(char* data, size_t size, size_t count, void* context) {
    if (size != 0 && count > std::numeric_limits<size_t>::max() / size) return 0;
    const size_t bytes = size * count;
    auto& headers = *static_cast<HeaderContext*>(context);
    if (bytes >= 5 && std::string_view(data, 5) == "HTTP/") {
        headers.hasLocation = false;
        headers.safeLocation = false;
    }
    if (bytes >= 9 && curl_strnequal(data, "Location:", 9)) {
        std::string_view location(data + 9, bytes - 9);
        while (!location.empty() && (location.front() == ' ' || location.front() == '\t')) location.remove_prefix(1);
        while (!location.empty() && (location.back() == '\r' || location.back() == '\n' ||
                                     location.back() == ' ' || location.back() == '\t')) location.remove_suffix(1);
        // Only root-relative redirects can be followed without risking disclosure
        // of the identity header to another host. Retail's header policy is unknown.
        const bool safeLocation = !location.empty() && location.front() == '/' &&
            (location.size() == 1 || (location[1] != '/' && location[1] != '\\')) &&
            location.find('\\') == std::string_view::npos &&
            std::none_of(location.begin(), location.end(), [](unsigned char c) { return c < 0x20 || c == 0x7f; });
        headers.safeLocation = safeLocation && (!headers.hasLocation || headers.safeLocation);
        headers.hasLocation = true;
    }
    if ((bytes == 2 && data[0] == '\r' && data[1] == '\n') ||
        (bytes == 1 && data[0] == '\n')) {
        long status = 0;
        curl_easy_getinfo(headers.curl, CURLINFO_RESPONSE_CODE, &status);
        const bool redirect = headers.followRedirects && headers.hasLocation &&
            (!headers.restrictRedirects || headers.safeLocation) && status >= 300 && status < 400;
        const bool unsafeRedirect = headers.restrictRedirects && headers.hasLocation &&
            !headers.safeLocation && status >= 300 && status < 400;
        if (unsafeRedirect || (!headers.rawResponse && status >= 200 && status != 200 && status != 201 && !redirect)) {
            headers.skippedErrorBody = true;
            return 0;
        }
    }
    return bytes;
}

size_t ReceiveBody(char* a_data, size_t a_size, size_t a_count, void* a_context) {
    if (a_size != 0 && a_count > std::numeric_limits<size_t>::max() / a_size) return 0;
    const size_t bytes = a_size * a_count;
    auto* body = static_cast<std::string*>(a_context);
    body->append(a_data, bytes);
    return bytes;
}
#endif
} // namespace

int NetworkBucketValue(const gmVariable& value) {
    int32_t word = 0;
    static_assert(sizeof(word) <= sizeof(value.m_value), "GameMonkey payload is too small");
    std::memcpy(&word, &value.m_value, sizeof(word));
    return word;
}

NetworkClient::NetworkClient(gmMachine& a_machine, std::string a_baseUrl,
                             std::string a_sessionId, std::string a_userId)
    : m_machine(a_machine), m_baseUrl(std::move(a_baseUrl)),
      m_sessionId(std::move(a_sessionId)), m_userId(std::move(a_userId)),
      m_savedSessionId(SystemConfig::Get().networkSavedSessionId),
      m_savedUserId(SystemConfig::Get().networkSavedUserId),
      m_manualSessionId(m_sessionId), m_manualUserId(m_userId) {
    while (!m_baseUrl.empty() && m_baseUrl.back() == '/') m_baseUrl.pop_back();
    m_roots = m_machine.AllocTableObject();
    m_machine.GetGlobals()->Set(&m_machine, kRootTableName, gmVariable(m_roots));
    m_featured = m_machine.AllocTableObject();
    m_roots->Set(&m_machine, "featured", gmVariable(m_featured));
    m_news = m_machine.AllocTableObject();
    m_roots->Set(&m_machine, "news", gmVariable(m_news));

#if defined(OPENEGG_HAS_CURL)
    static const CURLcode curlInit = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curlInit == CURLE_OK && !m_baseUrl.empty()) {
        m_worker = std::thread(&NetworkClient::WorkerMain, this, true);
        m_globalWorker = std::thread(&NetworkClient::WorkerMain, this, false);
    }
    if (m_worker.joinable()) m_coordinatorState = 4;
#else
    if (!m_baseUrl.empty())
        std::fprintf(stderr, "network: libcurl unavailable; replacement-server requests disabled\n");
#endif
}

NetworkClient::~NetworkClient() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopping = true;
    }
    m_wake.notify_all();
    if (m_worker.joinable()) m_worker.join();
    if (m_globalWorker.joinable()) m_globalWorker.join();
    m_machine.GetGlobals()->Set(&m_machine, kRootTableName, gmVariable::s_null);
}

int NetworkClient::QueueGet(const std::string& a_path, const gmVariable& a_callback,
                            const gmVariable& a_callbackData) {
    return Queue(a_path, a_callback, a_callbackData, gmVariable::s_null, 0, -1);
}

int NetworkClient::QueueRetailGet(const std::string& a_path, const gmVariable& a_callback,
                                  const gmVariable& a_callbackData, const gmVariable& a_tag,
                                  int a_bucket, int a_callbackMode, const std::string& a_contentType) {
    return Queue(a_path, a_callback, a_callbackData, a_tag, a_bucket, a_callbackMode, false, {}, a_contentType);
}

int NetworkClient::QueueRetailPost(const std::string& a_path, const std::string& a_body,
                                   const std::string& a_contentType,
                                   const gmVariable& a_callback,
                                   const gmVariable& a_callbackData,
                                   const gmVariable& a_tag, int a_bucket,
                                   int a_callbackMode) {
    return Queue(a_path, a_callback, a_callbackData, a_tag, a_bucket, a_callbackMode,
                 true, a_body, a_contentType);
}

int NetworkClient::QueueRetailImage(const std::string& a_path, const std::string& a_cacheKey,
                                   const gmVariable& a_callback, const gmVariable& a_callbackData,
                                   const gmVariable& a_tag, bool a_sendSession) {
    return Queue(a_path, a_callback, a_callbackData, a_tag, 0, 4, false, {}, {}, a_cacheKey, a_sendSession);
}

bool NetworkClient::QueueAssetBundle(const std::string& level, const std::string& version,
                                     const std::string& outputPath, const gmVariable& callback,
                                     const gmVariable& callbackData, const gmVariable& tag) {
    if (!m_identityReady || outputPath.empty() || m_nextAssetBundleId == std::numeric_limits<int>::max()) return false;
    const std::string prefix = "/otg/~level:" + level + "/~version:" + version + "/~content:";
    // Reject setup before dispatch if any of the three paths cannot be queued.
    if (!m_worker.joinable() || m_baseUrl.size() + prefix.size() + sizeof("sound/data.get") - 1 > 1023 ||
        m_nextId > std::numeric_limits<int>::max() - 4) return false;
    const char* types[] = {"level", "sound", "image"};
    std::array<int, 3> requests{};
    for (size_t i = 0; i < requests.size(); ++i) {
        requests[i] = QueueRetailGet(prefix + types[i] + "/data.get", gmVariable::s_null,
                                     gmVariable::s_null, tag, 0, 6);
        if (!requests[i]) {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (int id : requests) {
                auto pending = m_pending.find(id);
                if (pending != m_pending.end()) pending->second.purged = true;
            }
            return false;
        }
    }
    const int group = m_nextAssetBundleId++;
    gmTableObject* context = m_machine.AllocTableObject();
    context->Set(&m_machine, 0, callback);
    context->Set(&m_machine, 1, callbackData);
    m_roots->Set(&m_machine, -group, gmVariable(context));
    m_assetBundles.emplace(group, AssetBundle{outputPath});
    for (size_t i = 0; i < requests.size(); ++i)
        m_assetRequests.emplace(requests[i], std::make_pair(group, static_cast<int>(i)));
    return true;
}

bool NetworkClient::BeginTicketLogin(const std::string& a_ticket) {
    if (a_ticket.empty() || m_loginRequestId != 0) return false;
    if (!AllowsTicketTransport(m_baseUrl)) {
        std::fprintf(stderr, "network: ticket login requires HTTPS for remote servers\n");
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& entry : m_pending)
            if (entry.second.identity) entry.second.purged = true;
        m_sessionId.clear();
    }
    m_userId.clear();
    m_identityReady = false;
    m_desktopPlatformOnline = false;
    m_loginRequestId = QueueRetailPost("/identity/login/token/psn", a_ticket,
        "application/octet-stream", gmVariable::s_null, gmVariable::s_null,
        gmVariable::s_null, 0, 0);
    return m_loginRequestId != 0;
}

void NetworkClient::SetPlatformTicket(std::string a_ticket) {
    std::string country;
    const std::string onlineId = TicketOnlineId(a_ticket, country);
    const std::string& expectedOnlineId = SystemConfig::Get().networkPsnOnlineId;
    if (expectedOnlineId.empty()) {
        std::fprintf(stderr, "network: psn_online_id is required with a ticket\n");
        if (m_worker.joinable()) m_coordinatorState = 4;
        return;
    }
    if (onlineId.empty() || !SameOnlineId(onlineId, expectedOnlineId)) {
        std::fprintf(stderr, "network: ticket Online ID does not match psn_online_id\n");
        if (m_worker.joinable()) m_coordinatorState = 4;
        return;
    }
    m_platformOnlineId = onlineId;
    m_platformCountry = std::move(country);
    m_platformTicket = std::move(a_ticket);
    if (!m_platformTicket.empty() && m_worker.joinable()) m_coordinatorState = 4;
}

void NetworkClient::GoOnline() {
    if (m_coordinatorState != 2 && m_coordinatorState != 4) return;
    if (m_offlineTransitionPending) {
        m_reconnectAfterOfflineTransition = true;
        return;
    }
    m_managerAttached = true;
    m_chatPermissions = SystemConfig::Get().networkCommunityAllowed;
    m_coordinatorError = 0;
    if (!m_savedSessionId.empty() && !m_savedUserId.empty() &&
        !SystemConfig::Get().networkPsnOnlineId.empty()) {
        if (!AllowsTicketTransport(m_baseUrl)) {
            m_coordinatorError = -12;
            std::fprintf(stderr, "network: saved server sessions require HTTPS for remote servers\n");
            return;
        }
        m_platformOnlineId = SystemConfig::Get().networkPsnOnlineId;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& entry : m_pending)
                if (entry.second.identity) entry.second.purged = true;
            m_sessionId = m_savedSessionId;
        }
        m_userId = m_savedUserId;
        m_identityReady = false;
        m_savedValidationRequestId = QueueSavedSessionValidation();
        if (m_savedValidationRequestId) m_coordinatorState = 18;
        else m_coordinatorError = -12;
    } else if (!m_platformTicket.empty()) {
        if (BeginTicketLogin(m_platformTicket)) m_coordinatorState = 18;
        else m_coordinatorError = -12;
    } else if (m_platformTicket.empty() && !m_manualSessionId.empty() && !m_manualUserId.empty()) {
        if (!IsLoopbackUrl(m_baseUrl)) {
            m_coordinatorError = -12;
            std::fprintf(stderr, "network: manual sessions are restricted to loopback servers\n");
        } else {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (auto& entry : m_pending)
                    if (entry.second.identity) entry.second.purged = true;
                m_sessionId = m_manualSessionId;
            }
            m_userId = m_manualUserId;
            m_identityReady = false;
            m_manualValidationRequestId = QueueRetailGet("/otg/~identity:*.hello?identity=" + m_userId,
                gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
            if (m_manualValidationRequestId) m_coordinatorState = 18;
            else m_coordinatorError = -12;
        }
    } else {
        m_coordinatorError = -12;
        std::fprintf(stderr, "network: a PSN ticket is required for online login\n");
    }
}

bool NetworkClient::IsSendQueueFull() {
    if (!m_managerAttached) return true;
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_identityRequests.size() == 32;
}

const std::string* NetworkClient::FindNetworkTranslation(const std::string& key) const {
    const auto entry = m_networkTranslations.find(key);
    return entry == m_networkTranslations.end() ? nullptr : &entry->second;
}

void NetworkClient::GoOffline() {
    if (m_coordinatorState == 1) return;
    const bool managerAttached = m_managerAttached;
    const bool stopWorker = managerAttached && m_worker.joinable();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& entry : m_pending)
            if (entry.second.identity) entry.second.purged = true;
        m_sessionId.clear();
        m_offlineTransitionPending = stopWorker;
        m_identityWorkerStopRequested = stopWorker;
        m_identityWorkerStopped = false;
        m_reconnectAfterOfflineTransition = false;
    }
    m_loginRequestId = 0;
    m_manualValidationRequestId = 0;
    m_savedValidationRequestId = 0;
    m_helloRequestId = 0;
    m_eulaRequestId = 0;
    m_friendsFetchRequestId = 0;
    m_featuredRequestId = 0;
    m_newsRequestId = 0;
    m_translationRequestId = 0;
    m_userId.clear();
    m_identityReady = false;
    m_coordinatorError = 0;
    if (stopWorker) m_wake.notify_all();
    // State 2 remains until the identity worker stops and is restarted.
    m_coordinatorState = stopWorker || !managerAttached ? 2 : 4;
}

int NetworkClient::Queue(const std::string& a_path, const gmVariable& a_callback,
                         const gmVariable& a_callbackData, const gmVariable& a_tag,
                         int a_bucket, int a_callbackMode, bool a_post,
                         const std::string& a_body, const std::string& a_contentType,
                         const std::string& a_resourceKey, bool a_sendSession,
                         bool a_validateSavedSession) {
    const bool absolute = a_callbackMode == 4 &&
        (a_path.rfind("http://", 0) == 0 || a_path.rfind("https://", 0) == 0);
    if (m_baseUrl.empty() || !(a_sendSession ? m_worker.joinable() : m_globalWorker.joinable()) || a_path.empty() ||
        (!absolute && a_path.front() != '/') || a_path.size() > 1023 ||
        (!absolute && m_baseUrl.size() + a_path.size() > 1023) ||
        (a_callbackMode < 0 && a_callback.m_type != GM_FUNCTION) || m_roots == nullptr) {
        return 0;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    if (a_sendSession && (m_offlineTransitionPending || m_identityWorkerStopRequested)) return 0;
    const std::string cacheKey = ContentCacheKey(a_path, a_post, a_sendSession, a_resourceKey);
    const Cache& cache = a_sendSession ? m_identityCache : m_globalCache;
    const auto cached = cache.find(cacheKey);
    const bool cacheHit = !cacheKey.empty() && cached != cache.end();
    // Retail resolves cache hits before trying the 32-slot worker ring.
    auto& requests = a_sendSession ? m_identityRequests : m_globalRequests;
    if (!cacheHit)
        m_wake.wait(lock, [&] { return m_stopping || requests.size() < 32; });
    if (m_stopping || m_nextId <= 0 || m_nextId == std::numeric_limits<int>::max())
        return 0;

    const int id = m_nextId++;
    gmTableObject* context = m_machine.AllocTableObject();
    context->Set(&m_machine, 0, a_callback);
    context->Set(&m_machine, 1, a_callbackData);
    context->Set(&m_machine, 2, gmVariable(static_cast<gmint>(a_callbackMode)));
    context->Set(&m_machine, 3, gmVariable(static_cast<gmint>(a_bucket)));
    context->Set(&m_machine, 4, a_tag);
    if (a_callbackMode == 4)
        context->Set(&m_machine, 5, gmVariable(m_machine.AllocStringObject(a_resourceKey.c_str())));
    m_roots->Set(&m_machine, id, gmVariable(context));
    const int bucket = a_callbackMode >= 0 ? NetworkBucketValue(a_tag) : a_bucket;
    m_pending.emplace(id, PendingRequest{bucket, false, a_sendSession});
    if (cacheHit) {
        auto& cachedCompletions = a_sendSession ? m_identityCacheCompletions : m_globalCacheCompletions;
        cachedCompletions.push_back({id, cached->second.status, cached->second.body,
                                     cacheKey, a_sendSession, true});
    } else {
        requests.push_back({id, absolute ? a_path : m_baseUrl + a_path, a_body, a_contentType,
                            a_post, bucket, false, a_sendSession, cacheKey, a_callbackMode < 0,
                            a_validateSavedSession});
        m_wake.notify_all();
    }
    return id;
}

int NetworkClient::QueueSavedSessionValidation() {
    const std::string path = "/otg/~identity:*.hello?identity=" + m_savedUserId +
        "&online_id=" + SystemConfig::Get().networkPsnOnlineId;
    return Queue(path, gmVariable::s_null, gmVariable::s_null, gmVariable::s_null,
                 0, 0, false, {}, {}, {}, true, true);
}

void NetworkClient::Purge(int a_bucket) {
    if (a_bucket == 0) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& entry : m_pending) {
        if (entry.second.bucket == a_bucket) entry.second.purged = true;
    }
}

void NetworkClient::PurgeCache(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_identityCache.begin(); it != m_identityCache.end();) {
        if (CachePatternMatches(pattern, it->first)) it = m_identityCache.erase(it);
        else ++it;
    }
}

void NetworkClient::StoreCache(const Completion& completion) {
    if (completion.cacheKey.empty() || completion.cacheHit || completion.status == 0) return;
    Cache& cache = completion.identity ? m_identityCache : m_globalCache;
    const std::string body = (completion.status == 200 || completion.status == 201)
        ? completion.body : std::string();
    constexpr size_t kPoolSize = 1024 * 1024;
    auto bytesUsed = [&] {
        size_t bytes = 0;
        for (const auto* entries : {&m_identityCache, &m_globalCache})
            for (const auto& entry : *entries) bytes += entry.second.body.size();
        return bytes;
    };
    auto evictOldest = [&] {
        auto oldest = cache.begin();
        if (oldest == cache.end() || oldest->second.body.empty()) return false;
        for (auto it = std::next(oldest); it != cache.end(); ++it) {
            if (!it->second.body.empty() && it->second.inserted < oldest->second.inserted)
                oldest = it;
        }
        cache.erase(oldest);
        return true;
    };
    // Retail trims for the entry limit before looking up the key, then frees
    // an existing body before attempting to allocate its replacement.
    while (cache.size() >= 300) {
        if (!evictOldest()) return;
    }
    auto existing = cache.find(completion.cacheKey);
    if (existing != cache.end()) existing->second.body.clear();
    while (bytesUsed() + body.size() > kPoolSize) {
        if (!evictOldest()) {
            cache.erase(completion.cacheKey);
            return;
        }
    }
    cache[completion.cacheKey] = {completion.status, body,
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count()};
}

void NetworkClient::PumpCallbacks() {
    std::deque<Completion> completions;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // The retail owner shares four completion slots across its managers.
        int budget = 4;
        for (bool identity : {true, false}) {
            auto& queue = identity ? m_identityCompletions : m_globalCompletions;
            auto& cached = identity ? m_identityCacheCompletions : m_globalCacheCompletions;
            while (budget > 0 && (!queue.empty() || !cached.empty())) {
                auto& source = queue.empty() ? cached : queue;
                completions.push_back(std::move(source.front()));
                source.pop_front();
                --budget;
            }
        }
    }

    for (Completion& completion : completions) {
        const long responseStatus = completion.status;
        const bool transportFailed = completion.transportFailed;
        if (transportFailed) {
            completion.status = 0;
            completion.body.clear();
        }
        bool purged = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto pending = m_pending.find(completion.id);
            if (pending != m_pending.end()) {
                if (pending->second.purged) {
                    purged = true;
                    completion.status = 0;
                    completion.body.clear();
                }
                StoreCache(completion);
                m_pending.erase(pending);
            }
        }
        const auto assetRequest = m_assetRequests.find(completion.id);
        if (assetRequest != m_assetRequests.end()) {
            const int groupId = assetRequest->second.first;
            const int part = assetRequest->second.second;
            m_assetRequests.erase(assetRequest);
            auto group = m_assetBundles.find(groupId);
            if (group != m_assetBundles.end()) {
                auto& bundle = group->second;
                if (completion.status == 200 || completion.status == 201) {
                    static const char* suffixes[] = {".json.z", ".jef", ".png"};
                    bundle.sizes[part] = static_cast<int>(std::min<size_t>(completion.body.size(), INT32_MAX));
                    SaveData::RegisterTransient(bundle.path + suffixes[part], completion.body);
                }
                if (--bundle.remaining == 0) {
                    const gmVariable contextValue = m_roots->Get(-groupId);
                    if (contextValue.m_type == GM_TABLE) {
                        auto* context = static_cast<gmTableObject*>(m_machine.GetObject(contextValue.m_value.m_ref));
                        const gmVariable callback = context->Get(0);
                        if (callback.m_type == GM_FUNCTION) {
                            auto* function = static_cast<gmFunctionObject*>(m_machine.GetObject(callback.m_value.m_ref));
                            gmCall call;
                            if (call.BeginFunction(&m_machine, function)) {
                                gmTableObject* result = m_machine.AllocTableObject();
                                result->Set(&m_machine, "levelSize", gmVariable(static_cast<gmint>(bundle.sizes[0])));
                                result->Set(&m_machine, "audioSize", gmVariable(static_cast<gmint>(bundle.sizes[1])));
                                result->Set(&m_machine, "screenshotSize", gmVariable(static_cast<gmint>(bundle.sizes[2])));
                                call.AddParam(context->Get(1));
                                call.AddParam(gmVariable(result));
                                call.AddParamInt(bundle.sizes[0] > 0 && bundle.sizes[1] > 0 && bundle.sizes[2] > 0 ? 0 : -1);
                                call.End();
                            }
                        }
                    }
                    m_roots->Set(&m_machine, -groupId, gmVariable::s_null);
                    m_assetBundles.erase(groupId);
                }
            }
        }
        if (completion.id == m_manualValidationRequestId) {
            m_manualValidationRequestId = 0;
            if (completion.status == 200 || completion.status == 201) {
                m_identityReady = true;
                m_desktopPlatformOnline = true;
                m_helloRequestId = QueueRetailGet("/otg/~identity:*.hello",
                    gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
                if (m_helloRequestId) m_coordinatorState = 20;
                else m_coordinatorError = -12;
            } else {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_sessionId.clear();
                }
                m_userId.clear();
                m_coordinatorError = -12;
                std::fprintf(stderr, "network: configured session validation failed (HTTP %ld)\n", completion.status);
            }
        }
        if (completion.id == m_savedValidationRequestId) {
            m_savedValidationRequestId = 0;
            if (completion.status == 200 || completion.status == 201) {
                m_identityReady = true;
                m_desktopPlatformOnline = true;
                std::fprintf(stderr, "network: remembered server session validated\n");
                m_helloRequestId = QueueRetailGet("/otg/~identity:*.hello",
                    gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
                if (m_helloRequestId) m_coordinatorState = 20;
                else m_coordinatorError = -12;
            } else {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_sessionId.clear();
                }
                m_userId.clear();
                m_identityReady = false;
                m_desktopPlatformOnline = false;
                if (completion.status == 401 || completion.status == 403) {
                    SystemConfig::Get().SaveReplacementSession({}, {});
                    m_savedSessionId.clear();
                    m_savedUserId.clear();
                }
                if (!m_platformTicket.empty()) {
                    if (BeginTicketLogin(m_platformTicket)) {
                        m_coordinatorState = 18;
                        std::fprintf(stderr, "network: remembered session failed; trying configured ticket\n");
                    } else {
                        m_coordinatorError = -12;
                    }
                } else {
                    m_coordinatorError = -12;
                    m_coordinatorState = 4;
                    std::fprintf(stderr, "network: remembered server session validation failed (HTTP %ld)\n",
                                 completion.status);
                }
            }
        }
        if (completion.id == m_loginRequestId) {
            m_loginRequestId = 0;
            rapidjson::Document response;
            response.Parse(completion.body.data(), completion.body.size());
            if (!response.HasParseError() && response.IsObject() &&
                response.HasMember("openegg_authorized") &&
                (!response["openegg_authorized"].IsBool() || !response["openegg_authorized"].GetBool())) {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_sessionId.clear();
                }
                m_userId.clear();
                m_identityReady = false;
                m_desktopPlatformOnline = false;
                m_coordinatorError = -12;
                m_coordinatorState = 4;
                SystemConfig::Get().SaveReplacementSession({}, {});
                m_savedSessionId.clear();
                m_savedUserId.clear();
                std::fprintf(stderr, "network: replacement server rejected this PSN ticket\n");
                continue;
            }
            if ((completion.status == 200 || completion.status == 201) &&
                !response.HasParseError() && response.IsObject() &&
                response.HasMember("session") && response["session"].IsObject()) {
                const auto& session = response["session"];
                if (session.HasMember("id") && session["id"].IsString() &&
                    session.HasMember("person") && session["person"].IsObject() &&
                    session["person"].HasMember("id") && session["person"]["id"].IsString()) {
                    const auto& id = session["id"];
                    const auto& user = session["person"]["id"];
                    const auto& person = session["person"];
                    const bool hasDisplayName = person.HasMember("display_name") &&
                        person["display_name"].IsString();
                    const std::string displayName = hasDisplayName
                        ? person["display_name"].GetString() : std::string();
                    const bool hasPsnOnlineId = person.HasMember("openegg_psn_online_id") &&
                        person["openegg_psn_online_id"].IsString();
                    const std::string accountOnlineId = hasPsnOnlineId
                        ? person["openegg_psn_online_id"].GetString() : displayName;
                    const auto& expectedOnlineId = SystemConfig::Get().networkPsnOnlineId;
                    if (expectedOnlineId.empty() || !hasDisplayName || accountOnlineId.empty() ||
                        !SameOnlineId(accountOnlineId, expectedOnlineId)) {
                        m_coordinatorError = -12;
                        m_coordinatorState = 4;
                        std::fprintf(stderr, "network: server identity does not match psn_online_id\n");
                    } else {
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            for (auto& entry : m_pending)
                                if (entry.second.identity) entry.second.purged = true;
                            m_sessionId.assign(id.GetString(), std::min<size_t>(id.GetStringLength(), 36));
                        }
                        m_userId.assign(user.GetString(), std::min<size_t>(user.GetStringLength(), 36));
                        if (hasPsnOnlineId || hasDisplayName) m_platformOnlineId = accountOnlineId;
                        if (response.HasMember("openegg_remembered_session") &&
                            response["openegg_remembered_session"].IsBool() &&
                            response["openegg_remembered_session"].GetBool()) {
                            if (SystemConfig::Get().SaveReplacementSession(m_sessionId, m_userId)) {
                                m_savedSessionId = m_sessionId;
                                m_savedUserId = m_userId;
                                std::fprintf(stderr, "network: replacement-server session saved to OpenEgg.ini\n");
                            } else {
                                std::fprintf(stderr, "network: could not save replacement-server session to OpenEgg.ini\n");
                            }
                        }
                        m_identityReady = true;
                        m_desktopPlatformOnline = true;
                        std::fprintf(stderr, "network: ticket login succeeded\n");
                        m_helloRequestId = QueueRetailGet("/otg/~identity:*.hello",
                            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
                        if (m_helloRequestId) {
                            m_coordinatorState = 20;
                        } else {
                            m_coordinatorError = -12;
                        }
                    }
                } else {
                    m_coordinatorError = -12;
                    m_coordinatorState = 4;
                    std::fprintf(stderr, "network: ticket login returned an incomplete session\n");
                }
            } else {
                m_coordinatorError = -12;
                m_coordinatorState = 4;
                std::fprintf(stderr, "network: ticket login failed (HTTP %ld)\n", completion.status);
            }
        }
        if (completion.id == m_helloRequestId) {
            m_helloRequestId = 0;
            if (completion.status == 200 || completion.status == 201) {
                std::fprintf(stderr, "network: identity hello succeeded\n");
                const auto& config = SystemConfig::Get();
                const std::string eulaPath = std::string("/otg/ps3/") + config.networkProductRegion + "/" +
                    config.NetworkLanguage() + "/~eula.get";
                m_eulaRequestId = QueueRetailGet(eulaPath, gmVariable::s_null, gmVariable::s_null,
                                                 gmVariable::s_null, 0, 1);
                if (m_eulaRequestId) m_coordinatorState = 22;
                else m_coordinatorError = -12;
            } else {
                m_coordinatorError = -12;
                std::fprintf(stderr, "network: identity hello failed (HTTP %ld)\n", completion.status);
            }
        }
        if (completion.id == m_eulaRequestId) {
            m_eulaRequestId = 0;
            if (completion.status == 200 || completion.status == 201) {
                std::string text = completion.body;
                if (text.size() >= 3 && text.compare(0, 3, "\xef\xbb\xbf") == 0) text.erase(0, 3);
                for (size_t pos = 0; pos < text.size() && text[pos] != '\0';) {
                    const unsigned char ch = static_cast<unsigned char>(text[pos]);
                    if (ch == '\t') text[pos] = ' ';
                    const size_t width = ch < 0x80 ? 1 : ch < 0xe0 ? 2 : ch < 0xf0 ? 3 :
                                         ch < 0xf8 ? 4 : ch < 0xfc ? 5 : 6;
                    pos += std::min(width, text.size() - pos);
                }
                m_eulaVersion = Sha1Hex(text);
                m_eulaText = std::move(text);
                m_eulaText.push_back('\n');
                m_eulaReady = true;
                std::fprintf(stderr, "network: EULA received\n");
            } else {
                m_coordinatorError = -12;
                std::fprintf(stderr, "network: EULA failed (HTTP %ld)\n", completion.status);
            }
        }
        if (completion.id == m_friendsFetchRequestId) {
            m_friendsFetchRequestId = 0;
            m_friends.clear();
            if (completion.status == 200 || completion.status == 201) {
                rapidjson::Document response;
                response.Parse(completion.body.data(), completion.body.size());
                if (!response.HasParseError() && response.IsObject() && response.HasMember("friends") &&
                    response["friends"].IsArray()) {
                    for (const auto& row : response["friends"].GetArray()) {
                        if (!row.IsObject()) continue;
                        Friend entry;
                        if (row.HasMember("id") && row["id"].IsString())
                            entry.identity.assign(row["id"].GetString(), row["id"].GetStringLength());
                        if (row.HasMember("displayName") && row["displayName"].IsString())
                            entry.displayName.assign(row["displayName"].GetString(), row["displayName"].GetStringLength());
                        m_friends.push_back(std::move(entry));
                    }
                }
                m_friendsReady = true;
                std::fprintf(stderr, "network: friends synchronized\n");
            } else {
                m_coordinatorError = -12;
                std::fprintf(stderr, "network: friends fetch failed (HTTP %ld)\n", completion.status);
            }
        }
        if (completion.id == m_featuredRequestId || completion.id == m_newsRequestId) {
            const bool featured = completion.id == m_featuredRequestId;
            if (featured) { m_featuredRequestId = 0; m_featuredReady = true; }
            else { m_newsRequestId = 0; m_newsReady = true; }
            if (completion.status == 200 || completion.status == 201) {
                rapidjson::Document response;
                response.Parse(completion.body.data(), completion.body.size());
                if (!response.HasParseError() && response.IsObject()) {
                    gmTableObject* destination = featured ? m_featured : m_news;
                    for (auto member = response.MemberBegin(); member != response.MemberEnd(); ++member) {
                        gmVariable key(m_machine.AllocStringObject(member->name.GetString(),
                            static_cast<int>(member->name.GetStringLength())));
                        destination->Set(&m_machine, key, JsonToGm(m_machine, member->value, 0));
                    }
                }
            } else {
                std::fprintf(stderr, "network: %s request failed (HTTP %ld)\n",
                             featured ? "featured" : "news", completion.status);
            }
        }
        if (completion.id == m_translationRequestId) {
            m_translationRequestId = 0;
            if ((completion.status == 200 || completion.status == 201) && !completion.body.empty() &&
                !MergeNetworkTranslations(completion.body, m_networkTranslations))
                std::fprintf(stderr, "network: invalid translation catalog\n");
            m_translationReady = true;
        }
        const gmVariable contextValue = m_roots->Get(completion.id);
        if (contextValue.m_type == GM_TABLE) {
            auto* context = static_cast<gmTableObject*>(
                m_machine.GetObject(contextValue.m_value.m_ref));
            const gmVariable callbackValue = context->Get(0);
            const gmVariable callbackData = context->Get(1);
            const int callbackMode = static_cast<int>(context->Get(2).m_value.m_int);
            if (callbackMode == 8 && (completion.status == 200 || completion.status == 201))
                LoadNetworkAudioScript(m_machine, completion.body);
            if (callbackValue.m_type == GM_FUNCTION) {
                auto* callback = static_cast<gmFunctionObject*>(
                    m_machine.GetObject(callbackValue.m_value.m_ref));
                gmCall call;
                if (call.BeginFunction(&m_machine, callback)) {
                    call.AddParam(callbackData);
                    if (callbackMode < 0) {
                        call.AddParamString(completion.body.data(),
                                            static_cast<int>(completion.body.size()));
                        call.AddParamInt(static_cast<int>(completion.status));
                    } else if (callbackMode == 5) {
                        std::string level, upload;
                        const bool success = completion.status == 200 || completion.status == 201;
                        if (success) {
                            rapidjson::Document document;
                            document.Parse(completion.body.data(), completion.body.size());
                            if (!document.HasParseError() && document.IsObject() && document.HasMember("id") && document["id"].IsString()) {
                                const std::string path(document["id"].GetString(), document["id"].GetStringLength());
                                const size_t last = path.rfind(':');
                                if (last != std::string::npos) {
                                    const size_t previous = last == 0 ? std::string::npos : path.rfind(':', last - 1);
                                    const size_t start = previous != std::string::npos ? previous + 1 : (last == 0 ? 0 : 1);
                                    const size_t slash = path.find('/', start);
                                    level = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start).substr(0, 99);
                                    upload = path.substr(last + 1, 99);
                                }
                            }
                        }
                        if (level.empty()) call.AddParamNull(); else call.AddParamString(level.c_str());
                        if (upload.empty()) call.AddParamNull(); else call.AddParamString(upload.c_str());
                        call.AddParamInt(success ? 0 : 1);
                    } else {
                        gmVariable result = gmVariable::s_null;
                        int error = (purged || transportFailed) && responseStatus != 0
                            ? static_cast<int>(responseStatus)
                            : (completion.status == 0 ? 1 : static_cast<int>(completion.status));
                        if (callbackMode == 3 || callbackMode == 4 || callbackMode == 7 || callbackMode == 8) error = 1;
                        if (completion.status == 200 || completion.status == 201) {
                            error = 0;
                            if (callbackMode == 4) {
                                error = 1;
                                Renderer* renderer = GetEngineContext().renderer;
                                const gmVariable keyValue = context->Get(5);
                                if (renderer && keyValue.m_type == GM_STRING && !completion.body.empty() &&
                                    completion.body.size() <= static_cast<size_t>(std::numeric_limits<int>::max())) {
                                    auto* keyObject = static_cast<gmStringObject*>(m_machine.GetObject(keyValue.m_value.m_ref));
                                    const std::string key = keyObject->GetString();
                                    const auto* bytes = reinterpret_cast<const unsigned char*>(completion.body.data());
                                    int width = 0, height = 0, channels = 0;
                                    unsigned char* decoded = stbi_load_from_memory(bytes, static_cast<int>(completion.body.size()),
                                                                                  &width, &height, &channels, 0);
                                    const bool validImage = decoded != nullptr;
                                    if (decoded) stbi_image_free(decoded);
                                    const unsigned handle = validImage ? renderer->LoadTextureFromMemory(
                                        bytes, completion.body.size(), "3:" + key) : 0;
                                    if (handle) {
                                        auto lease = MakeTextureResourceLease(handle, 3, key);
                                        RetainManagedTextureResource(handle, 3, key);
                                        result = MakeTextureHandle(m_machine, handle, std::move(lease));
                                        error = 0;
                                    }
                                }
                            } else if (callbackMode == 3) {
                                result = ParseNetworkLevelData(m_machine, completion.body);
                            } else if (callbackMode == 1) {
                                result = gmVariable(m_machine.AllocStringObject(
                                    completion.body.data(), static_cast<int>(completion.body.size())));
                            } else if (callbackMode != 8 && !completion.body.empty()) {
                                result = ParseRetailJson(m_machine, completion.body, callbackMode);
                            }
                        }
                        call.AddParam(result);
                        call.AddParamInt(error);
                    }
                    call.End();
                }
            }
        }
        m_roots->Set(&m_machine, completion.id, gmVariable::s_null);
    }
}

void NetworkClient::UpdateCoordinator() {
    std::thread stoppedWorker;
    bool restartWorker = false;
    bool reconnect = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_offlineTransitionPending && m_identityWorkerStopped) {
            stoppedWorker = std::move(m_worker);
            restartWorker = true;
            m_identityWorkerStopRequested = false;
            m_identityWorkerStopped = false;
            m_offlineTransitionPending = false;
            m_coordinatorState = 4;
            reconnect = m_reconnectAfterOfflineTransition;
            m_reconnectAfterOfflineTransition = false;
        }
    }
    if (stoppedWorker.joinable()) stoppedWorker.join();
    if (restartWorker) m_worker = std::thread(&NetworkClient::WorkerMain, this, true);
    if (reconnect) GoOnline();

    if (m_coordinatorState == 24) {
        if (!m_friendsReady) {
            // Desktop has no PSN contacts; the replacement server supplies its friend list.
            m_friendsFetchRequestId = QueueRetailGet("/otg/~identity:" + m_userId + "/~friends.all",
                gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
            if (!m_friendsFetchRequestId) m_coordinatorError = -12;
        }
        m_coordinatorState = 25;
        return;
    }
    if (m_coordinatorState == 25 && m_friendsReady) {
        m_coordinatorState = 26;
        return;
    }
    if (m_coordinatorState == 26) {
        m_featuredReady = false;
        m_newsReady = false;
        m_featuredRequestId = QueueRetailGet("/otg/global/featured/~metadata:*.get",
            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
        m_newsRequestId = QueueRetailGet("/otg/global/news/~metadata:*.get",
            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
        m_coordinatorState = 27;
        return;
    }
    if (m_coordinatorState == 27 && m_featuredReady && m_newsReady) {
        m_coordinatorState = 28;
        std::fprintf(stderr, "network: initial community data requests complete\n");
        return;
    }
    if (m_coordinatorState == 28) {
        const char* locale = SystemConfig::Get().NetworkLanguage();
        m_featuredReady = false;
        m_newsReady = false;
        m_translationReady = false;
        m_featuredRequestId = QueueRetailGet(std::string("/otg/global/featured/") + locale + "/~metadata:*.get",
            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
        m_newsRequestId = QueueRetailGet(std::string("/otg/global/news/") + locale + "/~metadata:*.get",
            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
        const std::string translationPath = std::string("/otg/ps3/") +
            SystemConfig::Get().networkProductRegion + "/" + locale + "/~translation.get";
        m_translationRequestId = QueueRetailGet(translationPath, gmVariable::s_null, gmVariable::s_null,
                                                gmVariable::s_null, 0, 1);
        m_coordinatorState = 29;
        return;
    }
    if (m_coordinatorState == 29 && m_featuredReady && m_newsReady && m_translationReady) {
        m_coordinatorState = 30;
        std::fprintf(stderr, "network: coordinator online\n");
        return;
    }
    if ((m_coordinatorState != 22 && m_coordinatorState != 23) || !m_eulaReady) return;
    int accepted = 0;
    gmCall call;
    if (call.BeginGlobalFunction(&m_machine, "SAVEDATAHELPER_HasAcceptedEula")) {
        call.AddParamString(m_eulaVersion.c_str());
        call.End();
        if (call.DidReturnVariable()) call.GetReturnedInt(accepted);
    }
    m_coordinatorState = accepted ? 24 : 23;
}

void NetworkClient::EnqueueWorkerCompletion(Completion&& completion, bool identity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& queue = identity ? m_identityCompletions : m_globalCompletions;
    // The retail worker ignores the failure returned by a full completion ring.
    if (queue.size() < 32) queue.push_back(std::move(completion));
}

void NetworkClient::WorkerMain(bool a_identity) {
#if defined(OPENEGG_HAS_CURL)
    auto& requests = a_identity ? m_identityRequests : m_globalRequests;
    for (;;) {
        Request request;
        std::string sessionId;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, [&] {
                return m_stopping || !requests.empty() ||
                    (a_identity && m_identityWorkerStopRequested);
            });
            if (m_stopping) return;
            if (a_identity && m_identityWorkerStopRequested) {
                m_identityWorkerStopped = true;
                m_wake.notify_all();
                return;
            }
            request = std::move(requests.front());
            requests.pop_front();
            if (a_identity) m_identityRequestInFlight = true;
            m_wake.notify_all();
            const auto pending = m_pending.find(request.id);
            request.purged = pending != m_pending.end() && pending->second.purged;
            if (request.sendSession) sessionId = m_sessionId;
        }

        Completion completion;
        completion.id = request.id;
        completion.cacheKey = request.cacheKey;
        completion.identity = a_identity;
        if (request.purged) {
            EnqueueWorkerCompletion(std::move(completion), a_identity);
            if (a_identity) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_identityRequestInFlight = false;
                m_wake.notify_all();
            }
            continue;
        }
        CURL* curl = curl_easy_init();
        if (curl) {
            HeaderContext responseHeaders{curl, request.rawResponse, true, request.sendSession};
            curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, ReceiveHeaders);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReceiveBody);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &completion.body);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);
            // Retail enables redirects. Identity requests are restricted to
            // root-relative locations until retail's header policy is known.
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
#if LIBCURL_VERSION_NUM >= 0x075500
            curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
            curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif

            if (request.post) {
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.data());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                                 static_cast<curl_off_t>(request.body.size()));
            }

            struct curl_slist* headers = nullptr;
            if (!request.contentType.empty()) {
                const std::string contentType = "Content-Type: " + request.contentType;
                headers = curl_slist_append(headers, contentType.c_str());
            }
            if (request.validateSavedSession) {
                headers = curl_slist_append(headers, "X-OpenEgg-Validate-Session: 1");
            }
            if (request.post && request.url.find("/identity/login/token/psn") != std::string::npos) {
                headers = curl_slist_append(headers, "X-OpenEgg-Remember-Session: 1");
            }
            if (request.sendSession && !sessionId.empty()) {
                const std::string cookie = "X-OTG-Identity-SessionId: " + sessionId;
                headers = curl_slist_append(headers, cookie.c_str());
            }
            if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            const CURLcode result = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &completion.status);
            if (result != CURLE_OK && !responseHeaders.skippedErrorBody) {
                // Retail skips body reception for non-success HTTP statuses.
                // A later body error cannot turn those into transport failures.
                completion.transportFailed = completion.status == 0 ||
                    completion.status == 200 || completion.status == 201 ||
                    (completion.status >= 300 && completion.status < 400);
                std::fprintf(stderr, "network: GET failed: %s\n", curl_easy_strerror(result));
            }
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        EnqueueWorkerCompletion(std::move(completion), a_identity);
        if (a_identity) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_identityRequestInFlight = false;
            m_wake.notify_all();
        }
    }
#endif
}
