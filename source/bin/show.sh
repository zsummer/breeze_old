#!/bin/bash
for svr in agent auth center  logic  dbagent StressTest; 
do
	netstat -nlp|grep svr
done


