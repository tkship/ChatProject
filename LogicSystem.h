#pragma once
#include <string>
#include <map>
#include <memory>
#include <functional>

class HttpConnection;
class LogicSystem
{
	typedef std::function<void(std::shared_ptr<HttpConnection>)> FuncCallBack;
public:
	LogicSystem();
	~LogicSystem();

	static LogicSystem& GetInstance();

	bool HandleGet(const std::string& aUrl, std::shared_ptr<HttpConnection> aConnection);
	bool HandlePost(const std::string& aUrl, std::shared_ptr<HttpConnection> aConnection);

private:
	void RegisterGet(const std::string& aUrl, FuncCallBack aCallBack);
	void RegisterPost(const std::string& aUrl, FuncCallBack aCallBack);
private:
	std::map<std::string, FuncCallBack> mGetCallBackMap;
	std::map<std::string, FuncCallBack> mPostCallBackMap;
};

