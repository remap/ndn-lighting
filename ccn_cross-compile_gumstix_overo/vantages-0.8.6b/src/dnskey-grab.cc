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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <stdarg.h>

#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "base64.h"
#include "dns_packet.h"
#include "dns_defs.h"
#include "dns_rr.h"
#include "dns_name.h"
#include "dns_ns.h"
#include "dns_a.h"
#include "dns_dnskey.h"
#include "dns_opt.h"
#include "dns_resolver.h"
#include "dns_verifier.h"
#include "dns_err.h"

#define MSG_LEN 4096
#define NAME_LEN 1024
#define DEFAULT_TIMEOUT 5
#define DEFAULT_RETRIES 2

using namespace std;

typedef map< string, list<string> > NsKeyMap_t;
typedef NsKeyMap_t::iterator NsKeyIter_t;

typedef map<string, uint32_t> NsIpMap_t;
typedef NsIpMap_t::iterator NsIpIter_t;

void _usage(void)
{
  fprintf(stdout, "Usage: dnskey-grab [ OPTIONS ] <zone> | -h\n");
  fprintf(stdout, "\nOPTIONS:\n");
  fprintf(stdout, "      -a:               Check consistency of key across all name servers\n");
  fprintf(stdout, "      -v:               Verbose output\n");
  fprintf(stdout, "      -t <timeout>:     Timeout (in seconds) for DNS queries (default %d)\n", DEFAULT_TIMEOUT);
  fprintf(stdout, "      -r <retries>:     Number of retries for DNS queries (default %d)\n", DEFAULT_TIMEOUT);
  fprintf(stdout, "      -h:               Help (this message)\n");
}

void _fprintf(bool p_bVerbose, FILE *p_pFile, const char *p_szFormat, ...)
{
  if (p_bVerbose)
  {
    va_list pList;
    va_start(pList, p_szFormat);
    vfprintf(p_pFile, p_szFormat, pList);
    va_end(pList);
  }
}

int main(int argc, char *argv[])
{
  bool bAllNameServers = false;
  bool bVerbose = false;
  bool bUsage = false;
  int iRet = 0;
  int iTimeout = DEFAULT_TIMEOUT;
  int iRetries = DEFAULT_RETRIES;
  char *szZone = NULL;

  srand(time(NULL));

  int c;
  while (-1 != (c = getopt(argc - 1, argv, "avt:r:h")))
  {
    switch (c)
    {
      case 'a':
        bAllNameServers = true;
        break;
      case 'v':
        bVerbose = true;
        break;
      case 'h':
        bUsage = true;
        break;
      case 't':
        iTimeout = (int) strtol(optarg, NULL, 10);
        break;
      case 'r':
        iRetries = (int) strtol(optarg, NULL, 10);
        break;
      default:
        fprintf(stderr, "Unknown option: %c\n", c);
        _usage();
        iRet = 1;
        break;
    }
  }

  if (0 != iRet)
  {
    fprintf(stderr, "Exiting...\n");
  }
  else if (argc <= 1)
  {
    fprintf(stderr, "No parameters...\n");
    _usage();
    iRet = 1;
  }
  else if (bUsage || 0 == strncmp(argv[1], "-h", 2))
  {
    _usage();
    iRet = 0;
  }
  else if (0 >= iTimeout)
  {
    fprintf(stderr, "Invalid timeout.\n");
    _usage();
    iRet = 1;
  }
  else if (0 > iRetries)
  {
    fprintf(stderr, "Invalid number of retries: %d\n", iRetries);
  }
  else
  {
    szZone = argv[argc - 1];
    char *szTmp = new char[strlen(szZone) + 1];
    strcpy(szTmp, szZone);
    szZone = szTmp;

    string sZone(szZone);

    DnsPacket oResp;
    _fprintf(bVerbose, stdout, "Creating NS query.\n");
    DnsPacket oNsPkt(true, -1);
    DnsName oName(sZone);
    DnsRR *pNsRR = DnsRR::question(oName, DNS_RR_NS);
    oNsPkt.addQuestion(*pNsRR);

    if (bVerbose)
    {
      _fprintf(bVerbose, stdout, "NS request pkt:\n");
      oNsPkt.print();
    }

    // Start by getting the NS set for this zone.
    DnsResolver oRes;

    oRes.setBuffSize(4096);
    oRes.setDO(true);
    oRes.setTimeout(iTimeout);
    oRes.setRetries(iRetries + 1);

    NsKeyMap_t oNsMap;
    std::string sNS;
    _fprintf(bVerbose, stdout, "Sending NS query.\n");
    if (!oRes.send(oNsPkt, oResp))
    {
      _fprintf(bVerbose, stderr, "Unable to send()\n");
    }
    else
    {
      RRIter_t oRrIter;

      if (bVerbose)
      {
        _fprintf(bVerbose, stdout, "NS response pkt:\n");
        oResp.print();

        RRList_t oQuest;
        oResp.getQuestions(oQuest);
        for (oRrIter = oQuest.begin();
             oQuest.end() != oRrIter;
             oRrIter++)
        {
          (*oRrIter)->print();
        }
      }

      // Look through the answer section
      RRList_t oAns;
      oResp.getAnswers(oAns);
      int uSize = oAns.size();
      for (oRrIter = oAns.begin();
           oAns.end() != oRrIter;
           oRrIter++)
      {
        // Find the NS records.
        DnsRR *pAnsRR = *oRrIter;
        if (DNS_RR_NS == pAnsRR->type())
        {
          sNS = ((DnsNs *) pAnsRR)->getName();
          list<string> oList;
          oNsMap[sNS] = oList;

          // If we're not checking all NSes, we can just pick one to use
          if (!bAllNameServers && (rand() % uSize) == 0)
          {
            break;
          }
        }
      }

      // If we're tryng to find ALL NSes or there were no NSes in the ans, look in the auth section
      if (bAllNameServers || sNS.size() == 0)
      {
        RRList_t oAuth;
        oResp.getAuthority(oAuth);
        for (oRrIter = oAuth.begin();
            oAuth.end() != oRrIter;
            oRrIter++)
        {
          DnsRR *pAuthRR = *oRrIter;
          if (DNS_RR_NS == pAuthRR->type())
          {
            sNS = ((DnsNs *) pAuthRR)->getName();
            list<string> oList;
            oNsMap[sNS] = oList;

            if (!bAllNameServers)
            {
              // We can just pick one to use
              break;
            }
          }
        }
      }
    }

    uint32_t uIP = 0;
    NsIpMap_t oIpMap;

    // Did we find any NSes?
    if (sNS.size() == 0)
    {
      fprintf(stderr, "Got no name servers back.\n");
    }
    else
    {
      if (!bAllNameServers)
      {
        list<string> oTmpList;
        oNsMap.clear();
        oNsMap[sNS] = oTmpList;
      }

      for (NsKeyIter_t oIter = oNsMap.begin();
           oNsMap.end() != oIter;
           oIter++)
      {
        sNS = oIter->first;
        list<string> &oList = oIter->second;

        _fprintf(bVerbose, stdout, "Creating A query for \"%s\".\n", sNS.c_str());
        if (!oRes.send(sNS, oResp))
        {
          fprintf(stderr, "Unable to recvfrom A: %s\n", strerror(errno));
        }
        else
        {
          _fprintf(bVerbose, stdout, "Creating response from A query.\n");

          RRList_t oAAns;
          oResp.getAnswers(oAAns);
          for (RRIter_t tIter = oAAns.begin();
               oAAns.end() != tIter;
               tIter++)
          {
            if (DNS_RR_A == (*tIter)->type())
            {
              uIP = ((DnsA *) (*tIter))->ip();
              _fprintf(bVerbose, stdout, "Got resolver IP: %d.%d.%d.%d\n", (uIP>>24)&0x00ff, (uIP>>16)&0x00ff, (uIP>>8)&0x00ff,uIP&0x00ff);
              break;
            }
          }
        }

        oRes.setBuffSize(4096);
        oRes.setDO(true);
        oRes.setNameserver(uIP);

        _fprintf(bVerbose, stdout, "Sending DNSKEY query for zone '%s'.\n", sZone.c_str());
        if (0 == uIP)
        {
          fprintf(stderr, "Unable to get IP for zone '%s'\n", sZone.c_str());
        }
        else if (!oRes.send(sZone, DNS_RR_DNSKEY, oResp))
        {
          fprintf(stderr, "Unable to send DNSKEY: %s\n", DnsError::getInstance().getError().c_str());
        }
        else
        {
          oIpMap[sNS] = uIP;
          rcode_t tCode = oResp.getHeader().get_rcode();
          _fprintf(bVerbose, stdout, "Response code was: %u\n", tCode);
          if (oResp.getHeader().get_tc())
          {
            fprintf(stdout, "TC Bit was set.\n");
          }

          if (bVerbose)
          {
            oResp.print();
          }
        }

        DnsVerifier oVerifier;
        RRList_t oKeys;
        oResp.getAnswers(oKeys);

        string sTag;
        if (!oVerifier.verify(oKeys, oKeys))
        {
          _fprintf(bVerbose, stdout, "UNABLE to verify keys.\n");
          sTag = "UNVERIFIED\t";
          iRet = 2;
        }
        else
        {
          _fprintf(bVerbose, stdout, "VERIFIED KEYS!\n");
        }

        {
          for (RRIter_t tKeyIter = oKeys.begin();
               oKeys.end() != tKeyIter;
               tKeyIter++)
          {
            ostringstream oSS;
            DnsRR *pRR = *tKeyIter;
            if (DNS_RR_DNSKEY == pRR->type())
            {
              DnsDnskey *pKey = static_cast<DnsDnskey *>(pRR);
              string sDispName;
              pKey->get_name()->display_name(sDispName, true);
              oSS << sTag
                   << "\""
                   << sDispName
                   << "\"\t"
                   << pKey->ttl()
                   << "\t"
                   << "IN"
                   << "\t"
                   << "DNSKEY"
                   << "\t"
                   << pKey->getFlags()
                   << "\t"
                   << (int) pKey->getProto()
                   << "\t"
                   << (int) pKey->getAlgo()
                   << "\t"
                   << "\""
                   << pKey->getKey()
                   << "\";";
              // (*tKeyIter)->print();

              oList.push_back(oSS.str());
              oList.sort();
            }
          }
        }
      }

      bool bConsistent = false;
      list<string> oRefList;
      for (NsKeyIter_t tConIter = oNsMap.begin();
           oNsMap.end() != tConIter;
           tConIter++)
      {
        if (oRefList.empty())
        {
          oRefList = tConIter->second;
          bConsistent = true;
        }
        else
        {
          list<string>::iterator tRefIter = oRefList.begin();
          list<string> &oTmpList = tConIter->second;
          if (oRefList.size() != oTmpList.size())
          {
            _fprintf(bVerbose, stderr, "Mismatch on list sizes %u != %u\n",
                     oRefList.size(),
                     oTmpList.size());
            bConsistent = false;
          }
          else
          {
            for (list<string>::iterator tCandIter = oTmpList.begin();
                 oTmpList.end() != tCandIter;
                 tCandIter++, tRefIter++)
            {
              if (oRefList.end() == tRefIter)
              {
                _fprintf(bVerbose, stderr, "Different number of keys.\n");
                bConsistent = false;
                break;
              }
              else if ((*tCandIter) == (*tRefIter))
              {
                bConsistent = true;
              }
              else
              {
                bConsistent = false;
                break;
              }
            }
          }
        }

        if (!bConsistent)
        {
          break;
        }
      }

      if (bConsistent)
      {
        _fprintf(bVerbose, stdout, "All key sets are consistent on all name servers.\n");
        for (list<string>::iterator tRefIter = oRefList.begin();
             oRefList.end() != tRefIter;
             tRefIter++)
        {
          cout << *tRefIter << endl;
        }
      }
      else
      {
        _fprintf(bVerbose, stderr, "Key sets are NOT consistent on all name servers.\n");
        iRet = (0 == iRet) ? 3 : iRet;
      }
    }

    delete[] szTmp;
  }

  return iRet;
}

