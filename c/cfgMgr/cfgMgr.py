import config as cfg

def main():
    print("this is the config for app "+getConfigParam("appName"))
    print(getTotalDeviceCount())
    print(getDeviceID(1))
    print(getDeviceIP(1))
    print(getDeviceType(1))
    enumerateControlNamespace()

def appName():
	return cfg.__dict__['appName']
	
def appPrefix():
	return cfg.appPrefix

def appPath():
    #print "app Path..."
	return cfg.appPrefix+cfg.appName

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

# will get device id from nth of deviceList		
def getDeviceIP(n):
	if(n==0): n=1
	id = n*cfg.numValPerKey-cfg.numValPerKey+1;
	#print("returning device idx %i ",id)
	return cfg.deviceList[id]

# will get device TypeComponent from nth of deviceList		
def getDeviceType(n):
    if(n==0): n=1
    id = n*cfg.numValPerKey-cfg.numValPerKey+2;
    #print("returning device idx %i ",id)
    return cfg.deviceList[id]

	
# will over-write the IP of the device at key ID
#def setDeviceIP(ID):

# for auto-generation of namespace from capabilities and devices
def enumerateControlNamespace():
    for capability in cfg.capabilities:
        for deviceName in cfg.appDeviceNames:
            print(cfg.appPrefix+deviceName+"/"+capability)

def getConfigParam(param):
	return cfg.__dict__[param]

if __name__ == '__main__':
	main()

