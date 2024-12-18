#pragma once
#include "global.h"

#include <json/json.h>

#include <unordered_map>

// WebSocket«Î«ÛChatServer

class Session;
class LogicSystem
{
	enum MsgId
	{
		Chat_Login = 0,
	};

	typedef std::function<void(const Json::Value&, std::shared_ptr<Session>)> CallBackFunc;

public:
	LogicSystem();
	~LogicSystem();
	
	static LogicSystem& GetInstance();

	void HandleMsg(std::shared_ptr<Session> aSession);

private:
	void RegisterHandler(MsgId aMsgId, CallBackFunc aFunc);

private:
	std::unordered_map<MsgId, CallBackFunc> mCallBackMap;
};
