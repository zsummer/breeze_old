
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

#include "../core/GlobalFacade.h"
#include <zsummerX/FrameMessageDispatch.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <ProtoDefine.h>


class CNetManager
{
public:
	CNetManager();
	//连接所有认证服务和中央服务
	bool Start();

	//
	inline void SendOrgDataToCenter(const char * orgData, unsigned int orgDataLen)
	{
		if (m_onlineCenter.empty())
		{
			LOGW("not found any center. protocol send lost");
			return;
		}
		const ServerAuthSession & sas = m_onlineCenter.at(0);
		CTcpSessionManager::getRef().SendOrgSessionData(sas.sID, orgData, orgDataLen);
	}

	void event_OnSessionPulse(SessionID sID, unsigned int pulseInterval);
	void msg_OnDirectServerPulse(SessionID sID, ProtoID pID, ReadStreamPack & rs);

	//register 
	void event_OnSessionEstablished(SessionID);
	void event_OnSessionDisconnect(SessionID);
	void msg_SessionServerAuth(SessionID sID, ProtoID pID, ReadStreamPack & rs);

private:
	tagAcceptorConfigTraits m_configListen; //保存监听配置

	std::vector<ServerAuthSession> m_onlineCenter;


};




































#endif
