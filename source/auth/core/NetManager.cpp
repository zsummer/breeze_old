#include "NetManager.h"
#include <ProtoDefine.h>
#include <ProtoAuth.h>
#include <ServerConfig.h>

CNetManager::CNetManager()
{
	//注册事件
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_2));

	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_SessionServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	//注册心跳
	CMessageDispatcher::getRef().RegisterOnMySessionHeartbeatTimer(std::bind(&CNetManager::event_OnSessionHeartbeat, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerPulse,
		std::bind(&CNetManager::msg_OnDirectServerPulse, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

bool CNetManager::Start()
{
	m_configListen.aID = 1;
	m_configListen.listenIP = GlobalFacade::getRef().getServerConfig().getConfigListen(AuthNode).ip;
	m_configListen.listenPort = GlobalFacade::getRef().getServerConfig().getConfigListen(AuthNode).port;
	m_configListen.maxSessions = 100;

	if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return false;
	}
	LOGI("CNetManager Init Success.");
	return true;
}

void CNetManager::event_OnSessionEstablished(AccepterID aID, SessionID sID)
{
	WriteStreamPack ws;
	ProtoDirectServerAuth auth;
	auth.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	auth.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_DT2OS_DirectServerAuth << auth;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
	LOGD("send ProtoDirectServerAuth aID=" << aID << ", sID=" << sID << ", node=" << auth.srcNode << ", index=" << auth.srcIndex);
}

void CNetManager::event_OnSessionDisconnect(AccepterID aID, SessionID sID)
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


void CNetManager::msg_SessionServerAuth(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	ProtoDirectServerAuth auth;
	rs >> auth;
	LOGI("msg_SessionServerAuth. aID=" << aID << ", sID=" << sID << ", Node=" << auth.srcNode << ", index=" << auth.srcIndex);


	ServerAuthSession sac;
	sac.aID = aID;
	sac.sID = sID;
	sac.node = auth.srcNode;
	sac.index = auth.srcIndex;

	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(),
		[auth](const ServerAuthSession & cas){return cas.index == auth.srcIndex; });

	if (founder != m_onlineCenter.end())
	{
		CTcpSessionManager::getRef().KickSession(founder->aID, founder->sID);
		m_onlineCenter.erase(founder);
	}
	m_onlineCenter.push_back(sac);
}

void CNetManager::msg_OnDirectServerPulse(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(), [sID](const ServerAuthSession & sas) {return sas.sID == sID; });
	if (founder != m_onlineCenter.end())
	{
		LOGD("msg_OnDirectServerPulse lastActiveTime=" << founder->lastActiveTime);
		founder->lastActiveTime = time(NULL);
		return;
	}
}
void CNetManager::event_OnSessionHeartbeat(AccepterID aID, SessionID sID)
{
	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(), [sID](const ServerAuthSession & sas) {return sas.sID == sID; });
	if (founder == m_onlineCenter.end())
	{
		CTcpSessionManager::getRef().KickSession(aID, sID);
		LOGW("close session because  not found sID in online center. sID=" << sID);
		return;
	}
	if (founder->lastActiveTime + HEARTBEART_INTERVAL * 2 / 1000 < time(NULL))
	{
		CTcpSessionManager::getRef().KickSession(aID, sID);
		LOGW("close session because  not found sID in online center. sID=" << sID << ", lastActiveTime=" << founder->lastActiveTime);
		return;
	}

	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	ProtoDirectServerPulse pulse;
	pulse.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	pulse.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_DT2OS_DirectServerPulse << pulse;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
}


