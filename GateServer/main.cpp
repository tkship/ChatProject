#include "global.h"
#include "HttpServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h" //test
#include "MySQLMgr.h" //test

#include "boost/asio.hpp"
#include "hiredis/hiredis.h"

#include <iostream>
#include <string>

#include <cassert>


void TestRedisMgr() {
    assert(RedisMgr::GetInstance().Auth("123456"));
    assert(RedisMgr::GetInstance().Set("blogwebsite", "llfc.club"));
    std::string value = "";
    assert(RedisMgr::GetInstance().Get("blogwebsite", value));
    assert(RedisMgr::GetInstance().Get("nonekey", value) == false);
    assert(RedisMgr::GetInstance().HSet("bloginfo", "blogwebsite", "llfc.club"));
    assert(RedisMgr::GetInstance().HGet("bloginfo", "blogwebsite", value));
    assert(RedisMgr::GetInstance().ExistsKey("bloginfo"));
    assert(RedisMgr::GetInstance().Del("bloginfo"));
    assert(RedisMgr::GetInstance().Del("bloginfo"));
    assert(RedisMgr::GetInstance().ExistsKey("bloginfo") == false);
    assert(RedisMgr::GetInstance().LPush("lpushkey1", "lpushvalue1"));
    assert(RedisMgr::GetInstance().LPush("lpushkey1", "lpushvalue2"));
    assert(RedisMgr::GetInstance().LPush("lpushkey1", "lpushvalue3"));
    assert(RedisMgr::GetInstance().RPop("lpushkey1", value));
    assert(RedisMgr::GetInstance().RPop("lpushkey1", value));
    assert(RedisMgr::GetInstance().LPop("lpushkey1", value));
    assert(RedisMgr::GetInstance().LPop("lpushkey2", value) == false);
    RedisMgr::GetInstance().Stop();
}

void TestMySQLMgr()
{
    auto& mysqlMgr = MySQLMgr::GetInstance();
    assert(!mysqlMgr.Exists("UserInfo", "123456@qq.com"));
    assert(mysqlMgr.Insert("UserInfo", "zhangsan", "123456@qq.com", "123456"));
    assert(mysqlMgr.Exists("UserInfo", "123456@qq.com"));
}


int main()
{
    //TestRedisMgr();
    //TestMySQLMgr();
    
    ConfigMgr& configMgr = ConfigMgr::GetInstance();
    std::string portStr = configMgr["GateServer"]["port"];
    unsigned short port = atoi(portStr.c_str());

    boost::asio::io_service service;
    HttpServer server(port, service);

    service.run();


    return 0;
}