#include "NetManager.h"

CNetManager::CNetManager()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_PROTO_SERVER_INIT,
		std::bind(&CNetManager::msg_serverInit, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionDefaultMessage(
		std::bind(&CNetManager::msg_DefaultReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionOrgMessage(
		std::bind(&CNetManager::msg_OrgMessageReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));


	//注册事件
	CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CNetManager::event_OnConnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CNetManager::event_OnDisconnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_1));
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
		if (con.dstServer == DBAgentNode)
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
	if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return false;
	}
	LOGI("CNetManager Init Success.");
	return true;
}


void CNetManager::event_OnConnect(ConnectorID cID)
{
	auto founder = m_configDBAgent.find(cID);
	if (founder != m_configDBAgent.end())
	{
		m_onlineDBAgent.push_back(cID);
		LOGI("event_OnConnect Auth. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);

	}
	if (m_configDBAgent.size() != m_onlineDBAgent.size())
	{
		return;
	}

	//init
	WriteStreamPack ws;
	ws << ID_PROTO_SERVER_INIT << CenterNode << GlobalFacade::getRef().getServerConfig().getConfigListen(CenterNode).index;
	CTcpSessionManager::getRef().SendOrgConnectorData(cID, ws.GetStream(), ws.GetStreamLen());

	//所有connector已经建立连接 此时打开客户端监听端口
	if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return;
	}
	LOGI("AddAcceptor Success. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
}

void CNetManager::event_OnDisconnect(ConnectorID cID)
{
	auto founder = m_configDBAgent.find(cID);
	if (founder != m_configDBAgent.end())
	{
		auto searchiter = std::find_if(m_onlineDBAgent.begin(), m_onlineDBAgent.end(), [&cID](ConnectorID ccID){ return ccID == cID; });
		if (searchiter != m_onlineDBAgent.end())
		{
			m_onlineDBAgent.erase(searchiter);
		}
		LOGW("event_OnDisconnect Center. cID=" << cID << ", listenIP=" << founder->second.remoteIP << ", listenPort=" << founder->second.remotePort);
	}
}


void CNetManager::event_OnSessionEstablished(AccepterID, SessionID)
{

}

void CNetManager::event_OnSessionDisconnect(AccepterID aID, SessionID sID)
{
	auto founder = m_onlineAgent.find(sID);
	if (founder != m_onlineAgent.end())
	{
		m_onlineAgent.erase(founder);
		LOGW("event_OnSessionDisconnect Agent. index=" << founder->second.second);
		return;
	}
	founder = m_onlineLogic.find(sID);
	if (founder != m_onlineLogic.end())
	{
		m_onlineLogic.erase(founder);
		LOGW("event_OnSessionDisconnect Agent. index=" << founder->second.second);
		return;
	}
}







void CNetManager::msg_serverInit(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	ServerNodeType node;
	unsigned int index = 0;
	rs >> node >> index;
	if (node == AgentNode)
	{
		auto founder = std::find_if(m_onlineAgent.begin(), m_onlineAgent.end(), [index](OnlineServerType::value_type t){ return index == t.second.second; });
		if (founder != m_onlineAgent.end())
		{
			m_onlineAgent.erase(founder);
		}
		m_onlineAgent.insert(std::make_pair(sID, std::make_pair(aID, index)));
		LOGI("agent online. sID=" << sID << ", index=" << index);
		return;
	}
	else if (node == LogicNode)
	{
		auto founder = std::find_if(m_onlineLogic.begin(), m_onlineLogic.end(), [index](OnlineServerType::value_type t){ return index == t.second.second; });
		if (founder != m_onlineLogic.end())
		{
			m_onlineLogic.erase(founder);
		}
		m_onlineLogic.insert(std::make_pair(sID, std::make_pair(aID, index)));
		LOGI("logic online. sID=" << sID << ", index=" << index);
		return;
	}
	LOGW("unknown online. sID=" << sID << ", index=" << index);
}


bool CNetManager::msg_OrgMessageReq(AccepterID aID, SessionID sID, const char * blockBegin,  FrameStreamTraits::Integer blockSize)
{
	return true;
};

void CNetManager::msg_DefaultReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{

};


