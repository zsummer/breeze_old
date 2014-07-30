#include "NetManager.h"
#include <InProtoCommon.h>
#include <ServerConfig.h>

CNetManager::CNetManager()
{
	CMessageDispatcher::getRef().RegisterSessionDefaultMessage(
		std::bind(&CNetManager::msg_DefaultReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionOrgMessage(
		std::bind(&CNetManager::msg_OrgMessageReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_ConnectServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
		std::bind(&CNetManager::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_AS2C_AuthAck,
		std::bind(&CNetManager::msg_AuthAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


	//注册事件
	CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CNetManager::event_OnConnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CNetManager::event_OnDisconnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_2));
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
		tag.reconnectMaxCount = 2;
		tag.reconnectInterval = 5000;
		tag.curReconnectCount = true;
		if (con.dstNode == CenterNode)
		{
			m_configCenter.insert(std::make_pair(tag.cID, tag));
		}
		else if (con.dstNode == AuthNode)
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
	auto founder = m_configAuth.find(cID);
	if (founder != m_configAuth.end())
	{
		LOGW("event_OnDisconnect Auth Server. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
	}
	founder = m_configCenter.find(cID);
	if (founder != m_configCenter.end())
	{
		LOGW("event_OnDisconnect Center Server. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
	}

	{
		auto finder = std::find_if(m_onlineAuth.begin(), m_onlineAuth.end(),
			[cID](const ServerAuthConnect & sac){ return sac.cID == cID; });
		if (finder != m_onlineAuth.end())
		{
			m_onlineAuth.erase(finder);
			LOGW("event_OnDisconnect Auth Server erase. cID=" << cID << ", current online Auth count=" << m_onlineAuth.size());
		}
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
	m_mapSession.erase(founder);
}



void CNetManager::msg_ConnectServerAuth(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	ProtoDirectServerAuth auth;
	rs >> auth;
	LOGI("msg_ConnectServerAuth. cID=" << cID << ", Node=" << auth.srcNode << ", index=" << auth.srcIndex);


	if (auth.srcNode == AuthNode)
	{
		auto founder = std::find_if(m_onlineAuth.begin(), m_onlineAuth.end(),
			[auth](const ServerAuthConnect & sac){ return sac.index == auth.srcIndex; });
		if (founder != m_onlineAuth.end())
		{
			CTcpSessionManager::getRef().BreakConnector(founder->cID);
			m_onlineAuth.erase(founder);
		}
		ServerAuthConnect sac;
		sac.node = auth.srcNode;
		sac.index = auth.srcIndex;
		sac.cID = cID;
		m_onlineAuth.push_back(sac);
	}
	else if (auth.srcNode == CenterNode)
	{
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
		return;
	}

	if (m_configAuth.size() + m_configCenter.size() != m_onlineAuth.size() + m_onlineCenter.size())
	{
		return;
	}

	//所有connector已经建立连接成功 并且是程序启动时的第一次 此时打开客户端监听端口
	if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return;
	}
	LOGI("AddAcceptor Success. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
}



void CNetManager::msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	ProtoAuthReq req;
	rs >> req;
	LOGD("ID_C2AS_AuthReq user=" << req.info.user << ", pwd=" << req.info.pwd);
// 	//debug 可以重复认证 做认证压测
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
	std::shared_ptr<AgentSessionInfo> sinfo(new AgentSessionInfo);
	sinfo->sInfo.agentIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	sinfo->sInfo.aID = aID;
	sinfo->sInfo.sID = sID;
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
	ws << ID_C2AS_AuthReq << sinfo->sInfo << req ;
	auto authServer = m_onlineAuth.at(rand() % m_onlineAuth.size());
	CTcpSessionManager::getRef().SendOrgConnectorData(authServer.cID, ws.GetStream(), ws.GetStreamLen());
}

void CNetManager::msg_AuthAck(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	SessionInfo info;
	ProtoAuthAck ack;
	rs >> info;
	rs >> ack;

	auto founder = m_mapSession.find(info.sID);
	if (founder == m_mapSession.end())
	{
		LOGE("msg_AuthAck can not found session ID.  sID=" << info.sID);
		return;
	}

	if (ack.retCode == EC_SUCCESS)
	{
		founder->second->sInfo.accID = ack.accountID;
	}

	WriteStreamPack ws;
	ws << ID_AS2C_AuthAck << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(founder->second->sInfo.aID, founder->second->sInfo.sID, ws.GetStream(), ws.GetStreamLen());
}

bool CNetManager::msg_OrgMessageReq(AccepterID aID, SessionID sID, const char * blockBegin,  FrameStreamTraits::Integer blockSize)
{
	ReadStreamPack pack(blockBegin, blockSize);
	ProtocolID protoID = InvalidProtocolID;
	pack >> protoID;
	if (isClientPROTO(protoID) && isNeedAuthClientPROTO(protoID))
	{
		auto finditer = m_mapSession.find(sID);
		if (finditer == m_mapSession.end() || finditer->second->sInfo.accID == InvalidAccountID)
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
	if (finditer == m_mapSession.end() || finditer->second->sInfo.accID == InvalidAccountID)
	{
		ack.retCode = EC_AUTH_ERROR;
	}
	else
	{
		ProtocolID inProtoID = InvalidProtocolID;
		if (isNeedAuthClientPROTO(pID))
		{
			inProtoID = ID_RT2OS_RouteToOtherServer;
			ProtoRouteToOtherServer route;
			route.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
			route.srcServer = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
			route.dstServer = LogicNode;
			route.routerType = 3;
			route.dstIndex = 0;
			WriteStreamPack ws;
			ws << inProtoID << route << pID << finditer->second->sInfo;
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


