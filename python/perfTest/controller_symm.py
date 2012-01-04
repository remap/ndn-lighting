import sys
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,_pyccn,NameCrypto
import ssl
#import database as data
import subprocess
import os
import commands
import time
#from time import time
try:
    import json
except ImportError:
    import simplejson as json

current = 0

class sequencer(Closure.Closure):

	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFile()
		self.handle = CCN.CCN()
		self.getApplicationKey()
		#nameCrypto
		self.state = NameCrypto.new_state()
		self.cryptoKey = NameCrypto.generate_application_key(self.cfg.fixtureKey, self.cfg.appName)
		self.count = 0
		
	def loadConfigFile(self):
		command = "import "+self.appConfigFileName+" as appCfg"
		exec(command)
		self.appCfg = appCfg;
		self.cfg = appCfg;

	def getApplicationKey(self):
		print("getting application key for "+self.appCfg.appName)
		key = Key.Key()
		keyFile = self.appCfg.keyFile
		key.fromPEM(filename=keyFile)
		self.appKey = key
		self.key = key
		
	def start(self):
		print "starting "+self.cfg.appName
		self.startTime = time.time()
		#self.play()
		#self.send()
		self.discoProfile()
		#self.disco()
		#self.mobileDisco()
		#self.discoArt()
		#self.allWhite()
		#self.allBlack()
		#self.spazz()
		
	def upcall(self, kind, info):
			#t0 = time()
			
			#ignore interests
			if kind == Closure.UPCALL_INTEREST:
				return Closure.RESULT_OK
			
			#ignore timeouts
			if kind == Closure.UPCALL_INTEREST_TIMED_OUT:
				return Closure.RESULT_OK
			
			# make sure content has verified
			if kind == Closure.UPCALL_CONTENT_BAD:
				print "content bad"
				return Closure.RESULT_OK
		
			# handle verified content object
			if kind == Closure.UPCALL_CONTENT:
				print "upcall received ", info.ContentObject.content
				return Closure.RESULT_OK
		
			
		
			return Closure.RESULT_INTEREST_CONSUMED
			
		
	def spazz(self):
		for i in range(1,500000):
			self.allWhite()
			self.allBlack()
		
	def allWhite(self):
		for d in self.cfg.names:
			if d['name'] != "incandescent":
				print d['name']
				self.buildAndSendInterest(d['name'],255,255,255)
				
	def allBlack(self):
		for d in self.cfg.names:
			if d['name'] != "incandescent":
				print d['name']
				self.buildAndSendInterest(d['name'],0,0,0)
			self.buildAndSendArtInterest('incandescent',0,)
		
	def disco(self):
		print "fading..."
		for d in self.cfg.names:
			if d['name'] != "incandescent":
				print d['name']
				r = b = g = 0
				#red
				for i in range(0,85):
					r=i
					self.buildAndSendInterest(d['name'],r*3,g*3,b*3)
				for i in range(0,85):
					r=0
					g=i
					self.buildAndSendInterest(d['name'],r*3,g*3,b*3)
				for i in range(0,85):
					g=0
					b=i
					self.buildAndSendInterest(d['name'],r*3,g*3,b*3)
				self.buildAndSendInterest(d['name'],0,0,0)
				self.buildAndSendInterest(d['name'],255,255,255)
				self.buildAndSendInterest(d['name'],0,0,0)
				self.buildAndSendInterest(d['name'],255,255,255)
				self.buildAndSendInterest(d['name'],0,0,0)
				self.buildAndSendInterest(d['name'],0,0,0)
		return
		
		
	def discoProfile(self):
		print "fading..."
		for d in self.cfg.names:
			if d['name'] != "incandescent":
				print d['name']
				r = b = g = 0
				#red
				for i in range(0,85):
					r=i
					self.buildAndSendInterest(d['name'],r*3,g*3,b*3)
				for i in range(0,85):
					r=0
					g=i
					self.buildAndSendInterest(d['name'],r*3,g*3,b*3)
				for i in range(0,85):
					g=0
					b=i
					self.buildAndSendInterest(d['name'],r*3,g*3,b*3)
				self.buildAndSendInterest(d['name'],0,0,0)
				self.buildAndSendInterest(d['name'],255,255,255)
				self.buildAndSendInterest(d['name'],0,0,0)
				self.buildAndSendInterest(d['name'],255,255,255)
				self.buildAndSendInterest(d['name'],0,0,0)
				self.buildAndSendInterest(d['name'],0,0,0)
		print "DONE"
		#self.sendInterest("whatever/profileStop/000000")
		self.sendSignedInterest("whatever/profileStop/000000")
		self.endTime = time.time()
		print "total time is ",(self.endTime - self.startTime)
		print "number of interests is ",self.count
		print "average time per interest is ",((self.endTime - self.startTime)/self.count)
		return
		
	def mobileDisco(self):
		print "fading around"
		for i in range(0,85):
				r = b = g = 0
				#red
				for i in range(0,85):
					r=i
					self.sendValToAllLights(r*3,g*3,b*3)
				for i in range(0,85):
					r=0
					g=i
					self.sendValToAllLights(r*3,g*3,b*3)
				for i in range(0,85):
					g=0
					b=i
					self.sendValToAllLights(r*3,g*3,b*3)	
				self.sendValToAllLights(0,0,0)
				self.sendValToAllLights(255,255,255)
				self.sendValToAllLights(0,0,0)
				self.sendValToAllLights(255,255,255)
				self.sendValToAllLights(0,0,0)
		sys.exit()
		
		
	def sendValToAllLights(self,r,g,b):
		for d in self.cfg.names:
			self.buildAndSendInterest(d['name'],r,g,b)
	
		
	def discoArt(self):
		print "fading..."
		for d in self.cfg.names:
			if d['name'] == "incandescent":
				print d['name']
				r = b = g = 0
				#red
				for i in range(0,250):
					r=i
					self.buildAndSendArtInterest(d['name'],r)
		self.discoArt()
				
	def hx(self, inum):
		num = hex(inum)[2:]
		if (len(num) == 1):
			num = '0'+num
		#print num
		return str(num)

	def buildAndSendInterest(self,device,r,g,b):
		#rx = hex(r)[2:]
		#gx = hex(g)[2:]
		#bx = hex(b)[2:]

		rx = self.hx(r)
		gx = self.hx(g)
		bx = self.hx(b)

		#CLI = self.cfg.interestPrefix+device+"/rgb-8bit-hex/"+rx+gx+bx
		CLI = device+"/setRGB/"+rx+gx+bx
		#print CLI
		#self.sendInterest(CLI)
		self.sendSignedInterest(CLI)
		#living-room-right-wall/rgb-8bit-hex/d2741d
		#interestPrefix
		#ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right-wall/rgb-8bit-hex/d2741d
		
	def buildAndSendArtInterest(self,device,r):
		rx = self.hx(r)
		#gx = hex(g)[2:]
		#bx = hex(b)[2:]

		#rx = self.hx(r)

		#CLI = self.cfg.interestPrefix+device+"/rgb-8bit-hex/"+rx+gx+bx
		CLI = device+"/setBrightness/"+rx
		#print CLI
		self.sendSignedInterest(CLI)
		#living-room-right-wall/rgb-8bit-hex/d2741d
		#interestPrefix
		#ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right-wall/rgb-8bit-hex/d2741d

	def putInterest(self,command):
		
		n = Name.Name(self.cfg.appPrefix)
		print("\nInterest espressing for "+str(n))
		n += command
		#fullURI = self.cfg.interestPrefix + command
		#print fullURI
		i = Interest.Interest()
		#n = Name.Name([fullURI])	#this does not parse correctly
		#n = Name.Name(fullURI)
		
		print("Interest sending to "+str(n))
		co = self.handle.get(n,i,200)
		if not not co: 
			#if co is not empty,  print result for debugging
			print("content: "+str(co.content))
			
	def sendInterest(self,command):
		self.count = self.count +1
		#n = Name.Name(self.cfg.appPrefix)
		#print("\nInterest espressing for "+str(n))
		#n += command
		fullURI = self.cfg.appPrefix + command
		#print fullURI
		i = Interest.Interest()
		#n = Name.Name([fullURI])	#this does not parse correctly
		#n = Name.Name(fullURI)
		
		
		n = Name.Name(fullURI)
		
		print("Interest sending to "+str(n))
		
		
		co = self.handle.expressInterest(n,self)
		#co = self.handle.get(n,i,200)
		#if not not co: 
			#if co is not empty,  print result for debugging
		#	print("content: "+str(co.content))
			
	def sendSignedInterest(self,command):
		self.count = self.count +1
		#time.sleep(self.cfg.refreshInterval)
		fullURI = self.cfg.appPrefix + command
		print fullURI
		i = Interest.Interest()
		#self.state = NameCrypto.new_state()
		#build keyLocator to append to interest for NameCrypto on upcall
		keyLoc = Key.KeyLocator(self.key)
		keyLocStr = _pyccn.dump_charbuf(keyLoc.ccn_data)
		nameAndKeyLoc = Name.Name(str(fullURI))
		#print("there are "+str(len(nameAndKeyLoc))+" components")
		nameAndKeyLoc += keyLocStr
		#print("there are "+str(len(nameAndKeyLoc))+" components after adding keyLocStr")
		
		#symmetric
		authName = NameCrypto.authenticate_command(self.state, nameAndKeyLoc, self.cfg.appName, self.cryptoKey)
		
		#asymmetric
		#authName = NameCrypto.authenticate_command_sig(self.state, nameAndKeyLoc, self.cfg.appName, self.key)
		
		#print authName.components
		self.handle.setInterestFilter(Name.Name(authName), self)
		
		co = self.handle.expressInterest(authName,self)
		
		self.handle.run(0)
		#co = self.handle.get(authName,i,200)
		#if not not co: 
			#if co is not empty,  print result for debugging
		#	print("interest "+str(co.content))
		# for profiling - quit when the controller quits
		#else:
		#	print "it's done"
		#	sys.exit(1)

def usage():
	print("Usage: %s <Application configFileName>" % sys.argv[0])
	sys.exit(1)

if __name__ == '__main__':
	if (len(sys.argv) != 2):
		usage()
	
	runtime = sequencer(sys.argv[1])
	runtime.start()
