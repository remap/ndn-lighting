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
#include "dns_dnskey.h"
#include "dns_defs.h"

DnsDnskey::DnsDnskey()
  : DnsRR(DNS_RR_DNSKEY),
    m_uFlags(0),
    m_uProto(0),
    m_uAlgo(0),
    m_pBinKey(NULL),
    m_uBinKeyLen(0)
{

}

DnsDnskey::DnsDnskey(DnsDnskey &p_oRHS)
  : DnsRR(DNS_RR_DNSKEY),
    m_uFlags(0),
    m_uProto(0),
    m_uAlgo(0),
    m_pBinKey(NULL),
    m_uBinKeyLen(0)
{
  *this = p_oRHS;
}

DnsDnskey::~DnsDnskey()
{
  setBinKey(NULL, 0);
}

const DnsDnskey &DnsDnskey::operator=(DnsDnskey &p_oRHS)
{
  set_class(p_oRHS.get_class());
  set_name(*p_oRHS.get_name());
  set_ttl(p_oRHS.ttl());

  setFlags(p_oRHS.getFlags());
  setProto(p_oRHS.getProto());
  setAlgo(p_oRHS.getAlgo());
  setKey(p_oRHS.getKey());

  set_rdata(p_oRHS.get_rdata(), p_oRHS.get_rdlen());
  setBinKey(p_oRHS.getBinKey(), p_oRHS.getBinKeyLen());
  return *this;
}

bool DnsDnskey::operator==(DnsDnskey &p_oRHS)
{
  //return (getKey() == p_oRHS.getKey());
  return DnsRR::operator==(p_oRHS);
}

bool DnsDnskey::rdata_valid()
{
  // From the RFC, the protocol must be 3
  return (getProto() == 3);
}

uint16_t DnsDnskey::getFlags()
{
  return m_uFlags;
}

void DnsDnskey::setFlags(uint16_t p_uFlags)
{
  m_uFlags = p_uFlags;
}

uint8_t DnsDnskey::getProto()
{
  return m_uProto;
}

void DnsDnskey::setProto(uint8_t p_uProto)
{
  m_uProto = p_uProto;
}

uint8_t DnsDnskey::getAlgo()
{
  return m_uAlgo;
}

void DnsDnskey::setAlgo(uint8_t p_uAlgo)
{
  m_uAlgo = p_uAlgo;
}

std::string &DnsDnskey::getKey()
{
  return m_sKey;
}

void DnsDnskey::setKey(std::string &p_sKey)
{
  m_sKey = p_sKey;
}

u_char *DnsDnskey::getBinKey()
{
  return m_pBinKey;
}

size_t DnsDnskey::getBinKeyLen()
{
  return m_uBinKeyLen;
}

void DnsDnskey::setBinKey(u_char *p_pKey, size_t p_uLen)
{
  if (NULL != m_pBinKey)
  {
    delete[] m_pBinKey;
    m_pBinKey = NULL;
  }

  if (NULL != p_pKey && p_uLen > 0)
  {
    m_pBinKey = new u_char[p_uLen];
    m_uBinKeyLen = p_uLen;

    memcpy(m_pBinKey, p_pKey, m_uBinKeyLen);
  }
}

bool DnsDnskey::parseRData(u_char *p_pMsg,
                           size_t p_uMsgLen,
                           u_char *p_pRData,
                           size_t p_uRDataLen)
{
  bool bRet = false;

  if (!isQuestion() && p_uRDataLen < 5)
  {
    dns_log("DNSKEY is not long enough: %u\n", (unsigned) p_uRDataLen);
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

    setBinKey(p_pRData, p_uRDataLen);

    std::string sKey = base64_encode((const unsigned char *) p_pRData, p_uRDataLen);
    setKey(sKey);

    bRet = true;
  }

  return bRet;
}

DnsDnskey *DnsDnskey::dup()
{
  return new DnsDnskey();
}

void DnsDnskey::printRData()
{
  fprintf(stdout, "%u %u %u %s\n",
          (unsigned) getFlags(),
          (unsigned) getProto(),
          (unsigned) getAlgo(),
          getKey().c_str());
}
