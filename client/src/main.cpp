#include <ctime>
#include "Sender.h"

int main()
{
    srand(time(NULL));

    Sender sender;
    sender.Start("127.0.0.1", 8865);

    return 0;
}
