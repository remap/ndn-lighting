import sys
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,_pyccn,NameCrypto
import ssl

# UDP client
import socket

# profiler
import cProfile

# controller
# fixture application that receives interests & controls lights

class controller(Closure.Closure):
	
	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFile()
		self.handle = CCN.CCN()
		self.getApplicationKey()
		self.iFlex_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		
		self.state = NameCrypto.new_state()

	def __del__(self):
		self.iflex_socket.close()
		
	def loadConfigFile(self):
		command = "import "+self.appConfigFileName+" as appCfg"
		exec(command)
		self.appCfg = appCfg;
		self.cfg = appCfg;

	def getApplicationKey(self):
		#print("getting application key for "+self.appCfg.appName)
		key = Key.Key()
		keyFile = self.appCfg.keyFile
		key.fromPEM(filename=keyFile)
		self.appKey = key

	def listAppNameSpace(self):
		for name in self.appCfg.controlNameSpace:
			print name			
		
	def start(self):
		print "starting "+self.appCfg.appName
		#self.checkKeys()
		#self.listAppNameSpace()
		self.key = self.appKey
		self.keyLocator = Key.KeyLocator(self.key)
		self.URI = self.appCfg.appPrefix
		self.name = Name.Name(self.appCfg.appPrefix)
		#print "controller init & listening within "+self.appCfg.appPrefix
		print "controller init & listening within "+str(self.name)
		self.listen()

	def listen(self):
		#listen to requests in namespace
		print "key server thread, listening for "+self.URI
		self.co = self.makeDefaultContent(self.URI, "default Content")
		self.handle.setInterestFilter(Name.Name(self.URI), self)
		self.handle.run(60000)

	def makeDefaultContent(self, name, content):
		co = ContentObject.ContentObject()
		# since they want us to use versions and segments append those to our name
		co.name = Name.Name(name) # making copy, so any changes to co.name won't change self.name
		co.name.appendVersion() # timestamp which is our version
		co.name += b'\x00' # first segment
		co.content = content #"LIGHT OK"
		si = ContentObject.SignedInfo()
		si.publisherPublicKeyDigest = self.key.publicKeyID
		si.type = 0x28463F # key type
		#si.type = 0x0C04C0 # content type
		si.finalBlockID = b'\x00' # no more segments available
		si.keyLocator = self.keyLocator
		co.signedInfo = si
		co.sign(self.key)
		return co

	def upcall(self, kind, info):
		#print self.appCfg.appName +" upcall..."
		if kind != Closure.UPCALL_INTEREST:
			return Closure.RESULT_OK
	
		print "received interest "+str(info.Interest.name)
		#print info.Interest.name.components
		#print "interest has "+str(len(info.Interest.name))+" components"
			
		# verify interest
		n = info.Interest.name
		keyLocStr2 = n[-2]
		try:
			capsule = _pyccn.new_charbuf('KeyLocator_ccn_data', keyLocStr2)
			keyLoc2 = _pyccn.KeyLocator_obj_from_ccn(capsule)
			result = NameCrypto.verify_command(self.state, n, 10000, fixture_key=self.cfg.fixtureKey, pub_key=keyLoc2.key)
			content = result
			print result
		except:
			content = "Not an authenticated interest"
			print "Not an authenticated interest"
	
		# parse command & send to correct driver/IP
		#self.parseAndSendToKinet(info.Interest.name)

		# return content object 
		# (ideally based on success/fail - yet that's not implicitly supported by current kinet
		# so perhaps we put a self-verification of new driver state process here
		# meanwhile just return 'ok'
		self.handle.put(self.makeDefaultContent(info.Interest.name, content)) # send the prepared data
		print("published content object at "+str(info.Interest.name)+"\n")
		
		# self.handle.setRunTimeout(-1) # finish run()
		return Closure.RESULT_INTEREST_CONSUMED

	def parseAndSendToKinet(self, name):
		print "length of interest name is "+ str(len(name))
		iMax = len(name)
		#print "length of prefix name is "+ str(len(Name.Name([self.appCfg.appPrefix])))
		#ideally, derive algorithmically...
		# meanwhile we're using config params to get first ver working
		print "length of interest name is "+ str(name[(iMax-self.appCfg.deviceNameDepth)])
		
		rgbVal = str(name[iMax-self.appCfg.deviceNameDepth+3])
		command = str(name[iMax-self.appCfg.deviceNameDepth+2])
		devNum = str(name[iMax-self.appCfg.deviceNameDepth+1])
		devName = str(name[iMax-self.appCfg.deviceNameDepth])
		
		print " device "+devName+" ID : "+devNum+" color "+rgbVal +" plus command " + command

		#devNum = "0"
		r = str(int(rgbVal[0:2],16))
		g = str(int(rgbVal[2:4],16))
		b = str(int(rgbVal[4:6],16))
		
		print " r is "+r+" g is "+g+" b is "+b
		newData = self.cfg.numLights+"|"+devNum+"|"+r+"|"+g+"|"+b
		print "like to put data "+newData+" to port "+ str(self.getPortFromDevName(devName))
		myPort = self.getPortFromDevName(devName)
		self.sendData(newData,myPort)
    	
	    # NUM LIGHTS | ID1 | R | G | B | ID2 | R | ...
    	# send_data = "4|1|250|086|100";
		#data = "4|1|250|086|100"
		
	def sendData(self, data, port):
		self.iFlex_socket.sendto(data, ("localhost", port))

	def getPortFromDevName(self,devName):
		if not devName in self.cfg.ports:
			print "error, no port"
			exit
		return self.cfg.ports[devName]

def usage():
	print("Usage: %s <Application configFileName>" % sys.argv[0])
	sys.exit(1)

if __name__ == '__main__':
	if (len(sys.argv) != 2):
		usage()
	
	runtime = controller(sys.argv[1])
	runtime.start()