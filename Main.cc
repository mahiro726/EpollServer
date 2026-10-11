#include "EpollServer.hpp"
#include <iostream>
#include <memory>

using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        cout << "Usage: " << argv[0] << " port" << endl;
        return 1;
    }
    
    ENABLE_CONSOLE_LOG_STRATEGY();

    uint16_t server_port = std::stoi(argv[1]);

    std::unique_ptr<EpollServer> svr(new EpollServer(server_port));

    svr->Loop();
}