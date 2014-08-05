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


//! 测试



#include <zsummerX/FrameHeader.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
#include <unordered_map>
#include <ProtoDefine.h>
#include <ProtoCommon.h>
#include <ProtoAuth.h>
#include <ProtoLogin.h>
#include <ServerConfig.h>
using namespace zsummer::log4z;

//! 默认启动参数

unsigned short g_agentIndex = 0; //
unsigned short g_maxClient = 1; //如启动的客户端总数.


//!收发包测试统计数据
unsigned long long g_totalEchoCount = 0;
unsigned long long g_lastEchoCount = 0;
unsigned long long g_totalSendCount = 0;
unsigned long long g_totalRecvCount = 0;
void MonitorFunc()
{
	LOGI("per seconds Echos Count=" << (g_totalEchoCount - g_lastEchoCount) / 5
		<< ", g_totalSendCount[" << g_totalSendCount << "] g_totalRecvCount[" << g_totalRecvCount << "]");
	g_lastEchoCount = g_totalEchoCount;
	CTcpSessionManager::getRef().CreateTimer(5000, MonitorFunc);
};


class CStressHeartBeatManager
{
public:
	CStressHeartBeatManager()
	{
		//! 注册事件和消息
		CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CStressHeartBeatManager::OnConnecotrConnected, this,
			std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterOnMyConnectorHeartbeatTimer(std::bind(&CStressHeartBeatManager::OnConnecotrHeartbeatTimer, this,
			std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CStressHeartBeatManager::OnConnecotrDisconnect, this,
			std::placeholders::_1));
	}
	
	void OnConnecotrConnected(ConnectorID cID)
	{
		m_connectorHB[cID] = time(NULL);
		LOGI("connect sucess. cID=" << cID);
	}
	void OnConnecotrHeartbeatTimer(ConnectorID cID)
	{

	}
	void OnConnecotrDisconnect(ConnectorID cID)
	{
		m_connectorHB.erase(cID);
		LOGI("Disconnect. cID=" << cID);
	}

	void OnMsgServerHeartbeat(ConnectorID cID, ProtocolID pID, ReadStreamPack & pack)
	{
		auto iter = m_connectorHB.find(cID);
		if (iter != m_connectorHB.end())
		{
			iter->second = time(NULL);
		}
	}


private:
	std::unordered_map<ConnectorID, time_t> m_connectorHB;
};



class CStressClientHandler
{
public:
	CStressClientHandler()
	{
		CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CStressClientHandler::OnConnected, this, std::placeholders::_1));
		CMessageDispatcher::getRef().RegisterConnectorMessage(ID_AS2C_AuthAck,
			std::bind(&CStressClientHandler::msg_AuthAck_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		CMessageDispatcher::getRef().RegisterConnectorMessage(ID_LS2C_GetAccountInfoAck,
			std::bind(&CStressClientHandler::msg_GetAccountInfoAck_fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CStressClientHandler::OnConnectDisconnect, this, std::placeholders::_1));
	}

	void OnConnected(ConnectorID cID)
	{
		LOGI("OnConnected. ConnectorID=" << cID);
		WriteStreamPack ws;
		ProtoAuthReq req;
		req.info.user = "zhangyawei";
		req.info.pwd = "123";
		ws << ID_C2AS_AuthReq << req;
		CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
		LOGI("OnConnected. Send AuthReq. cID=" << cID << ", user=" << req.info.user << ", pwd=" << req.info.pwd);
		g_totalSendCount++;
	};
	void OnConnectDisconnect(ConnectorID cID)
	{
		m_sessionStatus[cID] = false;
	}

	inline void msg_AuthAck_fun(ConnectorID cID, ProtocolID pID, ReadStreamPack & rs)
	{

		ProtoAuthAck ack;
		rs >> ack;
		if (ack.retCode == EC_SUCCESS)
		{
			LOGD("Auth Success. cID=" << cID);
		}
		else
		{
			LOGE("Auth Failed. cID=" << cID);
			return;
		}


		g_totalRecvCount++;
		g_totalEchoCount++;



// 		{
// 			WriteStreamPack ws;
// 			ProtoAuthReq req;
// 			req.info.user = "zhangyawei";
// 			req.info.pwd = "123";
// 			ws << ID_C2AS_AuthReq << req;
// 			CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
// 			LOGD("msg_AuthAck. Send AuthReq. cID=" << cID << ", user=" << req.info.user << ", pwd=" << req.info.pwd);
// 			g_totalSendCount++;
// 			return;
// 		}

		WriteStreamPack ws;
		ProtoGetAccountInfoReq req;
		req.accountID = ack.accountID;
		ws << ID_C2LS_GetAccountInfoReq << req;
		CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
		LOGD("msg_AuthAck. Send LoginReq. cID=" << cID << ", user=" << req.accountID);
		g_totalSendCount++;
		

	};
	inline void msg_GetAccountInfoAck_fun(ConnectorID cID, ProtocolID pID, ReadStreamPack & rs)
	{
		ProtoGetAccountInfoAck ack;
		rs >> ack;
		if (ack.retCode == EC_SUCCESS)
		{
			LOGD("getaccount Success. cID=" << cID << ", accID=" << ack.info.accID << ", diamond=" << ack.info.diamond);
			{
				WriteStreamPack ws;
				ProtoAuthReq req;
				req.info.user = "zhangyawei";
				req.info.pwd = "123";
				ws << ID_C2AS_AuthReq << req;
				CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
				LOGD("msg_GetAccountInfoAck_fun. Send AuthReq. cID=" << cID << ", user=" << req.info.user << ", pwd=" << req.info.pwd);
				g_totalSendCount++;
				return;
			}
		}
		else
		{
			LOGE("getaccount Failed. cID=" << cID);
			return;
		}
	}
private:
	std::unordered_map<ConnectorID, bool> m_sessionStatus;
};

void sigInt(int sig)
{
	LOGI("catch SIGINT.");
	CTcpSessionManager::getRef().Stop();
}


int main(int argc, char* argv[])
{

#ifndef _WIN32
	//! linux下需要屏蔽的一些信号
	signal( SIGHUP, SIG_IGN );
	signal( SIGALRM, SIG_IGN ); 
	signal( SIGPIPE, SIG_IGN );
	signal( SIGXCPU, SIG_IGN );
	signal( SIGXFSZ, SIG_IGN );
	signal( SIGPROF, SIG_IGN ); 
	signal( SIGVTALRM, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGCHLD, SIG_IGN);
#endif
	signal(SIGINT, sigInt);
	if (argc == 2 && 
		(strcmp(argv[1], "--help") == 0 
		|| strcmp(argv[1], "/?") == 0))
	{
		cout << "please input like example:" << endl;
		cout << "tcpTest maxClient" << endl;
		cout << "./tcpTest 1" << endl;
		cout << "maxClient: limit max" << endl;
		return 0;
	}
	if (argc > 1)
		g_agentIndex = atoi(argv[1]);
	if (argc > 2)
		g_maxClient = atoi(argv[2]);


	ILog4zManager::GetInstance()->Config("log.config");
	ILog4zManager::GetInstance()->Start();





	ServerConfig serverConfig;
	if (!serverConfig.Parse("../ServerConfig.xml", AgentNode, g_agentIndex))
	{
		LOGE("serverConfig.Parse failed");
		return 0;
	}
	LOGI("g_remoteIP=" << "127.0.0.1" << ", g_remotePort=" << serverConfig.getConfigListen(AgentNode).port << ", g_maxClient=" << g_maxClient);

	CTcpSessionManager::getRef().Start();
	
	

	CTcpSessionManager::getRef().CreateTimer(5000, MonitorFunc);

	//创建心跳管理handler的实例 只要创建即可, 构造函数中会注册对应事件
	CStressHeartBeatManager statusManager;

	//这里创建服务handler和客户端handler 根据启动参数不同添加不同角色.
	CStressClientHandler client;
	
	//添加多个connector.
	for (int i = 0; i < g_maxClient; ++i)
	{
		tagConnctorConfigTraits traits;
		traits.cID = i;
		traits.remoteIP = "127.0.0.1";
		traits.remotePort = serverConfig.getConfigListen(AgentNode).port;
		traits.reconnectInterval = 5000;
		traits.reconnectMaxCount = 50;
		CTcpSessionManager::getRef().AddConnector(traits);
	}
	
	//启动主循环.
	CTcpSessionManager::getRef().Run();

	return 0;
}

