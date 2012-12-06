import sys
import pyccn
from pyccn import Key as Key
from pyccn import Interest
from pyccn import Name
from pyccn import ContentObject
from pyccn import Closure
import ssl
#from time import time
import time

# UDP client
import socket

# profiler
import cProfile

#logging
import logging, logging.handlers

#lighting driver
import dmx_serial as dmx

# controller
# fixture application that receives interests & controls lights

class controller(pyccn.Closure):

    def __init__(self, configFileName):
        self.appConfigFileName = configFileName
        self.loadConfigFile()
        self.handle = pyccn.CCN()
        self.getApplicationKey()
        self.iFlex_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.send = False
        self.count = 0

    def __del__(self):
        self.iflex_socket.close()

    def loadConfigFile(self):
        command = "import "+self.appConfigFileName+" as appCfg"
        exec(command)
        self.appCfg = appCfg;
        self.cfg = appCfg;

    def getApplicationKey(self):
        key = Key()
        keyFile = self.appCfg.keyFile
        key.fromPEM(filename=keyFile)
        self.appKey = key

    def listAppNameSpace(self):
        for name in self.appCfg.controlNameSpace:
            print name

    def start(self):
        print "starting "+self.appCfg.appName
        self.key = self.appKey
        self.keyLocator = pyccn.KeyLocator(self.key)
        self.URI = self.appCfg.appPrefix
        self.name = Name(self.appCfg.appPrefix)
        print "controller init & listening within "+str(self.name)
        self.listen()

    def listen(self):
        #listen to requests in namespace
        print "key server thread, listening for "+self.URI
        self.co = self.makeDefaultContent(self.URI, "default Content")
        self.handle.setInterestFilter(Name(self.URI), self)
        self.handle.run(self.cfg.runtimeDuration)

    def makeDefaultContent(self, name, content):
        co = ContentObject()
        # since they want us to use versions and segments append those to our name
        co.name = Name(name) # making copy, so any changes to co.name won't change self.name
        co.name.appendVersion() # timestamp which is our version
        co.name.append(b'\x00') # first segment
        co.content = content #"LIGHT OK"
        si = pyccn.SignedInfo()
        si.publisherPublicKeyDigest = self.key.publicKeyID
        si.type = 0x28463F # key type
        #si.type = 0x0C04C0 # content type
        si.finalBlockID = b'\x00' # no more segments available
        si.keyLocator = self.keyLocator
        co.signedInfo = si
        co.sign(self.key)
        return co

    def upcall(self, kind, info):

        #print self.appCfg.appName +" upcall..."
        if kind != pyccn.UPCALL_INTEREST:
            return pyccn.RESULT_OK

        #ignore timeouts
        if kind == pyccn.UPCALL_INTEREST_TIMED_OUT:
            return pyccn.RESULT_OK

        print "received interest "+str(info.Interest.name)
        #print info.Interest.name.components
        #print "interest has "+str(len(info.Interest.name))+" components"
        #self.state = NameCrypto.new_state()

    # note interest should be verified before any side-effect (control) is performed.
    # we're skipping all trust management & verification in this example

        n = info.Interest.name

        self.parseAndSendToDMXLight(info.Interest.name)

        return pyccn.RESULT_INTEREST_CONSUMED

    def parseAndSendToDMXLight(self,name):
        
        iMax = len(name)
        rgbVal = str(name[iMax-(self.appCfg.deviceNameDepth-2)])
        command = str(name[iMax-(self.appCfg.deviceNameDepth-1)])

        print "parse and send to DMX "+str(rgbVal)+" command: "+command

        # note we're ignoring the light name (and type of light)
        # original app changed to the correct driver (artnet, kinet - not yet DMX) per light name per table in config
        # as this is the basic example, we're just using local serial DMX directly.

        if(command=="setRGB"):

            r = int(rgbVal[0:2],16)
            g = int(rgbVal[2:4],16)
            b = int(rgbVal[4:6],16)

        dmx.serialsender.int_data[2] = r
        dmx.serialsender.int_data[3] = g
        dmx.serialsender.int_data[4] = b
        time.sleep(0.1)


def usage():
    print("Usage: %s <Application configFileName>" % sys.argv[0])
    sys.exit(1)

if __name__ == '__main__':
    if (len(sys.argv) != 2):
        usage()

    runtime = controller(sys.argv[1])
    runtime.start()
