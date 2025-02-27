#include "Sender.h"
#include "TestDataGenerator.h"
#include "UdpSocket.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>

const size_t NUMBER_OF_FILES = 3;
const size_t BUF_SIZE = 1472;
std::atomic_int Sender::idCounter(0);

Sender::Sender()
    : m_port(0)
{
}

Sender::~Sender()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void Sender::Start(const std::string& address, unsigned short port)
{
    m_address = address;
    m_port = port;
    m_thread = std::thread([this] { ThreadProc(); });
}

void Sender::ThreadProc()
{
    std::map<std::string, TestDataGenerator> files;
    std::vector<TestDataGenerator::Package> packages;

    for (size_t i = 0; i < NUMBER_OF_FILES; ++i)
    {
        const std::string id(("file" + std::to_string(idCounter++)).c_str(), 8);
        auto currentPackages = files[id].Generate(id, 20);
        packages.insert(packages.end(), currentPackages.begin(), currentPackages.end());
    }

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(packages.begin(), packages.end(), std::default_random_engine(seed));

    UdpSocket socket(NetworkProtocol::IPv4, true);

    while (std::any_of(packages.begin(), packages.end(), [](const TestDataGenerator::Package& package) { return !package.received; }))
    {
        for (auto& package : packages)
        {
            // There's an issue here:
            // If the server sends a packet with a checksum, but we do not receive it, 
            // then we will resend only the last packet, but the server has already deleted the rest of the packets from itself,
            // so we need to implement resending all packets again or use some other logic...
            if (!package.received && 
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()- package.sendTime).count() > 3 &&
                socket.CanWrite(0))
            {
                socket.Write((char*)package.data.data(), package.data.size(), m_address, m_port);
                package.sendTime = std::chrono::steady_clock::now();
            }
        }

        if (socket.CanRead(1000))
        {
            std::vector<char> buffer(BUF_SIZE);
            const int bytesRead = socket.Read(buffer.data(), buffer.size(), nullptr, nullptr);

            if (bytesRead > 0)
            {
                buffer.resize(bytesRead);
                ProcessResponse(buffer, files, packages);
            }
        }
    }
}

void Sender::ProcessResponse(const std::vector<char>& buffer, std::map<std::string, TestDataGenerator>& files
    , std::vector<TestDataGenerator::Package>& packages)
{
    static constexpr char ACK = 0;
    static constexpr size_t ID_SIZE = 8;
    static constexpr size_t HEADER_SIZE =
        sizeof(unsigned) +      // seq_number
        sizeof(unsigned) +      // seq_total
        sizeof(unsigned char) + // type
        ID_SIZE * sizeof(char); // id

    if (buffer.size() >= HEADER_SIZE)
    {
        const char* ptr = buffer.data();

        unsigned seq_number;
        memcpy(&seq_number, ptr, sizeof(unsigned));
        ptr += sizeof(unsigned);

        unsigned seq_total;
        memcpy(&seq_total, ptr, sizeof(unsigned));
        ptr += sizeof(unsigned);

        unsigned char type;
        memcpy(&type, ptr, sizeof(unsigned char));
        ptr += sizeof(unsigned char);

        if (type == ACK)
        {
            char id[ID_SIZE];
            memcpy(id, ptr, ID_SIZE);
            ptr += ID_SIZE;

            std::string fileId(id, ID_SIZE);

            std::cout << "ACK: id: " << fileId << ", seq_number: " << seq_number << std::endl;

            unsigned checksum = 0;

            if (buffer.size() == HEADER_SIZE + sizeof(unsigned))
            {
                memcpy(&checksum, ptr, sizeof(unsigned));
                std::cout << "CRC from Server: " << checksum << ", original: " << files[fileId].GetChecksum() << ", id: " << fileId << std::endl;
            }

            for (auto& package : packages)
            {
                if (package.id == fileId && package.index == seq_number)
                {
                    package.received = true;
                }
            }
        }
    }
}
