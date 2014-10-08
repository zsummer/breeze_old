
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

#ifndef _CHARACTER_MANAGER_H_
#define _CHARACTER_MANAGER_H_
#include <ProtoDefine.h>
#include <ProtoCommon.h>
#include <ProtoLogin.h>
#include <MongoManager.h>
#include <BaseHander.h>
#include "../core/GlobalFacade.h"


struct LogicCharacterInfo 
{
	CharacterInfo charInfo;
	SessionInfo sInfo;
};


class CCharacterManager : public CBaseHandler
{
public:
	CCharacterManager(){}
	~CCharacterManager(){}
	virtual bool Init() override final;
	bool mongo_LoadLastCharID();
	void msg_LoadAccountInfoReq(SessionID sID, ProtoID pID, ReadStreamPack & rs);
	void mongo_LoadAccountInfo(const std::shared_ptr<CMongoManager::MongoRetDatas> & retDatas, const std::string &errMsg, SessionID sID, SessionInfo & info);
	void mongo_LoadLittleCharInfo(const std::shared_ptr<CMongoManager::MongoRetDatas> & retDatas, const std::string &errMsg, SessionID sID, SessionInfo & info);

	void msg_CreateCharacterReq(SessionID sID, ProtoID pID, ReadStreamPack & rs);
	void mongo_CreateCharacter(const std::string &errMsg, SessionID sID, SessionInfo & info, const LittleCharInfo & lci);

	void msg_CharacterLoginReq(SessionID sID, ProtoID pID, ReadStreamPack & rs);
	void mongo_LoadCharacterInfo(const std::shared_ptr<CMongoManager::MongoRetDatas> & retDatas, const std::string &errMsg, SessionID sID, SessionInfo & info);

	void mongo_UpdateNormalHandler(const std::string &errMsg, SessionID sID, SessionInfo & info, const std::string & msg);


	void msg_CharacterLogout(SessionID sID, ProtoID pID, ReadStreamPack & rs);
	void on_CharLogin(const SessionInfo & info);
	void on_CharLogout(const SessionInfo & info);
private:
	std::unordered_map<AccountID, std::shared_ptr<AccountInfo> > m_accInfo;
	std::unordered_map<CharacterID, std::shared_ptr<LogicCharacterInfo> > m_charInfo;
	GenObjectID m_genObjCharID;
};




































#endif
