cd auth
start auth.exe ../ServerConfig.xml 0
start auth.exe ../ServerConfig.xml 1
ping 127.0.0.1 -n 3 >nul
cd ..

cd agent
start agent.exe ../ServerConfig.xml 0
start agent.exe ../ServerConfig.xml 1
ping 127.0.0.1 -n 3 >nul
cd ..

ping 127.0.0.1 -n 3 >nul