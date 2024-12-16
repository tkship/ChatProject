#pragma once

#include <boost/asio.hpp>

#include <vector>
#include <memory>
#include <thread>

class IoServicePool
{
	typedef boost::asio::io_service::work Work;
public:
	static IoServicePool& GetInstance();
	boost::asio::io_service& GetIoService();
private:
	IoServicePool(int aPoolSize = std::thread::hardware_concurrency());
	~IoServicePool();
	void Stop();

private:
	int mPoolSize;
	int mIndex;

	std::vector<boost::asio::io_service> mServices;
	std::vector<std::unique_ptr<Work>> mWorks;
	std::vector<std::thread> mThreads;
};

