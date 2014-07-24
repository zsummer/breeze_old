#include "NetManager.h"

CNetManager::CNetManager()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
		std::bind(&CNetManager::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionDefaultMessage(
		std::bind(&CNetManager::msg_DefaultReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionOrgMessage(
		std::bind(&CNetManager::msg_OrgMessageReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));


	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_AS2C_AuthAck,
		std::bind(&CNetManager::msg_AuthAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


	//注册事件
	CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CNetManager::event_OnConnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CNetManager::event_OnDisconnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_1));
}

bool CNetManager::Start()
{

	auto connecters = GlobalFacade::getRef().getServerConfig().getAgentConnect();
	for (auto con : connecters)
	{
		tagConnctorConfigTraits tag;
		tag.cID = m_lastConnectID++;
		tag.remoteIP = con.remoteIP;
		tag.remotePort = con.remotePort;
		tag.reconnectMaxCount = 720;
		tag.reconnectInterval = 5000;
		tag.curReconnectCount = true;
		if (con.dstServer == CenterNode)
		{
			m_configCenter.insert(std::make_pair(tag.cID, tag));
		}
		else if (con.dstServer == AuthNode)
		{
			m_configAuth.insert(std::make_pair(tag.cID, tag));
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
	m_configListen.listenIP = GlobalFacade::getRef().getServerConfig().getAgentListen().ip;
	m_configListen.listenPort = GlobalFacade::getRef().getServerConfig().getAgentListen().port;
	m_configListen.maxSessions = 5000;

	LOGI("CNetManager Init Success.");
	return true;
}


void CNetManager::event_OnConnect(ConnectorID cID)
{
	auto founder = m_configAuth.find(cID);
	if (founder != m_configAuth.end())
	{
		m_onlineAuth.push_back(cID);
		LOGI("event_OnConnect Auth. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);

	}
	founder = m_configCenter.find(cID);
	if (founder != m_configCenter.end())
	{
		m_onlineCenter.push_back(cID);
		LOGI("event_OnConnect Center. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
	}
// 	if (m_configAuth.size() + m_configCenter.size() != m_onlineAuth.size() + m_onlineCenter.size())
// 	{
// 		return;
// 	}

	//init
	WriteStreamPack ws;
	ws << ID_PROTO_SERVER_INIT << AgentNode << GlobalFacade::getRef().getServerConfig().getAgentListen().index;
	CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());

	//所有connector已经建立连接 此时打开客户端监听端口
	if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return;
	}
	LOGE("AddAcceptor Success. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
}
void CNetManager::event_OnDisconnect(ConnectorID cID)
{
	auto founder = m_configAuth.find(cID);
	if (founder != m_configAuth.end())
	{
		auto searchiter = std::find_if(m_onlineAuth.begin(), m_onlineAuth.end(), [&cID](ConnectorID ccID){ return ccID == cID; });
		if (searchiter != m_onlineAuth.end())
		{
			m_onlineAuth.erase(searchiter);
		}
		LOGW("event_OnDisconnect Auth. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
	}
	founder = m_configCenter.find(cID);
	if (founder != m_configCenter.end())
	{
		auto searchiter = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(), [&cID](ConnectorID ccID){ return ccID == cID; });
		if (searchiter != m_onlineCenter.end())
		{
			m_onlineCenter.erase(searchiter);
		}
		LOGW("event_OnDisconnect Center. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
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

	m_mapSession.erase(founder);
}






void CNetManager::msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	ProtoAuthReq req;
	rs >> req;
	LOGD("ID_C2AS_AuthReq user=" << req.info.user << ", pwd=" << req.info.pwd);
// 	//debug
	m_mapSession.erase(sID);
// 	//end
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
	std::shared_ptr<SessionInfo> sinfo(new SessionInfo);
	sinfo->accountID = InvalidAccountID;
	sinfo->accountName = req.info.user;
	sinfo->charID = InvalidCharacterID;
	sinfo->agentID = GlobalFacade::getRef().getServerConfig().getAgentListen().index;
	sinfo->accepterID = aID;
	sinfo->sessionID = sID;
	sinfo->lastLoginTime = time(nullptr);
	m_mapSession.insert(std::make_pair(sID, sinfo));

	if (m_onlineAuth.empty())
	{
		WriteStreamPack ws;
		ProtoAuthAck ack;
		ack.retCode = EC_SERVER_ERROR;
		ws << ID_AS2C_AuthAck << ack;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
		return;
	}

	WriteStreamPack ws;
	ws << ID_C2AS_AuthReq << *sinfo << req ;
	auto authID = m_onlineAuth.at(rand() % m_onlineAuth.size());
	CTcpSessionManager::getRef().SendOrgConnectorData(authID, ws.GetStream(), ws.GetStreamLen());
}

void CNetManager::msg_AuthAck(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	SessionInfo info;
	ProtoAuthAck ack;
	rs >> info;
	rs >> ack;

	auto founder = m_mapSession.find(info.sessionID);
	if (founder == m_mapSession.end())
	{
		LOGE("msg_AuthAck can not found session ID.  sID=" << info.sessionID);
		return;
	}

	if (ack.retCode == EC_SUCCESS)
	{
		founder->second->accountID = ack.accountID;
	}

	WriteStreamPack ws;
	ws << ID_AS2C_AuthAck << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(founder->second->accepterID, founder->second->sessionID, ws.GetStream(), ws.GetStreamLen());
}

bool CNetManager::msg_OrgMessageReq(AccepterID aID, SessionID sID, const char * blockBegin,  FrameStreamTraits::Integer blockSize)
{
	ReadStreamPack pack(blockBegin, blockSize);
	ProtocolID protoID = InvalidProtocolID;
	pack >> protoID;
	if (isClientPROTO(protoID) && isNeedAuthClientPROTO(protoID))
	{
		auto finditer = m_mapSession.find(sID);
		if (finditer == m_mapSession.end() || finditer->second->accountID == InvalidAccountID)
		{
			LOGW("msg_OrgMessageReq check false. sID=" << sID);
			return false;
		}
	}
	return true;
};

void CNetManager::msg_DefaultReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	auto finditer = m_mapSession.find(sID);
	ProtoAuthAck ack;
	ack.retCode = EC_SUCCESS;
	ack.accountID = InvalidAccountID;
	if (finditer == m_mapSession.end() || finditer->second->accountID == InvalidAccountID)
	{
		ack.retCode = EC_AUTH_ERROR;
	}
	else
	{
		ProtocolID inProtoID = InvalidProtocolID;
		if (isNeedAuthClientPROTO(pID))
		{
			if (isRouteToCenter(pID))
			{
				inProtoID = ID_PROTO_SERVER_ROUTE_CENTER;
			}
			else if (isRouteToBattle(pID))
			{
				inProtoID = ID_PROTO_SERVER_ROUTE_BATTLE;
			}
			else if (isRouteToLogic(pID))
			{
				inProtoID = ID_PROTO_SERVER_ROUTE_LOGIC;
			}
			else if (isRouteToDBAgent(pID))
			{
				inProtoID = ID_PROTO_SERVER_ROUTE_DBAGENT;
			}
		}
		if (inProtoID != InvalidProtocolID)
		{
			WriteStreamPack ws;
			ws << inProtoID << *finditer->second << pID;
			ws.AppendOriginalData(rs.GetStreamUnread(), rs.GetStreamUnreadLen());
			CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
			return;
		}
	}


	if (ack.retCode != EC_SUCCESS)
	{
		WriteStreamPack ws;
		ws << ID_AS2C_AuthAck << ack;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
	}

};


