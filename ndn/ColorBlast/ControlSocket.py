
import sys, traceback
import socket, time

from threading import Thread, Event

# Logging support
import logging

import KinetSender
from ColorBlast import *

# Control socket info
DEFAULT_PORT = 50009

#SRC_IP = '131.179.141.35'
SRC_IP = '127.0.0.1'
 
DATA_ENABLER_IP = '131.179.141.50'
NUM_PORTS   = 1
NUM_LIGHTS  = 4  #was 2

def _lightMap(id):
    # return (PORT, LIGHT NUMBER)
    # PORT numbers and LIGHT numbers start from 1
    # Light ID's also start from 1
    if id==1: return (1, 1)
    if id==2: return (1, 2)
    if id==3: return (1, 3)
    if id==4: return (1, 4)
    return (0, 0)

class ControlSocket(Thread):
    def __init__(self, cport=DEFAULT_PORT, start=True):
        Thread.__init__(self)
        logging.basicConfig(filename="ColorBlast.log", filemode='w',
                level=logging.INFO)
        self._log = logging.getLogger('ControlSocket')
        self._port = cport
#        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._stop = False
        self._complete = Event()
        self._colorBlast = ColorBlast(NUM_PORTS, NUM_LIGHTS, logging.getLogger('ColorBlast'))
        self._kinetSender = KinetSender.KinetSender(SRC_IP, DATA_ENABLER_IP, NUM_PORTS, 3*NUM_LIGHTS, logging.getLogger('KinetSender'))
        if start:
            self.start()
    
    def killAll(self):
        self._stop = True
        if self.isAlive():
            self._log.info('killAll(): Waiting for thread to finish')
            self._complete.wait()
        self._log.info('killAll(): Thread stopped')
        KinetSender.finish(self._kinetSender)

    def _processLight(self, id, r, g, b):
        (port, ch) = _lightMap(id)
        if port==0 or ch==0: return 0
        self._colorBlast.setRGB(r, g, b, [port-1], [ch-1])
        return port

    def _processCmd(self, cmd):
        # Command format:
        # NUM LIGHTS | ID1 | R | G | B | ID2 | R | ...
        cfields = cmd.strip().split('|')
        ports_to_update = []
        try:
            num_lights = int(cfields[0])
	    print("********* in _processCmd ************")
        except Exception, e:
            self._log.error('_processCmd(): Can not process # of lights - %s:%s',
                    type(e), e)
        else:
            if num_lights <= 0: return
            for i in range(num_lights):
                try:
                    id = int(cfields[4*i+1])
                    r = int(cfields[4*i+2])
                    g = int(cfields[4*i+3])
                    b = int(cfields[4*i+4])
                    ports_to_update.append(self._processLight(id, r, g, b))
                except Exception, e:
                    self._log.warning('_processCmd(): Error in processing cmd - %s:%s', type(e), e)
            for p in ports_to_update:
                if p==0: continue
                try:
                    self._kinetSender.setPayload(p, self._colorBlast.payload[p-1])
                except Exception, e:
                    self._log.warning('_processCmd(): Error in updating port %d', port)
                    self._log.warning('_processCmd(): Description - %s:%s', type(e), e)

    def run(self):
        try:
            self._log.info('run(): Binding to port %d', self._port)
#            self._sock.bind(('', self._port))
	    self._sock.bind(('127.0.0.1', self._port))
            self._log.info('run(): Listening for connections..')
#            self._sock.listen(1)
            self._sock.settimeout(2.0)
            
            # Begin
            while(not self._stop):
#                try:
#                    conn, addr = self._sock.accept()
#                except socket.timeout, t:
#                    self._log.debug('run(): Timeout in accepting connection')
                    #pass
#                else:
#                    self._log.info('run(): Accepted connection from '+str(addr))
#                    conn.settimeout(1.0)
#                    while (not self._stop):
                        try:
			    print("**************** before data received... **********");
                            data, recvaddr = self._sock.recvfrom(1024)
			    print("**************** data received... *****************");
                        except socket.timeout, t:
                            pass
                        else:
                            # When the client shuts down from user interrupt, this condition helps
                            # close the socket on the server side.
                            if not data: 
                                self._log.info('run(): Client closed connection.')
                                break
                            self._log.info('run(): Rcvd - %s', data)
#                            if data=='Q\n' or data=='q\n':
#                                self._log.info('run(): Client closed connection.')
#                                self_sock.send(data)
#                                break
                            self._processCmd(data)
#                            self._sock.send('OK\n') # conn.send('OK\n');
#                    self._log.info('run(): Closing server side connection from ')
#                    conn.close()
        except Exception, e:
            self._log.critical('run(): Raised Exception %s: %s', type(e), e)
            traceback.print_exc()
        finally:
            self._log.info('run(): Cleaning up thread')
            self._sock.close()
            self._complete.set()
		

if __name__ == '__main__':  
    try:
        print '**main**'
        C = ControlSocket()

        try:
            while (1):
                time.sleep(60)
        except KeyboardInterrupt, k:
            print 'Interrupted by user.'
            C.killAll()
    except Exception, e:
        print 'Program generated some Exception.'
        traceback.print_exc()
        C.killAll()
