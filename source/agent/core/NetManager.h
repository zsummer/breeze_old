﻿
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

#ifndef _NET_MANAGER_H_
#define _NET_MANAGER_H_
#include <ServerConfig.h>
#include <ProtoDefine.h>
#include <ProtoCommon.h>
#include <InProtoCommon.h>
#include <ProtoAuth.h>
#include "GlobalFacade.h"

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

	void event_OnSessionEstablished(AccepterID, SessionID);
	void event_OnSessionDisconnect(AccepterID, SessionID);

	void msg_ConnectServerAuth(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs);

	void msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);
	void msg_AuthAck(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs);

	void msg_DefaultConnectReq(ConnectorID cID, ProtocolID pID, ReadStreamPack & rs);
	void msg_DefaultSessionReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);

	bool msg_OrgMessageReq(AccepterID aID, SessionID sID, const char * blockBegin, FrameStreamTraits::Integer blockSize);


private:
	std::unordered_map<SessionID, std::shared_ptr<AgentSessionInfo>> m_mapSession;
	std::unordered_map<AccountID, std::shared_ptr<AgentSessionInfo>> m_mapAccount;
	std::unordered_map<CharacterID, std::shared_ptr<AgentSessionInfo>> m_mapChar;

	tagAcceptorConfigTraits m_configListen; //保存监听配置

	ConnectorID m_lastConnectID = 0; //自动递增的connectorID.
	std::unordered_map<ConnectorID, tagConnctorConfigTraits> m_configCenter;  //cID 对应的连接配置

	std::vector<ServerAuthConnect> m_onlineCenter; //在线的中心服务, 主备关系 不均衡

	char m_chunkWriteStream[SEND_RECV_CHUNK_SIZE];
};




































#endif