from lighting.KinetSender import KinetSender
import logging
import time
import sys
import datetime


from pyndn import Name, Interest, Data, ThreadsafeFace
from pyndn import Sha256WithRsaSignature
from pyndn.security import KeyChain

from pyndn.encoding import ProtobufTlv
from lighting.light_command_pb2 import LightCommandMessage
from ConfigParser import RawConfigParser

try:
    import asyncio
except ImportError:
    import trollius as asyncio

class LightController():
    shouldSign = False
    COLORS_PER_LIGHT = 3
    STRAND_SIZE = 50
    #mengchen: let's make the lighting lits only a half every time
    #HALF_STRAND_SIZE = 25
    def __init__(self, nStrands=1, myIP="192.168.1.1", lightIP="192.168.1.50", prefix="/testlight"):
        self.log = logging.getLogger("LightController")
        self.log.setLevel(logging.DEBUG)
        sh = logging.StreamHandler()
        sh.setLevel(logging.DEBUG)
        self.log.addHandler(sh)
        fh = logging.FileHandler("LightController.log")
        fh.setLevel(logging.INFO)
        self.log.addHandler(fh)

        self.payloadBuffer = [[0]*self.STRAND_SIZE*self.COLORS_PER_LIGHT for n in range(nStrands)]

        self.kinetsender = KinetSender(myIP, lightIP, nStrands, self.STRAND_SIZE*self.COLORS_PER_LIGHT)
        self.prefix = Name(prefix)
        self.keychain = KeyChain()

        self.address = ""
        self._isStopped = True

        self.lightState = False
        #mengchen: let's make the lighting lits only a half every time
        #self.uphalf =  True
        self.HALF_STRAND_SIZE = 25       
        self.certificateName = self.keychain.getDefaultCertificateName()
        self.receiveFile = open('interestReceiveFile', 'w')

    def unix_time_now(self):
        epoch = datetime.datetime.utcfromtimestamp(0)
        delta = datetime.datetime.utcnow() - epoch
        return delta.total_seconds() * 1000.0
    
    # XXX: we should get a thread for this or something!
    def start(self):

        self.loop = asyncio.get_event_loop()
        self.face = ThreadsafeFace(self.loop, self.address)
        self.face.setCommandSigningInfo(self.keychain, self.certificateName)
        self.face.registerPrefix(self.prefix, self.onLightingCommand, self.onRegisterFailed)
        self._isStopped = False
        self.face.stopWhen(lambda:self._isStopped)
        try:
            self.loop.run_forever()
        except KeyboardInterrupt:
            #self.stop()
            #print "key interrupt in run_forever"
            sys.exit()
        finally:
            #print "executing finally"
            self.stop()


    def stop(self):
        self.loop.close()
        self.kinetsender.stop = True
        #print "before wait"
        #self.kinetsender.complete.set
        #self.kinetsender.complete.wait()         
        self.face.shutdown()
        self.face = None
        #print "sys exit"
        self.receiveFile.close()
        sys.exit(1)

    def signData(self, data):
        if LightController.shouldSign:
            self.keychain.sign(data, self.certificateName)
        else:
            data.setSignature(Sha256WithRsaSignature())

    def setPayloadColor(self, strand, color):
    # will expand this to allow the repeats, etc
       # if self.uphalf and not self.lightState:
        #    self.uphalf = False
         #   self.payloadBuffer[strand] = [int(color.r)&0xff, int(color.g)&0xff, int(color.b)&0xff]*self.HALF_STRAND_SIZE+[int(0)&0xff, int(0)&0xff, int(0)&0xff]*self.HALF_STRAND_SIZE
        #if not self.uphalf and self.lightState :
         #   self.uphalf = True
            self.payloadBuffer[strand] = [int(color.r)&0xff, int(color.g)&0xff, int(color.b)&0xff]*self.STRAND_SIZE
            
    def onLightingCommand(self, prefix, interest, transport, prefixId):
        #print datetime.datetime.now()
        self.receiveFile.write('{0:f}'.format(self.unix_time_now()) + '\n')
        interestName = Name(interest.getName())
        #interstname: /ndn/ucla.edu/sculptures/ai_bus/lights/setRGB/%83%0D%84%0B%87%09%89%01%04%8A%01%01%8B%01%01
        #d: <pyndn.data.Data object at 0xb64825d0>
        print "interstname", interestName.toUri()
        d = Data(interest.getName())
        # get the command parameters from the name
        try:
            commandComponent = interest.getName().get(prefix.size())
            #print commandComponent.toEscapedString():setRGB
            #print "prefix ",prefix.toUri():/ndn/ucla.edu/sculpture/ai_bus/lights
            #print "get name",interest.getName().toUri()
            commandParams = interest.getName().get(prefix.size()+1)
            #print "commandParams ",commandParams:%83%0D%84%0B%87%09%89%01%04%8A%01%01%8B%01%01

            lightingCommand = LightCommandMessage()
            ProtobufTlv.decode(lightingCommand, commandParams.getValue())
            #self.log.info("Command: " + commandComponent.toEscapedString())
            requestedColor = lightingCommand.command.pattern.colors[0] 
            colorStr = str((requestedColor.r, requestedColor.g, requestedColor.b))
            #self.log.info("Requested color: " + colorStr)
            self.setPayloadColor(0, requestedColor)
            
            self.lightState = not self.lightState
            if self.lightState:
                print "Off"
            else:
                print "On"
            
            #print requestedColor
            
            self.sendLightPayload(1)
            d.setContent("Gotcha: " + colorStr+ "\n")
        except KeyboardInterrupt:
            print "key interrupt"
            sys.exit(1)
        except Exception as e:
            print e
            d.setContent("Bad command\n")
        finally:
            d.getMetaInfo().setFinalBlockID(0)
            self.signData(d)

        encodedData = d.wireEncode()
        transport.send(encodedData.toBuffer())

    def onRegisterFailed(self, prefix):
        self.log.error("Could not register " + prefix.toUri())
        self.stop()

    def sendLightPayload(self, port):
        # print port
        # print self.payloadBuffer[port-1]
        self.kinetsender.setPayload(port, self.payloadBuffer[port-1])

done = False
if __name__ == '__main__':
    l = LightController(prefix='/ndn/ucla.edu/sculptures/ai_bus/lights')
    try:
        l.start()
    except KeyboardInterrupt:
        print "captured here"
        l.stop()
