import sys
sys.path.append("/var/www/html/lighting/app/process")

from namespaces import name
import math
import constants
import operator
def dominantcluster(out,dl,dom,domc):
    dimmer=1
    fade=0
    templist = dl[:]

    numberoffixture = 10
    
    nero = (0,0,0)
    bianco = (255,255,255)

    
    #d = (sorted(dominant, key=lambda dominant:dominant[1][0], reverse=True ))
    #sorted(d, key=lambda d:d[1][0], reverse=True )
    #print 'before filter=================',dl
    
    #calculate the 3rd color
    for x in templist:
        #print x
        if (abs(x[1][0]-dom[0])<30) and (abs(x[1][1]-dom[1])<30) and (abs(x[1][2]-dom[2])<30):
            dl.remove(x)
            #print ' del:',x
        elif (abs(x[1][0]-domc[0])<30) and (abs(x[1][1]-domc[1])<30) and (abs(x[1][2]-domc[2])<30):
            dl.remove(x)
            #print ' del:',x          
  
    if (len(dl)>0):rddom = dl[0][1]
    else: rddom = dom

    #print 'after filter=================',dl, '-----',len(dl)
    print('1st: ',dom, '   2nd constr: ', domc, '    3rd: ', rddom)
  
 
    #fill up all the fixtures with one color at the time
    for x in range(1,numberoffixture):
        out.append((3,name[x], dom, 1,fade))
    for x in range(1,numberoffixture):
        out.append((3,name[x], domc, 1,fade))
    for x in range(1,numberoffixture):
        out.append((3,name[x], rddom, 1,fade))


    
    #fibonacci
    fib=[None]*50
    fib[0]=0
    fib[1]=1
    fib[2]=1
    #setup series
    for n in range(3,50):
        fib[n]=fib[n-1]+fib[n-2]
        #print fib[n]

    #random n 0-50
    import random
    rrange = 25
    n=random.randrange(1,rrange)
    print n
    
    #for x in range(1,rrange):
    #    newc = (operator.mod(dom[0]+fib[operator.mod(n+x,rrange)],256),
     #           operator.mod(dom[1]+fib[operator.mod(n+x,rrange)],256),
     #           operator.mod(dom[2]+fib[operator.mod(n+x,rrange)],256))
        #each 1sec
     #   out.append((1,name[operator.mod(x,numberoffixture)+1],newc, dimmer,fade))
       
    
                   
        
  
    #risoluzione
    #delay=5
    #for x in range(0,numberoffixture):
    #    delay = delay / math.sqrt(x)
    #    out.append((delay,name[operator.mod(x+9,numberoffixture)+1], dom, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+8,numberoffixture)+1], rddom, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+7,numberoffixture)+1], rddom, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+6,numberoffixture)+1], dom, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+5,numberoffixture)+1], dom, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+4,numberoffixture)+1], dom, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+3,numberoffixture)+1], dom, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+2,numberoffixture)+1], domc, dimmer,fade))
    #    out.append((delay,name[operator.mod(x+1,numberoffixture)+1], domc, dimmer,fade))
    #    out.append((delay,name[operator.mod(x,numberoffixture)+1],   domc, dimmer,fade))


    #finale
    for x in range(0,10):
        n=random.randrange(1,numberoffixture)
        out.append((0,name[n], bianco, 1,fade))
        out.append((0,name[n], nero, 1,fade))
       
        
    for x in range(0,180,10):
       out.append((0,name['incandescent'], (0,0,0), x,fade))
    out.append((1,name['incandescent'], (0,0,0), 0,fade))
    out.append((1,name['incandescent'], (0,0,0), 180,fade))
    out.append((1,name['incandescent'], (0,0,0), 0,fade))
    out.append((1,name['incandescent'], (0,0,0), 80,fade))
    out.append((1,name['incandescent'], (0,0,0), 0,fade))
    
    
    return out
