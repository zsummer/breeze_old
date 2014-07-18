#include "NetManager.h"
#include <mongo/client/dbclient.h>
CNetManager::CNetManager()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
		std::bind(&CNetManager::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_PROTO_SERVER_INIT,
		std::bind(&CNetManager::msg_ServerInit, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));


	//×¢²áÊÂ¼þ
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_1));
}

bool CNetManager::Start()
{
	m_configListen.aID = 1;
	m_configListen.listenIP = GlobalFacade::getRef().getServerConfig().getAuthListen().ip;
	m_configListen.listenPort = GlobalFacade::getRef().getServerConfig().getAuthListen().port;
	m_configListen.maxSessions = 100;

	if (CTcpSessionManager::getRef().AddAcceptor(m_configListen) == InvalidAccepterID)
	{
		LOGE("AddAcceptor Failed. listenIP=" << m_configListen.listenIP << ", listenPort=" << m_configListen.listenPort);
		return false;
	}
	LOGI("CNetManager Init Success.");
	return true;
}

void CNetManager::event_OnSessionEstablished(AccepterID, SessionID)
{

}
void CNetManager::event_OnSessionDisconnect(AccepterID aID, SessionID sID)
{
	LOGW("event_OnSessionDisconnect sID=" << sID);
	auto founder = m_onlineAgentSession.find(sID);
	if (founder == m_onlineAgentSession.end())
	{
		return;
	}
	m_onlineAgentIndex.erase(founder->second.second);
	m_onlineAgentSession.erase(founder);
}






void CNetManager::msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	ProtoAuthReq req;
	rs >> info >> req;
	LOGD("ID_C2AS_AuthReq user=" << req.info.user << ", pwd=" << req.info.pwd);

	//debug

	try {
		mongo::DBClientConnection c;
		c.connect("localhost");
		std::cout << "connected ok" << std::endl;
	}
	catch (const mongo::DBException &e) {
		std::cout << "caught " << e.what() << std::endl;
	}
	//end debug

	ProtoAuthAck ack;
	ack.retCode = EC_SUCCESS;
	ack.accountID = 100;
	WriteStreamPack ws;
	ws << ID_AS2C_AuthAck << info << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
}

void CNetManager::msg_ServerInit(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	std::string node;
	ui32 index = 0;
	rs >> node >> index;

	m_onlineAgentSession[sID].first = aID;
	m_onlineAgentSession[sID].second = index;
	m_onlineAgentIndex[index].first = aID;
	m_onlineAgentIndex[index].second = sID;

	LOGI("msg_ServerInit sID=" << sID << ", index=" << index);
}

