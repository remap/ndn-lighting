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
#include <arpa/inet.h>
#include <stdlib.h>

#include "dnskey_poll_task.h"
#include "ps_defs.h"
#include "ps_util.h"
#include "dns_packet.h"
#include "dns_rr.h"
#include "dns_a.h"
#include "dns_ns.h"
#include "dns_dnskey.h"
#include "dns_name.h"
#include "dns_verifier.h"
#include "dns_rrsig.h"
#include "ps_url_parser.h"
#include "dnskey_set_dao.h"
#include "dnskey_proc_dao.h"
#include "dao_factory.h"
#include "ps_app.h"
#include "ps_crypt_mgr.h"
#include "dns_defs.h"

using namespace std;

DnskeyPollTask::DnskeyPollTask(PsApp &p_oOwnerApp)
  : PsPollTask(p_oOwnerApp),
    m_iRound(0),
    m_eSrc(DNSKEY_SRC_PRE_INIT)
{

}

DnskeyPollTask::~DnskeyPollTask()
{

}

dnskey_src_e DnskeyPollTask::getSubmissionSrc()
{
  return m_eSrc;
}

void DnskeyPollTask::setSubmissionSrc(dnskey_src_e p_eSrc)
{
  m_eSrc = p_eSrc;
}

bool DnskeyPollTask::execute()
{
  bool bRet = false;

  if (-1 == m_iRound)
  {
    ps_elog(PSL_DEBUG, "This task is in an error state.\n");
  }
  else if (m_iRound > 0
           && NULL == getResponse() 
           && getRetry() < getMaxRetries())
  {
    m_iRound--;
    setRetry(getRetry() + 1);
    bRet = true;
  }
  else if (0 == m_iRound)
  {
    setRetry(0);
    bRet = prepareNS();
  }
  else if (1 == m_iRound)
  {
    setRetry(0);
    bRet = prepareA();
  }
  else if (2 == m_iRound)
  {
    setRetry(0);
    bRet = prepareDnskey();
  }
/*
  else if (3 == m_iRound)
  {
    bRet = processDnskey();
  }
*/
  else
  {
    ps_elog(PSL_DEBUG, "Error, too many rounds (%d)?\n", m_iRound);
  }

  m_iRound = (!bRet) ? -1 : m_iRound + 1;

  return bRet;
}

bool DnskeyPollTask::done()
{
  return m_iRound >= 3;
}

bool DnskeyPollTask::prepareNS()
{
  bool bRet = false;

  DnsRR *pQuestionRR = NULL;
  PsUrlParser oParser;
  oParser.parse(getURL());
  string &sZone = oParser.getQuery();
  DnsName oName(sZone);
  if (sZone == "")
  {
    ps_elog(PSL_DEBUG, "Unable to poll w/o zone being set.\n");
  }
  else if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_NS)))
  {
    ps_elog(PSL_DEBUG, "Unable to crete question for type: %d\n", DNS_RR_NS);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    string sURL = "dnssec:" + sZone + "?type=48&class=1";
    setURL(sURL);
    bRet = true;
  }

  return bRet;
}

bool DnskeyPollTask::prepareA()
{
  bool bRet = false;

  DnsRR *pQuestionRR = NULL;
  DnsPacket *pResp = getResponse();
  if (NULL == pResp)
  {
    ps_elog(PSL_DEBUG, "No response found, cannot query for A of non-existent NS.\n");
  }
  else if (DNS_NOERROR != pResp->getHeader().rcode())
  {
    ps_elog(PSL_WARNING, "Error %d in NS reponse for zone '%s'.\n", pResp->getHeader().rcode(), getURL().c_str());
  }
  else if (pResp->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "TC bit set in NS reponse for zone '%s'.\n", getURL().c_str());
  }
  else
  {
    int iCount = pResp->getHeader().an_count();
    string sNS;
    RRList_t tList;
    pResp->getAnswers(tList);
    RRIter_t tIter;
    for (tIter = tList.begin();
         tList.end() != tIter;
         tIter++)
    {
      DnsRR *pTmpRR = *tIter;
      if (DNS_RR_NS == pTmpRR->type())
      {
        sNS = ((DnsNs *) pTmpRR)->getName();
        if (0 == (rand() % iCount))
        {
          break;
        }
      }
    }

    if (sNS.size() == 0)
    {
      tList.clear();
      pResp->getAuthority(tList);
      for (tIter = tList.begin();
           tList.end() != tIter;
           tIter++)
      {
        DnsRR *pTmpRR = *tIter;
        if (DNS_RR_NS == pTmpRR->type())
        {
          sNS = ((DnsNs *) pTmpRR)->getName();
          break;
        }
      }
    }

//ps_elog(PSL_DEBUG, "GOT NAME '%s'\n", sNS.c_str());
    DnsName oNsName(sNS);
    PsUrlParser oParser;
    oParser.parse(getURL());
    if (sNS.size() == 0)
    {
      ps_elog(PSL_WARNING, "Unable to locate NS for zone: '%s'\n", oParser.getQuery().c_str());
    }
    else if (NULL == (pQuestionRR = DnsRR::question(oNsName, DNS_RR_A)))
    {
      ps_elog(PSL_ERROR, "Unable to create question for type: %d\n", DNS_RR_A);
    }
    else
    {
      DnsPacket *pPkt = new DnsPacket(true, -1);
      pQuestionRR->set_class(DNS_CLASS_IN);
      pPkt->addQuestion(*pQuestionRR);
      setQuery(pPkt);
      setResponse(NULL);
      string sURL = "dnssec://" + sNS + "/" + oParser.getQuery() + "?type=48&class=1";
      setURL(sURL);
      bRet = true;
    }
  }

  return bRet;
}

bool DnskeyPollTask::prepareDnskey()
{
  bool bRet = false;

  DnsRR *pQuestionRR = NULL;
  PsUrlParser oParser;
  oParser.parse(getURL());
  string &sZone = oParser.getQuery();
  DnsName oName(sZone);
  DnsPacket *pResp = getResponse();
  if (NULL == pResp)
  {
    ps_elog(PSL_WARNING, "No response received, cannot query for key.\n");
  }
  else if (DNS_NOERROR != pResp->getHeader().rcode())
  {
    ps_elog(PSL_WARNING, "Error %d in A reponse for zone '%s'.\n", pResp->getHeader().rcode(), sZone.c_str());
  }
  else if (pResp->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "TC bit set in A reponse for zone '%s'.\n", sZone.c_str());
  }
  else
  {
    uint32_t uIP = 0;
    RRList_t tList;
    RRIter_t tIter;

    pResp->getAnswers(tList);
    for (tIter = tList.begin();
         tList.end() != tIter;
         tIter++)
    {
      DnsRR *pRR = *tIter;
      if (DNS_RR_A == pRR->type())
      {
        uIP = ((DnsA *) pRR)->ip();
        break;
      }
    }

    if (0 == uIP)
    {
      ps_elog(PSL_DEBUG, "Unable to locate IP.\n");
    }
    else if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_DNSKEY)))
    {
      ps_elog(PSL_DEBUG, "Unable to create question for type: %d\n", DNS_RR_DNSKEY);
    }
    else
    {
      setNameserver(uIP);
      DnsPacket *pPkt = new DnsPacket(true, -1);
      pQuestionRR->set_class(DNS_CLASS_IN);
      pPkt->addQuestion(*pQuestionRR);
      setQuery(pPkt);
      setResponse(NULL);
      string sURL = "dnssec://" + ps_inet_ntoa(uIP) + "/" + sZone + "?type=48&class=1";
      setURL(sURL);
      bRet = true;
    }
  }

  return bRet;
}

bool DnskeyPollTask::processDnskey()
{
  bool bRet = false;

  DnsPacket *pResp = getResponse();
  if (NULL == pResp)
  {
    ps_elog(PSL_DEBUG, "No response received for key.\n");
  }
  else if (DNS_NOERROR != pResp->getHeader().rcode())
  {
    PsUrlParser oParser;
    oParser.parse(getURL());
    ps_elog(PSL_WARNING, "Error %d in DNSKEY reponse for zone '%s'.\n", pResp->getHeader().rcode(), oParser.getQuery().c_str());
  }
  else if (pResp->getHeader().get_tc())
  {
    PsUrlParser oParser;
    oParser.parse(getURL());
    ps_elog(PSL_WARNING, "TC bit set in A reponse for zone '%s'.\n", oParser.getQuery().c_str());
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool DnskeyPollTask::process()
{
  bool bRet = false;

  DnskeySetDao *pKeyDao = NULL;
  DnskeyProcDao *pProcDao = NULL;
  DnsPacket *pPkt = NULL;

  try
  {
    DaoFactory &oFact = DaoFactory::getInstance();
    pKeyDao = dynamic_cast<DnskeySetDao *>(oFact.create(getRawDaoName()));
    pProcDao = dynamic_cast<DnskeyProcDao *>(oFact.create(getProcDaoName()));
    if (NULL == pKeyDao)
    {
//      ps_elog(PSL_DEBUG, "Unable to create DAO for name: '%s'\n", DnskeySetDao::s_kszDaoName);
      ps_elog(PSL_WARNING, "Unable to create DAO for name: '%s'\n", getRawDaoName().c_str());
    }
    else if (NULL == pProcDao)
    {
//      ps_elog(PSL_DEBUG, "Unable to create DAO for name: '%s'\n", DnskeySetDao::s_kszDaoName);
      ps_elog(PSL_WARNING, "Unable to create DAO for name: '%s'\n", getProcDaoName().c_str());
    }
    else if (NULL == (pPkt = getResponse()))
    {
      ps_elog(PSL_WARNING, "Response packet is NULL for '%s'\n", getURL().c_str());
      delete pKeyDao;
      pKeyDao = NULL;
      delete pProcDao;
      pProcDao = NULL;
    }
    else if (DNS_NOERROR != pPkt->getHeader().rcode())
    {
      ps_elog(PSL_WARNING, "Error getting DNSKEY: %d for zone: '%s'\n", pPkt->getHeader().rcode(), getURL().c_str());
      delete pKeyDao;
      pKeyDao = NULL;
      delete pProcDao;
      pProcDao = NULL;
    }
    else if (pPkt->getHeader().get_tc())
    {
      ps_elog(PSL_WARNING, "Error: TC bit sec for DNSKEY for zone: '%s'\n", getURL().c_str());
      delete pKeyDao;
      pKeyDao = NULL;
      delete pProcDao;
      pProcDao = NULL;
    }
    else
    {
      pProcDao->addRawDao(pKeyDao);
      pProcDao->setScraperID(getScraperID());
      pProcDao->setScraperParamID(getScraperParamID());

      RRList_t oAnsList;
      if (!pPkt->getAnswers(oAnsList))
      {
        ps_elog(PSL_WARNING, "Unable to get answers from list.\n");
      }
      else if (pPkt->getHeader().get_rcode() != DNS_NOERROR)
      {
        ps_elog(PSL_WARNING, "Query returned rcode: %d\n", pPkt->getHeader().get_rcode());
      }
      else
      {
        int iKeys = 0;
        PsUrlParser oParser;
        oParser.parse(getURL());
        pKeyDao->init(oParser.getQuery(), getURL());
        for (RRIter_t oIter = oAnsList.begin();
             oAnsList.end() != oIter;
             oIter++)
        { 
          DnsRR *pRR = *oIter;

          uint16_t uType = pRR->type();
          if (DNS_RR_DNSKEY == uType)
          {
            DnsDnskey *pKey = (DnsDnskey *) pRR;
            string sName;
//            DnsName *pKeyName = pKey->get_name();
//            pKeyName->display_name(sName);
            ps_elog(PSL_DEBUG, "'%s': <Type, Class, TTL> = <%u, %u, %u>\n\t<Flags, Proto, Algo> = <%u, %u, %u>\n\t%s\n",
                    sName.c_str(),
                    pKey->type(),
                    pKey->get_class(),
                    pKey->ttl(),
                    pKey->getFlags(),
                    pKey->getProto(),
                    pKey->getAlgo(),
                    pKey->getKey().c_str());
            pKeyDao->addKey(*pKey);
            iKeys++;
          }
          else if (DNS_RR_RRSIG == uType)
          {
            DnsRrsig *pSig = static_cast<DnsRrsig *>(pRR);
            pKeyDao->addRrsig(*pSig);
          }
        }

        bool bSer = pKeyDao->deserialize();
        if (!bSer)
        {
          ps_elog(PSL_WARNING, "Unable to deserialize: '%s'\n", pKeyDao->getName().c_str());
        }
        string sData = (NULL == pKeyDao->getData()) ? "" : pKeyDao->getData();
        string sSig = pKeyDao->getSig();
        DnsVerifier oVerif;

        PsCryptMgr *pMgr = getCryptMgr();
        if (NULL == pMgr
            && NULL == (pMgr = getOwnerApp().getCryptMgr()))
        {
          ps_elog(PSL_WARNING, "No crypt mgrs specified.\n");
        }
        else if (sSig != "")
        {
          ps_elog(PSL_DEBUG, "Key set (raw DAO) is already signed, so skipping.\n");
        }
        else if (!pMgr->sign(sData, sSig, true))
        {
          ps_elog(PSL_CRITICAL, "Unable to sign data from raw DAO.\n");
        }
        else if (!pKeyDao->setSig(sSig))
        {
          ps_elog(PSL_CRITICAL, "Unable to set sig data in raw DAO.\n"); 
        }
        else
        {
          ps_elog(PSL_DEBUG, "Key set (raw DAO) signed.\n");
        }

        pProcDao->setName(pKeyDao->getName());
        if (0 == iKeys)
        {
          ps_elog(PSL_WARNING, "No keys in response from URL: '%s'.\n", getURL().c_str());
        }
        else if (!pKeyDao->serialize())
        {
          ps_elog(PSL_WARNING, "Unable to serialize key set for URL: '%s'.\n", getURL().c_str());
        }
        else if (!oVerif.verify(oAnsList, oAnsList))
        {
          ps_elog(PSL_WARNING, "DNSKEY set did NOT verify...  Proc DAO will NOT be invoked since this is not verified data.\n");
          bRet = true;
        }
        else if (!pProcDao->process())
        {
          ps_elog(PSL_WARNING, "Unable to execute process DAO\n");
        }
        else
        {
          if (NULL != pMgr)
          {
            sData = pProcDao->getVerifiableData(DNSKEY_CURRENT_VERSION);
            sSig = "";
            if (!pMgr->sign(sData, sSig, true))
            {
              ps_elog(PSL_CRITICAL, "Unable to sign data for proc DAO.\n");
            }
            else if (!pProcDao->setSig(sSig))
            {
              ps_elog(PSL_CRITICAL, "Unable to set sig data in proc DAO.\n");
            }
          }

          if (!pProcDao->serialize())
          {
            ps_elog(PSL_ERROR, "Unable to serialize process DAO\n");
          }
          else
          {
/*
            m_oDaoList.push_back(pProcDao);
            pProcDao = NULL;
*/
            bRet = true;
          }
        }
      }
    }
  }
  catch (...)
  {
    ps_elog(PSL_CRITICAL, "Caught exception.\n");
    bRet = false;
  }

  if (NULL != pProcDao)
  {
    delete pProcDao;
    pProcDao = NULL;
  }

  return bRet;
}
