#pragma once

#include "global.h"
#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <queue>

using grpc::ClientContext;
using grpc::Channel;
using grpc::Status;

using message::GetVerifyRsp;
using message::GetVerifyReq;
using message::VerifyService;

class GRPCVerifyConnectionPool
{
public:
	GRPCVerifyConnectionPool(int aPoolSize = 10);
	~GRPCVerifyConnectionPool();

	void Stop();

	std::unique_ptr<VerifyService::Stub> GetRPCConnection();
	void ReturnRPCConnection(std::unique_ptr<VerifyService::Stub>&& aStub);
private:
	std::queue<std::unique_ptr<VerifyService::Stub>> mStubQueue;
	std::mutex mMtx;
	std::condition_variable mCdv;

	bool mIsStop;
};

class VerifyClient
{
public:
	void Stop();

	static VerifyClient& GetInstance();
	message::GetVerifyRsp GetVerifyCode(const std::string& aEmail);

private:
	GRPCVerifyConnectionPool mConnectionPool;
};

