

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
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "ServerConfig.h"





const ListenConfig ServerConfig::getConfigListen(ServerNode node, NodeIndex index)
{
	if (index == InvalidNodeIndex)
	{
		index = m_ownNodeIndex;
	}
	auto founder = std::find_if(m_configListen.begin(), m_configListen.end(),
		[node, index](const ListenConfig & lc){return lc.node == node && lc.index == index; });
	if (founder == m_configListen.end())
	{
		static ListenConfig lc;
		return lc;
	}
	return *founder;
}


std::vector<ConnectorConfig > ServerConfig::getConfigConnect(ServerNode node)
{
	std::vector<ConnectorConfig > ret;
	for (const auto & cc : m_configConnect)
	{
		if (cc.srcNode != node)
		{
			continue;
		}
		ret.push_back(cc);
	}
	
	return ret;
}






static ServerNode toServerNode(std::string strNode)
{
	if (strNode == "agent")
	{
		return AgentNode;
	}
	else if (strNode == "center")
	{
		return CenterNode;
	}
	else if (strNode == "auth")
	{
		return AuthNode;
	}
	else if (strNode == "logic")
	{
		return LogicNode;
	}
	return InvalideServerNode;
}

bool ServerConfig::Parse(std::string filename, ServerNode ownNode, NodeIndex ownIndex)
{
	m_ownServerNode = ownNode;
	m_ownNodeIndex = ownIndex;
	try
	{
		boost::property_tree::ptree pt;
		boost::property_tree::read_xml(filename, pt);

		auto server = pt.get_child("server");
		m_gameid = server.get<unsigned int>("<xmlattr>.gameid");
		m_areaid = server.get<unsigned int>("<xmlattr>.areaid");

		auto listener = pt.get_child("listen");
		for (auto iter = listener.begin(); iter != listener.end(); ++iter)
		{
			std::string strNode = iter->first;
			ListenConfig lconfig;
			lconfig.ip = iter->second.get<std::string>("<xmlattr>.ip");
			lconfig.port = iter->second.get<unsigned short>("<xmlattr>.port");
			lconfig.index = iter->second.get<unsigned int>("<xmlattr>.index");
			lconfig.node = toServerNode(strNode);
			m_configListen.push_back(lconfig);
			LOGD("strNode=" << strNode << ", ip=" << lconfig.ip << ", port=" << lconfig.port << ", lconfig.index=" << lconfig.index);
		}


		auto connecter = pt.get_child("connect");
		for (auto iter = connecter.begin(); iter != connecter.end(); ++iter)
		{
			std::string srcStrNode = iter->first;
			ConnectorConfig lconfig;
			std::string dstStrNode = iter->second.get<std::string>("<xmlattr>.dstNode");
			lconfig.remoteIP = iter->second.get<std::string>("<xmlattr>.ip");
			lconfig.remotePort = iter->second.get<unsigned short>("<xmlattr>.port");
			lconfig.srcNode = toServerNode(srcStrNode);
			lconfig.dstNode = toServerNode(dstStrNode);
			m_configConnect.push_back(lconfig);
			LOGD("srcStrNode=" << srcStrNode << ", remoteIP=" << lconfig.remoteIP << ", remotePort=" << lconfig.remotePort << ", dstStrNode=" << dstStrNode);
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
			lconfig.needAuth = iter->second.get<bool>("<xmlattr>.needAuth");
			if (iter->first == AuthMongoDB)
			{
				m_authMongo = lconfig;
			}
			else if (iter->first == InfoMongoDB)
			{
				m_infoMongo = lconfig;
			}
			else if (iter->first == LogMongoDB)
			{
				m_logMongo = lconfig;
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


























