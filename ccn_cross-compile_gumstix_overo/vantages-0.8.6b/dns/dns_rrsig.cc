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

#include <string>

#include "dns_rrsig.h"
#include "dns_name.h"
#include "base64.h"
#include "dns_defs.h"

// The RRSIG has a mandatory 8 16-bit fields before the owner name and crypto carp
#define DNS_RRSIG_HEADER_LEN (2 * 8 + 2)

using namespace std;

DnsRrsig::DnsRrsig()
  : DnsRR(DNS_RR_RRSIG),
    m_uTypeCovered(0),
    m_uAlgo(0),
    m_uLabels(0),
    m_uOrigTTL(0),
    m_uExp(0),
    m_uIncep(0),
    m_uKeyTag(0),
    m_pBinSig(NULL),
    m_uBinSigLen(0)
{

}

DnsRrsig::DnsRrsig(DnsRrsig &p_oRHS)
  : DnsRR(DNS_RR_RRSIG),
    m_uTypeCovered(0),
    m_uAlgo(0),
    m_uLabels(0),
    m_uOrigTTL(0),
    m_uExp(0),
    m_uIncep(0),
    m_uKeyTag(0),
    m_pBinSig(NULL),
    m_uBinSigLen(0)
{
  *this = p_oRHS;
}

DnsRrsig::~DnsRrsig()
{
  setBinSig(NULL, 0);
}

const DnsRrsig &DnsRrsig::operator=(DnsRrsig &p_oRHS)
{
  set_class(p_oRHS.get_class());
  set_name(*p_oRHS.get_name());
  set_ttl(p_oRHS.ttl());
  set_rdata(p_oRHS.get_rdata(), p_oRHS.get_rdlen());

  setTypeCovered(p_oRHS.getTypeCovered());
  setAlgo(p_oRHS.getAlgo());
  setLabels(p_oRHS.getLabels());
  setOrigTTL(p_oRHS.getOrigTTL());
  setExpiration(p_oRHS.getExpiration());
  setInception(p_oRHS.getInception());
  setKeyTag(p_oRHS.getKeyTag());
  setBinSig(p_oRHS.getBinSig(), p_oRHS.getBinSigLen());
  setSignersName(p_oRHS.getSignersName());
  setSig(p_oRHS.getSig());

  return *this;
}

bool DnsRrsig::operator==(DnsRrsig &p_oRHS)
{
  return DnsRR::operator==(p_oRHS);
}

bool DnsRrsig::rdata_valid()
{
  return true;
}

uint16_t DnsRrsig::getTypeCovered()
{
  return m_uTypeCovered;
}

void DnsRrsig::setTypeCovered(uint16_t p_uTypeCovered)
{
  m_uTypeCovered = p_uTypeCovered;
}

uint8_t DnsRrsig::getAlgo()
{
  return m_uAlgo;
}

void DnsRrsig::setAlgo(uint8_t p_uAlgo)
{
  m_uAlgo = p_uAlgo;
}

uint8_t DnsRrsig::getLabels()
{
  return m_uLabels;
}

void DnsRrsig::setLabels(uint8_t p_uLabels)
{
  m_uLabels = p_uLabels;
}

uint32_t DnsRrsig::getOrigTTL()
{
  return m_uOrigTTL;
}

void DnsRrsig::setOrigTTL(uint32_t p_uOrigTTL)
{
  m_uOrigTTL = p_uOrigTTL;
}

uint32_t DnsRrsig::getExpiration()
{
  return m_uExp;
}

void DnsRrsig::setExpiration(uint32_t p_uExp)
{
  m_uExp = p_uExp;
}

uint32_t DnsRrsig::getInception()
{
  return m_uIncep;
}

void DnsRrsig::setInception(uint32_t p_uIncep)
{
  m_uIncep = p_uIncep;
}

uint16_t DnsRrsig::getKeyTag()
{
  return m_uKeyTag;
}

void DnsRrsig::setKeyTag(uint16_t p_uKeyTag)
{
  m_uKeyTag = p_uKeyTag;
}

std::string &DnsRrsig::getSig()
{
  return m_sSig;
}

void DnsRrsig::setSig(std::string &p_sSig)
{
  m_sSig = p_sSig;
}

u_char *DnsRrsig::getBinSig()
{
  return m_pBinSig;
}

size_t DnsRrsig::getBinSigLen()
{
  return m_uBinSigLen;
}

void DnsRrsig::setBinSig(u_char *p_pSig, size_t p_uLen)
{
  if (NULL != m_pBinSig)
  {
    delete[] m_pBinSig;
    m_pBinSig = NULL;
  }

  if (NULL != p_pSig && p_uLen > 0)
  {
    m_pBinSig = new u_char[p_uLen];
    m_uBinSigLen = p_uLen;
    memcpy(m_pBinSig, p_pSig, m_uBinSigLen);
  }
}

std::string &DnsRrsig::getSignersName()
{
  return m_sSignersName;
}

void DnsRrsig::setSignersName(std::string &p_sName)
{
  m_sSignersName = p_sName;
}

bool DnsRrsig::parseRData(u_char *p_pMsg,
                           size_t p_uMsgLen,
                           u_char *p_pRData,
                           size_t p_uRDataLen)
{
  bool bRet = false;

  if (!isQuestion() && p_uRDataLen < 5)
  {
    dns_log("RRSIG is not long enough: %u\n", (unsigned) p_uRDataLen);
  }
  else if (!isQuestion())
  {
    set_rdata(p_pRData, p_uRDataLen);

    setTypeCovered(ntohs(*(uint16_t *)p_pRData));
    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    uint8_t uAlgo = (ntohs(*(uint16_t *) p_pRData) >> 8) & 0x00ff;
    setAlgo(uAlgo);

    uint8_t uLabels = ntohs(*(uint16_t *) p_pRData) & 0x00ff;
    setLabels(uLabels);

    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    uint32_t uOrigTTL = ntohl(*(uint32_t *) p_pRData);
    setOrigTTL(uOrigTTL);
    p_pRData += sizeof(uint32_t);
    p_uRDataLen -= sizeof(uint32_t);

    uint32_t uExp = ntohl(*(uint32_t *) p_pRData);
    setExpiration(uExp);
    p_pRData += sizeof(uint32_t);
    p_uRDataLen -= sizeof(uint32_t);

    uint32_t uIncep = ntohl(*(uint32_t *) p_pRData);
    setInception(uIncep);
    p_pRData += sizeof(uint32_t);
    p_uRDataLen -= sizeof(uint32_t);

    setKeyTag(ntohs(*(uint16_t *)p_pRData));
    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    size_t uLen = 0;
    DnsName *pName = DnsName::from_wire(p_pRData, p_uRDataLen, uLen);
/*
    string sSignersName = (const char *) p_pRData;
    setSignersName(sSignersName);
    p_pRData += sSignersName.size() + 1;
    p_uRDataLen -= (sSignersName.size() + 1);
*/
    if (NULL == pName)
    {
      dns_log("Unable to parse name for RRSIG.\n");
    }
    else
    {
      string sSignersName = pName->toString();
      setSignersName(sSignersName);
//      p_pRData += sSignersName.size() + 4;
//      p_uRDataLen -= (sSignersName.size() + 4);
      p_pRData += pName->length();
      p_uRDataLen -= pName->length();

      setBinSig(p_pRData, p_uRDataLen);
      std::string sSig = base64_encode((const unsigned char *) p_pRData, p_uRDataLen);
      setSig(sSig);
      bRet = true;

      delete pName;
      pName = NULL;
    }
  }

  return bRet;
}

DnsRrsig *DnsRrsig::dup()
{
  return new DnsRrsig();
}

void DnsRrsig::printRData()
{
  fprintf(stdout, "%u %u %u %u %u %u %u %s %s\n",
          (unsigned) getTypeCovered(),
          (unsigned) getAlgo(),
          (unsigned) getLabels(),
          (unsigned) getOrigTTL(),
          (unsigned) getExpiration(),
          (unsigned) getInception(),
          (unsigned) getKeyTag(),
          getSignersName().c_str(),
          getSig().c_str());
}

int DnsRrsig::verificationRData(DnsBits_t &p_oOut)
{
  string &sSignersName = getSignersName();
  DnsName oName(sSignersName);

  u_char *pRData = get_rdata();

  p_oOut.clear();
  for (size_t u = 0; u < DNS_RRSIG_HEADER_LEN; u++)
  {
    p_oOut.push_back(pRData[u]);
  }

  u_char pBuff[255];
  memset(pBuff, 0, 255);
  int iLen = oName.to_wire_canonical(pBuff, 255);
  dns_log("Got name length: %d for name '%s'\n", iLen, oName.toString().c_str());
  //iLen++;

  for (int i = 0; i < iLen; i++)
  {
    p_oOut.push_back(pBuff[i]);
  }

  return (int) p_oOut.size();
}

