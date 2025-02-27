#pragma once

#include "IProtocol.h"

#include <map>
#include <set>
#include <vector>
#include <string>
#include <chrono>

class DefaultProtocol : public IProtocol
{
public:
    DefaultProtocol();
    
    virtual std::vector<char> Process(const std::vector<char>& data) override;
    bool IsEmpty() override;
    bool IsExpired() override;

private:
    struct Package
    {
        unsigned seq_number;
        std::vector<char> data;

        bool operator<(const Package& other) const;
    };

    std::map<std::string/*fileId*/, std::set<Package>> m_packages;
    std::chrono::time_point<std::chrono::steady_clock> m_lastUpdateTime;
};
