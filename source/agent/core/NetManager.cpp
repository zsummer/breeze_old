#include "NetManager.h"


CNetManager::CNetManager()
{
	//注册来自客户端的默认协议处理逻辑
	CMessageDispatcher::getRef().RegisterSessionDefaultMessage(
		std::bind(&CNetManager::msg_DefaultSessionReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	//注册来自服务器内部默认协议处理逻辑
	CMessageDispatcher::getRef().RegisterConnectorDefaultMessage(
		std::bind(&CNetManager::msg_DefaultConnectReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//服务器内部的节点注册
	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_ConnectServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	//认证逻辑 支持帐号多次认证
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
		std::bind(&CNetManager::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_AS2C_AuthAck,
		std::bind(&CNetManager::msg_AuthAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_LS2AS_KickCharacter,
		std::bind(&CNetManager::msg_KickCharacter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//注册事件
	CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CNetManager::event_OnConnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CNetManager::event_OnDisconnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_2));

	//注册心跳
	CMessageDispatcher::getRef().RegisterOnMySessionHeartbeatTimer(std::bind(&CNetManager::event_OnSessionHeartbeat, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterOnMyConnectorHeartbeatTimer(std::bind(&CNetManager::event_OnConnectorHeartbeat, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_ClientPulse,
		std::bind(&CNetManager::msg_OnClientPulse, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_DT2OS_DirectServerPulse,
		std::bind(&CNetManager::msg_OnDirectServerPulse, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

bool CNetManager::Start()
{

	auto connecters = GlobalFacade::getRef().getServerConfig().getConfigConnect(AgentNode);
	for (auto con : connecters)
	{
		tagConnctorConfigTraits tag;
		tag.cID = m_lastConnectID++;
		tag.remoteIP = con.remoteIP;
		tag.remotePort = con.remotePort;
		tag.reconnectMaxCount = 120;
		tag.reconnectInterval = 5000;
		if (con.dstNode == CenterNode)
		{
			m_configCenter.insert(std::make_pair(tag.cID, tag));
		}
		else
		{
			continue;
		}
		if (CTcpSessionManager::getRef().AddConnector(tag) == InvalidConnectorID)
		{
			LOGE("AddConnector failed. remoteIP=" << tag.remoteIP << ", remotePort=" << tag.remotePort);
			return false;
		}
	}

	m_configListen.aID = 1;
	m_configListen.listenIP = GlobalFacade::getRef().getServerConfig().getConfigListen(AgentNode).ip;
	m_configListen.listenPort = GlobalFacade::getRef().getServerConfig().getConfigListen(AgentNode).port;
	m_configListen.maxSessions = 5000;

	LOGI("CNetManager Init Success.");
	return true;
}


void CNetManager::event_OnConnect(ConnectorID cID)
{
	//auth
	WriteStreamPack ws;
	ProtoDirectServerAuth auth;
	auth.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	auth.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_DT2OS_DirectServerAuth << auth;
	CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
	LOGI("send ProtoDirectServerAuth cID=" << cID << ", node=" << auth.srcNode << ", index=" << auth.srcIndex)
}


void CNetManager::event_OnDisconnect(ConnectorID cID)
{
	
	auto founder = m_configCenter.find(cID);
	if (founder != m_configCenter.end())
	{
		LOGW("event_OnDisconnect Center Server. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
	}


	{
		auto finder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(),
			[cID](const ServerAuthConnect & sac){ return sac.cID == cID; });
		if (finder != m_onlineCenter.end())
		{
			m_onlineCenter.erase(finder);
			LOGW("event_OnDisconnect Center Server erase. cID=" << cID << ", current online Center count=" << m_onlineCenter.size());
		}
	}
}


void CNetManager::event_OnSessionEstablished(AccepterID, SessionID)
{

}

void CNetManager::event_OnSessionDisconnect(AccepterID aID, SessionID sID)
{
	auto founder = m_mapSession.find(sID);
	if (founder == m_mapSession.end())
	{
		return;
	}
	founder->second->sInfo.aID = InvalidAccepterID;
	founder->second->sInfo.sID = InvalidSeesionID;
	m_mapSession.erase(founder);
}



void CNetManager::msg_ConnectServerAuth(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	ProtoDirectServerAuth auth;
	rs >> auth;
	LOGI("msg_ConnectServerAuth. cID=" << cID << ", Node=" << auth.srcNode << ", index=" << auth.srcIndex);

	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(),
		[auth](const ServerAuthConnect & sac){ return sac.index == auth.srcIndex; });
	if (founder != m_onlineCenter.end())
	{
		CTcpSessionManager::getRef().BreakConnector(founder->cID);
		m_onlineCenter.erase(founder);
	}
	ServerAuthConnect sac;
	sac.node = auth.srcNode;
	sac.index = auth.srcIndex;
	sac.cID = cID;
	m_onlineCenter.push_back(sac);


	if (m_bListening || m_configCenter.size() !=  m_onlineCenter.size())
	{
		return;
	}

	//所有connector已经建立连接成功 并且是程序启动时的第一次 此时打开客户端监听端口
	if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return;
	}
	m_bListening = true;
	LOGI("AddAcceptor Success. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
}



void CNetManager::msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	ProtoAuthReq req;
	rs >> req;
	LOGD("ID_C2AS_AuthReq user=" << req.info.user << ", pwd=" << req.info.pwd);
	if (m_onlineCenter.empty())
	{
		WriteStreamPack ws;
		ProtoAuthAck ack;
		ack.retCode = EC_SERVER_ERROR;
		ws << ID_AS2C_AuthAck << ack;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
		return;
	}

	auto finditer = m_mapSession.find(sID);
	if (finditer != m_mapSession.end())
	{
		WriteStreamPack ws;
		ProtoAuthAck ack;
		ack.retCode = EC_AUTH_ING;
		ws << ID_AS2C_AuthAck << ack;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
		return;
	}

	std::shared_ptr<AgentSessionInfo> sinfo(new AgentSessionInfo);
	sinfo->sInfo.agentIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	sinfo->sInfo.srcNode = AgentNode;
	sinfo->sInfo.srcIndex = sinfo->sInfo.agentIndex;
	sinfo->sInfo.aID = aID;
	sinfo->sInfo.sID = sID;
	m_mapSession.insert(std::make_pair(sID, sinfo));



	
	//route
	ProtocolID inProtoID = ID_RT2OS_RouteToOtherServer;
	ProtoRouteToOtherServer route;
	route.dstNode = AuthNode;
	route.routerType = 1;
	route.dstIndex = 0;
	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	ws << inProtoID << route << ID_C2AS_AuthReq << sinfo->sInfo << req;
	CTcpSessionManager::getRef().SendOrgConnectorData(m_onlineCenter.at(0).cID, ws.GetStream(), ws.GetStreamLen());
}

void CNetManager::msg_AuthAck(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	SessionInfo info;
	ProtoAuthAck ack;
	rs >> info;
	rs >> ack;



	if (ack.retCode == EC_SUCCESS)
	{
		auto founder = m_mapSession.find(info.sID);
		if (founder == m_mapSession.end())
		{
			LOGE("msg_AuthAck can not found session ID.  sID=" << info.sID);
			return;
		}
		founder->second->sInfo.accID = ack.accountID;
		m_mapAccount[ack.accountID] = founder->second;
	}

	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	ws << ID_AS2C_AuthAck << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(info.aID, info.sID, ws.GetStream(), ws.GetStreamLen());
}

void CNetManager::msg_KickCharacter(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	SessionInfo info;
	rs >> info;
	auto fouder = m_mapChar.find(info.charID);
	if (fouder == m_mapChar.end())
	{
		if (info.agentIndex != GlobalFacade::getRef().getServerConfig().getOwnNodeIndex())
		{
			return;
		}
		
		auto fouderSession = m_mapSession.find(info.sID);
		if (fouderSession == m_mapSession.end())
		{
			return;
		}
		
		if (fouderSession->second->sInfo.accID != info.accID)
		{
			return;
		}
		m_mapChar.erase(fouderSession->second->sInfo.charID);
		fouderSession->second->sInfo.charID = info.charID;
		m_mapChar.insert(std::make_pair(info.charID, fouderSession->second));
	}
	else
	{
		SessionInfo & fInfo = fouder->second->sInfo;
		if (fInfo.agentIndex != GlobalFacade::getRef().getServerConfig().getOwnNodeIndex()
			|| fInfo.sID != info.sID)
		{
			fInfo.charID = InvalidCharacterID;
			CTcpSessionManager::getRef().KickSession(fInfo.aID, fInfo.sID);
		}
	}
	
}

void CNetManager::event_OnSessionHeartbeat(AccepterID aID, SessionID sID)
{
	auto founder = m_mapSession.find(sID);
	if (founder == m_mapSession.end())
	{
		CTcpSessionManager::getRef().KickSession(aID, sID);
		LOGW("kick session because session not found in m_mapSession. aID=" << aID << ", sID=" << sID);
		return;
	}
	if (founder->second->lastActiveTime + HEARTBEART_INTERVAL * 2 / 1000 < time(NULL))
	{
		CTcpSessionManager::getRef().KickSession(aID, sID);
		LOGW("kick session because session heartbeat timeout. aID=" << aID << ", sID=" << sID << ", lastActiveTime=" << founder->second->lastActiveTime);
		return;
	}

}

void CNetManager::event_OnConnectorHeartbeat(ConnectorID cID)
{
	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(), [cID](const ServerAuthConnect & sac) {return sac.cID == cID; });
	if (founder == m_onlineCenter.end())
	{
		CTcpSessionManager::getRef().BreakConnector(cID);
		LOGW("break connector because the connector not founder in online center. cID=" << cID);
		return;
	}
	if (founder->lastActiveTime + HEARTBEART_INTERVAL * 2 / 1000 < time(NULL))
	{
		CTcpSessionManager::getRef().BreakConnector(cID);
		LOGW("break connector because the connector heartbeat timeout. cID=" << cID << ", lastActiveTime=" << founder->lastActiveTime);
		return;
	}
	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	ProtoDirectServerPulse pulse;
	pulse.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	pulse.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();

	ws << ID_DT2OS_DirectServerPulse << pulse;
	CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());
}

void CNetManager::msg_OnDirectServerPulse(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(), [cID](const ServerAuthConnect & sac) {return sac.cID == cID; });
	if (founder != m_onlineCenter.end())
	{
		LOGD("msg_OnDirectServerPulse lastActiveTime=" << founder->lastActiveTime);
		founder->lastActiveTime = time(NULL);
		return;
	}
}
void CNetManager::msg_OnClientPulse(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	auto founder = m_mapSession.find(sID);
	if (founder != m_mapSession.end())
	{
		founder->second->lastActiveTime = time(NULL);
		LOGD("msg_OnClientPulse lastActiveTime=" << founder->second->lastActiveTime);
		return;
	}
	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	ProtoClientPulseAck ack;
	ack.svrTimeStamp = time(NULL);
	ws << ID_AS2C_ClientPulseAck << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
}


void CNetManager::msg_DefaultConnectReq(ConnectorID cID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	rs >> info;
	if (info.aID != InvalidAccepterID && info.sID != InvalidSeesionID)
	{
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
		ws << pID;
		ws.AppendOriginalData(rs.GetStreamUnread(), rs.GetStreamUnreadLen());
		CTcpSessionManager::getRef().SendOrgSessionData(info.aID, info.sID, ws.GetStream(), ws.GetStreamLen());
	}
}
void CNetManager::msg_DefaultSessionReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	auto finditer = m_mapSession.find(sID);

	if (finditer == m_mapSession.end() || finditer->second->sInfo.accID == InvalidAccountID)
	{
		ProtoAuthAck ack;
		ack.retCode = EC_AUTH_NOT_FOUND;
		ack.accountID = InvalidAccountID;
		if (ack.retCode != EC_SUCCESS)
		{
			WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
			ws << ID_AS2C_AuthAck << ack;
			CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
		}
	}
	else
	{
		ProtocolID inProtoID = InvalidProtocolID;
		if (isNeedAuthClientPROTO(pID) && !m_onlineCenter.empty())
		{
			inProtoID = ID_RT2OS_RouteToOtherServer;
			ProtoRouteToOtherServer route;
			route.dstNode = LogicNode;
			route.routerType = 1;
			route.dstIndex = 0;
			
			WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
			ws << inProtoID << route << pID << finditer->second->sInfo;
			ws.AppendOriginalData(rs.GetStreamUnread(), rs.GetStreamUnreadLen());
			CTcpSessionManager::getRef().SendOrgConnectorData(m_onlineCenter.at(0).cID,  ws.GetStream(), ws.GetStreamLen());
			return;
		}
	}



};


