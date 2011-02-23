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

#include "config.h"
#include <stdio.h>
#include <algorithm>

#include "dns_rr_fact.h"
#include "dns_defs.h"
#include "dns_a.h"
#include "dns_ns.h"
#include "dns_opt.h"
#include "dns_dnskey.h"
#include "dns_rrsig.h"
#include "dns_ds.h"
#include "dns_tsig.h"
#include "dns_srv.h"

DnsRrFactory DnsRrFactory::s_oInstance;

DnsRrFactory::DnsRrFactory()
{
  reg(DNS_RR_A, *(new DnsA()));
  reg(DNS_RR_NS, *(new DnsNs()));
  reg(DNS_RR_SRV, *(new DnsSrv()));
  reg(DNS_RR_OPT, *(new DnsOpt()));
  reg(DNS_RR_RRSIG, *(new DnsRrsig()));
  reg(DNS_RR_DNSKEY, *(new DnsDnskey()));
  reg(DNS_RR_DS, *(new DnsDs()));
  reg(DNS_RR_TSIG, *(new DnsTsig()));

  //RFC 5395 20080710

  //TYPES
  mTypeToProtoNum["A"] = 1;
  mTypeToProtoNum["NS"] = 2;
  mTypeToProtoNum["MD"] = 3;
  mTypeToProtoNum["MF"] = 4;
  mTypeToProtoNum["CNAME"] = 5;
  mTypeToProtoNum["SOA"] = 6;
  mTypeToProtoNum["MB"] = 7;
  mTypeToProtoNum["MG"] = 8;
  mTypeToProtoNum["MR"] = 9;
  mTypeToProtoNum["NULL"] = 10;
  mTypeToProtoNum["WKS"] = 11;
  mTypeToProtoNum["PTR"] = 12;
  mTypeToProtoNum["HINFO"] = 13;
  mTypeToProtoNum["MINFO"] = 14;
  mTypeToProtoNum["MX"] = 15;
  mTypeToProtoNum["TXT"] = 16;
  mTypeToProtoNum["RP"] = 17;
  mTypeToProtoNum["AFSDB"] = 18 ;
  mTypeToProtoNum["X25"] = 19;
  mTypeToProtoNum["ISDN"] = 20;
  mTypeToProtoNum["RT"] = 21;
  mTypeToProtoNum["NSAP"] = 22;
  mTypeToProtoNum["NSAP-PTR"] = 23;
  mTypeToProtoNum["SIG"] = 24;
  mTypeToProtoNum["KEY"] = 25;
  mTypeToProtoNum["PX"] = 26;
  mTypeToProtoNum["GPOS"] = 27;
  mTypeToProtoNum["AAAA"] = 28;
  mTypeToProtoNum["LOC"] = 29;
  mTypeToProtoNum["NXT"] = 30;
  mTypeToProtoNum["EID"] = 31;
  mTypeToProtoNum["NIMLOC"] =32 ;
  mTypeToProtoNum["SRV"] = 33;
  mTypeToProtoNum["ATMA"] = 34;
  mTypeToProtoNum["NAPTR"] = 35;
  mTypeToProtoNum["KX"] = 36;
  mTypeToProtoNum["CERT"] = 37;
  mTypeToProtoNum["A6"] = 38;
  mTypeToProtoNum["DNAME"] = 39 ;
  mTypeToProtoNum["SINK"] = 40;
  mTypeToProtoNum["OPT"] = 41;
  mTypeToProtoNum["APL"] = 42;
  mTypeToProtoNum["DS"] = 43;
  mTypeToProtoNum["SSHFP"] = 44 ;
  mTypeToProtoNum["IPSECKEY"] = 45 ;
  mTypeToProtoNum["RRSIG"] = 46;
  mTypeToProtoNum["NSEC"] = 47;
  mTypeToProtoNum["DNSKEY"] = 48;
  mTypeToProtoNum["DHCID"] = 49;
  mTypeToProtoNum["NSEC3"] = 50;
  mTypeToProtoNum["NSEC3PARAM"] = 51;
  mTypeToProtoNum["HIP"] = 55;
  mTypeToProtoNum["NINFO"] = 56;
  mTypeToProtoNum["RKEY"] = 57;
  mTypeToProtoNum["SPF"] = 99;
  mTypeToProtoNum["UINFO"] = 100;
  mTypeToProtoNum["UID"] = 101;
  mTypeToProtoNum["GID"] = 102;
  mTypeToProtoNum["UNSPEC"] = 103;
  mTypeToProtoNum["TKEY"] = 249;
  mTypeToProtoNum["TSIG"] = 250;
  mTypeToProtoNum["IXFR"] = 251;
  mTypeToProtoNum["AXFR"] = 252;
  mTypeToProtoNum["MAILB"] = 253;
  mTypeToProtoNum["MAILA"] = 254;
  mTypeToProtoNum["TA"] = 32768;
  mTypeToProtoNum["DLV"] = 32769;
  
  //CLASSES
  mTypeToProtoNum["IN"] = 1;
  mTypeToProtoNum["CH"] = 3;
  mTypeToProtoNum["HS"] = 4;
  mTypeToProtoNum["NONE"] = 254;
  mTypeToProtoNum["ANY"] = 255;
}

DnsRrFactory::~DnsRrFactory()
{
  reset();
}

DnsRrFactory &DnsRrFactory::getInstance()
{
  return s_oInstance;
}

IDnsDupable *DnsRrFactory::create(int p_iKey)
{
  IDnsDupable *pRet = DnsFactory<int>::create(p_iKey);

  if (NULL == pRet)
  {
    pRet = new DnsRR(p_iKey);
  }

  return pRet;
}

int DnsRrFactory::getProtoNum(std::string p_sType)
{
  std::transform(p_sType.begin(), p_sType.end(), p_sType.begin(), ::toupper); 
  return mTypeToProtoNum[p_sType]; 
}
