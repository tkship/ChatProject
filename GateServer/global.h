#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <cassert>

enum ErrorCode
{
	Success = 0,
	GRPC_Failed = 1001,
	Redis_KeyNotFound = 1002,
	VerifyCodeNotMatch = 1003,
	MySQL_UserExist = 1004,
	MySQL_Error = 1005,
	MySQL_UserNotExist = 1006,
	MySQL_UserNotMatch = 1007,
	Json_ParseError = 1008,
	Server_Failed = 1009, // 加一个笼统的错误码表示服务器某处崩溃了

	Chat_InvalidMsgId = 2001,
	Chat_InvalidUser = 2002,
};

class Defer
{
	typedef  std::function<void()> DeferFunction;
public:
	Defer(DeferFunction aDeferFunction)
		: mDeferFunction(aDeferFunction)
	{}

	~Defer()
	{
		mDeferFunction();
	}
private:
	DeferFunction mDeferFunction;
};