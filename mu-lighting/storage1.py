#!/usr/bin/env python
#coding=utf-8

# scan file, find music data and insert them to repo
#
# code by Mengchen
# Aug 21, 2014
#


import sys 
import os
import thread
import threading
import time
import logging

from pyndn import Name,Interest,ThreadsafeFace,Sha256WithRsaSignature
from pyndn import Data
from pyndn import Face
from pyndn.security import KeyChain
from pyPutFile import ThreadsafeFaceWrapper,PutFile
import trollius as asyncio

currentList=[]

#reply the song list when receive the list collection request from controller
class RegisterSongList(object):


    def __init__(self,sFace,sLoop,skeychain,scertificateName,prefix="/ndn/edu/ucla/remap/music/list"):
	logging.basicConfig()        
    	self.device = "PC1"
    	self.deviceComponent = Name.Component(self.device)
	self.excludeDevice = None
        self.listPrefix = Name(prefix)
        self.address = ""
        self._isStopped = True

	self.face = sFace
	self.loop = sLoop
	self.keychain = skeychain
	self.certificateName = scertificateName
    
    def start(self):
        self.face.registerPrefix(self.listPrefix,self.onInterest,self.onRegisterFailed)

    def stop(self):
        self.loop.close()       
        self.face.shutdown()
        self.face = None
        sys.exit(1)

    def signData(self,data):
	data.setSignature(Sha256WithRsaSignature())


    def onInterest(self, prefix, interest, transport, registeredPrefixId):
        initInterest = Name(interest.getName())
        print "interest name:",initInterest.toUri()
	d = Data(interest.getName().append(self.deviceComponent))
	
		  
        try:
	    if(initInterest == self.listPrefix):
	    
		#receive the .../list interest
		currentString = ','.join(currentList)
		d.setContent(currentString)
		encodedData = d.wireEncode()
		transport.send(encodedData.toBuffer())
		print d.getName().toUri()
		print d.getContent()
		
            else:
		self.excludeDevice = initInterest.get(self.listPrefix.size())
		excDevice = self.excludeDevice.toEscapedString()
		if(excDevice != str("exc")+self.device):
			# receive the .../list/excOther interest                
			currentString = ','.join(currentList)
			d.setContent(currentString)
			encodedData = d.wireEncode()
			transport.send(encodedData.toBuffer())
			print d.getName().toUri()
			print d.getContent()
			
		else:
			# receive the .../list/excMe interest
			print"controller has exclude me, I have to remove register!"
                	self.face.removeRegisteredPrefix(registeredPrefixId)
			time.sleep(30)
			print"register again"
                	self.face.registerPrefix(self.listPrefix,self.onInterest,self.onRegisterFailed)
    

	except KeyboardInterrupt:
		print "key interrupt"
		sys.exit(1)
	except Exception as e:
		print e
		d.setContent("Bad command\n")
	finally:
		self.keychain.sign(d,self.certificateName)
		


    def onRegisterFailed(self, prefix):
        self.log.error("Could not register " + prefix.toUri())
        self.stop()	

class CheckList(object):

    listDelete = []
    listAdd = []
    @staticmethod
	#scan the file and get the most updated song list
    def scan_files(directory,fprefix=None,fpostfix=None):
        files_list=[]
     
        for root, sub_dirs, files in os.walk(directory):
            for special_file in files:
                if fpostfix:
                    if special_file.endswith(fpostfix):
                        files_list.append(os.path.join(special_file))
                elif fprefix:
                    if special_file.startswith(fprefix):
                        files_list.append(os.path.join(special_file))
                else:
                    files_list.append(os.path.join(special_file))
                           
        return files_list
   
    @staticmethod
	#detect if the song list has been updated
    def list_check(currentList):
        setA=set(currentList)
        while True:
        	alist = CheckList.scan_files("/home/bupt/ndn-test/",fpostfix=".mp3")
        	tempList =[] 
        	for p in alist:
            		s = p[:-4]  
            		tempList.append(s)
        	setB = set(tempList)
       		listDelete = setA.difference(setB)
        	listAdd = setB.difference(setA)
		currentList = tempList
        	time.sleep(600)


if __name__ == '__main__':
    logging.basicConfig()
    #get the initial version of song list
    alist = CheckList.scan_files("/home/bupt/ndn-test/mu-lighting/music-file",fpostfix=".mp3")
    for p in alist:
    	s = p[:-4]  
    	currentList.append(s)
    print currentList

    skeychain = KeyChain()
    scertificateName = skeychain.getDefaultCertificateName()
    sLoop = asyncio.get_event_loop()
    sFace = ThreadsafeFace(sLoop,"")
    sFace.setCommandSigningInfo(skeychain,scertificateName)

    
    #put the local file(music data, onset data and frequency data) to repo
    faceWrapper = ThreadsafeFaceWrapper()
    for i in currentList:
	musicFile = i+str(".mp3")
	p = PutFile(Name(i), Name("ndn:/example/repo/1"), sFace,sLoop)
    	p.insertStream = open(str("music-file/")+musicFile, 'rb')
    	p.fileSize = os.stat(str("music-file/")+musicFile).st_size    
    	p.start()
	
	

	osFile = i+str("-o.txt")
	of = PutFile(Name(i+str("-o")), Name("ndn:/example/repo/1"), sFace,sLoop)
    	of.insertStream = open(str("music-file/")+osFile, 'rb')
    	of.fileSize = os.stat(str("music-file/")+osFile).st_size    
    	of.start()

	freqFile = i+str(".txt")
	fq = PutFile(Name(i+str("-f")), Name("ndn:/example/repo/1"),sFace,sLoop)
    	fq.insertStream = open(str("music-file/")+freqFile, 'rb')

    	fq.fileSize = os.stat(str("music-file/")+freqFile).st_size    
    	fq.start()


    reg = RegisterSongList(sFace,sLoop,skeychain,scertificateName,prefix="/ndn/broadcast/mulighting/list")
  
    

    #keep updating the temporary list
    print "threading before"
    t = threading.Thread(target = CheckList.list_check, args = (currentList,))
    t.daemon = True
    t.start()
    reg.start()



	 # Always call this when everything else is done, since it blocks in run_forever
    faceWrapper.startProcessing()

