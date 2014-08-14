
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


/*
*  文件说明
*  网络管理类
*  提供服务节点的网络模块配置启动,维护网络的连接/断开,心跳脉冲, 节点注册, 默认消息处理等.
*  提供所有人较原始的网络访问接口.
*/



#ifndef _NET_MANAGER_H_
#define _NET_MANAGER_H_
#include <ServerConfig.h>
#include <ProtoDefine.h>
#include <ProtoAuth.h>
#include "../core/GlobalFacade.h"
#include <zsummerX/FrameMessageDispatch.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <unordered_map>
/*
* NetManager
*/


class CNetManager
{
public:
	CNetManager();
	//连接所有认证服务和中央服务
	bool Start();
	void event_OnConnect(ConnectorID cID);
	void event_OnDisconnect(ConnectorID cID);

	void event_OnSessionEstablished(AccepterID aID, SessionID sID);
	void event_OnSessionDisconnect(AccepterID aID, SessionID sID);

	void msg_ConnectServerAuth(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs);
	void msg_SessionServerAuth(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);


	void msg_DefaultConnectReq(ConnectorID cID, ProtocolID pID, ReadStreamPack & rs);
	void msg_DefaultSessionReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);
	

	bool msg_OrgMessageReq(AccepterID aID, SessionID sID, const char * blockBegin, FrameStreamTraits::Integer blockSize);

	void event_OnSessionHeartbeat(AccepterID aID, SessionID sID);
	void event_OnConnectorHeartbeat(ConnectorID cID);
	void msg_OnConnectorPulse(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs);
	void msg_OnSessionPulse(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);

protected:
	void msg_TranslateToOtherServer(ProtocolID pID, ReadStreamPack & rs);
private:
	tagAcceptorConfigTraits m_configListen; //保存监听配置
	bool m_bListening = false;
	ConnectorID m_lastConnectID = 0; //自动递增的connectorID.
	std::unordered_map<ConnectorID, tagConnctorConfigTraits> m_configConnect; //cID 对应的连接配置

	std::vector<ServerAuthSession> m_onlineSession;
	std::vector<ServerAuthConnect> m_onlineConnect;
};




































#endif
