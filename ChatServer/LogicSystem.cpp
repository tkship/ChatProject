#include "LogicSystem.h"
#include "ChatServer.h"
#include "RedisMgr.h"
#include "StatusClient.h"

#include <iostream>

LogicSystem::LogicSystem()
{
	RegisterHandler(Chat_Login, [](const Json::Value& aRoot, std::shared_ptr<Session> aSession) {
		Json::Value retRoot;
		Defer defer([&retRoot, aSession]() {
			aSession->Send(retRoot.toStyledString());
		});

		std::string uid = aRoot["uid"].asString();
		std::string token = aRoot["token"].asString();

		std::string cachedToken;
		if (!RedisMgr::GetInstance().Get(uid, cachedToken))
		{
			retRoot["error"] = Server_Failed;
			return;
		}

		if (cachedToken.empty() || cachedToken != token)
		{
			retRoot["error"] = Chat_InvalidUser;
			return;
		}

		// 将Token写入StatusServer
		LoginRsp loginRsp = StatusClient::GetInstance().Login(std::atoi(uid.c_str()), token);
		retRoot["error"] = loginRsp.error();
	});
}

LogicSystem::~LogicSystem()
{
}

LogicSystem& LogicSystem::GetInstance()
{
	static LogicSystem self;
	return self;
}

void LogicSystem::HandleMsg(std::shared_ptr<Session> aSession)
{
	// 解析Json
	Json::Value root;
	Json::Reader reader;
	Json::Value retRoot;
	if (!reader.parse(aSession->GetRecvData(), root))
	{
		retRoot["error"] = Json_ParseError;
		aSession->Send(retRoot.toStyledString());
		return;
	}

	if (!root.isMember("msgId"))
	{
		retRoot["error"] = Json_ParseError;
		aSession->Send(retRoot.toStyledString());
		return;
	}

	MsgId msgId = (MsgId)root["msgId"].asInt();
	if (mCallBackMap.find(msgId) == mCallBackMap.end())
	{
		retRoot["error"] = Chat_InvalidMsgId;
		aSession->Send(retRoot.toStyledString());
		return;
	}

	mCallBackMap[msgId](root, aSession);
}

void LogicSystem::RegisterHandler(MsgId aMsgId, CallBackFunc aFunc)
{
	if (mCallBackMap.find(aMsgId) != mCallBackMap.end())
	{
		assert("重复MesId注册");
	}

	mCallBackMap[aMsgId] = aFunc;
}
