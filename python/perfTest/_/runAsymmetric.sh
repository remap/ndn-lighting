#!/bin/sh

#for some reason gumstix $HOSTNAME not getting set automatically
# can't make it work per-context yet... requires editing:

#INTERFACE_HOST = "131.179.141.19" #GUMSTIX
#CONTROLLER_HOST = "borges.metwi.ucla.edu"

# 'here', on $CONTROLLER_HOST :

echo "stopping tcpdump"
sudo /usr/bin/pkill tcpdump

echo "stopping python logger"
sudo /usr/bin/pkill -f logger_server.py


# clean namespace
echo "cleaning namespace..."
ccnrm /ndn/ucla.edu/apps/lighting

# 1
# run logger_server (application log)
echo "\nstarting network log server..."
python logger_server.py events_asymmetric.log &

# 2
# run packet capture on controller host, pre-filter for interface host
echo " starting network packet capture..."
sudo /usr/sbin/tcpdump host 131.179.141.19 -w network_asymmetric.pcap -s 5000 -Nf &

# 3 - lighting interface
echo "\nrunning lighting interface..."
# ssh INTERFACE_HOST, run:
ssh tv1 cd /home/lighting/ndn-lighting/python/perfTest/; /usr/bin/python -m cProfile -o interface_asymm.profile interface_asymm.py interface_cfg &

sleep 5 #give ssh time to start listening on gumstix

# 4 - lighting controller
echo "\nrunning lighting controller..."
# on CONTROLLER HOST, run:
cd /home/lighting/ndn-lighting/python/perfTest/
/usr/bin/python -m cProfile -o control_asymm.profile controller_asymm.py controller_cfg &


#echo "stopping tcpdump"
#sudo /usr/bin/pkill tcpdump

#echo "stopping python logger"
#sudo /usr/bin/pkill -f logger_server.py




#one must run the pairs of the following files, on the different machines:

#A) asymmetric interest (with asymmetric content object):

#controller_asymm.py
#interface_asymm.py

#B) symmetric interest (with asymmetric content object):

#controller_symm.py
#interface_symm.py

#C) unsigned (with no content object)
#controller_unsigned.py
#interface_unsigned.py
