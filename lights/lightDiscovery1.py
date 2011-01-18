import sys, traceback
#from RGBLamp import RGBLamp
#import KinetSender
import time
import socket
from ctypes import *
from threading import Thread, Lock, Event
import logging
import struct

# There are 3 kinds of discovery. (1) Supply discovery; (2) Port discovery; and (3) Fixture discovery

#DATA_ENABLER_IP = '131.179.143.99'
DATA_ENABLER_IP = '172.17.5.10'
DEST_PORT = 6038
SupplyList = {}
_start = False

logging.basicConfig(filename="lightDiscovery.log", filemode='w', level=logging.INFO)

class SupplyDiscovery(Thread):
    def __init__(self, disc_socket, start=True):  # see other parameters need to add as input in __init__
        Thread.__init__(self)
        # logging.basicConfig(filename="SupplyDiscovery.log", filemode='w', level=logging.INFO)
        self._log = logging.getLogger('SupplyDiscovery')
        self._port = DEST_PORT
        self._sock = disc_socket
        self._stop = False
	global _start
	print '*************** init Supply disc: ', _start
        if start:  # threading.Thread.start() -> must be called at most once per thread object
            self.start()

    def killAll(self):
        self._stop = True

    def run(self):
        while not self._stop:
            try:
                self._sock.sendto(KinetPktSplDisc(), ('<broadcast>', self._port))
                self._log.info('run(): Sent Discovery Packet')
            except Exception, e:
                self._log.critical('run(): Raised Exception %s: %s', type(e), e)
                traceback.print_exc()
            finally:
                time.sleep(5)

class SupplyDiscoveryReply(Thread):
    def __init__(self, disc_socket, start=True):  # see other parameters need to add as input in __init__
        Thread.__init__(self)
        self._log = logging.getLogger('WaitForDiscovery')
        self._sock = disc_socket
        self._stop = False
	global _start
	print '**************** init Supply Reply disc: ', _start
        if start:  # threading.Thread.start() -> must be called at most once per thread object
            self.start()
        
    def killAll(self):
        self._stop = True

    def _processReply(self, sList, reply):
        # Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)IP(4)MAC(6)Version(2)Serial(4)Universe(4)DeviceString(Variable)IDString(Variable)
        try:
	    sList.Version = reply[4:6]
	    sList.IP = str(ord(reply[12]))+'.'+str(ord(reply[13]))+'.'+str(ord(reply[14]))+'.'+str(ord(reply[15]))
            sList.MAC = reply[16:22]
            sList.SupplyVersion = reply[22:24]
            sList.SupplySerialNum = reply[24:28]
            sList.Universe = reply[28:32]
            sList.DeviceString = reply[32:]
	    print 'SupplyList[', sList.sid, '].sid = ',             sList.sid
	    print 'SupplyList[', sList.sid, '].Version = ',         sList.Version
	    print 'SupplyList[', sList.sid, '].IP = ',              sList.IP
	    print 'SupplyList[', sList.sid, '].MAC = ',             sList.MAC
	    print 'SupplyList[', sList.sid, '].SupplyVersion = ',   sList.SupplyVersion
	    print 'SupplyList[', sList.sid, '].SupplySerialNum = ', sList.SupplySerialNum
	    print 'SupplyList[', sList.sid, '].Universe = ',        sList.Universe
	    print 'SupplyList[', sList.sid, '].DeviceString = ',    sList.DeviceString
	    global _start
	    print '******************** Supply in processReply: ', _start
	    _start = True
	    print '******************** Supply in processReply 2: ', _start 
        except Exception, e:
            self._log.warning('_processReply(): Error in processing Supply reply')

    def run(self):
        while(not self._stop):
            try:
                reply_m, reply_a = self._sock.recvfrom(1024)
		global SupplyList
		if len(SupplyList) == 0:
		    SupplyList[1] = SupplyInfo()
		    SupplyList[1].sid = 1
		    SupplyList[1].TimeOut = time.time()
		    self._processReply(SupplyList[1], reply_m)
		    global _start
		    _start = True
		    print '***************************** after SupplyReply: ', _start
		else:
		    for i in SupplyList.keys():
			if (reply_m[24:28] == SupplyList[i].SupplySerialNum):
			    SupplyList[i].TimeOut = time.time()
			    self._processReply(SupplyList[i], reply_m) 
			else:
			    temp = SupplyList.keys()[-1]+1
			    SupplyList[temp] = SupplyInfo()  # last keys == SupplyList.keys()[-1]
			    SupplyList[temp].sid = temp
			    SupplyList[temp].TimeOut = time.time()
			    self._processReply(SupplyList.temp, reply_m)
		for i in SupplyList.keys():
		    if time.time() - SupplyList[i].TimeOut >= 120:  # if no reply from a supply for 2 minutes, delete the supplyInfo
			del SupplyList[i]
		#print 'Got reply from ', reply_a
		#print 'current SupplyList length: ', len(SupplyList)
            except socket.timeout, t:
                pass
            except Exception, e:
                self._log.critical('run(): Raised Exception %s: %s', type(e), e)
                traceback.print_exc()
	    finally:
		time.sleep(5)

class PortDiscovery(Thread):
    def __init__(self, start):  # each port from lightdiscovery variable run this Thread
	Thread.__init__(self)
	self._log = logging.getLogger('PortDiscovery')
	self._port = DEST_PORT
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	self._stop = False
	print '************ init Port Start:', start
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    print '============================= Port Discovery is started.'
	    self.start()

    def killAll(self):
	self._stop = True

    def _processReply(self, sList, reply):
	# Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)Ports(1)Pad1(1)Pad2(2)Port(1)Type(1)Pad1(2)Pad2(4)
	try:
	  #for j in SupplyList.keys():
	    #print '*****************************j: ', j
	    sList.PortsNum = ord(reply[12])
	    sList.Port = range(sList.PortsNum)
	    sList.PortType = range(sList.PortsNum)
	    for i in range(sList.PortsNum):
		sList.Port[i] = ord(reply[16+8*i])
		sList.PortType[i] = ord(reply[17+8*i])
	    #print 'Port discovery reply: ', map(ord, reply[:])
	    #print 'Number of Port: ', ord(reply[12])
	    print 'SupplyList[', sList.sid, '].PortsNum = ', sList.PortsNum
	    print 'SupplyList[', sList.sid, '].Port = ',     sList.Port
	    print 'SupplyList[', sList.sid, '].PortType = ', sList.PortType
	    print '******************* Port in processReply: ', _start
	except Exception, e:
	    self._log.warning('_processReply(): Error in processing port reply packet')

    def run(self):
	global _start
	while((not self._stop) & (_start==True)):
	    try:
		print '******************* Port run: ', _start
		for i in SupplyList.keys():
		    self._sock.sendto(KinetPktPortDisc(), (SupplyList[i].IP, self._port)) 
		    self._log.info('run(): Sent PortDiscovery Packet')
		    #time.sleep(1)
		    reply_m, reply_a = self._sock.recvfrom(1024)
		    #print 'Got reply from Port Discovery socket', reply_a
		    self._processReply(SupplyList[i], reply_m)
	    except socket.timeout, t:
		pass
	    except Exception, e:
		self._log.critical('run(): PortDiscovery - Raised Exception %s: %s', type(e), e)
		traceback.print_exc()
	    #finally:
	    #	time.sleep(1)

class FixtureDiscovery(Thread):
    def __init__(self, start):  # see other parameters need to add as input in __init__
        Thread.__init__(self)
        self._log = logging.getLogger('FixtureDiscovery')
        self._port = DEST_PORT
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._stop = False
	print '*************** init Fixture Discovery: ', start
        if start:  # threading.Thread.start() -> must be called at most once per thread object
	    print '============================== Fixture Discovery is started.'
            self.start()

    def killAll(self):
        self._stop = True

    def _processReply(self, sList, reply):
        # Version 1 - Type: reply[6:8] = 0x0202 (non-Chromasic) - BlinkScan Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)Serial(4)
	# Version 1 - Type: reply[6:8] = 0x0206 (CA) - BlinkScan CA Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)Cnt1(1)Cnt2(1)Cnt3(1)Cnt4(1)
	self._version = ord(reply[4])
	try:
	    if self._version == 1:
		try:
		    if ord(reply[6]) == 6:
			sList.Cnt1 = ord(reply[12])  
			sList.Cnt2 = ord(reply[13])
			sList.Cnt3 = ord(reply[14])
			sList.Cnt4 = ord(reply[15])
			print 'SupplyList[', sList.sid, '].Cnt1 = ', sList.Cnt1
			print 'SupplyList[', sList.sid, '].Cnt2 = ', sList.Cnt2
			print 'SupplyList[', sList.sid, '].Cnt3 = ', sList.Cnt3
			print 'SupplyList[', sList.sid, '].Cnt4 = ', sList.Cnt4
			global _start
			print '******************************* in the fixture processReply: ', _start
		    elif ord(reply[7]) == 2:
			sList.FixtureSerial = reply[12:16]
			#print 'Fixture serial number: ', map(ord, reply[12:16])
			print 'SupplyList[', sList.sid, '].FixtureSerial = ', sList.FixtureSerial
		except Exception, e:
			pass
	    elif self._version == 2:
		try:
		    sList.Ents = reply[12]
		    sList.More = reply[13]
		    sList.PortNum = reply[16]
		    sList.EntryType = reply[17]
		    sList.Len = reply[18]
		    print 'SupplyList[', sList.sid, '].Ents = ',      sList.Ents
		    print 'SupplyList[', sLsit.sid, '].More = ',      sList.More
		    print 'SupplyList[', sList.sid, '].PortNum = ',   sList.PortNum
		    print 'SupplyList[', sList.sid, '].EntryType = ', sList.EntryType
		    print 'SupplyList[', sList.sid, '].Len = ',       sList.Len
		except Exception, e:
		    pass
	except Exception, e:
	    self._log.warning('_processReply(): Error in processing fixture discovery reply packet info')

    def run(self):
	global _start
	while((not self._stop) & (_start==True)):
	    try:
		for i in SupplyList.keys():
		    self._sock.sendto(KinetPktBlkScan(), (SupplyList[i].IP, self._port))
		    self._log.info('run(): Sent FixtureDiscovery packet ')
		    #time.sleep(1)
		    reply_m, reply_a = self._sock.recvfrom(1024)
		    #print 'Got reply from Fixture Discovery socket', reply_a
		    self._processReply(SupplyList[i], reply_m)
		    global _start
		    print '************* Just After fixture disc:', _start
		    _start = False
		    print '************* After fixture disc:', _start
	    except socket.timeout, t:
		    pass
	    except Exception, e:
		    self._log.critical('run(): FixtureDiscovery - Raised Exception %s: %s', type(e), e)
		    traceback.print_exc()
	    #finally:
		#    time.sleep(1)

# Supply Discovery packet and Reply packet
class KinetPktSplDisc(LittleEndianStructure):
    _fields_ = [("magic", c_int),     # c_int: 4 bytes; c_short - 2 bytes; c_ubyte - 1 byte; c_long - 8 bytes
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("tempIP", c_int)]  

    def __init__(self):
        self.magic      = 0x4ADC0104
        self.version    = 0x0001
        self.type       = 0x0001
        self.seqnum     = 0
        #self.tempIP     = 0x0a0511ac
        self.tempIP     = 0x0a878889
    
    def __str__(self):
        return "KinetPktSplDisc"

class KinetPktPortDisc(LittleEndianStructure): 
    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),  
                ("pad", c_int)]
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0001
        self.type       = 0x000A
        self.seqnum     = 0x00000000
	self.pad        = 0x00000000                
    def __str__(self):
        return "KinetPktPortDisc"

class KinetPktBlkScan(LittleEndianStructure): 
    _fields_ = [("magic", c_int),     
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("pad", c_int)]    
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0001      # isn't it 0x0001??
        self.type       = 0x0201
        self.seqnum     = 0x00000000
        self.pad        = 0x00000000              
    def __str__(self):
        return "KinetPktBlkScan"

class SupplyInfo:
    def __init__(self):
	self.sid             = 0
	self.Version         = 0
	self.IP              = 00000000
	self.MAC             = 00
	self.SupplyVersion   = 0
	self.SupplySerialNum = 00
	self.Universe        = 0
	self.DeviceString    = 'init'
	self.PortsNum        = 0
	self.Port            = [] 
	self.PortType        = []
	self.FixtureSerial   = 00
	self.Cnt1            = 0
	self.Cnt2            = 0
	self.Cnt3            = 0
	self.Cnt4            = 0
	self.Ents            = 0
	self.More            = 0
	self.PortNum         = 0
	self.EntryType       = 0
	self.Len             = 0
	self.TimeOut         = 0

    def __str__(self):
	return "SupplyInfo"

if __name__ == '__main__':  
    try:
        print '**main**'
        _host = ''  # bind to all interfaces
        _sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        _sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        _sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        _sock.bind((_host, 0))
        _sock.settimeout(2.0)
        A = SupplyDiscovery(_sock)
        B = SupplyDiscoveryReply(_sock)
	time.sleep(5)
	global _start
	C = PortDiscovery(_start)
	D = FixtureDiscovery(_start)
    except Exception, e:
        print 'Exception in program init. Exiting...'
        traceback.print_exc()
        sys.exit(2)
        
    try:
        while (1):
            time.sleep(60)
    except KeyboardInterrupt, k:
        print 'Interrupted by user.'
    except Exception, e:
        print 'Program generated some Exception.'
        traceback.print_exc()
    finally:
        A.killAll()
        B.killAll()
	C.killAll()
	D.killAll()
