import math
from colormath.color_objects import HSLColor, RGBColor

def hsltorgb(h, s, l):
    rgb = HSLColor(h,s,l).convert_to('rgb')
    return rgb.rgb_r, rgb.rgb_g, rgb.rgb_b

def rgbtohsl(r, g, b):
    hsl = RGBColor(r,g,b).convert_to('hsl')
    return hsl.hsl_h, hsl.hsl_s, hsl.hsl_l

class RGBLamp(object):
    def __init__(self):
        self.r = 0
        self.g = 0
        self.b = 0
        self.h = 0.0
        self.s = 0.0
        self.l = 0.0
        self._hsldirty = False  # HSL Values changed since last recalc
        self._rgbdirty = False  # RGB Values changed since last recalc

    def setRGB(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b
    
    def setHSL(self, h, s, l):
        self.h = h
        self.s = s
        self.l = l
    
    def copy(self, B):
        self.r = B.r
        self.g = B.g
        self.b = B.b
    
    # attributes rgb, hsl implemented below -- return tuples
    
    def __setattr__(self, name, value):
        if name=='r' or name=='g' or name=='b':
            #  print "__setattr__ ", name, value    
            value = int(round(value))
            if value > 255:
                value = 255
            elif value < 0:
                value = 0        
            self._rgbdirty=True
        elif name=='h':
            # print "__setattr__ ", name, value
            if math.fabs(value) > 360.0:
                value = value % 360
            if value < 0: 
                value = 360 + value
            self._hsldirty=True
        elif name=='s' or name=='l':
            if value > 1.0:
                value = 1.0
            elif value < 0.0:
                value = 0.0
            self._hsldirty=True
        object.__setattr__(self,name,value)
            
    def __getattribute__(self, name):        
        if name=='r' or name=='g' or name=='b':
            #  print "__getattribute__ ", name
            if object.__getattribute__(self,'_hsldirty')==True:
                self._hsldirty=False
                self._recalcRGB()                
        elif name=='h' or name=='s' or name=='l':
            # print "__getattribute__ ", name
            if object.__getattribute__(self,'_rgbdirty')==True:
                self._rgbdirty=False
                self._recalcHSL()
        elif name=='hsl':
                return (self.h, self.s, self.l)
        elif name=='rgb':
                return (self.r, self.g, self.b)
        return object.__getattribute__(self,name)   
    

    
    def _recalcRGB(self):
        #print "_recalcRGB()"
        rgb = HSLColor(self.h,self.s,self.l).convert_to('rgb')
        object.__setattr__(self,'r',int(round(rgb.rgb_r)))
        object.__setattr__(self,'g',int(round(rgb.rgb_g)))
        object.__setattr__(self,'b',int(round(rgb.rgb_b)))
                
    def _recalcHSL(self):
        #print "_recalcHSL()"
        hsl = RGBColor(self.r,self.g,self.b).convert_to('hsl')
        object.__setattr__(self,'h',hsl.hsl_h)
        object.__setattr__(self,'s',hsl.hsl_s)
        object.__setattr__(self,'l',hsl.hsl_l)
    
    def __str__(self):
        return "r=%i g=%i b=%i h=%0.1f s=%0.3f l=%0.3f" % (self.r,self.g,self.b,self.h,self.s,self.l)
    
    
    
if __name__ == '__main__':  
    print '**main**'
    A = RGBLamp()
    A.r = 240
    A.g = 211
    A.b = 41
    print "A", A
    A.l = 0.75
    print "A", A
    
    A.setRGB(42,42,42)
    print "A", A
    A.setHSL(42,0.75,0.75)
    print "A", A
    print "A.hsl", A.hsl
    print "A.rgb", A.rgb
    
    A.setHSL(710, 2, 2)
    print "A.hsl", A.hsl