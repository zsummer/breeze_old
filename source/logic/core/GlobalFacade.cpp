#include <MongoManager.h>
#include "GlobalFacade.h"
#include <ServerConfig.h>
#include "NetManager.h"
#include "../logic/CharacterManager.h"

GlobalFacade::GlobalFacade()
{
	m_serverConfig = new ServerConfig;
	m_netManger = new CNetManager();
	m_mongoManager = new CMongoManager();
	m_charManager = new CCharacterManager();
}

GlobalFacade::~GlobalFacade()
{
}

GlobalFacade & GlobalFacade::getRef()
{
	static GlobalFacade g_facade;
	return g_facade;
}