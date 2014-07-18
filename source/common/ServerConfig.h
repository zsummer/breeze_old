

/*
* breeze License
* Copyright (C) 2014 YaweiZhang <yawei_zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _SERVER_CONFIG_H_
#define _SERVER_CONFIG_H_

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <log4z/log4z.h>
#include <map>
#include <iostream>


struct ListenConfig 
{
	std::string ip;
	unsigned short port = 0;
	unsigned int index = 0;
};

struct ConnectorConfig 
{
	std::string dstServer;
	std::string remoteIP;
	unsigned short remotePort = 0;
};

struct MongoConfig 
{
	std::string ip;
	unsigned short port = 28017;
	std::string db;
	std::string user;
	std::string pwd;
};

const std::string AgentNode = "agent";
const std::string AuthNode = "auth";
const std::string CenterNode = "center";
const std::string DBAgentNode = "dbagent";
const std::string LogicNode = "logic";

const std::string AuthMongoDB = "auth";

class ServerConfig
{
public:
	inline bool Parse(std::string serverName, std::string filename, unsigned int index);
public:
	inline ListenConfig & getAgentListen(){ return m_AgentListen; }
	inline ListenConfig & getAuthListen(){ return m_AuthListen; }
	inline ListenConfig & getCenterListen(){ return m_CenterListen; }
	inline ListenConfig & getDBListen(){ return m_DBListen; }

	inline std::vector<ConnectorConfig > &getAgentConnect(){ return m_AgentConnect; }
	inline std::vector<ConnectorConfig> & getCenterConnect(){ return m_CenterConnect; }

	inline MongoConfig & getAuthMongoDB(){ return m_authMongo; }
private:
	ListenConfig m_AgentListen;
	ListenConfig m_CenterListen;
	ListenConfig m_DBListen;
	ListenConfig m_AuthListen;

	std::vector<ConnectorConfig> m_AgentConnect;
	std::vector<ConnectorConfig> m_CenterConnect;

	MongoConfig m_authMongo;
};








bool ServerConfig::Parse(std::string serverName, std::string filename, unsigned int index)
{
	try
	{
		boost::property_tree::ptree pt;
		boost::property_tree::read_xml(filename, pt);
		auto listener = pt.get_child("listen");
		for (auto iter = listener.begin(); iter != listener.end(); ++iter)
		{
			std::string server = iter->first;
			ListenConfig lconfig;
			lconfig.ip = iter->second.get<std::string>("<xmlattr>.ip");
			lconfig.port = iter->second.get<unsigned short>("<xmlattr>.port");
			lconfig.index = iter->second.get<unsigned int>("<xmlattr>.index");
			if (serverName == server && lconfig.index == index)
			{
				if (server == AgentNode)
				{
					m_AgentListen = lconfig;
				}
				else if (server == AuthNode)
				{
					m_AuthListen = lconfig;
				}
				LOGI("serverName=" << serverName << ", ip=" << lconfig.ip << ", port=" << lconfig.port << ", lconfig.index=" << lconfig.index);
			}
		}


		auto connecter = pt.get_child("connect");
		for (auto iter = connecter.begin(); iter != connecter.end(); ++iter)
		{
			std::string server = iter->first;
			ConnectorConfig lconfig;
			lconfig.dstServer = iter->second.get<std::string>("<xmlattr>.server");
			lconfig.remoteIP = iter->second.get<std::string>("<xmlattr>.ip");
			lconfig.remotePort = iter->second.get<unsigned short>("<xmlattr>.port");
			if (server == serverName && serverName == AgentNode)
			{
				m_AgentConnect.push_back(lconfig);
			}
		}

		auto mongoParse = pt.get_child("mongo");
		for (auto iter = mongoParse.begin(); iter != mongoParse.end(); ++iter)
		{
			MongoConfig lconfig;
			lconfig.db = iter->first;
			lconfig.ip = iter->second.get<std::string>("<xmlattr>.ip");
			lconfig.port = iter->second.get<unsigned short>("<xmlattr>.port");
			lconfig.user = iter->second.get<std::string>("<xmlattr>.user");
			lconfig.pwd = iter->second.get<std::string>("<xmlattr>.pwd");
			if (lconfig.db == AuthMongoDB)
			{
				m_authMongo = lconfig;
			}
		}

	}
	catch (std::string err)
	{
		LOGE("ServerConfig catch exception: error=" << err);
		return false;
	}
	catch (std::exception e)
	{
		LOGE("ServerConfig catch exception: error=" << e.what());
		return false;
	}
	catch (...)
	{
		LOGE("ServerConfig catch exception: unknow exception.");
		return false;
	}
	return true;
}





























#endif