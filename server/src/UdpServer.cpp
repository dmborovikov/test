#include "UdpServer.h"
#include "UdpSocket.h"

const size_t BUF_SIZE = 1472;
const unsigned POLL_TIMEOUT = 10;


UdpServer::UdpServer()
    : m_stop(false)
{
}

UdpServer::~UdpServer()
{
    Stop();
}

void UdpServer::Start(const std::string& address, unsigned short port)
{
    printf("Server started\n");

    UdpSocket socket(NetworkProtocol::IPv4, true);

    if (socket.Bind(port, address, true, 1000))
    {
        while (!m_stop)
        {
            if (socket.CanRead(POLL_TIMEOUT))
            {
                std::vector<char> buffer(BUF_SIZE);
                std::string clientAddress;
                unsigned short clientPort;

                const int bytesRead = socket.Read(buffer.data(), buffer.size(), &clientAddress, &clientPort);

                if (bytesRead > 0)
                {
                    buffer.resize(bytesRead);
                    m_handler.AddRequest(clientAddress, clientPort, std::move(buffer));
                }
            }

            if (m_handler.HasResponses())
            {
                for (const auto& response : m_handler.GetResponses())
                {
                    if (socket.Write(response.second.data(), response.second.size(), response.first.address, response.first.port) == (int)response.second.size())
                    {
                        //printf("Package sent\n");
                    }
                    else
                    {
                        printf("Can't send package\n");
                    }
                }
            }
        }
    }
    else
    {
        printf("Can't bind: %s:%hu\n", address.c_str(), port);
    }
}

void UdpServer::Stop()
{
    m_stop = true;
}
