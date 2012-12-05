
import serial
import sys
import time
from threading import Thread, Lock, Event


def get_command_line():
    if len(sys.argv) > 1:
        try:
            port_num = sys.argv[1]
        except:
            print USAGE_STRING
            sys.exit(0)
    else:
        # Default serial port number
        port_num = "/dev/tty.usbserial-ENQBGFZT" #"/dev/ttyUSB0"
    return port_num


def test_dmx_send():
	print "Sending 512 DMX data bytes: 00 01 02 ... FF 00 01 02 ... FF"
	int_data = [0]+[210] + [255] + [255] + [255]
	for k in range(0,508):
                int_data = int_data + [0]
        print int_data
	msg_data = [chr(int_data[j]) for j in range(len(int_data))]
	transmit_to_widget(OUTPUT_ONLY_SEND_DMX_LABEL, msg_data, len(msg_data))
	print "lentgh",len(msg_data)
         

class serialSender(Thread):
        PACKET_INTERVAL_SECONDS = 0.023
        SOM_VALUE = 0x7E
        EOM_VALUE = 0xE7

        REPROGRAM_FIRMWARE_LABEL 	= 1
        PROGRAM_FLASH_PAGE_LABEL	= 2
        RECEIVED_DMX_LABEL		= 5
        OUTPUT_ONLY_SEND_DMX_LABEL	= 6
        RDM_SEND_DMX_LABEL		= 7
        INVALID_LABEL 			= 0xFF
        DMXFRAME = 0.023
        ser = None
        stop = False
        complete = Event()
        stTime = time.time()
        int_data = []

        def elapsed(self):
                return time.time() - self.stTime

        def __init__(self,port_num, start=True):
                Thread.__init__(self)
                # Initialize serial port
                try:
                        # Open serial port with receive timeout
                        self.ser = serial.Serial(port=port_num, baudrate=115600, timeout=1, stopbits=2, parity="N")

                except:
                        print "ERROR: Could not open %s" % (port_num)
                        sys.exit(0)
                else:
                        print "Using %s" % (self.ser.portstr)
                        self.start()


        def close_serial_port(self):
                self.ser.close()
                
        def transmit_to_widget(self, label, data, data_size):
                #print "trans"
                try:
                        self.ser.write(chr(self.SOM_VALUE))
                        self.ser.write(chr(label))
                        self.ser.write(chr(data_size & 0xFF))
                        self.ser.write(chr((data_size >> 8) & 0xFF))
                        for j in range(data_size):
                                self.ser.write(data[j])
                        self.ser.write(chr(self.EOM_VALUE))
                except Exception, e:
                        print "exception ",e
                        self.complete.set()
                        
        def setdmx(value):
            self.int_data=value

        def run(self):  
                # Begin
                while(not self.stop):
                    try:
                        te = self.elapsed() 
                        if te > self.DMXFRAME:
                                print te
                                self.stTime = time.time()
                                self.test_dmx_send1()
                        else:
                                time.sleep(0.0005)  
                        #print "send"
                        
                    except KeyboardInterrupt:
                        print "key interr"
                        sys.exit(1)
                    except Exception, e:
                        print "exception ",e
                        self.complete.set()
        
        def test_dmx_send1(self):
                #print "send1"
                print self.int_data[2]
                msg_data = [chr(self.int_data[j]) for j in range(len(self.int_data))]  
                self.transmit_to_widget(self.OUTPUT_ONLY_SEND_DMX_LABEL, msg_data, len(msg_data))
                #print len(msg_data)


port_num = get_command_line()
serialsender = serialSender(port_num)

N=0
serialsender.int_data = [0]+[210]+[0]+[0]+[0]       
for r in range (0, 256,3):
    try: 
        serialsender.int_data[3] = r  
        time.sleep(0.1)  

    except Exception, e:
        print "exception ",e

for r in range (255, 0,3):
    try:
        serialsender.int_data[2] = r
        time.sleep(0.1)

    except Exception, e:
        print "exception ",e

serialsender.stop = True
serialsender.complete.wait()
serialsender.close_serial_port()
sys.exit(1)

