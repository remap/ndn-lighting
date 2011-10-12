NDN Lighting v2.07 - Python

There are three parts to the lighting application:

1) configuration manager

	The configuration manager that authorizes runtime control applications for fixtures.
	
	usage:

	> python cm.py <app_cfg>
	
	at writing, sample app config was sequencer_cfg (leave off the .py)

2) sequencer

	The sequencer reads commands (current from a database) and sends corrensponding signed interests.

	usage:

	> python sequencer.py <app_cfg>
	

3) controller

	the controller runs on the embedded system and verifies and processes commands to control the light.
	
	the controller needn't be signed, but it does need a correct config file.
	
	> python controller.py <app_cfg>


Status:

as of writing - 1,2 are complete.

3 functions in NDN, but is pending kinet driver control (revision) and signed interests.

note first draft is just raw function, future todo:

1) will need to be rewritten w/ two handles replacing the N processes
2) will have to not just issue interests, but listen for the returned status objects

misc notes:

- though required elements are emerging, config files still in flux. 
  necessary elements are towards top, and noted as such.

- app key files created via openssl, script to do so at _/makeAppKey.sh

- path _/ also contains other scratch work, can ignore.




