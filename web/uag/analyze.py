import database as data
import subprocess
import os
import commands
import json
#import simplejson #json - if python 2.6 or greater
import config as cfg

import sys
sys.path.append("/srv/www/htdocs/lighting/app/process")

import process

def histogram(id, mode):
	imageFileName = cfg.imageFilePath+data.getFilenameFrom(id);
	aName = "histogram_"+str(mode)
	data.insertAnalysisWithID(id, aName, process.imgprocess(imageFileName, mode))
	return data.getEntryFromID(id)
	
	
	'''	
	imageFileName = cfg.imageFilePath+data.getFilenameFrom(id);
	result = commands.getoutput("convert "+imageFileName+" histogram:- | identify -format %c -")
	aName = "histogram"
	data.insertAnalysisWithID(id, aName, json.dumps(result));
	return data.getEntryFromID(id)
	'''
