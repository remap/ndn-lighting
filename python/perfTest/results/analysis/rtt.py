#!/usr/bin/python

import sys
import db as data

#t this extracts RTT from a log file via mongo DB

# assuming fields:
# LogNumhost,status,logTime,CCN_STATUS,VerifyValue,Name 
#fields = [LogNum,host,status,logTime,CCN_STATUS,VerifyValue,Name]

#instead of just csv, we've inserted into mongodb from CSV:
# mongoimport -v -h localhost --port 27017 -d results_asymm -c results -f LogNum,host,status,logTime,CCN_STATUS,VerifyValue,Name --type csv -file asymmetric_new.log 
# mongoimport -v -h localhost --port 27017 -d results_symm -c results -f LogNum,host,status,logTime,CCN_STATUS,VerifyValue,Name --type csv -file symmetric_new.log 
# mongoimport -v -h localhost --port 27017 -d results_unsigned -c results -f LogNum,host,status,logTime,CCN_STATUS,VerifyValue,Name --type csv -file unsigned_new.log 
# note port 27017 is standard

#dbName = sys.argv[1]

#data.setCollection(dbName)

print "analyzing data for "+data.cfg.dbName

everything = data.getAllLogs()
print("there are "+str(everything.count())+" log entries")

# get just borges upcalls
borges = data.getControllerExpressed()
print("there are "+str(borges.count())+" interests expressed from controller")

# see how many were recieved on interface (don't have interface upcall yet)
# results = data.getInterfaceReceived()
# print("there are "+str(results.count())+" upcalls received on interface")

# see how many were verified on interface
results = data.getInterfaceVerified()
print("there are "+str(results.count())+" signed interests verified on interface")

# let's derive final content objects two ways to be sure

borgesCo = data.getControllerReceivedCo()
print("there are "+str(borgesCo.count())+" data objects recieved & verified on controller")

# find RTT
# get all co, find matching interest, subtract times for duration
'''
c = 0
for co in borgesCo:
	#print co['Name']
	c=c+1
	rec = data.getExpressedWithName(co['Name']);
	#print "co: "+str(co['logTime']-rec['logTime'])
	#print co['CCN_STATUS']+":"+str(co['logTime'])
	#print rec[0]['CCN_STATUS']+":"+str(rec[0]['logTime'])
	#print c
	#print co['logTime'] - rec[0]['logTime']
'''

#find RTT another way to be sure

# loop through expressed interests
# find matching content objects
'''
allInterests = data.getControllerExpressed()

found = 0
co = 0
for interest in allInterests:
	try:
		co = data.getReceivedWithName(interest['Name'])
		co = co[0]
		found = found +1
		print found
		print co['logTime'] - interest['logTime']		
	except Exception: 
		exit(0)
		pass
'''		
# ok, they match. the first is slight more elegant, so we'll use that. 

# the results are horrible, but let's plot them anyway. 
# Lets format for analysis / plotting within R.

# just a dump of all RTT values:

'''
c=0
for co in borgesCo:
	#print co['Name']
	c=c+1
	rec = data.getExpressedWithName(co['Name']);
	print co['logTime'] - rec[0]['logTime']
'''


#gstixCO = data.getInterfaceCO()
#print("there are "+str(gstixCO.count())+" content objects put from Interface")

#sub = data.getAllWithSubName("/ndn/ucla.edu/apps/lighting/TV1/living-room-left/setRGB/6f0000/")

#print sub.count()
#for rec in sub:
#	print rec


'''
print "calculating RTT per received CO:"

allOut = data.getControllerExpressed()
allIn = data.getControllerReceivedCO()
# rtt
match = 0
cos = 0
ints = 0
for co in allIn:
	cos = cos+1
	for expr in allOut:
		ints = ints + 1
		if(co['Name'] == expr['Name']):
			match = match +1
			print "match "+str(match)+" : "+co['Name']
	#print str(ints)+" ints checked"
	#print str(cos)+" co checked"
			
print "there are "+str(match)+" matches"
print str(cos)+" content objs checked"

'''

# per borges CO
	#find matching interest
		#subtract interest time from CO time
		#print
		

		
		

##### CSV Graveyard
#ifile  = open(f, "rb")
#reader = csv.reader(ifile)
#data.open(f)
#data.dump()
#ifile.close()
#



