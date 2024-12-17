#include "ChatServer.h"
#include "IoServicePool.h"
#include "ConfigMgr.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <iostream>

Session::Session(boost::asio::io_context& aIoService, ChatServer* aChatServerPtr)
	: mChatServerPtr(aChatServerPtr)
	, mIoService(aIoService)
	// 为了防止同时多次读事件造成的数据紊乱，加上strand保证读完第一次消息才会读第二次消息
	// 但这样读写回调也会串行，会不会有效率问题...
	, mWebSocket(std::make_unique<websocket::stream<beast::tcp_stream>>(boost::asio::make_strand(aIoService)))
{
	boost::uuids::uuid uid = boost::uuids::random_generator()();
	mUuid = boost::uuids::to_string(uid);
}

Session::~Session()
{
	Close();
}

void Session::Start()
{
	// 闭包，保证回调函数执行时Session未被析构
	auto self = shared_from_this();
	mWebSocket->async_read(mRecvBuffer, [self](beast::error_code aEc, std::size_t aBufferBytes) {
		try
		{
			if (aEc)
			{
				// 读取出错，删除自身连接
				std::cout << "Uid[" << self->mUuid << "] read wrong, error is :" << std::endl;
				std::cout << aEc.what() << std::endl;
				self->mChatServerPtr->RemoveSession(self->mUuid);
				return;
			}

			// 暂时原模原样发回去
			self->mWebSocket->text(self->mWebSocket->got_text());
			std::string recvData = beast::buffers_to_string(self->mRecvBuffer.data());
			self->mRecvBuffer.consume(self->mRecvBuffer.size());  // 清除缓冲区
			self->Send(std::move(recvData));
			self->Start();
		}
		catch (const std::exception& aE)
		{
			// 读取出错，删除自身连接
			std::cout << "Uid[" << self->mUuid << "] read wrong, error is :" << std::endl;
			std::cout << aE.what() << std::endl;
			self->mChatServerPtr->RemoveSession(self->mUuid);
		}
	});
}

void Session::Send(const std::string& aSendData)
{
	// json应该是放在logicsystem中处理
	/*Json::Reader reader;
	Json::Value root;   
	if (!reader.parse(aSendData, root))
	{
		std::cout << "json parse wrong" << std::endl;
		return;
	}*/

	auto self = shared_from_this();
	mWebSocket->async_write(boost::asio::buffer(aSendData.c_str(), aSendData.size()), [self](beast::error_code aEc, std::size_t) {
		try
		{
			if (aEc)
			{
				std::cout << "Uid[" << self->mUuid << "] write wrong, error is :" << std::endl;
				std::cout << aEc.what() << std::endl;
				self->mChatServerPtr->RemoveSession(self->mUuid);
				return;
			}
		}
		catch (const std::exception& aE)
		{
			std::cout << "Uid[" << self->mUuid << "] write wrong, error is :" << std::endl;
			std::cout << aE.what() << std::endl;
			self->mChatServerPtr->RemoveSession(self->mUuid);
		}
	});
}

void Session::Accept()
{
	auto self = shared_from_this();
	mWebSocket->async_accept([self](beast::error_code aEc) {
		if (aEc)
		{
			std::cout << "Uid[" << self->mUuid << "] accept wrong, error is :" << std::endl;
			std::cout << aEc.what() << std::endl;
			return;
		}

		self->mChatServerPtr->AddSession(self);
		self->Start();
	});
}

void Session::Close()
{
}

std::string Session::GetUuid()
{
	return mUuid;
}

boost::asio::ip::tcp::socket& Session::GetSocket()
{
	return beast::get_lowest_layer(*mWebSocket).socket();
}

ChatServer::ChatServer(unsigned short aPort, boost::asio::io_context& aIoContext)
	: mAcceptIoContext(aIoContext)
	, mAcceptor(aIoContext, { boost::asio::ip::tcp::v4(), aPort })
{
	std::cout << "ChatServer is running" << std::endl;
	Start();
}

ChatServer::~ChatServer()
{
}

void ChatServer::Start()
{
	std::shared_ptr<Session> newSession = std::make_shared<Session>(IoServicePool::GetInstance().GetIoService(), this);
	mAcceptor.async_accept(newSession->GetSocket(), [this, newSession](boost::system::error_code ec) {
		if (ec)
		{
			Start();
			return;
		}

		newSession->Accept();
		Start();
	});
}

void ChatServer::AddSession(std::shared_ptr<Session> aSession)
{
	std::lock_guard<std::mutex> lg(mMtx);
	mSessions[aSession->GetUuid()] = aSession;
}

void ChatServer::RemoveSession(const std::string& aUid)
{
	// 要不要加锁？
	// 虽然send，read回调因为加了strand是串行，但毕竟这里还是会有多个线程进入
	std::lock_guard<std::mutex> lg(mMtx);
	mSessions.erase(aUid);
}
