CC =    /home/nesl/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-gcc
CXX =    /home/nesl/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-g++
DEFINES = -g -Wall -Werror -DHAVE_CONFIG_H
CFLAGS =  -g -O2  $(DEFINES)
CXXFLAGS =  -g -O2 
CPPFLAGS = -DDEBUG -D_POSIX_THREADS -D_REENTRANT -I//home/nesl/overo-oe/tmp/sysroots/armv7a-angstrom-linux-gnueabi/usr/include/ -I//home/nesl/Documents/ccn/vantages-0.8.6b/dns/ 
LDFLAGS = -L/home/nesl/Documents/ccn/vantages-0.8.6b/dns/lib -L/home/nesl/overo-oe/tmp/sysroots/armv7a-angstrom-linux-gnueabi/usr/lib/ -L/home/nesl/overo-oe/tmp/sysroots/armv7a-angstrom-linux-gnueabi/lib/  
LIBS = -lvdns -lcrypto -lpthread 

prefix = /usr/local
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib

OBJS = CCNListener.o CCNPeerMap.o CCNRegEntry.o CCNRegMsg.o \
       RPClient.o ccn_global.o ccnrp_config.o

PROG = ccnrp ccnrp_client 

.cpp.o:
	$(CXX) $(CXXFLAGS) $(DEFINES) $(CPPFLAGS) -c $<

.cc.o:
	$(CXX) $(CXXFLAGS) $(DEFINES) $(CPPFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) $(CPPFLAGS) -c $<

all: $(PROG)

$(PROG): $(OBJS)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(CPPFLAGS) $(LDFLAGS) -o ./bin/$@ $@.cpp $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) 
