#include "StatusServer.h"
#include "ConfigMgr.h"

int main()
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();

	std::string serverAddress = configMgr["StatusServer"]["host"] + ":" + configMgr["StatusServer"]["port"];
	StatusServer statusServer;

	grpc::ServerBuilder builder;
	
	// grpc���� �����˿�
	builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
	builder.RegisterService(&statusServer);

	// ����grpc������
	std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
	if (!server)
	{
		std::cout << "StatusServer start failed" << std::endl;
		return -1;
	}

	std::cout << "StatusServer is running" << std::endl;

	// �����ȴ�
	server->Wait();

	return 0;
}