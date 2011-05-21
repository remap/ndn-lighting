# path to images
imageFilePath = "/var/www/html/lighting/app/files/"

# mongodb collection name
colName = "lighting"



#without trailing slash
interestPrefix = "/ucla.edu/cens/nano/lights"

# NDN node & user with signedLightInterest command
lightHost = "ssh nano@192.168.1.2"

# full path to interest executable on host
signedInterestCommand = "/Users/nano/UCLA/apps/lighting/nano-lighting/signedInterest/signedNDNClient"

#temporary, just for testing on friday
names = {"living-room-front":0,"living-room-right":1,"entrance-door":2,"window-right":3,"stairs":1,"bedroom":2,"kitchen":3,"window-left":0}