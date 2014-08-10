
#include "LoginHandler.h"
#include "../core/GlobalFacade.h"
#include "../core/NetManager.h"


bool CLoginHandler::Init()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_GetAccountInfoReq,
		std::bind(&CLoginHandler::msg_GetAccountReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	return true;
}


void CLoginHandler::msg_GetAccountReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	ProtoGetAccountInfoReq req;
	rs >> info >> req;
	LOGD("ID_C2LS_GetAccountInfoReq accountID=" << req.accountID);

	CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
	std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
	ns += ".cl_account";
	mgr.async_query(mgr.getInfoMongo(), ns, QUERY("_id" << (long long)req.accountID),
		std::bind(&CLoginHandler::mongo_GetAccount, this, std::placeholders::_1, std::placeholders::_2, aID, sID, info, req));

}


void CLoginHandler::mongo_GetAccount(std::shared_ptr<mongo::DBClientCursor> & cursor, std::string &errMsg, AccepterID cID, SessionID sID, SessionInfo & info, const ProtoGetAccountInfoReq &req)
{
	ProtoGetAccountInfoAck ack;
	ack.retCode = EC_SERVER_ERROR;
	ack.info.accID = req.accountID;
	try
	{
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
	}
	catch (...)
	{
		LOGW("auth mongo UNKNOWN ERROR");
	}


	
	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
	ProtoRouteToOtherServer route;
	route.dstIndex = info.agentIndex;
	route.dstNode = AgentNode;
	route.routerType = 0;
	info.srcNode = LogicNode;
	info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_GetAccountInfoAck << info << ack;
	GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
}