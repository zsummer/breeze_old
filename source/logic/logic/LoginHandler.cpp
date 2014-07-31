#include "LoginHandler.h"



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

	ProtoGetAccountInfoAck ack;
	ack.retCode = EC_SUCCESS;
	ack.info.accID = req.accountID;
	ack.info.diamond = 0;
	ack.info.hisDiamond = 0;
	ack.info.giftDmd = 0;
	ack.info.hisGiftDmd = 0;


	WriteStreamPack ws;
	ProtoRouteToOtherServer route;
	route.dstIndex = info.agentIndex;
	route.dstNode = AgentNode;
	route.routerType = 0;
	ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_GetAccountInfoAck << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
}


