
/*
* breeze License
* Copyright (C) 2014 YaweiZhang <yawei_zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _MONGO_MANAGER_H_
#define _MONGO_MANAGER_H_
#include <ProtoDefine.h>
#include <ProtoCommon.h>
#include <InProtoCommon.h>
#include <ProtoAuth.h>
#include <ServerConfig.h>
#include <mongo/client/dbclient.h>
#include <zsummerX/FrameTcpSessionManager.h>



//! 服务节点配置必须已经解析好才可以调用这个类的初始化
class CMongoManager
{
public:
	typedef std::shared_ptr<mongo::DBClientConnection> MongoPtr;
public:
	CMongoManager(){}
	~CMongoManager(){}
	inline bool StartPump();
	inline bool StopPump();
	inline bool ConnectMongo(MongoPtr &mongoPtr, const MongoConfig & mc);


	inline void async_query(MongoPtr &mongoPtr, const string &ns, const mongo::Query & query,
		const std::function<void(shared_ptr<mongo::DBClientCursor>, std::string)> & handler);


public:

	inline MongoPtr & getAuthMongo(){ return m_authMongo; }
	inline MongoPtr & getInfoMongo(){ return m_infoMongo; }

protected:
	inline void _async_query(MongoPtr &mongoPtr, const string &ns, const mongo::Query & query,
		std::function<void(shared_ptr<mongo::DBClientCursor>, std::string)> handler);
	void Run()
	{
		do 
		{
			if (!m_bRuning)
			{
				break;
			}
			try
			{
				m_summer.RunOnce();
			}
			catch (...)
			{
			}
			
		} while (true);
	}
private:
	std::shared_ptr<mongo::DBClientConnection> m_authMongo;
	std::shared_ptr<mongo::DBClientConnection> m_infoMongo;
	std::shared_ptr<std::thread> m_thread;
	zsummer::network::CZSummer m_summer;
	bool m_bRuning = false;
};

inline bool CMongoManager::StartPump()
{
	if (m_thread)
	{
		return false;
	}
	m_bRuning = true;
	m_thread = std::shared_ptr<std::thread>(new std::thread(std::bind(&CMongoManager::Run, this)));
	return true;
}

bool CMongoManager::ConnectMongo(MongoPtr & mongoPtr, const MongoConfig & mc)
{
	if (mongoPtr)
	{
		return false;
	}
	mongoPtr = std::shared_ptr<mongo::DBClientConnection>(new mongo::DBClientConnection(true));
	try
	{
		std::string dbhost = mc.ip + ":" + boost::lexical_cast<std::string>(mc.port);
		mongoPtr->connect(dbhost);
		if (mc.needAuth)
		{
			std::string errorMsg;
			if (!mongoPtr->auth(mc.db, mc.user, mc.pwd, errorMsg))
			{
				LOGI("ConnectAuth failed. db=" << mc.db << ", user=" << mc.user << ", pwd=" << mc.pwd << ", errMSG=" << errorMsg);
				return false;
			}
		}

	}
	catch (const mongo::DBException &e)
	{
		LOGE("ConnectAuth caught:" << e.what());
		return false;
	}
	catch (...)
	{
		LOGE("ConnectAuth mongo auth UNKNOWN ERROR");
		return false;
	}
	LOGI("ConnectAuth mongo Success");
	return true;
}


void CMongoManager::async_query(MongoPtr & mongoPtr, const string &ns, const mongo::Query & query,
	const std::function<void(shared_ptr<mongo::DBClientCursor>, std::string)> &handler)
{
	m_summer.Post(std::bind(&CMongoManager::_async_query, this, mongoPtr, ns, query, handler));
}


void CMongoManager::_async_query(MongoPtr & mongoPtr, const string &ns, const mongo::Query & query,
	std::function<void(shared_ptr<mongo::DBClientCursor>, std::string)>  handler)
{
	std::string ret;
	try
	{
		std::shared_ptr<mongo::DBClientCursor> sc(mongoPtr->query(ns, query));
		CTcpSessionManager::getRef().Post(std::bind(handler, sc , ret));
		return;
	}
	catch (const mongo::DBException &e)
	{
		ret += "mongodb async_query catch error. ns=" + ns + ", query=" + query.toString() + ", error=" + e.what();
	}
	catch (...)
	{
		ret += "mongodb async_query unknown error. ns=" + ns + ", query=" + query.toString();
	}
	CTcpSessionManager::getRef().Post(std::bind(handler, shared_ptr<mongo::DBClientCursor>(), ret));
}































#endif
