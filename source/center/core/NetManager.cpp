#include "NetManager.h"

CNetManager::CNetManager()
{

	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_SessionServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	CMessageDispatcher::getRef().RegisterSessionDefaultMessage(
		std::bind(&CNetManager::msg_DefaultSessionReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//注册事件
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1));

	//注册心跳
	CMessageDispatcher::getRef().RegisterOnSessionPulse(std::bind(&CNetManager::event_OnSessionPulse, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerPulse,
		std::bind(&CNetManager::msg_OnSessionPulse, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

bool CNetManager::Start()
{
	auto connecters = GlobalFacade::getRef().getServerConfig().getConfigConnect(CenterNode);
	for (auto con : connecters)
	{
		tagConnctorConfigTraits tag;
		tag.remoteIP = con.remoteIP;
		tag.remotePort = con.remotePort;
		tag.reconnectMaxCount = 120;
		tag.reconnectInterval = 5000;
		SessionID cID = CTcpSessionManager::getRef().AddConnector(tag);
		if (cID == InvalidSeesionID)
		{
			LOGE("AddConnector failed. remoteIP=" << tag.remoteIP << ", remotePort=" << tag.remotePort);
			return false;
		}
		m_configConnect.insert(std::make_pair(cID, tag));
	}

	m_configListen.listenIP = GlobalFacade::getRef().getServerConfig().getConfigListen(CenterNode).ip;
	m_configListen.listenPort = GlobalFacade::getRef().getServerConfig().getConfigListen(CenterNode).port;
	m_configListen.maxSessions = 50;

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
	auto founder = std::find_if(m_onlineSession.begin(), m_onlineSession.end(),
		[&sID](const ServerAuthSession & sac){ return sac.sID == sID; });
	if (founder != m_onlineSession.end())
	{
		m_onlineSession.erase(founder);
	}
}


void CNetManager::msg_SessionServerAuth(SessionID sID, ProtoID pID, ReadStreamPack & rs)
{
	DT2OS_DirectServerAuth auth;
	rs >> auth;
	LOGI("msg_SessionServerAuth. sID=" << sID << ", Node=" << auth.srcNode << ", index=" << auth.srcIndex);

	ServerAuthSession sas;
	sas.sID = sID;
	sas.node = auth.srcNode;
	sas.index = auth.srcIndex;

	auto founder = std::find_if(m_onlineSession.begin(), m_onlineSession.end(),
		[auth](const ServerAuthSession & sas){ return sas.index == auth.srcIndex && sas.node == auth.srcNode; });
	if (founder != m_onlineSession.end())
	{
		m_onlineSession.erase(founder);
	}
	m_onlineSession.push_back(sas);
	int curConnectCount = 0;
	std::find_if(m_onlineSession.begin(), m_onlineSession.end(),
		[&curConnectCount](const ServerAuthSession & sas){if (IsConnectID(sas.sID)) curConnectCount++; return false; });
	if (!m_bListening &&  curConnectCount == m_configConnect.size())
	{
		if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
		{
			LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
			return;
		}
		m_bListening = true;
	}
}


void CNetManager::msg_DefaultSessionReq(SessionID sID, ProtoID pID, ReadStreamPack & rs)
{
	if (pID == ID_RT2OS_RouteToOtherServer)
	{
		RT2OS_RouteToOtherServer route;
		rs >> route;

		if (route.routerType == RT_SPECIFIED || route.routerType == RT_ANY)
		{
			auto founder = std::find_if(m_onlineSession.begin(), m_onlineSession.end(),
				[route](const ServerAuthSession & sas){return sas.node == route.dstNode && (route.routerType == RT_ANY || sas.index == route.dstIndex); });
			if (founder != m_onlineSession.end())
			{
				WriteStreamPack  ws(zsummer::proto4z::UBT_STATIC_AUTO);
				ws.AppendOriginalData(rs.GetStreamUnread(), rs.GetStreamUnreadLen());
				CTcpSessionManager::getRef().SendOrgSessionData(founder->sID, ws.GetStream(), ws.GetStreamLen());
				return;
			}
		}
		else/* if (route.routerType == RT_RAND || RT_WITHOUT_SPECIFIED || RT_BROADCAST)*/
		{
			std::vector<SessionID> IDs;
			for (const auto & s : m_onlineSession)
			{
				if (s.node == route.dstNode)
				{
					if (route.routerType == RT_WITHOUT_SPECIFIED && route.dstIndex == s.index)
					{
						continue;
					}
					else if (route.routerType == RT_RAND)
					{
						IDs.push_back(s.sID);
						continue;
					}
					WriteStreamPack  ws(zsummer::proto4z::UBT_STATIC_AUTO);
					ws.AppendOriginalData(rs.GetStreamUnread(), rs.GetStreamUnreadLen());
					CTcpSessionManager::getRef().SendOrgSessionData(s.sID, ws.GetStream(), ws.GetStreamLen());
				}
			}
			if (route.routerType == RT_RAND && !IDs.empty())
			{
				size_t rIndex = rand() % IDs.size();
				WriteStreamPack  ws(zsummer::proto4z::UBT_STATIC_AUTO);
				ws.AppendOriginalData(rs.GetStreamUnread(), rs.GetStreamUnreadLen());
				CTcpSessionManager::getRef().SendOrgSessionData(IDs[rIndex], ws.GetStream(), ws.GetStreamLen());
			}
		}
	}
	else
	{
		LOGD("unknow protoID. pID=" << pID);
	}
};


void CNetManager::event_OnSessionPulse(SessionID sID, unsigned int pulseInterval)
{
	auto founder = std::find_if(m_onlineSession.begin(), m_onlineSession.end(), [sID](const ServerAuthSession & sas) {return sas.sID == sID; });
	if (founder == m_onlineSession.end())
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

void CNetManager::msg_OnSessionPulse(SessionID sID, ProtoID pID, ReadStreamPack & rs)
{
	auto founder = std::find_if(m_onlineSession.begin(), m_onlineSession.end(), [sID](const ServerAuthSession & sas) {return sas.sID == sID; });
	if (founder != m_onlineSession.end())
	{
		LOGD("msg_OnDirectServerPulse lastActiveTime=" << founder->lastActiveTime);
		founder->lastActiveTime = time(NULL);
		return;
	}
}


