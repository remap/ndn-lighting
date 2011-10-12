from pyccn import CCN,Name,Interest,ContentObject,Key,Closure

class basicPut():

	def __init__(self):
		self.handle = CCN.CCN()
		#self.key = Key.Key()

	def send(self):
		n = Name.Name("ccnx:/ndn/ucla.edu/apps/lighting/TV1/fixture/kitchen/rgb-8bit-hex/090909")
		i = Interest.Interest()
		print("Interest sending to "+str(n))
		co = self.handle.get(n,i,2000)
		if not not co: 
			#if co is not empty,  print result for debugging
			print "interest sent & CO received for "+str(co.name)

if __name__ == '__main__':
	
	put = basicPut();
	put.send()
