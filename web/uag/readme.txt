UAG: Upload Analyze Generate
lighting web app
http://bigriver.remap.ucla.edu/lighting/app/upload

purpose:

to allow crowdsourced images to drive the colors of lights at an installation.

4 main components:

upload:
	to upload, rename, save, and create metadata object in database
	right now it's a web form, may best (also?) be triggered by .fwd from custom email address
	or other mobile-friendly method

analyze:
	 to find unanalyzed records, analyze them, and store result in db object

(pattern) generator:
	 to find analysis output, and generate command sequence for lights
	this is the 'subjective' part - how to drive specific colors and timings from the analysis.

sequencer:
	 to 'play' a playlist (of sequences) to the lighting installation.
	sequencer is likely a command line NDN application, and not a web app.

as of this release, upload & one analysis (histogram) is complete.

notes:

The application/data analysis object is an array of objects, each differentiated by a name that is written at time of analysis. This allows multiple analysis to be run on each image.

histogram currently runs at time of upload as it's our default analysis; though further analysis should happen separate from upload thread, on it's own cron.

any data handling should be put in database class so all objects can re-use methods. 

also note the config.py imported as cfg - please place any constants in here.

also, we have a db admin tool at 
http://bigriver.remap.ucla.edu/lighting/db/
user: admin
pwd: l!ghting2011