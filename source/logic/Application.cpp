#include <MongoManager.h>
#include "Application.h"
#include <ServerConfig.h>
#include <log4z/log4z.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
#include "core/GlobalFacade.h"

#include <BaseHander.h>
#include "core/NetManager.h"
#include "logic/CharacterManager.h"
using namespace zsummer::log4z;

Appliction::Appliction()
{
	
}

Appliction::~Appliction()
{
}

Appliction & Appliction::getRef()
{
	static Appliction g_facade;
	return g_facade;
}


bool Appliction::Init(std::string filename, unsigned int index)
{
	bool ret = false;

	//加载配置
	ret = GlobalFacade::getRef().getServerConfig().Parse(filename, LogicNode, index);
	if (!ret)
	{
		LOGE("getServerConfig failed.");
		return ret;
	}
	//连接mongodb
	ret = GlobalFacade::getRef().getMongoManger().ConnectMongo(GlobalFacade::getRef().getMongoManger().getInfoMongo(), GlobalFacade::getRef().getServerConfig().getInfoMongoDB());
	if (!ret )
	{
		LOGE("Connect Info mongo failed.");
		return ret;
	}
	//连接mongodb
	ret = GlobalFacade::getRef().getMongoManger().ConnectMongo(GlobalFacade::getRef().getMongoManger().getLogMongo(), GlobalFacade::getRef().getServerConfig().getLogMongoDB());
	if (!ret)
	{
		LOGE("Connect Log mongo failed.");
		return ret;
	}
	if (!GlobalFacade::getRef().getMongoManger().StartPump())
	{
		LOGE("startPump mongo failed.");
		return ret;
	}
	//启动底层网络
	ret = CTcpSessionManager::getRef().Start();
	if (!ret)
	{
		LOGE("CTcpSessionManager Start false.");
		return ret;
	}


	//! -- warning
	//! -- 以下所有handler性质的类 如果需要在启动程序的第一时间进行数据库拉取 需要在init直接对mongo句柄进行阻塞拉取.
	//! -- 在初始化过程中不要调用CMongoManager的异步操作, 避免多线程冲突以及异步后的繁琐处理.
	//! -- end

	//初始化角色类
	ret = GlobalFacade::getRef().getCharManager().Init();
	if (!ret)
	{
		LOGE("CharManager Init false.");
		return ret;
	}
	//初始化handler类
	std::vector<CBaseHandler*> handlers;
	for (auto ptr : handlers)
	{
		if (!ptr->Init())
		{
			LOGW("Appliction Init handler false");
			return false;
		}
	}


	//启动网络
	ret = GlobalFacade::getRef().getNetManger().Start();
	if (!ret)
	{
		LOGE("NetManager Start false.");
		return ret;
	}


	LOGI("Appliction Init success.");
	return true;
}

void Appliction::RunPump()
{
	return CTcpSessionManager::getRef().Run();
}

void Appliction::Stop()
{
	CTcpSessionManager::getRef().CreateTimer(100, std::bind(&Appliction::_Stop, this));	
}



void Appliction::_Stop()
{
	if (GlobalFacade::getRef().getMongoManger().getPostCount() == GlobalFacade::getRef().getMongoManger().getPostCount())
	{
		GlobalFacade::getRef().getMongoManger().StopPump();
		CTcpSessionManager::getRef().Stop();
	}
	else
	{
		CTcpSessionManager::getRef().CreateTimer(1000, std::bind(&Appliction::_Stop, this));
	}
}