#include "RequestHandler.h"
#include "DefaultProtocol.h"
#include <algorithm>
#include <numeric>


bool RequestHandler::ClientInfo::operator<(const RequestHandler::ClientInfo& other) const
{
    return port < other.port || (port == other.port && address < other.address);
}

RequestHandler::Request::Request(ClientInfo&& clientInfo, Buffer&& data)
    : clientInfo(std::move(clientInfo))
    , data(std::move(data))
{
}

RequestHandler::RequestHandler()
    : m_stop(false)
    , m_hasResponses(false)
    , m_eventFlag(false)
{
    m_thread = std::thread([this] { ThreadProc(); });
}

RequestHandler::~RequestHandler()
{
    Stop();
}

void RequestHandler::Stop()
{
    m_stop = true;
    SetEvent();

    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void RequestHandler::AddRequest(const std::string& clientAddress, unsigned short clientPort, std::vector<char>&& data)
{
    {
        std::lock_guard<std::mutex> lock(m_requestsLock);
        m_requests.emplace_back(ClientInfo{ clientAddress, clientPort }, std::move(data));
    }

    SetEvent();
}

std::list<std::pair<RequestHandler::ClientInfo, RequestHandler::Buffer>> RequestHandler::GetResponses()
{
    std::list<std::pair<ClientInfo, Buffer>> responses;
    std::lock_guard<std::mutex> lock(m_responsesLock);

    if (!m_responses.empty())
    {
        m_responses.swap(responses);
    }

    m_hasResponses = false;

    return responses;
}

bool RequestHandler::HasResponses()
{
    return m_hasResponses;
}

void RequestHandler::ThreadProc()
{
    while (!m_stop)
    {
        WaitForEvent();
        Process();
    }
}

void RequestHandler::Process()
{
    std::list<Request> requests;

    {
        std::lock_guard<std::mutex> lock(m_requestsLock);
        m_requests.swap(requests);
    }

    for (auto& request : requests)
    {
        auto& protocol = m_protocols[request.clientInfo];
        if (!protocol)
        {
            protocol = std::make_unique<DefaultProtocol>();
        }

        auto response = protocol->Process(request.data);

        if (!response.empty())
        {
            std::lock_guard<std::mutex> lock(m_responsesLock);
            m_responses.emplace_back(request.clientInfo, std::move(response));
            m_hasResponses = true;
        }
    }

    for (auto it = m_protocols.begin(); it != m_protocols.end();)
    {
        if (it->second && (it->second->IsEmpty() || it->second->IsExpired()))
        {
            it = m_protocols.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void RequestHandler::SetEvent()
{
    {
        std::lock_guard<std::mutex> lock(m_eventLock);
        m_eventFlag = true;
    }

    m_eventCondition.notify_all();
}

void RequestHandler::WaitForEvent()
{
    std::unique_lock<std::mutex> lock(m_eventLock);
    m_eventCondition.wait(lock, [&] { return m_eventFlag; });
    m_eventFlag = false;
}
