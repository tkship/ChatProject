#include "StatusServer.h"

#include "ConfigMgr.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

StatusServer::StatusServer()
	: mIndex(0)
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();
	std::string host1 = configMgr["ChatServer1"]["host"];
	std::string port1 = configMgr["ChatServer1"]["port"];
	std::string host2 = configMgr["ChatServer1"]["host"];
	std::string port2 = configMgr["ChatServer2"]["port"];
	mChatServerMes.push_back({ host1, port1 });
	mChatServerMes.push_back({ host2, port2 });
}

Status StatusServer::GetChatServer(ServerContext* context, const GetChatServerReq* aReq, GetChatServerRsp* aRsp)
{
	mIndex = (mIndex++) % mChatServerMes.size();
	ChatServerMes& chosenChatServer = mChatServerMes[mIndex];

	aRsp->set_error(ErrorCode::Success);
	aRsp->set_host(chosenChatServer.host);
	aRsp->set_port(chosenChatServer.port);
	aRsp->set_token(GenerateToken());

	return Status::OK;
}

std::string StatusServer::GenerateToken()
{
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	return boost::uuids::to_string(uuid);
}
