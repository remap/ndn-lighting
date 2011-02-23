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

#include "dsync_init_task.h"
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

DsyncInitTask::DsyncInitTask(std::string zName) : validNameserver(0), m_iRound(0), syncState(-1), badQueries(0) 
{
  zDao.zoneName = zName;
  taskType = "init";
}


DsyncInitTask::~DsyncInitTask()
{
  /*
  DsyncZoneDao::clearList(keys);
  DsyncZoneDao::clearList(keySigs);
  DsyncZoneDao::clearList(Ds);
  DsyncZoneDao::clearList(DsSigs);
  */
}

void DsyncInitTask::setKeyFromFile(RRList_t &k)
{
  keyFromFile = k;
}

bool DsyncInitTask::execute()
{
  bool bRet = false;
  if (badQueries > MAX_BAD_QUERIES)
  {
    ps_elog(PSL_ERROR, "Hit the limit on MAX_BAD_QUERIES: Terminating initialization task for zone: %s\n", zDao.zoneName.c_str());
    m_iRound = -1;
    bRet = false;
  }
  if (m_iRound == 0)
  {
    bRet = prepareBlanketNs();
  }
  else if (m_iRound == 1)
  {
    if (unverifiedNs.size() == 0)
    {
      ps_elog(PSL_WARNING, "Could not locate any nameservers for zone: %s\n", zDao.zoneName.c_str());
      m_iRound = -1;
    }
    bRet = prepareBlanketA();
  }
  else if (m_iRound == 2)
  {
    prepareDnskey();
  }
  else if (m_iRound == 3)
  {
    prepareNs();
  }
  else if (m_iRound == 4)
  {
    if (verifiedNs.size() == 0)
    { 
      m_iRound = 6;
      execute();
    }
    else
    {
      prepareA();
    }
  }
  else if (m_iRound == 5)
  {
    prepareA(privateName);
  }
  else if (m_iRound == 6)
  {
    prepareDs();
  }
  return bRet;
}

bool DsyncInitTask::done()
{
  return m_iRound > 6 || m_iRound == -1 ? true : false;
}

bool DsyncInitTask::process()
{
  bool bRet = false;
  if (m_iRound == 0)
  {
    bRet = processBlanketNs();
    m_iRound++;
  }
  else if (m_iRound == 1)
  {
    bRet = processBlanketA();
    if (unverifiedNs.size() == 0)
    {
      m_iRound++;
    }
  }
  else if (m_iRound == 2)
  {
    bRet = processDnskey();
    if (uNs.size() == 0)
    {
      if (validNameserver == 0)
      {
        ps_elog(PSL_CRITICAL, "Could not verify keyset from any servers using given key file.\n");
        m_iRound = 6;
      }
      m_iRound++;
    }
  }
  else if (m_iRound == 3)
  {
    processNs();
    m_iRound++;
  }
  else if (m_iRound == 4)
  {
    processA();
    if (verifiedNs.size() == 0)
    {
      //m_iRound = 6;
    }
  }
  else if (m_iRound == 5)
  {
    processA(privateName);
    m_iRound = 4;
  }
  else if (m_iRound == 6)
  {
    processDs();
    serialize();
    m_iRound++;
  }

  return bRet;
}

bool DsyncInitTask::prepareBlanketNs()
{
  bool bRet = false;
  DnsName oName(zDao.zoneName);
  
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

bool DsyncInitTask::prepareNs()
{
  bool bRet = false;
  setNameserver(validNameserver);
  
  DnsName oName(zDao.zoneName);
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

bool DsyncInitTask::prepareDs()
{
  bool bRet = false;
  setNameserver(0);
  DnsName oName(zDao.zoneName);

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


bool DsyncInitTask::processBlanketNs()
{
  bool bRet = false;
  
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
    badQueries++;
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
    badQueries++;
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
        unverifiedNs.push_back(((DnsNs*)pAnsRR)->getName());
      }
    }
  }
  return bRet;
}

bool DsyncInitTask::processNs()
{
  bool bRet = false;
  
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
    badQueries++;
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
    badQueries++;
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
    
   //verify results, if legit use them 
   /*
   for (RRIter_t it = keys.begin(); it != keys.end(); it++)
    {
       printf("Key: ");
      (*it)->print();
    }
    
    for (RRIter_t it = v_oNsAndSigs.begin(); it != v_oNsAndSigs.end(); it++)
    {
      printf("Ns/sig: ");
      (*it)->print();
    }
    */
    bool verified = verifier.verify(keys, v_oNsAndSigs);


    if (!verified)
    {
      ps_elog(PSL_CRITICAL, "Failed to verify NS set using verified keys off the wire.\n");
      exit(1);
    }
    else
    {
      ps_elog(PSL_INFO, "Verify OK for NS set.\n");
      for (RRIter_t it = v_oNsAndSigs.begin();
           it != v_oNsAndSigs.end();
           it++)
      {
        if ((*it)->type() == DNS_RR_NS)
        {
          //printf("verified ns: %s\n", ((DnsNs*)*it)->get_name()->toString().c_str());
          verifiedNs.push_back(((DnsNs*)*it)->getName());
        }
      }
    }
  }
  return bRet;
}

bool DsyncInitTask::processBlanketA()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
 
  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
    badQueries++;
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
    badQueries++;
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

          NsObj * temp = new NsObj();
          temp->name = pRR->get_name()->toString();
          temp->ip = tIP;
          temp->verified = false;
          uNs.push_back(temp);
          uNsB.push_back(uIP);          

          bRet = true;
        }
      }

    } 
  }
  return bRet;
}

bool DsyncInitTask::processA()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
 
  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
    badQueries++;
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
    badQueries++;
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
      
      //if REFUSED, we have to resort to a recursive query and hope it's legit
      if (pPkt->getHeader().get_rcode() == 5)
      {
        RRList_t oQuestList;
        if (!pPkt->getQuestions(oQuestList))
        {
          ps_elog(PSL_WARNING, "Unable to get questions from list.\n");
        }
        else 
        {
          privateName = (*oQuestList.begin())->get_name()->toString();
          ps_elog(PSL_WARNING, "No verifiable data for %s A.  Issuing a recursive query.\n", privateName.c_str());
          m_iRound = 5;
        }
      }
    }
    else
    {
      RRList_t AAndSig;
      NsObj * temp = new NsObj();
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
          temp->ip = tIP;
          temp->name = pRR->get_name()->toString();
          tuIP = uIP;
          bRet = true;
        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          AAndSig.push_back(pRR);
        }
      } 

      bool verified = verifier.verify(keys, AAndSig);

      if (verified)
      {
        temp->verified = true;
        ps_elog(PSL_INFO, "Verified A Record for '%s' as %s\n",temp->name.c_str(), temp->ip.c_str());
        vNs.push_back(temp);
        vNsB.push_back(tuIP);          
      }
      else
      {
        ps_elog(PSL_WARNING, "Unable to verify A record for '%s' as %s\n", temp->name.c_str(), temp->ip.c_str());
      }  

    } 
  }
  return bRet;
}

bool DsyncInitTask::processDs()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
    badQueries++;
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set");
    badQueries++;
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
          zDao.parentName = ((DnsRrsig*)(pRR))->getSignersName(); 
          printf("Got parent name of %s\n", zDao.parentName.c_str());
        }
      }
      if (oDs.size() == 0)
      {
        ps_elog(PSL_WARNING, "No DS Record Found\n");
        syncState = -1;
      }
      else if (oSig.size() == 0)
      {
        ps_elog(PSL_WARNING, "Unable to determine parent of %s, No sigs included with DS records\n", zDao.zoneName.c_str());
      }
      else
      {
        /*
        for (RRIter_t a = keys.begin(); a != keys.end(); a++)
        {
          printf("Key: ");
          (*a)->print();
        }
        */
        bool verified = false;
        for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
        {
          //printf("Ds:  ");
          //(*it)->print();
          if (verifier.verifyDs(*(DnsDs*)(*it), keys))
          {
            verified = true;
            break;
          }
        }

        if (verified)
        {
          ps_elog(PSL_INFO, "Found valid DS record at the parent. Matches our DNSKEY set but is unverified.\n");
          syncState = 0;        
          for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
          {
            DnsDs* ds = new DnsDs(*(DnsDs*)(*it));
            Ds.push_back(ds);
          }
          for (RRIter_t it = oSig.begin(); it != oSig.end(); it++)
          {
            DnsRrsig* sig = new DnsRrsig(*(DnsRrsig*)(*it));
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

bool DsyncInitTask::prepareBlanketA()
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  if (unverifiedNs.size() == 0)
  {
    return false;
  }
  DnsName oName(unverifiedNs.back());
  unverifiedNs.pop_back();
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

bool DsyncInitTask::prepareA()
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(verifiedNs.back());
  verifiedNs.pop_back();
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

bool DsyncInitTask::prepareDnskey()
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(zDao.zoneName);
  setNameserver(uNsB.back());
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

bool DsyncInitTask::processDnskey()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;

  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
    badQueries++;
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
    badQueries++;
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

      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      { 
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_DNSKEY)
        { 
          DnsDnskey *key = (DnsDnskey*)pRR;
          v_oKeysAndSigs.push_back(key);
          bRet = true;
        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          DnsRrsig * sig = (DnsRrsig*) pRR;
          v_oKeysAndSigs.push_back(sig);
        }
      }

      bool verified = verifier.verify(keyFromFile, v_oKeysAndSigs);
      
      if (!verified)
      {
        ps_elog(PSL_WARNING, "Verify failed (couldn't verify keyset) for zone '%s' from nameserver '%s'(%s) using key from input file.\n",
               (*v_oKeysAndSigs.begin())->get_name()->toString().c_str(), uNs.back()->name.c_str(), uNs.back()->ip.c_str());
        delete uNs.back();
	uNs.pop_back();
        uNsB.pop_back();
      }
      else
      {
        ps_elog(PSL_INFO, "Verify OK (verified keyset) for zone '%s' from nameserver '%s'(%s) using key from input file.\n",
               (*v_oKeysAndSigs.begin())->get_name()->toString().c_str(), uNs.back()->name.c_str(), uNs.back()->ip.c_str());

        //add the keys and their sigs to our vectors to be passed to the dao at the end
        for (RRIter_t oIter = oAnsList.begin();
            oAnsList.end() != oIter;
            oIter++)
        { 
          DnsRR *pRR = *oIter;
          if (pRR->type() == DNS_RR_DNSKEY)
          {
            DnsDnskey * tKey= new DnsDnskey(*(DnsDnskey*)pRR);
            keys.push_back(tKey);
          }
          if (pRR->type() == DNS_RR_RRSIG)
          {
            DnsRrsig * tSig = new DnsRrsig(*(DnsRrsig*)pRR);
            keySigs.push_back(tSig);
          }
        }

        validNameserverName = uNs.back()->name;
        validNameserverIp = uNs.back()->ip;
        validNameserver = uNsB.back();
   
        while (uNs.size() > 0)
        {
	  delete uNs.back();
          uNs.pop_back();
        }
        uNs.clear();
        uNsB.clear();
        bRet = true;
      }
    } 
  }
  return bRet;
}

void DsyncInitTask::serialize()
{
  ps_elog(PSL_DEBUG, "Writing zone '%s' to the database\n", zDao.zoneName.c_str());
  //printf("Serialize: keys.size() = %d\n", (int)keys.size());
  //printf("Serialize: Ds.size() = %d\n", (int)Ds.size());
  zDao.addKeyset(keys, keySigs, validNameserverName, validNameserverIp);
  zDao.addDsset(Ds, DsSigs);
  zDao.setLastNs(validNameserverName, validNameserverIp);
  zDao.setSyncState(syncState);
  for (NsVector::iterator it = vNs.begin(); it != vNs.end(); it++)
  {
    zDao.addNs((*it)->name, (*it)->ip, (*it)->verified);
    delete *it;
  }
  zDao.serialize();
  ps_syslog("Notification: Monitoring new zone: %s", zDao.zoneName.c_str());
  ps_elog(PSL_CRITICAL, "Notification: Monitoring new zone: %s\n", zDao.zoneName.c_str());
  //zDao.printAll();
}

bool DsyncInitTask::prepareA(std::string n)
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
  //printf("Preparing question for: %s, A\n", oName.toString().c_str());
  return bRet;
}

bool DsyncInitTask::processA(std::string n)
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
 
  if (NULL == (pPkt = getResponse()))
  {
    ps_elog(PSL_WARNING, "Response packet is NULL\n");
    badQueries++;
  }
  else if (pPkt->getHeader().get_tc())
  {
    ps_elog(PSL_WARNING, "Error: TC bit set\n");
    badQueries++;
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
      NsObj * temp = new NsObj();
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

          temp->ip = tIP;
          temp->name = pRR->get_name()->toString();
          temp->verified = false;
          tuIP = uIP;
          bRet = true;
    
          ps_elog(PSL_DEBUG, "Recursive query got a response: %s: %s\n", temp->name.c_str(), temp->ip.c_str());
        
          vNs.push_back(temp);
          vNsB.push_back(tuIP);          
        }
      }
    } 
  }
  return bRet;
}
