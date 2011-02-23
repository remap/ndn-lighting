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
#include <arpa/inet.h>

#include "dns_a.h"
#include "dns_rr.h"
#include "dns_name.h"
#include "dns_defs.h"

DnsA::DnsA()
  : DnsRR(DNS_RR_A)
{
}

DnsA::DnsA(const DnsName &name, uint32_t addr)
  : DnsRR(DNS_RR_A)
{
  init(new DnsName(name), DNS_CLASS_IN, 300, (u_char *)&addr, 4);
}

bool DnsA::rdata_valid()
{
  bool ret = true;
  if (m_rdlen != 4)
    ret = false;
  return ret;
}

// should probably throw exception if the size isn't 4
uint32_t DnsA::ip()
{
  assert(m_rdlen == 4);
//fprintf(stderr, "IP is %d.%d.%d.%d\n", (int)m_rdata[0], (int)m_rdata[1], (int)m_rdata[2], (int)m_rdata[3]);
  return ntohl(*(uint32_t *)m_rdata);
}

void DnsA::set_ip(uint32_t ip)
{
  assert(m_rdlen == 4);
  *(uint32_t *)m_rdata = htonl(ip);
}

DnsA *DnsA::dup()
{
  return new DnsA();
}

void DnsA::printRData()
{
  fprintf(stdout, "%d.%d.%d.%d", (int)m_rdata[0], (int)m_rdata[1], (int)m_rdata[2], (int)m_rdata[3]);
}

