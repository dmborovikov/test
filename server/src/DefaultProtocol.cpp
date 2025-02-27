#include "DefaultProtocol.h"
#include "Crc.h"
#include <cstring>
#include <iostream>

DefaultProtocol::DefaultProtocol()
    : m_lastUpdateTime(std::chrono::steady_clock::now())
{
}

std::vector<char> DefaultProtocol::Process(const std::vector<char>& buffer)
{
    static constexpr unsigned char ACK = 0;
    static constexpr unsigned char PUT = 1;
    static constexpr size_t ID_SIZE = 8;
    static constexpr size_t HEADER_SIZE =
        sizeof(unsigned) +      // seq_number
        sizeof(unsigned) +      // seq_total
        sizeof(unsigned char) + // type
        ID_SIZE * sizeof(char); // id

    std::vector<char> response;

    if (buffer.size() > HEADER_SIZE)
    {
        Package package;

        const char* ptr = buffer.data();

        memcpy(&package.seq_number, ptr, sizeof(unsigned));
        ptr += sizeof(unsigned);

        unsigned seq_total;
        memcpy(&seq_total, ptr, sizeof(unsigned));
        ptr += sizeof(unsigned);

        unsigned char type;
        memcpy(&type, ptr, sizeof(unsigned char));
        ptr += sizeof(unsigned char);

        if (type == PUT)
        {
            char id[ID_SIZE];
            memcpy(id, ptr, ID_SIZE);
            ptr += ID_SIZE;

            const size_t dataSize = buffer.size() - HEADER_SIZE;

            package.data.resize(dataSize);
            memcpy(package.data.data(), ptr, dataSize);

            const std::string fileId(id, ID_SIZE);

            auto& currentPackages = m_packages[fileId];
            currentPackages.insert(package);

            std::cout << "Received: id: " << fileId << ", seq_number: " << package.seq_number << std::endl;

            /**** Creating response ****/

            const unsigned packagesCount = currentPackages.size();
            const bool isLastPackage = packagesCount == seq_total;
            response.resize(isLastPackage ? HEADER_SIZE + sizeof(unsigned) : HEADER_SIZE);

            char* ptr = response.data();

            memcpy(ptr, &package.seq_number, sizeof(unsigned));
            ptr += sizeof(unsigned);

            memcpy(ptr, &packagesCount, sizeof(unsigned));
            ptr += sizeof(unsigned);

            memcpy(ptr, &ACK, sizeof(unsigned char));
            ptr += sizeof(unsigned char);

            memcpy(ptr, id, ID_SIZE);
            ptr += ID_SIZE;

            if (isLastPackage)
            {
                unsigned checksum = 0;

                for (const auto& package : currentPackages)
                {
                    checksum = crc32c(checksum, (const unsigned char*)package.data.data(), package.data.size());
                }

                std::cout << "CRC: " << checksum << ", id: " << fileId << std::endl;

                m_packages.erase(fileId);
                memcpy(ptr, &checksum, sizeof(unsigned));
            }
        }
    }

    m_lastUpdateTime = std::chrono::steady_clock::now();

    return response;
}

bool DefaultProtocol::IsEmpty()
{
    return m_packages.empty();
}

bool DefaultProtocol::IsExpired()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_lastUpdateTime).count() > 10;
}

bool DefaultProtocol::Package::operator<(const DefaultProtocol::Package& other) const
{
    return seq_number < other.seq_number;
}
