UAG: Upload Analyze Generate
lighting web app
v1.5 11/04/2011

Features:


1) to allow crowdsourced images to drive the colors of lights at an installation:
		upload image, email image, resize, analyze, pattern generation
2) to allow always-on web control of sequencer:
		turn on / turn off sequencer on borges
3) to allow expression of interests from web client
4) to start/stop listening of TV1 faderboard script
5) to allow individual light control (via colorpicker)


dependencies:
	mongodb
	modpy
	apache
	pymongo
        pil and numpy for the img analysis
	pyCCN

Overview:

5 main components (parameters of which are controlled in config.py)


control:
	http://borges.metwi.ucla.edu/lighting/app/control.html (password removed - ask maintainer if needed)
	to upload a single image
	to clear a pattern
	to stop & start the sequencer on borges
	to stap & start the fader
	to turn off all lights
	to view the TV1 camera
	to control individual light color (via color picker)
	

upload:
	http://borges.metwi.ucla.edu/lighting/app/upload
	to upload, rename, save, and create metadata object in database
	right now it's a web form.

eMail:
	the same functionality as upload, but via a Gmail (or any imap) account.
	if the service ever times out due to checking too often (right now it's at 5min on crontab)
	visit https://www.google.com/accounts/UnlockCaptcha
	also note there is some undocumented gmail IMAP data rate limit, seems to be around 50/MB pull an hr. 
	upload requires manual moderation of images - tag them 'tftinstall' and they'll end up in the sequencer.
	currently email account is uclaremap@gmail.com
	(password removed - ask maintainer if needed)

analyze:
	to find unanalyzed records, analyze them, and store result in db object
	this has been extended with the 'process' directory - analyze.py is now an interface to methods in process dir
	and combines analysis with generation.

(pattern) generator:
	to find analysis output, and generate command sequence for lights
	this is the 'subjective' part - how to drive specific colors and timings from the analysis.
	the files in 'process' dire have merged analysis and pattern generation. 

sequence:
	to 'play' a playlist (of sequences) to the lighting installation.
	
	two ways - use 'upload' function of control or upload above. 
	if one does not 'clear database' it will just add the image to the DB (playing newly uploaded analysis next in the queue)
	if cleared/just one in the DB, will just loop that one image analysis.

	start/stop sequencer using 'control', above.


how to clear patterns:

		using 'control.html' above

Notes:

The application/data analysis object is an array of objects, each differentiated by a name that is written at time of analysis. This allows multiple analysis to be run on each image.

also note the config.py imported as cfg - please place any constants in here.


VERSION HISTORY

v1   5/21/2011

had upload/analyze/generate/email/sequencer in advance of PARC demo

v1.5 11/04/2011

sequence now uses python version
added web control UI to start & stop sequencer & faders
added web control to turn off lights

v2 6-9/2012

added color picker control
improved web control