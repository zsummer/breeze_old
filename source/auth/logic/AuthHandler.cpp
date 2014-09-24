
#include "AuthHandler.h"
#include "../core/GlobalFacade.h"




bool CAuthHandler::Init()
{
	CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
		std::bind(&CAuthHandler::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	return true;
}


void CAuthHandler::msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
{
	SessionInfo info;
	C2AS_AuthReq req;
	rs >> info >> req;
	LOGD("ID_C2AS_AuthReq user=" << req.info.user << ", pwd=" << req.info.pwd);





	// 			//debug 截断认证流程检测协议QPS速度
	// 			ack.retCode = EC_SUCCESS;
	// 			ack.accountID = 101;
	// 			break;
	// 			//end debug

	std::string ns = GlobalFacade::getRef().getServerConfig().getAuthMongoDB().db;
	ns += ".cl_auth";
	CMongoManager & mgr = GlobalFacade::getRef().getMongoManger();
	mgr.async_query(mgr.getAuthMongo(), ns, QUERY("_id" << req.info.user),
		std::bind(&CAuthHandler::mongo_GetAuthInfo, this, std::placeholders::_1, std::placeholders::_2, aID, sID, info, req));

	// 			//debug
	// 			static long long seq = 0;
	// 			seq++;
	// 			{
	// 				authMongo->update(db, BSON("_id" << req.info.user + boost::lexical_cast<std::string>(seq)),
	// 					 BSON("pwd" << "123" << "accID" << seq), true);
	// 				std::string errStr = authMongo->getLastError();
	// 				if (!errStr.empty())
	// 				{
	// 					LOGW("error: " << errStr);
	// 				}
	// 				
	// 				else
	// 				{
	// 					ack.accountID = seq;
	// 					ack.retCode = EC_SUCCESS;
	// 					LOGD("auth success req user=" << req.info.user << ", req pwd=" << req.info.pwd << ", result pwd=" << "123" << ", result accID=" << seq);
	// 					break;
	// 				}
	// 			}
	// 			//!end debug





}


void CAuthHandler::mongo_GetAuthInfo(const std::shared_ptr<CMongoManager::MongoRetDatas>  &retDatas, const std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo info, const C2AS_AuthReq & req)
{
	AS2C_AuthAck ack;
	ack.retCode = EC_DB_ERROR;
	ack.accountID = InvalidAccountID;
	do
	{
		try
		{
			if (!retDatas->empty())
			{
				auto obj = retDatas->at(0);
				std::string pwd = obj.getField("pwd").str();
				AccountID accID = obj.getField("accID").numberLong();
				if (pwd == req.info.pwd)
				{
					ack.accountID = accID;
					ack.retCode = EC_SUCCESS;
					LOGD("auth success req user=" << req.info.user << ", req pwd=" << req.info.pwd << ", result pwd=" << pwd << ", result accID=" << accID);
					break;
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


	WriteStreamPack ws(zsummer::proto4z::UBT_STATIC_AUTO);
	RT2OS_RouteToOtherServer route;
	route.dstNode = info.srcNode;
	route.dstIndex = info.srcIndex;
	route.routerType = RT_SPECIFIED;
	info.srcNode = AgentNode;
	info.srcIndex = GlobalFacade::getRef().getServerConfig().getOwnNodeIndex();
	ws << ID_RT2OS_RouteToOtherServer << route << ID_AS2C_AuthAck << info << ack;
	CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());

}