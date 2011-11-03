NDN Lighting v2.75 - Python

There are three parts to the lighting application:

1) configuration manager

	The configuration manager that authorizes runtime control applications for fixtures.
	
	usage:

	> python cm.py <app_cfg>
	
	at writing, sample app config was sequencer_cfg (leave off the .py for the config files)

2) sequencer

	The sequencer reads commands (current from a database) and sends corrensponding signed interests.

	usage:

	> python sequencer.py <app_cfg>
	

3) controller

	the controller runs on the embedded system and verifies and processes commands to control the light.
	
	the controller needn't be signed, but it does need a correct config file.
	
	> python controller.py <app_cfg>


Status:

as of writing -  1,2,3 are complete.

Signed interests verify, and content objects w/ state of verification are returned. 

next step is to add interpolation / fading for the sequencer...


future todo:

* CM will need to be rewritten w/ two handles replacing the N processes


misc notes:

- though required elements are emerging, config files still in flux. 
  necessary elements are towards top, and noted as such.

- app key files created via openssl, script to do so at _/makeAppKey.sh

- path _/ also contains other scratch work, can ignore.

- to profile, run each script like so (each in different terminal, in this order):

	python -m cProfile -o control.profile controller.py controller_cfg
	python -m cProfile -o sequencer.profile sequencer.py sequencer_cfg

	they will run for a minute (see ccnrun length in controller)
	sequencer will stop if there is no interest response / if controller stops. 

	then run the analysis scripts like so:

	python analyze_stats.py sequencer.profile > sequencer.txt
	python analyze_stats.py control.profile > control.txt