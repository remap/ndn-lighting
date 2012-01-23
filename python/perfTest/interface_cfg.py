###########################
# NDN Application Configuration
###########################
# this section is needed for configuration manager to authorize the app's namespace

appName = "controller"
appPrefix = "ccnx:/ndn/ucla.edu/apps/lighting/TV1/"
appDescription = "lighting controller"
keyFile = "controller.pem"

# for namecrypto
fixtureKey = "1234567812345678"
#length of time (in ms) valid
window = 3000000


capabilities = {"setRGB", "readRGB"}
appDeviceNames = {"living-room-front","living-room-right","window-left"}

controlNameSpace = {
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/window-left/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-front/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/window-left/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-front/setRGB"
}

# list of authorized applications - used to determine interest priority. 
authorizedApplications = {
"alarm",
"TV1webSequecner",
"TV1ArtNetFader",
"TV1Sequencer"
}

# simulation of burned in names
#
# these could be pulled in from external server via https or ccnx
# but that is in itself a hack / temporary, so for now we put them here & can extend in future
# note this was built to also be read from C -
# thus if expanded, change to match per-item param count:
numValPerKey = 4 # right now just MAC/UID, IP, MfrTypeComponent, KINET UDP Port
deviceList = (
'00:1c:42:00:00:00', '192.168.3.52', 'phillips/ColorBlast', 50009,
'00:1c:42:00:00:02', '192.168.3.53', 'phillips/ColorBlastTRX', 50011,
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

#Logging
logIP = '131.179.141.15'
logPort = 9020

# MongoDB (for image analysis)
# collection name
colName = "lighting"
dbHost = "localhost"
dbPort = 27016
#if dev on localhost w/o mongodb, just forward the borges port. ie:
# ssh -v -L 27016:localhost:27016 borges.metwi.ucla.edu

#temporary runtime block (also for use by analysis)
#technically all we need here is name, UDP, and DMX channel
#yet until we decide how/where to merge it:

names=[

{'name':'living-room-left' , 'DMX':1,'TYPE':"ColorBlazeL", 'UDP':50013},
{'name':'living-room-right', 'DMX':1,'TYPE':"ColorBlazeR", 'UDP':50012},
{'name':'window-right'     , 'DMX':1,'TYPE':"ColorBlast", 'UDP':50009},
{'name':'entrance-door'    , 'DMX':2,'TYPE':"ColorBlast", 'UDP':50009},
{'name':'stairs'           , 'DMX':3,'TYPE':"ColorBlast", 'UDP':50009},
{'name':'bedroom'          , 'DMX':4,'TYPE':"ColorBlast", 'UDP':50009},
{'name':'incandescent'     , 'DMX':1,'TYPE':"ArtNet"    , 'UDP':50010},
{'name':'pastel-right-c'   , 'DMX':1,'TYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-left-b'    , 'DMX':2,'TYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-right-d'   , 'DMX':3,'TYPE':"ColorBlastTRX", 'UDP':50011},
{'name':'pastel-left-a'    , 'DMX':4,'TYPE':"ColorBlastTRX", 'UDP':50011}

]

#max number of lights per Data Enabler
#needed by driver but legacy/to be depricated
numLights = "4"


#depth (from right) of device name
deviceNameDepth = 5
#ccnx:/ndn/ucla.edu/apps/lighting/TV1/pastel-left-b/setRGB/000000
# ideally this should be derived during runtime (once)

# in seconds: -1 is forver
# for main ccnx run, useful for profiling
runtimeDuration = -1