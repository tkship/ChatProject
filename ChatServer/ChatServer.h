#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>

#include <queue>
#include <unordered_map>
#include <memory>


namespace beast = boost::beast;
namespace websocket = beast::websocket;

class ChatServer;

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(boost::asio::io_context& aIoService, ChatServer* aChatServerPtr);
	~Session();

	void Start();
	void Send(const std::string& aSendData);
	void Accept();
	void Close();

	std::string GetUuid();
	boost::asio::ip::tcp::socket& GetSocket();
private:
	std::string mUuid;  // 反向检索ChatServer中的自己，错误时删除自身
	ChatServer* mChatServerPtr;  // 反指ChatServer;
	boost::asio::io_context& mIoService;
	std::unique_ptr<websocket::stream< beast::tcp_stream>>  mWebSocket; // 就是socket的角色
	std::mutex mMtx; // 发送队列的锁
	//std::queue<Json::Value> mSendQueue;  // 发送队列
	beast::flat_buffer mRecvBuffer;  // 接收缓冲区（可变大小）
};

class ChatServer
{
public:
	ChatServer(unsigned short aPort, boost::asio::io_context& aIoContext);
	~ChatServer();

	void Start();
	void AddSession(std::shared_ptr<Session> aSession);
	void RemoveSession(const std::string& aUid);
private:
	boost::asio::io_context& mAcceptIoContext;
	boost::asio::ip::tcp::acceptor mAcceptor;
	std::unordered_map<std::string, std::shared_ptr<Session>> mSessions;
	std::mutex mMtx;
};

