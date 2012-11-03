import re
import sys
import pymongo
from pymongo import Connection
from pymongo.errors import CollectionInvalid
import datetime
import time

import sequencer_cfg as cfg

connection = Connection(cfg.dbHost, cfg.dbPort)
#db = connection.test
#collection = db.test

db = connection[cfg.colName]
collection = db[cfg.colName]

def index(req):
  	sys.stderr = sys.stdout
  	req.content_type = "text/plain"
  	req.write("Hello World!\n")
  	anotherMethod(req)
  	#writeDatabase(req)
  	
def anotherMethod(req):
	req.write("another method...\n")

def writeDatabase(name, email, title, comment):
    #connect to mongoDB
	#connection = Connection()
	#db = connection.test
	#collection = db.test

	data = post={"author":name, "title":title, "text":comment, "email":email, "tags":["UCLA","TFT","festival", "test"],"date":datetime.datetime.utcnow()}

	lastID = collection.insert(data)

	return lastID


def getEntryFromID(id):
	return(collection.find_one({"_id":id}))


def updateFilenameFromID(id,filename):

	collection.update({"_id":id }, { "$push": { "filename":filename} } );

	return getEntryFromID(id)


def insertAnalysisWithID(id,aName,analysisResult):

	collection.update({"_id":id }, { "$push": { "analysis":{"analysisName":aName, "result":analysisResult}}} );

	return getEntryFromID(id)


def getFilenameFrom(id):
	entry = getEntryFromID(id)	
	return str(entry['filename'][0])

###### unused methods follow

def getAllUnanalyzed():
	# get all entries that have no 'analysis' field

	#return(collection.find( { analysis : { $exists : false } } ));
	# or something like that 

	return


def getAllAnalyzed():
	#get all entries that have been analyzed
	return collection.find( { "analysis" : { "$exists" : "true"} } );


def getLastEntry():
	# function currently unused, leaving for Reference

	# no simple equivalent to mysql's LAST_ID... none of these work yet ;)
	# going a different way for now... (passing lastID around)
	#cursor = collection.find_one()

	#orderedEntries = collection.find({}).sort({'date':1})
	collection.create_index("date",pymongo.ASCENDING)
	lastEntry = collection.find_one();
	#lastEntry = collection.find({}).sort({"date":1}).limit(1)

	#db.<mycollection>.find(fields = {"_id"}).sort("_id", -1).limit(X)


	#for x in cursor:
	#	req.write(x['author']+"\n")

	#last = collection.find_one()
	#req.write("last object written was:\n\n")
	#for x in last:
	#	req.write(x+": "+str(last[x])+"\n")

	#req.write("found anything?")
	return lastEntry
