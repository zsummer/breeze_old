#include <MongoManager.h>
#include "GlobalFacade.h"
#include <ServerConfig.h>

#include "NetManager.h"

GlobalFacade::GlobalFacade()
{
}

GlobalFacade::~GlobalFacade()
{
}

GlobalFacade & GlobalFacade::getRef()
{
	static GlobalFacade g_facade;
	return g_facade;
}

ServerConfig & GlobalFacade::getServerConfig()
{
	static ServerConfig sc;
	return sc;
}
CNetManager & GlobalFacade::getNetManger()
{
	static CNetManager nm;
	return nm;
}
CMongoManager & GlobalFacade::getMongoManger()
{
	static CMongoManager mm;
	return mm;
}