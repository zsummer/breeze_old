
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
#include <mongo/client/dbclient.h>
#include <ProtoDefine.h>
#include <ServerConfig.h>



//! 服务节点配置必须已经解析好才可以调用这个类的初始化
class CMongoManager
{
public:
	typedef std::shared_ptr<mongo::DBClientConnection> MongoPtr;
public:
	CMongoManager(){}
	~CMongoManager();
	bool StartPump();
	bool StopPump();
	bool ConnectMongo(MongoPtr &mongoPtr, const MongoConfig & mc);


	void async_query(MongoPtr &mongoPtr, const string &ns, const mongo::Query &query,
		const std::function<void(std::shared_ptr<mongo::DBClientCursor> &, std::string &)> & handler);


public:

	inline MongoPtr & getAuthMongo(){ return m_authMongo; }
	inline MongoPtr & getInfoMongo(){ return m_infoMongo; }

protected:
	void _async_query(MongoPtr &mongoPtr, const string &ns, const mongo::Query &query,
		std::function<void(shared_ptr<mongo::DBClientCursor>&, std::string &)> &handler);
	inline void Run()
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






























#endif
