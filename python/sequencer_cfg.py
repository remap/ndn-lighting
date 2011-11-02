###########################
# NDN Application Configuration
###########################
# this section is needed for configuration manager to authorize the app's namespace

appName = "TV1 Sequencer"
appPrefix = "ccnx:/ndn/ucla.edu/apps/lighting/TV1/"
appDescription = "builds and expresses interests from analysis DB"
keyFile = "sequencer.pem"

# for namecrypto
fixtureKey = "1234"

capabilities = {"setRGB", "readRGB"}
appDeviceNames = {"living-room-front","living-room-right","window-left"}

controlNameSpace = {
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-right/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/window-left/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-front/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-right/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/window-left/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-front/setRGB"}


############################
# Application Runtime:
############################
# the following are not NDN specific / not required for CM 
# yet still required by app

# MongoDB (for image analysis)
# collection name
colName = "lighting"
dbHost = "localhost"
dbPort = 27016
#if dev on localhost w/o mongodb, just forward the borges port. ie:
# ssh -v -L 27016:localhost:27016 borges.metwi.ucla.edu

#temporary runtime block (also used by analysis)
#technically all we need here is name, UDP, and DMX channel
#yet until we decide how/where to merge it:


#not actually needed by sequencer - but leaving for now until we super-unify everything (again)

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
