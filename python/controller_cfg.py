
## human configuration for control application

appName = "controller"
#real
appPrefix = "ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/"
#temp (old int)
#appPrefix = "/ucla.edu/apps/lighting/TV1/fixture/"
appDescription = "lighting controller"

keyFile = "controller.pem"
# for namecrypto
fixtureKey = "1234"

numLights = "4"

capabilities = {"setRGB", "readRGB"}
appDeviceNames = {"living-room-front","living-room-right","window-left"}

#depth (from right) of device name
deviceNameDepth = 6

controlNameSpace = {
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/window-left/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-front/readRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/window-left/setRGB",
"ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-front/setRGB"
}

#mappings from name to driver port
ports = {'ArtNet': 50010, 'ColorBlast': 50009, 'ColorBlaze1':50012,'ColorBlaze2':50013}

# simulation of burned in names
#
# these could be pulled in from external server via https or ccnx
# but that is in itself a hack / temporary, so for now we put them here & can extend python in future

deviceList = (
'00:1c:42:00:00:08', '169.192.0.13', 'phillips/ColorBlast/',
'00:1c:42:00:00:04', '169.192.0.14', 'phillips/ColorBlaze',
'00:1c:42:00:00:06', '169.192.0.12', 'gumstix/overo/fire'
'00:0A:C5:23:82:55', '169.192.0.51', 'phillips/ColorFlex'
)
# to allow expansion of list if needed
# change if change above list schema
numValPerKey = 3
# right now just MAC, IP, MfrTypeComponent


# linkage of deviceList to application aliases
#
#not used in this file (yet), but in web/config.py (for UAG / sequencer / analyzer)
# application device name (alias) mapping to default/config device name

deviceNames = {
"living-room-front":"ColorBlast/1",
"living-room-right":"ColorBlast/1",
"entrance-door":"ColorBlast/2",
"window-right":"ColorBlast/3",
"stairs":"ColorBlast/4",
"bedroom":"ColorBlast/2",
"kitchen":"ColorBlast/2",
"window-left":"ColorBlast/3",
"incandescent":"ArtNet"}

deviceNamesToDriverPort = {
"living-room-front":"ColorBlast/1",
"living-room-right":"ColorBlast/1",
"entrance-door":"ColorBlast/2",
"window-right":"ColorBlast/3",
"stairs":"ColorBlast/4",
"bedroom":"ColorBlast/2",
"kitchen":"ColorBlast/2",
"window-left":"ColorBlast/3",
"incandescent":"ArtNet"}


# this is for TV1 - not used @ mo, just for notes
kinetDeviceList = (
'192.168.3.52', 'colorBlast',
'192.168.3.51', 'colorBlaze1',
'192.168.3.50', 'colorBlaze2',
'131.179.141.17', 'ArtNet'
)
# also note IP address is for Kinet and is to be auto-detected and written *after* ccnx cfg handshake
# the only thing humans should enter to this file is the serial of the devices & the typeComponent
# the serial field could be populated with MAC address for now.
# as well as anything necessary for application signing


#temporary translation until finalized / moved elsewhere
names = {
"living-room-front":"ColorBlast/1",
"living-room-right":"ColorBlast/1",
"entrance-door":"ColorBlast/2",
"window-right":"ColorBlast/3",
"stairs":"ColorBlast/4",
"bedroom":"ColorBlast/2",
"kitchen":"ColorBlast/2",
"window-left":"ColorBlast/3",
"incandescent":"ArtNet"}

nameFromAnalysis = {'living-room-front': 'living-room-front-wall',
        'living-room-right': 'living-room-right-wall',
        'kitchen':'kitchen',
        'stairs':'stairs',
        'bedroom':'bedroom',
        'entrance-door':'entrance-door',
        'window-left':'window-left',
        'window-right':'window-right',
        'incandescent':'incandescent',
        'living-room-left/fill':'living-room-left/fill',
        'living-room-right/fill':'living-room-right/fill',
        #'colorflex':'colorflex',
        2: 'living-room-front-wall',
        3: 'living-room-right-wall',
        1:'kitchen',
        6:'stairs',
        5:'bedroom',
        7:'entrance-door',
        4:'window-left',
        8:'window-right',
        9:'living-room-left/fill',
        10:'living-room-right/fill',

        }