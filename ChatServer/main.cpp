#include "ChatServer.h"
#include "ConfigMgr.h"

int main()
{
    

    ConfigMgr& configMgr = ConfigMgr::GetInstance();
    std::string portStr = configMgr["ChatServer1"]["port"];
    unsigned short port = atoi(portStr.c_str());

    boost::asio::io_service service;
    ChatServer server(port, service);

    service.run();

    return 0;
}