#pragma once

#include "global.h"

#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/exception.h>

#include <queue>
#include <thread>
#include <mutex>

class MySQLConnection
{
public:
	MySQLConnection();
	~MySQLConnection();

	bool CreateConnection(const std::string& aHost, const std::string& aPort,
		const std::string& aUserName, const std::string& aPwd, const std::string& aDatabase);

	bool Insert(const std::string& aTableName, const std::string& aUserName, const std::string& aEmail, const std::string& aPwd);
	bool Exists(const std::string& aTableName, const std::string& aEmail);
	//bool Exists(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd);
	bool Update(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd);
	bool GetUserId(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd, int& oUserId);
	bool IsConnectionValid();
	
private:
	sql::mysql::MySQL_Driver* mDriver;
	sql::Connection* mConnection;
	sql::Statement* mStatement;
	sql::ResultSet* mRes;
};

class MySQLConnectionPool
{
public:
	MySQLConnectionPool(const std::string& aHost, const std::string& aPort,
		const std::string& aUserName, const std::string& aPwd, const std::string& aDatabase, int aPoolSize = 10);
	~MySQLConnectionPool();

	void Stop();
	void CheckConnections();

	std::unique_ptr<MySQLConnection> GetConnection();
	void ReturnConnection(std::unique_ptr<MySQLConnection>&& aConnection);

private:
	int mPoolSize;
	std::queue<std::unique_ptr<MySQLConnection>> mConnectionPool;
	std::mutex mMtx;
	std::condition_variable mCdv;
	std::unique_ptr<std::thread> mCheckThread;
	std::atomic<bool> mIsStop;


	std::string mHost;
	std::string mPort;
	std::string mUserName;
	std::string mPwd;
	std::string mDatabase; 
};

class MySQLMgr
{
public:
	MySQLMgr();
	~MySQLMgr();

	static MySQLMgr& GetInstance();

	void Stop();

	bool Insert(const std::string& aTableName, const std::string& aUserName, const std::string& aEmail, const std::string& aPwd);
	bool Exists(const std::string& aTableName, const std::string& aEmail);
	//bool Exists(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd);
	bool Update(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd);
	bool GetUserId(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd, int& oUserId);

private:
	std::unique_ptr<MySQLConnectionPool> mUserInfoDBConnectionsPool;
};

