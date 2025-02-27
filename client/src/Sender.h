#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <map>
#include "TestDataGenerator.h"

class Sender
{
public:
    Sender();
    ~Sender();
    void Start(const std::string& address, unsigned short port);

private:
    void ThreadProc();
    static void ProcessResponse(const std::vector<char>& buffer, std::map<std::string, TestDataGenerator>& files, 
        std::vector<TestDataGenerator::Package>& packages);

private:
    std::string m_address;
    unsigned short m_port;

    std::thread m_thread;
    static std::atomic_int idCounter;
};
