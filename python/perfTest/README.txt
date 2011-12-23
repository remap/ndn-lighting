#PROFILING

profiling of NDN performance in testbed on controller & interface

controller - 'sequencer' - expreses interests on borges
interface  - 'interface controller' - processes interests on gumstix


as of writing, we have three types of interest (unsigned, asymmetric, symmetric) and two types of content object (asymmetric, unsigned). 

to gauge performance of each, one must run the pairs of the following files, on the different machines:

1) asymmetric interest (with asymmetric content object):

controller_asymm.py
interface_asymm.py

2) symmetric interest (with asymmetric content object):

controller_symm.py
interface_symm.py

3) unsigned (with no content object)
controller_unsigned.py
interface_unsigned.py


##################
# how to profile

log into machines & execute each script, in this order:

user: lighting
pass: l!ghting2011

on borges:
	cd /home/lighting/ndn-lighting/python/perfTest
	python -m cProfile -o control.profile controller.py controller_cfg

on TV1 gumstix:
	cd /home/lighting/ndn-lighting/python/perfTest
	python -m cProfile -o interface.profile interface.py interface_cfg

	they will run for a moment, sequencer will stop when controller stops. 

	when the processes stop, the *.profile files have the session information.

	then run the analysis scripts to make it human-readable:

	python analyze_stats.py interface.profile > interface.txt
	python analyze_stats.py control.profile > control.txt

##################
# Capture traffic:

	to capture the traffic for later analysis
	one can run on borges:
	
	/usr/sbin/tcpdump -w profile_1.pcap -s 5000 tcp port 55126

##################
# Detailed packet logs:

	need to get full Verify Result, RTT, and Dropped Packet per interest and content object.
	to do so, making a log structure to capture everything in RAM while performing
	and then write out / analyze
	for RTT, verify, and drops for interests & COs

#####
TODO:

	detailed packet logs
	improve timing loops
 