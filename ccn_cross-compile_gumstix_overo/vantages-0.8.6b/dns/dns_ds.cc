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
#include <string.h>

#include "dns_a.h"
#include "dns_rr.h"
#include "dns_name.h"
#include "base64.h"
#include "dns_ds.h"
#include "dns_defs.h"


DnsDs::DnsDs()
  : DnsRR(DNS_RR_DS),
    m_uFlags(0),
    m_uProto(0),
    m_uAlgo(0),
    m_pBinDig(NULL),
    m_uBinDigLen(0)
{

}

DnsDs::DnsDs(DnsDs &p_oRHS)
  : DnsRR(DNS_RR_DS),
    m_uFlags(0),
    m_uProto(0),
    m_uAlgo(0),
    m_pBinDig(NULL),
    m_uBinDigLen(0)
{
  *this = p_oRHS;
}

DnsDs::~DnsDs()
{
  setBinDig(NULL, 0);
}

const DnsDs &DnsDs::operator=(DnsDs &p_oRHS)
{
  set_class(p_oRHS.get_class());
  set_name(*p_oRHS.get_name());
  set_ttl(p_oRHS.ttl());

  setFlags(p_oRHS.getFlags());
  setProto(p_oRHS.getProto());
  setAlgo(p_oRHS.getAlgo());
  setDig(p_oRHS.getDig());

  set_rdata(p_oRHS.get_rdata(), p_oRHS.get_rdlen());
  setBinDig(p_oRHS.getBinDig(), p_oRHS.getBinDigLen());

  return *this;
}

bool DnsDs::rdata_valid()
{
  // From the RFC, the protocol must be 3
  return (getProto() == 3);
}

uint16_t DnsDs::getFlags()
{
  return m_uFlags;
}

void DnsDs::setFlags(uint16_t p_uFlags)
{
  m_uFlags = p_uFlags;
}

uint8_t DnsDs::getProto()
{
  return m_uProto;
}

void DnsDs::setProto(uint8_t p_uProto)
{
  m_uProto = p_uProto;
}

uint8_t DnsDs::getAlgo()
{
  return m_uAlgo;
}

void DnsDs::setAlgo(uint8_t p_uAlgo)
{
  m_uAlgo = p_uAlgo;
}

std::string &DnsDs::getDig()
{
  return m_sDig;
}

void DnsDs::setDig(std::string &p_sDig)
{
  m_sDig = p_sDig;
}

u_char *DnsDs::getBinDig()
{
  return m_pBinDig;
}

size_t DnsDs::getBinDigLen()
{
  return m_uBinDigLen;
}

void DnsDs::setBinDig(u_char *p_pDig, size_t p_uLen)
{
  if (NULL != m_pBinDig)
  {
    delete[] m_pBinDig;
    m_pBinDig = NULL;
  }

  if (NULL != p_pDig && p_uLen > 0)
  {
    m_pBinDig = new u_char[p_uLen];
    m_uBinDigLen = p_uLen;

    memcpy(m_pBinDig, p_pDig, m_uBinDigLen);
  }
}

bool DnsDs::parseRData(u_char *p_pMsg,
                           size_t p_uMsgLen,
                           u_char *p_pRData,
                           size_t p_uRDataLen)
{
  bool bRet = false;

  if (!isQuestion() && p_uRDataLen < 5)
  {
    dns_log("DS is not long enough: %u\n", (unsigned) p_uRDataLen);
  }
  else if (!isQuestion())
  {
    set_rdata(p_pRData, p_uRDataLen);

    setFlags(ntohs(*(uint16_t *)p_pRData));
    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    uint8_t uProto = (ntohs(*(uint16_t *) p_pRData) >> 8) & 0x00ff;
    setProto(uProto);

    uint8_t uAlgo = ntohs(*(uint16_t *) p_pRData) & 0x00ff;
    setAlgo(uAlgo);

    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    setBinDig(p_pRData, p_uRDataLen);
   
    //this seems like a sloppy way to do this 
    std::string sDig;
    char buf[3] = {'\0', '\0', '\0'};
    for (unsigned int i = 0; i < m_uBinDigLen; i++)
    {
      sprintf(buf, "%2X", m_pBinDig[i]);
      sDig += buf;
    }
    setDig(sDig);

    bRet = true;
  }

  return bRet;
}

DnsDs *DnsDs::dup()
{
  return new DnsDs();
}

void DnsDs::printRData()
{
  fprintf(stdout, "%u %u %u %s",
          (unsigned) getFlags(),
          (unsigned) getProto(),
          (unsigned) getAlgo(),
          getDig().c_str());
}
