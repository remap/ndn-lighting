#!/usr/local/bin/bash
PID=`ps -aux|grep ccnrr|grep -v kill|grep -v 'grep'|awk '{print \$2}'`
kill -9 ${PID}
