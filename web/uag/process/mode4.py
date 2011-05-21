from namespaces import name
import math
import operator
def dominantcluster(out,dominant,dom,domc):
    dimmer=1
    fade=0
    templist = dominant[:]
    for x in templist:
        #print x
        if (abs(x[1][0]-dom[0])<40) and (abs(x[1][1]-dom[1])<40) and (abs(x[1][2]-dom[2])<40):
            dominant.remove(x)
            #print ' del:',x
        elif (abs(x[1][0]-domc[0])<40) and (abs(x[1][1]-domc[1])<40) and (abs(x[1][2]-domc[2])<40):
            dominant.remove(x)
            #print ' del:',x
    #print dominant
    #print '========',dom, ' -----', domc
    rddom = dominant[0][1]

    out.append((0,name['living-room-front'], dom, dimmer,fade))
    out.append((0,name['living-room-right'], rddom, dimmer,fade))
    out.append((0,name['kitchen'], dom, dimmer,fade))
    out.append((0,name['stairs'], domc, dimmer,fade))
    out.append((0,name['bedroom'], domc, dimmer,fade))
    out.append((0,name['entrance-door'], domc, dimmer,fade))
    out.append((0,name['window-left'], domc, dimmer,fade))
    out.append((0,name['window-right'], domc, dimmer,fade))

    for x in range(1,8):
        out.append((0.5,name[x], dom, 1,fade))
    for x in range(1,8):
        out.append((0.5,name[x], domc, 1,fade))
    for x in range(1,8):
        out.append((0.5,name[x], rddom, 1,fade))

    delay=4
    for x in range(1,8):
        delay = delay / math.sqrt(x)
        out.append((delay,name[x], dom, 1,fade))
    delay=4
    for x in range(1,8):
        delay = delay / math.sqrt(x)
        out.append((delay,name[x], domc, 1,fade))
    delay=5
    for x in range(1,8):
        delay = delay / math.sqrt(x)
        out.append((delay,name[x], rddom, 1,fade))

    delay=5
    for x in range(1,8):
        delay = delay / math.sqrt(x)
        out.append((delay,operator.mod(x,8), dom, dimmer,fade))
        out.append((delay,operator.mod(x+8,8), rddom, dimmer,fade))
        out.append((delay,operator.mod(x+2,8), dom, dimmer,fade))
        out.append((delay,operator.mod(x+3,8), domc, dimmer,fade))
        out.append((delay,operator.mod(x+4,8), domc, dimmer,fade))
        out.append((delay,operator.mod(x+5,8), domc, dimmer,fade))
        out.append((delay,operator.mod(x+6,8), domc, dimmer,fade))
        out.append((delay,operator.mod(x+7,8), domc, dimmer,fade))    
        
        
    out.append((0,name['incandescent'], (0,0,0), 1,fade))
    out.append((1,name['incandescent'], (0,0,0), 0,fade))
    out.append((0,name['incandescent'], (0,0,0), 1,fade))
    out.append((1,name['incandescent'], (0,0,0), 0,fade))
    
    return out
