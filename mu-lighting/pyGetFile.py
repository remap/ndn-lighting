#!/usr/bin/env python
#coding=utf-8

# Put file into repo-ng in Python, following repo protocol:
# http://redmine.named-data.net/projects/repo-ng/wiki
# Under testing: unversioned segmented data writing completed
#
# code by Mengchen, Zhehao
# Aug 21, 2014
#

import sys
import time
import trollius as asyncio
import logging

from pyndn import Name, Data, Interest, ThreadsafeFace, Exclude, Blob
from pyndn.security import KeyChain

class GetFile:

    def __init__(self, repoCommandPrefix, dataName, songHandlerFace, playFunction, *args):
        
        self._repoCommandPrefix = Name(repoCommandPrefix)       
        self._dataName = Name(dataName)

        self._interestLifetime = 1000
        self.m_timeout = 100
        self.m_nextSegment=0
        self.m_totalSize=0
        self.m_retryCount=0

        self.m_mustBeFresh = False

        self._face = songHandlerFace
        
        self._keyChain = KeyChain()
        self._certificateName = self._keyChain.getDefaultCertificateName()
        
        self.oStream = None
        self._writtenSegment = 0
        self._playFunction = playFunction
        self._args = args

        self.zero = None

	self.count = 0
        
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

    def start(self):
        self.zero = GetFile.componentFromSingleDigitNumberWithMarkerCXX(0, 0x00)
        self.startFetchingData(Name(self._dataName).append(self.zero))
          
    def stop(self):
        sys.exit(1)

    def startFetchingData(self,name):
        interest = Interest(name)
        interest.setInterestLifetimeMilliseconds(self._interestLifetime)
        interest.setMustBeFresh(self.m_mustBeFresh)      
        self._face.expressInterest(interest, self.onFirstData, self.onTimeout)
        

    def fetchData(self,name):
        interest = Interest(name)
        interest.setInterestLifetimeMilliseconds(self._interestLifetime)
        interest.setMustBeFresh(self.m_mustBeFresh)
        
        self._face.expressInterest(interest,self.onData,self.onTimeout)
        
    def onData(self,interest,data):
        #print "receive data from repo"
        dataName = data.getName()
        #print dataName.toUri()
        
        if(dataName.size() != self._dataName.size() + 1):
            raise Exception("unexpected data name size.")
        segment = dataName[-1].toSegment()

        content = data.getContent()
        
        # Write the correct segment; if segments arrive in wrong order, let the user know
        if self._writtenSegment + 1 == segment:
            self.oStream.write(content.toBuffer())
            self._writtenSegment += 1
        else:
            print "Segment arrived in wrong order."
        self.m_totalSize += content.size()
        
        # finalBlockId does not work well with repo-ng for now
        finalBlockId = data.getMetaInfo().getFinalBlockID()
        
        # These comparisons are apparently wrong
	if self.count >1000:
        	print "finalBlockID,Name[-1]",finalBlockId.toEscapedString(), dataName[-1].toSegment()
		self.count -= 1000
	self.count +=1
        if (finalBlockId == dataName[-1].toSegment()):
            # Reach EOF
            print "INFO: End of file is reached."
            print "INFO: Total",(self.m_nextSegment - 1)," of segments received"
            print "INFO: Total",(self.m_totalSize)," bytes of content received"

        else:
            #Reset retry counter
            self.m_retryCount = 0
            #Fetch next segment
            self.m_nextSegment += 1
            self.fetchData(Name(self._dataName).appendSegment(self.m_nextSegment))
            
    #In PyNDN the first interest's name has a different segment number with other(First:%00,others:%00%xx)            
    def onFirstData(self,interest,data):
        #print "receive first data from repo"
        dataName = data.getName()
        if(dataName.size() != self._dataName.size() + 1):
            raise Exception("unexpected data name size.")
        
        content = data.getContent()
        self.oStream.write(content.toBuffer())
        
        self.m_totalSize += content.size()
        
        finalBlockId = data.getMetaInfo().getFinalBlockID()
        
        print "finalBlockID,Name[-1]",finalBlockId.toEscapedString(), dataName[-1].toSegment()
        if (finalBlockId == dataName[-1].toSegment()):
            # Reach EOF
            print "INFO: End of file is reached."
            print "INFO: Total",(self.m_nextSegment - 1)," of segments received"
            print "INFO: Total",(self.m_totalSize)," bytes of content received"

        else:
            #Reset retry counter
            self.m_retryCount = 0
            self.m_nextSegment += 1
        
        # fetch regular data after receiving the first segment
        self.fetchData(Name(self._dataName).appendSegment(self.m_nextSegment))
        
    def onTimeout(self,interest):
        MAX_RETRY = 1  
        self.m_retryCount += 1
        if(self.m_retryCount <= MAX_RETRY):
            self.fetchData(interest.getName())
            print"TIMEOUT: retransmit interest..."
        else:
            print "TIMEOUT: last interest sent for segment",(self.m_nextSegment - 1)
            print "TIMEOUT: abort fetching after",(MAX_RETRY),"times of retry"
            # Probably want to close file after it finished writing
            self.oStream.close()
            self._playFunction(*self._args)
	 
def callbackFunc(repoCommandPrefix, dataName, face, callback):
    print "callback called"
    g1 = GetFile(repoCommandPrefix, dataName, face, callback)
    
    musicOutputFile = "PC3-received-2.txt"
    g1.oStream = open(musicOutputFile, 'wb')
    g1.start()

def callback2():
    print "it works again"

if __name__ == '__main__':
    logging.basicConfig()
    #songList = ['Hotel California', 'river flows in you', 'simple way', 'star', 'mozart', 'yellow', 'canon', 'a comm amour', 'RICHA', 'merry christmas', 'love story', 'juliet', 'geqian', 'nocturne', 'RICHA1111', 'canon1', 'nocturne1', 'house_lo', 'house_lo']
    #for i in songList:
    dataName = "/clear"
    repoCommandPrefix = "/example/repo/1"
    
    loop = asyncio.get_event_loop()
    face = ThreadsafeFace(loop, "")
    keyChain = KeyChain()
    certificateName = keyChain.getDefaultCertificateName()
    face.setCommandSigningInfo(keyChain, certificateName)
        
    g = GetFile(repoCommandPrefix, dataName, face, callbackFunc(repoCommandPrefix, "/clear-f", face, callback2))
    
    musicOutputFile = "PC3-received.txt"
    g.oStream = open(musicOutputFile, 'wb')
    g.start()
    
    
    print "it works"
    loop.run_forever()
