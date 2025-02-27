#pragma once
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifndef INVALID_SOCKET
    #define INVALID_SOCKET (-1)
#endif

#ifndef SOCKET_ERROR
    #define SOCKET_ERROR (-1)
#endif

enum class NetworkProtocol
{
    IPv4,
    IPv6,
    Unknown
};

class SockAddr;

std::vector<SockAddr> GetAddressInfo(const std::string& address, unsigned short port, NetworkProtocol networkProtocol);

class UdpSocket
{
public:
    UdpSocket();
    UdpSocket(NetworkProtocol netProtocol, bool nonBlocking);
    ~UdpSocket();

    bool Create(NetworkProtocol netProtocol, bool nonBlocking);
    void Close();

    bool Bind(unsigned short port, const std::string& address, bool reuseAddress, unsigned timeout);

    bool IsSet() const;

    bool SetNonBlockingMode(bool nonBlocking);
    bool IsNonBlocking() const;

    int Read(char* buff, unsigned bufSize, std::string* outAddress, unsigned short* outPort);
    int Write(const char* buff, unsigned bufSize, const std::string& address, unsigned short port);

    bool CanRead(unsigned waitTimeoutMillis) const;
    bool CanWrite(unsigned waitTimeoutMillis) const;

private:
    bool Poll(bool readEvent, int timeout) const;
    bool GetSocketInfo(const SockAddr& addrStorage,
        std::string* outPeerAddress, unsigned short* outPeerPort, NetworkProtocol* outNetworkProtocol);

private:
    int m_socketId;
    NetworkProtocol m_netProtocol;
    bool m_nonBlocking;
};

class SockAddr
{
public:
    SockAddr();
    SockAddr(const sockaddr* addr, socklen_t addrSize);
    ~SockAddr() = default;

    void Init(const sockaddr* addr, socklen_t addrSize);
    bool IsSet() const;
    NetworkProtocol GetNetworkProtocol() const;
    const sockaddr* GetSockAddr() const;
    socklen_t GetSockAddrSize() const;
    sockaddr* GetSockAddrPtr();
    socklen_t* GetSockAddrSizePtr();

private:
    bool Init();

private:
    std::vector<char> m_sockaddrBuffer;
    socklen_t m_sockaddrSize;
};
