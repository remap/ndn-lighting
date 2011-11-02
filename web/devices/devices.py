import subprocess
import os
import commands
from datetime import datetime
import time

outFile = "/var/www/html/lighting/devices/index.html"


# receives data from a gumstix system that has booted & found IP address
def postData(systemOutput):
	now = datetime.now()
	header = "<hr>"+now.strftime("%A, %d. %B %Y %I:%M%p %Ss")+"<p>new system online:<br>"
	body = "<pre>"+systemOutput+"</pro>"
	footer = "<p/>"
	
	
	# damn old python 2.4 on borges!
	# re-writing following so it works if less than2.5...
	#write new system at top of file
	#with open(outFile,"r+") as f:
	#	old = f.read()
	#	f.seek(0)
	#	f.write(header+body+footer+old)
	#	f.close()
	
	f = open(outFile, 'r+')
	try:
		old = f.read()
		f.seek(0)
		f.write(header+body+footer+old)
	finally:
		f.close()
	
	#open(outFile, 'ab').write(header+body+footer)
	return (systemOutput)