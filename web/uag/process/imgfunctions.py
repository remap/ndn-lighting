

def HSL_to_RGB(h,s,l):
    ''' Converts HSL colorspace (Hue/Saturation/Value) to RGB colorspace.
        Formula from http://www.easyrgb.com/math.php?MATH=M19#text19
 
        Input:
            h (float) : Hue (0...1, but can be above or below
                              (This is a rotation around the chromatic circle))
            s (float) : Saturation (0...1)    (0=toward grey, 1=pure color)
            l (float) : Lightness (0...1)     (0=black 0.5=pure color 1=white)
 
        Ouput:
            (r,g,b) (integers 0...255) : Corresponding RGB values
 
        Examples:
            >>> print HSL_to_RGB(0.7,0.7,0.6)
            (110, 82, 224)
            >>> r,g,b = HSL_to_RGB(0.7,0.7,0.6)
            >>> print g
            82
    '''
    def Hue_2_RGB( v1, v2, vH ):
        while vH<0.0: vH += 1.0
        while vH>1.0: vH -= 1.0
        if 6*vH < 1.0 : return v1 + (v2-v1)*6.0*vH
        if 2*vH < 1.0 : return v2
        if 3*vH < 2.0 : return v1 + (v2-v1)*((2.0/3.0)-vH)*6.0
        return v1
 
    if not (0 <= s <=1): raise ValueError,"s (saturation) parameter must be between 0 and 1."
    if not (0 <= l <=1): raise ValueError,"l (lightness) parameter must be between 0 and 1."
 
    r,b,g = (l*255,)*3
    if s!=0.0:
       if l<0.5 : var_2 = l * ( 1.0 + s )
       else     : var_2 = ( l + s ) - ( s * l )
       var_1 = 2.0 * l - var_2
       r = 255 * Hue_2_RGB( var_1, var_2, h + ( 1.0 / 3.0 ) )
       g = 255 * Hue_2_RGB( var_1, var_2, h )
       b = 255 * Hue_2_RGB( var_1, var_2, h - ( 1.0 / 3.0 ) )
 
    return (int(round(r)),int(round(g)),int(round(b)))
 
 
def RGB_to_HSL(r,g,b):
    ''' Converts RGB colorspace to HSL (Hue/Saturation/Value) colorspace.
        Formula from http://www.easyrgb.com/math.php?MATH=M18#text18
 
        Input:
            (r,g,b) (integers 0...255) : RGB values
 
        Ouput:
            (h,s,l) (floats 0...1): corresponding HSL values
 
        Example:
            >>> print RGB_to_HSL(110,82,224)
            (0.69953051643192476, 0.69607843137254899, 0.59999999999999998)
            >>> h,s,l = RGB_to_HSL(110,82,224)
            >>> print s
            0.696078431373
    '''
    if not (0 <= r <=255): raise ValueError,"r (red) parameter must be between 0 and 255."
    if not (0 <= g <=255): raise ValueError,"g (green) parameter must be between 0 and 255."
    if not (0 <= b <=255): raise ValueError,"b (blue) parameter must be between 0 and 255."
 
    var_R = r/255.0
    var_G = g/255.0
    var_B = b/255.0
 
    var_Min = min( var_R, var_G, var_B )    # Min. value of RGB
    var_Max = max( var_R, var_G, var_B )    # Max. value of RGB
    del_Max = var_Max - var_Min             # Delta RGB value
 
    l = ( var_Max + var_Min ) / 2.0
    h = 0.0
    s = 0.0
    if del_Max!=0.0:
       if l<0.5: s = del_Max / ( var_Max + var_Min )
       else:     s = del_Max / ( 2.0 - var_Max - var_Min )
       del_R = ( ( ( var_Max - var_R ) / 6.0 ) + ( del_Max / 2.0 ) ) / del_Max
       del_G = ( ( ( var_Max - var_G ) / 6.0 ) + ( del_Max / 2.0 ) ) / del_Max
       del_B = ( ( ( var_Max - var_B ) / 6.0 ) + ( del_Max / 2.0 ) ) / del_Max
       if    var_R == var_Max : h = del_B - del_G
       elif  var_G == var_Max : h = ( 1.0 / 3.0 ) + del_R - del_B
       elif  var_B == var_Max : h = ( 2.0 / 3.0 ) + del_G - del_R
       while h < 0.0: h += 1.0
       while h > 1.0: h -= 1.0
 
    return (h,s,l)

import ImageStat
import math

def pbrightness( im ):
   stat = ImageStat.Stat(im)
   r = stat.rms[0]
   g = stat.rms[1]
   b = stat.rms[2]
   return math.sqrt(0.241*(r**2) + 0.691(g**2) + 0.068*(b**2))

import ImageFilter

def medFilter(im):
    fim = im.filter(ImageFilter.MedianFilter(3))
    return fim

from numpy import *

def gaussian_grid(size = 5):
    """
    Create a square grid of integers of gaussian shape
    e.g. gaussian_grid() returns
    array([[ 1,  4,  7,  4,  1],
           [ 4, 20, 33, 20,  4],
           [ 7, 33, 55, 33,  7],
           [ 4, 20, 33, 20,  4],
           [ 1,  4,  7,  4,  1]])
    """
    m = size/2
    n = m+1  # remember python is 'upto' n in the range below
    x, y = mgrid[-m:n,-m:n]
    # multiply by a factor to get 1 in the corner of the grid
    # ie for a 5x5 grid   fac*exp(-0.5*(2**2 + 2**2)) = 1
    fac = 10
    #exp(m**2)
    #g = fac*exp(-0.5*(x**2 + y**2))
    g = fac*(x**4+y**4)
    return g.round().astype(int)

class GAUSSIAN(ImageFilter.BuiltinFilter):
    name = "Gaussian"
    gg = gaussian_grid().flatten().tolist()
    filterargs = (5,5), sum(gg), 0, tuple(gg)

def filterg(im):
    im1 = im.filter(GAUSSIAN)
    for x in range(40):
        im1 = im1.filter(GAUSSIAN)
    return im1

