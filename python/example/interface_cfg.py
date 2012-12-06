###########################
# NDN Application Configuration
###########################
# this section is needed for configuration manager to authorize the app's namespace

appName = "interface"
appPrefix = "ccnx:/ndn/ucla.edu/apps/lighting/basic/"
appDescription = "lighting interface controller"
keyFile = "interface.pem"


############################
# Application Runtime:
############################
# the following are not NDN specific / not required for CM 
# yet still required by app

#depth (from right) of device name

deviceNameDepth = 3 # change to 5 if using namecrypto

#ccnx:/ndn/ucla.edu/apps/lighting/TV1/pastel-left-b/setRGB/000000
# ideally this should be derived during runtime (once)

# in seconds: -1 is forver
# for main ccnx run, useful for profiling
runtimeDuration = -1