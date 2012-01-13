###########################
# NDN Application Configuration
###########################
# this section is needed for configuration manager to authorize the app's namespace

appName = "TV1 Rainbow"
appPrefix = "ccnx:/ndn/ucla.edu/apps/lighting/TV1/"
appDescription = "Color test pattern for TV1"
keyFile = "sequencer.pem"

# for namecrypto
fixtureKey = "1234567812345678"

capabilities = {"setRGB", "readRGB"}
appDeviceNames = {"living-room-front","living-room-right","window-left"}

controlNameSpace = {
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-right/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/window-left/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-front/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-right/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/window-left/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/living-room-front/setRGB"}

# simulation of burned in names
#
# these could be pulled in from external server via https or ccnx
# but that is in itself a hack / temporary, so for now we put them here & can extend in future
# note this was built to also be read from C -
# thus if expanded, change to match per-item param count:
numValPerKey = 4 # right now just MAC/UID, IP, MfrTypeComponent, KINET UDP Port
deviceList = (
'00:1c:42:00:00:00', '192.168.3.52', 'phillips/ColorBlast', 50009,
'00:1c:42:00:00:02', '192.168.3.53', 'phillips/ColorBlast', 50010,
'00:1c:42:00:00:04', '192.168.3.51', 'phillips/ColorBlaze', 50012,
'00:1c:42:00:00:08', '169.192.0.50', 'phillips/ColorBlaze', 50013,
'00:1c:42:00:00:10', '131.179.141.17', 'ArtNet', 50010
)

# also note IP address is for Kinet and is to be auto-detected and written *after* ccnx cfg handshake
# the only thing humans should enter to this file is the serial of the devices & the typeComponent
# the serial field could be populated with MAC address for now.
# as well as anything necessary for application signing


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


#in seconds - performance seems to be between .18 and .2
refreshInterval = .1

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
#{'name':'incandescent'     ,'UID':'######','DMX':1,'MFRTYPE':"ArtNet"    , 'UDP':50010},
{'name':'pastel-right-c'   ,'UID':'######','DMX':1,'MFRTYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-left-b'    ,'UID':'######','DMX':2,'MFRTYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-right-d'   ,'UID':'######','DMX':3,'MFRTYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-left-a'    ,'UID':'######','DMX':4,'MFRTYPE':"ColorBlastTRX", 'UDP':50011}

]
