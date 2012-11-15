
import sys, traceback
from RGBLamp import RGBLamp
import KinetSender
import time
import types
from ctypes import c_ubyte

# Logging support
import logging

class ColorBlast(object):
    
    _ports = 0    
    _lights = []
    _payload = []
    _payloadDirty = False
    ch = []
    log = None
    
    # Ports are base 0 in this implementation
    def __init__(self, ports, lights, log):    
        self.log = log
        self._ports = ports
        if type(lights)==types.IntType:
            self._lights = [ lights for k in range(self._ports) ]
        else:
            self._lights = lights
        self._payload = [(c_ubyte * (3 * self._lights[k]))() for k in range(self._ports)]      
        self.ch = [[RGBLamp() for l in xrange(self._lights[p])] for p in range(self._ports)]

    #def __len__(self):
    #    return self._size
        
    def __str__(self):
        S = "<ColorBlast, ports="+str(self._ports)+", lights="+str(self._lights)+">"
        #S+="\n"
        #for k in range(0,self._size):
        #    S += "\t"+str(k) + ": " + str(self.ch[k]) + "\n"
        return S
    
    def __getitem__(self, port):
        return self.ch[port]
    
    def __getattribute__(self, name):       
        if name=='payload':
            return self._dmxpayload()
        return object.__getattribute__(self, name)
    
    def _dmxpayload(self):
        #print "_dmxpayload, portstart=", self.portstart
        if (self._payloadDirty):
            self.log.debug('Generating payload')
            for p in xrange(self._ports):
                for k in xrange(self._lights[p]):
                    self._payload[p][3*k] = self.ch[p][k].r
                    self._payload[p][3*k+1] = self.ch[p][k].g 
                    self._payload[p][3*k+2] = self.ch[p][k].b
            self._payloadDirty = False
        return self._payload
  
    def setRGB(self, r, g, b, ports=None, channels=None):
        if ports==None:
            ports = range(self._ports)
        for p in ports:
            if channels==None:
                channels = range(self._lights[p])
            for k in channels:
                if r is not None: self.ch[p][k].r = r
                if g is not None: self.ch[p][k].g = g
                if b is not None: self.ch[p][k].b = b
                self.log.info('Set port='+str(p)+',light='+str(k)+':'+str(r)+','+str(g)+','+str(b))
        self._payloadDirty = True
        
    def setHSL(self, h, s, l, ports=None, channels=None):
        if ports==None:
            ports = range(self._ports)
        for p in ports:
            if channels==None:
                channels = range(0,self._lights[p])
            for k in channels:
                if h is not None: self.ch[p][k].h = h
                if s is not None: self.ch[p][k].s = s
                if l is not None: self.ch[p][k].l = l        
        self._payloadDirty = True
    
    def addRGB(self, r, g, b, ports=None, channels=None):
        if ports==None:
            ports = range(self._ports)
        for p in ports:
            if channels==None:
                channels = range(0,self._lights[p])
            for k in channels:
                self.ch[p][k].r += r
                self.ch[p][k].g += g
                self.ch[p][k].b += b
        self._payloadDirty = True
        
    def addHSL(self, h, s, l, ports=None, channels=None):
        if ports==None:
            ports = range(self._ports)
        for p in ports:
            if channels==None:
                channels = range(0,self._lights[p])
            for k in channels:
                self.ch[p][k].h += h
                self.ch[p][k].s += s
                self.ch[p][k].l += l    
        self._payloadDirty = True
            
    def multRGB(self, r, g, b, ports=None, channels=None):
        if ports==None:
            ports = range(self._ports)
        for p in ports:
            if channels==None:
                channels = range(0,self._lights[p])
            for k in channels:
                self.ch[p][k].r *= r
                self.ch[p][k].g *= g
                self.ch[p][k].b *= b
        self._payloadDirty = True
        
    def multHSL(self, h, s, l, ports=None, channels=None):
        if ports==None:
            ports = range(self._ports)
        for p in ports:
            if channels==None:
                channels = range(0,self._lights[p])
            for k in channels:
                self.ch[p][k].h *= h
                self.ch[p][k].s *= s
                self.ch[p][k].l *= l    
        self._payloadDirty = True
            

NUM_PORTS   = 1
NUM_LIGHTS  = 12 # was 4

if __name__ == '__main__':  
    print '**main**'
    logging.basicConfig(filename="ColorBlast.log", filemode='w', level=logging.DEBUG)
    Z = ColorBlast(NUM_PORTS, NUM_LIGHTS, logging.getLogger('ColorBlast'))
#    K = KinetSender.KinetSender("131.179.143.102", "131.179.143.99", NUM_PORTS,
#            3*NUM_LIGHTS, logging.getLogger('KinetSender'))
    K = KinetSender.KinetSender("127.0.0.1", "131.179.141.50", NUM_PORTS,
            3*NUM_LIGHTS, logging.getLogger('KinetSender'))

    N1=-1
    N2=256
    kintr=0
    while (N1<255):
    	try:
            N1+=1
            N2-=1
            Z.setRGB(N1, N1, N1, None, [0, 1])
            Z.setRGB(N2, N2, N2, None, [2, 3])
            K.setPayload(1, Z.payload[0])  # Port = 0 in ColorBlast.py, equivalent to Port = 1 in KinetSender.py   
            print str(K.getPayload(1))
            time.sleep(2)
        except KeyboardInterrupt, k:
            print 'Interrupted by user.'
            KinetSender.finish(K)
        except Exception, e:
            print 'Program generated some Exception.'
            traceback.print_exc()
            KinetSender.finish(K)
    
    KinetSender.finish(K)
