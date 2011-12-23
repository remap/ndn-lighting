To profile, run each script like so (each in different terminal, in this order):

	python -m cProfile -o control.profile controller.py controller_cfg
	python -m cProfile -o sequencer.profile sequencer.py sequencer_cfg

	they will run for a minute (see ccnrun length in controller)
	sequencer will stop if there is no interest response / if controller stops. 

	then run the analysis scripts like so:

	python analyze_stats.py sequencer.profile > sequencer.txt
	python analyze_stats.py control.profile > control.txt
	
	to capture the traffic for later analysis one can run on borges:
	
	/usr/sbin/tcpdump -w profile_1.pcap -s 5000 tcp port 55126
