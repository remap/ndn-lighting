import math
from namespaces import name
from decimal import *


#switch color between dominant and dominant contrast 
def iterone(out,dom,domc):
    delay=5
    for x in range(1,9):
        delay = delay / math.sqrt(x)
        
        # print delay
        color = (0,0,0)
        dimmer=1
        fade=0
        #switch color
        d=dom
        dom=domc
        domc = d
        out.append((delay,name['living-room-front'], dom, dimmer,fade))
        out.append((delay,name['living-room-right'], dom, dimmer,fade))
        out.append((delay,name['kitchen'], dom,dimmer,fade))
        out.append((delay,name['stairs'], domc, dimmer,fade))
        out.append((delay,name['bedroom'], domc, dimmer,fade))
        out.append((delay,name['entrance-door'], domc, dimmer,fade))
        out.append((delay,name['window-left'], domc, dimmer,fade))
        out.append((delay,name['window-right'], domc, dimmer,fade))
        #out.append((x,'===========================', domc, dimmer,fade))
    out.append((0,name['incandescent'], (0,0,0), 1,fade))
    out.append((1,name['incandescent'], (0,0,0), 0,fade))
    out.append((0,name['incandescent'], (0,0,0), 1,fade))
    out.append((1,name['incandescent'], (0,0,0), 0,fade))
    return out

