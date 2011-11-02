#current histogram mode used by upload & email
analysisMode = 4

#email account info
emailUser = "uclaremap"
emailPass = "l!ghting2011"
# the gmail label that gets scanned for new work
emailLabel = "TFTInstall"

#path to email files (temporary)
emailImagePath = "/var/www/html/lighting/app/emailFiles/"

# path to images
imageFilePath = "/var/www/html/lighting/app/files/"
# web path to images
imageWebPath = "http://borges.metwi.ucla.edu/lighting/app/files/"

# mongodb collection name
colName = "lighting"
dbPort = 27016 #27017 is default

#with trailing slash
interestPrefix = "/ucla.edu/apps/lighting/TV1/fixture/"
# seperate prefix for colorflex (PARC Demo)
flexPrefix = "/ucla.edu/apps/lighting/fixture/"

# NDN node & user with signedLightInterest command
# (sequencer was running as www-data from bigriver)
# SSH was used for parc demo... don't need anymore, but leaving for emergency / fallback
lightHost = "ssh nano@borges.metwi.ucla.edu"

# full path to interest executable on host

	# ssh (path on borges)
signedInterestCommand = "/home/nano/ndn-lighting/ndn/signedInterest/signedNDNClient"


controlHost = "ssh lighting@remap.is-very-good.org"
# ssh command (on TV1 gumstix)
startSequencerCommand = "/home/lighting/ndn-lighting/scripts/webStartLightingTV1"
stopSequencerCommand = "/home/lighting/ndn-lighting/scripts/webStopLightingTV1"
	
# local interest command:
#signedInterestCommand = "/home/lighting/ndn-lighting/ndn/signedInterest/signedNDNClient"

#temporary translation until finalized / moved elsewhere
names = {
"living-room-front":"ColorBlast/1",
"living-room-right":"ColorBlast/2",
"entrance-door":"ColorBlast/3",
"window-right":"ColorBlast/4",
"stairs":"ColorBlaze1/1",
"bedroom":"ColorBlaze2/1",
"kitchen":"ColorBlast/1",
"window-left":"ColorBlast/4",
"incandescent":"ArtNet"}