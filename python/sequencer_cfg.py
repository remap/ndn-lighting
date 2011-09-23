## human configuration for application signing:

appName = "TV1Sequencer"
appPrefix = "/ndn/ucla.edu/apps/lighting/TV1"
appDescription = "Sequencer for TV1."
keyFile = "sequencer.pem"

capabilities = {"setRGB", "readRGB"}
appDeviceNames = {"living-room-front","living-room-right","window-left"}

controlNameSpace = {
"/ndn/ucla.edu/apps/lighting/TV1/living-room-right/readRGB",
"/ndn/ucla.edu/apps/lighting/TV1/window-left/readRGB",
"/ndn/ucla.edu/apps/lighting/TV1/living-room-front/readRGB",
"/ndn/ucla.edu/apps/lighting/TV1/living-room-right/setRGB",
"/ndn/ucla.edu/apps/lighting/TV1/window-left/setRGB",
"/ndn/ucla.edu/apps/lighting/TV1/living-room-front/setRGB"}

# simulation of burned in names
#
# these could be pulled in from external server via https or ccnx
# but that is in itself a hack / temporary, so for now we put them here & can extend python in future

deviceList = (
'00:1c:42:00:00:08', '169.192.0.13', 'phillips/ColorBlast/',
'00:1c:42:00:00:04', '169.192.0.14', 'phillips/ColorBlaze',
'00:1c:42:00:00:06', '169.192.0.12', 'gumstix/overo/fire'
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