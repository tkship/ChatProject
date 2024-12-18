#include "StatusClient.h"
#include "StatusClient.h"
#include "ConfigMgr.h"

#include "global.h"

GRPCStatusConnectionPool::GRPCStatusConnectionPool(int aPoolSize)
	: mIsStop(false)
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();
	std::string host = configMgr["StatusServer"]["host"];
	std::string portStr = configMgr["StatusServer"]["port"];
	std::string url = host + ":" + portStr;

	std::shared_ptr<Channel> channel = grpc::CreateChannel(
		url, grpc::InsecureChannelCredentials());

	for (int i = 0; i < aPoolSize; ++i)
	{
		mStubQueue.push(StatusService::NewStub(channel));
	}
}

GRPCStatusConnectionPool::~GRPCStatusConnectionPool()
{
	Stop(); // Stop不做析构的事，而是将指示符置为关闭状态

	std::lock_guard<std::mutex> lg(mMtx);
	while (!mStubQueue.empty())
	{
		mStubQueue.pop();
	}

	std::cout << "~GRPCConnectionPool" << std::endl;
}

void GRPCStatusConnectionPool::Stop()
{
	std::lock_guard<std::mutex> lg(mMtx);

	mIsStop = true;
	mCdv.notify_all();
}

std::unique_ptr<StatusService::Stub> GRPCStatusConnectionPool::GetRPCConnection()
{
	std::unique_lock<std::mutex> ul(mMtx);
	mCdv.wait(ul, [this]() {
		if (mStubQueue.empty() && !mIsStop)
		{
			return false;
		}

		return true;
		});

	if (mIsStop)
	{
		return nullptr;
	}

	std::unique_ptr<StatusService::Stub> ret = std::move(mStubQueue.front());
	mStubQueue.pop();
	return ret;
}

void GRPCStatusConnectionPool::ReturnRPCConnection(std::unique_ptr<StatusService::Stub>&& aStub)
{
	std::lock_guard<std::mutex> lg(mMtx);
	if (mIsStop)
	{
		return;
	}

	mStubQueue.push(std::move(aStub));
	mCdv.notify_one();
}

void StatusClient::Stop()
{
	mConnectionPool.Stop();
}

StatusClient& StatusClient::GetInstance()
{
	static StatusClient self;
	return self;
}

message::LoginRsp StatusClient::Login(int aUid, const std::string& aToken)
{
	ClientContext context;
	LoginReq request;
	LoginRsp response;

	request.set_uid(aUid);
	request.set_token(aToken);
	std::unique_ptr<StatusService::Stub> stub = mConnectionPool.GetRPCConnection();
	Status status = stub->Login(&context, request, &response);
	mConnectionPool.ReturnRPCConnection(std::move(stub));
	if (!status.ok())
	{
		response.set_error(ErrorCode::GRPC_Failed);
	}

	return response;
}