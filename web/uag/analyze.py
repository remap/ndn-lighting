import database as data
import subprocess
import os
import commands
import json
import config as cfg

# in future this should be run from seperate thread via cron that only operates on
# uploaded images that have not yet been analyzed
# ie
# db.getAllUnanalyzed()
	

# meanwhile, since it's just histogram:


# perform analysis on media

def histogram(id):
	imageFileName = cfg.imageFilePath+data.getFilenameFrom(id);
	result = commands.getoutput("convert "+imageFileName+" histogram:- | identify -format %c -")
	aName = "histogram"
	data.insertAnalysisWithID(id, aName, json.dumps(result));
	return data.getEntryFromID(id)
	
	#note for pattern generation:
	#json.loads(result) to reconsistute newline (or just explode on \\n)
 

