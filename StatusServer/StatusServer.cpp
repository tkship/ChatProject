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

	// ��ʱ���ڵ�½�����һ��
	// �Ƚ�token���ý�redis����ʱ����
	// ���client����chatserver��login����Я����token��redis���˵���ǺϷ���
	// ��ʱ����chatserver����statusserver��tokenд��map��
	// �������������ֱ�ӽ�tokenд��map��Ҫ��
	// ��ΪstatusServer��ά�������ߵ��û�������ʱ��ȷ��û�к�chatserver�������ӣ�Ҳ�Ͳ��������������û�
	// ��������ʱ���߼�Ҳ���дһЩ�����ô���cli��chatserver����ʧ�ܻ�Ҫɾ��map��Ԫ�ص��߼�
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
