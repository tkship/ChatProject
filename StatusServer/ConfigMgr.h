#pragma once

#include "global.h"
#include <map>


struct ModuleInfo
{
public:
	ModuleInfo();
	~ModuleInfo();

	std::string operator[](const std::string& aKey);

public:
	std::map<std::string, std::string> mMap;
};

class ConfigMgr
{
public:
	ConfigMgr();
	static ConfigMgr& GetInstance();

	ModuleInfo operator[](const std::string& aKey);
private:
	std::map<std::string, ModuleInfo> mMap;
};

