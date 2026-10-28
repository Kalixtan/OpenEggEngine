#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <array>
#include <vector>

class gmMachine;
class gmTableObject;
class gmVariable;

class NetworkClient {
public:
    struct Friend {
        std::string identity;
        std::string displayName;
    };
    NetworkClient(gmMachine& a_machine, std::string a_baseUrl, std::string a_sessionId,
                  std::string a_userId);
    ~NetworkClient();

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Development entry point used to verify a replacement-server connection.
    int QueueGet(const std::string& a_path, const gmVariable& a_callback,
                 const gmVariable& a_callbackData);
    int QueueRetailGet(const std::string& a_path, const gmVariable& a_callback,
                       const gmVariable& a_callbackData, const gmVariable& a_tag,
                       int a_bucket, int a_callbackMode);
    int QueueRetailPost(const std::string& a_path, const std::string& a_body,
                        const std::string& a_contentType, const gmVariable& a_callback,
                        const gmVariable& a_callbackData, const gmVariable& a_tag,
                        int a_bucket, int a_callbackMode);
    int QueueRetailImage(const std::string& a_path, const std::string& a_cacheKey,
                         const gmVariable& a_callback, const gmVariable& a_callbackData,
                         const gmVariable& a_tag, bool a_sendSession = true);
    bool QueueAssetBundle(const std::string& level, const std::string& version,
                          const std::string& outputPath, const gmVariable& callback,
                          const gmVariable& callbackData, const gmVariable& tag);
    const std::string& UserId() const { return m_userId; }
    void SetPlatformTicket(std::string a_ticket);
    void GoOnline();
    void GoOffline();
    int CoordinatorState() const { return m_coordinatorState; }
    int CoordinatorError() const { return m_coordinatorError; }
    bool IdentityReady() const { return m_identityReady; }
    const std::string& PlatformOnlineId() const { return m_platformOnlineId; }
    const std::string& PlatformCountry() const { return m_platformCountry; }
    bool HasChatPermissions() const { return m_chatPermissions; }
    bool IsSendQueueFull();
    bool HasEula() const { return m_eulaReady; }
    const std::string& EulaText() const { return m_eulaText; }
    const std::string& EulaVersion() const { return m_eulaVersion; }
    bool FriendsReady() const { return m_friendsReady; }
    const std::vector<Friend>& Friends() const { return m_friends; }
    const std::string* FindNetworkTranslation(const std::string& key) const;
    gmTableObject* Featured() const { return m_featured; }
    gmTableObject* News() const { return m_news; }
    void UpdateCoordinator();
    void Purge(int a_bucket);
    void PumpCallbacks();

private:
    struct Request {
        int id = 0;
        std::string url;
        std::string body;
        std::string contentType;
        bool post = false;
        int bucket = 0;
        bool purged = false;
        bool sendSession = true;
    };
    struct Completion {
        int id = 0;
        long status = 0;
        std::string body;
    };
    struct PendingRequest {
        int bucket;
        bool purged = false;
        bool identity = true;
    };
    struct AssetBundle {
        std::string path;
        int remaining = 3;
        std::array<int, 3> sizes{};
    };

    void WorkerMain();
    int Queue(const std::string& a_path, const gmVariable& a_callback,
              const gmVariable& a_callbackData, const gmVariable& a_tag,
              int a_bucket, int a_callbackMode, bool a_post = false,
              const std::string& a_body = {}, const std::string& a_contentType = {},
              const std::string& a_resourceKey = {}, bool a_sendSession = true);
    bool BeginTicketLogin(const std::string& a_ticket);

    gmMachine& m_machine;
    gmTableObject* m_roots = nullptr;
    std::string m_baseUrl;
    std::string m_sessionId;
    std::string m_userId;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<Request> m_requests;
    std::deque<Completion> m_completions;
    std::unordered_map<int, PendingRequest> m_pending;
    std::unordered_map<int, AssetBundle> m_assetBundles;
    std::unordered_map<int, std::pair<int, int>> m_assetRequests;
    std::thread m_worker;
    bool m_stopping = false;
    int m_nextId = 1;
    int m_nextAssetBundleId = 1;
    int m_loginRequestId = 0;
    int m_manualValidationRequestId = 0;
    int m_helloRequestId = 0;
    int m_eulaRequestId = 0;
    int m_friendsUploadRequestId = 0;
    int m_friendsFetchRequestId = 0;
    int m_featuredRequestId = 0;
    int m_newsRequestId = 0;
    int m_translationRequestId = 0;
    std::string m_platformTicket;
    std::string m_manualSessionId;
    std::string m_manualUserId;
    std::string m_platformOnlineId;
    std::string m_platformCountry;
    int m_coordinatorState = 1;
    int m_coordinatorError = 0;
    bool m_identityReady = false;
    bool m_managerAttached = false;
    bool m_chatPermissions = false;
    bool m_eulaReady = false;
    std::string m_eulaText;
    std::string m_eulaVersion;
    bool m_friendsReady = false;
    std::vector<Friend> m_friends;
    std::unordered_map<std::string, std::string> m_networkTranslations;
    gmTableObject* m_featured = nullptr;
    gmTableObject* m_news = nullptr;
    bool m_featuredReady = false;
    bool m_newsReady = false;
    bool m_translationReady = false;
};
