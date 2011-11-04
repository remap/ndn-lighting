from ctypes import *

class ArtNetPacket (LittleEndianStructure):
    PORT=0x1936
    _fields_ = [("id", c_char * 8), 
                ("opcode", c_ushort), 
                ("protverh", c_ubyte), 
                ("protver", c_ubyte),
                ("sequence", c_ubyte),          # Currently not supported, set to zero
                ("physical", c_ubyte),          # Physical input, unused
                ("universe", c_ushort), 
                ("lengthhi", c_ubyte),
                ("length", c_ubyte), 
                ("payload", c_ubyte * 512)]
    def __str__(self):
        return "{id:%s, opcode:0x%0X, protverh:0x%0X, protver:0x%0X, sequence:0x%0X, physical:0x%0X, universe:0x%0X, lengthhi:0x%0X, length:0x%0X, payload:%s}" % (self.id, self.opcode, self.protverh, self.protver, self.sequence, self.physical, self.universe, self.lengthhi, self.length, [i for i in self.payload])

# ArtNet 2 basic output protocol           
class ArtNetDMXOut(ArtNetPacket):     
    def __init__(self, universe): 
        self.id = "Art-Net"
        self.opcode = 0x5000  # OpOutput
        self.protver = 14
        self.universe = universe
        self.lengthhi = 2   # 512, send whole thing
           