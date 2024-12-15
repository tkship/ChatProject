#include "VarifyClient.h"
#include "ConfigMgr.h"

#include "global.h"

GRPCConnectionPool::GRPCConnectionPool(int aPoolSize)
	: mIsStop(false)
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();
	std::string host = configMgr["VarifyServer"]["host"];
	std::string portStr = configMgr["VarifyServer"]["port"];
	std::string url = host + ":" + portStr;

	std::shared_ptr<Channel> channel = grpc::CreateChannel(
		url, grpc::InsecureChannelCredentials());

	for (int i = 0; i < aPoolSize; ++i)
	{
		mStubQueue.push(VarifyService::NewStub(channel));
	}
}

GRPCConnectionPool::~GRPCConnectionPool()
{
	Stop(); // Stop不做析构的事，而是将指示符置为关闭状态

	std::lock_guard<std::mutex> lg(mMtx);
	while (!mStubQueue.empty())
	{
		mStubQueue.pop();
	}

	std::cout << "~GRPCConnectionPool" << std::endl;
}

void GRPCConnectionPool::Stop()
{
	std::lock_guard<std::mutex> lg(mMtx);

	mIsStop = true;
	mCdv.notify_all();
}

std::unique_ptr<VarifyService::Stub> GRPCConnectionPool::GetRPCConnection()
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

	std::unique_ptr<VarifyService::Stub> ret = std::move(mStubQueue.front());
	mStubQueue.pop();
	return ret;
}

void GRPCConnectionPool::ReturnRPCConnection(std::unique_ptr<VarifyService::Stub>&& aStub)
{
	std::lock_guard<std::mutex> lg(mMtx);
	if (mIsStop)
	{
		return;
	}

	mStubQueue.push(std::move(aStub));
	mCdv.notify_one();
}

void VarifyClient::Stop()
{
	mConnectionPool.Stop();
}

VarifyClient& VarifyClient::GetInstance()
{
	static VarifyClient self;
	return self;
}

message::GetVarifyRsp VarifyClient::GetVarifyCode(std::string aEmail)
{
	ClientContext context;
	GetVarifyReq request;
	GetVarifyRsp response;
	
	request.set_email(aEmail);
	std::unique_ptr<VarifyService::Stub> stub = mConnectionPool.GetRPCConnection();
	Status status = stub->GetVarifyCode(&context, request, &response);
	mConnectionPool.ReturnRPCConnection(std::move(stub));

	if (!status.ok())
	{
		response.set_error(ErrorCode::GRPC_Failed);
	}
	
	return response;
}

