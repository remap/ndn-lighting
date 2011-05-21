# returns a list of tuples [delay, fixture name, RGB-dec-color, intensity[0,1], fade]
# utilities.dimmer(comb,stddev_c) calculates the light intensity according to stddev combined

# threshold for contrast img combined stddev = 250
stddev_c = 250


import Image
import imgfunctions
import utilities
import ImageStat
from namespaces import name

def imgprocess(namef, mode):
    im = Image.open(namef)
    print namef

    #get 5 dominant colors
    print im.getbands()
    l = im.getcolors(im.size[0]*im.size[1])
    dominant = (sorted(l, key=lambda l:l[0], reverse=True ))[0:150]

    #print dominant
    
    #convert RGB to HSL
    
  #  for color in dominant:
  #      print imgfunctions.RGB_to_HSL(color[1][0],color[1][1],color[1][2])

    #print imganalysis.pbrightness(im)

    #stddev comb
    comb = (ImageStat.Stat(im).stddev)[0]+(ImageStat.Stat(im).stddev)[1]+(ImageStat.Stat(im).stddev)[2]
    #print comb


    if stddev_c < comb:
        #high contrast img
        fade = 0
    else: fade = 3

    #values to assign
    dimmerv = utilities.dimmer(comb,stddev_c)
    domc = dominant[0][1]
    delay=0
  
    #select the most contrastant color vs the dominant

    if stddev_c > comb:
        #if low contrast apply gaussian filter
        from imgfunctions import filterg 
        medim = filterg(im)
        #medim.show()
        mediml = medim.getcolors(medim.size[0]*medim.size[1])
        dominant = (sorted(l, key=lambda mediml:mediml[0], reverse=True ))[0:150]
    
    #get the most distant color from the dominant
    domvalue = dominant[0][1][0]+dominant[0][1][1]+dominant[0][1][2]
    maxl = 0, dominant[0][1]
    for color in dominant:
      if maxl[0] < abs(dominant[0][1][0] - color[1][0]) :
          maxl = abs(dominant[0][1][0] - color[1][0]) , color[1]

    #values to pass to the list
    
    domcontrst = maxl[1]
    
    #mode1 -swap 2 planes
    if mode==1:
        delay=0
        out = []
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

    if mode==2:
        #uses 8 most dominant colors
        out=list((delay,name['living-room-front'], dominant[0][1], dimmerv,fade))
        out.append((delay,name['living-room-right'], dominant[1][1], dimmerv,fade))
        out.append((delay,name['kitchen'], dominant[2][1], dimmerv,fade))
        out.append((delay,name['stairs'], dominant[3][1], dimmerv,fade))
        out.append((delay,name['bedroom'], dominant[4][1], dimmerv,fade))
        out.append((delay,name['entrance-door'], dominant[5][1], dimmerv,fade))
        out.append((delay,name['window-left'], dominant[6][1], dimmerv,fade))
        out.append((delay,name['window-right'], dominant[7][1], dimmerv,fade))
        
    #print out
    return out

