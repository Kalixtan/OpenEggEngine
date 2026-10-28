#include "network_client.h"

#include "gmCall.h"
#include "gmFunctionObject.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "system_config.h"
#include "sha1.h"
#include "network_translation.h"
#include "network_level_data.h"
#include <rapidjson/document.h>

#include <algorithm>
#include <cstdio>
#include <limits>
#include <utility>

#if defined(OPENEGG_HAS_CURL)
#include <curl/curl.h>
#endif

namespace {
constexpr const char* kRootTableName = "__OpenEggNetworkRequestRoots";

// Desktop ticket files stand in for the PSN online-ID query. This reads only
// the common identity fields from PS3/Vita ticket versions 2.1 and 3.0.
std::string TicketOnlineId(const std::string& ticket) {
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
            return id;
        }
        pos += length;
    }
    return {};
}

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
        table->Set(&a_machine, "size", gmVariable(static_cast<gmint>(index)));
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
    rapidjson::Document document;
    document.Parse(a_body.data(), a_body.size());
    if (document.HasParseError()) return gmVariable::s_null;
    return JsonToGm(a_machine, document, a_mode);
}

#if defined(OPENEGG_HAS_CURL)
size_t ReceiveBody(char* a_data, size_t a_size, size_t a_count, void* a_context) {
    if (a_size != 0 && a_count > std::numeric_limits<size_t>::max() / a_size) return 0;
    const size_t bytes = a_size * a_count;
    auto* body = static_cast<std::string*>(a_context);
    body->append(a_data, bytes);
    return bytes;
}
#endif
} // namespace

NetworkClient::NetworkClient(gmMachine& a_machine, std::string a_baseUrl,
                             std::string a_sessionId, std::string a_userId)
    : m_machine(a_machine), m_baseUrl(std::move(a_baseUrl)),
      m_sessionId(std::move(a_sessionId)), m_userId(std::move(a_userId)) {
    while (!m_baseUrl.empty() && m_baseUrl.back() == '/') m_baseUrl.pop_back();
    m_roots = m_machine.AllocTableObject();
    m_machine.GetGlobals()->Set(&m_machine, kRootTableName, gmVariable(m_roots));
    m_featured = m_machine.AllocTableObject();
    m_roots->Set(&m_machine, "featured", gmVariable(m_featured));
    m_news = m_machine.AllocTableObject();
    m_roots->Set(&m_machine, "news", gmVariable(m_news));

#if defined(OPENEGG_HAS_CURL)
    static const CURLcode curlInit = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curlInit == CURLE_OK && !m_baseUrl.empty())
        m_worker = std::thread(&NetworkClient::WorkerMain, this);
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
    m_wake.notify_one();
    if (m_worker.joinable()) m_worker.join();
    m_machine.GetGlobals()->Set(&m_machine, kRootTableName, gmVariable::s_null);
}

int NetworkClient::QueueGet(const std::string& a_path, const gmVariable& a_callback,
                            const gmVariable& a_callbackData) {
    return Queue(a_path, a_callback, a_callbackData, gmVariable::s_null, 0, -1);
}

int NetworkClient::QueueRetailGet(const std::string& a_path, const gmVariable& a_callback,
                                  const gmVariable& a_callbackData, const gmVariable& a_tag,
                                  int a_bucket, int a_callbackMode) {
    return Queue(a_path, a_callback, a_callbackData, a_tag, a_bucket, a_callbackMode);
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

bool NetworkClient::BeginTicketLogin(const std::string& a_ticket) {
    if (a_ticket.empty() || m_loginRequestId != 0) return false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessionId.clear();
    }
    m_userId.clear();
    m_loginRequestId = QueueRetailPost("/identity/login/token/psn", a_ticket,
        "application/octet-stream", gmVariable::s_null, gmVariable::s_null,
        gmVariable::s_null, 0, 0);
    return m_loginRequestId != 0;
}

void NetworkClient::SetPlatformTicket(std::string a_ticket) {
    m_platformOnlineId = TicketOnlineId(a_ticket);
    m_platformTicket = std::move(a_ticket);
    if (!m_platformTicket.empty() && m_worker.joinable()) m_coordinatorState = 4;
}

void NetworkClient::GoOnline() {
    if (m_coordinatorState != 2 && m_coordinatorState != 4) return;
    m_managerAttached = true;
    m_chatPermissions = SystemConfig::Get().networkCommunityAllowed;
    m_coordinatorError = 0;
    if (BeginTicketLogin(m_platformTicket)) {
        m_coordinatorState = 18;
    } else {
        m_coordinatorError = -12;
    }
}

bool NetworkClient::IsSendQueueFull() {
    if (!m_managerAttached) return true;
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_requests.size() >= 32;
}

const std::string* NetworkClient::FindNetworkTranslation(const std::string& key) const {
    const auto entry = m_networkTranslations.find(key);
    return entry == m_networkTranslations.end() ? nullptr : &entry->second;
}

void NetworkClient::GoOffline() {
    if (m_coordinatorState == 1) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto pending = m_pending.find(m_loginRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        pending = m_pending.find(m_helloRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        pending = m_pending.find(m_eulaRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        pending = m_pending.find(m_friendsUploadRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        pending = m_pending.find(m_friendsFetchRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        pending = m_pending.find(m_featuredRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        pending = m_pending.find(m_newsRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        pending = m_pending.find(m_translationRequestId);
        if (pending != m_pending.end()) pending->second.purged = true;
        m_sessionId.clear();
    }
    m_loginRequestId = 0;
    m_helloRequestId = 0;
    m_eulaRequestId = 0;
    m_friendsUploadRequestId = 0;
    m_friendsFetchRequestId = 0;
    m_featuredRequestId = 0;
    m_newsRequestId = 0;
    m_translationRequestId = 0;
    m_userId.clear();
    m_identityReady = false;
    m_coordinatorError = 0;
    m_coordinatorState = 4;
}

int NetworkClient::Queue(const std::string& a_path, const gmVariable& a_callback,
                         const gmVariable& a_callbackData, const gmVariable& a_tag,
                         int a_bucket, int a_callbackMode, bool a_post,
                         const std::string& a_body, const std::string& a_contentType) {
    if (m_baseUrl.empty() || !m_worker.joinable() || a_path.empty() ||
        a_path.front() != '/' || a_path.size() > 1023 ||
        m_baseUrl.size() + a_path.size() > 1023 ||
        (a_callbackMode < 0 && a_callback.m_type != GM_FUNCTION) || m_roots == nullptr) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stopping || m_nextId <= 0 || m_nextId == std::numeric_limits<int>::max())
        return 0;

    const int id = m_nextId++;
    gmTableObject* context = m_machine.AllocTableObject();
    context->Set(&m_machine, 0, a_callback);
    context->Set(&m_machine, 1, a_callbackData);
    context->Set(&m_machine, 2, gmVariable(static_cast<gmint>(a_callbackMode)));
    context->Set(&m_machine, 3, gmVariable(static_cast<gmint>(a_bucket)));
    context->Set(&m_machine, 4, a_tag);
    m_roots->Set(&m_machine, id, gmVariable(context));
    const int bucket = a_callbackMode >= 0 && a_tag.m_type == GM_INT
        ? static_cast<int>(a_tag.m_value.m_int) : a_bucket;
    m_pending.emplace(id, PendingRequest{bucket, false});
    m_requests.push_back({id, m_baseUrl + a_path, a_body, a_contentType, a_post, bucket, false});
    m_wake.notify_one();
    return id;
}

void NetworkClient::Purge(int a_bucket) {
    if (a_bucket == 0) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& entry : m_pending) {
        if (entry.second.bucket == a_bucket) entry.second.purged = true;
    }
}

void NetworkClient::PumpCallbacks() {
    std::deque<Completion> completions;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        completions.swap(m_completions);
    }

    for (Completion& completion : completions) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto pending = m_pending.find(completion.id);
            if (pending != m_pending.end()) {
                if (pending->second.purged) {
                    completion.status = 0;
                    completion.body.clear();
                }
                m_pending.erase(pending);
            }
        }
        if (completion.id == m_loginRequestId) {
            m_loginRequestId = 0;
            rapidjson::Document response;
            response.Parse(completion.body.data(), completion.body.size());
            if ((completion.status == 200 || completion.status == 201) &&
                !response.HasParseError() && response.IsObject() &&
                response.HasMember("session") && response["session"].IsObject()) {
                const auto& session = response["session"];
                if (session.HasMember("id") && session["id"].IsString() &&
                    session.HasMember("person") && session["person"].IsObject() &&
                    session["person"].HasMember("id") && session["person"]["id"].IsString()) {
                    const auto& id = session["id"];
                    const auto& user = session["person"]["id"];
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_sessionId.assign(id.GetString(), std::min<size_t>(id.GetStringLength(), 36));
                    }
                    m_userId.assign(user.GetString(), std::min<size_t>(user.GetStringLength(), 36));
                    m_identityReady = true;
                    std::fprintf(stderr, "network: ticket login succeeded\n");
                    m_helloRequestId = QueueRetailGet("/otg/~identity:*.hello",
                        gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
                    if (m_helloRequestId) {
                        m_coordinatorState = 20;
                    } else {
                        m_coordinatorError = -12;
                    }
                } else {
                    m_coordinatorError = -12;
                    std::fprintf(stderr, "network: ticket login returned an incomplete session\n");
                }
            } else {
                m_coordinatorError = -12;
                std::fprintf(stderr, "network: ticket login failed (HTTP %ld)\n", completion.status);
            }
        }
        if (completion.id == m_helloRequestId) {
            m_helloRequestId = 0;
            if (completion.status == 200 || completion.status == 201) {
                std::fprintf(stderr, "network: identity hello succeeded\n");
                // The traced PS3 build uses the SCEA product ID. Its EULA
                // route maps French and Spanish to their American locales.
                const int language = SystemConfig::Get().languageCode;
                const char* locale = language == 2 ? "fr" : language == 3 ? "es_mx" : "en";
                m_eulaRequestId = QueueRetailGet(std::string("/otg/ps3/SCEA/") + locale + "/~eula.get",
                    gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 1);
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
        if (completion.id == m_friendsUploadRequestId) {
            m_friendsUploadRequestId = 0;
            if (completion.status == 200 || completion.status == 201) {
                m_friendsFetchRequestId = QueueRetailGet("/otg/~identity:" + m_userId + "/~friends.all",
                    gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
                if (!m_friendsFetchRequestId) m_coordinatorError = -12;
            } else {
                m_coordinatorError = -12;
                std::fprintf(stderr, "network: friends upload failed (HTTP %ld)\n", completion.status);
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
                    } else {
                        gmVariable result = gmVariable::s_null;
                        int error = completion.status == 0 ? 1 : static_cast<int>(completion.status);
                        if (callbackMode == 3) error = 1;
                        if (completion.status == 200 || completion.status == 201) {
                            error = 0;
                            if (callbackMode == 3) {
                                result = ParseNetworkLevelData(m_machine, completion.body);
                            } else if (callbackMode == 1) {
                                result = gmVariable(m_machine.AllocStringObject(
                                    completion.body.data(), static_cast<int>(completion.body.size())));
                            } else if (!completion.body.empty()) {
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
    if (m_coordinatorState == 24) {
        if (!m_friendsReady) {
            // Desktop has no PSN friend provider yet; send an empty list.
            m_friendsUploadRequestId = QueueRetailPost("/identity/person/" + m_userId + "/data/psn/friends-list",
                "", "text/plain", gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
            if (!m_friendsUploadRequestId) m_coordinatorError = -12;
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
        static const char* locales[] = {"ja", "en", "fr", "es", "de", "it", "nl", "pt", "ru", "ko",
                                        "zh_hant", "zh", "fi", "sv", "da", "no", "pl", "pt_br", "en_gb", "es_mx"};
        const int code = SystemConfig::Get().languageCode;
        const char* locale = code >= 0 && code < 20 ? locales[code] : "en";
        m_featuredReady = false;
        m_newsReady = false;
        m_translationReady = false;
        m_featuredRequestId = QueueRetailGet(std::string("/otg/global/featured/") + locale + "/~metadata:*.get",
            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
        m_newsRequestId = QueueRetailGet(std::string("/otg/global/news/") + locale + "/~metadata:*.get",
            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 0);
        m_translationRequestId = QueueRetailGet(std::string("/otg/ps3/SCEA/") + locale + "/~translation.get",
            gmVariable::s_null, gmVariable::s_null, gmVariable::s_null, 0, 1);
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

void NetworkClient::WorkerMain() {
#if defined(OPENEGG_HAS_CURL)
    for (;;) {
        Request request;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, [this] { return m_stopping || !m_requests.empty(); });
            if (m_stopping) return;
            request = std::move(m_requests.front());
            m_requests.pop_front();
            const auto pending = m_pending.find(request.id);
            request.purged = pending != m_pending.end() && pending->second.purged;
        }

        Completion completion;
        completion.id = request.id;
        if (request.purged) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_completions.push_back(std::move(completion));
            continue;
        }
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReceiveBody);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &completion.body);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);

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
            std::string sessionId;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                sessionId = m_sessionId;
            }
            if (!sessionId.empty()) {
                const std::string cookie = "X-OTG-Identity-SessionId: " + sessionId;
                headers = curl_slist_append(headers, cookie.c_str());
            }
            if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            const CURLcode result = curl_easy_perform(curl);
            if (result == CURLE_OK)
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &completion.status);
            else
                std::fprintf(stderr, "network: GET failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_completions.push_back(std::move(completion));
        }
    }
#endif
}
