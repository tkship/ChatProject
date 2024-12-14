#include "MySQLMgr.h"
#include "ConfigMgr.h"

MySQLConnection::MySQLConnection()
	: mDriver(nullptr)
	, mConnection(nullptr)
	, mStatement(nullptr)
	, mRes(nullptr)
{
}

MySQLConnection::~MySQLConnection()
{
	delete mRes;
	delete mStatement;
	delete mConnection;
}

bool MySQLConnection::CreateConnection(const std::string& aHost, const std::string& aPort,
	const std::string& aUserName, const std::string& aPwd, const std::string& aDatabase)
{
	mDriver = sql::mysql::get_driver_instance();
	if (!mDriver)
	{
		std::cout << "MySQLDriver创建失败" << std::endl;
		return false;
	}

	try
	{
		mConnection = mDriver->connect(aHost + ":" + aPort, aUserName, aPwd);
	}
	catch (sql::SQLException e)
	{
		std::cout << e.what() << std::endl;
	}
	
	if (!mConnection)
	{
		std::cout << "MySQLConnection创建失败" << std::endl;
		return false;
	}

	mConnection->setSchema(aDatabase);

	mStatement = mConnection->createStatement();
	if (!mStatement)
	{
		std::cout << "MySQLmStatement创建失败" << std::endl;
		return false;
	}

	return true;
}

bool MySQLConnection::Insert(const std::string& aTableName, const std::string& aUserName, const std::string& aEmail, const std::string& aPwd)
{
	try
	{
		std::string command = "insert into `" + aTableName + "` (`UserName`, `Email`, `Password`) values("
			+ "\"" + aUserName + "\",\"" + aEmail + "\",\"" + aPwd + "\")";
		if (mStatement->executeUpdate(command) == 0)
		{
			// 影响行数为0，认为执行失败
			std::cout << "Command Insert Failed" << std::endl;
			return false;

		}

		return true;

	}
	catch (sql::SQLException e)
	{
		std::cout << "Command Insert Wrong, Error is: " << std::endl;
		std::cout << e.what() << std::endl;

		return false;
	}
}

bool MySQLConnection::Exists(const std::string& aTableName, const std::string& aEmail)
{
	try
	{
		std::string command = "select * from " + aTableName + " where "
			+ "Email=\"" + aEmail + "\"";

		mRes = mStatement->executeQuery(command);
		if (!mRes)
		{
			std::cout << "Command Exists Failed" << std::endl;
			return false;
		}

		if (!mRes->next())
		{
			//std::cout << "未查询到匹配结果，认证失败" << std::endl;
			return false;
		}

		return true;
	}
	catch (sql::SQLException e)
	{
		std::cout << "Command Exists Wrong, Error is: " << std::endl;
		std::cout << e.what() << std::endl;

		return false;
	}
}

bool MySQLConnection::Update(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd)
{
	// update UserInfo set `Password` = "123sdf" where `Email` = "...";
	try
	{
		std::string command = "update " + aTableName +
			" set Password = \"" + aPwd + "\"" +
			" where email=\"" + aEmail + "\"";

		if (mStatement->executeUpdate(command) == 0)
		{
			// 影响行数为0，认为执行失败
			std::cout << "Command Update Failed" << std::endl;
			return false;

		}

		return true;

	}
	catch (sql::SQLException e)
	{
		std::cout << "Command Update Wrong, Error is: " << std::endl;
		std::cout << e.what() << std::endl;

		return false;
	}
}

bool MySQLConnection::IsConnectionValid()
{
	try {
		mRes = mStatement->executeQuery("SELECT 1");
		return true; // 连接有效
	}
	catch (const sql::SQLException& e) {
		// 处理异常，连接可能已断开
		std::cerr << "Error checking connection: " << e.what() << std::endl;
		return false; // 连接无效
	}
}

MySQLConnectionPool::MySQLConnectionPool(const std::string& aHost, const std::string& aPort,
	const std::string& aUserName, const std::string& aPwd, const std::string& aDatabase, int aPoolSize)
	: mPoolSize(aPoolSize)
	, mIsStop(false)
	, mHost(aHost)
	, mPort(aPort)
	, mUserName(aUserName)
	, mPwd(aPwd)
	, mDatabase(aDatabase)
{
	for (int i = 0; i < aPoolSize; ++i)
	{
		std::unique_ptr<MySQLConnection> connection = std::make_unique<MySQLConnection>();
		if (!connection->CreateConnection(aHost, aPort, aUserName, aPwd, aDatabase))
		{
			continue;
		}

		std::cout << "MySQL链接池[" << i << "]号链接连接成功" << std::endl;

		mConnectionPool.push(std::move(connection));
	}

	mCheckThread = std::make_unique<std::thread>([this]() {
		while (!mIsStop)
		{
			CheckConnections();
			std::this_thread::sleep_for(std::chrono::minutes(5));
		}
	});

	mCheckThread->detach();
}

MySQLConnectionPool::~MySQLConnectionPool()
{
	Stop();

	std::lock_guard<std::mutex> lg(mMtx);
	while (!mConnectionPool.empty())
	{
		mConnectionPool.pop();
	}
}

void MySQLConnectionPool::Stop()
{
	mIsStop = true;
	mCdv.notify_all();
}

void MySQLConnectionPool::CheckConnections()
{
	std::lock_guard<std::mutex> lg(mMtx);
	
	int size = mConnectionPool.size();
	for (int i = 0; i < size; ++i)
	{
		std::unique_ptr<MySQLConnection> connection = std::move(mConnectionPool.front());
		mConnectionPool.pop();
		if (!connection->IsConnectionValid())
		{
			// 重建链接
			std::unique_ptr<MySQLConnection> newConnection = std::make_unique<MySQLConnection>();
			if (!newConnection->CreateConnection(mHost, mPort, mUserName, mPwd, mDatabase))
			{
				continue;
				// 丢掉newConnection
			}

			mConnectionPool.push(std::move(newConnection));
			continue;  // 丢掉外面的connection
		}

		mConnectionPool.push(std::move(connection));
	}
}

std::unique_ptr<MySQLConnection> MySQLConnectionPool::GetConnection()
{
	std::unique_lock<std::mutex> ul(mMtx);
	mCdv.wait(ul, [this]() {
		if (mConnectionPool.empty() && !mIsStop)
		{
			return false;
		}
		return true;
	});

	if (mIsStop)
	{
		return nullptr;
	}

	std::unique_ptr<MySQLConnection> connection = std::move(mConnectionPool.front());
	mConnectionPool.pop();

	return connection;
}

void MySQLConnectionPool::ReturnConnection(std::unique_ptr<MySQLConnection>&& aConnection)
{
	std::lock_guard<std::mutex> lg(mMtx);
	mConnectionPool.push(std::move(aConnection));
	mCdv.notify_one();
}

MySQLMgr::MySQLMgr()
	: mUserInfoDBConnectionsPool(nullptr)
{
	ConfigMgr& configMgr = ConfigMgr::GetInstance();
	std::string host = configMgr["MySQLServer"]["host"];
	std::string port = configMgr["MySQLServer"]["port"];
	std::string userName = configMgr["MySQLServer"]["userName"];
	std::string password = configMgr["MySQLServer"]["password"];
	std::string userInfoDB = configMgr["MySQLServer"]["userInfoDB"];
	mUserInfoDBConnectionsPool.reset(new MySQLConnectionPool(host, port, userName, password, userInfoDB));
}

MySQLMgr::~MySQLMgr()
{
}

MySQLMgr& MySQLMgr::GetInstance()
{
	static MySQLMgr self;
	return self;
}

void MySQLMgr::Stop()
{
	mUserInfoDBConnectionsPool->Stop();
}

bool MySQLMgr::Insert(const std::string& aTableName, const std::string& aUserName, const std::string& aEmail, const std::string& aPwd)
{
	std::unique_ptr<MySQLConnection> connection = mUserInfoDBConnectionsPool->GetConnection();
	bool ret = connection->Insert(aTableName, aUserName, aEmail, aPwd);
	mUserInfoDBConnectionsPool->ReturnConnection(std::move(connection));
	return ret;
}

bool MySQLMgr::Exists(const std::string& aTableName, const std::string& aEmail)
{
	std::unique_ptr<MySQLConnection> connection = mUserInfoDBConnectionsPool->GetConnection();
	bool ret = connection->Exists(aTableName, aEmail);
	mUserInfoDBConnectionsPool->ReturnConnection(std::move(connection));
	return ret;
}

bool MySQLMgr::Update(const std::string& aTableName, const std::string& aEmail, const std::string& aPwd)
{
	std::unique_ptr<MySQLConnection> connection = mUserInfoDBConnectionsPool->GetConnection();
	bool ret = connection->Update(aTableName, aEmail, aPwd);
	mUserInfoDBConnectionsPool->ReturnConnection(std::move(connection));
	return ret;
}
