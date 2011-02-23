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
#include "ps_logger.h"
#ifndef __USE_BSD
#define __USE_BSD
#endif

#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <pthread.h>

#include <list>
#include <string>
#include <map>
#include <sstream>
#include <iostream>

#include "ps_defs.h"
#include "dns_packet.h"
#include "dns_task.h"
#include "dns_resolver.h"
#include "dns_rr.h"
#include "dns_a.h"
#include "dns_name.h"
#include "dns_ns.h"

using namespace std;

typedef list<DnsResolver *> ResList_t;
typedef ResList_t::iterator ResIter_t;

ResList_t g_oResList;
pthread_mutex_t g_tMutex;
//list<sockaddr_in> g_oNsList;
map<string, int> g_oSeenMap;
list<string> g_oSeenList;
int g_iSeenMax;
int g_iSeenCount;
bool g_bRun = true;

bool _seenRecently(DnsPacket &p_oPkt, uint32_t p_uIP)
{
  bool bRet = false;

  string sKey = DnsResolver::makeKey(p_oPkt, p_uIP);
  map<string, int>::iterator tIter = g_oSeenMap.find(sKey);
  if (g_oSeenMap.end() == tIter)
  {
    if (g_iSeenCount == g_iSeenMax)
    {
      string sTmp = g_oSeenList.back();
      g_oSeenMap.erase(sTmp);
      g_oSeenList.pop_back();
    }
    else
    {
      g_iSeenCount++;
    }

    g_oSeenList.push_front(sKey);
    g_oSeenMap[sKey] = 1;
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

void _reflect(u_char *useless,
              const struct pcap_pkthdr* pkthdr,
              const u_char* packet)
{
//  ps_elog(PSL_CRITICAL, "IN\n");
  uint16_t uType = ntohs(((struct ether_header *) packet)->ether_type);
  if (ETHERTYPE_IP != uType)
  {
    ps_elog(PSL_CRITICAL, "Incorrect ETHERTYPE: 0x%x\n", uType);
  }
  else
  {
    struct ip *pIP = (struct ip *) (packet + sizeof(struct ether_header));
    if (4 != (pIP->ip_v & 0x00ff))
    {
      ps_elog(PSL_CRITICAL, "Got IP header version: %u (not 4)\n", (pIP->ip_v & 0x00ff));
    }
    else if (5 > pIP->ip_hl & 0x00ff)
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
      u_char *pPkt = (u_char *) (packet + sizeof(struct ether_header) + uHdrLen + sizeof(udphdr));
      RRList_t tQs;
      DnsPacket oPkt;
//ps_elog(PSL_CRITICAL, "NBO LEN: %u, HBO LEN: %u, HDRLEN: %u, UDPHDR LEN: %u, PASSING LEN: %u\n", pIP->ip_len, ntohs(pIP->ip_len), uHdrLen, sizeof(struct udphdr), uDataLen);
      if (!oPkt.fromWire(pPkt, uDataLen))
      {
        ps_elog(PSL_CRITICAL, "Unable to create packet.\n");
      }
      else if (!oPkt.getHeader().response())
      {
        oPkt.getQuestions(tQs);
        DnsRR *pRR = tQs.front();
        if (1 == tQs.size()
            && pRR->type() == DNS_RR_A)
        {
/*
          for (list<sockaddr_in>::iterator tIter = g_oNsList.begin();
               g_oNsList.end() != tIter;
               tIter++)
          {
            uint32_t uIP = (*tIter).sin_addr.s_addr;
*/
            for (ResIter_t tResIter = g_oResList.begin();
                 g_oResList.end() != tResIter;
                 tResIter++)
            {
              DnsResolver *pRes = *tResIter;
              DnsTask *pTask = new DnsTask();
//              pTask->setNameserver(uIP);
//              pTask->setPort((*tIter).sin_port);
              DnsPacket *pTmpPkt = new DnsPacket;
              pTmpPkt->fromWire(pPkt, uDataLen);
              pTask->setQuery(pTmpPkt);
              pthread_mutex_lock(&g_tMutex);
              if (_seenRecently(*pTmpPkt, pRes->getNameserver()))
              {
                delete pTask;
              }
              else if (!pRes->send(pTask))
              {
                ps_elog(PSL_CRITICAL, "Unable to send task.\n");
//              pTmpPkt->print();
                delete pTask;
              }
              pthread_mutex_unlock(&g_tMutex);
            }
/*
          }
*/
        }
      }
    }
  }
//  ps_elog(PSL_CRITICAL, "OUT\n");
}

void *_recv(void *p_pBlah)
{
  bool bRun = false;
  while (g_bRun || bRun)
  {
    bRun = false;
    bool bSleep = true;
    for (ResIter_t tResIter = g_oResList.begin();
         g_oResList.end() != tResIter;
         tResIter++)
    {
    pthread_mutex_lock(&g_tMutex);
      DnsResolver *pRes = *tResIter;
    if (pRes->hasTasks())
    {
      bRun = true;
      DnsTask *pTask = NULL;
      pTask = pRes->recv();
      if (NULL != pTask)
      {
        bSleep = false;
        DnsPacket *pPkt = pTask->getResponse();
        if (NULL != pPkt
            && pPkt->getHeader().get_rcode() == DNS_NOERROR
            && !pPkt->getHeader().get_tc())
        {
          ostringstream oSS;
          uint32_t uNsIP = pTask->getNameserver();
          RRList_t tQs;
          pPkt->getQuestions(tQs);
          oSS << (int) ((uNsIP >> 24) & 0x00ff)
               << "."
               << (int) ((uNsIP >> 16) & 0x00ff)
               << "."
               << (int) ((uNsIP >> 8) & 0x00ff)
               << "."
               << (int) ((uNsIP) & 0x00ff)
               << "\t"
               << (unsigned) time(NULL)
               << "\t"
               << tQs.front()->get_name()->toString()
               << "\t";

/*
          fprintf(stdout, "%d.%d.%d.%d\t%u\t%s\t",
                  (uNsIP >> 24) & 0x00ff,
                  (uNsIP >> 16) & 0x00ff,
                  (uNsIP >> 8) & 0x00ff,
                  (uNsIP) & 0x00ff,
                  (unsigned) time(NULL),
                  tQs.front()->get_name()->toString().c_str());
*/

          uint32_t uTTL = 0;
          int i = 0;
          list<string> oIpList;
          RRList_t tAns;
          pPkt->getAnswers(tAns);
          RRIter_t tIter;
          for (tIter = tAns.begin();
               tAns.end() != tIter;
               tIter++)
          {
            DnsRR *pRR = *tIter;
            if (pRR->type() == DNS_RR_A)
            {
              i++;
              struct in_addr tAddr;
              tAddr.s_addr = htonl(((DnsA *) pRR)->ip());
              string sIP = inet_ntoa(tAddr);
              oIpList.push_back(sIP);
              uTTL = pRR->ttl();
            }
          }
          oIpList.sort();

          int j = 0;
          RRList_t tAuth;
          list<string> oNsList;
          pPkt->getAuthority(tAuth);
          for (tIter = tAuth.begin();
               tAuth.end() != tIter;
               tIter++)
          {
            DnsRR *pRR = *tIter;
            if (pRR->type() == DNS_RR_NS)
            {
              j++;
              string sNS = ((DnsNs *) pRR)->getName();
              transform(sNS.begin(), sNS.end(), sNS.begin(), ::tolower);
              oNsList.push_back(sNS);
            }
          }
          oNsList.sort();

          if (i > 0)
          {
            oSS << (unsigned) uTTL << "\t";
            list<string>::iterator tStrIter;
            for (tStrIter = oIpList.begin();
                 oIpList.end() != tStrIter;
                 tStrIter++)
            {
              if (oIpList.begin() != tStrIter)
              {
                oSS << ",";
              }
              oSS << *tStrIter;
            }

            oSS << "\t";

            for (tStrIter = oNsList.begin();
                 oNsList.end() != tStrIter;
                 tStrIter++)
            {
              if (oNsList.begin() != tStrIter)
              {
                oSS << ",";
              }
              oSS << *tStrIter;
            }

            fprintf(stdout, "%s\n", oSS.str().c_str());
          }
//          fprintf(stdout, "\n");
        }

        delete pTask;
      }
    }
    pthread_mutex_unlock(&g_tMutex);
    }

    if (bSleep)
    {
      usleep(2000);
    }
  }

  return NULL;
}

void _usage()
{
  printf("reflector [ -f <tracefile> <timefmt> <statfile>  <ns list> ] | [ -p <program> <ns list> [ <device> ] ] | [ -h ]\n");
}

bool _runFromFile(char *p_szFile, char *p_szTimeFmt, char *p_szStatusFile)
{
  bool bRet = false;

  FILE *pTraceFile = NULL;
  FILE *pStatFile = NULL;
  if (NULL == p_szFile)
  {
    dns_log("Unable to open NULL file.\n");
  }
  else if (NULL == p_szStatusFile)
  {
    dns_log("Unable to open with NULL status file.\n");
  }
  else if (NULL == p_szTimeFmt)
  {
    dns_log("Unable to process without time format.\n");
  }
  else if (NULL == (pTraceFile = fopen(p_szFile, "r")))
  {
    dns_log("Unable to open tracefile: '%s'\n", p_szFile);
  }
  else if (NULL == (pStatFile = fopen(p_szStatusFile, "r+"))
           && NULL == (pStatFile = fopen(p_szStatusFile, "w+")))
  {
    dns_log("Unable to open stat file: '%s'\n", p_szStatusFile);
    fclose(pTraceFile);
  }
  else
  {
    struct tm tTime;
    char szLine[4096];
    memset(szLine, 0, 4096);
    time_t tLast = 0;

    rewind(pStatFile);
    if (NULL != fgets(szLine, 4096, pStatFile))
    {
      tLast = (int) strtol(szLine, NULL, 10);
      memset(szLine, 0, 4096);
      rewind(pStatFile);
    }

    while (NULL != fgets(szLine, 4096, pTraceFile))
    {
      bRet = true;

      time_t tNow = 0;
      memset(&tTime, 0, sizeof(tTime));
      if (NULL == strptime(szLine, p_szTimeFmt, &tTime))
      {
        dns_log("Unable to get time from line: '%s'\n", szLine);
      }
      else if (tLast <= (tNow = mktime(&tTime)))
// else if (tLast > (tNow = mktime(&tTime)))
// {
//   fprintf(stderr, "Skipping old line: %d > %d\n", (int) tLast, (int) tNow);
// }
// else
      {
        int iSleep = (0 == tLast) ? 0 : tNow - tLast;
        if (iSleep > 0)
        {
          sleep(iSleep);
        }

        string sLine = szLine;
        size_t uPos = sLine.find_last_of(" ");
        if (string::npos != uPos)
        {
          string sName = sLine.substr(uPos + 1);
          if (string::npos != (uPos = sName.find("\n")))
          {
            sName.erase(uPos);
          }
          DnsName oName(sName);
/*
          for (list<sockaddr_in>::iterator tIter = g_oNsList.begin();
               g_oNsList.end() != tIter;
               tIter++)
          {
            uint32_t uIP = (*tIter).sin_addr.s_addr;
*/
            for (ResIter_t tResIter = g_oResList.begin();
                 g_oResList.end() != tResIter;
                 tResIter++)
            {
              DnsResolver *pRes = *tResIter;
            DnsTask *pTask = new DnsTask();
//            pTask->setNameserver(uIP);
//            pTask->setPort((*tIter).sin_port);
pTask->setNameserver(pRes->getNameserver());
pTask->setPort(pRes->getPort());
            DnsPacket *pTmpPkt = new DnsPacket(true);
            pTmpPkt->addQuestion(*(DnsRR::question(oName, DNS_RR_A)));
            pTask->setQuery(pTmpPkt);
            pthread_mutex_lock(&g_tMutex);
            if (!pRes->send(pTask))
            {
              ps_elog(PSL_CRITICAL, "Unable to send task.\n");
              delete pTask;
            }
            pthread_mutex_unlock(&g_tMutex);
            }
/*
          }
*/
        }
        rewind(pStatFile);
        fprintf(pStatFile, "%d\n", (int) tNow);
        tLast = tNow;
      }
    }
  }

  return bRet;
}

bool _loadNSes(string p_sNSes)
{
  size_t u = 0;
  size_t u2 = 0;
  do
  {
    u = p_sNSes.find(",");
    struct sockaddr_in tAddr;
    string sNS = p_sNSes.substr(0, u);
    u2 = sNS.find(":");
    if (string::npos == u2)
    {
      tAddr.sin_port = 53;
    }
    else
    {
      tAddr.sin_port = atoi(sNS.substr(u2 + 1).c_str());
      sNS = sNS.substr(0, u2);
    }

    if (0 == inet_aton(sNS.c_str(), &(tAddr.sin_addr)))
    {
      ps_elog(PSL_CRITICAL, "Can't convert: '%s'\n", sNS.c_str());
    }
    else
    {
//      tAddr.sin_addr.s_addr = ntohl(tAddr.sin_addr.s_addr);
//      g_oNsList.push_back(tAddr);
      DnsResolver *pRes = new DnsResolver();
      pRes->setNameserver(ntohl(tAddr.sin_addr.s_addr));
      pRes->setPort(tAddr.sin_port);
//fprintf(stderr, "SETTING IP:PORT = %s:%u\n", inet_ntoa(tAddr.sin_addr), tAddr.sin_port);
      pRes->setConcurrency(500);
      pRes->setTimeout(4);
      g_oResList.push_back(pRes);
    }
    p_sNSes.erase(0, u + 1);
  } while (string::npos != u);

  return true;
}

int main(int argc, char *argv[])
{
  int iRet = 1;
  char *dev = NULL; 
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t* descr;
  struct bpf_program fp;      /* hold compiled program     */
  bpf_u_int32 maskp;          /* subnet mask               */
  bpf_u_int32 netp;           /* ip                        */

  g_iSeenMax = 1000;
  g_iSeenCount = 0;

  if(argc < 3)
  {
    fprintf(stderr, "Noit enough arguments\n");
    _usage();
  }
  else if (0 == strcmp(argv[1], "-h"))
  {
    _usage();
    iRet = 0;
  }
  else if (0 != strcmp(argv[1], "-f") && 0 != strcmp(argv[1], "-p"))
  {
    fprintf(stderr, "Unable to process command line.\n");
    _usage();
  }
  else if (0 == strcmp(argv[1], "-f"))
  {
    if (argc != 6)
    {
      fprintf(stderr, "Wrong nuymber of parameter: %d\n", argc);
      _usage();
    }
    else
    {
      string sList = argv[5];
      _loadNSes(sList);

      pthread_t tID;
      int iErr = pthread_create(&tID, NULL, _recv, NULL);
      if (0 != iErr)
      {
        ps_elog(PSL_CRITICAL, "Could not spawn thread.\n");
      }

      bool bResult = _runFromFile(argv[2], argv[3], argv[4]);
      if (!bResult)
      {
        fprintf(stderr, "Unable to _runFromFile().\n");
      }

      sleep(1);
      g_bRun = false;
      pthread_join(tID, NULL);
    }
  }
  else
  {
    string sList = argv[3];
    _loadNSes(sList);

    if (argc == 5)
    {
      dev = argv[4];
    }
    else
    {
      /* grab a device to peak into... */
      dev = pcap_lookupdev(errbuf);
      if(dev == NULL)
      {
        fprintf(stderr,"%s\n",errbuf); exit(1);
      }
    }

    pthread_t tID;
    int iErr = pthread_create(&tID, NULL, _recv, NULL);
    if (0 != iErr)
    {
      ps_elog(PSL_CRITICAL, "Could not spawn thread.\n");
    }

    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(dev,&netp,&maskp,errbuf);

    descr = pcap_open_live(dev,BUFSIZ,0,100,errbuf);
    if(descr == NULL)
    { printf("pcap_open_live(): %s\n",errbuf); exit(1); }

    /* Lets try and compile the program.. non-optimized */
    if(pcap_compile(descr,&fp,argv[2],0,netp) == -1)
    { fprintf(stderr,"Error calling pcap_compile\n"); exit(1); }

    /* set the compiled program as the filter */
    if(pcap_setfilter(descr,&fp) == -1)
    { fprintf(stderr,"Error setting filter\n"); exit(1); }

    ps_elog(PSL_CRITICAL, "DEV is: '%s'\n", dev);
    while (1)
    {
//ps_elog(PSL_CRITICAL, "PRE\n");
      pcap_loop(descr,1,_reflect,NULL);
//ps_elog(PSL_CRITICAL, "POST\n");
    }

    iRet = 0;
  }

  return iRet;
}

