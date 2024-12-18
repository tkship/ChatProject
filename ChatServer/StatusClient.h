#pragma once

#include "global.h"
#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <queue>

using grpc::ClientContext;
using grpc::Channel;
using grpc::Status;

using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

// ��������д��ģ��
class GRPCStatusConnectionPool
{
public:
	GRPCStatusConnectionPool(int aPoolSize = 10);
	~GRPCStatusConnectionPool();

	void Stop();

	std::unique_ptr<StatusService::Stub> GetRPCConnection();
	void ReturnRPCConnection(std::unique_ptr<StatusService::Stub>&& aStub);
private:
	std::queue<std::unique_ptr<StatusService::Stub>> mStubQueue;
	std::mutex mMtx;
	std::condition_variable mCdv;

	bool mIsStop;
};

class StatusClient
{
public:
	void Stop();

	static StatusClient& GetInstance();
	message::LoginRsp Login(int aUid, const std::string& aToken);

private:
	GRPCStatusConnectionPool mConnectionPool;
};

