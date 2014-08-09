#include "LoginHandler.h"
#include <ProtoDefine.h>
#include <ProtoAuth.h>
#include <ProtoLogin.h>
#include <MongoManager.h>



bool CLoginHandler::Init()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_GetAccountInfoReq,
		std::bind(&CLoginHandler::msg_GetAccountInfoReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
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
	auto & infoMongo = GlobalFacade::getRef().getMongoManger().getInfoMongo();
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
			auto cursor = infoMongo->query(cl, builder.obj());
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



	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	ws << ID_LS2C_GetAccountInfoAck << info << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
}


