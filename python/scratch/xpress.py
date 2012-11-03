import sys
from pyccn import CCN,Name,Interest,ContentObject,Key,Closure,_pyccn,NameCrypto
import ssl
#import database as data
import subprocess
import os
import commands
import time
#from time import time
try:
    import json
except ImportError:
    import simplejson as json

current = 0

class sequencer(Closure.Closure):

        def __init__(self, configFileName, interestURI):
                self.appConfigFileName = configFileName
                self.loadConfigFile()
                self.handle = CCN.CCN()
                self.getApplicationKey()
                #nameCrypto
                self.state = NameCrypto.new_state()
                self.cryptoKey = NameCrypto.generate_application_key(self.cfg.fixtureKey, self.cfg.appName)
                self.count = 0
		self.iURI = interestURI

        def loadConfigFile(self):
                command = "import "+self.appConfigFileName+" as appCfg"
                exec(command)
                self.appCfg = appCfg;
                self.cfg = appCfg;

        def getApplicationKey(self):
                print("getting application key for "+self.appCfg.appName)
                key = Key.Key()
                keyFile = self.appCfg.keyFile
                key.fromPEM(filename=keyFile)
                self.appKey = key
                self.key = key

        def start(self):
		print "starting "+self.cfg.appName

		self.sendSignedInterest(self.iURI)

        def sendSignedInterest(self,command):
                self.count = self.count +1
                time.sleep(self.cfg.refreshInterval)
                fullURI = self.cfg.appPrefix + command
                print fullURI
                i = Interest.Interest()
                #self.state = NameCrypto.new_state()
                #build keyLocator to append to interest for NameCrypto on upcall
                keyLoc = Key.KeyLocator(self.key)
                keyLocStr = _pyccn.dump_charbuf(keyLoc.ccn_data)
                nameAndKeyLoc = Name.Name(str(fullURI))
                #print("there are "+str(len(nameAndKeyLoc))+" components")
                nameAndKeyLoc += keyLocStr
                #print("there are "+str(len(nameAndKeyLoc))+" components after adding keyLocStr")

                #symmetric
                authName = NameCrypto.authenticate_command(self.state, nameAndKeyLoc, self.cfg.appName, self.cryptoKey)

                #asymmetric
                #authName = NameCrypto.authenticate_command_sig(self.state, nameAndKeyLoc, self.cfg.appName, self.key)

                #print authName.components


                co = self.handle.expressInterest(authName,self)


def usage():
        print("Usage: %s <Application configFileName> <ccnx://URI>" % sys.argv[0])
        sys.exit(1)

if __name__ == '__main__':
        if (len(sys.argv) != 3):
                usage()

        runtime = sequencer(sys.argv[1], sys.argv[2])
        runtime.start()

