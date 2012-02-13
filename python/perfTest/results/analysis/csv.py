

#mongo - nice, but no access right now
#everything = data.getAllLogs()
#print("there are "+str(everything.count())+" entries")

def open(f):
	print "analyzing data from "+f
	ifile  = open(f, "rb")
	reader = csv.reader(ifile)
	
def close():
	self.ifile.close()
	
def dump():
	rownum = 0
	for row in reader:
	    # Save header row.
	    if rownum == 0:
	        header = row
	    else:
	        colnum = 0
	        for col in row:
	            print '%-8s: %s' % (header[colnum], col)
	            colnum += 1
            
	    rownum += 1