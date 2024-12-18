#pragma once

#include "hiredis/hiredis.h"

#include <string>
#include <queue>
#include <mutex>

class RedisConnectionPool
{
public:
	RedisConnectionPool(const std::string& aHost, int aPort, const std::string& aPwd, size_t aPoolSize = 10);
	~RedisConnectionPool();

	void Stop();

	redisContext* GetRedisConnection();
	void ReturnRedisConnection(redisContext* aContext);
private:
	std::queue<redisContext*> mRedisConnections;
	std::mutex mMtx;
	std::condition_variable mCdv;
	bool mIsStop;
};

class RedisMgr
{
public:
	void Stop();
	static RedisMgr& GetInstance();

	bool Get(const std::string& aKey, std::string& oValue);
	bool Set(const std::string& aKey, const std::string& aValue);
	bool SetEX(const std::string& aKey, const std::string& aValue, int aExpireTime);
	bool Auth(const std::string& aPassword);
	bool LPush(const std::string& aKey, const std::string& aValue);
	bool LPop(const std::string& aKey, std::string& oValue);
	bool RPush(const std::string& aKey, const std::string& aValue);
	bool RPop(const std::string& aKey, std::string& oValue);
	bool HSet(const std::string& aKey, const std::string& aHKey, const std::string& aValue);
	bool HGet(const std::string& aKey, const std::string& aHKey, std::string& oValue);
	bool Del(const std::string& aKey);
	bool ExistsKey(const std::string& aKey);
private:
	RedisMgr();
	~RedisMgr();
private:
	std::unique_ptr<RedisConnectionPool> mRedisConnectionPool;
};

