#include "NetManager.h"

CNetManager::CNetManager()
{

	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_SessionServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_ConnectServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	CMessageDispatcher::getRef().RegisterSessionDefaultMessage(
		std::bind(&CNetManager::msg_DefaultReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionOrgMessage(
		std::bind(&CNetManager::msg_OrgMessageReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));


	//×¢²áÊÂ¼þ
	CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CNetManager::event_OnConnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CNetManager::event_OnDisconnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_2));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_2));
}

bool CNetManager::Start()
{

	auto connecters = GlobalFacade::getRef().getServerConfig().getConfigConnect(CenterNode);
	for (auto con : connecters)
	{
		tagConnctorConfigTraits tag;
		tag.cID = m_lastConnectID++;
		tag.remoteIP = con.remoteIP;
		tag.remotePort = con.remotePort;
		tag.reconnectMaxCount = 2;
		tag.reconnectInterval = 5000;
		tag.curReconnectCount = true;
		if (con.dstNode == DBAgentNode)
		{
			m_configDBAgent.insert(std::make_pair(tag.cID, tag));
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
	m_configListen.listenIP = GlobalFacade::getRef().getServerConfig().getConfigListen(CenterNode).ip;
	m_configListen.listenPort = GlobalFacade::getRef().getServerConfig().getConfigListen(CenterNode).port;
	m_configListen.maxSessions = 50;

	LOGI("CNetManager Init Success.");
	return true;
}


void CNetManager::event_OnConnect(ConnectorID cID)
{
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
	auto founder = m_configDBAgent.find(cID);
	if (founder != m_configDBAgent.end())
	{
		LOGW("event_OnDisconnect Center. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
	}

	{
		auto searchiter = std::find_if(m_onlineDBAgent.begin(), m_onlineDBAgent.end(),
			[&cID](const ServerAuthConnect & sac){ return sac.cID == cID; });
		if (searchiter != m_onlineDBAgent.end())
		{
			m_onlineDBAgent.erase(searchiter);
		}
	}
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
	{
		auto founder = std::find_if(m_onlineAgent.begin(), m_onlineAgent.end(),
			[&sID](const ServerAuthSession & sac){ return sac.sID == sID; });
		if (founder != m_onlineAgent.end())
		{
			m_onlineAgent.erase(founder);
		}
	}

	{
		auto founder = std::find_if(m_onlineLogic.begin(), m_onlineLogic.end(),
			[&sID](const ServerAuthSession & sac){ return sac.sID == sID; });
		if (founder != m_onlineLogic.end())
		{
			m_onlineLogic.erase(founder);
		}
	}
}



void CNetManager::msg_ConnectServerAuth(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	ProtoDirectServerAuth auth;
	rs >> auth;
	LOGI("msg_ConnectServerAuth. cID=" << cID << ", Node=" << auth.srcNode << ", index=" << auth.srcIndex);

	if (auth.srcNode == DBAgentNode)
	{
		auto founder = std::find_if(m_onlineDBAgent.begin(), m_onlineDBAgent.end(),
			[auth](const ServerAuthConnect &sac){return sac.index == auth.srcIndex; });
		if (founder != m_onlineDBAgent.end())
		{
			m_onlineDBAgent.erase(founder);
		}
		ServerAuthConnect sac;
		sac.cID = cID;
		sac.node = auth.srcNode;
		sac.index = auth.srcIndex;
		m_onlineDBAgent.push_back(sac);
	}

	if (m_configDBAgent.size() == m_onlineDBAgent.size())
	{
		if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
		{
			LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
			return;
		}
	}
}


void CNetManager::msg_SessionServerAuth(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	ProtoDirectServerAuth auth;
	rs >> auth;
	LOGI("msg_SessionServerAuth. aID=" << aID << ", sID=" << sID << ", Node=" << auth.srcNode << ", index=" << auth.srcIndex);

	ServerAuthSession sas;
	sas.aID = aID;
	sas.sID = sID;
	sas.node = auth.srcNode;
	sas.index = auth.srcIndex;

	if (auth.srcNode == AgentNode)
	{
		auto founder = std::find_if(m_onlineAgent.begin(), m_onlineAgent.end(), 
			[auth](const ServerAuthSession & sas){ return sas.index == auth.srcIndex; });
		if (founder != m_onlineAgent.end())
		{
			m_onlineAgent.erase(founder);
		}
		m_onlineAgent.push_back(sas);
	}
	else if (auth.srcNode == LogicNode)
	{
		auto founder = std::find_if(m_onlineLogic.begin(), m_onlineLogic.end(), 
			[auth](const ServerAuthSession & sas){ return sas.index == auth.srcIndex; });
		if (founder != m_onlineLogic.end())
		{
			m_onlineLogic.erase(founder);
		}
		m_onlineLogic.push_back(sas);
	}
}


bool CNetManager::msg_OrgMessageReq(AccepterID aID, SessionID sID, const char * blockBegin,  FrameStreamTraits::Integer blockSize)
{
	return true;
};

void CNetManager::msg_DefaultReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{

};


