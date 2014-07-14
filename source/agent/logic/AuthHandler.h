
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
#include <zsummerX/FrameMessageDispatch.h>
#include <zsummerX/FrameTcpSessionManager.h>

/*
* ·þÎñ¶ËhandlerÀà
*/
class AuthHandler
{
public:
	AuthHandler()
	{
		CMessageDispatcher::getRef().RegisterSessionMessage(ID_C2AS_AuthReq,
			std::bind(&AuthHandler::msg_AuthReq, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}

	void msg_AuthReq(AccepterID aID, SessionID sID, ProtocolID pID, ReadStreamPack & rs)
	{
		ProtoAuthReq req;
		rs >> req;
		LOGD("ID_C2AS_AuthReq user=" << req.info.user << ", pwd=" << req.info.pwd);

		WriteStreamPack ws;
		ProtoAuthAck ack;
		ack.retCode = 0;
		ws << ID_AS2C_AuthAck << ack;
		CTcpSessionManager::getRef().SendOrgSessionData(aID, sID, ws.GetStream(), ws.GetStreamLen());
	};
};




































#endif
