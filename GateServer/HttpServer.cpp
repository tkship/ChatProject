#include "HttpServer.h"
#include "LogicSystem.h"
#include "IoServicePool.h"

#include <iostream>

HttpConnection::HttpConnection(tcp::socket* aSocket)
	: mSocket(aSocket)
	, mBuffer(8192)
	, mTimer(mSocket->get_executor(), asio::chrono::seconds(10))
{
}

HttpConnection::~HttpConnection()
{
	delete mSocket;
}


void HttpConnection::Start()
{
	auto self = shared_from_this();
	http::async_read(*mSocket, mBuffer, mRequest, [self](beast::error_code aEc, size_t) {
		if (aEc)
		{
			std::cout << "read wrong, errorcode: " << aEc << std::endl;
			self->mSocket->close();
			return;
		}

		self->HandleRequest();
		self->SetDeadline();
	});
}

void HttpConnection::HandleRequest()
{
	mResponse.version(mRequest.version());
	mResponse.keep_alive(false);

	if (!ParseUrl(mRequest.target()))
	{
		// log日志之类的
		std::cout << "Parse Url Wrong" << std::endl;
		return;
	}

	if (mRequest.method() == http::verb::get)
	{
		bool success = LogicSystem::GetInstance().HandleGet(mParsedUrl, shared_from_this());
		if (!success)
		{
			mResponse.result(http::status::not_found);
			mResponse.set(http::field::content_type, "text/plain");
			beast::ostream(mResponse.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		mResponse.result(http::status::ok);
		mResponse.set(http::field::server, "GateServer");
		WriteResponse();
	}
	else if (mRequest.method() == http::verb::post)
	{
		bool success = LogicSystem::GetInstance().HandlePost(mParsedUrl, shared_from_this());
		if (!success)
		{
			mResponse.result(http::status::not_found);
			mResponse.set(http::field::content_type, "text/plain");
			beast::ostream(mResponse.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		mResponse.result(http::status::ok);
		mResponse.set(http::field::server, "GateServer");
		WriteResponse();
	}
}

void HttpConnection::WriteResponse()
{
	auto self = shared_from_this();
	mResponse.content_length(mResponse.body().size());
	http::async_write(*mSocket, mResponse, [self](boost::beast::error_code ec, size_t) {
		self->mSocket->shutdown(tcp::socket::shutdown_send, ec);
		self->mTimer.cancel();
	});
}

void HttpConnection::SetDeadline()
{
	auto self = shared_from_this();
	mTimer.async_wait([self](boost::system::error_code) {
		self->mSocket->close();
	});
}


char HttpConnection::ToHex(char aC)
{
	aC += aC > 9 ? 87 : 48;
	return aC;
}

char HttpConnection::FromHex(char aC)
{
	if (aC >= 'a' && aC <= 'f')
	{
		aC -= 87;
	}
	else if (aC >= 'A' && aC <= 'F')
	{
		aC -= 55;
	}
	else if (aC >= '0' && aC <= '9')
	{
		aC -= 48;
	}
	else
	{
		assert("输入错误参数");
		return -1;
	}

	return aC;
}

std::string HttpConnection::ToUnicode(const std::string& aStr)
{
	std::string ret;
	for (int i = 0; i < aStr.size(); ++i)
	{
		if ((aStr[i] >= 'a' && aStr[i] <= 'z') ||
			(aStr[i] >= 'A' && aStr[i] <= 'Z') ||
			(aStr[i] >= '0' && aStr[i] <= '9') ||
			aStr[i] == '-' ||
			aStr[i] == '_' ||
			aStr[i] == '.' ||
			aStr[i] == '~' ||
			aStr[i] == ':' ||
			aStr[i] == '/' ||
			aStr[i] == '?' ||
			aStr[i] == '=' ||
			aStr[i] == '&')
		{
			ret += aStr[i];
		}
		else if (aStr[i] == ' ')
		{
			ret += '+';
		}
		else
		{
			ret += '%';
			ret += ToHex((aStr[i] >> 4) & 0x0F);
			ret += ToHex(aStr[i] & 0x0F);
		}
	}

	return ret;
}

std::string HttpConnection::FromUnicode(const std::string& aStr)
{
	std::string ret;
	for (int i = 0; i < aStr.size(); ++i)
	{
		if (isalnum(aStr[i]) ||
			aStr[i] == '-' ||
			aStr[i] == '_' ||
			aStr[i] == '.' ||
			aStr[i] == '~' ||
			aStr[i] == ':' ||
			aStr[i] == '/' ||
			aStr[i] == '?' ||
			aStr[i] == '=' ||
			aStr[i] == '&')
		{
			ret += aStr[i];
		}
		else if (aStr[i] == '+')
		{
			ret += ' ';
		}
		else if (aStr[i] == '%')
		{
			assert(i + 2 < aStr.size());
			char temp = 0;
			temp += FromHex(aStr[++i]) << 4;
			temp += FromHex(aStr[++i]);
			ret += temp;
		}
	}

	return ret;
}

bool HttpConnection::ParseUrl(const std::string& aUrl)
{
	try
	{
		auto pos = aUrl.find('?');
		if (pos == aUrl.npos)
		{
			mParsedUrl = aUrl;
			return true;
		}

		mParsedUrl = aUrl.substr(0, pos);

		std::string restUrl = aUrl.substr(pos + 1);

		int prevPos = 0;
		int frontPos = restUrl.find('&');
		while (frontPos < restUrl.size())
		{
			std::string temp = restUrl.substr(prevPos, frontPos - prevPos);
			pos = temp.find('=');
			mParamsMap[FromUnicode(temp.substr(0, pos))] = FromUnicode(temp.substr(pos + 1));
			prevPos = frontPos + 1;
			frontPos = restUrl.find('&', prevPos);
		}

		std::string temp = restUrl.substr(prevPos);
		pos = temp.find('=');
		mParamsMap[FromUnicode(temp.substr(0, pos))] = FromUnicode(temp.substr(pos + 1));
	}
	catch (const std::exception&)
	{
		return false;
	}

	return true;
	
}

HttpServer::HttpServer(unsigned short aPort, asio::io_service& aService)
	: mService(aService)
	, mAcceptor(mService, { tcp::v4(), aPort })
	, mPendingSocket(new tcp::socket(mService))
{
	Start();
}

HttpServer::~HttpServer()
{
}

void HttpServer::Start()
{
	mAcceptor.async_accept(*mPendingSocket, [this](boost::system::error_code ec) {
		if (ec)
		{
			Start();
			return;
		}

		// 这里是短连接 需要维护一个vector吗？ 会内存泄露的
		auto newConnection = std::make_shared<HttpConnection>(mPendingSocket);
		mConnections.emplace_back(newConnection);
		newConnection->Start();

		mPendingSocket = new tcp::socket(IoServicePool::GetInstance().GetIoService());
		Start();
	});
}

