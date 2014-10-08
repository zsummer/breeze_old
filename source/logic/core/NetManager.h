
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
#include <ProtoAuth.h>
#include "../core/GlobalFacade.h"
#include <zsummerX/FrameMessageDispatch.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <unordered_map>



/*
 *  文件说明
 *  网络管理类
 *  提供服务节点的网络模块配置启动,维护网络的连接/断开,心跳脉冲, 节点注册, 默认消息处理等.
 *  提供所有人较原始的网络访问接口.
*/

class CNetManager
{
private:
	std::vector<ServerAuthSession> m_onlineSession;
public:
	CNetManager();
	bool Start();
	void event_OnSessionEstablished(SessionID sID);
	void event_OnSessionDisconnect(SessionID sID);

	void msg_SessionServerAuth(SessionID sID, ProtoID pID, ReadStreamPack & rs);
	void event_OnSessionPulse(SessionID sID, unsigned int pulseInterval);
	void msg_OnDirectServerPulse(SessionID sID, ProtoID pID, ReadStreamPack & rs);
	void SendOrgDataToCenter(const char * orgData, unsigned int orgDataLen);
private:
	tagAcceptorConfigTraits m_configListen; //保存监听配置
	std::unordered_map<SessionID, tagConnctorConfigTraits> m_configConnect; //cID 对应的连接配置
};




//helper
inline void CNetManager::SendOrgDataToCenter(const char * orgData, unsigned int orgDataLen)
{
	if (m_onlineSession.empty())
	{
		LOGW("not found any center. protocol send lost");
		return;
	}
	const ServerAuthSession & sas = m_onlineSession.at(0);
	CTcpSessionManager::getRef().SendOrgSessionData(sas.sID, orgData, orgDataLen);
}





























#endif
