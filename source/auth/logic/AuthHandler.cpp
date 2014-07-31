#include "AuthHandler.h"
#include <mongo/client/dbclient.h>



bool CAuthHandler::Init()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
		std::bind(&CAuthHandler::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

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
	return true;
}


void CAuthHandler::msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
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
			std::string db = GlobalFacade::getRef().getServerConfig().getAuthMongoDB().db;
			db += ".cl_auth";
			auto cursor = m_authMongo->query(db, builder.obj());
			if (cursor->more())
			{
				auto obj = cursor->next();
				std::string pwd = obj.getField("pwd").str();
				AccountID accID = obj.getField("accID").numberLong();
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


