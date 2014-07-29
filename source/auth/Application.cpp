#include "Application.h"
#include <log4z/log4z.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
#include "core/GlobalFacade.h"
#include <ServerConfig.h>
#include "core/NetManager.h"
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


	ret = GlobalFacade::getRef().getServerConfig().Parse(filename, AuthNode, index);
	if (!ret)
	{
		LOGE("getServerConfig failed." );
		return ret;
	}
	ret = CTcpSessionManager::getRef().Start();
	if (!ret)
	{
		LOGE("CTcpSessionManager Start false.");
		return ret;
	}
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
	CTcpSessionManager::getRef().Stop();
}