##

# To do:  Add exception handling 
# And add logging
# Borrow PEIR status, logging, and exception handling mechanisms
##

import random
import traceback
import sys, time, socket, types 
from ctypes import *
from threading import Thread, Lock, Event

# Logging support
import logging

# Sends to one IP
#
class KinetSender(Thread):

    PACKET_INTERVAL_SECONDS = 0.025
    srcIP = ""
    srcPort = 0  # any available
    destIP = "" 
    destPort = 6038
    fixturePorts = 0      
    fixtureLength = None   
    dataPackets = None
    syncPacket = None
    sock = None
    log = None
    stop = False
    complete = Event()

    # set srcip="" for all available interfaces
    # pass array as fixtureLength for varying lengths, integer if all the same
    def __init__(self, srcip, destip, fixtureports, fixtureLength, log, start=True):
        Thread.__init__(self)
        self.log = log
        self.srcIP = srcip
        self.destIP = destip
        self.fixturePorts = fixtureports
        self.fixtureLength = []
        if type(fixtureLength)==types.IntType:
            for k in range(0, self.fixturePorts):
                self.fixtureLength.append(fixtureLength)
        else:
            self.fixtureLength = fixtureLength
        self.syncPacket = KinetPktSync()
        self.dataPackets = []
        for k in range(1, self.fixturePorts+1):
            self.dataPackets.append(KinetPktPortOut(k))
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if start:
            self.start()
    
    def getPayload(self, port):
        if port<1 or port>self.fixturePorts:
            return None
        self.dataPackets[port-1].payloadLock.acquire()
        retval = self.dataPackets[port-1]
        self.dataPackets[port-1].payloadLock.release()
        return retval
    
    def setPayload(self, port, payload):
        if port<1 or port>self.fixturePorts:
            return None
        self.dataPackets[port-1].payloadLock.acquire()
        self.dataPackets[port-1].touched = True
        for k in range(0, self.fixtureLength[port-1]):
            try:
                self.dataPackets[port-1].payload[k] = payload[k]
            except Exception, e:
                self.log.error('setPayload(): Exception - %s', str(e))
        self.dataPackets[port-1].payloadLock.release()
        
    def run(self):
        if self.srcIP == "": host="any" 
        else: host=self.srcIP
        self.log.info('run(): Init thread to send from ' + host + ' to ' + self.destIP + ':' + str(self.destPort) + ' fixturePorts ' + str(self.fixturePorts) + ' fixtureLength ' + str(self.fixtureLength))        
        
        # Begin
        while(not self.stop):
            send_sync = False
            for k in range(0,self.fixturePorts):
                try:
                    #self.log.debug('run(): Trying to acquire lock')
                    self.dataPackets[k].payloadLock.acquire()     
                    #self.log.debug('run(): Acquired lock')
                    self.sock.sendto(self.dataPackets[k], (self.destIP, self.destPort))
                    #self.log.debug('run(): Sent port packet')
                    if self.dataPackets[k].touched:
                        self.log.error(str(self.dataPackets[k].payload[0])+':'+str(time.time()))
                        self.dataPackets[k].touched = False
                    #send_sync = True
                except Exception, e:
                    self.log.error('run(): Exception sending data packet - %s', str(e))
                finally: 
                    self.dataPackets[k].payloadLock.release()

            try:  
            	# Guard the sync packet with some delay
            	# This seems to produce the most reliable sync between ports
            	# Sending sync immediately after the data packets above 
            	# sometimes fails.
                time.sleep(self.PACKET_INTERVAL_SECONDS * 0.5)
                #if send_sync:
                self.sock.sendto(self.syncPacket, (self.destIP, self.destPort))
                #self.log.debug('run(): Sent sync packet')
                time.sleep(self.PACKET_INTERVAL_SECONDS * 0.5)
            except Exception, e:
                self.log.error('run(): Exception sending sync packet - %s', str(e))
        self.complete.set()
		
# What's ugly about this is that the length of the various data types
# is platform dependent -- need to clean up and make portable. This works
# on both linux and windows python 2.6 so far
class KinetPktPortOut(LittleEndianStructure):

    MAX_PAYLOAD = 512   # ColorBlast PowerCore specific!!
    START_CODE = 0x0FFF # ColorBlast PowerCore: Non-ChromASIC light; iFlex: ChromASIC light

    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("universe", c_int),
                ("port", c_ubyte), 
                ("pad", c_ubyte), 
                ("flags", c_short), 
                ("length", c_short), 
                ("startcode", c_short),
                ("payload", c_ubyte * MAX_PAYLOAD )]
    payloadLock = Lock()
    def __init__(self, port):
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x0108
        self.seqnum     = 0x0000 # is it 0x00000000?? 4 bytes
        self.universe   = 0xFFFFFFFF
        self.length     = self.MAX_PAYLOAD
        self.startcode  = self.START_CODE
        self.port       = port # 
        self.touched    = False
    def __str__(self):
        return "KinetPktPortOut{ port=" + str(self.port) + "; len(payload)=" + str(len(self.payload))+"; payload=" + str([str(D) for D in self.payload]) + "; }"
                 
class KinetPktSync(LittleEndianStructure): 
    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("pad", c_ulong)]
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x0109
        self.seqnum     = 0x0000  # is it 0x00000000?? 4 bytes
    def __str__(self):
        return "KinetPktSyncOut"
    
def finish(kinetsender):
    kinetsender.stop = True
    if kinetsender.isAlive():
        kinetsender.log.info('finish(): Waiting for thread to finish')
        kinetsender.complete.wait()			
    kinetsender.log.info('finish(): Thread stopped')
    sys.exit(1)

if __name__ == '__main__':  
    
    logging.basicConfig(filename="KinetSender.log", filemode='w', level=logging.CRITICAL)
    kinetsender = KinetSender("127.0.0.1", "131.179.141.51", 1, 6, logging.getLogger('KinetSender'))
    
    N=-1
    while (N<255):
    	try:
			N+=1
			bob = range(6)
			for i in range(6):
				bob[i]=N        
			for i in range(1,2):
				kinetsender.setPayload(i, bob)    
			print str(kinetsender.getPayload(1))
			time.sleep(2)
        except KeyboardInterrupt, k:
            print 'Interrupted by user.'
            finish(kinetsender)
        except Exception, e:
            print 'Program generated some Exception.'
            traceback.print_exc()
            finish(kinetsender)
    finish(kinetsender)
