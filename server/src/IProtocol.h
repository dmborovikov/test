#pragma once

#include <vector>

class IProtocol
{
public:
    virtual ~IProtocol() = default;
    virtual std::vector<char> Process(const std::vector<char>& data) = 0;
    virtual bool IsEmpty() = 0;
    virtual bool IsExpired() = 0;
};
