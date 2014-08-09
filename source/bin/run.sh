#!/usr/bin/bash
cd auth
./auth ../ServerConfig.xml 0  2>/dev/null 1>&2 &
./auth ../ServerConfig.xml 1  2>/dev/null 1>&2 &
sleep 2

cd ../dbagent
./dbagent ../ServerConfig.xml 0  2>/dev/null 1>&2 &
sleep 2

cd ../logic
./logic ../ServerConfig.xml 0  2>/dev/null 1>&2 &
sleep 2

cd ../center
./center ../ServerConfig.xml 0  2>/dev/null 1>&2 &
sleep 2


cd ../agent
./agent ../ServerConfig.xml 0  2>/dev/null 1>&2 &
./agent ../ServerConfig.xml 1  2>/dev/null 1>&2 &

sleep 2
