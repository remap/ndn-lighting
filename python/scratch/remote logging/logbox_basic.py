import logging
FORMAT = '%(asctime)s %(levelname)s: %(name)s: %(module)s: %(funcName)s: %(lineno)d: %(message)s'
FORMAT_DATE='%Y/%m/%d %a %I:%M:%S %p'
#logging.basicConfig(filename='myapp.log',format=FORMAT,datefmt=FORMAT_DATE,level=logging.DEBUG)
logging.basicConfig(format=FORMAT,datefmt=FORMAT_DATE)
logging.basicConfig()
log = logging.getLogger("Machine 3")
log.setLevel(logging.DEBUG)
wolf = 3048

def doIt():
	log.info("wolf is %s", wolf)
	log.warn("Warning")
	log.error("error")
	log.warn("warn again")
	log.info("whatever")
	
doIt()