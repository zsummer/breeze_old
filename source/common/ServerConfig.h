

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



class ServerConfig
{
public:
	bool Load(std::string serverName, std::string filename, unsigned int index)
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
					if (server == "agent")
					{
						m_AgentListen = lconfig;
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
				if (server == serverName && serverName == "agent")
				{
					m_AgentConnect.push_back(lconfig);
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
public:
	inline ListenConfig & getAgentListen(){ return m_AgentListen; }
	inline ListenConfig & getCenterListen(){ return m_CenterListen; }
	inline ListenConfig & getDBListen(){ return m_DBListen; }

	inline std::vector<ConnectorConfig > &getAgentConnect(){ return m_AgentConnect; }
	inline std::vector<ConnectorConfig> & getCenterConnect(){ return m_CenterConnect; }
private:
	ListenConfig m_AgentListen;
	ListenConfig m_CenterListen;
	ListenConfig m_DBListen;

	std::vector<ConnectorConfig> m_AgentConnect;
	std::vector<ConnectorConfig> m_CenterConnect;
};






































#endif