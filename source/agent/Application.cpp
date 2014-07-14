#include "Application.h"
#include <log4z/log4z.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
#include "core/GlobalFacade.h"
#include <ServerConfig.h>
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


	ret = GlobalFacade::getRef().getServerConfig()->Load("agent", filename, index);
	if (!ret)
	{
		return ret;
	}
	ret = CTcpSessionManager::getRef().Start();
	if (!ret)
	{
		return ret;
	}

	tagAcceptorConfigTraits traits;
	traits.aID = 1;
	traits.listenIP = GlobalFacade::getRef().getServerConfig()->getAgentListen().ip;
	traits.listenPort = GlobalFacade::getRef().getServerConfig()->getAgentListen().port;
	traits.maxSessions = 5000;
	if (CTcpSessionManager::getRef().AddAcceptor(traits) == InvalidAccepterID)
	{
		return false;
	}
	LOGI("Application Init Success." );
	return true;
}

void Appliction::RunPump()
{
	return CTcpSessionManager::getRef().Run();
}