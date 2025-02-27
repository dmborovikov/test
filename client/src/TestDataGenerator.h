#pragma once

#include <string>
#include <vector>
#include <chrono>

class TestDataGenerator
{
private:
    typedef std::vector<std::vector<char>> Parts;

public:
    struct Package
    {
        std::string id;
        unsigned index;
        std::vector<char> data;
        bool received;
        std::chrono::time_point<std::chrono::steady_clock> sendTime;
    };

public:
    TestDataGenerator();
    std::vector<Package> Generate(const std::string& id, size_t numOfParts);
    unsigned GetChecksum() const;

private:
    Parts m_data;
    unsigned m_checksum;
};
