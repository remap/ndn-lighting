import SocketServer, sys
from ArtNetPacket import ArtNetPacket            
from time import strftime, time
import pyccn
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,_pyccn, NameCrypto


#@staticmethod

global lastr, lastg, lastb
lastr=0
lastg=0
lastb=0


class ArtNetHandler(SocketServer.BaseRequestHandler):

	#def __init__(self, request, client_address, server):
	#	self.hello = "HI!"
	#	dict.__init__(cls)
	
	#def setup(self):
	
	def handle(self):
		global lastr, lastg, lastb
		packet = ArtNetPacket.from_buffer(bytearray(self.request[0])) 
		# make sure packet is coming from TV1      
		if (self.client_address[0] == "131.179.141.17"):
			print packet.payload
			if (packet.payload[9]!=lastr or packet.payload[10]!=lastg or packet.payload[11]!=lastb):
				T = strftime("%Y-%m-%d %H:%M:%S")
				print T		
				print "rgb : ",packet.payload[9],packet.payload[10],packet.payload[11]
				seq.sendAllLights(packet.payload[9],packet.payload[10],packet.payload[11])
	      		lastr = packet.payload[9]
	    		lastg = packet.payload[10]
	    		lastb = packet.payload[11]  		
	    		#send interest if any RGB is more than Zero
	        	#if((int(packet.payload[9])+int(packet.payload[10])+int(packet.payload[11]))>0):
	    		
	    	
	    	#self.finish()
	        #

class sequencer(Closure):

	#ensure singleton
	_instance = None
	def __new__(cls, *args, **kwargs):
		if not cls._instance:
			cls._instance = super(sequencer, cls).__new__(cls, *args, **kwargs)
		return cls._instance

	def __init__(self, configFileName):
		self.appConfigFileName = configFileName
		self.loadConfigFile()
		self.handle = CCN()
		self.getApplicationKey()
		#nameCrypto
		self.state = NameCrypto.new_state()
		self.cryptoKey = NameCrypto.generate_application_key(self.cfg.fixtureKey, self.cfg.appName)
		
	def loadConfigFile(self):
		command = "import "+self.appConfigFileName+" as appCfg"
		exec(command)
		self.appCfg = appCfg;
		self.cfg = appCfg;

	def getApplicationKey(self):
		print("getting application key for "+self.appCfg.appName)
		key = Key()
		keyFile = self.appCfg.keyFile
		key.fromPEM(filename=keyFile)
		self.appKey = key
		self.key = key
		
	def start(self):
		print "starting "+self.cfg.appName
		#self.allWhite()
		#self.allBlack()
		#self.spazz()
		
	def spazz(self):
		for i in range(1,500000):
			self.allWhite()
			self.allBlack()
		
	def allWhite(self):
		for d in self.cfg.names:
			if d['name'] != "incandescent":
				print d['name']
				self.buildAndSendInterest(d['name'],255,255,255)
				
	def allBlack(self):
		for d in self.cfg.names:
			if d['name'] != "incandescent":
				print d['name']
				self.buildAndSendInterest(d['name'],0,0,0)
		
	def sendAllLights(self,r,g,b):
		for d in self.cfg.names:
			if d['name'] != "incandescent":
				self.buildAndSendInterest(d['name'],r,g,b)
				
		return
				
	def hx(self, inum):
		num = hex(inum)[2:]
		if (len(num) == 1):
			num = '0'+num
		#print num
		return str(num)

	def buildAndSendInterest(self,device,r,g,b):

		rx = self.hx(r)
		gx = self.hx(g)
		bx = self.hx(b)
		
		CLI = device+"/setRGB/"+rx+gx+bx
		#print CLI
		self.sendSignedInterest(CLI)

	def sendInterest(self,command):
		
		n = Name.Name(self.cfg.appPrefix)
		print("\nInterest espressing for "+str(n))
		n += command

		i = Interest.Interest()
		
		print("Interest sending to "+str(n))
		co = self.handle.get(n,i,20)
		if not not co: 
			#if co is not empty,  print result for debugging
			print("content: "+str(co.content))
		pass
			
	def sendSignedInterest(self,command):
		fullURI = self.cfg.appPrefix + command
		#print fullURI
		i = Interest()
		
		#build keyLocator to append to interest for NameCrypto on upcall
		keyLoc = pyccn.KeyLocator(self.key)
		keyLocStr = _pyccn.dump_charbuf(keyLoc.ccn_data)
		nameAndKeyLoc = Name(str(fullURI))
		#print("there are "+str(len(nameAndKeyLoc))+" components")
		nameAndKeyLoc += keyLocStr
		#print("there are "+str(len(nameAndKeyLoc))+" components after adding keyLocStr")
		
		t0 = time()
		authName = NameCrypto.authenticate_command(self.state, nameAndKeyLoc, self.cfg.appName, self.cryptoKey)
		t1 = time()
		
		#print "elapsed sign", t1-t0
		#print authName.components
		
		t0 = time()
		co = self.handle.expressInterest(authName,self)
		t1 = time()
		#print "elapsed get", t1-t0
		#if not not co: 
			#if co is not empty,  print result for debugging
		#	print("interest "+str(co.content))
		# for profiling - quit when the controller quits
		#else:
		#	print "it's done"
		#	sys.exit(1)
		#return

if __name__ == "__main__":
    if  len(sys.argv) > 1:
        HOST = sys.argv[1]
    else:
	HOST = "" # listen on all interfaces
	#sys.exit(1)
    server = SocketServer.UDPServer((HOST, ArtNetPacket.PORT), ArtNetHandler)
    print "Starting socket server on 0x%0X" % ArtNetPacket.PORT
    seq = sequencer("faderboard_cfg")
    server.serve_forever()
    
