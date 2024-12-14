#include "LogicSystem.h"
#include "HttpServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "VarifyClient.h"
#include "MySQLMgr.h"

#include "json/json.h"

#include <cassert>
#include <iostream> // test

LogicSystem::LogicSystem()
{
    RegisterGet("/GetTest", [](std::shared_ptr<HttpConnection> aConnection) {
        beast::ostream(aConnection->mResponse.body()) << "Test Success" << std::endl;;
        for (auto e : aConnection->mParamsMap)
        {
            beast::ostream(aConnection->mResponse.body())
              << e.first << ":" << e.second << std::endl;

            std::cout << e.first << ":" << e.second << std::endl;
        }
    });

    RegisterPost("/GetVarifyCode", [](std::shared_ptr<HttpConnection> aConnection) {
        auto bodyData = boost::beast::buffers_to_string(aConnection->mRequest.body().data());
        
        Json::Value root;
        Json::Reader reader;
        Json::Value retRoot;
        if (!reader.parse(bodyData, root))
        {
            retRoot["error"] = "JsonParseError";
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        if (!root.isMember("email"))
        {
            retRoot["error"] = "JsonParseError";
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        std::string email = root["email"].asString();
        std::cout << "receive email is:" << email << std::endl;

        GetVarifyRsp response = VarifyClient::GetInstance().GetVarifyCode(email);
        retRoot["error"] = response.error();
        retRoot["email"] = response.email();
        beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
    });

    RegisterPost("/Register", [](std::shared_ptr<HttpConnection> aConnection) {
        auto bodyData = boost::beast::buffers_to_string(aConnection->mRequest.body().data());

        Json::Value root;
        Json::Reader reader;
        Json::Value retRoot;
        if (!reader.parse(bodyData, root))
        {
            retRoot["error"] = "JsonParseError";
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        if (!root.isMember("userName")
            || !root.isMember("email")
            || !root.isMember("password")
            || !root.isMember("confirmPwd")
            || !root.isMember("verifyCode"))
        {
            retRoot["error"] = "JsonParseError";
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        // 验证
        // 1. 用户名和邮箱是否已存在..
        // 2. 验证码是否正确
        std::string email = root["email"].asString();
        std::string trueVerifyCode;
        
        if (!RedisMgr::GetInstance().Get(email, trueVerifyCode))
        {
            retRoot["error"] = Redis_KeyNotFound;
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        if (root["verifyCode"].asString() != trueVerifyCode)
        {
            retRoot["error"] = VerifyCodeNotMatch;
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        // 注册.. 
        MySQLMgr& mysqlMgr = MySQLMgr::GetInstance();
        ConfigMgr& configMgr = ConfigMgr::GetInstance();
        if (mysqlMgr.Exists(configMgr["MySQLServer"]["userInfoTable"], root["email"].asString()))
        {
            retRoot["error"] = MySQL_UserExist;   // 这里还有可能是数据库服务器有问题
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        if (!mysqlMgr.Insert(configMgr["MySQLServer"]["userInfoTable"], root["userName"].asString(), root["email"].asString(), root["password"].asString()))
        {
            retRoot["error"] = MySQL_Error;
            beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
            return;
        }

        // 返回
        retRoot["error"] = Success;
        beast::ostream(aConnection->mResponse.body()) << retRoot.toStyledString();
    });
}

LogicSystem::~LogicSystem()
{
}

LogicSystem& LogicSystem::GetInstance()
{
    static LogicSystem self;
    return self;
}

bool LogicSystem::HandleGet(const std::string& aUrl, std::shared_ptr<HttpConnection> aConnection)
{
    if (mGetCallBackMap.find(aUrl) == mGetCallBackMap.end())
    {
        return false;
    }

    mGetCallBackMap[aUrl](aConnection);
    return true;
}

bool LogicSystem::HandlePost(const std::string& aUrl, std::shared_ptr<HttpConnection> aConnection)
{
    if (mPostCallBackMap.find(aUrl) == mPostCallBackMap.end())
    {
        return false;
    }

    mPostCallBackMap[aUrl](aConnection);
    return true;
}

void LogicSystem::RegisterGet(const std::string& aUrl, FuncCallBack aCallBack)
{
    if (mGetCallBackMap.find(aUrl) != mGetCallBackMap.end())
    {
        assert("重复url注册");
    }

    mGetCallBackMap[aUrl] = aCallBack;
}

void LogicSystem::RegisterPost(const std::string& aUrl, FuncCallBack aCallBack)
{
    if (mPostCallBackMap.find(aUrl) != mPostCallBackMap.end())
    {
        assert("重复url注册");
    }

    mPostCallBackMap[aUrl] = aCallBack;
}
