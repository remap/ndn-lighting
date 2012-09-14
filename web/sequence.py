import database as data
import subprocess
import os
import commands
try:
    import json
except ImportError:
    import simplejson as json
import config as cfg
import time

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
	writeStatusFile(analysis)
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
				# CCNDC between Borges & Big River... so just launch local command
				newRGB = "%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
				if(newRGB != oldRGB):
					# idea was to send flex command only when the color changed in the TV1 pattern
					# it worked as well as the flex driver, but not sending flex commands now... 
					#flexCommand = /ucla.edu/apps/lighting/fixture/iColorFlex/2/*/rgb-8bit-hex/10101
					#flexCommand = "iColorFlex/2/*/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
					oldRGB = newRGB
			else:
				#ARTNET/*/INTENSITY
				CLIcommand = replaceNameWithID(line[1])+"/*/"+str(line[3])
			#/ucla.edu/cens/nano/lights/1/fixture/1/rgb-8bit-hex/FAFAFA
			#print CLIcommand
			#time.sleep(float(line[0]))
			time.sleep(0.0)
			sendInterest(CLIcommand)
			#sendFlexInterest(flexCommand)

def sendInterest(command):

	#ssh root@host 
	# SSH was used for parc demo... don't need anymore, but leaving for emergency
	fullCLI = cfg.lightHost+" "+cfg.signedInterestCommand+" "+cfg.interestPrefix + command
	# now we assume CCNDC between bigriver & borges, thus just run locally
	#fullCLI = cfg.signedInterestCommand+" "+cfg.interestPrefix + command
	#print fullCLI
	result = commands.getoutput(fullCLI)
	print result
	
def sendFlexInterest(command):

	fullCLI = cfg.lightHost+" "+cfg.signedInterestCommand+" "+cfg.flexPrefix + command
	#print fullCLI
	#result = commands.getoutput(fullCLI)
	#print result


#def writeStatusFilePy26(analysis):
#	print "writing status..."
#	author= str(analysis['author'])
#	email= str(analysis['email'])
#	title = str(analysis['title'])
#	description = str(analysis['text'])
#	image = str(analysis['filename'][0])
#	htmlSrc = "<img src='"+cfg.imageWebPath+image+"'>"
	#print title+"<br>"+author+"<p>"+description+htmlSrc
	#with open("/var/www/html/lighting/app/current.html","w") as f:
	#	f.write('<meta http-equiv="refresh" content="2"><span style="color:white"><H3>'+title+'</H3><H4>'+author+'</H4><H5>'+description+'</H5></span>'+htmlSrc+'</div>')
	#	f.close()
		
# the above method is better, but migrated environments from 2.6 to 2.4 so had to re-write:
def writeStatusFile(analysis):
	print "writing status..."
	author= str(analysis['author'])
	email= str(analysis['email'])
	title = str(analysis['title'])
	description = str(analysis['text'])
	image = str(analysis['filename'][0])
	htmlSrc = "<img src='"+cfg.imageWebPath+image+"'>"
	#print title+"<br>"+author+"<p>"+description+htmlSrc
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