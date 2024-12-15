#include "StatusServer.h"
#include "ConfigMgr.h"

int main()
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();

	std::string serverAddress = configMgr["StatusServer"]["host"] + ":" + configMgr["StatusServer"]["port"];
	StatusServer statusServer;

	grpc::ServerBuilder builder;
	
	// grpc服务 监听端口
	builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
	builder.RegisterService(&statusServer);

	// 启动grpc服务器
	std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
	if (!server)
	{
		std::cout << "StatusServer start failed" << std::endl;
		return -1;
	}

	std::cout << "StatusServer is running" << std::endl;

	// 阻塞等待
	server->Wait();

	return 0;
}