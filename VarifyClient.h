#pragma once

#include "global.h"
#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <queue>

using grpc::ClientContext;
using grpc::Channel;
using grpc::Status;

using message::GetVarifyRsp;
using message::GetVarifyReq;
using message::VarifyService;

class GRPCConnectionPool
{
public:
	GRPCConnectionPool(int aPoolSize = 10);
	~GRPCConnectionPool();

	void Stop();

	std::unique_ptr<VarifyService::Stub> GetRPCConnection();
	void ReturnRPCConnection(std::unique_ptr<VarifyService::Stub>&& aStub);
private:
	std::queue<std::unique_ptr<VarifyService::Stub>> mStubQueue;
	std::mutex mMtx;
	std::condition_variable mCdv;

	bool mIsStop;
};

class VarifyClient
{
public:
	void Stop();

	static VarifyClient& GetInstance();
	message::GetVarifyRsp GetVarifyCode(std::string aEmail);

private:
	GRPCConnectionPool mConnectionPool;
};

