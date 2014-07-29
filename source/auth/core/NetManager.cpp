#include "NetManager.h"
#include <mongo/client/dbclient.h>
CNetManager::CNetManager()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
		std::bind(&CNetManager::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_DT2OS_DirectServerInit,
		std::bind(&CNetManager::msg_ServerInit, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));


	//注册事件
	CMessageDispatcher::getRef().RegisterOnSessionEstablished(std::bind(&CNetManager::event_OnSessionEstablished, this, std::placeholders::_1, std::placeholders::_1));
	CMessageDispatcher::getRef().RegisterOnSessionDisconnect(std::bind(&CNetManager::event_OnSessionDisconnect, this, std::placeholders::_1, std::placeholders::_1));
}

bool CNetManager::Start()
{

	if (m_authMongo)
	{
		return false;
	}
	m_authMongo = std::make_shared<mongo::DBClientConnection>(new mongo::DBClientConnection);
	try
	{
		std::string errorMsg;
		std::string dbhost = GlobalFacade::getRef().getServerConfig().getAuthMongoDB().ip;
		dbhost += ":" + boost::lexical_cast<std::string>(GlobalFacade::getRef().getServerConfig().getAuthMongoDB().port);
		std::string db = GlobalFacade::getRef().getServerConfig().getAuthMongoDB().db;
		std::string user = GlobalFacade::getRef().getServerConfig().getAuthMongoDB().user;
		std::string pwd = GlobalFacade::getRef().getServerConfig().getAuthMongoDB().pwd;
		m_authMongo->connect(dbhost);
		if (!m_authMongo->auth(db, user, pwd, errorMsg))
		{
			LOGI("auth failed. db=" << db << ", user=" << user << ", pwd=" << pwd << ", errMSG=" << errorMsg);
			return false;
		}
	}
	catch (const mongo::DBException &e)
	{
		LOGE("connect caught:" << e.what());
		return false;
	}
	catch (...)
	{
		LOGE("connect mongo auth UNKNOWN ERROR");
		return false;
	}


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


	ProtoAuthAck ack;
	ack.retCode = EC_AUTH_ERROR;
	ack.accountID = InvalidAccountID;

	do
	{
		try 
		{
// 			//debug 截断认证流程检测协议QPS速度
// 			ack.retCode = EC_SUCCESS;
// 			break;
// 			//end debug
			mongo::BSONObjBuilder builder;
			builder.append("_id", req.info.user);
			auto cursor = m_authMongo->query("auth.users", builder.obj());
			if (cursor->more())
			{
				auto obj = cursor->next();
				std::string pwd = obj.getField("pwd").str();
				AccountID accID = obj.getField("accountID").numberLong();
				if (pwd == req.info.pwd)
				{
					ack.accountID = accID;
					ack.retCode = EC_SUCCESS;
					LOGD("auth success req user=" << req.info.user << ", req pwd=" << req.info.pwd << ", result pwd=" << pwd << ", result accID=" << accID);
				}
				else
				{
					LOGD("auth failed req user=" << req.info.user << ", req pwd=" << req.info.pwd << ", result pwd=" << pwd << ", result accID=" << accID);
				}
			}
			else
			{
				LOGI("auth failed. req user=" << req.info.user << ", req pwd=" << req.info.pwd << ", result empty.");
			}
		}
		catch (const mongo::DBException &e)
		{
			LOGW("auth mongo caught:" << e.what());
			break;
		}
		catch (...)
		{
			LOGW("auth mongo UNKNOWN ERROR");
			break;
		}
	} while (0);

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

