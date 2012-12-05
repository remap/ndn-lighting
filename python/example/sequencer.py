import sys
#from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,_pyccn,NameCrypto
import pyccn
from pyccn import Key as Key
from pyccn import Interest
from pyccn import Name
import ssl
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

class sequencer(pyccn.Closure):

	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFile()
		self.handle = pyccn.CCN()
		self.getApplicationKey()
		#nameCrypto
		#self.state = NameCrypto.new_state()
		#self.cryptoKey = NameCrypto.generate_application_key(self.cfg.fixtureKey, self.cfg.appName)

	def loadConfigFile(self):
		command = "import "+self.appConfigFileName+" as appCfg"
		exec(command)
		self.appCfg = appCfg;
		self.cfg = appCfg;

	def getApplicationKey(self):
		print("getting application key for "+self.appCfg.appName)
		key = Key()
		keyFile = self.appCfg.keyFile
		key.fromPEM(filename=keyFile)
		self.appKey = key
		self.key = key

	def start(self):
		print "starting "+self.cfg.appName
		self.disco()

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
		return

	def buildAndSendInterest(self,device,r,g,b):
		#rx = hex(r)[2:]
		#gx = hex(g)[2:]
		#bx = hex(b)[2:]

		rx = self.hx(r)
		gx = self.hx(g)
		bx = self.hx(b)

		#CLI = self.cfg.interestPrefix+device+"/rgb-8bit-hex/"+rx+gx+bx
		CLI = device+"/rgb-8bit-hex/setRGB/"+rx+gx+bx
		print CLI
		self.sendInterest(CLI)
		#living-room-right-wall/rgb-8bit-hex/d2741d
		#interestPrefix
		#ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/living-room-right-wall/rgb-8bit-hex/d2741d

	def sendInterest(self,command):

		n = Name(self.cfg.appPrefix+command)
		print("\nInterest espressing for "+str(self.cfg.appPrefix))
		print("\ncommand is "+str(command))
		#fullURI = self.cfg.interestPrefix + command
		#print fullURI
		i = Interest()
		#n = Name.Name([fullURI])	#this does not parse correctly
		#n = Name.Name(fullURI)

		print("Interest sending to "+str(n))
		self.handle.get(n,i,20)


	def hx(self, inum):
	    # converts hex to integer
		num = hex(inum)[2:]
		if (len(num) == 1):
			num = '0'+num
		#print num
		return str(num)

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
