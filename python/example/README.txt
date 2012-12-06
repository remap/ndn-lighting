####################
ndn-lighting example
####################

this is the most basic call/response for lighting control. 

interface.py is a 'server' that receives interests, and controls lights. 

sequencer.py is a 'controller' that forms and expresses interests to control lights. 

############
DEPENDENCIES:
############

ccnx	https://github.com/ProjectCCNx/ccnx
pyccn	https://github.com/remap/PyCCN
(with or without 'namecrypto' branch - (which is necessary for other code beyond this example))

######
USAGE:
######

1) plug in USB DMX controller and DMX light. 
2) make sure the device name in the dmx_serial.py file matches the serial name of the port (view dmesg)
3) run the interface controller:

> python interface.py interface_cfg

4) run the sequencer:

> python sequencer.py sequencer_cfg

5) behold ! the DMX light will cycle through values of R, G, B, and then stop. 

######
NOTES:
######

this is a basic example, with all trust / crypto removed. 
This is meant for introduction to pyccn and lighting control - NOT for production or public applications. 
Security must be re-implemented before deployment. see 'perfTest' signed, unsigned, symmetric for various styles using 'namecrypto' library.