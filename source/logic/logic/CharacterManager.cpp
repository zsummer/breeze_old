
#include "CharacterManager.h"
#include "../core/GlobalFacade.h"
#include "../core/NetManager.h"


bool CCharacterManager::Init()
{
	//注册消息
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_LoadAccountInfoReq,
		std::bind(&CCharacterManager::msg_LoadAccountInfoReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_CreateCharacterReq,
		std::bind(&CCharacterManager::msg_CreateCharacterReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2LS_CharacterLoginReq,
		std::bind(&CCharacterManager::msg_CharacterLoginReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));


	return mongo_LoadLastCharID();
}

bool CCharacterManager::mongo_LoadLastCharID()
{
	//初始化
	long long curArea = GlobalFacade::getRef().getServerConfig().getAreaID();
	long long nextArea = curArea + 1;
	curArea = curArea << 32;
	nextArea = nextArea << 32;
	CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
	std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
	ns += ".cl_character";
	mongo::BSONObjBuilder b;
	b << "_id" << mongo::GTE << curArea << "_id" << mongo::LT << nextArea;
	
	auto cursor = mgr.getInfoMongo()->query(ns, mongo::Query(b.obj()).sort("_id", -1), 1);
	
}
void CCharacterManager::msg_LoadAccountInfoReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	rs >> info;
	LOGD("ID_C2LS_GetAccountInfoReq accountID=" << info.accID);

	auto founder = m_accInfo.find(info.accID);
	if (founder == m_accInfo.end())
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
		if (m_accInfo.find(pinfo->accID) == m_accInfo.end())
		{
			m_accInfo.insert(std::make_pair(pinfo->accID, pinfo));
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
		auto founder = m_accInfo.find(info.accID);
		if (founder == m_accInfo.end())
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



void CCharacterManager::msg_CreateCharacterReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	ProtoCreateCharacterReq req;
	rs >> info >> req;
	LOGD("msg_CreateCharacterReq accountID=" << info.accID);
	


	auto founder = m_accInfo.find(info.accID);
	if (founder == m_accInfo.end() || founder->second->charInfos.size() > 5)
	{
		ProtoCreateCharacterAck ack;
		ack.retCode = EC_SERVER_ERROR;
		ack.lci.charID = info.charID;
		ack.lci.charName = req.charName;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
		info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CreateCharacterAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
		return;
	}

	LittleCharInfo lci;
	lci.charID = m_seqCharID++;
	lci.charName = req.charName;
	lci.iconID = 0;
	lci.level = 1;
	CharacterInfo ci;
	ci.accID = info.accID;
	ci.charID = lci.charID;
	ci.charName = lci.charName;
	ci.iconID = lci.iconID;
	ci.level = lci.level;

	CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
	std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
	ns += ".cl_character";
	GlobalFacade::getRef().getMongoManger().async_update(mgr.getInfoMongo(), 
		ns, QUERY("_id" << (long long)ci.charID), 
		BSON("level" << ci.level << "accID" << (long long)ci.accID << "charName" << ci.charName << "iconID" << ci.iconID), 
		true,
		std::bind(&CCharacterManager::mongo_CreateCharacter, this, std::placeholders::_1, aID, sID, info, lci));

	return;
}



void CCharacterManager::mongo_CreateCharacter(std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info, const LittleCharInfo & lci)
{
	if (errMsg.empty())
	{
		auto founder = m_accInfo.find(info.accID);
		assert(founder != m_accInfo.end());
		founder->second->charInfos.push_back(lci);
		ProtoCreateCharacterAck ack;
		ack.retCode = EC_SUCCESS;
		ack.lci = lci;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
		info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CreateCharacterAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
		return;
	}
	else
	{
		ProtoCreateCharacterAck ack;
		ack.retCode = EC_SERVER_ERROR;
		ack.lci = lci;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
		info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CreateCharacterAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
		return;
	}
}


void CCharacterManager::msg_CharacterLoginReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	ProtoCharacterLoginReq req;
	rs >> info >> req;
	LOGD("msg_CreateCharacterReq accountID=" << info.accID);

	//check invalide
	bool checkValide = false;
	do
	{
		auto founder = m_accInfo.find(info.accID);
		if (founder == m_accInfo.end())
		{
			break;
		}
		auto & lcis = founder->second->charInfos;
		auto foundChar = std::find_if(lcis.begin(), lcis.end(), [&req](const LittleCharInfo & lci){return lci.charID == req.charID; });
		if (foundChar == lcis.end())
		{
			break;
		}
		checkValide = true;
		
	} while (0);

	if (!checkValide)
	{
		ProtoCharacterLoginAck ack;
		ack.retCode = EC_SERVER_ERROR;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
		return;
	}
	
	auto founder = m_charInfo.find(req.charID);
	if (founder == m_charInfo.end())
	{
		CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
		std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
		ns += ".cl_character";
		info.charID = req.charID;
		mgr.async_query(mgr.getInfoMongo(), ns, QUERY("_id" << (long long)req.charID),
			std::bind(&CCharacterManager::mongo_LoadCharacterInfo, this, std::placeholders::_1, std::placeholders::_2, aID, sID, info));
		return;
	}

	info.charID = founder->second->charInfo.charID;
	founder->second->sInfo = info;

	{
		ProtoCharacterLoginAck ack;
		ack.retCode = EC_SUCCESS;
		ack.info = founder->second->charInfo;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}

	//登陆成功 踢人广播
	{
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 2;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2AS_KickCharacter << info;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}

	
	
}


void CCharacterManager::mongo_LoadCharacterInfo(std::shared_ptr<mongo::DBClientCursor> & cursor, std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info)
{
	try
	{
		if (!errMsg.empty())
		{
			LOGE("mongo_LoadCharacterInfo error. errMsg=" << errMsg);
			throw 1;
		}
		

		if (cursor->more())
		{
			auto obj = cursor->next();
			std::shared_ptr<LogicCharacterInfo> plci(new LogicCharacterInfo);
			plci->charInfo.charID = obj.getField("_id").numberLong();
			plci->charInfo.charName = obj.getField("charName").str();
			plci->charInfo.iconID = obj.getField("iconID").numberInt();
			plci->charInfo.level = obj.getField("level").numberInt();
			plci->sInfo = info;
			m_charInfo.insert(std::make_pair(info.charID, plci));

			{
				ProtoCharacterLoginAck ack;
				ack.retCode = EC_SUCCESS;
				ack.info = plci->charInfo;
				WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
				ProtoRouteToOtherServer route;
				route.dstIndex = info.agentIndex;
				route.dstNode = AgentNode;
				route.routerType = 0;
				ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
				GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
			}

			//登陆成功 踢人广播
			{
				WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
				ProtoRouteToOtherServer route;
				route.dstIndex = info.agentIndex;
				route.dstNode = AgentNode;
				route.routerType = 2;
				ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2AS_KickCharacter << info;
				GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
			}
			return;
		}
		else
		{
			LOGE("mongo_LoadCharacterInfo error. not found charID. charID=" << info.charID);
			throw 1;
		}

	}
	catch (const mongo::DBException &e)
	{
		LOGW("mongo_LoadLittleCharInfo caught:" << e.what());
	}
	catch (...)
	{
		LOGW("mongo_LoadLittleCharInfo UNKNOWN ERROR");
	}
	{
		ProtoCharacterLoginAck ack;
		ack.retCode = EC_SERVER_ERROR;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = 0;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}

}












