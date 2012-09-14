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
imageWebPath = "http://bigriver.remap.ucla.edu/lighting/app/files/"

# mongodb collection name
colName = "lighting"

#mongodb IP address
mongoHost = "bigriver.remap.ucla.edu"

#with trailing slash
interestPrefix = "/ucla.edu/apps/lighting/TV1/fixture/"
# seperate prefix for colorflex (PARC Demo)
flexPrefix = "/ucla.edu/apps/lighting/fixture/"

# NDN node & user with signedLightInterest command
# (sequencer was running as www-data from bigriver)
lightHost = "ssh nano@borges.metwi.ucla.edu"

# full path to interest executable on host
signedInterestCommand = "/home/nano/ndn-lighting/ndn/signedInterest/signedNDNClient"

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