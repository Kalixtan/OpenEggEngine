#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

class gmMachine;
class gmTableObject;
class gmVariable;

class NetworkClient {
public:
    NetworkClient(gmMachine& a_machine, std::string a_baseUrl, std::string a_sessionId);
    ~NetworkClient();

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Development entry point used to verify a replacement-server connection.
    int QueueGet(const std::string& a_path, const gmVariable& a_callback,
                 const gmVariable& a_callbackData);
    int QueueRetailGet(const std::string& a_path, const gmVariable& a_callback,
                       const gmVariable& a_callbackData, const gmVariable& a_tag,
                       int a_bucket, int a_callbackMode);
    void PumpCallbacks();

private:
    struct Request {
        int id = 0;
        std::string url;
    };
    struct Completion {
        int id = 0;
        long status = 0;
        std::string body;
    };

    void WorkerMain();
    int Queue(const std::string& a_path, const gmVariable& a_callback,
              const gmVariable& a_callbackData, const gmVariable& a_tag,
              int a_bucket, int a_callbackMode);

    gmMachine& m_machine;
    gmTableObject* m_roots = nullptr;
    std::string m_baseUrl;
    std::string m_sessionId;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<Request> m_requests;
    std::deque<Completion> m_completions;
    std::thread m_worker;
    bool m_stopping = false;
    int m_nextId = 1;
};
