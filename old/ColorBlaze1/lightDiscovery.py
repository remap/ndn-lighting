import sys, traceback
from RGBLamp import RGBLamp
import KinetSender
import time
import socket
import types
from ctypes import *
from threading import Thread, Lock, Event
import logging

# There are 3 kinds of discovery. (1) Supply discovery; (2) Port discovery; and (3) Fixture discovery

DEFAULT_PORT = 50009  
#DATA_ENABLER_IP = '131.179.143.99'
DATA_ENABLER_IP = '172.17.5.10'
#lightdisc =   # init lightdisc. keep the parsed data into a global variable. dictionary? or a two-layer array?
checklock = False
replymsg = {'supply':{'ip':0, 'mac':0, 'version':0, 'serial':0, 'universe':0, 'deviceString':0, 'idString':0}, 'port':{'ports':0, 'port':0, 'type':4}, 'fixture':{'serial':0, 'cnt1':0, 'cnt2':0, 'cnt3':0, 'cnt4':0}}

logging.basicConfig(filename="lightDiscovery.log", filemode='w', level=logging.INFO)

class SupplyDiscovery(Thread):
    def __init__(self, tport=DEFAULT_PORT, start=True):  # see other parameters need to add as input in __init__
	Thread.__init__(self)
#	logging.basicConfig(filename="SupplyDiscovery.log", filemode='w', level=logging.INFO)
	self._log = logging.getLogger('lightDiscovery')
	self._port = tport
	# need to implement packet sending configuration or sth. here?? socket options set to BROADCAST
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	self._stop = False
	self._complete = Event()
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    self.start()

    def killAll(self):
	self._stop = True
	if self.isAlive():
	    self._log.info('killAll(): Supply Discovery - Waiting for thread to finish')
	    self._complete.wait()
	self._log.info('killAll(): Supply Discovery - Thread stopped')
	# ?? (Ref: ControlSocket, do we need this killAll()?? )

    def run(self):
	try:
	    self._log.info('run(): Supply Discovery - Listening for connections..')
            self._sock.listen(1)
	    self._sock.settimeout(2.0)
	except Exception, e:
            self._log.critical('run(): Supply Discovery - Raised Exception %s: %s', type(e), e)
            traceback.print_exc()
#	when to kill the Thread??
#        finally:
#            self._log.info('run(): Supply Discovery - Cleaning up thread')
#            self._sock.close()
#            self._complete.set()

class SupplyDiscoveryReply(Thread):
    def __init__(self, tport=DEFAULT_PORT, start=True):  # see other parameters need to add as input in __init__
	Thread.__init__(self)
	self._log = logging.getLogger('lightDiscovery')
	self._port = tport
	# need to implement packet sending configuration or sth. here?? socket options set to BROADCAST
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._stop = False
	self._complete = Event()
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    self.start()

    def killAll(self):
	self._stop = True
	if self.isAlive():
	    self._log.info('killAll():  SupplyDiscoveryReply - Waiting for thread to finish')
	    self._complete.wait()
	self._log.info('killAll(): SupplyDiscoveryReply - Thread stopped')
	# ?? (Ref: ControlSocket, do we need this killAll()?? )

    def _processReply(self, reply):
	# Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)IP(4)MAC(6)Version(2)Serial(4)Universe(4)DeviceString(Variable)IDString(Variable)
	self._replydata = reply.strip()
	print self._replydata
	try:
	    replymsg['supply']['ip'] = self._replydata[12:16]
	    replymsg['supply']['mac'] = self._replydata[16:22]
	    replymsg['supply']['version'] = self._replydata[22:24]
	    replymsg['supply']['serial'] = self._replydata[24:28]
	    replymsg['supply']['universe'] = self._replydata[28:32]
#	    replymsg['supply']['deviceString'] = 
#	    replymsg['supply']['idString'] = 
	except Exception, e:
	    self._log.warning('_processReply(): Error in processing supply reply packet info')
	else:
	    if checklock == True:
		print "Another thread is in process... Please wait."
		time.sleep(0.5)  # wait for 0.5s?
	    else:
		lock.acquire()
		checklock = True
#	    lightdisc =   # store supply info to lightdisc. (global??) and implement the lock().
	finally:
	    lock.released()
	    checklock = False

    def run(self):
	try:
	    while(not self._stop):
	        try:
		    conn, addr = self._sock.accept()
	        except socket.timeout, t:
		    self._log.debug('run(): Supply Discovery Reply - Timeout in accepting connection')
	        else:
		    self._log.info('run(): Supply Discovery Reply - Accepted connection from ' + str(addr))
		    conn.settimeout(1.0)
		    while (not self._stop):         # why still need to check self._stop??
			try:
			    data = conn.recv(1024)  # try
			except socket.timeout, t:   # if there is exception
			    pass
			else:                       # if there is no exception 
			    self._processReply(data)
#			    conn.send(data)         # where to send?? what to send??
		    self._log.info('run(): Supply Discovery Reply - Closing server side connection from ' + str(addr))
		    conn.close()
#			    do I need to implement client check here? anything to send? to C.M. the enumerate info?

	except Exception, e:
	    self._log.critical('run(): Supply Discovery Reply - Raised Exception %s: %s', type(e), e)
	    traceback.print_exc()
	finally:
	    self._log.info('run(): Supply Discovery Reply - Cleaning up thread')
	    self._sock.close()
	    self._complete.set()  # Set the internal flag to true, all threads. All threads waiting for it to become true are awakened  

class PortDiscovery(Thread):
    def __init__(self, tport=DEFAULT_PORT, start=True):  # each port from lightdiscovery variable run this Thread
	Thread.__init__(self)
	self._log = logging.getLogger('lightDiscovery')
	self._port = tport
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._sock.connect(replymsg['supply']['ip'])  # self._sock.connect((HOST, PORT)), Assign HOST and PORT to here!!
	self._stop = False
	self._complete = Event()
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    self.start()

    def killAll(self):
	self._stop = True
	if self.isAlive():
	    self._log.info('killAll(): PortDiscovery - Waiting for thread to finish')
	    self._complete.wait()
	self._log.info('killAll(): PortDiscovery - Thread stopped')
	# ?? (Ref: ControlSocket, do we need this killAll()?? )

    def run(self):
	try:
	    self._log.info('run(): PortDiscovery - Listening for connections..')
            self._sock.listen(1)
	    self._sock.settimeout(2.0)
	except Exception, e:
            self._log.critical('run(): PortDiscovery - Raised Exception %s: %s', type(e), e)
            traceback.print_exc()
#        finally:
#            self._log.info('run(): PortDiscovery - Cleaning up thread')
#            self._sock.close()
#            self._complete.set()  

class PortDiscoveryReply(Thread):
    def __init__(self, tport=DEFAULT_PORT, start=True):  # see other parameters need to add as input in __init__
	Thread.__init__(self)
	self._log = logging.getLogger('lightDiscovery')
	self._port = tport
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._stop = False
	self._complete = Event()
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    self.start()

    def killAll(self):
	self._stop = True
	if self.isAlive():
	    self._log.info('killAll(): PortDiscoveryReply - Waiting for thread to finish')
	    self._complete.wait()
	self._log.info('killAll(): PortDiscoveryReply - Thread stopped')
	# ?? (Ref: ControlSocket, do we need this killAll()?? )

    def _processReply(self, reply):
	# Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)Ports(1)Pad1(1)Pad2(2)Port(1)Type(1)Pad1(2)Pad2(4)
	self._replydata = reply.strip()
	print self._replydata
	try:
	    replymsg['port']['ports'] = self._replydata[12]
	    replymsg['port']['port'] = self._replydata[16]
	    replymsg['port']['type'] = self._replydata[17]
	except Exception, e:
	    self._log.warning('_processReply(): Error in processing port reply packet info')
	else:
	    if checklock == True:
		time.sleep(0.5)
	    else:
		lock.acquire()
		checklock = True
#	    lightdisc =   # store supply info to lightdisc. (global??) and implement the lock().
	finally:
	    lock.released()
	    checklock = False


    def run(self):
	try:
	    while(not self._stop):
	        try:
		    conn, addr = self._sock.accept()
	        except socket.timeout, t:
		    self._log.debug('run(): PortDiscoveryReply - Timeout in accepting connection')
	        else:
		    self._log.info('run(): PortDiscoveryReply - Accepted connection from ' + str(addr))
		    conn.settimeout(1.0)
		    while (not self._stop):         # why still need to check self._stop??
			try:
			    data = conn.recv(1024)  # try
			except socket.timeout, t:   # if there is exception
			    pass
			else:                       # if there is no exception 
			    self._processReply(data)
#			    conn.send(data)         # where to send?? what to send??
		    self._log.info('run(): PortDiscoveryReply - Closing server side connection from ' + str(addr))
		    conn.close()
#			    do I need to implement client check here? anything to send? to C.M. the enumerate info?

	except Exception, e:
	    self._log.critical('run(): PortDiscoveryReply - Raised Exception %s: %s', type(e), e)
	    traceback.print_exc()
	finally:
	    self._log.info('run(): PortDiscoveryReply - Cleaning up thread')
	    self._sock.close()
	    self._complete.set()  # Set the internal flag to true, all threads. All threads waiting for it to become true are awakened 
		
class FixtureDiscovery(Thread):
    def __init__(self, tport=DEFAULT_PORT, start=True):  # see other parameters need to add as input in __init__
	Thread.__init__(self)
	self._log = logging.getLogger('lightDiscovery')
	self._port = tport
	# need to implement packet sending configuration or sth. here?? socket options set to BROADCAST
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._stop = False
	self._complete = Event()
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    self.start()

    def killAll(self):
	self._stop = True
	if self.isAlive():
	    self._log.info('killAll(): FixtureDiscovery - Waiting for thread to finish')
	    self._complete.wait()
	self._log.info('killAll(): FixtureDiscovery - Thread stopped')
	# ?? (Ref: ControlSocket, do we need this killAll()?? )

    def run(self):
	try:
	    self._log.info('run(): FixtureDiscovery - Listening for Fixture Discovery connections..')
            self._sock.listen(1)
	    self._sock.settimeout(2.0)
	except Exception, e:
            self._log.critical('run(): FixtureDiscovery - Raised Exception %s: %s', type(e), e)
            traceback.print_exc()
#        finally:
#            self._log.info('run(): FixtureDiscovery - Cleaning up thread')
#            self._sock.close()
#            self._complete.set()  

class FixtureDiscoveryReply(Thread):
    def __init__(self, tport=DEFAULT_PORT, start=True):  # see other parameters need to add as input in __init__
	Thread.__init__(self)
	self._log = logging.getLogger('lightDiscovery')
	self._port = tport
	# need to implement packet sending configuration or sth. here?? socket options set to BROADCAST
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._stop = False
	self._complete = Event()
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    self.start()

    def killAll(self):
	self._stop = True
	if self.isAlive():
	    self._log.info('killAll(): FixtureDiscoveryReply - Waiting for thread to finish')
	    self._complete.wait()
	self._log.info('killAll(): FixtureDiscoveryReply - Thread stopped')
	# ?? (Ref: ControlSocket, do we need this killAll()?? )

    def _processReply(self, reply):
	# Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)Serial(4)
	replydata = reply.strip()
	try:
	    replymsg['fixture']['serial'] = replydata[12:16]
	except Exception, e:
	    self._log.warning('_processReply(): Error in processing fixture discovery reply packet info')
	else:
	    if checklock == True:
		time.sleep(0.5)
	    else:
		lock.acquire()
		checklock = True
##	    lightdisc =   # store supply info to lightdisc. (global??) and implement the lock().
	finally:
	    lock.released()
	    checklock = False

    def run(self):
	try:
	   while(not self._stop):
	        try:
		    conn, addr = self._sock.accept()
	        except socket.timeout, t:
		    self._log.debug('run(): Supply Discovery - Timeout in accepting connection')
	        else:
		    self._log.info('run(): Supply Discovery - Accepted connection from ' + str(addr))
		    conn.settimeout(1.0)
		    while (not self._stop):         # why still need to check self._stop??
			try:
			    data = conn.recv(1024)  # try
			except socket.timeout, t:   # if there is exception
			    pass
			else:                       # if there is no exception 
			    self._processReply(data)
#			    conn.send(data)         # where to send?? what to send??
		    self._log.info('run(): Supply Discovery - Closing server side connection from ' + str(addr))
		    conn.close()
#			    do I need to implement client check here? anything to send? to C.M. the enumerate info?

	except Exception, e:
	    self._log.critical('run(): Supply Discovery - Raised Exception %s: %s', type(e), e)
	    traceback.print_exc()
	finally:
	    self._log.info('run(): Supply Discovery - Cleaning up thread')
	    self._sock.close()
	    self._complete.set()  # Set the internal flag to true, all threads. All threads waiting for it to become true are awakened  

class FixtureDiscoveryCAReply(Thread):
    def __init__(self, tport=DEFAULT_PORT, start=True):  # see other parameters need to add as input in __init__
	Thread.__init__(self)
	self._log = logging.getLogger('lightDiscovery')
	self._port = tport
	# need to implement packet sending configuration or sth. here?? socket options set to BROADCAST
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._stop = False
	self._complete = Event()
	if start:  # threading.Thread.start() -> must be called at most once per thread object
	    self.start()

    def killAll(self):
	self._stop = True
	if self.isAlive():
	    self._log.info('killAll(): FixtureDiscoveryCAReply - Waiting for thread to finish')
	    self._complete.wait()
	self._log.info('killAll(): FixtureDiscoveryCAReply - Thread stopped')
	# ?? (Ref: ControlSocket, do we need this killAll()?? )

    def _processReply(self, reply):
	# Reply packet - MagicNumber(4)Version(2)Type(2)SequenceNumber(4)Cnt1(1)Cnt2(1)Cnt3(1)Cnt4(1)
	self._replydata = reply.strip()
	print self._replydata
	try:
	    replymsg['fixture']['cnt1'] = self._replydata[12]
	    replymsg['fixture']['cnt2'] = self._replydata[13]
	    replymsg['fixture']['cnt3'] = self._replydata[14]
	    replymsg['fixture']['cnt4'] = self._replydata[15]
	except Exception, e:
	    self._log.warning('_processReply(): Error in processing fixture discovery CA reply packet info')
	else:
	    if checklock == True:
		time.sleep(0.5)
	    else:
		lock.acquire()
		checklock = True
##	    lightdisc =   # store supply info to lightdisc. (global??) and implement the lock().
	finally:
	    lock.released()
	    checklock = False

    def run(self):
	try:
	   while(not self._stop):
	        try:
		    conn, addr = self._sock.accept()
	        except socket.timeout, t:
		    self._log.debug('run(): Fixture Discovery CA Reply - Timeout in accepting connection')
	        else:
		    self._log.info('run(): Fixture Discovery CA Reply - Accepted connection from ' + str(addr))
		    conn.settimeout(1.0)
		    while (not self._stop):         # why still need to check self._stop??
			try:
			    data = conn.recv(1024)  # try
			except socket.timeout, t:   # if there is exception
			    pass
			else:                       # if there is no exception 
			    self._processReply(data)
#			    conn.send(data)         # where to send?? what to send??
		    self._log.info('run(): Fixture Discovery CA Reply - Closing server side connection from ' + str(addr))
		    conn.close()
#			    do I need to implement client check here? anything to send? to C.M. the enumerate info?

	except Exception, e:
	    self._log.critical('run(): Fixture Discovery CA Reply - Raised Exception %s: %s', type(e), e)
	    traceback.print_exc()
	finally:
	    self._log.info('run(): Fixture Discovery CA Reply - Cleaning up thread')
	    self._sock.close()
	    self._complete.set()  # Set the internal flag to true, all threads. All threads waiting for it to become true are awakened  


# Supply Discovery packet and Reply packet
class KinetPktSplDisc(LittleEndianStructure):

    _fields_ = [("magic", c_int),     # c_int: 4 bytes; c_short - 2 bytes; c_ubytes - 1 byte; c_long - 8 bytes
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("tempIP", c_int)]  # ??
    payloadLock = Lock()
    def __init__(self, port):
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x0001
        self.seqnum     = 0x00000000
        self.tempIP     = 0xFFFFFFFF  # ??
    def __str__(self):
        return "KinetPktSplDisc"

class KinetPktSplDiscReply(LittleEndianStructure):

    _fields_ = [("magic", c_int),       
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("IP", c_int),  # ??
                ("MAC", c_ulong),
                ("ver", c_short),
                ("serial", c_int),
                ("universe", c_int),
                ("devstring", c_int),  # ??
                ("idstring", c_ulong)]  # ??
    def __init__(self, port):
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x0002
        self.seqnum     = 0x0000  #shoud match the sequence number of the Discover packet. ??
#       self.IP         =         # IP address of the supply issuing the discover reply packet - "no need to initialize here, got it from light automatically"
#	self.MAC        =         # MAC address of the supply issuing the discover reply packet
#	self.ver        =         # supported KiNET version for the supply - "also a given part from the light??
#	self.serial     =         # serial number of the supply issuing the discover reply packet
#	self.universe   =         # universe number of the supply issuing the discover reply packet. universe numbers can be used when sending data to achieve multicast-like semantics
#	self.devstring  =         # Hard-coded device description string
#	self.idstring   =         # user-modifiable device identity string
    def __str__(self):
        return "KinetPktSplDiscReply"

# Port Discovery packet and Reply packet
class KinetPktPortDisc(LittleEndianStructure): 
    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),  # 4 bytes ??
                ("pad", c_int)]
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x000A
        self.seqnum     = 0x00000000
	self.pad        = 0x00000000                
    def __str__(self):
        return "KinetPktPortDisc"

class KinetPktPortDiscReply(LittleEndianStructure): 
    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("ports", c_ubyte),  # KiNET discover ports reply header... Number of Port Entry blocks in the packet
                ("pad1", c_ubyte),   # KiNET discover ports reply header... Not used, set to 0x00
                ("pad2", c_short),   # KiNET discover ports reply header... Not used, set to 0x0000
                ("port", c_ubyte),   # KiNET discover ports reply entry ... Port number
                ("typeRE", c_ubyte), # KiNET discover ports reply entry ... Type of port
                ("pad1RE", c_short), # KiNET discover ports reply entry ... Not used, set to 0x0000
                ("pad2RE", c_int)]   # KiNET discover ports reply entry ... Not used, set to 0x00000000
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x000A
#       self.seqnum     = 0x0000     # ??
#	self.ports      =            # ??
	self.pad1       = 0x00
	self.pad2       = 0x0000
#	self.port       = 
#	self.typeRE     = 
	self.pad1RE     = 0x0000
	self.pad2RE     = 0x00000000           
    def __str__(self):
        return "KinetPktPortDiscReply"

# BlinkScan (KiNET v1) Fixture Discovery packet and Reply packet
class KinetPktBlkScan(LittleEndianStructure): 
    _fields_ = [("magic", c_int),     
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),    
                ("pad", c_int)]       # 4 bytes
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0002      # isn't it 0x0001??
        self.type       = 0x0201
        self.seqnum     = 0x00000000 
	self.pad        = 0x00000000              
    def __str__(self):
        return "KinetPktBlkScan"

class KinetPktBlkScanReply(LittleEndianStructure): 
    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),  
                ("serial", c_int)]    # Serial number of the attached fixture
    payloadLock = Lock()
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x0202
#       self.seqnum     = 0x00000000  
#	self.serial     
#       self.touched    = False         # ??
    def __str__(self):
        return "KinetPktBlkScanReply"   # ??

class KinetPktBlkScanCAReply(LittleEndianStructure): 
    _fields_ = [("magic", c_int), 
                ("version", c_short), 
                ("type", c_short), 
                ("seqnum", c_int),
                ("cnt1", c_ubyte),   # Number of Chromasic nodes on output 1
                ("cnt2", c_ubyte),   # Number of chromasic nodes on output 2
                ("cnt3", c_ubyte),   # Number of Chromasic nodes on output 3
                ("cnt4", c_ubyte)]   # Number of Chromasic nodes on output 4
    def __init__(self): 
        self.magic      = 0x4ADC0104
        self.version    = 0x0002
        self.type       = 0x0206
#       self.seqnum     = 0x0000     # ??
#	cnt1            =
#	cnt2            =
#	cnt3            = 
#	cnt4            =         
    def __str__(self):
        return "KinetPktPortBlkScanCAReply"

if __name__ == '__main__':  
    try:
        print '**main**'
        A = SupplyDiscovery()
	B = SupplyDiscoveryReply()
	C = PortDiscovery()
	D = PortDiscoveryReply()
	E = FixtureDiscovery()
	F = FixtureDiscoveryReply()
	G = FixtureDiscoveryCAReply()

        try:
            while (1):
                time.sleep(60)
        except KeyboardInterrupt, k:
            print 'Interrupted by user.'
#            C.killAll()
    except Exception, e:
        print 'Program generated some Exception.'
        traceback.print_exc()
#        A.killAll()
	
