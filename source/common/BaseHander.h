
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


/*
 *  文件说明
 *  CBaseHandler主要作用是给Application提供一个统一管理消息Hander类的一个机制. 
 *  这个机制目前主要有两个. 1是实例化.  2是初始化. 
 *  注册消息可以放在handler类的构造函数中 也可以放在Init()中.
 */


#ifndef _BASE_HANDLER_H_
#define _BASE_HANDLER_H_
#include <ProtoDefine.h>


class CBaseHandler
{
public:
	CBaseHandler(){}
	virtual ~CBaseHandler(){};
	//! 初始化中所有mongo操作必须是同步 禁止在Init中调用异步操作方法.
	virtual bool Init() = 0;
};




































#endif
