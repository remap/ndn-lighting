 #
 # Copyright (c) 2008,2009, University of California, Los Angeles and 
 # Colorado State University All rights reserved.
 # 
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met:
 # 
 #     * Redistributions of source code must retain the above copyright notice,
 #       this list of conditions and the following disclaimer.
 #     * Redistributions in binary form must reproduce the above copyright
 #       notice, this list of conditions and the following disclaimer in the
 #       documentation and/or other materials provided with the distribution.
 #     * Neither the name of NLnetLabs nor the names of its
 #       contributors may be used to endorse or promote products derived from this
 #       software without specific prior written permission.
 # 
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 # AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 # ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 # LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 # CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 # SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 # INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 # CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 # ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 # POSSIBILITY OF SUCH DAMAGE.
 # 


CC =    /home/nesl/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-gcc
CXX =    /home/nesl/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-g++
libtool   = ./libtool
LIBTOOL = $(libtool) --tag=CXX
LINK_LIB  = $(LIBTOOL) --mode=link $(CXX) $(CXXFLAGS) $(LDFLAGS) -version-number 0:8:6 -no-undefined

prefix = /usr/local
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib
# RUNTIME_PATH  = @RUNTIME_PATH@
LIBS = -lssl -lcrypto -lpcap -lexpat -lgpg-error -lgpgme-pthread -lsqlite3 -lpthread  -ldl  -lcurl

DEFINES = -g -Wall -Werror -D_LIBVDNS_VERSION=0:8:6 -DHAVE_CONFIG_H --no-strict-aliasing
CFLAGS =  -g -O2  $(DEFINES)
CXXFLAGS =  -g -O2 $(DEFINES)
CPPFLAGS = -DDEBUG -D_POSIX_THREADS -D_REENTRANT -I/usr/local/include -I/opt/local/include  

DNS_OBJS = dns_a.o dns_compression.o dns_header.o dns_name.o \
           dns_packet.o dns_rr_fact.o dns_dnskey.o dns_rr.o \
           dns_ns.o dns_opt.o dns_resolver.o dns_task.o dns_ds.o \
           dns_err.o dns_rrsig.o dns_srv.o dns_verifier.o dns_info.o \
           dns_mutex.o dns_mutex_hdlr.o \
           base64.o b64.o dns_tsig.o tsig_key.o md5.o

DNS_L_OBJS = $(DNS_OBJS:.o=.lo)

.cc.o:
	$(LIBTOOL) --mode=compile $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<
#	$(CXX) $(CXXFLAGS) $(DEFINES) $(CPPFLAGS) -c $<

.c.o:
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) $(CFLAGS) -c $<
#	$(CC) $(CFLAGS) $(DEFINES) $(CPPFLAGS) -c $<

all: $(DNS_OBJS)
	$(LINK_LIB) -o ./libvdns.la $(DNS_L_OBJS) $(LDFLAGS) -lssl -lcrypto -rpath $(libdir) 
	-( ln -s ./.libs lib )

clean:
	rm -rf $(DNS_OBJS) $(DNS_L_OBJS) ./.libs lib
