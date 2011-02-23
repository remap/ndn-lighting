#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>

#include <cstdlib>
#include <string>
#include <algorithm>
#include <list>
#include <iostream>
#include <sstream>
#include <getopt.h>

#include "dns_rr_fact.h"
#include "dns_dnskey.h"
#include "dns_ns.h"
#include "dns_a.h"
#include "dns_resolver.h"
#include "dns_packet.h"
#include "dns_defs.h"

#define FLOOR 512
#define CIELING 4096
#define TIMEOUT_DEFAULT 3
#define RETRY_DEFAULT 2
#define NO_ARGUMENT 0
#define REQUIRED_ARGUMENT 1

using namespace std;

typedef struct dns_pmtu_walk_s
{
  bool m_bSuccess;
  int m_iSmallestBuff;
  int m_iLargestBuff;
  int m_iLargestMsg;
} dns_pmtu_walk_t;

void _version()
{
  fprintf(stdout, "\ndnsfunnel %s (tools@netsec.colostate.edu)\n\n", PACKAGE_VERSION);
}

void _usage()
{
  _version();
  fprintf(stdout, "usage: dnsfunnel [@server] [OPTION] <zone>\n"
  "\nOptions:\n"
  "\t-T  --timeout=<#seconds before timeout>\n"
  "\t-r  --retries=<#retries before giving up>\n"
  "\t-t  --type <type>\n"
  "\t-c  --class <class>\n"
  "\t-v  --version\n"
  "\t-h  --help\n");
}

int parse_arguments(int argc, char *argv[], int* iTimeout, int* iRetries, int* iType, int* iClass)
{
  string sType = "DNSKEY";
  string sClass = "IN";
  int next_option;
  int long_opt_index = 0;
  int longval = 0;
  
  if (argc < 2)
  {
    return -1;
  }
  else if (0 == memcmp("-h", argv[1], 2) || 0 == memcmp("--help", argv[1], 6))
  {
    return -1;
  }
  else if (0 == memcmp("-v", argv[1], 2) || 0 == memcmp("--version", argv[1], 9))
  {
    _version();
    exit(0);
  }
  else if (NULL == argv[1])
  {
    fprintf(stderr, "Name is NULL?");
    return -1;
  }

  const char* const short_options = "T:r:t:c:hv";
  const struct option long_options[] = {
    { "timeout", REQUIRED_ARGUMENT, &longval, 'T' },
    { "retries", REQUIRED_ARGUMENT, &longval, 'r'},
    { "type", REQUIRED_ARGUMENT, &longval, 't'},
    { "class", REQUIRED_ARGUMENT, &longval, 'c'},
    { "help", NO_ARGUMENT, NULL, 'h'},
    { "version", NO_ARGUMENT, NULL, 'v'},
    { NULL, 0, NULL, 0}
  };

  while ((next_option = getopt_long(argc-1, argv, short_options, long_options, &long_opt_index)) != -1){
    switch(next_option) {
      case 'T':
        *iTimeout= strtol(optarg, NULL, 10);
        break;
      case 'r':
        *iRetries = strtol(optarg, NULL, 10);
        break;
      case 't':
        sType = optarg;
        break;
      case 'c':
        sClass = optarg;
        break;
      case 'h':
        return -1;
      case 'v':
        _version();
        exit(0);
      case 0:
        switch (longval) {
          case 'T':
            *iTimeout= strtol(optarg, NULL, 10);
            break;
          case 'r':
            *iRetries = strtol(optarg, NULL, 10);
            break;
          case 't':
            sType = optarg;
            break;
          case 'c':
            sType = optarg;
          case 'h':
            return -1;
          case 'v':
            _version();
            exit(0);
          default:
            return -1;
        }
        break;
      
      default:
        return -1;
    }
  }

  *iType = DnsRrFactory::getInstance().getProtoNum(sType);
  if (!(*iType)){
    fprintf(stderr,"\nUnrecognized type: %s specified.\n", sType.c_str());
    return -1;
  }
  
  *iClass = DnsRrFactory::getInstance().getProtoNum(sClass);
  if (!(*iClass)){
    fprintf(stderr,"\nUnrecognized class: %s specified.\n", sClass.c_str());
    return -1;
  }
  
  return 0;
}

bool pmtuWalk(string &p_sZone, DnsResolver &p_oRes, uint32_t p_uIP, dns_pmtu_walk_t &p_tWalk, int iRetries, int iTimeout, int iType, int iClass)
{
  bool bRet = false;

  DnsPacket oResp;
  p_oRes.setBuffSize(FLOOR);
  p_oRes.setDO(false);
  p_oRes.setNameserver(p_uIP);
  p_oRes.setRetries(3);
  if (!p_oRes.send(p_sZone, DNS_RR_NS, oResp))
  {
    bRet = false;
  }
  else
  {
    int iLastSize = 0;
    int iCeiling = CIELING;
    int iFloor = FLOOR;
    int iDropCeiling = CIELING;

    memset(&p_tWalk, 0, sizeof(p_tWalk));
    p_oRes.setDO(true);

    struct in_addr tAddr;
    memset(&tAddr, 0, sizeof(tAddr));
    tAddr.s_addr = htonl(p_uIP);
    string sIP = inet_ntoa(tAddr);

    p_oRes.setRetries(1);
    int iRetry = 0;

    for (int i = 0; i < 2; i++)
    {
      for (int iSize = iDropCeiling;
           iSize >= iFloor && iSize <= iCeiling && iSize != iLastSize;
          )
      {
//fprintf(stderr, "%d\n", iSize);
        fflush(stdout);
        RRList_t tAns;
        oResp.clear();
        p_oRes.setBuffSize(iSize);

        struct timeval tStart;
        struct timeval tStop;
        memset(&tStart, 0, sizeof(tStart));
        memset(&tStop, 0, sizeof(tStop));

        if (0 == iRetry)
        {
          fprintf(stdout, "\n%s\t%dB\t", sIP.c_str(), iSize);
        }

        gettimeofday(&tStart, NULL);
        bool bQuery = p_oRes.send(p_sZone, iType, iClass, oResp);
        gettimeofday(&tStop, NULL);
        double dDiff = (tStop.tv_sec - tStart.tv_sec) + ((double) (tStop.tv_usec - tStart.tv_usec) / 1000000);

        if (!bQuery)
        {
          fprintf(stdout, "* ");
          // Drop
          if (iRetry < iRetries)
          {
            iRetry++;
            continue;
          }

          iRetry = 0;
          iLastSize = iSize;
          iSize = ((iSize - iFloor) / 2) + iFloor;
          iCeiling = iLastSize;
          iDropCeiling = iCeiling;
        }
        else if (oResp.getHeader().get_tc())
        {
          // Truncated, go up
          fprintf(stdout, "%f (truncated)", dDiff);
          iRetry = 0;
          if (false == p_tWalk.m_bSuccess && p_tWalk.m_iSmallestBuff < iSize)
          {
            p_tWalk.m_iSmallestBuff = iSize;
          }
          iLastSize = iSize;
          iSize = ((iCeiling - iSize) / 2) + iSize;
          iFloor = iLastSize;
        }
        else if (!oResp.getAnswers(tAns))
        {
          fprintf(stdout, "%f (no DNSKEYs)", dDiff);
          iRetry = 0;
          memset(&p_tWalk, 0, sizeof(p_tWalk));
          bRet = false;
          break;
        } 
        else
        {
          iRetry = 0;
          bool bHasKeys = false;

          for (RRIter_t tIter = tAns.begin();
               tAns.end() != tIter;    
               tIter++)
          {
            if ((*tIter)->type() == iType)
            {
              bHasKeys = true;
              break;
            }
          }

          if (!bHasKeys)
          {
            fprintf(stdout, "%f (no DNSKEYs)", dDiff);
            memset(&p_tWalk, 0, sizeof(p_tWalk));
            bRet = false;
            break;
          }

          fprintf(stdout, "%f", dDiff);
          if (!p_tWalk.m_bSuccess)
          {
            memset(&p_tWalk, 0, sizeof(p_tWalk));
          }

          p_tWalk.m_bSuccess = true;
          if (0 == p_tWalk.m_iSmallestBuff
              || p_tWalk.m_iSmallestBuff > iSize)
          {
            p_tWalk.m_iSmallestBuff = iSize;
          }

          if (p_tWalk.m_iLargestBuff < iSize)
          {
            p_tWalk.m_iLargestBuff = iSize;
          }

          if (p_tWalk.m_iLargestMsg < (int) oResp.getRecvLen())
          {
            p_tWalk.m_iLargestMsg = (int) oResp.getRecvLen();
          }

          iLastSize = iSize;

          if (0 == i)
          {
            iSize = ((iSize - iFloor) / 2) + iFloor;
            iCeiling = iLastSize;
          }
          else
          {
            iSize = ((iCeiling - iSize) / 2) + iSize;
            iFloor = iLastSize;
          }
        }
      }
      iCeiling = iDropCeiling;
    }

    bRet = true;
  }
  fprintf(stdout, "\n---------------------------------\n");
  p_oRes.setBuffSize(FLOOR);
  p_oRes.setDO(false);

  return bRet;
}

int main(int argc, char *argv[])
{
  string sDesignatedNameserver = "";
  int iType;
  int iClass;
  list<string> oNameList;
  DnsResolver oRes;
  DnsPacket oResp;

  list<uint32_t> oIpList;
  RRIter_t tIter;
 
  struct in_addr oDesignatedIP;

  if (argc > 2){
    for (int i=1; i<argc-1; ++i){
      if (argv[i][0] == '@'){
        sDesignatedNameserver =  argv[i];
        sDesignatedNameserver = sDesignatedNameserver.substr(1);
        if (inet_pton(AF_INET, sDesignatedNameserver.c_str(), &oDesignatedIP)) {
          oIpList.push_back(htonl(oDesignatedIP.s_addr));
        }
        else {
          oNameList.push_back(sDesignatedNameserver);
        }
      }
    }
  }
  
  string sZone = argv[argc-1];
  int iRet = 1;
  int iRetries = RETRY_DEFAULT;
  int iTimeout = TIMEOUT_DEFAULT;
  if (parse_arguments(argc, argv, &iTimeout, &iRetries, &iType, &iClass) != 0){
    _usage();
    exit(1);
  }

  if (sDesignatedNameserver == "" && !oRes.send(sZone, DNS_RR_NS, oResp))
  {
    fprintf(stderr, "Unable to retrieve NS set for zone '%s'\n", sZone.c_str());
  }
  else if (sDesignatedNameserver == "")
  {
    RRList_t tAns;
    oResp.getAnswers(tAns);
    for (tIter = tAns.begin();
         tAns.end() != tIter;
         tIter++)
    {
      string sNameserver;
      DnsRR *pRR = *tIter;
      if (DNS_RR_NS == pRR->type())
      {
        sNameserver = (static_cast<DnsNs *>(pRR))->getName();
        oNameList.push_back(sNameserver);
      }
    }

    oResp.clear();
  }

  map<uint32_t, string> oIpToNsMap;

  if (oNameList.empty() && oIpList.empty())
  {
    fprintf(stderr, "Unable to find NS records for zone '%s'\n", sZone.c_str());
  }
  else
  {
    for (list<string>::iterator tNameIter = oNameList.begin();
         oNameList.end() != tNameIter;
         tNameIter++)
    {
      string sName = *tNameIter;
      oResp.clear();
      if (!oRes.send(sName, DNS_RR_A, oResp))
      {
        fprintf(stderr, "Unable to locate A record for name server '%s'\n", sName.c_str());
      }
      else
      {
        RRList_t tAns;
        oResp.getAnswers(tAns);
        for (tIter = tAns.begin();
             tAns.end() != tIter;
             tIter++)
        {
          uint32_t uIP = 0;
          DnsRR *pRR = *tIter;
          if (DNS_RR_A == pRR->type())
          {
            uIP = (static_cast<DnsA *>(pRR))->ip();
            oIpList.push_back(uIP);
            oIpToNsMap[uIP] = sName;
          }
        }
      }
    }
  }

  if (oIpList.empty() && sDesignatedNameserver == "")
  {
    fprintf(stderr, "Unable to locate any name server IPs for zone: '%s'\n", sZone.c_str());
  }

  else
  {
    if (oIpList.empty() && sDesignatedNameserver != ""){
      fprintf(stderr, "Unable to locate designated nameserver(s): ");
      for (list<string>::iterator tNameIter = oNameList.begin();
           oNameList.end() != tNameIter;
           tNameIter++)
      {
        fprintf(stderr, "%s, ", (*tNameIter).c_str());
      }
      fprintf(stderr, "\n");
      return iRet;
    }          

    oRes.setTimeout(iTimeout);
    ostringstream oSS;
    for (list<uint32_t>::iterator tIpIter = oIpList.begin();
         oIpList.end() != tIpIter;
         tIpIter++)
    {
      uint32_t uIP = *tIpIter;
      dns_pmtu_walk_t tWalk;
      memset(&tWalk, 0, sizeof(tWalk));

      struct in_addr tAddr;
      memset(&tAddr, 0, sizeof(tAddr));
      tAddr.s_addr = htonl(uIP);

      if (!pmtuWalk(sZone, oRes, uIP, tWalk, iRetries, iTimeout, iType, iClass))
      {
        fprintf(stderr, "Unable to send walk to name server %s for zone '%s'\n", inet_ntoa(tAddr), sZone.c_str());
      }
      else
      {
        string sName = oIpToNsMap[uIP];
        const char *szFit = (tWalk.m_bSuccess) ? "yes  " : "no   ";
        oSS << sName
            << "\n\t"
            << inet_ntoa(tAddr) 
            << "\t" 
            << szFit 
            << "\t" 
            << tWalk.m_iSmallestBuff 
            << "\t" 
            << tWalk.m_iLargestBuff 
            << "\t" 
            << tWalk.m_iLargestMsg 
            << endl
            << endl;
      }
    }

    fprintf(stdout, "PMTU walking summary:\n======================================================\n");
    fprintf(stdout, "Name   \t           \tKeys\tSmall \tLargest\tOptimal\n");
    fprintf(stdout, "Server \t  IP         \tfit?\tBuffer\tBuffer \tBuffer\n");
    fprintf(stdout, "------------------------------------------------------\n%s", oSS.str().c_str());
    iRet = 0;
  }
  

  return iRet;
}
