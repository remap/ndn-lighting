import sys
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure
from multiprocessing import Process
import ssl

# configuration manager 
# trusted authority that signs application namespaces for control

class controller(Closure.Closure):
	
	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFile()
		self.handle = CCN.CCN()
		self.sendHandle = CCN.CCN()
		self.getApplicationKey()
		
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
		self.name = Name.Name([self.appCfg.appPrefix])
		self.co = self.makeDefaultContent()
		print "controller init & listening within "+self.appCfg.appPrefix
		print "controller init & listening within "+str(self.name)
		self.listen()

	def listen(self):
		#listen to requests in namespace
		#print "key server thread, listening for "+self.URI
		self.handle.setInterestFilter(Name.Name(self.URI), self)
		self.handle.run(-1)

	def makeDefaultContent(self):
		co = ContentObject.ContentObject()
		# since they want us to use versions and segments append those to our name
		co.name = Name.Name(self.name) # making copy, so any changes to co.name won't change self.name
		co.name.appendVersion() # timestamp which is our version
		co.name += b'\x00' # first segment
		co.content = "OK"
		si = ContentObject.SignedInfo()
		si.publisherPublicKeyDigest = self.key.publicKeyID
		si.type = 0x28463F # key type 
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

		# parse command & send to correct driver/IP

		# return content object 
		# (ideally based on success/fail - yet that's not implicitly supported by current kinet
		# so perhaps we put a self-verification of new driver state process here
		# meanwhile just return 'ok'
		# self.sendHandle.run(-1)
		# self.handle.put(self.co) # send the prepared data
		# self.sendHandle.setRunTimeout(0) # finish run()
		# self.handle.setRunTimeout(0) # finish run()
		return Closure.RESULT_INTEREST_CONSUMED
	

def usage():
	print("Usage: %s <Application configFileName>" % sys.argv[0])
	sys.exit(1)

if __name__ == '__main__':
	if (len(sys.argv) != 2):
		usage()
	
	runtime = controller(sys.argv[1])
	runtime.start()