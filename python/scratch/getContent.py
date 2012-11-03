from pyccn import CCN,Name,Interest,ContentObject,Key,Closure

class basicPut():

	n = Name.Name()
	i = Interest.Interest()

	def __init__(self):
		self.handle = CCN.CCN()
		#n = Name.Name("ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/")
		#self.n = Name.Name("ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/")
		self.n = Name.Name("ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/kitchen/rgb-8bit-hex/5444a6")
		self.i = Interest.Interest()
		#self.key = Key.Key()

	def send(self):
		for i in ['one','two','thre']:
			print("\nInterest espressing for "+str(self.n))
			co = self.handle.get(self.n,self.i,200)
			if not not co: 
				#if co is not empty,  print result for debugging
				print "interest sent & CO received for "+str(co.name)
				print co.content

if __name__ == '__main__':
	
	put = basicPut();
	put.send()
