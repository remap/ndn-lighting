UAG: Upload Analyze Generate
lighting web app

to allow crowdsourced images to drive the colors of lights at an installation.

Features:

	upload image, email image, resize, analyze, pattern generation

dependencies:
	mongodb
	modpy
	apache
	pymongo
        pil and numpy for the img analysis

Overview:

5 main components (parameters of which are controlled in config.py)

upload:
	to upload, rename, save, and create metadata object in database
	right now it's a web form.

eMail:
	the same functionality as upload, but via a Gmail (or any imap) account.
	if the service ever times out due to checking too often (right now it's at 5min on crontab)
	visit https://www.google.com/accounts/UnlockCaptcha
	also note there is some undocumented gmail IMAP data rate limit, seems to be around 50/MB pull an hr. 
	upload requires manual moderation of images - tag them 'tftinstall' and they'll end up in the sequencer.
	currently email account is uclaremap@gmail.com
	u: uclaremap
	p: l!ghting2011

analyze:
	to find unanalyzed records, analyze them, and store result in db object
	this has been extended with the 'process' directory - analyze.py is now an interface to methods in process dir
	and combines analysis with generation

(pattern) generator:
	to find analysis output, and generate command sequence for lights
	this is the 'subjective' part - how to drive specific colors and timings from the analysis.
	the files in 'process' dire have merged analysis and pattern generation. 

sequence:
	to 'play' a playlist (of sequences) to the lighting installation.
	sequencer is a command line NDN application, and not a web app.
	it is currently run manually by typing 'python sequencer.py' from web app dire
	(use screen so it can run in bg when logged out)
	current state can be seen at http://bigriver.remap.ucla.edu/lighting/status.html


how to clear patterns:

	delete all documents in database
	
	use db admin tool at http://bigriver.remap.ucla.edu/lighting/db/
	u/p: admin/l!ghting2011

Notes:

The application/data analysis object is an array of objects, each differentiated by a name that is written at time of analysis. This allows multiple analysis to be run on each image.

also note the config.py imported as cfg - please place any constants in here.

