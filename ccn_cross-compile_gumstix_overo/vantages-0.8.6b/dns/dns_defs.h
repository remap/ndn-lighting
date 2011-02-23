/*
 * Copyright (c) 2008,2009, University of California, Los Angeles and 
 * Colorado State University All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of NLnetLabs nor the names of its
 *       contributors may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef _DNS_DEFS_H
#define _DNS_DEFS_H

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <list>
#include <vector>

#ifdef _DNS_DEBUG
#define dns_log(X, ...) fprintf(stderr, "%s [%d] - " X, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define dns_log(X, ...)
#endif

#define DNS_RR_A 1
#define DNS_RR_NS 2
#define DNS_RR_SRV 33
#define DNS_RR_OPT 41
#define DNS_RR_DS 43
#define DNS_RR_RRSIG 46
#define DNS_RR_DNSKEY 48
#define DNS_RR_TSIG 250

#define DNS_CLASS_IN 1
#define DNS_CLASS_ANY 255

#define DNS_MAX_PACKET_SIZE 4096

#define DNS_TSIG_DEFAULT_FUDGE 300

// Room for a max message size + a huge TSIG RR
#define DNS_TSIG_MAX_BUFSIZ DNS_MAX_PACKET_SIZE + 255


enum rcode_t
{
    DNS_NOERROR = 0, DNS_FORMERR, DNS_SERVFAIL, DNS_NXDOMAIN, DNS_NOTIMP,
    DNS_REFUSED, DNS_YXDOMAIN, DNS_YXRRSET, DNS_NXRRSET, DNS_NOTAUTH,
    DNS_NOTZONE, DNS_BADSIG, DNS_BADKEY, DNS_BADTIME
};

class DnsRR;

struct q_header
{
  int16_t type;
  int16_t qclass;
} __attribute__((__packed__));

struct rr_header
{
  q_header q;
  int32_t ttl;
  uint16_t rdlen;
} __attribute__((__packed__));

typedef unsigned char u_char;

typedef std::list<DnsRR *> RRList_t;
typedef RRList_t::iterator RRIter_t; 

typedef std::vector<u_char> DnsBits_t;
typedef DnsBits_t::iterator DnsBitIter_t;

#endif
