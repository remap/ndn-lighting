import cmConfig as cfg

thing1 = "whatever1"
thing2 = "whatever2"

def main():
	#print("this is the config for app "+)
	print("returning appName "+getConfigParam("appName"));

def appName():
	return cfg.__dict__['appName']
	
def appURI():
	return cfg.appURI

def appPath():
        print "app Path..."
	return cfg.appURI+cfg.appName

def testValue():
        return 4

def multiply():
    c = 12345*6789
    print 'within python: the result of 12345 x 6789 :', c
    return c

def getConfigParam(param):
	return cfg.__dict__[param]


if __name__ == '__main__':
	main()

