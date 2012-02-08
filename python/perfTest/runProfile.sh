#!/bin/sh

#for some reason gumstix $HOSTNAME not getting set automatically
# can't make it work per-context yet... requires editing:

#INTERFACE_HOST = "131.179.141.19" #GUMSTIX
#CONTROLLER_HOST = "borges.metwi.ucla.edu"

# 'here', on $CONTROLLER_HOST :

# 1
# run logger_server (application log)

python logger_server.py &

# 2
# run packet capture on controller host, pre-filter for interface host

/usr/sbin/tcpdump host 131.179.141.19 -w profile_0.pcap -s 5000 -Nf

# 3 - lighting interface

# ssh INTERFACE_HOST, run:
ssh tv1 python -m cProfile -o interface_symm.profile interface_symm.py interface_cfg &

sleep 5

# 4 - lighting controller

# on CONTROLLER HOST, run:
python -m cProfile -o control_symm.profile controller_symm.py controller_cfg &




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

