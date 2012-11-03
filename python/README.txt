NDN Lighting v2.5 - Python

######
Usage:
######

In general, this application is very specific to the UCLA campus as it's driving physical lights from physical machines. 

If you would like to have a local version to play with - see the ./perfTest directory - that will issue & respond to interests without controlling lights.

meanwhile, note there are three parts to the lighting application:

1) configuration manager

	The configuration manager that authorizes runtime control applications for fixtures.
	usage:
	> python cm.py <app_cfg>
	at writing, sample app config was sequencer_cfg (leave off the .py for the config files)

2) sequencer

	The sequencer reads commands (current from a database) and sends corresponding signed interests.
	usage:
	> python sequencer.py <app_cfg>
	
	note, instead of reading from DB, one may use any of the patterns (rainbow, fade, etc) in ./patterns directory
	

3) controller

	the controller runs on the embedded system and verifies and processes commands to control the light.
	the controller needn't be signed, but it does need a correct config file.
	usage:
	> python controller.py <app_cfg>


######
Status:
######

1,2,3 are complete.

Signed interests verify, and content objects w/ state of verification are returned. 

TV1 lighting system is controlled via gumstix. TV1 is stable. see

http://borges.metwi.ucla.edu/lighting/app/control.html (u/p tv1/tv1)

(but check camera to ensure room is empty before controlling!)

###########
Next Steps:
###########

in no particular order:

* RGBAW 16-bit lighting control of ColorBlaze fixtures in TV1 (currently they're all 8 bit)
* CM will need to be rewritten w/ two handles replacing the N processes
* multiple names per controller instance (new applications/cm have names, but controller only has it's runtime names)
* serial DMX lighting driver integration to controller
* faster! new signing methods (as RSA is perceptibly slow) elliptic curve possible w/ ccnx, HMAC possible (only w/ UCI ccnx patch)
* fuzz the apps in preparation for red teaming
* build another lighting instance (red team / honeypot)


######################################
Architectural vs Implementation Notes:
######################################

	the following are the remaining differences between tech report & current implementation:

	the 'configuration manager' publishes application keys in control namespace (ie, authorizes applications), but the 'first check' that the app key is signed by root trust is not actually performed on the controller. This nonrepudiation and runtime key hash must be added. also any new application names must somehow make it into the controller, per next issue:
	
	the lights have only one name, across applications. The design specifies and implementation allows more than one name (one namespace per application), but the lighting controller currently only registers prefix for a single namespace. When multiple names per light controller are implemented, the multiple names must live on the lighting controller - likely periodic discovery of names & prefix registrations is preferable to manual copying & instance re-starting of controller instance.

	namecrypto (signed interest library) as implemented still fails to verify consistently enough to allow verification to control side effect (light control) - thus we use it, but perform side effect anyway. The fix may be something simple - it may also be something deeper in the pyccn namecrypto bindings. It's possible but unlikely the problem is with namecrypto itself.

	the 'bootstrapping' as indicated in the tech report is not possible (key not burned-in @ mfr, not all lighting devices can be discovered), but we speak to that with a single config file that can be populated partially automatically. In practice we just edit the config files by hand.


###########
misc notes:
###########

- though required elements are emerging, config files still in flux. 
  necessary elements are towards top, and noted as such.

- app key files created via openssl, script to do so at _/makeAppKey.sh

- path _/ also contains other scratch work, can ignore.

- ./perfTest is used for performance profiling

- no local db? fwd port so local dev is easy: ssh borges.metwi.ucla.edu -L27017:localhost:27017