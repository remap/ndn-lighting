#PROFILING : profiling of NDN performance in testbed on controller & interface

controller - 'sequencer' - expreses interests on borges
interface  - 'interface controller' - processes interests on gumstix

########
overview:
########

the goal is to report both following variables:

		# of iterations
		RTT max/mean/std dev
		packet verify times
		packet verify failures
		content objects missing

	 as of writing, we have three types of interest:
	 				(unsigned, asymmetric, symmetric)
	 and one type of content object:
	 				(asymmetrically signed).

	to measure each each, we run the applications on the different hosts and capture the following data
	
######
before running:

	ensure route is established between gumstix & borges
	ensure kinet is off (it's auto-on after a gstix reboot)
		gumstix: sudo pkill -9 python
	ensure ssh keys for user 'lighting' have been exchanged. 
	
##################
# Capture all traffic:

	to capture the network traffic for later analysis
	one can run on borges:
	
	/usr/sbin/tcpdump -w -N profile_1.pcap -s 5000 tcp port 55126
	or to limit to just gumstix:
	/usr/sbin/tcpdump host 131.179.141.19 -w profile_0.pcap -s 5000 -Nf
	
##################
# Detailed app state packet logs:
	useful for RTT, jitter, verification failure and missing content objects.

		both applications log over udp back to controller host.

		to run server, 

		python logger_server.py [logfilename]

		a log file with every app verify and upcall event is created.
		
		this assumes that logging ports are open, ie to make sure:
		
		[root@borges nano]# /sbin/iptables -I INPUT -p udp -m udp -s 131.179.141.19 --dport 9020 -j ACCEPT
		[root@borges nano]# /sbin/iptables -I INPUT -p tcp -m tcp -s 131.179.141.19 --dport 9020 -j ACCEPT
		
		to analyze these logs, see the python and R scripts in results/analyze
		ie, analyzePacketLogs.py (editing config.py w/ correct db name)

	

##################
# how to profile
	useful for application verify and other loop times

	log into machines & execute each script, in this order:

	user: lighting
	pass: l!ghting[currentyear]


	on borges:
		ccnrm /ndn/ucla.edu/apps/lighting (to clear out namespace)
		cd /home/lighting/ndn-lighting/python/perfTest
		python -m cProfile -o control.profile controller.py controller_cfg

	on TV1 gumstix:
		ccnrm /ndn/ucla.edu/apps/lighting (to clear out namespace)
		cd /home/lighting/ndn-lighting/python/perfTest
		python -m cProfile -o interface.profile interface.py interface_cfg

		they will run for a moment, sequencer will stop when controller stops. 

		when the processes stop, the *.profile files have the session information.

		then run the analysis scripts to make it human-readable:

		python analyze_stats.py interface.profile > interface.txt
		python analyze_stats.py control.profile > control.txt

	
		you will likely want to stop, rename output, & re-start tcpdump & python_logger between tests
		
		
		note this must be done for:
		controller_symm	  &	  interface_sym
		controller_asym	  &	  interface_asym
		controller_unsigned & interface_unsigned
		
		
########
# SUMMARY:

	these are the commands, per machine, used per trial:

	borges:
		ccnrm /ndn/ucla.edu/apps/lighting
		cd /home/lighting/ndn-lighting/python/perfTest
		sudo /usr/sbin/tcpdump host 131.179.141.19 -w symmetric_0.pcap -s 5000 -Nf &
		python logger_server.py symmetric_0.log &
	gumstix:
		ccnrm /ndn/ucla.edu/apps/lighting
		cd /home/lighting/ndn-lighting/python/perfTest
		python -m cProfile -o symm_interface_0.profile interface_symm.py interface_cfg
	borges:
		python -m cProfile -o symm_controller_0.profile controller_symm.py controller_cfg

	upon completion:

	borges:
		pkill -f logger_server.py (or just ctrl-c or ctrl-\ if in seperate window)
		sudo pkill tcpdump (or just ctrl-c or ctrl-\ if in seperate window)

	
	and yes, this must be done for each:
		controller_symm	  &	  interface_sym
		controller_asym	  &	  interface_asym
		controller_unsigned & interface_unsigned


#####
NOTES: 

	i think i fixed whatever was super slow before in asymmetric 
		(we don't need to parse keylocator for symmetric verification)
		
	there is a start at automatic execution, but the interface script never closes / stdio thing, must debug. meanwhile, manual is best. 

#####
TODO:

	improve timing loops
	determine why so much verify failure (improved, but can be improved further)
  