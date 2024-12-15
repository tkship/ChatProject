#pragma once
#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "boost/beast/http.hpp"

#include <map>
#include <memory>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
namespace ip = boost::asio::ip;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class LogicSystem;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(tcp::socket* aSocket);
	~HttpConnection();
	void Start();

private:
	HttpConnection(const HttpConnection&) = delete;
	HttpConnection& operator=(const HttpConnection&) = delete;

	void HandleRequest();
	void WriteResponse();
	void SetDeadline();

	// 参数是内存的值小于0x0F的char，转变为打印出来为其内存的值的char
	char ToHex(char aC);
	// 参数是打印的值小于0x0F的char
	char FromHex(char aC);

	std::string ToUnicode(const std::string& aStr);
	std::string FromUnicode(const std::string& aStr);

	bool ParseUrl(const std::string& aUrl);
private:
	tcp::socket* mSocket;
	beast::flat_buffer mBuffer;
	http::request<http::dynamic_body> mRequest;
	http::response<http::dynamic_body> mResponse;
	asio::steady_timer mTimer;
	std::string mParsedUrl;
	std::map<std::string, std::string> mParamsMap;
};

class HttpServer
{
public:
	HttpServer(unsigned short aPort, asio::io_service& aService);
	~HttpServer();

	void Start();

private:
	HttpServer(const HttpServer&) = delete;
	HttpServer& operator=(const HttpServer&) = delete;

private:
	asio::io_service& mService;
	tcp::socket* mPendingSocket;
	asio::ip::tcp::acceptor mAcceptor;
	std::vector<std::shared_ptr<HttpConnection>> mConnections;
};


