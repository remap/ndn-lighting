import config as cfg

def main():
	#print("this is the config for app "+)
	print("returning appName "+getConfigParam("appName"))
	print(getTotalDeviceCount())
	print(getDeviceID(1))

def appName():
	return cfg.__dict__['appName']
	
def appURI():
	return cfg.appURI

def appPath():
    #print "app Path..."
	return cfg.appURI+cfg.appName

def testValue():
    return 4

def getTotalDeviceCount():
	return len((cfg.deviceList))/cfg.numValPerKey

# will get device id from nth of deviceList		
def getDeviceID(n):
	if(n==0): n=1
	id = n*cfg.numValPerKey-cfg.numValPerKey
	#print("returning device idx %i ",id)
	return cfg.deviceList[id]
	
	
# will over-write the IP of the device at key ID
#def setDeviceIP(ID):

def getConfigParam(param):
	return cfg.__dict__[param]

if __name__ == '__main__':
	main()

