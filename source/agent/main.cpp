
/*
* breeze License
* Copyright (C) 2014 YaweiZhang <yawei_zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http ://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include <zsummerX/FrameHeader.h>
#include <zsummerX/FrameTcpSessionManager.h>
#include <zsummerX/FrameMessageDispatch.h>
#include <unordered_map>
using namespace zsummer::log4z;

#include "Application.h"


int main(int argc, char* argv[])
{

#ifndef _WIN32
//! linux下需要屏蔽的一些信号
signal( SIGHUP, SIG_IGN );
signal( SIGALRM, SIG_IGN );
signal( SIGPIPE, SIG_IGN );
signal( SIGXCPU, SIG_IGN );
	signal( SIGXFSZ, SIG_IGN );
	signal( SIGPROF, SIG_IGN ); 
	signal( SIGVTALRM, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGCHLD, SIG_IGN);
#endif

	
	
	std::string filename = "../ServerConfig.xml";
	unsigned int serverIndex = 0;
	if (argc > 1)
	{
		filename = argv[1];
	}
	if (argc > 2)
	{
		serverIndex = atoi(argv[2]);
	}
	
	Appliction app;
	if (!app.Init(filename, serverIndex))
	{
		return 1;
	}
	app.RunPump();


	return 0;
}

