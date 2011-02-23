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

#include "dns_srv.h"
#include "dns_rr.h"
#include "dns_name.h"
#include "dns_defs.h"

using namespace std;

DnsSrv::DnsSrv()
  : DnsRR(DNS_RR_SRV),
    m_uPriority(0),
    m_uWeight(0),
    m_uPort(0)
{

}

DnsSrv::DnsSrv(const DnsSrv &p_oRHS)
  : DnsRR(DNS_RR_SRV),
    m_uPriority(0),
    m_uWeight(0),
    m_uPort(0)
{
  *this = p_oRHS;
}

const DnsSrv &DnsSrv::operator=(const DnsSrv &p_oRHS)
{
  m_uPriority = p_oRHS.m_uPriority;
  m_uWeight = p_oRHS.m_uWeight;
  m_uPort = p_oRHS.m_uPort;
  m_sTarget = p_oRHS.m_sTarget;

  return *this;
}

bool DnsSrv::operator==(DnsSrv &p_oRHS)
{
  return (m_uPriority == p_oRHS.m_uPriority
          && m_uWeight == p_oRHS.m_uWeight
          && m_uPort == p_oRHS.m_uPort
          && m_sTarget == p_oRHS.m_sTarget);
}

bool DnsSrv::rdata_valid()
{
  return true;
}

uint16_t DnsSrv::getPriority()
{
  return m_uPriority;
}

void DnsSrv::setPriority(uint16_t p_uPriority)
{
  m_uPriority = p_uPriority;
}

uint16_t DnsSrv::getWeight()
{
  return m_uWeight;
}

void DnsSrv::setWeight(uint16_t p_uWeight)
{
  m_uWeight = p_uWeight;
}

uint16_t DnsSrv::getPort()
{
  return m_uPort;
}

void DnsSrv::setPort(uint16_t p_uPort)
{
  m_uPort = p_uPort;
}

std::string &DnsSrv::getTarget()
{
  return m_sTarget;
}

void DnsSrv::setTarget(std::string &p_sTarget)
{
  m_sTarget = p_sTarget;
}

bool DnsSrv::parseRData(u_char *p_pMsg,
                        size_t p_uMsgLen,
                        u_char *p_pRData,
                        size_t p_uRDataLen)
{
  bool bRet = false;

  if (!isQuestion() && p_uRDataLen < 5)
  {
    dns_log("SRV is not long enough: %u\n", (unsigned) p_uRDataLen);
  }
  else if (!isQuestion())
  {
    set_rdata(p_pRData, p_uRDataLen);

    setPriority(ntohs(*(uint16_t *)p_pRData) & 0x00ff);
    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    setWeight(ntohs(*(uint16_t *)p_pRData) & 0x00ff);
    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    setPort(ntohs(*(uint16_t *)p_pRData) & 0xffff);
    p_pRData += sizeof(uint16_t);
    p_uRDataLen -= sizeof(uint16_t);

    size_t uOffset =  (size_t) (p_pRData - p_pMsg);
    DnsName *pName = DnsName::from_wire(p_pMsg, p_uMsgLen, uOffset);
    if (NULL == pName)
    {
      dns_log("Unable to DnsName::from_wire()\n");
    }
    else
    {
      std::string sName = pName->toString();
      setTarget(sName);
      bRet = true;
      delete pName;
    }

    bRet = true;
  }

  return bRet;
}

DnsSrv *DnsSrv::dup()
{
  return new DnsSrv();
}

void DnsSrv::printRData()
{
  fprintf(stdout, "%u %u %u %s",
          getPriority(),
          getWeight(),
          getPort(),
          getTarget().c_str());
}

int DnsSrv::verificationRData(DnsBits_t &p_oVec)
{
  int iRet = 0;

  u_char pBuff[255];
  memset(pBuff, 0, 255);
  string &sName = getTarget();
  DnsName oName(sName);
  iRet = oName.to_wire_canonical(pBuff, 255);

  DnsBits_t oLocal;

  for (int i = 0; i < iRet; i++)
  {
    oLocal.push_back(pBuff[i]);
  }

  uint16_t uPriority = htons(getPriority());
  p_oVec.push_back((htons(uPriority) >> 8) & 0x00ff);
  p_oVec.push_back(htons(uPriority) & 0x00ff);

  uint16_t uWeight = htons(getWeight());
  p_oVec.push_back((htons(uWeight) >> 8) & 0x00ff);
  p_oVec.push_back(htons(uWeight) & 0x00ff);

  uint16_t uPort = htons(getPort());
  p_oVec.push_back((htons(uPort) >> 8) & 0x00ff);
  p_oVec.push_back(htons(uPort) & 0x00ff);

  p_oVec.insert(p_oVec.end(), oLocal.begin(), oLocal.end());
  iRet+=2;
  dns_log("Canonical rdata length: %d\n", iRet);

  return iRet;
}
