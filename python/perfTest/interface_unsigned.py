import sys
#from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,pyccn._pyccn,NameCrypto
import ssl
import time

import pyccn
from pyccn import Key as Key
from pyccn import NameCrypto
from pyccn import Interest
from pyccn import Name
from pyccn import ContentObject
from pyccn import Closure

# UDP client
import socket

# profiler
import cProfile

#logging
import logging, logging.handlers

# controller
# fixture application that receives interests & controls lights

class controller(pyccn.Closure):
	
	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFile()
		self.initLog()
		self.handle = pyccn.CCN()
		self.getApplicationKey()
		self.iFlex_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		
		# we really need one of these per sender (pubkey)
		self.state = NameCrypto.new_state()
		self.cryptoKey = NameCrypto.generate_application_key(self.cfg.fixtureKey, self.cfg.appName)
		#self.symmKey = Key.Key().generateRSA(512)
		#self.symmKey = self.symmKey.generateRSA()
		
		# list of sender public keys
		self.senders = []
		# list of sender states 
		self.states = []
		self.send = False
		self.count = 0
		self.avgTime = 0
		self.startTime =0 
		self.endTime =0

	def __del__(self):
		self.iflex_socket.close()
		
	def loadConfigFile(self):
		command = "import "+self.appConfigFileName+" as appCfg"
		exec(command)
		self.appCfg = appCfg;
		self.cfg = appCfg;
		
	def initLog(self):
		# logging
		self.log = logging.getLogger(self.cfg.appName)
		self.log.setLevel(logging.DEBUG)
		socketHandler = logging.handlers.SocketHandler(self.cfg.logIP,self.cfg.logPort)
		self.log.addHandler(socketHandler)

	def getApplicationKey(self):
		#print("getting application key for "+self.appCfg.appName)
		key = Key()
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
		self.keyLocator = pyccn.KeyLocator(self.key)
		self.URI = self.appCfg.appPrefix
		self.name = Name(self.appCfg.appPrefix)
		#print "controller init & listening within "+self.appCfg.appPrefix
		print "controller init & listening within "+str(self.name)
		self.listen()

	def listen(self):
		#listen to requests in namespace
		print "listening for "+self.URI
		self.co = self.makeDefaultContent(self.URI, "default Content")
		self.handle.setInterestFilter(Name(self.URI), self)
		self.startTime = time.time()
		self.handle.run(self.cfg.runtimeDuration)

	def makeDefaultContent(self, name, content):
		co = ContentObject()
		# since they want us to use versions and segments append those to our name
		co.name = Name(name) # making copy, so any changes to co.name won't change self.name
		co.name.appendVersion() # timestamp which is our version
		co.name.append(b'\x00') # first segment
		co.content = content #"LIGHT OK"
		si = pyccn.SignedInfo()
		si.publisherPublicKeyDigest = self.key.publicKeyID
		si.type = 0x28463F # key type
		#si.type = 0x0C04C0 # content type
		si.finalBlockID = b'\x00' # no more segments available
		si.keyLocator = self.keyLocator
		co.signedInfo = si
		co.sign(self.key)
		#co.sign(self.cryptoKey)
		#co.sign(self.symmKey)
		return co

	def upcall(self, kind, info):
		tR = time.time()

		#print "received interest "+str(info.Interest.name)

		#print self.appCfg.appName +" upcall..."
		if kind != pyccn.UPCALL_INTEREST:
			return pyccn.RESULT_OK

		#ignore timeouts
		if kind == pyccn.UPCALL_INTEREST_TIMED_OUT:
			return pyccn.RESULT_OK

	
		#print info.Interest.name.components
		#print "interest has "+str(len(info.Interest.name))+" components"
		self.state = NameCrypto.new_state()
	
		# verify interest
		n = info.Interest.name
		#keyLocStr2 = n[-2]
		
		#print "\n ncrypt: "+ keyLocStr2 + "\n"
		#try:
		#capsule = _pyccn.new_charbuf('KeyLocator_ccn_data', keyLocStr2)
		#keyLoc2 = _pyccn.KeyLocator_obj_from_ccn(capsule)
		
		### if we know the key
		### use the state for it
		
		### if it's a new key
		### and we trust it
		### make a state for it
		### and use that state
		
		#meanwhile, until this is implemented... restart controller if use with a new sender/app
		
		self.send = False
		
		#symmetric
		#result = NameCrypto.verify_command(self.state, n, self.cfg.window, fixture_key=self.cfg.fixtureKey)
		
		#asymmetric
		#result = NameCrypto.verify_command(self.state, n, self.cfg.window, pub_key=keyLoc2.key)
		
		#content = result
		#if(result == True):
			#print "Verify "+str(result)
		#	self.send = True
		#else:
			# we don't care about duplicates right now
			#if (result != -4):
				#content = "Verify False : "+str(result)
			#	print "Verify False : "+ str(result)+" for "+str(info.Interest.name)

		# if verified, send interest
		#if self.send:
			# parse command & send to correct driver/IP
			# must ignore right now, still get too many false / 'outside window'
		#self.parseAndSendToLight(info.Interest.name)
		self.parseAndSendToLight_Unsigned(info.Interest.name)


		# return content object 
		# (ideally based on success/fail - yet that's not implicitly supported by current kinet
		# so perhaps we put a self-verification of new driver state process here
		# meanwhile just return 'ok'
		content = "ok"
		tS = time.time()
		if (content):
		#if (self.send):
		#if(content):
			self.handle.put(self.makeDefaultContent(info.Interest.name, content)) # send the prepared data
			self.log.info(str(tS-self.startTime)+",CONTENT_PUT,"+str(content)+","+str(info.Interest.name))
		#print("published content object at "+str(info.Interest.name)+"\n")
		
		t1 = time.time()
		self.count = self.count+1
		self.avgTime = ((t1-tR) + (self.avgTime / self.count))
		#print str(self.avgTime) +" at "+str(self.count)
		# self.handle.setRunTimeout(-1) # finish run()
		return pyccn.RESULT_INTEREST_CONSUMED

	def parseAndSendToLight(self, name):
		#print "length of interest name is "+ str(len(name))
		iMax = len(name-1)
		#print "length of prefix name is "+ str(len(Name.Name([self.appCfg.appPrefix])))
		#ideally, derive algorithmically...
		# meanwhile we're using config params to get first ver working
		#print "length of interest name is "+ str(name[(iMax-self.appCfg.deviceNameDepth)])
		
		rgbVal = str(name[iMax-(self.appCfg.deviceNameDepth-2)])
		command = str(name[iMax-(self.appCfg.deviceNameDepth-1)])
		dName = str(name[iMax-self.appCfg.deviceNameDepth])
		DMXPort = self.getDMXFromName(dName)
		
		#print " device "+dName+" ID : "+DMXPort+" color "+rgbVal +" plus command " + command
		
		UDPPort = self.getUDPFromName(dName)
		#UDPPort = 99999
		
		#devNum = "0"

		if(command=="setRGB"):

			r = str(int(rgbVal[0:2],16))
			g = str(int(rgbVal[2:4],16))
			b = str(int(rgbVal[4:6],16))
			
			#print " r is "+r+" g is "+g+" b is "+b
			newData = self.cfg.numLights+"|"+DMXPort+"|"+r+"|"+g+"|"+b
			
			#print "like to put data "+newData+" to port "+ str(UDPPort)
	    	
		    # NUM LIGHTS | ID1 | R | G | B | ID2 | R | ...
	    	# send_data = "4|1|250|086|100";
			#data = "4|1|250|086|100"
			self.sendData(newData,UDPPort)
			
		elif (command=="setBrightness"):
		
			r = str(int(rgbVal[0:2],16))
			newData = "*|"+r
			#print "artNet Data: "+ newData
			self.sendData(newData,UDPPort)
		
		#for profile testing	
		elif (command=="profileStop"):
			self.endTime = time()
			print "avg upcall time ",self.avgTime
			print "total interests ", self.count
			print "total time ",(self.endTime - self.startTime)
			print "avg time per int ",((self.endTime - self.startTime)/self.count)
			self.handle.setRunTimeout(0) # finish run()
			return
					
		else:
		
			print"command unknown: !!! "+command

# UNSIGNED VERSION - PERFORMANCE BASELINE
	
	def parseAndSendToLight_Unsigned(self, name):
		#print "length of interest name is "+ str(len(name))
		#print "received interest "+str(name)
		iMax = len(name)-1
		#print name.components
		#print "length of prefix name is "+ str(len(Name([self.appCfg.appPrefix])))
		#ideally, derive algorithmically...
		# meanwhile we're using config params to get first ver working
		#print "length of interest name is "+ str(name[(iMax-self.appCfg.deviceNameDepth)])
		
		#self.appCfg.deviceNameDepth = self.appCfg.deviceNameDepth+2;
		
		rgbVal = str(name[iMax])
		command = str(name[iMax-1])
		dName = str(name[iMax-2])
		DMXPort = self.getDMXFromName(dName)
		
		#print " device "+dName+" ID : "+DMXPort+" color "+rgbVal +" plus command " + command
		
		UDPPort = self.getUDPFromName(dName)
		#UDPPort = 99999
		
		#devNum = "0"

		if(command=="setRGB"):

			r = str(int(rgbVal[0:2],16))
			g = str(int(rgbVal[2:4],16))
			b = str(int(rgbVal[4:6],16))
			
			#print " r is "+r+" g is "+g+" b is "+b
			newData = self.cfg.numLights+"|"+DMXPort+"|"+r+"|"+g+"|"+b
			
			#print "like to put data "+newData+" to port "+ str(UDPPort)
	    	
		    # NUM LIGHTS | ID1 | R | G | B | ID2 | R | ...
	    	# send_data = "4|1|250|086|100";
			#data = "4|1|250|086|100"
			self.sendData(newData,UDPPort)
			
		elif (command=="setBrightness"):
		
			r = str(int(rgbVal[0:2],16))
			newData = "*|"+r
			#print "artNet Data: "+ newData
			self.sendData(newData,UDPPort)
		
		#for profile testing	
		elif ((command=="profileStop") | (str(name[iMax-3])=="profileStop")):
			self.endTime = time.time()
			print "avg upcall time ",self.avgTime
			print "total interests ", self.count
			print "total time ",(self.endTime - self.startTime)
			print "avg time per int ",((self.endTime - self.startTime)/self.count)
			self.log.info(str(self.endTime )+",End Time, , ")
			self.log.info(str((self.endTime - self.startTime) )+",Total Time, , ")
			self.log.info(str(((self.endTime - self.startTime)/self.count) )+",AVG TIME PER INT, , ")
			self.log.info(str(self.endTime )+",Num Bad Packets,"+str(self.badPacket)+", ")
			self.log.info(str(self.endTime )+",Num Good Packets,"+str(self.goodPacket)+", ")
			self.handle.setRunTimeout(0) # finish run()
			return
					
		else:
		
			print"command unknown: "+command
		
		
	def sendData(self, data, port):
		self.iFlex_socket.sendto(data, ("localhost", int(port)))
		

	def getDMXFromName(self,devName):
		dmx = 1
		for n in self.cfg.names:
			#print "DMX comparing "+devName+" to "+n['name']
			if(n['name']==devName):
				dmx = n['DMX']
				return str(dmx)
		#print "error - no DMX port matches names"
		return str(dmx)

	def getUDPFromName(self,devName):
		udp = -1
		for n in self.cfg.names:
			#print "UDP comparing "+devName+" to "+n['name']
			if(n['name']==devName):
				udp = n['UDP']
				return str(udp)
		p#rint "error - no UDP port matches name "+devName
		return str(1000)

def usage():
	print("Usage: %s <Application configFileName>" % sys.argv[0])
	sys.exit(1)

if __name__ == '__main__':
	if (len(sys.argv) != 2):
		usage()
	
	runtime = controller(sys.argv[1])
	runtime.start()