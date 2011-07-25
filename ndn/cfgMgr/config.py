appName = "/lighting"
appURI = "/ucla.edu/apps"



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
# right now just MAC, IP, typeComponent

# may need to expand to include default namespace

	
# also note IP address is for Kinet and is to be auto-detected and written *after* ccnx cfg handshake
# the only thing humans should enter to this file is the serial of the devices
# the serial field could be populated with MAC address for now.
	