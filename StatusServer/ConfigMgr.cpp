#include "ConfigMgr.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <iostream>

ModuleInfo::ModuleInfo()
{
}

ModuleInfo::~ModuleInfo()
{
}

std::string ModuleInfo::operator[](const std::string& aKey)
{
	if (mMap.find(aKey) != mMap.end())
	{
		return mMap[aKey];
	}

	assert("尝试获取不存在的设置项");
	return std::string();
}

ConfigMgr::ConfigMgr()
{
	boost::filesystem::path currentPath = boost::filesystem::current_path();
	boost::filesystem::path configPath = currentPath / "config.ini";

	std::cout << configPath << std::endl; // test

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(configPath.string(), pt);

	for (auto& modulePair : pt)
	{
		std::string moduleName = modulePair.first;
		boost::property_tree::ptree moduleConfig = modulePair.second;
		
		for (auto& configPair : moduleConfig)
		{
			mMap[moduleName].mMap[configPair.first] = configPair.second.get_value<std::string>();
		}
	}
}

ConfigMgr& ConfigMgr::GetInstance()
{
	static ConfigMgr self;
	return self;
}

ModuleInfo ConfigMgr::operator[](const std::string& aKey)
{
	if (mMap.find(aKey) != mMap.end())
	{
		return mMap[aKey];
	}
	
	assert("尝试获取不存在的设置项");
	return ModuleInfo();
}
