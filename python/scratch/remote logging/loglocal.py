import logging, logging.handlers
log = logging.getLogger("Machine 3")
log.setLevel(logging.DEBUG)
socketHandler = logging.handlers.SocketHandler('localhost',
                    50205)
log.addHandler(socketHandler)

wolf = 3048

def doIt():
	log.info("wolf is %s", wolf)
	log.warn("Warning")
	log.error("error")
	log.warn("warn again")
	log.info("whatever")
	print("default  port is %s",logging.handlers.DEFAULT_TCP_LOGGING_PORT)
	
doIt()
