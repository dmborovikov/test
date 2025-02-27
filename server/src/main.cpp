#include "UdpServer.h"

int main()
{
    UdpServer server;
    server.Start("127.0.0.1", 8865);

    return 0;
}
