#include "LoginHandler.h"
#include <mongo/client/dbclient.h>
#include <ProtoLogin.h>



bool CLoginHandler::Init()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_GetAccountInfoReq,
		std::bind(&CLoginHandler::msg_GetAccountInfoReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	if (m_infoMongo)
	{
		return false;
	}
	m_infoMongo = std::make_shared<mongo::DBClientConnection>(new mongo::DBClientConnection);
	try
	{
		std::string errorMsg;
		std::string dbhost = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().ip;
		dbhost += ":" + boost::lexical_cast<std::string>(GlobalFacade::getRef().getServerConfig().getInfoMongoDB().port);
		std::string db = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
		std::string user = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().user;
		std::string pwd = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().pwd;
		m_infoMongo->connect(dbhost);
		if (!m_infoMongo->auth(db, user, pwd, errorMsg))
		{
			LOGI("auth failed. db=" << db << ", user=" << user << ", pwd=" << pwd << ", errMSG=" << errorMsg);
		//	return false;
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


void CLoginHandler::msg_GetAccountInfoReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	ProtoGetAccountInfoReq req;
	rs >> info >> req;
	LOGD("ID_C2LS_GetAccountInfoReq accID=" << req.accountID);


	ProtoGetAccountInfoAck ack;
	ack.retCode = EC_KEY_NOTFOUND;
	ack.info.accID = req.accountID;

	do
	{
		try 
		{
// 			//debug 截断认证流程检测协议QPS速度
// 			ack.retCode = EC_SUCCESS;
// 			break;
// 			//end debug
			mongo::BSONObjBuilder builder;
			builder.append("_id", (long long)req.accountID);
			std::string cl = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
			cl += ".cl_account";
			auto cursor = m_infoMongo->query(cl, builder.obj());
			if (cursor->more())
			{
				auto obj = cursor->next();
				ack.info.diamond = obj.getField("diamond").numberInt();
				ack.info.hisDiamond = obj.getField("hisDiamond").numberInt();
				ack.info.giftDmd = obj.getField("giftDmd").numberInt();
				ack.info.hisGiftDmd = obj.getField("hisGiftDmd").numberInt();
				ack.retCode = EC_SUCCESS;
			}
			else
			{
				ack.info.diamond = 0;
				ack.info.hisDiamond = 0;
				ack.info.giftDmd = 0;
				ack.info.hisGiftDmd = 0;
				ack.retCode = EC_SUCCESS;
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
	ws << ID_LS2C_GetAccountInfoAck << info << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
}


