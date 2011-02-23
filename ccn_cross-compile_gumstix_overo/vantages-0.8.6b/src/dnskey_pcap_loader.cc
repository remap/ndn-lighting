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

#include <pcap.h>

#ifndef __USE_BSD
#define __USE_BSD
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>

#include <stdio.h>

#include "dnskey_pcap_loader.h"
#include "dnskey_app.h"
#include "dnskey_defs.h"
#include "dao_factory.h"
#include "ps_logger.h"
#include "ps_config.h"
#include "ps_defs.h"

#include "dns_packet.h"
#include "dns_task.h"
#include "dns_resolver.h"
#include "dns_rr.h"
#include "dns_a.h"
#include "dns_name.h"
#include "dns_ns.h"

using namespace std;

void _get_ns(u_char *p_pHandle,
             const struct pcap_pkthdr *p_pHdr,
             const u_char *p_pPkt)
{
  uint16_t uType = ntohs(((struct ether_header *) p_pPkt)->ether_type);
  if (ETHERTYPE_IP != uType)
  {
    ps_elog(PSL_WARNING, "Incorrect ETHERTYPE: 0x%x\n", uType);
  }
  else if (p_pHdr->caplen < p_pHdr->len)
  {
    ps_elog(PSL_INFO, "Packet is a fragment and to be safe we are discarding (%u < %u)...\n",
            p_pHdr->caplen,
            p_pHdr->len);
  }
  else
  {
    struct ip *pIP = (struct ip *) (p_pPkt + sizeof(struct ether_header));
    if (4 != (pIP->ip_v & 0x00ff))
    {
      ps_elog(PSL_CRITICAL, "Got IP header version: %u (not 4)\n", (pIP->ip_v & 0x00ff));
    }
    else if (5 > (pIP->ip_hl & 0x00ff))
    {
      ps_elog(PSL_CRITICAL, "Header len is too short: %u\n", pIP->ip_hl & 0x00ff);
    }
    else if (pIP->ip_hl >= ntohs(pIP->ip_len))
    {
      ps_elog(PSL_CRITICAL, "Header as long or longer than packet? %u >= %u\n", pIP->ip_hl, ntohs(pIP->ip_len));
    }
    else if (IPPROTO_UDP != pIP->ip_p)
    {
      ps_elog(PSL_CRITICAL, "Only processing UDP for now.\n");
    }
    else
    {
      size_t uHdrLen = pIP->ip_hl * 4;
      size_t uDataLen = ntohs(pIP->ip_len) - uHdrLen - sizeof(struct udphdr);
      u_char *pPkt = (u_char *) (p_pPkt + sizeof(struct ether_header) + uHdrLen + sizeof(udphdr));
      RRList_t tRRs;
      RRIter_t tIter;
      DnsPacket oPkt;

      if (!oPkt.fromWire(pPkt, uDataLen))
      {
        ps_elog(PSL_WARNING, "Unable to create packet.\n");
//oPkt.print();
      }
      else
      {
        oPkt.getAnswers(tRRs);
        for (tIter = tRRs.begin();
             tRRs.end() != tIter;
             tIter++)
        {
          DnsRR *pRR = *tIter;
          if (pRR->type() == DNS_RR_NS)
          {
            string sName = pRR->get_name()->toString();
            ((DnskeyPcapLoader *) p_pHandle)->checkName(sName);
          }
        }

        tRRs.clear();
        oPkt.getAuthority(tRRs);
        for (tIter = tRRs.begin();
             tRRs.end() != tIter;
             tIter++)
        {
          DnsRR *pRR = *tIter;
          if (pRR->type() == DNS_RR_NS)
          {
            string sName;
            pRR->get_name()->display_name(sName, true);
            ((DnskeyPcapLoader *) p_pHandle)->checkName(sName);
          }
        }
      }
/*
*/
    }
  }
}

DnskeyPcapLoader::DnskeyPcapLoader()
  : m_bInit(false),
    m_pApp(NULL),
    m_pHandle(NULL)
{
  memset(&m_tProg, 0, sizeof(m_tProg));
}

DnskeyPcapLoader::~DnskeyPcapLoader()
{
  if (NULL != m_pHandle)
  {
    pcap_close(m_pHandle);
    m_pHandle = NULL;
  }

  pcap_freecode(&m_tProg);
}

bool DnskeyPcapLoader::init(DnskeyApp &p_oApp)
{
  m_pApp = &p_oApp;

  PsConfig &oConfig = PsConfig::getInstance();
  struct bpf_program tProg;
  bpf_u_int32 uNet;
  bpf_u_int32 uMask;

  const char *szProgram = NULL;
  const char *szDev = oConfig.getValue(DNSKEY_CONFIG_PCAP_DEV);
  char szErrBuff[PCAP_ERRBUF_SIZE];
  memset(szErrBuff, 0, PCAP_ERRBUF_SIZE);
  memset(&tProg, 0, sizeof(tProg));

  m_bInit = false;

  if (NULL == (szProgram = oConfig.getValue(DNSKEY_CONFIG_PCAP_PROGRAM)))
  {
    ps_elog(PSL_CRITICAL, "Unable to run pcap loader without program in config file for key: '%s'\n", DNSKEY_CONFIG_PCAP_PROGRAM);
  }
  else if (NULL == szDev
           && NULL == (szDev = pcap_lookupdev(szErrBuff)))
  {
    ps_elog(PSL_CRITICAL, "Unable to lookup pcap device (try setting it in the config file w/ key '%s': '%s'\n",
            DNSKEY_CONFIG_PCAP_DEV,
            szErrBuff);
  }

  else if (-1 == pcap_lookupnet((char *) szDev, &uNet, &uMask, szErrBuff))
  {
    ps_elog(PSL_CRITICAL, "Unable to lookup net and mask: '%s'\n", szErrBuff);
  }
  else if (NULL == (m_pHandle = pcap_open_live((char *) szDev, DNSKEY_PCAP_BUFF_SIZE, 0, 100, szErrBuff)))
  {
    ps_elog(PSL_CRITICAL, "Unable to open live pcap handle: %s\n", szErrBuff);
  }
  else if (-1 == pcap_compile(m_pHandle, &m_tProg, (char *) szProgram, 0, uNet))
  {
    ps_elog(PSL_CRITICAL, "Unable to compile program: '%s'\n", szProgram);
  }
  else if (-1 == pcap_setfilter(m_pHandle, &m_tProg))
  {
    ps_elog(PSL_CRITICAL, "Unable to set filter.\n");
  }
  else
  {
    m_bInit = true;
  }

  return m_bInit;
}

bool DnskeyPcapLoader::checkName(std::string &p_sName)
{
  bool bRet = false;

  string sURL = "dnssec:";
  sURL += p_sName + "?type=48&class=1";
  if (NULL == m_oCache.get(p_sName))
  {
    ps_elog(PSL_DEBUG, "Adding new zone to hopper: '%s'\n", p_sName.c_str());
    m_pApp->addNewTarget(p_sName, sURL, DNSKEY_DEFAULT_DNS_SCRAPER_ID, DNSKEY_SRC_PCAP);
    string *pStr = new string(p_sName);
    m_oCache.add(p_sName, *pStr);

    bRet = true;
  }

  return bRet;
}

bool DnskeyPcapLoader::run()
{
  bool bRet = false;

  if (!m_bInit)
  {
    ps_elog(PSL_CRITICAL, "Unable to run pcap loader without call init().\n");
  }
  else
  {
    bRet = true;
    PsConfig &oConfig = PsConfig::getInstance();
    ps_elog(PSL_DEBUG, "Running pcap on device '%s' with program '%s'\n", 
            oConfig.getValue(DNSKEY_CONFIG_PCAP_DEV),
            oConfig.getValue(DNSKEY_CONFIG_PCAP_PROGRAM));

    while (getRun())
    {
      pcap_loop(m_pHandle, 1, _get_ns, (u_char *) this);
    }
  }


  return bRet;
}

