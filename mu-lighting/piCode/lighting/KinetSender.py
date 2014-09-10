##

# To do:  Add exception handling 
# And add logging
# Borrow PEIR status, logging, and exception handling mechanisms
##

import random
import sys, time, socket, types 
from ctypes import *
from threading import Thread, Lock, Event



# Logging support
import logging
from BeaconLogging import BeaconLogging

# Sends to one IP
#
class KinetSender(object):

    PACKET_INTERVAL_SECONDS = 0.007
    srcIP = ""
    srcPort = 0  # any available
    destIP = "" 
    destPort = 6038
    fixturePorts = 0      
    fixtureLength = None   
    dataPackets = None
    dataLocks = None
    syncPacket = None
    sock = None
    payloadobjs = []
    log = None
    stop = False
    complete = Event()

    # set srcip="" for all available interfaces
    # pass array as fixtureLength for varying lengths, integer if all the same
    def __init__(self, srcip, destip, fixtureports, fixtureLength, log=None, start=True):
        if log:
            self.log = log
        else:
            self.log = BeaconLogging("Beacon-beta", "KinetSender", "KinetSender.log", logging.WARNING,logging.WARNING, None)       
        self.srcIP = srcip
        self.destIP = destip
        self.fixturePorts = fixtureports
        self.name = "KinetSender_" + self.destIP  # threadname
        self.fixtureLength = []
        if type(fixtureLength)==types.IntType:
            for k in range(0, self.fixturePorts):
                self.fixtureLength.append(fixtureLength)
        self.syncPacket = KinetPktSync()
        self.dataPackets = []
        for k in range(1, self.fixturePorts+1):
            self.dataPackets.append(KinetPktPortOut(k, self.fixtureLength[k-1]))
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.runThread = Thread(target=self.run, name='Payload transmit thread')
        try:
            self.sock.bind((self.srcIP, self.srcPort))
        except Exception, e:
            self.log.log(logging.ERROR, "KinetSender.__init__()", "Exception binding socket", exception=e)
        else:     
            if start:
                self.runThread.start()
    
    def addPayloadObject(self, obj):
        #TODO Check object has payload attribute
        #TODO a delete object!
        self.payloadobjs.append(obj)
        
    def getPayload(self, port):
        if port<1 or port>self.fixturePorts:
            return None
        self.dataPackets[port-1].payloadLock.acquire()
        retval = self.dataPackets[port-1].payload
        self.dataPackets[port-1].payloadLock.release()
        return retval
    
    def setPayload(self, port, payload):
        if port<1 or port>self.fixturePorts:
            return None
        try:
            self.dataPackets[port-1].payloadLock.acquire()
            for k in range(0, self.dataPackets[port-1].length):
                self.dataPackets[port-1].payload[k] = payload[k]
        except Exception, e:
            self.log.log(logging.ERROR, "KinetSender.setPayload()", "Exception", exception=e)
        finally:
            self.dataPackets[port-1].payloadLock.release()
        
    def run(self):
        # Print out what we're doing
        if self.srcIP == "": host="any" 
        else: host=self.srcIP
        self.log.log(logging.INFO, "KinetSender:run()", "Init thread to send from " + host + " to " + self.destIP+":"+str(self.destPort) + " fixturePorts " + str(self.fixturePorts) + "fixtureLength" + str(self.fixtureLength))        
        
        # Begin
        while(not self.stop):
            # check our payload hooks:
            if len(self.payloadobjs)>0:
                for p in self.payloadobjs:
                    for n in xrange(len(p.payload)):
                        #print "get payload", p, n
                        self.setPayload(n + p.portstart, p.payload[n])
            for k in range(0,self.fixturePorts):
                #print "PORT ", k
                #print str(self.dataPackets[k])
                try:
                    self.dataPackets[k].payloadLock.acquire()     
                    self.sock.sendto(self.dataPackets[k], (self.destIP, self.destPort))
                    self.dataPackets[k].touched = False
                except Exception, e:
                    self.log.log(logging.ERROR, "KinetSender.run()", "Exception sending data packet", exception=e)
                finally: 
                    self.dataPackets[k].payloadLock.release()

            try:  
                # Guard the sync packet with some delay
                # This seems to produce the most reliable sync between ports
                # Sending sync immediately after the data packets above 
                # sometimes fails. 
                time.sleep(self.PACKET_INTERVAL_SECONDS * 0.5)
                self.sock.sendto(self.syncPacket, (self.destIP, self.destPort))
                time.sleep(self.PACKET_INTERVAL_SECONDS * 0.5)
            except KeyboardInterrupt:
                sys.exit(1)
            except Exception, e:
                self.log.log(logging.ERROR, "KinetSender.run()", "Exception sending sync packet", exception=e)     
        print "kinetsender thread stopped"                
        self.complete.set()
        
# What's ugly about this is that the length of the various data types
# is platform dependent -- need to clean up and make portable. This works
# on both linux and windows python 2.6 so far
class KinetPktPortOut(LittleEndianStructure):

    MAX_PAYLOAD = 150  # iColor specific!!

    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("port", c_ubyte), 
                ("pad", c_ubyte), 
                ("flags", c_short), 
                ("timer", c_int), 
                ("universe", c_ubyte),
                ("payload", c_ubyte * MAX_PAYLOAD )]    ## Need to do this dynamically based on length??    
    payloadLock = Lock()
    def __init__(self, port, length):
        self.magic = 0x4ADC0104
        self.version = 0x0001
        self.type = 0x0101
        self.seqnum = 0x00000000
        self.pad = 0x00
        self.flags = 0x0000
        self.timer = 0xffffffff
        self.universe = 0x00
        self.port = port # 
        self.length=length
    def __str__(self):
        return "KinetPktPortOut{ port=" + str(self.port) + "; length=" + str(self.length) + "; len(payload)=" + str(len(self.payload))+"payload=" + str([str(D) for D in self.payload]) + " }"
                 
class KinetPktSync(LittleEndianStructure): 
    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("pad", c_ulong)]
    def __init__(self): 
        self.magic = 0x4ADC0104
        self.version = 0x0002
        self.type = 0x0109
        self.seqnum = 0x0000                
    def __str__(self):
        return "KinetPktSyncOut"
    
if __name__ == '__main__':  
    

    log = BeaconLogging("Beacon-beta", "KinetSender", "KinetSenderTest.log", None,logging.INFO, None) 


    kinetsender = KinetSender("192.168.1.1", "192.168.1.50", 1, 150, log) #"192.168.42.2", "192.168.42.10", 12, 150)
    
    N=0 
    patterns = [(0, 0, 1), (0, 1, 0), (1, 0, 0), (1, 0, 1),
		(1, 1, 0), (0, 1, 1), (1, 1, 1)]
    while (True):
        try:
            N+=1
            p = patterns[(N/50)%len(patterns)]
            bob = range(1,151)
            for i in range(0,50):
                bob[i*3:i*3+3]=(N*x for x in p)     
            kinetsender.setPayload(1,bob)    
        except KeyboardInterrupt, k:
            kinetsender.setPayload(1, [0 for i in range(150)])
            kinetsender.stop = True
            kinetsender.complete.wait()         
            sys.exit(1)
            
