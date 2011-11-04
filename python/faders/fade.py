import sys, socket, math, time
from ArtNetPacket import ArtNetPacket, ArtNetDMXOut 
    
# In this example, specify an IP address on the command line
# Start and end values, and an optional delay
# and the routine will fade from start to end 
#
def main(): 
    if len(sys.argv) > 3:
        hostIP = sys.argv[1]   # target IP
        start = int(sys.argv[2])
        end = int(sys.argv[3])
        
    else:                                                   
        print "ARGS: <target_IP> <start_value> <end_value> [<delay_ms>]"
        sys.exit(1)
    if len(sys.argv) > 4:
        delay_sec = int(sys.argv[4])/1000.0
    else:
        delay_sec = 0.05  # 50 ms
    
    S = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    packet = ArtNetDMXOut(0)   # universe 1 in this example
    
    # DO NOT BOUNCE INCANDESCENT FIXTURES FROM 0 to 100% WHEN THEY ARE COLD!
    #
    if (end-start) > 0: 
        dir = 1
    else:
        dir = -1
    
    for d in range(start,end+dir,dir):   # fade channels from 0 to 85%
        print "Setting intensity of all channels to %i " % d
        time.sleep (delay_sec)
        for ch in range(0,512):    # all DMX channels 
            packet.payload[ch] = d
        S.sendto(packet, (hostIP, ArtNetDMXOut.PORT));
    S.close

if __name__ == "__main__":
    main()
    
