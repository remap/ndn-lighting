log=[]

def trace(time, name, data):
	
	l = time+","+name+","+data
	log.append(l)


def writeOut(name):
	print("writing "+name+".log")
	filename = name+".log"
	FILE = open(filename,"w")
	for val in log:
		#print val
		FILE.write(str(val)+"\n")
	