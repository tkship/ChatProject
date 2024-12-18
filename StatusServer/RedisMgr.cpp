#include "RedisMgr.h"

#include "ConfigMgr.h"

#include <iostream>

RedisConnectionPool::RedisConnectionPool(const std::string& aHost, int aPort, const std::string& aPwd, size_t aPoolSize)
	: mIsStop(false)
{
	for (size_t i = 0; i < aPoolSize; ++i)
	{
		redisContext* context = redisConnect(aHost.c_str(), aPort);
		if (context->err)
		{
			std::cout << "Redis链接池[" << i << "]号链接尝试连接失败" << std::endl;
			redisFree(context);
			continue;
		}

		redisReply* reply = (redisReply*)redisCommand(context, "AUTH %s", aPwd.c_str());
		if (reply->type == REDIS_REPLY_ERROR)
		{
			std::cout << "Redis链接池[" << i << "]号链接认证失败" << std::endl;
			freeReplyObject(reply);
			redisFree(context);
			continue;
		}

		freeReplyObject(reply);
		mRedisConnections.push(context);

		std::cout << "Redis链接池[" << i << "]号链接连接成功" << std::endl;
	}
}

RedisConnectionPool::~RedisConnectionPool()
{
	Stop();

	std::lock_guard<std::mutex> lg(mMtx);
	while (!mRedisConnections.empty())
	{
		redisContext* context = mRedisConnections.front();
		redisFree(context);
		mRedisConnections.pop();
	}
}

void RedisConnectionPool::Stop()
{
	std::lock_guard<std::mutex> lg(mMtx);

	mIsStop = true;
	mCdv.notify_all();
}

redisContext* RedisConnectionPool::GetRedisConnection()
{
	std::unique_lock<std::mutex> ul(mMtx);
	mCdv.wait(ul, [this]() {
		if (mRedisConnections.empty() && !mIsStop)
		{
			return false;
		}

		return true;
	});

	if (mIsStop)
	{
		return nullptr;
	}

	redisContext* context = mRedisConnections.front();
	mRedisConnections.pop();

	return context;
}

void RedisConnectionPool::ReturnRedisConnection(redisContext* aContext)
{
	std::lock_guard<std::mutex> lg(mMtx);

	mRedisConnections.push(aContext);
	mCdv.notify_one();
}

RedisMgr::RedisMgr()
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();
	std::string host = configMgr["RedisServer"]["host"];
	int port = atoi(configMgr["RedisServer"]["port"].c_str());
	std::string password = configMgr["RedisServer"]["password"];

	mRedisConnectionPool.reset(new RedisConnectionPool(host, port, password));
}

RedisMgr::~RedisMgr()
{
}

void RedisMgr::Stop()
{
	mRedisConnectionPool->Stop();
}

RedisMgr& RedisMgr::GetInstance()
{
	static RedisMgr self;
	return self;
}

bool RedisMgr::Get(const std::string& aKey, std::string& oValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "GET %s", aKey.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ GET " << aKey << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type == REDIS_REPLY_NIL)
	{
		// 没找到对应的key
		oValue = "";
		return true;
	}

	if (reply->type != REDIS_REPLY_STRING)
	{
		std::cout << "Execute Command [ GET " << aKey << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	oValue = reply->str;
	freeReplyObject(reply);
	std::cout << "Execute Command [ GET " << aKey << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::Set(const std::string& aKey, const std::string& aValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "SET %s %s", aKey.c_str(), aValue.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ SET " << aKey << " " << aValue << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_STATUS || (strcmp(reply->str, "OK") != 0 && strcmp(reply->str, "ok") != 0))
	{
		std::cout << "Execute Command [ SET " << aKey << " " << aValue << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ SET " << aKey << " " << aValue << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::SetEX(const std::string& aKey, const std::string& aValue, int aExpireTime)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "SETEX %s %d %s", aKey.c_str(), aExpireTime, aValue.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ SETEX " << aKey << " " << aExpireTime << " " << aValue << "] Failed" << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_STATUS || (strcmp(reply->str, "OK") != 0 && strcmp(reply->str, "ok") != 0))
	{
		std::cout << "Execute Command [ SETEX " << aKey << " " << aExpireTime << " " << aValue << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ SET " << aKey << " " << aValue << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::Auth(const std::string& aPassword)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "AUTH %s", aPassword.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ AUTH " << aPassword << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type == REDIS_REPLY_ERROR)
	{
		std::cout << "Execute Command [ AUTH " << aPassword << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ AUTH " << aPassword << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::LPush(const std::string& aKey, const std::string& aValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "LPUSH %s %s", aKey.c_str(), aValue.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ LPUSH " << aKey << " " << aValue << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0)
	{
		std::cout << "Execute Command [ LPUSH " << aKey << " " << aValue << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ LPUSH " << aKey << " " << aValue << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::LPop(const std::string& aKey, std::string& oValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "LPOP %s", aKey.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ LPOP " << aKey << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type == REDIS_REPLY_NIL)
	{
		std::cout << "Execute Command [ LPOP " << aKey << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	oValue = reply->str;
	freeReplyObject(reply);
	std::cout << "Execute Command [ LPOP " << aKey << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::RPush(const std::string& aKey, const std::string& aValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "RPUSH %s %s", aKey.c_str(), aValue.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ RPUSH " << aKey << " " << aValue << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0)
	{
		std::cout << "Execute Command [ RPUSH " << aKey << " " << aValue << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ RPUSH " << aKey << " " << aValue << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::RPop(const std::string& aKey, std::string& oValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "RPOP %s", aKey.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ RPOP " << aKey << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type == REDIS_REPLY_NIL)
	{
		std::cout << "Execute Command [ RPOP " << aKey << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	oValue = reply->str;
	freeReplyObject(reply);
	std::cout << "Execute Command [ RPOP " << aKey << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::HSet(const std::string& aKey, const std::string& aHKey, const std::string& aValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "HSET %s %s %s", aKey.c_str(), aHKey.c_str(), aValue.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ HSET " << aKey << " " << aHKey << " " << aValue << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER)
	{
		std::cout << "Execute Command [ HSET " << aKey << " " << aHKey << " " << aValue << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ HSET " << aKey << " " << aHKey << " " << aValue << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::HGet(const std::string& aKey, const std::string& aHKey, std::string& oValue)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "HGET %s %s", aKey.c_str(), aHKey.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ HGET " << aKey << " " << aHKey << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type == REDIS_REPLY_NIL)
	{
		std::cout << "Execute Command [ HGET " << aKey << " " << aHKey << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	oValue = reply->str;
	freeReplyObject(reply);
	std::cout << "Execute Command [ HGET " << aKey << " " << aHKey << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::Del(const std::string& aKey)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "DEL %s", aKey.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ DEL " << aKey << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER)
	{
		std::cout << "Execute Command [ DEL " << aKey << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ DEL " << aKey << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}

bool RedisMgr::ExistsKey(const std::string& aKey)
{
	redisContext* context = mRedisConnectionPool->GetRedisConnection();
	redisReply* reply = (redisReply*)redisCommand(context, "EXISTS %s", aKey.c_str());
	if (reply == nullptr)
	{
		std::cout << "Execute Command [ EXISTS " << aKey << " ] Failed" << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer == 0)
	{
		std::cout << "Execute Command [ EXISTS " << aKey << " ] Failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute Command [ EXISTS " << aKey << " ] Successed" << std::endl;

	mRedisConnectionPool->ReturnRedisConnection(context);
	return true;
}
