# returns a list of tuples [delay, fixture name, RGB-dec-color, intensity[0,1], fade]
# utilities.dimmer(comb,stddev_c) calculates the light intensity according to stddev combined

# threshold for contrast img combined stddev = 250
stddev_c = 250


import Image
import imgfunctions
import utilities
import ImageStat
import constants
import mode3
import mode4
from namespaces import name
import fixtureuty

def imgprocess(namef, mode):
    im = Image.open(namef)
    print 'image filename: ',namef, 'ch: ', im.getbands()

    #get dominant colors
    l = im.getcolors(im.size[0]*im.size[1])
    dominant = (sorted(l, key=lambda l:l[0], reverse=True ))[0:550]

    #print dominant[0:10]
    
    #convert RGB to HSL
    
  #  for color in dominant:
  #      print imgfunctions.RGB_to_HSL(color[1][0],color[1][1],color[1][2])

    #print imganalysis.pbrightness(im)

    #stddev comb
    comb = (ImageStat.Stat(im).stddev)[0]+(ImageStat.Stat(im).stddev)[1]+(ImageStat.Stat(im).stddev)[2]
    print comb



    if stddev_c < comb:
        #high contrast img
        fade = 0
    else: fade = 3

    #values to assign
    dimmerv = utilities.dimmer(comb,stddev_c)
    domc = dominant[0][1]
    delay=0
  
    #select the most contrastant color vs the dominant

    from imgfunctions import filterg 
    medim = filterg(im)
    #medim.show()
    mediml = medim.getcolors(medim.size[0]*medim.size[1])
    dominant = (sorted(l, key=lambda mediml:mediml[0], reverse=True ))[0:150]

    #filter threshold
    appdf=[elem for elem in dominant if elem[0] > 5]
    df=[e for e in appdf if (e[1][0]+e[1][1]+e[1][2])>30]
    #print df
    #get the most distant color (brightness) from the dominant
    domvalue = dominant[0][1][0]+dominant[0][1][1]+dominant[0][1][2]
    maxl = 0, dominant[0][1]
    for color in df:
      if maxl[0] < abs(df[0][1][0] - color[1][0]) :
          maxl = abs(df[0][1][0] - color[1][0]) , color[1]

    #values to pass to the list
    
    domcontrst = maxl[1]

    #create list
    out = []
    
    
    #mode1 -swap 2 planes
    if mode==1:
        delay=0
        out.append((delay,name['living-room-front'], domc, dimmerv,fade))
        out.append((delay,name['living-room-right'], domc, dimmerv,fade))
        out.append((delay,name['kitchen'], domc,dimmerv,fade))
        #and put it on the 'stairs'  ..to contrast the color in the 'living-room-front'
        out.append((delay,name['stairs'], domcontrst, dimmerv,fade))
        out.append((delay,name['bedroom'], domcontrst, dimmerv,fade))
        out.append((delay,name['entrance-door'], domcontrst, dimmerv,fade))
        out.append((delay,name['window-left'], domcontrst, dimmerv,fade))
        out.append((delay,name['window-right'], domcontrst, dimmerv,fade))
       

        #after 5sec
        delay=5
        out.append((delay,name['living-room-front'], domcontrst, dimmerv,fade))
        out.append((delay,name['living-room-right'], domcontrst, dimmerv,fade))
        out.append((delay,name['kitchen'], domcontrst, dimmerv,fade))
        out.append((delay,name['stairs'], domc, dimmerv,fade))
        out.append((delay,name['bedroom'], domc, dimmerv,fade))
        out.append((delay,name['entrance-door'], domc, dimmerv,fade))
        out.append((delay,name['window-left'], domc, dimmerv,fade))
        out.append((delay,name['window-right'], domc, dimmerv,fade))

        #after 5sec
        delay=5
        out.append((delay,name['living-room-front'], nero, 0,fade))
        out.append((delay,name['living-room-right'], nero, 0,fade))
        out.append((delay,name['kitchen'], nero, 0,fade))
        out.append((delay,name['stairs'], nero, 0,fade))
        out.append((delay,name['bedroom'], nero, 0,fade))
        out.append((delay,name['entrance-door'], nero, 0,fade))
        out.append((delay,name['window-left'], nero, 0,fade))
        out.append((delay,name['window-right'], nero, 0,fade))

        out.append((3,name['incandescent'], (0,0,0), 230,fade))
        out.append((1,name['incandescent'], (0,0,0), 0,fade))
        out.append((0,name['incandescent'], (0,0,0), 230,fade))
        out.append((1,name['incandescent'], (0,0,0), 0,fade))

    if mode==2:
        #uses 8 most dominant colors
        out.append((delay,name['living-room-front'], dominant[0][1], dimmerv,fade))
        out.append((delay,name['living-room-right'], dominant[1][1], dimmerv,fade))
        out.append((delay,name['kitchen'], dominant[2][1], dimmerv,fade))
        out.append((delay,name['stairs'], dominant[3][1], dimmerv,fade))
        out.append((delay,name['bedroom'], dominant[4][1], dimmerv,fade))
        out.append((delay,name['entrance-door'], dominant[5][1], dimmerv,fade))
        out.append((delay,name['window-left'], dominant[6][1], dimmerv,fade))
        out.append((delay,name['window-right'], dominant[7][1], dimmerv,fade))

    if mode==3:
        mode3.iterone(out,domc,domcontrst)

    if mode==4:
        mode4.dominantcluster(out,df,domc,domcontrst)

    fixtureuty.alloff(0,out,0)
    #print out
    return out

