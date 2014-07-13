#include "GlobalFacade.h"
#include <ServerConfig.h>
#include "../logic/AuthHandler.h"

GlobalFacade::GlobalFacade()
{
	m_serverConfig = new ServerConfig;
	m_authHandler = new AuthHandler;
}

GlobalFacade::~GlobalFacade()
{
}

GlobalFacade & GlobalFacade::getRef()
{
	static GlobalFacade g_facade;
	return g_facade;
}