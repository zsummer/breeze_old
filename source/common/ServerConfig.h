

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


typedef unsigned int ServerNodeType;
typedef unsigned int ServerIndexType;

const ServerNodeType AgentNode = 0;
const ServerNodeType AuthNode = 1;
const ServerNodeType CenterNode = 2;
const ServerNodeType DBAgentNode = 3;
const ServerNodeType LogicNode = 4;



const std::map<ServerNodeType, std::string>  configServerNode =
{
	{ AgentNode, "agent" },
	{ AuthNode, "auth" },
	{ CenterNode, "center" },
	{ DBAgentNode, "dbagent" },
	{ LogicNode, "logic" },
};



struct ListenConfig 
{
	std::string ip;
	unsigned short port = 0;
	ServerIndexType index = 0;
};

struct ConnectorConfig 
{
	ServerNodeType dstServer;
	std::string remoteIP;
	unsigned short remotePort = 0;
};


//Êý¾Ý¿â
const std::string AuthMongoDB = "auth";

struct MongoConfig 
{
	std::string ip;
	unsigned short port = 28017;
	std::string db;
	std::string user;
	std::string pwd;
};


class ServerConfig
{
public:
	inline bool Parse(std::string filename, unsigned int index);
public:
	inline const ListenConfig & getConfigListen(ServerNodeType nodeType, ServerIndexType index = (ServerIndexType) -1)
	{
		if (index == (ServerIndexType)-1)
		{
			index = m_index;
		}
		auto founder = m_configListen.find(std::make_pair(nodeType, index));
		if (founder == m_configListen.end())
		{
			static ListenConfig lc;
			return lc;
		}
		return founder->second;
	}


	inline std::vector<ConnectorConfig > &getConfigConnect(ServerNodeType nodeType)
	{
		auto founder = m_configConnect.find(nodeType);
		if (founder == m_configConnect.end())
		{
			static std::vector<ConnectorConfig > vct;
			return vct;
		}
		return founder->second; 
	}

	inline MongoConfig & getAuthMongoDB(){ return m_authMongo; }
private:
	ServerIndexType m_index = 0;
	std::map<std::pair<ServerNodeType, ServerIndexType>, ListenConfig> m_configListen;
	std::map<ServerNodeType, std::vector<ConnectorConfig> > m_configConnect;

	MongoConfig m_authMongo;
};








bool ServerConfig::Parse(std::string filename, unsigned int index)
{
	m_index = index;
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
			auto founder = std::find_if(configServerNode.begin(), 
				configServerNode.end(), 
				[server](std::map<ServerNodeType, std::string>::value_type v){return v.second == server; });
			if (founder == configServerNode.end())
			{
				continue;
			}
			m_configListen.insert(std::make_pair(std::make_pair(founder->first,lconfig.index), lconfig));
			LOGI("serverName=" << server << ", ip=" << lconfig.ip << ", port=" << lconfig.port << ", lconfig.index=" << lconfig.index);
		}


		auto connecter = pt.get_child("connect");
		for (auto iter = connecter.begin(); iter != connecter.end(); ++iter)
		{
			std::string server = iter->first;
			ConnectorConfig lconfig;
			std::string dstServer = iter->second.get<std::string>("<xmlattr>.server");
			auto founderDstServer = std::find_if(configServerNode.begin(),
				configServerNode.end(),
				[dstServer](std::map<ServerNodeType, std::string>::value_type v){return v.second == dstServer; });
			if (founderDstServer == configServerNode.end())
			{
				continue;
			}
			lconfig.dstServer = founderDstServer->first;

			lconfig.remoteIP = iter->second.get<std::string>("<xmlattr>.ip");
			lconfig.remotePort = iter->second.get<unsigned short>("<xmlattr>.port");
			auto founder = std::find_if(configServerNode.begin(),
				configServerNode.end(),
				[server](std::map<ServerNodeType, std::string>::value_type v){return v.second == server; });
			if (founder == configServerNode.end())
			{
				continue;
			}
			m_configConnect[founder->first].push_back(lconfig);
			LOGI("serverName=" << server << ", remoteIP=" << lconfig.remoteIP << ", remotePort=" << lconfig.remotePort << ", lconfig.dstServer=" << lconfig.dstServer);
		}

		auto mongoParse = pt.get_child("mongo");
		for (auto iter = mongoParse.begin(); iter != mongoParse.end(); ++iter)
		{
			MongoConfig lconfig;
			lconfig.ip = iter->second.get<std::string>("<xmlattr>.ip");
			lconfig.port = iter->second.get<unsigned short>("<xmlattr>.port");
			lconfig.db = iter->second.get<std::string>("<xmlattr>.db");
			lconfig.user = iter->second.get<std::string>("<xmlattr>.user");
			lconfig.pwd = iter->second.get<std::string>("<xmlattr>.pwd");
			if (iter->first == AuthMongoDB)
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