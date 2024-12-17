#include "StatusServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"

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
	mIndex = (mIndex+1) % mChatServerMes.size();
	ChatServerMes& chosenChatServer = mChatServerMes[mIndex];

	// 此时处在登陆的最后一步
	// 先将token设置进redis，定时过期
	// 如果client请求chatserver的login请求携带的token在redis里，就说明是合法的
	// 这时再由chatserver请求statusserver将token写入map中
	// 这样会比在这里直接将token写入map中要好
	// 因为statusServer是维护的在线的用户，而此时的确还没有和chatserver建立连接，也就不是真正的在线用户
	// 而且离线时的逻辑也会好写一些，不用处理cli与chatserver连接失败还要删除map中元素的逻辑
	std::string uid = std::to_string(aReq->uid());
	std::string token = GenerateToken();
	RedisMgr::GetInstance().SetEX(uid, token, 20);

	aRsp->set_error(ErrorCode::Success);
	aRsp->set_host(chosenChatServer.host);
	aRsp->set_port(chosenChatServer.port);
	aRsp->set_token(token);

	return Status::OK;
}

std::string StatusServer::GenerateToken()
{
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	return boost::uuids::to_string(uuid);
}
