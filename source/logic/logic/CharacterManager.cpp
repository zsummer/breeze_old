
#include "CharacterManager.h"
#include "../core/GlobalFacade.h"
#include "../core/NetManager.h"


bool CCharacterManager::Init()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_LoadAccountInfoReq,
		std::bind(&CCharacterManager::msg_LoadAccountInfoReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	return true;
}


void CCharacterManager::msg_LoadAccountInfoReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	LOGD("ID_C2LS_GetAccountInfoReq accountID=" << info.accID);

	auto founder = m_accountCache.find(info.accID);
	if (founder == m_accountCache.end())
	{
		CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
		std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
		ns += ".cl_account";
		mgr.async_query(mgr.getInfoMongo(), ns, QUERY("_id" << (long long)info.accID),
			std::bind(&CCharacterManager::mongo_LoadAccountInfo, this, std::placeholders::_1, std::placeholders::_2, aID, sID, info));
	}
	else
	{
		ProtoLoadAccountInfoAck ack;
		ack.retCode = EC_SUCCESS;
		ack.info = *founder->second;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
		info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_LoadAccountInfoAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}
}


void CCharacterManager::mongo_LoadAccountInfo(std::shared_ptr<mongo::DBClientCursor> & cursor, std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info)
{
	try
	{
		if (!errMsg.empty())
		{
			LOGE("mongo_LoadAccountInfo error. errMsg=" << errMsg);
			throw 1;
		}
		

		std::shared_ptr<AccountInfo> pinfo(new AccountInfo);
		pinfo->accID = info.accID;
		if (cursor->more())
		{
			auto obj = cursor->next();
			pinfo->accName = obj.getField("accName").str();
			pinfo->diamond = obj.getField("diamond").numberInt();
			pinfo->hisDiamond = obj.getField("hisDiamond").numberInt();
			pinfo->giftDmd = obj.getField("giftDmd").numberInt();
			pinfo->hisGiftDmd = obj.getField("hisGiftDmd").numberInt();
		}
		else
		{
			pinfo->diamond = 0;
			pinfo->hisDiamond = 0;
			pinfo->giftDmd = 0;
			pinfo->hisGiftDmd = 0;
		}
		if (m_accountCache.find(pinfo->accID) == m_accountCache.end())
		{
			m_accountCache.insert(std::make_pair(pinfo->accID, pinfo));
			CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
			std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
			ns += ".cl_character";
			mgr.async_query(mgr.getInfoMongo(), ns, QUERY("accID" << (long long)pinfo->accID),
				std::bind(&CCharacterManager::mongo_LoadAccountInfo, this, std::placeholders::_1, std::placeholders::_2, aID, sID, info));
			return;
		}
		
		
	}
	catch (const mongo::DBException &e)
	{
		LOGW("mongo_LoadAccountInfo caught:" << e.what());
	}
	catch (...)
	{
		LOGW("mongo_LoadAccountInfo UNKNOWN ERROR");
	}

	ProtoLoadAccountInfoAck ack;
	ack.retCode = EC_SERVER_ERROR;
	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
	ProtoRouteToOtherServer route;
	route.dstIndex = info.agentIndex;
	route.dstNode = AgentNode;
	route.routerType = 0;
	info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_LoadAccountInfoAck << info << ack;
	GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
}

void CCharacterManager::mongo_LoadLittleCharInfo(std::shared_ptr<mongo::DBClientCursor> & cursor, std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info)
{
	try
	{
		if (!errMsg.empty())
		{
			LOGE("mongo_LoadLittleCharInfo error. errMsg=" << errMsg);
			throw 1;
		}
		auto founder = m_accountCache.find(info.accID);
		if (founder == m_accountCache.end())
		{
			LOGE("mongo_LoadLittleCharInfo error. not found account ID");
			throw 2;
		}
		
		while (cursor->more())
		{
			auto obj = cursor->next();
			LittleCharInfo charInfo;
			charInfo.charID = obj.getField("_id").numberLong();
			charInfo.charName = obj.getField("charName").str();
			charInfo.iconID = obj.getField("iconID").numberInt();
			charInfo.level = obj.getField("level").numberInt();
			
			founder->second->charInfos.push_back(charInfo);
		}
		ProtoLoadAccountInfoAck ack;
		ack.retCode = EC_SUCCESS;
		ack.info = *founder->second;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
		info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_LoadAccountInfoAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
		return;
	}
	catch (const mongo::DBException &e)
	{
		LOGW("mongo_LoadLittleCharInfo caught:" << e.what());
	}
	catch (...)
	{
		LOGW("mongo_LoadLittleCharInfo UNKNOWN ERROR");
	}
	ProtoLoadAccountInfoAck ack;
	ack.retCode = EC_SERVER_ERROR;
	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
	ProtoRouteToOtherServer route;
	route.dstIndex = info.agentIndex;
	route.dstNode = AgentNode;
	route.routerType = 0;
	info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_LoadAccountInfoAck << info << ack;
	GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
}
