import sys
#from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,_pyccn,NameCrypto
import pyccn
from pyccn import Key as Key
from pyccn import NameCrypto
from pyccn import Interest
from pyccn import Name
import ssl
import database as data
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
		self.state = NameCrypto.new_state()
		self.cryptoKey = NameCrypto.generate_application_key(self.cfg.fixtureKey, self.cfg.appName)

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
		self.play()
		#self.send()

	def play(self):
		while 1:
			# load all objects from database
			global current
			print "current is "+str(current)
			everything = data.getAllAnalyzed()
			print("there are "+str(everything.count())+" entries")
			# load sequence from database
			max = everything.count()
			self.sendImageSequence(everything[current])
			current = current+1
			print "image done - check for new image..."
			everythingAndMore = data.getAllAnalyzed()
			if(everythingAndMore.count()>max):
				print " play first image, then return to former loop"
				self.sendImageSequence(everythingAndMore[max])
			if(current >= max):
				current = 0

	def sendImageSequence(self,analysis):
		#print "analysis : "+str(analysis['analysis'][0]['result'])
		print "analysis : "+str(analysis['filename'][0])
		#writeStatusFile(analysis)
		sequence = analysis['analysis'][0]['result']
		for line in sequence:
			for command in line:
				if(line[1] != "incandescent"):
					try:
						CLIcommand = line[1]+"/setRGB/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
						#newRGB = "%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
						#if(newRGB != oldRGB):
						#	#flexCommand = /ucla.edu/apps/lighting/fixture/iColorFlex/2/*/rgb-8bit-hex/10101
						#	flexCommand = "iColorFlex/2/*/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
						#	oldRGB = newRGB
					except:
						print "bad form, skipping"
				else:
					#ARTNET/*/INTENSITY
					CLIcommand = line[1]+"/setBrightness/"+str(line[3])
				#print CLIcommand
				#time.sleep(float(line[0])/5)
				time.sleep(self.cfg.refreshInterval)
				#self.sendInterest(CLIcommand)
				self.sendSignedInterest(CLIcommand)
				#sendFlexInterest(flexCommand)

	def sendInterest(self,command):

		n = Name.Name(self.cfg.appPrefix)
		print("\nInterest espressing for "+str(n))
		n += command
		i = Interest()
		print("Interest sending to "+str(n))

		# for just expressing
		self.handle.expressInterest(authName,self)

		# put & block for full round-trip:
		#co = self.handle.get(n,i,20)
		#if not not co: 
			#if co is not empty,  print result for debugging
		#	print("content: "+str(co.content))

	def sendSignedInterest(self,command):
		fullURI = self.cfg.appPrefix + command
		print fullURI
		i = Interest()

		self.state = NameCrypto.new_state()
		#build keyLocator to append to interest for NameCrypto on upcall
		#
		keyLoc = pyccn.KeyLocator(self.key)
		keyLocStr = pyccn._pyccn.dump_charbuf(keyLoc.ccn_data)
		nameAndKeyLoc = Name(str(fullURI))
		#print("there are "+str(len(nameAndKeyLoc))+" components")
		nameAndKeyLoc = nameAndKeyLoc.append(keyLocStr)
		#print("there are "+str(len(nameAndKeyLoc))+" components after adding keyLocStr")
		authName = NameCrypto.authenticate_command(self.state, nameAndKeyLoc, self.cfg.appName, self.cryptoKey)
		#print authName.components

		# send interest

		# for just expressing
		co = self.handle.expressInterest(authName,self)

		# put & block for full round-trip:
		#co = self.handle.get(authName,i,2000)
		#if not not co: 
			#if co is not empty,  print result for debugging
		#	print("interest "+str(co.content))

		# for profiling - quit when the controller quits (only works w/ put)
		#else:
		#	print "it's done"
		#	sys.exit(1)

	# executes interest on remote host (legacy)
	def sendInterestSSH(command):

		#ssh root@host 

		fullCLI = cfg.lightHost+" "+cfg.signedInterestCommand+" "+cfg.appPrefix + command
		print fullCLI
		#result = commands.getoutput(fullCLI)
		#print result

	def sendFlexInterest(command):

		fullCLI = cfg.lightHost+" "+cfg.signedInterestCommand+" "+cfg.flexPrefix + command
		print fullCLI
		#result = commands.getoutput(fullCLI)
		#print result



	def writeStatusFile(analysis):
		print "writing status..."
		author= str(analysis['author'])
		email= str(analysis['email'])
		title = str(analysis['title'])
		description = str(analysis['text'])
		image = str(analysis['filename'][0])
		htmlSrc = "<img src='"+cfg.imageWebPath+image+"'>"
		#print title+"<br>"+author+"<p>"+description+htmlSrc
		#for python >2.5
		#with open("/var/www/html/lighting/app/current.html","w") as f:
		#	f.write('<meta http-equiv="refresh" content="2"><span style="color:white"><H3>'+title+'</H3><H4>'+author+'</H4><H5>'+description+'</H5></span>'+htmlSrc+'</div>')
		#	f.close()
		f = open("/var/www/html/lighting/app/current.html", 'r+')
		try:
			f.write('<meta http-equiv="refresh" content="2"><span style="color:white"><H3>'+title+'</H3><H4>'+author+'</H4><H5>'+description+'</H5></span>'+htmlSrc+'</div>')
		finally:
			f.close()

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
