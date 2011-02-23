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

#include "ps_logger.h"
#include <stdio.h>
#include <arpa/inet.h>

#include "dsync_third_task.h"
#include "dnskey_poll_task.h"
#include "ps_defs.h"
#include "ps_util.h"
#include "dns_packet.h"
#include "dns_rr.h"
#include "dns_a.h"
#include "dns_ns.h"
#include "dns_dnskey.h"
#include "dns_name.h"
#include "ps_url_parser.h"
#include "dao_factory.h"
#include "ps_app.h"
#include "ps_crypt_mgr.h"
#include "dns_defs.h"
#include "algorithm"

using namespace std;

DsyncThirdTask::DsyncThirdTask(DsyncZoneDao* zD) : zDao(zD), validNameserver(0), m_iRound(0), syncState(zD->getSyncState()) 
{
  //zDao = zD;
  taskType = "third";
}

DsyncThirdTask::~DsyncThirdTask()
{
  delete zDao;
}

bool DsyncThirdTask::execute()
{
  bool bRet = false;
  if (m_iRound == 0)
  {
    NsObj *qNs = zDao->getNsDao().getRandomNs();
    validNameserverName = qNs->name;
    validNameserverIp = qNs->ip;
    validNameserver = htonl(inet_addr(qNs->ip.c_str()));
   
    m_iRound = 1;
    return execute();
  }
  //ask 1 nameserver for dnskey
  else if (m_iRound == 1)
  {
    bRet = prepareDnskey();
  }
  else if (m_iRound == 2)
  {
    //get the ns set
    bRet = prepareNs();
  }
  else if (m_iRound == 3)
  {
    //get the a record for each ns
    if (NsList.size() == 0)
    {
      m_iRound = 5;
      return execute();
    }
    else
    {
      prepareA();
    }
  }
  else if (m_iRound == 4)
  {
    prepareA(privateName);
  }
  else if (m_iRound == 5)
  {
    //ask every nameserver for the keyset
    prepareNDnskey();
  }
  else if (m_iRound == 6)
  {
    serialize();
    m_iRound++;
  }
  else if (m_iRound == 100)
  {
    prepareNsC();
  }
  else if (m_iRound == 101)
  {
    prepareAC();
  }
  else if (m_iRound == 102)
  {
    printf("NOT YET IMPLEMENTED\n");
  }
  return bRet;
}

bool DsyncThirdTask::done()
{
  return m_iRound == 7 ? true : false;
}

bool DsyncThirdTask::process()
{
  bool bRet = false;
  if (m_iRound == 0)
  {
  }
  else if (m_iRound == 1)
  {
    bRet = processDnskey();
    if (!bRet)
    {
      m_iRound = 6;
    }
    else
    {
      m_iRound++;
    }
  }
  else if (m_iRound == 2)
  {
    processNs();
    if (NsList.size() == 0)
    {
      m_iRound = 6;
    }
    else
    {
      m_iRound++;
    }
  }
  else if (m_iRound == 3)
  {
    processA();
    if (NsList.size() == 0)
    {
      //m_iRound = 5;
    }
  }
  else if (m_iRound == 4)
  {
    processA(privateName);
    m_iRound = 3;
  }
  else if (m_iRound == 5)
  {
    if (!processNDnskey())
    {
      ps_elog(PSL_WARNING, "Unable to advance zone status: %s, couldn't not verify new keyset\n", zDao->zoneName.c_str());
    }
    if (NsB.size() == 0)
    {
      ps_elog(PSL_INFO, "All nameservers are showing the new keyset! Back in sync\n");
      syncState = 10;
      sleepTTL();
    }
  }
  else if (m_iRound == 6)
  {
    serialize();
  }
  else if (m_iRound == 100)
  {
    processNsC();
    m_iRound++;
  }
  else if (m_iRound == 101)
  {
    processAC();
    m_iRound = 1;
    printf("ProcessAC finished, m_iRound = %d\n", m_iRound);
  }
  
  return bRet;
}

bool DsyncThirdTask::prepareBlanketNs()
{
  bool bRet = false;
  DnsName oName(zDao->zoneName);
  
  DnsRR *pQuestionRR = NULL;
  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_NS)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type: %d\n", DNS_RR_NS);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}

bool DsyncThirdTask::prepareNs()
{
  bool bRet = false;
  setNameserver(validNameserver);
  
  DnsName oName(zDao->zoneName);
  DnsRR *pQuestionRR = NULL;
  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_NS)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type: %d\n", DNS_RR_NS);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }

  return bRet;
}


bool DsyncThirdTask::prepareNsC()
{
  bool bRet = false;
  setNameserver(validNameserver);
  
  DnsName oName(zDao->zoneName);
  DnsRR *pQuestionRR = NULL;
  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_NS)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type: %d\n", DNS_RR_NS);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}



bool DsyncThirdTask::prepareDs()
{
  bool bRet = false;
  setNameserver(0);
  DnsName oName(zDao->zoneName);

  DnsRR *pQuestionRR = NULL;
  
  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_DS)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type: %d\n", DNS_RR_DS);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}


bool DsyncThirdTask::processBlanketNs()
{
  bool bRet = false;
  
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
    RRIter_t oRrIter;
    RRList_t oAns;
    pPkt->getAnswers(oAns);
    for (oRrIter = oAns.begin();
         oRrIter != oAns.end();
         oRrIter++)
    {
      DnsRR *pAnsRR = *oRrIter;
      if (DNS_RR_NS == pAnsRR->type())
      {
        ps_elog(PSL_DEBUG, "Got Nameserver: %s\n", ((DnsNs*)pAnsRR)->getName().c_str());
      }
    }
  }
  return bRet;
}

bool DsyncThirdTask::processNs()
{
  //printf("ProcessNS: printing zone dao:\n");
  //zDao->printAll();

  bool bRet = false;
  
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
    RRIter_t oRrIter;
    RRList_t oAns;
    RRList_t v_oNsAndSigs;
    pPkt->getAnswers(oAns);
    for (oRrIter = oAns.begin();
         oRrIter != oAns.end();
         oRrIter++)
    {
      //printf("\n---\nAnswer Record: type = %d:\n", (*oRrIter)->type());
      //(*oRrIter)->print();
      //DnsRR *pAnsRR = *oRrIter;
      if (DNS_RR_NS == (*oRrIter)->type())
      {
        ps_elog(PSL_DEBUG, "Asking %s (%s), got Nameserver: %s\n", validNameserverName.c_str(), validNameserverIp.c_str(), ((DnsNs*)(*oRrIter))->getName().c_str());
        v_oNsAndSigs.push_back((DnsNs*)(*oRrIter));
      }
      if (DNS_RR_RRSIG == (*oRrIter)->type())
      {
        ps_elog(PSL_DEBUG, "Got RRSIG\n");
        v_oNsAndSigs.push_back((DnsRrsig*)(*oRrIter));
      }
    }
   
    RRList_t dbKeyset;// = zDao->getKeyset();
    std::vector<DsyncRrsetDao*> daos = zDao->getRrsets();
    std::vector<DsyncRrsetDao*>::iterator iter;
    for (iter = daos.begin(); iter != daos.end(); iter++)
    {
      RRList_t keys = (*iter)->getKeyset();
      for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
      {
        //printf("Adding following key to dbKeyset:\n");
        //((DnsDnskey*)(*kit))->print();
        dbKeyset.push_back((DnsDnskey*)(*kit));
      }
    }

    /*
    printf("\nKeys: size=%d\n", (int) v_oNsAndSigs.size());
    for (RRIter_t oIt = dbKeyset.begin(); oIt != dbKeyset.end(); oIt++)
    {
        (*oIt)->print();
    }

    printf("\nNs and Sigs: size=%d\n", (int) v_oNsAndSigs.size());
    for (RRIter_t oIt = v_oNsAndSigs.begin(); oIt != v_oNsAndSigs.end(); oIt++)
    {
       (*oIt)->print();
    }
    */

    bool verified = verifier.verify(dbKeyset, v_oNsAndSigs);

    if (!verified)
    {
      ps_elog(PSL_WARNING, "Failed to verify NS set with keyset from DB.\n");
    }
    else
    {
      ps_elog(PSL_INFO, "Verify OK for NS set.\n");
      for (oRrIter = oAns.begin();
           oRrIter != oAns.end();
           oRrIter++)
      {
        DnsRR *pAnsRR = *oRrIter;
        if (DNS_RR_NS == pAnsRR->type())
        {
          NsList.push_back(((DnsNs*)(pAnsRR))->getName());
        }
      }
    }
  }
  return bRet;
}

bool DsyncThirdTask::processBlanketA()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
 
  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
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
      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      { 
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_A)
        { 
          uint32_t uIP =  ((DnsA *) pRR)->ip();
          char tIP [32];
          bzero(tIP, 32);
          sprintf(tIP, "%d.%d.%d.%d", (uIP>>24)&0x00ff, (uIP>>16)&0x00ff, (uIP>>8)&0x00ff,uIP&0x00ff);
          ps_elog(PSL_DEBUG, "Unauthenticated: %s: %s\n", pRR->get_name()->toString().c_str(), tIP); 

         /*
          NsObj * temp = new NsObj();
          temp.name = pRR->get_name()->toString();
          temp.ip = tIP;
          temp.verified = false;
          uNs.push_back(temp);
          uNsB.push_back(uIP);          
         */
          bRet = true;
        }
      }

    } 
  }
  return bRet;
}

bool DsyncThirdTask::processA()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
  NsList.pop_back();
 
  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
    RRList_t oAnsList;
    if (!pPkt->getAnswers(oAnsList))
    {
      ps_elog(PSL_WARNING, "Unable to get answers from list.\n");
    }
    else if (pPkt->getHeader().get_rcode() != DNS_NOERROR)
    { 
      ps_elog(PSL_WARNING, "Query returned rcode: %d\n", pPkt->getHeader().get_rcode());
      //if REFUSED we have to resort to a recursive query and hope it's legit
      if (pPkt->getHeader().get_rcode() == 5)
      {
        RRList_t oQuestList;
        if (!pPkt->getQuestions(oQuestList))
        {
          ps_elog(PSL_WARNING, "Unable to get questions from the list.\n");
        }
        else
        {
          privateName = (*oQuestList.begin())->get_name()->toString();
          ps_elog(PSL_WARNING, "No verifiable data for %s.  Issuing a recursive query.\n", privateName.c_str());
          m_iRound = 4; //FINDME
          return false;
        }
      }
    } 
    else
    {
      RRList_t AAndSig;
      NsObj temp;// = new NsObj();
      uint32_t tuIP;
      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      {
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_A)
        { 
          uint32_t uIP =  ((DnsA *) pRR)->ip();
          char tIP [32];
          bzero(tIP, 32);
          sprintf(tIP, "%d.%d.%d.%d", (uIP>>24)&0x00ff, (uIP>>16)&0x00ff, (uIP>>8)&0x00ff,uIP&0x00ff);

          AAndSig.push_back(pRR);
          temp.ip = tIP;
          temp.name = pRR->get_name()->toString();
          tuIP = uIP;
          bRet = true;

          NsB.push_back(uIP);

        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          AAndSig.push_back(pRR);
        }
      } 

      bool verified = verifier.verify(keys, AAndSig);

      if (verified)
      {
        ps_elog(PSL_INFO, "Verified A Record for '%s' as %s\n",temp.name.c_str(), temp.ip.c_str());
        temp.verified = true;
        //vNs.push_back(temp);
        vNsB.push_back(tuIP);          
      }
      else
      {
        ps_elog(PSL_WARNING, "Unable to verify A record for '%s' as %s\n", temp.name.c_str(), temp.ip.c_str());
      }  

    } 
  }
  return bRet;
}

bool DsyncThirdTask::processDs()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set");
  }
  else
  {
    RRList_t oAnsList;
    RRList_t oDs;
    RRList_t oSig;
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
      for (RRIter_t oIter = oAnsList.begin();
          oAnsList.end() != oIter;
          oIter++)
      {
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_DS)
        {
          oDs.push_back(pRR);
          bRet = true;
        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          oSig.push_back(pRR);
        }
      }
      if (oDs.size() == 0)
      {
        ps_elog(PSL_WARNING, "No DS Record Found\n");
        syncState = -1;
      }
      else
      {
        bool verified = false;
        for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
        {
          if (verifier.verifyDs(*(DnsDs*)(*it), keys))
          {
            verified = true;
            break;
          }
        }

        if (verified)
        {
          ps_elog(PSL_INFO, "Found valid DS record at the parent. Matches our DNSKEY set but is unverified.\n");
          for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
          {
            DnsDs* ds = new DnsDs(*(DnsDs*)*it);//DnsDs(*(DnsDs*)(*it));
            Ds.push_back(ds);
          }
          for (RRIter_t it = oSig.begin(); it != oSig.end(); it++)
          {
            DnsRrsig* sig = new DnsRrsig(*(DnsRrsig*)*it);//DnsRrsig(*(DnsRrsig*)(*it));
            DsSigs.push_back(sig);
          }
        }
        else
        {
          ps_elog(PSL_WARNING, "No valid DS found at the parent. Setting to error state\n");
          syncState = -1;
        }
      }
    }
  }
  return bRet;
}

bool DsyncThirdTask::prepareA()
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(NsList.back());
  setNameserver(validNameserver);

  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_A)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type %d\n", DNS_RR_A);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}

bool DsyncThirdTask::prepareDnskey()
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(zDao->zoneName);
  setNameserver(validNameserver);
  //printf("Asking: %s for %s DNSKEY\n", validNameserverIp.c_str(), zDao->zoneName.c_str());
  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_DNSKEY)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type %d\n", DNS_RR_DNSKEY);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}

bool DsyncThirdTask::prepareNDnskey()
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(zDao->zoneName);
  setNameserver(vNsB.back());
  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_DNSKEY)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type %d\n", DNS_RR_DNSKEY);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}

bool DsyncThirdTask::processNDnskey()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;

  validNameserver = vNsB.back();
  vNsB.pop_back();

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
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
      RRList_t v_oKeysAndSigs;
      RRList_t v_oKeys;
      RRList_t v_oSigs;

      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      { 
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_DNSKEY)
        { 
          DnsDnskey *key = (DnsDnskey*)pRR;
          v_oKeysAndSigs.push_back(key);
          v_oKeys.push_back(key);
        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          DnsRrsig * sig = (DnsRrsig*) pRR;
          v_oKeysAndSigs.push_back(sig);
          v_oSigs.push_back(sig);
        }
      }

      RRList_t dbKeyset;
      std::vector<DsyncRrsetDao*> daos = zDao->getRrsets();
      std::vector<DsyncRrsetDao*>::iterator iter;
      for (iter = daos.begin(); iter != daos.end(); iter++)
      {
        RRList_t keys = (*iter)->getKeyset();
        for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
        {
          dbKeyset.push_back((DnsDnskey*)(*kit));
        }
      }

      bool verified = verifier.verify(dbKeyset, v_oKeysAndSigs);

      if (!verified)
      {
        ps_elog(PSL_WARNING, "%s: Keyset in DB does not sign keys from %s\n", zDao->zoneName.c_str(), validNameserverIp.c_str());
        return false;
      }
      else
      {
        if (zDao->getRrsets().at(0)->setsMatch(dbKeyset, v_oKeys))
        {
          char tIP [32];
          bzero(tIP, 32);
          sprintf(tIP, "%d.%d.%d.%d", (validNameserver>>24)&0x00ff, (validNameserver>>16)&0x00ff, (validNameserver>>8)&0x00ff, validNameserver&0x00ff);
          ps_elog(PSL_INFO, "%s: Nameserver %s correctly shows the new keyset\n", zDao->zoneName.c_str(), tIP);
        }
        bRet = true;
      }
    }
  }
  return bRet;
}


bool DsyncThirdTask::processDnskey()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
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

      RRList_t v_oKeysAndSigs;
      RRList_t v_oKeys;
      RRList_t v_oSigs;

      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      { 
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_DNSKEY)
        { 
          DnsDnskey *key = new DnsDnskey(*(DnsDnskey*)pRR);//(DnsDnskey*)pRR;
          v_oKeysAndSigs.push_back(key);
          v_oKeys.push_back(key);
        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          DnsRrsig * sig = new DnsRrsig(*(DnsRrsig*)pRR);//(DnsRrsig*) pRR;
          v_oKeysAndSigs.push_back(sig);
          v_oSigs.push_back(sig);
        }
      }

      RRList_t dbKeyset;
      std::vector<DsyncRrsetDao*> daos = zDao->getRrsets();
      std::vector<DsyncRrsetDao*>::iterator iter;
      for (iter = daos.begin(); iter != daos.end(); iter++)
      {
        RRList_t keys = (*iter)->getKeyset();
        for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
        {
          dbKeyset.push_back((DnsDnskey*)(*kit));
        }
      }
      
      bool verified = verifier.verify(dbKeyset, v_oKeysAndSigs);
      
      if (!verified)
      {
        ps_elog(PSL_WARNING, "Failed to verify key set...\n");
      }
      else
      {
        if (zDao->getRrsets().at(0)->setsMatch(dbKeyset, v_oKeys))
        {
          ps_elog(PSL_INFO, "Keyset has not changed for %s\n", zDao->zoneName.c_str());
          zDao->addKeyset(v_oKeys, v_oSigs, validNameserverName, validNameserverIp); 
          return bRet;
        }
        else
        {
          //printf("\n\n+++++++++++\nDETECTED A CHANGE IN KEYSET\n+++++++++++++\n\n");
          //printf("BEFORE ADDKEYSET:\n");
          //zDao->printAll();
          zDao->addKeyset(v_oKeys, v_oSigs); 
          //printf("\n\nAFTER ADDKEYSET:\n");
          //zDao->printAll();
          //printf("\n\n\n");
          //
          ps_elog(PSL_INFO, "Detected change in key set: Verify OK for new set.\n");
          //syncState = 1;
          bRet = true;
        }
      }
    } 
  }
  return bRet;
}

void DsyncThirdTask::serialize()
{
  ps_elog(PSL_DEBUG, "Writing zone '%s' to the database\n", zDao->zoneName.c_str());
  //zDao->printAll();
  zDao->setLastNs(validNameserverName, validNameserverIp);
  zDao->setSyncState(syncState);
  zDao->serialize();
}

void DsyncThirdTask::sleepTTL()
{
  ps_elog(PSL_DEBUG, "Sleeping zone '%s' for the DNSKEY TTL period of %d\n", zDao->zoneName.c_str(), zDao->getDnskeyTTL());
  zDao->setSyncState(syncState);
  zDao->serialize(zDao->getDnskeyTTL());
  m_iRound = 7;
}


bool DsyncThirdTask::prepareA(std::string n)
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(n);
  setNameserver(0);
  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_A)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type %d\n", DNS_RR_A);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}


bool DsyncThirdTask::processA(std::string n)
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
 
  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
    RRList_t oAnsList;
    if (!pPkt->getAnswers(oAnsList))
    {
      ps_elog(PSL_WARNING, "Unable to get answers from list.\n");
    }
    else if (pPkt->getHeader().get_rcode() != DNS_NOERROR)
    { 
      ps_elog(PSL_WARNING, "Recursive Query returned rcode: %d\n", pPkt->getHeader().get_rcode());
    }
    else
    {
      NsObj temp;// = new NsObj();
      uint32_t tuIP;
      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      {
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_A)
        { 
          uint32_t uIP =  ((DnsA *) pRR)->ip();
          char tIP [32];
          bzero(tIP, 32);
          sprintf(tIP, "%d.%d.%d.%d", (uIP>>24)&0x00ff, (uIP>>16)&0x00ff, (uIP>>8)&0x00ff,uIP&0x00ff);

          temp.ip = tIP;
          temp.name = pRR->get_name()->toString();
          temp.verified = false;
          tuIP = uIP;
          bRet = true;
  
          ps_elog(PSL_DEBUG, "Recursive query returned ip for %s as %s\n", temp.name.c_str(), temp.ip.c_str());
        
          //vNs.push_back(temp);
          vNsB.push_back(tuIP);          
        }
      }
    } 
  }
  return bRet;
}

bool DsyncThirdTask::processNsC()
{
  bool bRet = false;
  
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
    RRIter_t oRrIter;
    RRList_t oAns;
    RRList_t v_oNsAndSigs;
    pPkt->getAnswers(oAns);
    for (oRrIter = oAns.begin();
         oRrIter != oAns.end();
         oRrIter++)
    {
      DnsRR *pAnsRR = *oRrIter;
      if (DNS_RR_NS == pAnsRR->type())
      {
        ps_elog(PSL_DEBUG, "Asking %s (%s), got Nameserver: %s\n", validNameserverName.c_str(), validNameserverIp.c_str(), ((DnsNs*)pAnsRR)->getName().c_str());
        v_oNsAndSigs.push_back((DnsNs*)pAnsRR);
      }
      if (DNS_RR_RRSIG == pAnsRR->type())
      {
        ps_elog(PSL_DEBUG, "Got RRSIG\n");
        v_oNsAndSigs.push_back((DnsRrsig*)pAnsRR);
      }
    }
   
    RRList_t dbKeyset;
    std::vector<DsyncRrsetDao*> daos = zDao->getRrsets();
    std::vector<DsyncRrsetDao*>::iterator iter;
    for (iter = daos.begin(); iter != daos.end(); iter++)
    {
      RRList_t keys = (*iter)->getKeyset();
      for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
      {
        dbKeyset.push_back((DnsDnskey*)(*kit));
      }
    }

    bool verified = verifier.verify(dbKeyset, v_oNsAndSigs);

    if (!verified)
    {
      ps_elog(PSL_WARNING, "Failed to verify NS set with keyset from DB.\n");
    }
    else
    {
      ps_elog(PSL_INFO, "Verify OK for NS set.\n");
      for (oRrIter = oAns.begin();
           oRrIter != oAns.end();
           oRrIter++)
      {
        DnsRR *pAnsRR = *oRrIter;
        if (DNS_RR_NS == pAnsRR->type())
        {
          NsList.push_back(((DnsNs*)(pAnsRR))->getName());
        }
      }
    }
  }
  return bRet;
}

bool DsyncThirdTask::prepareAC()
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(NsList.back());
  setNameserver(validNameserver);

  if (NULL == (pQuestionRR = DnsRR::question(oName, DNS_RR_A)))
  {
    ps_elog(PSL_ERROR, "Unable to create question for type %d\n", DNS_RR_A);
  }
  else
  {
    DnsPacket *pPkt = new DnsPacket(true, -1);
    pQuestionRR->set_class(DNS_CLASS_IN);
    pPkt->addQuestion(*pQuestionRR);
    setQuery(pPkt);
    setResponse(NULL);
    bRet = true;
  }
  return bRet;
}

bool DsyncThirdTask::processAC()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
  NsList.pop_back();
 
  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
  }
  else
  {
    RRList_t oAnsList;
    if (!pPkt->getAnswers(oAnsList))
    {
      ps_elog(PSL_WARNING, "Unable to get answers from list.\n");
    }
    else if (pPkt->getHeader().get_rcode() != DNS_NOERROR)
    { 
      ps_elog(PSL_WARNING, "Query returned rcode: %d\n", pPkt->getHeader().get_rcode());
      //if REFUSED we have to resort to a recursive query and hope it's legit
      if (pPkt->getHeader().get_rcode() == 5)
      {
        RRList_t oQuestList;
        if (!pPkt->getQuestions(oQuestList))
        {
          ps_elog(PSL_WARNING, "Unable to get questions from the list.\n");
        }
        else
        {
          privateName = (*oQuestList.begin())->get_name()->toString();
          ps_elog(PSL_WARNING, "No verifiable data for %A.  Issuing a recursive query.\n", privateName.c_str());
          m_iRound = 102;
        }
      }
    } 
    else
    {
      RRList_t AAndSig;
      NsObj temp;// = new NsObj();
      uint32_t tuIP;
      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      {
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_A)
        { 
          uint32_t uIP =  ((DnsA *) pRR)->ip();
          char tIP [32];
          bzero(tIP, 32);
          sprintf(tIP, "%d.%d.%d.%d", (uIP>>24)&0x00ff, (uIP>>16)&0x00ff, (uIP>>8)&0x00ff,uIP&0x00ff);

          AAndSig.push_back(pRR);
          temp.ip = tIP;
          temp.name = pRR->get_name()->toString();
          tuIP = uIP;
          bRet = true;

          NsB.push_back(uIP);

        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          AAndSig.push_back(pRR);
        }
      } 

      bool verified = verifier.verify(keys, AAndSig);

      if (verified)
      {
        temp.verified = true;
        ps_elog(PSL_INFO, "Verified A Record for '%s' as %s\n",temp.name.c_str(), temp.ip.c_str());
        //vNs.push_back(temp);
        vNsB.push_back(tuIP);          
      }
      else
      {
        ps_elog(PSL_WARNING, "Unable to verify A record for '%s' as %s\n", temp.name.c_str(), temp.ip.c_str());
      }  

    } 
  }
  return bRet;
}

