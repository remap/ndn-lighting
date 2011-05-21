import database as data
import subprocess
import os
import commands
import json
import config as cfg
import time


def play():
	# load all objects from database
	everything = data.getAllAnalyzed()
	# load sequence from database
	for analysis in everything:
		#print "analysis : "+str(analysis['analysis'][0]['result'])
		sequence = analysis['analysis'][0]['result']
		for line in sequence:
			#print line
			#i = 0;
			for command in line:
				#print str(i) + " : " + str(command)
				#i = i + 1
				CLIcommand = cfg.interestPrefix+"/"+replaceNameWithID(line[1])+"/rgb-8bit-hex/"+"%0.2x"%line[2][0]+"%0.2x"%line[2][1]+"%0.2x"% line[2][2]
				#/ucla.edu/cens/nano/lights/1/fixture/1/rgb-8bit-hex/FAFAFA
				#print CLIcommand
				time.sleep(1)
				sendInterest(CLIcommand)
				
	
		#execute lighting command on host
		#ssh lighthost signedInterest name/command/
		
def sendInterest(command):

	#ssh root@host 
	
	fullCLI = cfg.lightHost+" "+cfg.signedInterestCommand+" "+cfg.interestPrefix + command
	#print fullCLI
	result = commands.getoutput(fullCLI)
	print result



def replaceNameWithID(name):
	return(str(cfg.names[name]))


if __name__ == "play":
    print("sequencer instantiating...")
    #model()
    
if __name__ == "__main__":
    print("sequencer running")
    play()