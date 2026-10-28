#include "network_client.h"

#include "gmCall.h"
#include "gmFunctionObject.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "system_config.h"
#include <rapidjson/document.h>

#include <cstdio>
#include <limits>
#include <utility>

#if defined(OPENEGG_HAS_CURL)
#include <curl/curl.h>
#endif

namespace {
constexpr const char* kRootTableName = "__OpenEggNetworkRequestRoots";

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
                             std::string a_sessionId)
    : m_machine(a_machine), m_baseUrl(std::move(a_baseUrl)),
      m_sessionId(std::move(a_sessionId)) {
    while (!m_baseUrl.empty() && m_baseUrl.back() == '/') m_baseUrl.pop_back();
    m_roots = m_machine.AllocTableObject();
    m_machine.GetGlobals()->Set(&m_machine, kRootTableName, gmVariable(m_roots));

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

int NetworkClient::Queue(const std::string& a_path, const gmVariable& a_callback,
                         const gmVariable& a_callbackData, const gmVariable& a_tag,
                         int a_bucket, int a_callbackMode) {
    if (m_baseUrl.empty() || !m_worker.joinable() || a_path.empty() ||
        a_path.front() != '/' || a_path.size() > 1023 ||
        m_baseUrl.size() + a_path.size() > 1023 ||
        a_callback.m_type != GM_FUNCTION || m_roots == nullptr)
        return 0;

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
    m_requests.push_back({id, m_baseUrl + a_path});
    m_wake.notify_one();
    return id;
}

void NetworkClient::PumpCallbacks() {
    std::deque<Completion> completions;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        completions.swap(m_completions);
    }

    for (Completion& completion : completions) {
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
                        if (completion.status == 200 || completion.status == 201) {
                            error = 0;
                            if (callbackMode == 1) {
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
        }

        Completion completion;
        completion.id = request.id;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReceiveBody);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &completion.body);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);

            struct curl_slist* headers = nullptr;
            if (!m_sessionId.empty()) {
                const std::string cookie = "X-OTG-Identity-SessionId: " + m_sessionId;
                headers = curl_slist_append(headers, cookie.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }

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
