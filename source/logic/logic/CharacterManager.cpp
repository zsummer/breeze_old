
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
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_AS2LS_CharacterLogout,
		std::bind(&CCharacterManager::msg_CharacterLogout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));


	return mongo_LoadLastCharID();
}

bool CCharacterManager::mongo_LoadLastCharID()
{
	//初始化
	m_genObjCharID.InitConfig(GlobalFacade::getRef().getServerConfig().getPlatID(), GlobalFacade::getRef().getServerConfig().getAreaID());


	CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
	std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
	ns += ".info_character";
	mongo::BSONObjBuilder b;
	b << "_id" << mongo::GTE << (long long)m_genObjCharID.GetMinObjID() << "_id" << mongo::LT << (long long)m_genObjCharID.GetMaxObjID();
	try
	{
		auto cursor = mgr.getInfoMongo()->query(ns, mongo::Query(b.obj()).sort("_id", -1), 1);
		if (cursor->more())
		{
			auto obj = cursor->next();
			m_genObjCharID.SetCurObjID(obj.getField("_id").numberLong());
			LOGI("mongo_LoadLastCharID minCharID=" << m_genObjCharID.GetMinObjID() << ", maxCharID=" << m_genObjCharID.GetMaxObjID() << ", curCharID=" << m_genObjCharID.GetCurObjID());
			return true;
		}
		else
		{
			LOGI("mongo_LoadLastCharID not found any character. minCharID=" << m_genObjCharID.GetMinObjID() << ", maxCharID=" << m_genObjCharID.GetMaxObjID() << ", curCharID=" << m_genObjCharID.GetCurObjID());
			return true;
		}
	}
	catch (const mongo::DBException &e)
	{
		LOGW("mongo_LoadLastCharID caught:" << e.what());
	}
	catch (...)
	{
		LOGW("mongo_LoadLastCharID UNKNOWN ERROR");
	}
	
	return false;
}

void CCharacterManager::msg_LoadAccountInfoReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	rs >> info;
	LOGD("msg_LoadAccountInfoReq accountID=" << info.accID);

	auto founder = m_accInfo.find(info.accID);
	if (founder == m_accInfo.end())
	{
		CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
		std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
		ns += ".info_account";
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
		route.routerType = RT_SPECIFIED;
		info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
		info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_LoadAccountInfoAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}
}


void CCharacterManager::mongo_LoadAccountInfo(const std::shared_ptr<CMongoManager::MongoRetDatas> & retDatas, const std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info)
{
	try
	{
		if (!errMsg.empty())
		{
			LOGE("mongo_LoadAccountInfo error. errMsg=" << errMsg);
			throw 1;
		}
		
		std::shared_ptr<AccountInfo> pinfo(new AccountInfo);
		
		if (!retDatas->empty())
		{
			auto obj = retDatas->at(0);
			pinfo->accID = info.accID;
			pinfo->accName = obj.getField("accName").str();
			pinfo->diamond = obj.getField("diamond").numberInt();
			pinfo->hisDiamond = obj.getField("hisDiamond").numberInt();
			pinfo->giftDmd = obj.getField("giftDmd").numberInt();
			pinfo->hisGiftDmd = obj.getField("hisGiftDmd").numberInt();
		}
		else
		{
			pinfo->accID = info.accID;
			pinfo->diamond = 0;
			pinfo->hisDiamond = 0;
			pinfo->giftDmd = 0;
			pinfo->hisGiftDmd = 0;
		}
		if (m_accInfo.find(pinfo->accID) == m_accInfo.end())
		{
			m_accInfo.insert(std::make_pair(pinfo->accID, pinfo));
			CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
			if (retDatas->empty())
			{
				std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
				ns += ".info_account";
				mgr.async_update(mgr.getInfoMongo(), ns, QUERY("_id" << (long long)pinfo->accID), 
					BSON("diamond" << pinfo->diamond << "hisDiamond" << pinfo->hisDiamond << "giftDmd" << pinfo->giftDmd << "hisGiftDmd" << pinfo->hisGiftDmd ), true,
					std::bind(&CCharacterManager::mongo_UpdateNormalHandler, this, std::placeholders::_1, aID, sID, info, "insert account."));

				ProtoLoadAccountInfoAck ack;
				ack.retCode = EC_SUCCESS;
				ack.info = *pinfo;
				WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
				ProtoRouteToOtherServer route;
				route.dstIndex = info.agentIndex;
				route.dstNode = AgentNode;
				route.routerType = RT_SPECIFIED;
				info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
				info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
				ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_LoadAccountInfoAck << info << ack;
				GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
				return;
			}
			std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
			ns += ".info_character";
			mgr.async_query(mgr.getInfoMongo(), ns, QUERY("accID" << (long long)pinfo->accID),
				std::bind(&CCharacterManager::mongo_LoadLittleCharInfo, this, std::placeholders::_1, std::placeholders::_2, aID, sID, info));
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
	ack.retCode = EC_DB_ERROR;
	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
	ProtoRouteToOtherServer route;
	route.dstIndex = info.agentIndex;
	route.dstNode = AgentNode;
	route.routerType = RT_SPECIFIED;
	info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
	info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_LoadAccountInfoAck << info << ack;
	GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
}

void CCharacterManager::mongo_LoadLittleCharInfo(const std::shared_ptr<CMongoManager::MongoRetDatas> & retDatas, const std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info)
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
		for (auto & obj : *retDatas)
		{
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
		route.routerType = RT_SPECIFIED;
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
	route.routerType = RT_SPECIFIED;
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
		route.routerType = RT_SPECIFIED;
		info.srcNode = GlobalFacade::getRef().getServerConfig().getOwnServerNode();
		info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CreateCharacterAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
		return;
	}

	LittleCharInfo lci;
	lci.charID = m_genObjCharID.GenNewObjID();
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
	ns += ".info_character";
	GlobalFacade::getRef().getMongoManger().async_update(mgr.getInfoMongo(), 
		ns, QUERY("_id" << (long long)ci.charID), 
		BSON("level" << ci.level << "accID" << (long long)ci.accID << "charName" << ci.charName << "iconID" << ci.iconID), 
		true,
		std::bind(&CCharacterManager::mongo_CreateCharacter, this, std::placeholders::_1, aID, sID, info, lci));

	return;
}



void CCharacterManager::mongo_CreateCharacter(const std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info, const LittleCharInfo & lci)
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
		route.routerType = RT_SPECIFIED;
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
		route.routerType = RT_SPECIFIED;
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
		route.routerType = RT_SPECIFIED;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
		return;
	}
	
	auto founder = m_charInfo.find(req.charID);
	if (founder == m_charInfo.end())
	{
		CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
		std::string ns = GlobalFacade::getRef().getServerConfig().getInfoMongoDB().db;
		ns += ".info_character";
		info.charID = req.charID;
		info.loginTime = time(NULL);
		mgr.async_query(mgr.getInfoMongo(), ns, QUERY("_id" << (long long)req.charID),
			std::bind(&CCharacterManager::mongo_LoadCharacterInfo, this, std::placeholders::_1, std::placeholders::_2, aID, sID, info));
		return;
	}

	//登陆成功 回调
	{
		info.charID = founder->second->charInfo.charID;
		info.loginTime = time(NULL);
		founder->second->sInfo = info;
		on_CharLogin(founder->second->sInfo);
	}

	//登陆成功 踢人广播
	{
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = RT_BROADCAST;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2AS_CharacterLogin << info;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}

	//登陆成功 返回通知
	{
		ProtoCharacterLoginAck ack;
		ack.retCode = EC_SUCCESS;
		ack.info = founder->second->charInfo;
		WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
		ProtoRouteToOtherServer route;
		route.dstIndex = info.agentIndex;
		route.dstNode = AgentNode;
		route.routerType = RT_SPECIFIED;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}




	
	
}


void CCharacterManager::mongo_LoadCharacterInfo(const std::shared_ptr<CMongoManager::MongoRetDatas> & retDatas, const std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info)
{
	try
	{
		if (!errMsg.empty())
		{
			LOGE("mongo_LoadCharacterInfo error. errMsg=" << errMsg);
			throw 1;
		}
		

		if (!retDatas->empty())
		{
			auto obj = retDatas->at(0);
			std::shared_ptr<LogicCharacterInfo> plci(new LogicCharacterInfo);
			plci->charInfo.charID = obj.getField("_id").numberLong();
			plci->charInfo.charName = obj.getField("charName").str();
			plci->charInfo.iconID = obj.getField("iconID").numberInt();
			plci->charInfo.level = obj.getField("level").numberInt();
			plci->sInfo = info;
			m_charInfo.insert(std::make_pair(info.charID, plci));

			//登陆成功 回调
			{
				on_CharLogin(plci->sInfo);
			}

			//角色登陆广播
			{
				WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
				ProtoRouteToOtherServer route;
				route.dstIndex = info.agentIndex;
				route.dstNode = AgentNode;
				route.routerType = RT_BROADCAST;
				ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2AS_CharacterLogin << info;
				GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
			}

			{
				ProtoCharacterLoginAck ack;
				ack.retCode = EC_SUCCESS;
				ack.info = plci->charInfo;
				WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);;
				ProtoRouteToOtherServer route;
				route.dstIndex = info.agentIndex;
				route.dstNode = AgentNode;
				route.routerType = RT_SPECIFIED;
				ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
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
		route.routerType = RT_SPECIFIED;
		ws << ID_RT2OS_RouteToOtherServer << route << ID_LS2C_CharacterLoginAck << info << ack;
		GlobalFacade::getRef().getNetManger().SendOrgDataToCenter(ws.GetStream(), ws.GetStreamLen());
	}

}





void CCharacterManager::mongo_UpdateNormalHandler(const std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info, const std::string & msg)
{
	if (errMsg.empty())
	{
		LOGD("CCharacterManager::mongo_UpdateNormalHandler Success. aID=" << aID << ", sID=" << sID << info << ", msg=" << msg);
	}
	else
	{
		LOGE("CCharacterManager::mongo_UpdateNormalHandler Failed. aID=" << aID << ", sID=" << sID << info << ", msg=" << msg << ",mongoErrorMsg=" << errMsg);
	}
}


void CCharacterManager::msg_CharacterLogout(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	rs >> info;
	LOGI("CCharacterManager::msg_CharacterLogout. info=" << info);
	
	auto founder = m_charInfo.find(info.charID);
	if (founder != m_charInfo.end())
	{
		auto & sInfo = founder->second->sInfo;
		if (sInfo.sID == info.sID
			&& sInfo.agentIndex == info.agentIndex)
		{
			sInfo.agentIndex = InvalidNodeIndex;
			sInfo.aID = InvalidAccepterID;
			sInfo.sID = InvalidSeesionID;
		}
	}

	on_CharLogout(info);
}
void CCharacterManager::on_CharLogin(const SessionInfo & info)
{
	LOGI("CCharacterManager::on_CharLogin. info=" << info);

	//写登录日志
	CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
	std::string ns = GlobalFacade::getRef().getServerConfig().getLogMongoDB().db;
	ns += ".log_login";
	mgr.async_insert(mgr.getLogMongo(), ns, 
		BSON(mongo::GENOID << "accID" << (long long)info.accID << "charID" << (long long)info.charID << "login" << true << "logTime" << (long long)info.loginTime
		<< "agentIndex" << info.agentIndex << "sID" << info.sID),
		std::bind(&CCharacterManager::mongo_UpdateNormalHandler, this, std::placeholders::_1, info.aID, info.sID, info, "insert log login."));
}
void CCharacterManager::on_CharLogout(const SessionInfo & info)
{
	LOGI("CCharacterManager::on_CharLogout. info=" << info);
	//写登出日志
	unsigned long long onlineTime = time(NULL);
	if (onlineTime > info.loginTime)
	{
		onlineTime -= info.loginTime;
	}
	else
	{
		onlineTime = 0;
	}
	CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
	std::string ns = GlobalFacade::getRef().getServerConfig().getLogMongoDB().db;
	ns += ".log_login";
	mgr.async_insert(mgr.getLogMongo(), ns,
		BSON(mongo::GENOID << "accID" << (long long)info.accID << "charID" << (long long)info.charID << "login" << false << "logTime" << (long long)info.loginTime
		<< "agentIndex" << info.agentIndex << "sID" << info.sID << "onlineTime" << (long long)onlineTime),
		std::bind(&CCharacterManager::mongo_UpdateNormalHandler, this, std::placeholders::_1, info.aID, info.sID, info, "insert log logout."));
}


