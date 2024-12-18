#pragma once

#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <unordered_map>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

struct ChatServerInfo
{
	std::string host;
	std::string port;
};

class StatusServer : public StatusService::Service
{
public:
	StatusServer();

	Status GetChatServer(ServerContext* context, const GetChatServerReq* aReq, GetChatServerRsp* aRsp) override;
	Status Login(ServerContext* context, const LoginReq* aReq, LoginRsp* aRsp) override;
	std::string GenerateToken();
private:
	// 管理所有ChatServer
	std::vector<ChatServerInfo> mChatServerInfo;
	// 暂时用index来选择ChatServer
	int mIndex;

	std::unordered_map<int, std::string> mUserIdTokenMap;
	std::unordered_map<int, ChatServerInfo> mUserIdChatServerMap;
};

