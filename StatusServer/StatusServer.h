#pragma once

#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

struct ChatServerMes
{
	std::string host;
	std::string port;
};

class StatusServer : public StatusService::Service
{
public:
	StatusServer();

	Status GetChatServer(ServerContext* context, const GetChatServerReq* aReq, GetChatServerRsp* aRsp) override;
	std::string GenerateToken();
private:
	// 管理所有ChatServer
	std::vector<ChatServerMes> mChatServerMes;
	// 暂时用index来选择ChatServer
	int mIndex;
};

