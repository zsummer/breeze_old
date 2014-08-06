#include "LoginHandler.h"
#include "../core/GlobalFacade.h"
#include "../core/NetManager.h"


bool CLoginHandler::Init()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_GetAccountInfoReq,
		std::bind(&CLoginHandler::msg_GetAccountReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterConnectorMessage(ID_LS2C_GetAccountInfoAck,
		std::bind(&CLoginHandler::msg_GetAccountAck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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


	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	ws << ID_C2LS_GetAccountInfoReq << info << req;
	GlobalFacade::getRef().getNetManger().SendOrgDataToDBAgent(ws.GetStream(), ws.GetStreamLen());
}


void CLoginHandler::msg_GetAccountAck(ConnectorID cID, ProtocolID pID, ReadStreamPack &rs)
{
	SessionInfo info;
	ProtoGetAccountInfoAck ack;
	rs >> info >> ack;
	LOGD("ID_LS2C_GetAccountInfoAck accID=" << ack.info.accID << ", retCode=" << ack.retCode);

	
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