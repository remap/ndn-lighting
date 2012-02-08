#PROFILING : profiling of NDN performance in testbed on controller & interface

controller - 'sequencer' - expreses interests on borges
interface  - 'interface controller' - processes interests on gumstix

####


########
overview:
########

the goal 

as of writing, we have three types of interest (unsigned, asymmetric, symmetric) and one type of content object (asymmetrically signed).

to gauge performance of each, one must run the pairs of the following files, on the different machines:

A) asymmetric interest (with asymmetric content object):

controller_asymm.py
interface_asymm.py

B) symmetric interest (with asymmetric content object):

controller_symm.py
interface_symm.py

C) unsigned (with no content object)
controller_unsigned.py
interface_unsigned.py


##################
# Capture all traffic:

	to capture the network traffic for later analysis
	one can run on borges:
	
	/usr/sbin/tcpdump -w -N profile_1.pcap -s 5000 tcp port 55126
	or to limit to just gumstix:
	/usr/sbin/tcpdump host 131.179.141.19 -w profile_0.pcap -s 5000 -Nf
	

##################
# how to profile
	useful for application verify and other loop times

	log into machines & execute each script, in this order:

	user: lighting
	pass: l!ghting2012

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

		can't hurt to ccnrm /ndn/ucla.edu/apps/lighting between tests to clear out namespace

##################
# Detailed app state packet logs:
	useful for RTT, jitter, verification failure and missing content objects.

		both applications log over udp back to controller host.
	
		to run server, 
	
		python logger_server.py
	
		a log file with every app verify and upcall event is created.


#####
NOTES: 

	i think i fixed whatever was super slow before in asymmetric 
		(we don't need to parse keylocator for symmetric verification)

#####
TODO:

	improve timing loops
	determine why so much verify failure
 