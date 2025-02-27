#pragma once

#include <atomic>
#include <string>
#include "RequestHandler.h"

class UdpServer
{
public:
    UdpServer();
    ~UdpServer();
    void Start(const std::string& address, unsigned short port);
    void Stop();

private:
    std::atomic_bool m_stop;
    RequestHandler m_handler;
};
