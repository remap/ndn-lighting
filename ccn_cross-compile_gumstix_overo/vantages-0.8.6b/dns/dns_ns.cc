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
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include <errno.h>
#include <string.h>

#include "dns_rr.h"
#include "dns_name.h"
#include "dns_ns.h"
#include "dns_defs.h"

using namespace std;

DnsNs::DnsNs()
  : DnsRR(DNS_RR_NS)
{
}

/*
DnsNs::DnsNs(const DnsName &name, std::string &p_sName)
  : DnsRR(DNS_RR_NS)
{
  DnsNs(name, (int) p_sName.size(), (char *) p_sName.c_str());
}

DnsNs::DnsNs(const DnsName &name, int p_iLen, char *p_pRData)
  : DnsRR(DNS_RR_NS)
{
  init(new DnsName(name), DNS_CLASS_INNS_RR_NS, 300, p_iLen, (u_char *)p_pRData);
}
*/

DnsNs::~DnsNs()
{

}

bool DnsNs::rdata_valid()
{
  return true;
}

std::string &DnsNs::getName()
{
  return m_sName;
}

void DnsNs::setName(std::string &p_sName)
{
  m_sName = p_sName;
  set_rdata((u_char *) p_sName.c_str(), p_sName.size());
}

bool DnsNs::parseRData(u_char *p_pMsg,
                       size_t p_uMsgLen,
                       u_char *p_pRData,
                       size_t p_uRDataLen)
{
  bool bRet = false;

//  u_char *pEOM = &(p_pMsg[p_uMsgLen]);

  set_rdata(p_pRData, p_uRDataLen);

  char szDname[NS_MAXDNAME + 1];
  memset(szDname, 0, NS_MAXDNAME + 1);

  size_t uOffset =  (size_t) (p_pRData - p_pMsg);
  DnsName *pName = DnsName::from_wire(p_pMsg, p_uMsgLen, uOffset);
  if (NULL == pName)
  {
    dns_log("Unable to DnsName::from_wire()\n");
  }
  else
  {
    std::string sName = pName->toString();
    setName(sName);
    bRet = true;
    delete pName;
  }

/*
//fprintf(stderr, "Message %x len is: %u\n", (unsigned)p_pMsg, (unsigned) p_uMsgLen);
//fprintf(stderr, "RData %x len is: %u\n", (unsigned)p_pRData, (unsigned) p_uRDataLen);
//fprintf(stderr, "EOM %x\n", (unsigned)pEOM);
  int iLen = 0;
  if (p_uRDataLen > 0 && -1 == (iLen = dn_expand(p_pMsg, pEOM, p_pRData, szDname, NS_MAXDNAME)))
  {
//fprintf(stderr, "Name is: %s\n", (char *)p_pRData);
    dns_log("Unable to dn_expand(): %s\n", strerror(errno));
  }
  else
  {
//fprintf(stderr, "Name is: %s\n", (char *)p_pRData);
//fprintf(stderr,"BYTES: ");
//for(int i=0;i<(int)p_uRDataLen;i++)
//{
//  fprintf(stderr,"%x:",(unsigned)p_pRData[i]);
//}
//fprintf(stderr,"\n");
//fprintf(stderr, "NAME HAD LEN %d AND BECAME: \"%s\"\n", iLen, szDname);
    std::string sName(szDname);
    setName(sName);
    bRet = true;
  }
*/

  return bRet;
}

DnsNs *DnsNs::dup()
{
  return new DnsNs();
}

void DnsNs::printRData()
{
  fprintf(stdout, "%s", getName().c_str());
}

int DnsNs::verificationRData(DnsBits_t &p_oVec)
{
  int iRet = 0;

  u_char pBuff[255];
  memset(pBuff, 0, 255);
  string &sName = getName();
  DnsName oName(sName);
  iRet = oName.to_wire_canonical(pBuff, 255);

  DnsBits_t oLocal;

  for (int i = 0; i < iRet; i++)
  {
    oLocal.push_back(pBuff[i]);
  }

  uint16_t uRDLen = htons(iRet);
  p_oVec.push_back((htons(uRDLen) >> 8) & 0x00ff);
  p_oVec.push_back(htons(uRDLen) & 0x00ff);
  p_oVec.insert(p_oVec.end(), oLocal.begin(), oLocal.end());
  iRet+=2;
  dns_log("Canonical rdata length: %d\n", iRet);

  return iRet;
}
