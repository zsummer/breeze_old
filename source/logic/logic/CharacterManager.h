
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

#include <MongoManager.h>
#include <BaseHander.h>
#include "../core/GlobalFacade.h"
#include <ProtoDefine.h>
#include <ProtoCommon.h>
#include <ProtoLogin.h>

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
	void msg_LoadAccountInfoReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);
	void mongo_LoadAccountInfo(std::shared_ptr<mongo::DBClientCursor> & cursor, std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info);
	void mongo_LoadLittleCharInfo(std::shared_ptr<mongo::DBClientCursor> & cursor, std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info);

	void msg_CreateCharacterReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);
	void mongo_CreateCharacter(std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info, const LittleCharInfo & lci);

	void msg_CharacterLoginReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);
	void mongo_LoadCharacterInfo(std::shared_ptr<mongo::DBClientCursor> & cursor, std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info);

	void mongo_UpdateNormalHandler(std::string &errMsg, AccepterID aID, SessionID sID, SessionInfo & info, const std::string & msg);


	void msg_CharacterLogout(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs);
	void on_CharLogin(const SessionInfo & info);
	void on_CharLogout(const SessionInfo & info);
private:
public:
private:
	std::unordered_map<AccountID, std::shared_ptr<AccountInfo> > m_accInfo;
	std::unordered_map<CharacterID, std::shared_ptr<LogicCharacterInfo> > m_charInfo;
	CharacterID m_seqCharID = 1;
};




































#endif
