import sys, socket, math, time
from ArtNetPacket import ArtNetPacket, ArtNetDMXOut 
    
# In this example, specify an IP address on the command line
# Start and end values, and an optional delay
# and the routine will fade from start to end 
#
def main(): 
    if len(sys.argv) > 2:
        hostIP = sys.argv[1]   # target IP
        start = int(sys.argv[2])
        
    else:                                                   
        print "ARGS: <target_IP> <start_value>  [<delay_ms>]"
        sys.exit(1)
    if len(sys.argv) > 3:
        delay_sec = int(sys.argv[3])/1000.0
    else:
        delay_sec = 0.05  # 50 ms
    
    S = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    packet = ArtNetDMXOut(0)   # universe 1 in this example
    
    # DO NOT BOUNCE INCANDESCENT FIXTURES FROM 0 to 100% WHEN THEY ARE COLD!
    #
 
    print "Setting intensity of all channels to %i " % start

    while(True):
        time.sleep (delay_sec)
        for ch in range(0,512):    # all DMX channels 
            packet.payload[ch] = start
        S.sendto(packet, (hostIP, ArtNetDMXOut.PORT));
    S.close

if __name__ == "__main__":
    main()
    
