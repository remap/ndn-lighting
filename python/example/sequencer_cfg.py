###########################
# NDN Application Configuration
###########################
# this section is needed for configuration manager to authorize the app's namespace
appName = "Basic Lighting sequencer"
appPrefix = "ccnx:/ndn/ucla.edu/apps/lighting/basic/"
appDescription = "Sequencer for lighting control."
keyFile = "sequencer.pem"


############################
# Application Runtime:
############################
# the following are not NDN specific / not required for CM 
# yet still required by app

#in seconds - performance seems to be between .18 and .2
refreshInterval = .015

#temporary runtime block (also used by analysis)
#technically all we need here is name, UDP, and DMX channel
#yet until we decide how/where to merge it:

names=[

{'name':'living-room-left' ,'UID':'######','DMX':1,'MFRTYPE':"ColorBlaze", 'UDP':50013},
{'name':'living-room-right','UID':'######','DMX':1,'MFRTYPE':"ColorBlaze", 'UDP':50012},
{'name':'window-right'     ,'UID':'######','DMX':1,'MFRTYPE':"ColorBlast", 'UDP':50009},
{'name':'entrance-door'    ,'UID':'######','DMX':2,'MFRTYPE':"ColorBlast", 'UDP':50009},
{'name':'stairs'           ,'UID':'######','DMX':3,'MFRTYPE':"ColorBlast", 'UDP':50009},
{'name':'bedroom'          ,'UID':'######','DMX':4,'MFRTYPE':"ColorBlast", 'UDP':50009},
{'name':'incandescent'     ,'UID':'######','DMX':1,'MFRTYPE':"ArtNet"    , 'UDP':50010},
{'name':'pastel-right-c'   ,'UID':'######','DMX':1,'MFRTYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-left-b'    ,'UID':'######','DMX':2,'MFRTYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-right-d'   ,'UID':'######','DMX':3,'MFRTYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-left-a'    ,'UID':'######','DMX':4,'MFRTYPE':"ColorBlastTRX", 'UDP':50011}

]
