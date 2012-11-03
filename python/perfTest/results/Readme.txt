Tests were performed Jan 8 on ccnx 4.2
https://github.com/remap/ndn-lighting/tree/master/python/perfTest

Goal:

per type of interest (symmetric, asymmetric, unsigned):

sign and verify times per system
number of verify failures
number of missing content objects
RTT time


Diagram:

       BORGES                              GUMSTIX
       (controller)                        (interface)

       makeSignedInterest
       expressInterest+-------------------> upcall

Methodology:

	hardware:
		Controller - borges: CPU spect
		Inteface - gumstix: CPU spec
	Network
		100 BaseT Ethernet with Cisco Switch ('no hops')
			average ping latency of .52 ms
		ccndc tcp /ncn/ucla.edu/apps/lighting tcp borges.metwi.ucla.edu

	Sofware:
		ccnx 4.2 on both hosts
		PyCCN
		ndn-lighting performance test code
	

	cProfile for profiling code
	custom log for tracking ccn packets through application
	tcpdump for capturing all traffic (for troubleshooting if needed)

	cProfile was used to derive verify times (total time in verify / # interests)
	verify failures, missing COs, and RTT time were derived from logs. 

	detailed commands can be found at GIT url, above.

Results:

Number of packets used each time: 2611
very
Symmetric:

Sign time at 
