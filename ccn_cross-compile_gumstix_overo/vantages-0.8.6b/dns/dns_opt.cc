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
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

#include "dns_name.h"
#include "dns_opt.h"
#include "dns_defs.h"

DnsOpt::DnsOpt(uint16_t p_uMax /*= 512*/, bool p_bDO /*= false*/)
  : DnsRR(DNS_RR_OPT)
{
  set_ttl(0);
  setMax(p_uMax);
  setDO(p_bDO);

  std::string sName = "";
  DnsName *pName = new DnsName(sName);
  init(pName, get_class(), ttl());
  delete pName;
}

DnsOpt::~DnsOpt()
{

}

bool DnsOpt::rdata_valid()
{
  return (get_class() >= 512);
}

uint16_t DnsOpt::getMax()
{
  return get_class();
}

void DnsOpt::setMax(uint16_t p_uMax)
{
  set_class(p_uMax);
}

bool DnsOpt::getDO()
{
  return m_bDO;
}

void DnsOpt::setDO(bool p_bDO)
{
  m_bDO = p_bDO;

  // We need to handle this as it will be over
  // the wire.  So, since we call htonl later, we
  // need to be sure we set this in network byte order
  // and then convert it to host byte order so that
  // it will be converted back to netowrk byte order...
  // Are you ready for lunch yet?
  uint32_t uTTL = htonl(ttl());

  if (m_bDO)
  {
    uTTL |= htonl(0x00008000);
  }
  else
  {
    uTTL &= htonl(0xffff7fff);
  }
  set_ttl(ntohl(uTTL));
}

bool DnsOpt::isQuestion()
{
  return false;
}

DnsOpt *DnsOpt::dup()
{
  return new DnsOpt();
}
