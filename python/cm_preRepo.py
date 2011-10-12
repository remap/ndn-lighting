import sys
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure
from multiprocessing import Process
import ssl
from RepoControl import RepoUpload 
# configuration manager 
# trusted authority that signs application namespaces for control

class cm():
	
	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFiles()
		self.handle = CCN.CCN()
		#self.name = Name.Name("ccnx:/Foo/key")
		self.getCMKey()
		self.getApplicationKey()
		
	def start(self):
		print "authorizing namespace for "+self.appCfg.appName
		#self.checkKeys()
		#self.listAppNameSpace()
		self.publishSignedAppKeys()
		
	def loadConfigFiles(self):
		command = "import "+self.appConfigFileName+" as appCfg"
		exec(command)
		import cm_cfg as cfg
		self.appCfg = appCfg;
		self.cfg = cfg
		
	def getCMKey(self):
		#print("getting cm key...")
		key = Key.Key()
		keyFile = self.cfg.keyFile
		key.fromPEM(filename=keyFile)
		self.key = key

	def getApplicationKey(self):
		#print("getting application key for "+self.appCfg.appName)
		key = Key.Key()
		keyFile = self.appCfg.keyFile
		key.fromPEM(filename=keyFile)
		self.appKey = key

	def checkKeys(self):
		print("checking keys")
		print(self.key.publicKeyID)
		print(self.appKey.publicKeyID)
		print("public")
		print(self.appKey.publicToPEM())

	def listAppNameSpace(self):
		for name in self.appCfg.controlNameSpace:
			print name			

	def publishSignedAppKeys(self):
		print "publishing Application Keys"

		keyServers = []

		for name in self.appCfg.controlNameSpace:
			
			# make key servers
			# first let's do control namespace
			#<controlURI><key>
			keyServers.append(KeyServer(self.key, self.appKey, "ccnx:"+name+"/key"))
			
			# now let's do trust namespace
			#<appPrefix><appName><authority><controlURI>			
			trustName = "ccnx:"+self.cfg.appPrefix+self.appCfg.appName+"/"+self.cfg.appName+name
			#print "Trust name is:"+ trustName
			keyServers.append(KeyServer(self.key, self.appKey, trustName+"/key"))


		for ks in keyServers:
			# start key servers
			p = Process(target=ks.listen)
			p.start()

		for name in self.appCfg.controlNameSpace:
			# express interests for keys in control namespace
			n = Name.Name("ccnx:"+name+"/key")
			i = Interest.Interest()
			co = self.handle.get(n,i,5000)
			print "Keyserved for ccnx:"+str(co.name)

			trustName = "ccnx:"+self.cfg.appPrefix+self.appCfg.appName+"/"+self.cfg.appName+name
			n = Name.Name(trustName+"/key")
			i = Interest.Interest()
			co = self.handle.get(n,i,5000)
			print "Key published for ccnx:"+str(co.name)


class KeyServer(Closure.Closure):
	def __init__(self, cmkey, appKey, name):
		#Process.__init__()
		self.handle = CCN.CCN()
		self.key = cmkey
		self.content = appKey
		self.keyLocator = Key.KeyLocator(self.key)
		self.URI = name
		self.name = Name.Name(name)
		self.co = self.makeSignedKey()
		print "Keyserver for "+self.URI+" initialized"

	def listen(self):
		#listen to requests in namespace
		#print "key server thread, listening for "+self.URI
		self.handle.setInterestFilter(self.name, self)
		self.handle.run(-1)

	def makeSignedKey(self):
		co = ContentObject.ContentObject()

		# since they want us to use versions and segments append those to our name
		co.name = Name.Name(self.name) # making copy, so any changes to co.name won't change self.name
		co.name.appendVersion() # timestamp which is our version
		co.name += b'\x00' # first segment

		co.content = self.content

		si = ContentObject.SignedInfo()
		si.publisherPublicKeyDigest = self.key.publicKeyID
		si.type = 0x28463F # key type 
		si.finalBlockID = b'\x00' # no more segments available
		si.keyLocator = self.keyLocator
		co.signedInfo = si

		co.sign(self.key)

		return co

	def upcall(self, kind, info):
		if kind != Closure.UPCALL_INTEREST:
			return Closure.RESULT_OK
		#print "sending signed key at "+str(info.Interest.name)
		self.handle.put(self.co) # send the prepared data
		self.handle.setRunTimeout(0) # finish run()

		return Closure.RESULT_INTEREST_CONSUMED
	

def usage():
	print("Usage: %s <Application configFileName>" % sys.argv[0])
	sys.exit(1)

if __name__ == '__main__':
	if (len(sys.argv) != 2):
		usage()
	
	cfgMgr = cm(sys.argv[1])
	cfgMgr.start()