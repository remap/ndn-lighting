import SocketServer, sys
from ArtNetPacket import ArtNetPacket            
from time import strftime
        
class ArtNetHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        packet = ArtNetPacket.from_buffer(bytearray(self.request[0]))       
       
       	if (self.client_address[0] == "131.179.141.17"):
	        # pretty print received packet
	        T = strftime("%Y-%m-%d %H:%M:%S")
	        print "%s Received from %s:%i %s \n" % (T, self.client_address[0], self.client_address[1], packet)
	        #print "rgb : ",packet.payload[9],packet.payload[10],packet.payload[11]
	        
        
        
	# example channel access
        # print "channel 12 = %d \n" % packet.payload[11]

	# should probably not perform an action without verifying the IP
	# of the sender to be authorize (e.g., the 131.179.141.17 
	# of the light controller board)


if __name__ == "__main__":
    if  len(sys.argv) > 1:
        HOST = sys.argv[1]
    else:
	HOST = "" # listen on all interfaces
	#sys.exit(1)
    server = SocketServer.UDPServer((HOST, ArtNetPacket.PORT), ArtNetHandler)
    print "Starting socket server on 0x%0X" % ArtNetPacket.PORT
    server.serve_forever()
    
