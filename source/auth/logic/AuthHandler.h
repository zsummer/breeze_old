
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

#ifndef _AUTH_HANDLER_H_
#define _AUTH_HANDLER_H_
#include <ProtoDefine.h>
#include <ProtoAuth.h>
#include <ProtoServerControl.h>
#include "../core/GlobalFacade.h"
#include <BaseHander.h>
#include <MongoManager.h>

class CAuthHandler : public CBaseHandler
{
public:
	CAuthHandler(){}
	~CAuthHandler(){}
	virtual bool Init() override final;
	void msg_AuthReq(SessionID sID, ProtoID pID, ReadStreamPack & rs);
	void mongo_GetAuthInfo(const std::shared_ptr<CMongoManager::MongoRetDatas>  &retDatas, const std::string &errMsg, SessionID sID, SessionInfo info, const C2AS_AuthReq & req);
private:
};




































#endif
