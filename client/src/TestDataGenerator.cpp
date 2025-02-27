#include "TestDataGenerator.h"
#include "Crc.h"
#include <random>
#include <climits>
#include <algorithm>
#include <functional>
#include <cstring>

TestDataGenerator::TestDataGenerator()
    : m_checksum(0)
{
}

std::vector<TestDataGenerator::Package> TestDataGenerator::Generate(const std::string& id, size_t numOfParts)
{
    std::vector<Package> result;

    m_data.clear();
    m_checksum = 0;

    for (size_t i = 0; i < numOfParts; ++i)
    {
        const size_t rndPartSize = 10 + rand() % 777;
        std::vector<char> part(rndPartSize);

        for (auto& it : part)
        {
            it = rand() % 255;
        }

        m_checksum = crc32c(m_checksum, (unsigned char*)part.data(), part.size());
        m_data.push_back(part);
    }

    static constexpr unsigned char PUT = 1;
    static constexpr size_t ID_SIZE = 8;
    static constexpr size_t HEADER_SIZE =
        sizeof(unsigned) +      // seq_number
        sizeof(unsigned) +      // seq_total
        sizeof(unsigned char) + // type
        ID_SIZE * sizeof(char); // id

    const unsigned size = m_data.size();

    for (unsigned i = 0; i < size; ++i)
    {
        std::vector<char> package(m_data[i].size() + HEADER_SIZE);
        auto pos = package.data();

        memcpy(pos, &i, sizeof(unsigned));
        pos += sizeof(unsigned);

        memcpy(pos, &size, sizeof(unsigned));
        pos += sizeof(unsigned);

        memcpy(pos, &PUT, sizeof(unsigned char));
        pos += sizeof(unsigned char);

        memset(pos, 0, ID_SIZE);
        memcpy(pos, id.data(), std::min(id.size(), ID_SIZE));
        pos += ID_SIZE;

        memcpy(pos, m_data[i].data(), m_data[i].size());

        result.push_back({ id, i, package, false, std::chrono::time_point<std::chrono::steady_clock>() });
    }

    m_data.clear();

    return result;
}

unsigned TestDataGenerator::GetChecksum() const
{
    return m_checksum;
}
