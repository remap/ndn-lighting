Gumstix Notes:

how to connect to, and install OS on, the gumstix

##########
CONNECTING
##########

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

That all can be done without a kernel in the gumstix. 

##########
INSTALLING
##########

To get gumstix OS installed - either refer to scratch instructions here (benefit is you could do ubuntu server)

http://www.gumstix.org/create-a-bootable-microsd-card.html
http://wiki.gumstix.org/index.php?title=Main_Page
http://wiki.gumstix.org/index.php?title=Installing_Ubuntu_10.04_on_Gumstix_Overo - (yet IIRC rootstock is no longer supported; so this is a quagmire)

or use our (2GB) image from TV1 - here:

http://borges.metwi.ucla.edu/lighting/images/TV1_12_23_2011_rebuilt.img

if the latter, LMK & I'll log in when it's set & stop the TV1 autostart scripts & show you around the system ( for updating ccnx/pyccn, etc)


if feeling impossible, give cross-compiling a shot - but i had no luck. the main toolchain (bitbake) was very buggy. 
