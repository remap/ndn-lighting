from pyccn import CCN,Name,Interest,ContentObject,Key

# load config

# issue control interest
# <root><device><capabilty><options>

import database as data
import subprocess
import os
import commands
import sequencer_cfg as cfg
import time
try:
    import json
except ImportError:
    import simplejson as json

current = 0

def play():
	# load all objects from database
	global current
	print "current is "+str(current)
	everything = data.getAllAnalyzed()
	print("there are "+str(everything.count())+" entries")
	# load sequence from database
	max = everything.count()
	sendImageSequence(everything[current])
	current = current+1
	print "image done - check for new image..."
	everythingAndMore = data.getAllAnalyzed()
	if(everythingAndMore.count()>max):
		print " play first image, then return to former loop"
		sendImageSequence(everythingAndMore[max])
	if(current >= max):
		current = 0
	play()

def sendImageSequence(analysis):
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
				CLIcommand = replaceNameWithID(line[1])+"/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
				newRGB = "%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
				if(newRGB != oldRGB):
					#flexCommand = /ucla.edu/apps/lighting/fixture/iColorFlex/2/*/rgb-8bit-hex/10101
					flexCommand = "iColorFlex/2/*/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
					oldRGB = newRGB
			else:
				#ARTNET/*/INTENSITY
				CLIcommand = replaceNameWithID(line[1])+"/*/"+str(line[3])
			#/ucla.edu/cens/nano/lights/1/fixture/1/rgb-8bit-hex/FAFAFA
			print CLIcommand
			#time.sleep(float(line[0])/5)
			time.sleep(0.008)
			#sendInterest(CLIcommand)
			#sendFlexInterest(flexCommand)

def sendInterest(command):

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


if __name__ == "play":
    print("sequencer instantiating...")
    #model()
    
if __name__ == "__main__":
    print("sequencer running")
    play()