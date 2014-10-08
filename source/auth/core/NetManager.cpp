#include "NetManager.h"
#include <ProtoDefine.h>
#include <ProtoAuth.h>
#include <ServerConfig.h>

CNetManager::CNetManager()
{
	//注册事件
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1));

	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_SessionServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//注册心跳
	CMessageDispatcher::getRef().RegisterOnSessionPulse(std::bind(&CNetManager::event_OnSessionPulse, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerPulse,
		std::bind(&CNetManager::msg_OnDirectServerPulse, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

bool CNetManager::Start()
{
	m_configListen.listenIP = GlobalFacade::getRef().getServerConfig().getConfigListen(AuthNode).ip;
	m_configListen.listenPort = GlobalFacade::getRef().getServerConfig().getConfigListen(AuthNode).port;
	m_configListen.maxSessions = 100;

	if (!CTcpSessionManager::getRef().AddAcceptor(m_configListen))
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return false;
	}
	LOGI("CNetManager Init Success.");
	return true;
}

void CNetManager::event_OnSessionEstablished(SessionID sID)
{
	WriteStreamPack ws;
	DT2OS_DirectServerAuth auth;
	auth.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	auth.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_DT2OS_DirectServerAuth << auth;
	CTcpSessionManager::getRef().SendOrgSessionData(sID, ws.GetStream(), ws.GetStreamLen());
	LOGD("send DT2OS_DirectServerAuth sID=" << sID << ", node=" << auth.srcNode << ", index=" << auth.srcIndex);
}

void CNetManager::event_OnSessionDisconnect(SessionID sID)
{
	LOGW("event_OnSessionDisconnect sID=" << sID);
	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(),
		[sID](const ServerAuthSession & sac){ return sac.sID == sID; });
	if (founder == m_onlineCenter.end())
	{
		LOGW("event_OnSessionDisconnect not found the sID=" << sID);
		return;
	}
	m_onlineCenter.erase(founder);
}


void CNetManager::msg_SessionServerAuth(SessionID sID, ProtoID pID, ReadStreamPack & rs)
{
	DT2OS_DirectServerAuth auth;
	rs >> auth;
	LOGI("msg_SessionServerAuth. sID=" << sID << ", Node=" << auth.srcNode << ", index=" << auth.srcIndex);


	ServerAuthSession sac;
	sac.sID = sID;
	sac.node = auth.srcNode;
	sac.index = auth.srcIndex;

	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(),
		[auth](const ServerAuthSession & cas){return cas.index == auth.srcIndex; });

	if (founder != m_onlineCenter.end())
	{
		CTcpSessionManager::getRef().KickSession(founder->sID);
		m_onlineCenter.erase(founder);
	}
	m_onlineCenter.push_back(sac);
}

void CNetManager::msg_OnDirectServerPulse(SessionID sID, ProtoID pID, ReadStreamPack & rs)
{
	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(), [sID](const ServerAuthSession & sas) {return sas.sID == sID; });
	if (founder != m_onlineCenter.end())
	{
		LOGD("msg_OnDirectServerPulse lastActiveTime=" << founder->lastActiveTime);
		founder->lastActiveTime = time(NULL);
		return;
	}
}
void CNetManager::event_OnSessionPulse(SessionID sID, unsigned int pulseInterval)
{
	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(), [sID](const ServerAuthSession & sas) {return sas.sID == sID; });
	if (founder == m_onlineCenter.end())
	{
		CTcpSessionManager::getRef().KickSession(sID);
		LOGW("close session because  not found sID in online center. sID=" << sID);
		return;
	}
	if (founder->lastActiveTime + pulseInterval * 10 / 1000 < time(NULL))
	{
		CTcpSessionManager::getRef().KickSession(sID);
		LOGW("close session because  not found sID in online center. sID=" << sID << ", lastActiveTime=" << founder->lastActiveTime);
		return;
	}

	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	DT2OS_DirectServerPulse pulse;
	pulse.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	pulse.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_DT2OS_DirectServerPulse << pulse;
	CTcpSessionManager::getRef().SendOrgSessionData(sID, ws.GetStream(), ws.GetStreamLen());
}


