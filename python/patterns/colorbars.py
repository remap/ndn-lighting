import sys
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,_pyccn,NameCrypto
import ssl
#import database as data
import subprocess
import os
import commands

import time
try:
    import json
except ImportError:
    import simplejson as json

current = 0

class sequencer():

	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFile()
		self.handle = CCN.CCN()
		self.getApplicationKey()
		#nameCrypto
		self.state = NameCrypto.new_state()
		self.cryptoKey = NameCrypto.generate_application_key(self.cfg.fixtureKey, self.cfg.appName)
		
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
		self.disco()
		
	def disco(self):
		print "fading..."
		for name in self.cfg.names:
			if name != "incandescent":
				print name
				r = b = g = 0
				#red
				for i in range(0,256):
					r=i
					self.buildAndSendInterest(name,r,g,b)
				for i in range(0,256):
					r=0
					g=i
					self.buildAndSendInterest(name,r,g,b)
				for i in range(0,256):
					g=0
					b=i
					self.buildAndSendInterest(name,r,g,b)
				self.buildAndSendInterest(name,0,0,0)
				self.buildAndSendInterest(name,255,255,255)
			
				
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
		CLI = device+"/rgb-8bit-hex/"+rx+gx+bx
		print CLI
		self.sendSignedInterest(CLI)
		#living-room-right-wall/rgb-8bit-hex/d2741d
		#interestPrefix
		#ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right-wall/rgb-8bit-hex/d2741d

	def sendInterest(self,command):
		
		n = Name.Name(self.cfg.interestPrefix)
		print("\nInterest espressing for "+str(n))
		n += command
		#fullURI = self.cfg.interestPrefix + command
		#print fullURI
		i = Interest.Interest()
		#n = Name.Name([fullURI])	#this does not parse correctly
		#n = Name.Name(fullURI)
		
		print("Interest sending to "+str(n))
		co = self.handle.get(n,i,20)
		if not not co: 
			#if co is not empty,  print result for debugging
			print("content: "+str(co.content))
			
	def sendSignedInterest(self,command):
		fullURI = self.cfg.interestPrefix + command
		print fullURI
		i = Interest.Interest()
		
		#build keyLocator to append to interest for NameCrypto on upcall
		keyLoc = Key.KeyLocator(self.key)
		keyLocStr = _pyccn.dump_charbuf(keyLoc.ccn_data)
		nameAndKeyLoc = Name.Name(str(fullURI))
		#print("there are "+str(len(nameAndKeyLoc))+" components")
		nameAndKeyLoc += keyLocStr
		#print("there are "+str(len(nameAndKeyLoc))+" components after adding keyLocStr")
		authName = NameCrypto.authenticate_command(self.state, nameAndKeyLoc, self.cfg.appName, self.cryptoKey)
		#print authName.components
		
		co = self.handle.get(authName,i,20)
		if not not co: 
			#if co is not empty,  print result for debugging
			print("interest "+str(co.content))
		# for profiling - quit when the controller quits
		#else:
		#	print "it's done"
		#	sys.exit(1)

	def replaceNameWithID(name):
		if name in cfg.names:
			return(str(cfg.names[name]))
		else:
			return("ColorBlast/2")


def usage():
	print("Usage: %s <Application configFileName>" % sys.argv[0])
	sys.exit(1)

if __name__ == '__main__':
	if (len(sys.argv) != 2):
		usage()
	
	runtime = sequencer(sys.argv[1])
	runtime.start()