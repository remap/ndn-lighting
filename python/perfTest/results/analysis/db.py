import re
import sys
import pymongo
from pymongo import Connection
from pymongo.errors import CollectionInvalid
import datetime
import time
import re

import config as cfg

connection = Connection('localhost', cfg.dbPort)
#db = connection.test
#collection = db.test

db = connection[cfg.dbName]
collection = db[cfg.colName]

#collection = db[cfg.colName]

# assuming fields:
# LogNumhost,status,logTime,CCN_STATUS,VerifyValue,Name 
#fields = [LogNum,host,status,logTime,CCN_STATUS,VerifyValue,Name]

# basic DB selects for performance analysis:

def getAllLogs():
	#get all entries that have been analyzed
	return collection.find()

def getAllController():
	#get all entries that have been analyzed
	return collection.find( { "host" : "borges controller"} )

def getControllerExpressed():
	return collection.find( { "CCN_STATUS" : "INTEREST_EXPRESSED"})

def getExpressedWithName(name):
	return collection.find( { "CCN_STATUS" : "INTEREST_EXPRESSED", "Name" :name})
	
def getReceivedWithName(name):
	return collection.find( { "CCN_STATUS" : "UPCALL_CONTENT_OK", "Name" :name})

def getControllerReceivedCo():
	return collection.find( { "CCN_STATUS" : "UPCALL_CONTENT_OK"})

def getInterfaceCO():
	return collection.find( { "CCN_STATUS" : "CONTENT_PUT"})

##

def getInterfaceReceived():
	#upcalls received on interface"
	subName = "gumstix"
	# note I don't have 'UPCALL_CONTENT_OK' from interface in first batch of data
	return collection.find( { "CCN_STATUS" : "UPCALL_CONTENT_OK", "host" :re.compile("^"+subName)})
	
def getInterfaceVerified():
	#signed interests verified on interface
	subName = "gumstix"
	return collection.find( { "CCN_STATUS" : "NC_VERIFY", "VerifyValue" : "True", "host" :re.compile("^"+subName)})



def getAllWithSubName(subName):
	# /ndn/ucla.edu/apps/lighting/TV1/living-room-left/setRGB/6f0000/
	#return collection.find( { name : /acme.*corp/i } );
	return collection.find( { "Name" : re.compile("^"+subName) } );