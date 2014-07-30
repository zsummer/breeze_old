cd auth
start auth.exe ../ServerConfig.xml 0
start auth.exe ../ServerConfig.xml 1
rem set /p wait=
ping 127.0.0.1 -n 2 >nul

cd ../dbagent
start dbagent.exe ../ServerConfig.xml 0
rem set /p wait=
ping 127.0.0.1 -n 2 >nul


cd ../center
start center.exe ../ServerConfig.xml 0
rem set /p wait=
ping 127.0.0.1 -n 2 >nul

cd ../logic
start logic.exe ../ServerConfig.xml 0
rem set /p wait=
ping 127.0.0.1 -n 2 >nul

cd ../agent
start agent.exe ../ServerConfig.xml 0
start agent.exe ../ServerConfig.xml 1


cd ..
ping 127.0.0.1 -n 2 >nul