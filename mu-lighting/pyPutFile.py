# Put file into repo-ng in Python, following repo protocol:
# http://redmine.named-data.net/projects/repo-ng/wiki
# Under testing: unversioned segmented data writing completed
#
# code by Mengchen, Zhehao
# Aug 21, 2014
#

import sys
import time
import os
import math

from pyndn import Name, Data, Interest, ThreadsafeFace, Exclude, Sha256WithRsaSignature
from pyndn.security import KeyChain
from pyndn.util import Blob

# For Py2.7
import trollius as asyncio
import logging

from CommandParameters import RepoCommandParameter

# This wrapper for ThreadsafeFace may facilitate the process of 
# putting multiple files. Though it may cause more initial timeouts, 
# but shouldn't matter in the long run.

class ThreadsafeFaceWrapper(object):
    def __init__(self):
        self._loop = asyncio.get_event_loop()
        self._face = ThreadsafeFace(self._loop, "")
        self._keyChain = KeyChain()
        self._certificateName = self._keyChain.getDefaultCertificateName()
        self._face.setCommandSigningInfo(self._keyChain, self._certificateName)
	
        
    def startProcessing(self):
        try:
            self._loop.run_forever()        
        finally:
            self.stop()
        
    def stopProcessing(self):
        self._loop.close()
        self._face.shutdown()
        self._face = None
        sys.exit(1)
    
    def getFace(self):
        return self._face
        
    def getLoop(self):
        return self._loop
        
class PutFile:
    def __init__(self, repoDataPrefix, repoCommandPrefix, face, loop):
        self._repoCommandPrefix = repoCommandPrefix
        
        self._dataName = Name(repoDataPrefix)
        self._nDataToPrepare = 10
        self._interestLifetime = 4000
        
        self._mData = []
        self._face = face
        self._loop = loop
        
        self._keyChain = KeyChain()
        self._certificateName = self._keyChain.getDefaultCertificateName()

        self._currentSegmentNo = 0
        self._isFinished = False
        self.insertStream = None
        self.fileSize = 0
        
        self._segmentSize = 1000
	self._endBlockId = 0

	self._count = 0

    def start(self):
        self._face.registerPrefix(self._dataName,self.onInterest,self.onRegisterFailed)
        self.startInsertCommand();
          
    # Just for appending segment number 0, making it encoded as %00%00, instead of %00 in PyNDN.
    # Potential bug/loss of interoperability?
    @staticmethod
    def componentFromSingleDigitNumberWithMarkerCXX(number, marker):
        value = []
        value.append(number & 0xFF)
        number >>= 8
        value.reverse()
        value.insert(0, marker)
        return Name.Component(Blob(value, False))

    def prepareNextData(self,referenceSegmentNo):
        if (self._isFinished):
            return
        if  self._mData:
            maxSegmentNo = len(self._mData)
            if(maxSegmentNo - referenceSegmentNo >= self._nDataToPrepare):
                return            
            self._nDataToPrepare -= (maxSegmentNo - referenceSegmentNo)
        
        # Prepare _nDataToPrepare number of data segments, and insert them into _mData
        for i in range(0,self._nDataToPrepare):
            buffer = self.insertStream.read(self._segmentSize)
            
            if not buffer:
                self._isFinished = True
                # For now, repo-ng cannot handle FinalBlockID: 
                # ERROR: Invalid length for nonNegativeInteger (only 1, 2, 4, and 8 are allowed)
                #d.getMetaInfo().setFinalBlockID(self._currentSegmentNo)
                
                # using EndBlockId in repo protocol spec instead
                break
            
            if self._currentSegmentNo == 0:
                dataName = Name(self._dataName).append(PutFile.componentFromSingleDigitNumberWithMarkerCXX(0, 0x00))
            else:
                dataName = Name(self._dataName).appendSegment(self._currentSegmentNo)
            d = Data(dataName)
            
            # Biggest mistake: wrong data name. Still finding out why, though
            # Original one:
            #d = Data(Name(self._dataName).append(self._currentSegmentNo))
            
            print "Given data name", d.getName().toUri(), " Segment no", self._currentSegmentNo
            
            d.setContent(buffer)
            self._keyChain.sign(d, self._certificateName)
            
            self._mData.append(d)

            self._currentSegmentNo += 1
            

    def startInsertCommand(self):
        parameters = RepoCommandParameter()
        parameters.setName(self._dataName)
        parameters.setStartBlockId(0)
        
        # Adding endBlockID so that repo does not crash on putfile
        # should be replaced by FinalBlockID in data.metainfo, once that's fixed
        parameters.setEndBlockId(math.trunc(self.fileSize/self._segmentSize))
        #print (math.trunc(self.fileSize / self._segmentSize))
	self._endBlockId = math.trunc(self.fileSize / self._segmentSize)
        
        commandInterest = self.generateCommandInterest(self._repoCommandPrefix, "insert", parameters)
        self._face.makeCommandInterest(commandInterest)
        #print commandInterest.getName().toUri()
        
        self._face.expressInterest(commandInterest,self.onInsertCommandResponse, self.onInsertCommandTimeout)

    def onInsertCommandResponse(self,interest,data):
        #print "receive the data of command interest"
        pass
        
    def onInsertCommandTimeout(self,interest):
        raise Exception("command response timeout")
        sys.exit(1)

    def onInterest(self,prefix,interest,transport,registeredPrefixId):  
        #print "enter the onInterest"    
        if (interest.getName().size() != prefix.size() + 1):
            print "Unrecognized Interest"
            sys.exit(1)
	if self._count >1000:
        	print interest.getName().toUri()
		self._count -=1000
	self._count +=1
        segmentComponent = interest.getName().get(prefix.size())
        segmentNo = segmentComponent.toSegment()
        
        self.prepareNextData(segmentNo)
        
        # this could be slightly inferior to the logic in repo-ng-cxx test
        if (segmentNo >= self._currentSegmentNo):
            print "requested segment does not exist, or is not prepared"
            return
            #sys.exit(1)

	
	
        
        item = self._mData[segmentNo]
        
        encodedData = item.wireEncode()
        transport.send(encodedData.toBuffer())
	'''if (segmentNo == self.fileSize):
		time.sleep()'''
		
        
        # TODO: if it's the last segment, call stop after a safe period of time...
        # Or should send check after a safe amount of time, and based on the results 
        # of check, call stop.
        
        
    def onRegisterFailed(self, prefix):
        self.log.error("Could not register " + prefix.toUri())
        sys.exit(1)

    def stop(self):
        self.insertStream.close()
    
    def generateCommandInterest(self, commandPrefix, command, commandParameter):     
        interest = Interest(commandPrefix.append(command).append(commandParameter.wireEncode()))
        interest.setInterestLifetimeMilliseconds(self._interestLifetime)
        return interest
    
if __name__ == '__main__':
    logging.basicConfig()
    musicFile = "star-o.txt"
    
    faceWrapper = ThreadsafeFaceWrapper()
    
    p = PutFile(Name("star-o"), Name("ndn:/example/repo/1"), faceWrapper.getFace(), faceWrapper.getLoop())
    p.insertStream = open(musicFile, 'rb')
    p.fileSize = os.stat(musicFile).st_size
    
    p.start()
    
    # Always call this when everything else is done, since it blocks in run_forever
    faceWrapper.startProcessing()
