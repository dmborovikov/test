#include "UdpSocket.h"
#include <cstring>
#include <algorithm>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>

UdpSocket::UdpSocket()
    : m_socketId(INVALID_SOCKET)
    , m_netProtocol(NetworkProtocol::Unknown)
    , m_nonBlocking(false)
{
}

UdpSocket::UdpSocket(NetworkProtocol netProtocol, bool nonBlocking)
    : UdpSocket()
{
    Create(netProtocol, nonBlocking);
}

UdpSocket::~UdpSocket()
{
    Close();
}

bool UdpSocket::IsSet() const
{
    return m_socketId != INVALID_SOCKET;
}

bool UdpSocket::Create(NetworkProtocol netProtocol, bool nonBlocking)
{
    Close();
    auto socketId = ::socket(netProtocol == NetworkProtocol::IPv4 ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (socketId != INVALID_SOCKET)
    {
        m_socketId = socketId;
        m_netProtocol = netProtocol;

        if (SetNonBlockingMode(nonBlocking))
        {
            linger lingerOptions { 1, 0 };
            setsockopt(socketId, SOL_SOCKET, SO_LINGER, &lingerOptions, sizeof(lingerOptions));
        }
        else
        {
            Close();
        }
    }

    return m_socketId != INVALID_SOCKET;
}

void UdpSocket::Close()
{
    if (IsSet() && ::close(m_socketId) != SOCKET_ERROR)
    {
        m_socketId = INVALID_SOCKET;
        m_netProtocol = NetworkProtocol::Unknown;
        m_nonBlocking = false;
    }
}

bool UdpSocket::Bind(unsigned short port, const std::string& address, bool reuseAddress, unsigned timeout)
{
    bool isSuccess = false;

    if (IsSet())
    {
        if (reuseAddress)
        {
            int reuse = 1;
            setsockopt(m_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
        }

        if (m_netProtocol == NetworkProtocol::IPv6)
        {
            int onlyIpv6 = 1;
            setsockopt(m_socketId, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&onlyIpv6, sizeof(onlyIpv6));
        }

        auto addresses = GetAddressInfo(address, port, m_netProtocol);
        const auto startTime = std::chrono::steady_clock::now();

        for (bool tryAgain = true; tryAgain;)
        {
            for (size_t i = 0; i < addresses.size(); ++i)
            {
                const auto& sockAddr = addresses[i];
                isSuccess = ::bind(m_socketId, sockAddr.GetSockAddr(), sockAddr.GetSockAddrSize()) == 0;

                if (isSuccess || std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() > timeout)
                {
                    tryAgain = false;
                    break;
                }
                else if (reuseAddress || errno != EADDRINUSE)
                {
                    addresses.erase(addresses.begin() + i);
                    break;
                }
                else
                {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(10ms);
                }
            }

            tryAgain = tryAgain && !addresses.empty();
        }
    }

    return isSuccess;
}

bool UdpSocket::SetNonBlockingMode(bool nonBlocking)
{
    unsigned long mode = nonBlocking ? 1 : 0;
    const bool isSuccess = ::ioctl(m_socketId, FIONBIO, &mode) != -1;

    if (isSuccess)
    {
        m_nonBlocking = nonBlocking;
    }

    return isSuccess;
}

bool UdpSocket::IsNonBlocking() const
{
    return m_nonBlocking;
}

int UdpSocket::Read(char* buff, unsigned bufSize, std::string* outAddress, unsigned short* outPort)
{
    int bytesRead = SOCKET_ERROR;

    if (IsSet() && buff && bufSize > 0)
    {
        SockAddr sockAddr;
        bytesRead = recvfrom(m_socketId, buff, bufSize, 0, sockAddr.GetSockAddrPtr(), sockAddr.GetSockAddrSizePtr());

        if (bytesRead > 0)
        {
            GetSocketInfo(sockAddr, outAddress, outPort, NULL);
        }
    }

    return bytesRead;
}

int UdpSocket::Write(const char* buff, unsigned bufSize, const std::string& address, unsigned short port)
{
    int bytesSent = SOCKET_ERROR;

    if (IsSet() && buff && bufSize > 0 && !address.empty() && port > 0)
    {
        const auto addresses = GetAddressInfo(address, port, m_netProtocol);

        for (size_t i = 0; i < addresses.size() && bytesSent <= 0; ++i)
        {
            bytesSent = sendto(m_socketId, buff, bufSize, 0, addresses[i].GetSockAddr(), addresses[i].GetSockAddrSize());
        }
    }

    return bytesSent;
}

bool UdpSocket::CanRead(unsigned waitTimeoutMillis) const
{
    return Poll(true, waitTimeoutMillis);
}

bool UdpSocket::CanWrite(unsigned waitTimeoutMillis) const
{
    return Poll(false, waitTimeoutMillis);
}

bool UdpSocket::Poll(bool readEvent, int timeout) const
{
    bool canReadOrWrite = false;

    if (IsSet())
    {
        short pollEvent = readEvent ? POLLIN : POLLOUT;

        struct pollfd fdArray{ 0, 0, 0 };
        fdArray.fd = m_socketId;
        fdArray.events = pollEvent;

        canReadOrWrite = poll(&fdArray, 1, timeout) > 0 && (fdArray.revents & pollEvent) != 0;

        if (!canReadOrWrite && (fdArray.revents & (POLLERR | POLLHUP | POLLNVAL)))
        {
            // TODO
        }
    }

    return canReadOrWrite;
}

bool UdpSocket::GetSocketInfo(const SockAddr& addrStorage,
    std::string* outPeerAddress, unsigned short* outPeerPort, NetworkProtocol* outNetworkProtocol)
{
    bool isSuccess = false;

    char peerAddressBuffer[NI_MAXHOST];
    char peerPortBuffer[NI_MAXSERV];

    if (getnameinfo(addrStorage.GetSockAddr(), addrStorage.GetSockAddrSize(),
        peerAddressBuffer, sizeof(peerAddressBuffer), peerPortBuffer, sizeof(peerPortBuffer), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
    {
        isSuccess = true;

        if (outPeerAddress)
        {
            *outPeerAddress = std::string(peerAddressBuffer);
        }

        if (outPeerPort)
        {
            *outPeerPort = (unsigned short)atoi(peerPortBuffer);
        }

        if (outNetworkProtocol)
        {
            *outNetworkProtocol = addrStorage.GetNetworkProtocol();
        }
    }

    return isSuccess;
}


SockAddr::SockAddr()
    : m_sockaddrSize(0)
{
    Init();
}

SockAddr::SockAddr(const sockaddr* addr, socklen_t addrSize)
    : m_sockaddrSize(0)
{
    Init(addr, addrSize);
}

void SockAddr::Init(const sockaddr* addr, socklen_t addrSize)
{
    if (Init() && addrSize <= (socklen_t)m_sockaddrBuffer.size())
    {
        memcpy(m_sockaddrBuffer.data(), addr, addrSize);
        m_sockaddrSize = addrSize;
    }
}

bool SockAddr::IsSet() const
{
    return m_sockaddrSize > 0 && !m_sockaddrBuffer.empty();
}

NetworkProtocol SockAddr::GetNetworkProtocol() const
{
    if (IsSet())
    {
        switch (reinterpret_cast<const sockaddr_storage*>(m_sockaddrBuffer.data())->ss_family)
        {
            case AF_INET:
            {
                return NetworkProtocol::IPv4;
            }
            case AF_INET6:
            {
                return NetworkProtocol::IPv6;
            }
            default: break;
        }
    }

    return NetworkProtocol::Unknown;
}

const sockaddr* SockAddr::GetSockAddr() const
{
    return IsSet()
        ? reinterpret_cast<const sockaddr*>(m_sockaddrBuffer.data())
        : nullptr;
}

socklen_t SockAddr::GetSockAddrSize() const
{
    return m_sockaddrSize;
}

sockaddr* SockAddr::GetSockAddrPtr()
{
    return IsSet()
        ? reinterpret_cast<sockaddr*>(m_sockaddrBuffer.data())
        : nullptr;
}

socklen_t* SockAddr::GetSockAddrSizePtr()
{
    return &m_sockaddrSize;
}

bool SockAddr::Init()
{
    m_sockaddrSize = sizeof(sockaddr_storage);
    m_sockaddrBuffer.resize(m_sockaddrSize);
    memset(m_sockaddrBuffer.data(), 0, m_sockaddrSize);

    return m_sockaddrSize > 0;
}

std::vector<SockAddr> GetAddressInfo(const std::string& address, unsigned short port, NetworkProtocol netProtocol)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    if (netProtocol == NetworkProtocol::IPv6)
    {
        hints.ai_family = AF_INET6;
    }
    else if (netProtocol == NetworkProtocol::IPv4)
    {
        hints.ai_family = AF_INET;
    }
    else
    {
        hints.ai_family = AF_UNSPEC;
    }

    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;

    if (hints.ai_family != AF_UNSPEC)
    {
        hints.ai_flags = hints.ai_flags | AI_NUMERICHOST;
    }

    std::vector<SockAddr> addresses;
    addrinfo* originalAddr = nullptr;

    if (getaddrinfo(address.empty() ? nullptr : address.data(), std::to_string(port).data(), &hints, &originalAddr) == 0)
    {
        for (auto* addrInfo = originalAddr; addrInfo != nullptr; addrInfo = addrInfo->ai_next)
        {
            addresses.push_back(SockAddr(addrInfo->ai_addr, (socklen_t)addrInfo->ai_addrlen));
        }

        freeaddrinfo(originalAddr);
    }

    return addresses;
}
