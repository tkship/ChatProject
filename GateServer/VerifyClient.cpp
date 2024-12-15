#include "VerifyClient.h"
#include "ConfigMgr.h"

#include "global.h"

GRPCVerifyConnectionPool::GRPCVerifyConnectionPool(int aPoolSize)
	: mIsStop(false)
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();
	std::string host = configMgr["VerifyServer"]["host"];
	std::string portStr = configMgr["VerifyServer"]["port"];
	std::string url = host + ":" + portStr;

	std::shared_ptr<Channel> channel = grpc::CreateChannel(
		url, grpc::InsecureChannelCredentials());

	for (int i = 0; i < aPoolSize; ++i)
	{
		mStubQueue.push(VerifyService::NewStub(channel));
	}
}

GRPCVerifyConnectionPool::~GRPCVerifyConnectionPool()
{
	Stop(); // Stop不做析构的事，而是将指示符置为关闭状态

	std::lock_guard<std::mutex> lg(mMtx);
	while (!mStubQueue.empty())
	{
		mStubQueue.pop();
	}

	std::cout << "~GRPCConnectionPool" << std::endl;
}

void GRPCVerifyConnectionPool::Stop()
{
	std::lock_guard<std::mutex> lg(mMtx);

	mIsStop = true;
	mCdv.notify_all();
}

std::unique_ptr<VerifyService::Stub> GRPCVerifyConnectionPool::GetRPCConnection()
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

	std::unique_ptr<VerifyService::Stub> ret = std::move(mStubQueue.front());
	mStubQueue.pop();
	return ret;
}

void GRPCVerifyConnectionPool::ReturnRPCConnection(std::unique_ptr<VerifyService::Stub>&& aStub)
{
	std::lock_guard<std::mutex> lg(mMtx);
	if (mIsStop)
	{
		return;
	}

	mStubQueue.push(std::move(aStub));
	mCdv.notify_one();
}

void VerifyClient::Stop()
{
	mConnectionPool.Stop();
}

VerifyClient& VerifyClient::GetInstance()
{
	static VerifyClient self;
	return self;
}

message::GetVerifyRsp VerifyClient::GetVerifyCode(const std::string& aEmail)
{
	ClientContext context;
	GetVerifyReq request;
	GetVerifyRsp response;
	
	request.set_email(aEmail);
	std::unique_ptr<VerifyService::Stub> stub = mConnectionPool.GetRPCConnection();
	Status status = stub->GetVerifyCode(&context, request, &response);
	mConnectionPool.ReturnRPCConnection(std::move(stub));

	if (!status.ok())
	{
		response.set_error(ErrorCode::GRPC_Failed);
	}
	
	return response;
}

