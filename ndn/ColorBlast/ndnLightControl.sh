#!/bin/bash
export PATH=$PATH:/home/root/ccn/ccnx/bin
ccndstart
ccndc add / udp $1
#ccndc add $1 udp $2
./ndnLightControl_1.sh &
#&>control1.log 
./ndnLightControl_2.sh &
#&>control2.log
