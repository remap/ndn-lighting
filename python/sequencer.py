import sys
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure
import ssl
import database as data
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
		self.play()
		#self.send()

	def play(self):
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
			sendImageSequence(everythingAndMore[max])
		if(current >= max):
			current = 0
		self.play()

	def sendImageSequence(self,analysis):
		#print "analysis : "+str(analysis['analysis'][0]['result'])
		print "analysis : "+str(analysis['filename'][0])
		#writeStatusFile(analysis)
		sequence = analysis['analysis'][0]['result']
		for line in sequence:
			#print line
			#i = 0;
			oldRGB = ""
			newRGB = ""
			flexCommand =""
			for command in line:
				#print str(i) + " : " + str(command)
				#i = i + 1
				if(line[1] != "incandescent"):
					#CLIcommand = replaceNameWithID(line[1])+"/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
					CLIcommand = line[1]+"/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
					newRGB = "%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
					if(newRGB != oldRGB):
						#flexCommand = /ucla.edu/apps/lighting/fixture/iColorFlex/2/*/rgb-8bit-hex/10101
						flexCommand = "iColorFlex/2/*/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
						oldRGB = newRGB
				#else:
					#ARTNET/*/INTENSITY
					#CLIcommand = replaceNameWithID(line[1])+"/*/"+str(line[3])
				#/ucla.edu/cens/nano/lights/1/fixture/1/rgb-8bit-hex/FAFAFA
				#print CLIcommand
				#time.sleep(float(line[0])/5)
				time.sleep(1)
				self.sendInterest(CLIcommand)
				#self.actuallySendInterest(CLIcommand)
				#sendFlexInterest(flexCommand)

	def sendInterest(self,command):
		fullURI = self.cfg.interestPrefix + command
		print fullURI
		i = Interest.Interest()
		#n = Name.Name([fullURI])	#this does not parse correctly
		n = Name.Name(str(fullURI))
		print("Interest sending to "+str(n))
		co = self.handle.get(n,i,20)
		if not not co: 
			#if co is not empty,  print result for debugging
			print("interest "+str(co.content)+" for "+str(co.name))		
			
	# executes interest on remote host (legacy)
	def sendInterestSSH(command):

		#ssh root@host 
	
		fullCLI = cfg.lightHost+" "+cfg.signedInterestCommand+" "+cfg.interestPrefix + command
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