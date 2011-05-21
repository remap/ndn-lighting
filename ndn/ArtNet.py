import sys, socket, math, time
from ctypes import *
 
# ArtNet 2 basic output protocol
class ArtNetDMXOut(LittleEndianStructure):
    PORT = 0x1936
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
    def __init__(self, universe):
        self.id = "Art-Net"
        self.opcode = 0x5000  # OpOutput
        self.protver = 14
        self.universe = universe
        self.lengthhi = 2   # 512, send whole thing
   
    
# In this example, specify an IP address on the command line
# And it will run a simple fade of all DMX channels.
#
def main():
    recvPort = 50010  # port that receives the parsed interest command
    if len(sys.argv) > 1:
	recvIP = sys.argv[1]
	hostIP = sys.argv[2]
    else:
	print "provide controller and target IP as arguments"
	sys.exit(1)
		
    recvS = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    recvS.bind((recvIP, recvPort))
#    recvS.settimeout(2.0)
#    try:
    while(1):
        print("*************** before data received... ******");
        data, recvaddr = recvS.recvfrom(1024)
        print("*************** data received... *************");
    #except recvS.timeout, t:
#	pass
#    else:
	#if not data:
        #    print ("*********** Client closed connection. ***");
	#    break
        datafields = data.strip().split('|')
	# if ch != '*', then control single ArtNet lighting fixture
        if (datafields[0] != '*'):
            ch = int(datafields[0])
        else:
            ch = datafields[0]
            d = int(datafields[1])

	print ch
	print d
	S = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        packet = ArtNetDMXOut(0)   # universe 1 in this example
   
    # DO NOT BOUNCE THESE FIXTURES FROM 0 to 100% WHEN THEY ARE COLD!
    #
    #while(True):
        #for d in range(0,217):   # fade channels from 0 to 85%
        #    print d
        #    time.sleep (.075)    # roughly 75ms per step, should be log fade, though
        #    for ch in range(0,511):    # all DMX channels
        #        packet.payload[ch] = d
        #    S.sendto(packet, (hostIP, ArtNetDMXOut.PORT));
		
        print ch
        print d
    #while(True):
	if (ch == '*'):
	    for ch in range(0, 511):
		packet.payload[ch] = d
	    else:
		packet.payload[ch] = d  # Here, we remain the 'other channels previous intensity
        S.sendto(packet, (hostIP, ArtNetDMXOut.PORT));
	
        S.close
 
if __name__ == "__main__":
    main()
