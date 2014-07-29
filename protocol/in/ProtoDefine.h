

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

#ifndef _DEFINE_PROTO_H_
#define _DEFINE_PROTO_H_
#include <string>
#include <zsummerX/FrameHeader.h>
//! 基本类型
typedef char i8;
typedef unsigned char ui8;
typedef short i16;
typedef unsigned short ui16;
typedef int i32;
typedef unsigned int ui32;
typedef long long i64;
typedef unsigned long long ui64;

//! 逻辑类型
typedef ui64 AccountID;
const ui64 InvalidAccountID = (AccountID) -1;
typedef ui64 CharacterID;
const ui64 InvalidCharacterID = (CharacterID)-1;
typedef ui64 ItemID;
const ui64 InvalidItemID = (ItemID)-1;
typedef ui32 NodeIndex;
const NodeIndex InvalidNodeIndex = (NodeIndex)-1;

typedef ui32 ServerNode;
const ServerNode InvalideServerNode = (ServerNode)-1;
const ServerNode AgentNode = 0;
const ServerNode AuthNode = 1;
const ServerNode CenterNode = 2;
const ServerNode DBAgentNode = 3;
const ServerNode LogicNode = 4;

typedef ui32 NodeIndex;
const NodeIndex InvalideNodeIndex = (NodeIndex)-1;



// 服务器内部控制用通讯协议区间为[)
const ui16 MIN_SERVER_CONTROL_PROTO_ID = 0;
const ui16 MAX_SERVER_CONTROL_PROTO_ID = 1000;

// 服务器内部逻辑用通讯协议区间为[)
const ui16 MIN_IN_PROTO_ID = 1000;
const ui16 MAX_IN_PROTO_ID = 20000;

//非认证情况下客户端通讯协议[)
const ui16 MIN_OUT_UNAUTH_PROTO_ID = 20000;
const ui16 MAX_OUT_UNAUTH_PROTO_ID = 20100;

//认证后的通讯协议区间为[)
const ui16 MIN_OUT_PROTO_ID = 20100;
const ui16 MAX_OUT_PROTO_ID = 40000;


//CLIENT
const ui16 MIN_C_R_RESERVE_PROTO_ID = 51000;
const ui16 MAX_C_R_RESERVE_PROTO_ID = 65535;



//客户端的请求协议可以根据以下函数判断
inline bool isClientPROTO(ui16 protoID) { return protoID >= MIN_OUT_UNAUTH_PROTO_ID && protoID < MAX_OUT_PROTO_ID; }
inline bool isNeedAuthClientPROTO(ui16 protoID) { return protoID >= MIN_OUT_PROTO_ID && protoID < MAX_OUT_PROTO_ID; }


struct SessionInfo 
{
	AccountID accID = InvalidAccountID;
	CharacterID charID = InvalidCharacterID;
	NodeIndex agentIndex = InvalideNodeIndex;
	AccepterID aID = InvalidAccepterID;
	SessionID sID = InvalidSeesionID;
};

template <class STM>
STM & operator << (STM & stm, const SessionInfo & info)
{
	stm << info.accID << info.charID
		<< info.agentIndex << info.aID << info.sID;
	return stm;
}
template <class STM>
STM & operator >> (STM & stm, SessionInfo & info)
{
	stm >> info.accID >> info.charID
		>> info.agentIndex >> info.aID >> info.sID;
	return stm;
}

struct AgentSessionInfo
{
	SessionInfo sInfo;
	time_t lastLoginTime = time(NULL);
	time_t lastActiveTime = time(NULL);
};

























#endif
