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