import ImageFilter

def medFilter(im):
    fim = im.filter(ImageFilter.MedianFilter(9))
    return fim 
