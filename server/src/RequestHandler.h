#pragma once

#include <string>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <memory>
#include <condition_variable>

#include "IProtocol.h"

class RequestHandler
{
public:
    typedef std::vector<char> Buffer;

    struct ClientInfo
    {
        std::string address;
        unsigned short port;

        bool operator<(const ClientInfo& other) const;
    };

private:
    struct Request
    {
        Request(ClientInfo&& clientInfo, Buffer&& data);
        ClientInfo clientInfo;
        Buffer data;
    };

public:
    RequestHandler();
    ~RequestHandler();

    void Stop();

    void AddRequest(const std::string& clientAddress, unsigned short clientPort, std::vector<char>&& data);
    std::list<std::pair<ClientInfo, Buffer>> GetResponses();
    bool HasResponses();

private:
    void ThreadProc();
    void Process();
    void SetEvent();
    void WaitForEvent();

private:
    std::thread m_thread;
    std::atomic_bool m_stop;

    std::mutex m_requestsLock;
    std::list<Request> m_requests;

    std::mutex m_responsesLock;
    std::list<std::pair<ClientInfo, Buffer>> m_responses;
    std::atomic_bool m_hasResponses;

    std::mutex m_eventLock;
    std::condition_variable m_eventCondition;
    bool m_eventFlag;

    std::map<ClientInfo, std::unique_ptr<IProtocol>> m_protocols;
};
