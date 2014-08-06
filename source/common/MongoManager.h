
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



//! 服务节点配置必须已经解析好才可以调用这个类的初始化
class CMongoManager
{
public:
	CMongoManager(){}

	inline bool ConnectAuth(const MongoConfig & mc);
	inline bool ConnectInfo(const MongoConfig & mc);

public:
	typedef std::shared_ptr<mongo::DBClientConnection> MongoPtr;

	inline MongoPtr & getAuthMongo(){ return m_authMongo; }
	inline MongoPtr & getInfoMongo(){ return m_infoMongo; }
private:
	std::shared_ptr<mongo::DBClientConnection> m_authMongo;
	std::shared_ptr<mongo::DBClientConnection> m_infoMongo;
};


bool CMongoManager::ConnectAuth(const MongoConfig & mc)
{
	if (m_authMongo)
	{
		return false;
	}
	m_authMongo = std::shared_ptr<mongo::DBClientConnection>(new mongo::DBClientConnection(true));
	try
	{
		std::string dbhost = mc.ip + ":" + boost::lexical_cast<std::string>(mc.port);
		m_authMongo->connect(dbhost);
		if (mc.needAuth)
		{
			std::string errorMsg;
			if (!m_authMongo->auth(mc.db, mc.user, mc.pwd, errorMsg))
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




bool CMongoManager::ConnectInfo(const MongoConfig & mc)
{
	if (m_infoMongo)
	{
		return false;
	}
	m_infoMongo = std::make_shared<mongo::DBClientConnection>(new mongo::DBClientConnection(true));
	try
	{
		std::string dbhost = mc.ip + ":" + boost::lexical_cast<std::string>(mc.port);
		m_infoMongo->connect(dbhost);
		if (mc.needAuth)
		{
			std::string errorMsg;
			if (!m_infoMongo->auth(mc.db, mc.user, mc.pwd, errorMsg))
			{
				LOGI("ConnectInfo failed. db=" << mc.db << ", user=" << mc.user << ", pwd=" << mc.pwd << ", errMSG=" << errorMsg);
				return false;
			}
		}

	}
	catch (const mongo::DBException &e)
	{
		LOGE("ConnectInfo caught:" << e.what());
		return false;
	}
	catch (...)
	{
		LOGE("ConnectInfo mongo auth UNKNOWN ERROR");
		return false;
	}
	LOGI("ConnectInfo mongo Success");
	return true;
}































#endif
