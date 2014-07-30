#include "NetManager.h"

CNetManager::CNetManager()
{


	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_DT2OS_DirectServerAuth,
		std::bind(&CNetManager::msg_ConnectServerAuth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//×¢²áÊÂ¼þ
	CMessageDispatcher::getRef().RegisterOnConnectorEstablished(std::bind(&CNetManager::event_OnConnect, this, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnConnectorDisconnect(std::bind(&CNetManager::event_OnDisconnect, this, std::placeholders::_1));
}

bool CNetManager::Start()
{

	auto connecters = GlobalFacade::getRef().getServerConfig().getConfigConnect(LogicNode);
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
		auto searchiter = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(),
			[&cID](const ServerAuthConnect & sac){ return sac.cID == cID; });
		if (searchiter != m_onlineCenter.end())
		{
			m_onlineCenter.erase(searchiter);
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
		auto founder = std::find_if(m_onlineCenter.begin(), m_onlineCenter.end(),
			[auth](const ServerAuthConnect &sac){return sac.index == auth.srcIndex; });
		if (founder != m_onlineCenter.end())
		{
			m_onlineCenter.erase(founder);
		}
		ServerAuthConnect sac;
		sac.cID = cID;
		sac.node = auth.srcNode;
		sac.index = auth.srcIndex;
		m_onlineCenter.push_back(sac);
	}
}



