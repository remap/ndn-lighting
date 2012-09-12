import sys
sys.path.append("/srv/www/htdocs/lighting/app")

import os
# this wasn't necessary on original install (bigriver)
# yet /etc/httpd/conf.d/python.conf directive is not responding
# so placing here manually
os.environ['PYTHON_EGG_CACHE'] = '/tmp'

import database as data
import analyze
import config as cfg
import image

# if not auth
	# return apache.HTTP_FORBIDDEN

# accept image upload

	# save temp image

	# make new media record
	
	#rename & save image, UID suffix


def postData(name, email, comment):
	#req.write("posting data: "+name+""+email+""+comment+" \n")
	#req.write("posting data...")
	lastID = data.writeDatabase(name, email, comment)
	#return ( name, email, comment)
	#last = data.getLastEntry()
	
	last = data.getEntryFromID(lastID)
	
	
	tempStr = "\n"
	for x in last:
		tempStr+=(x+": "+str(last[x])+"\n")
	return "Data Posted... "+tempStr

def postAndUpload(req, name, email, comment):
	try: tmpfile = req.form['imageFile']
	except:
		return "no file"

def upload(req):
	fileitem = req.form['file']
	if fileitem.filename:
		# strip leading path from file name to avoid directory traversal attacks
		fname = os.path.basename(fileitem.filename)
		# build absolute path to files directory
		dir_path = os.path.join(os.path.dirname(req.filename), 'files')
		open(os.path.join(dir_path, fname), 'wb').write(fileitem.file.read())
		message = 'The file "%s" was uploaded successfully' % fname
	else:
		message = 'No file was uploaded'
	
	return " file uploaded "

def uploadNoInput(req):
	data.clearDatabase()
	lastID = data.writeDatabase("auto", "uclaremap@gmail.com", "auto", "auto")
	last = data.getEntryFromID(lastID)
	obj = ""
	fileitem = req.form['file']

	if fileitem.filename:
		# strip leading path from file name to avoid directory traversal attacks
		# also prepend DB ID so filenames are unique
		fname = str(last['_id'])+"_"+os.path.basename(fileitem.filename)
		obj = data.updateFilenameFromID(last['_id'],fname)
		# build absolute path to files directory
		dir_path = os.path.join(os.path.dirname(req.filename), 'files')
		open(os.path.join(dir_path, fname), 'wb').write(fileitem.file.read())
		#obj = analyze.histogram(last['_id'])
		obj = image.resize(last['_id'])
		obj = analyze.histogram(last['_id'], 4)
		message = "<HTML>"
		message += 'The file "%s" was uploaded, resized, and analyzed successfully' % fname
		#message += 'go <a href="#" onClick = "history.back()"> back </a>
		message += '\n<img src="'+cfg.imageWebPath+fname+'">'
		f = open("/srv/www/htdocs/lighting/app/control.html",'r')
		message += f.read()
		
		#message += '<br>debug of DB object: <br>'
		#for x in obj:
		#	message +=(x+": "+str(obj[x])+"<br>")
		#message += "<p>"
		#message += "</HTML>"
	else:
		message = 'No file was uploaded'
		
	return message
	
def uploadAndInput(req, name, email, title, comment):
	data.clearDatabase()
	lastID = data.writeDatabase(name, email, title, comment)
	last = data.getEntryFromID(lastID)
	obj = ""
	fileitem = req.form['file']

	if fileitem.filename:
		# strip leading path from file name to avoid directory traversal attacks
		# also prepend DB ID so filenames are unique
		fname = str(last['_id'])+"_"+os.path.basename(fileitem.filename)
		obj = data.updateFilenameFromID(last['_id'],fname)
		# build absolute path to files directory
		dir_path = os.path.join(os.path.dirname(req.filename), 'files')
		open(os.path.join(dir_path, fname), 'wb').write(fileitem.file.read())
		#obj = analyze.histogram(last['_id'])
		obj = image.resize(last['_id'])
		obj = analyze.histogram(last['_id'], 4)
		message = "<HTML>"
		message += 'The file "%s" was uploaded, resized, and analyzed successfully' % fname
		message += '\n<img src="'+cfg.imageWebPath+fname+'">'
		message += '<br>debug of DB object: <br>'
		for x in obj:
			message +=(x+": "+str(obj[x])+"<br>")
		message += "<p>"
		message += "</HTML>"
	else:
		message = 'No file was uploaded'

	
	return message
