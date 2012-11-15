

#  If ever I implement this again, each channel should have an alpha !!
#
#


from RGBLamp import RGBLamp
import types
from ctypes import c_ubyte

# Logging support
import logging
#from BeaconLogging import BeaconLogging

class IColorFlex(object):
    
    _STRANDSIZE = 50
    _ports = 0    
    _size = 0       # use len() to get externally
    _payload = []
    _payloadDirty = False
    _portorder = []  # array that does ch to dmx mapping (1 counts forward, -1 counts backward)
    ch = []
    portstart = 1   # power supply ports are base 1
    log = None
    
## ** Copy doesn't work!!)        
    # Ports are base 1
    def __init__(self, ports=1, portstart=1, portorder=None, log=None, icf=None):    
        if log:
            self.log = log
        #else:
        #    self.log = BeaconLogging("Beacon-beta", "IColorFlex", "IColorFlex.log", logging.WARNING,logging.WARNING, None)
        if portorder:
            self._portorder = portorder
        else:
            self._portorder = [1 for n in range(ports)] 
        self._ports = ports
        self.portstart = portstart 
        self._size = self._STRANDSIZE * ports
        self._payload = [(c_ubyte * (3 * self._STRANDSIZE))() for k in xrange(self._ports)]      
        self.ch = [RGBLamp() for k in xrange(self._size)]
        if icf != None:
            self.copy(icf)

    def copy(self, icf):
        self._payloadDirty = True
        for k in range(0,self._size):
            self.ch[k].copy(icf.ch[k])
    
    def __len__(self):
        return self._size
        
    def __str__(self):
        S = "<iColorFlex, ports="+str(self._ports)+", portstart="+str(self.portstart)+">"
        #S+="\n"
        #for k in range(0,self._size):
        #    S += "\t"+str(k) + ": " + str(self.ch[k]) + "\n"
        return S
    
    def __getitem__(self,key):
        return self.ch[key]
    
    def __getattribute__(self, name):       
        if name=='payload':
            return self._dmxpayload()
        return object.__getattribute__(self, name)
    
    def _dmxpayload(self):
        #print "_dmxpayload, portstart=", self.portstart
        if (self._payloadDirty):
            #print "[Generating Payload]"
            for p in xrange(self._ports):
                if self._portorder[p] > 0:
                    for k in xrange(self._STRANDSIZE):
                        self._payload[p][3*k] = self.ch[k+p*self._STRANDSIZE].r
                        self._payload[p][3*k+1] = self.ch[k+p*self._STRANDSIZE].g 
                        self._payload[p][3*k+2] = self.ch[k+p*self._STRANDSIZE].b
                else:
                    for k in xrange(self._STRANDSIZE):
                        
                        self._payload[p][3*(self._STRANDSIZE-1-k)] = self.ch[k+p*self._STRANDSIZE].r
                        self._payload[p][3*(self._STRANDSIZE-1-k)+1] = self.ch[k+p*self._STRANDSIZE].g 
                        self._payload[p][3*(self._STRANDSIZE-1-k)+2] = self.ch[k+p*self._STRANDSIZE].b
                self._payloadDirty = False
        return self._payload
  
    def setRGB(self, r, g, b, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            if r is not None: self.ch[k].r = r
            if g is not None: self.ch[k].g = g
            if b is not None: self.ch[k].b = b
        self._payloadDirty = True
        
    def setHSL(self, h, s, l, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            if h is not None: self.ch[k].h = h
            if s is not None: self.ch[k].s = s
            if l is not None: self.ch[k].l = l        
        self._payloadDirty = True
    
    def setHSLArray(self, h, s, l, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            if h is not None: self.ch[k].h = h[k]
            if s is not None: self.ch[k].s = s[k]
            if l is not None: self.ch[k].l = l[k]     
        self._payloadDirty = True

    def setRGBArray(self, r, g, b, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            if r is not None: self.ch[k].r = r[k]
            if g is not None: self.ch[k].g = g[k]
            if b is not None: self.ch[k].b = b[k]     
        self._payloadDirty = True

    def setRGBMixArray(self, r1, g1, b1, r2, g2, b2, p, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            if r1 is not None: self.ch[k].r = p*r1[k] + (1-p)*r2[k]
            if g1 is not None: self.ch[k].g = p*g1[k] + (1-p)*g2[k]
            if b1 is not None: self.ch[k].b = p*b1[k] + (1-p)*b2[k]     
        self._payloadDirty = True
                   
    def addRGB(self, r, g, b, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r += r
            self.ch[k].g += g
            self.ch[k].b += b
        self._payloadDirty = True
        
    def addHSL(self, h, s, l, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].h += h
            self.ch[k].s += s
            self.ch[k].l += l    
        self._payloadDirty = True
            
    def multRGB(self, r, g, b, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r *= r
            self.ch[k].g *= g
            self.ch[k].b *= b
        self._payloadDirty = True
        
    def multHSL(self, h, s, l, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].h *= h
            self.ch[k].s *= s
            self.ch[k].l *= l  
        self._payloadDirty = True
             
    def add(self, B, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r += B.ch[k].r
            self.ch[k].g += B.ch[k].g
            self.ch[k].b += B.ch[k].b 
        self._payloadDirty = True
        
    def sub(self, B, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r -= B.ch[k].r
            self.ch[k].g -= B.ch[k].g
            self.ch[k].b -= B.ch[k].b 
        self._payloadDirty = True
        
    # x = amount of B to mix in
    def mixRGB(self, B, x, channels=None):
        if channels==None:
            channels = range(0,self._size)
        if type(x)!=tuple:
            x = (x,x,x)        
        for k in channels:
            if x[0] is not None: self.ch[k].r = (1.0-x[0])*self.ch[k].r + x[0] * B.ch[k].r
            if x[1] is not None: self.ch[k].g = (1.0-x[1])*self.ch[k].g + x[1] * B.ch[k].g
            if x[2] is not None: self.ch[k].b = (1.0-x[2])*self.ch[k].b + x[2] * B.ch[k].b
        self._payloadDirty = True

    # icf is an array, x is an array (one lambda per icf, not rgb)
    #
    def mix(self, L, x):
        return sum( [L[i]*x[i] for i in range(len(L))] )
                     
    def loadFromMixArrayRGB(self, icf, x, channels=None):
        if channels==None:
            channels = range(0,self._size)

        # list of lambda values
        X = [x[i] for i in range(len(icf))]
        
        # mix each component
        for k in channels:
            self.ch[k].r = self.mix( [icf[i].ch[k].r for i in range(len(icf))], X)
            self.ch[k].g = self.mix( [icf[i].ch[k].g for i in range(len(icf))], X)
            self.ch[k].b = self.mix( [icf[i].ch[k].b for i in range(len(icf))], X)
            
        self._payloadDirty = True
    
    def setFrom(self, B, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r = B.ch[k].r
            self.ch[k].g = B.ch[k].g
            self.ch[k].b = B.ch[k].b
        self._payloadDirty = True
         
    def mixHSL(self, B, x, channels=None):
        if channels==None:
            channels = range(0,self._size)
        if type(x)!=tuple:
            x = (x,x,x)        
        for k in channels:
            if x[0] is not None: self.ch[k].h = (1.0-x[0])*self.ch[k].h + x[0] * B.ch[k].h
            if x[1] is not None: self.ch[k].s = (1.0-x[1])*self.ch[k].s + x[1] * B.ch[k].s
            if x[2] is not None: self.ch[k].l = (1.0-x[2])*self.ch[k].l + x[2] * B.ch[k].l
        self._payloadDirty = True
      
    def funcRGB(self, f, channels=None, param=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r, self.ch[k].g, self.ch[k].b = f(self.ch[k].r, self.ch[k].g, self.ch[k].b, k, param)            
        self._payloadDirty = True

    def funcHSL(self, f, channels=None, param=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].h, self.ch[k].s, self.ch[k].l = f(self.ch[k].h, self.ch[k].s, self.ch[k].l, k, param)            
        self._payloadDirty = True
                
    def htp(self, B, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r = max(self.ch[k].r, B.ch[k].r)
            self.ch[k].g = max(self.ch[k].g, B.ch[k].g)
            self.ch[k].b = max(self.ch[k].b, B.ch[k].b)
        self._payloadDirty = True
        
    def ltp(self, B, channels=None):
        if channels==None:
            channels = range(0,self._size)
        for k in channels:
            self.ch[k].r = min(self.ch[k].r, B.ch[k].r)
            self.ch[k].g = min(self.ch[k].g, B.ch[k].g)
            self.ch[k].b = min(self.ch[k].b, B.ch[k].b)
        self._payloadDirty = True    

if __name__ == '__main__':  
    print '**main**'
    Z = IColorFlex(2)

    for c in xrange(6):
        Z.setRGB(c,c,c,[c])
   
    #A.setRGB(10,10,10)
#    Z = IColorFlex(2,A)
  #  print A

 #   print Z
  #  Z.multRGB(0, 0, 0)
   
  #  Z.mixRGB(A, (0.5, 0.5, 0.5),[0]) 
  #  Z.mixRGB(A, (0.5, 0.5, 0.5),[1])
  #  Z.mixHSL(A, (0.5, 0.5, 1),[2])

    print Z
 #   print A
    
    # ctype ubyte array 3 x size
    
    for p in xrange(2):
        for c in xrange(3):
            print p,c,"=>", Z.payload[p][3*c], Z.payload[p][3*c+1], Z.payload[p][3*c+2]
