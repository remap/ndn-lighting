Gumstix Notes:

plug USB into gumstix & computer. 

type dmesg to see what the device is called. Edit config file accordingly. 

to connect, use Kermit. 

save settings in a file .gumKermConfig, and run like:

kermit .gumKermConfig


#Kermit Config File for Gumstix Connection
#
#linux
#set line /dev/ttyUSB0    
#OSX:
#set line /dev/tty.usbserial-A5004cJk
#set line /dev/tty.usbserial-A5003vrg
#set line /dev/tty.usbserial-A5004cWG
set line /dev/tty.usbserial-AE00FKWM
set flow-control none
set carrier-watch off
set speed 115200
connect
#EOF


