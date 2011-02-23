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

#include "dsync_second_task.h"
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

DsyncSecondTask::DsyncSecondTask(DsyncZoneDao* zD) :zDao(zD),  validNameserver(0), m_iRound(0), syncState(zD->getSyncState())
{
  //zDao = zD;  
  taskType = "second";
}

DsyncSecondTask::~DsyncSecondTask()
{
  delete zDao;
}

bool DsyncSecondTask::execute()
{
  //ps_elog(PSL_DEBUG, "Execute: m_iRound = %d\n", m_iRound);
  bool bRet = false;
  if (m_iRound == 0)
  {
    NsObj *qNs = zDao->getNsDao().getRandomNs();
    validNameserverName = qNs->name;
    validNameserverIp = qNs->ip;
    validNameserver = htonl(inet_addr(qNs->ip.c_str()));

    bRet = prepareNs(zDao->parentName, 0);
  }
  else if (m_iRound == 1)
  {
    if (NsList.size() == 0)
    {
      ps_elog(PSL_ERROR, "Error, query for nonexistant record\n");
      return false;
    }
    else
    {
      bRet = prepareA(NsList.back());
    }
  }
  else if (m_iRound == 2)
  {
    if (pNsB.size() == 0)
    {
      ps_elog(PSL_ERROR, "Error, query to nonexistant nameserver\n");
      return false;
    }
    else
    {
      bRet = prepareDs(pNsB.back());
      pNsB.clear();
    }
  } 
  else if (m_iRound == 3)
  {
    //query parent's NS set
    bRet = prepareNs(zDao->parentName, 0);
  }
  else if (m_iRound == 4)
  {
    //query 'A' records for parent's NS
    if (NsList.size() != 0)
    {
      bRet = prepareA(NsList.back());
    }
    else
    {
      ps_elog(PSL_ERROR, "Tried to build query for an NS that didn't exist... \n");
    }
  }
  else if (m_iRound == 5)
  {
    m_iRound = 6;
    execute();
  }
  else if (m_iRound == 6)
  {
    //query each nameserver for the DS
    if (pNsB.size() != 0)
    {
      prepareDs(pNsB.back());
      //pNsB.pop_back(); -- this is done in processDs() if the server shows the new ds
    }
    else
    {
      ps_elog(PSL_ERROR, "Tried to ask an NS that didn't exist for the DS record...\n");
    }
  }
  else if (m_iRound == 7)
  {
    //serialize();
    //all nameservers show the new DS, sleep for ds ttl
  }
  else if (m_iRound == 8)
  {
    //nothing
  }
  return bRet;
}

bool DsyncSecondTask::done()
{
  return m_iRound == 8;
}

bool DsyncSecondTask::process()
{
  //ps_elog(PSL_DEBUG, "Process: m_iRound = %d\n", m_iRound);
  bool bRet = false;
  if (m_iRound == 0)
  {
    processNs();
    m_iRound++;
  }
  else if (m_iRound == 1)
  {
    processA();
    NsList.clear();
    m_iRound++;
  }
  else if (m_iRound == 2)
  {
    if (processDs())
    {
      m_iRound++;
    }
    else
    {
      m_iRound = 7;
    }
  }
  else if (m_iRound == 3)
  {
    if (processNs())
    {
      m_iRound++;
    }
    else
    {
      m_iRound = 7;
    }
  }
  else if (m_iRound == 4)
  {
    processA();
    NsList.pop_back();
    if (NsList.size() == 0)
    {
      m_iRound = 5;
    }
  }
  else if (m_iRound == 5)
  {
    processA();
  }
  else if (m_iRound == 6)
  {
    processDsA();
    if (pNsB.size() == 0)
    {
      syncState = 12;
      sleepTTL();
    }
  }
  else if (m_iRound == 7)
  {
    serialize();
    m_iRound++;
  }
  return bRet;
}

bool DsyncSecondTask::prepareDs(uint32_t tNs)
{
  //ps_elog(PSL_DEBUG, "prepareDs() called\n");
  bool bRet = false;
  setNameserver(tNs);
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


void DsyncSecondTask::serialize()
{
  ps_elog(PSL_DEBUG, "Writing zone '%s' to the database\n", zDao->zoneName.c_str());
  zDao->setSyncState(syncState);
  zDao->serialize();

  //make sure we clean up all the memory, just in case
}

void DsyncSecondTask::sleepTTL()
{
  ps_elog(PSL_DEBUG, "All nameservers show the new DS record. Sleeping zone '%s' for the DS TTL period of %d\n", zDao->zoneName.c_str(), zDao->getDsTTL());
  zDao->setSyncState(syncState);
  zDao->serialize(zDao->getDsTTL());
  m_iRound = 8;
}

bool DsyncSecondTask::processDs()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
  RRList_t oAnsList;
  RRList_t oDs;
  RRList_t oSig;
  RRList_t oDsAndSig;
  RRList_t oDbDs;
  RRList_t oDbKeys;

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
          //oDs.push_back(new DnsDs(*(DnsDs*)pRR));
          DnsDs* ptemp = new DnsDs(*(DnsDs*)pRR);
          oDs.push_back(ptemp);
          oDsAndSig.push_back(ptemp);
          bRet = true;
        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          //oSig.push_back(new DnsRrsig(*(DnsRrsig*)pRR));
          DnsRrsig* ptemp = new DnsRrsig(*(DnsRrsig*)pRR);
          oSig.push_back(ptemp);
          oDsAndSig.push_back(ptemp);
        }
      }
      std::vector<DsyncRrsetDao*> daos = zDao->getRrsets();
      std::vector<DsyncRrsetDao*>::iterator iter;
      for (iter = daos.begin(); iter != daos.end(); iter++)
      {
        RRList_t keys = (*iter)->getKeyset();
        RRList_t dss = (*iter)->getDsset();
        for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
        {
          oDbKeys.push_back((DnsDnskey*)(*kit));
        }
        for (RRIter_t kit = dss.begin(); kit != dss.end(); kit++)
        {
          oDbDs.push_back((DnsDs*)(*kit));
        }
      }
      if (oDs.size() == 0)
      {
        ps_elog(PSL_WARNING, "No DS Record Found\n");
        syncState = -1;
      }
      else
      {
        
        //debugging info
        /*
	printf ("====================\nKeys(db):\n--------\n");
        for (RRIter_t it = oDbKeys.begin(); it != oDbKeys.end(); it++)
        {
          (*it)->print();
        }
        printf("Ds(live):\n-----------\n");
        for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
        {
          (*it)->print();
        }
        printf("====================\n\n");
        */
        bool verified = false;
        for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
        {
          if (verifier.verifyDs(*(DnsDs*)(*it), oDbKeys))
          {
            verified = true;
            break;
          }
        }
        //printf("Verified Ds = %d\n", verified);
        if (verified)
        {
           /* 
            printf("Live DS:\n");
            for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
            {
              (*it)->print();
            }
            printf("DB DS:\n");
            for (RRIter_t it = oDbDs.begin(); it != oDbDs.end(); it++)
            {
              (*it)->print();
            }
           */
            
            
            if (zDao->getRrsets().at(0)->setsMatch(oDs, oDbDs))
            {
              ps_elog(PSL_INFO, "Ds has not changed, waiting...\n");
  	      DsyncZoneDao::clearList(oDs);
              DsyncZoneDao::clearList(oSig);
              syncState = 1;
              bRet = false;
            }
            else
            {
              ps_elog(PSL_INFO, "Ds has changed \n");
              zDao->addDsset(oDs, oSig);
              zDao->printAll();
              bRet = true;
            }
        }
        else //(not verified)
        {
          ps_elog(PSL_WARNING, "Ds does not match any key in the set.\n");
          //zDao->printAll();

          /*
          for (RRIter_t t = oDs.begin(); t != oDs.end(); t++)
          {
            printf("live ds: ");
            (*t)->print();
          }
          */

          syncState = -1;
          m_iRound = 7;
        }
       
      }
    }   
  }
  return bRet;
}


bool DsyncSecondTask::processDsA()
{
  bool bRet = false;
  DnsPacket *pPkt = NULL;
  RRList_t oAnsList;
  RRList_t oDs;
  RRList_t oSig;
  RRList_t oDsAndSig;
  RRList_t oDbDs;
  RRList_t oDbKeys;

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
          DnsDs* ptemp = new DnsDs(*(DnsDs*)pRR);
          oDs.push_back(ptemp);
          //oDs.push_back(new DnsDs(*(DnsDs*)pRR));
          oDsAndSig.push_back(ptemp);
          bRet = true;
        }
        if (pRR->type() == DNS_RR_RRSIG)
        {
          DnsRrsig* ptemp = new DnsRrsig(*(DnsRrsig*)pRR);
          oSig.push_back(ptemp);
          //oSig.push_back(new DnsRrsig(*(DnsRrsig*)pRR));
          oDsAndSig.push_back(ptemp);
        }
      }
      std::vector<DsyncRrsetDao*> daos = zDao->getRrsets();
      std::vector<DsyncRrsetDao*>::iterator iter;
      for (iter = daos.begin(); iter != daos.end(); iter++)
      {
        RRList_t keys = (*iter)->getKeyset();
        RRList_t dss = (*iter)->getDsset();
        for (RRIter_t kit = keys.begin(); kit != keys.end(); kit++)
        {
          oDbKeys.push_back((DnsDnskey*)(*kit));
        }
        for (RRIter_t kit = dss.begin(); kit != dss.end(); kit++)
        {
          oDbDs.push_back((DnsDs*)(*kit));
        }
      }
      if (oDs.size() == 0)
      {
        ps_elog(PSL_WARNING, "No DS Record Found\n");
        syncState = -1;
      }
      else
      {
        //debugging info
        /*
	printf ("====================\nKeys(db):\n--------\n");
        for (RRIter_t it = oDbKeys.begin(); it != oDbKeys.end(); it++)
        {
          (*it)->print();
        }
        printf("Ds(live):\n-----------\n");
        for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
        {
          (*it)->print();
        }
        printf("====================\n\n");
        */
        bool verified = false;
        for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
        {
          if (verifier.verifyDs(*(DnsDs*)(*it), oDbKeys))
          {
            verified = true;
            break;
          }
        }
        //printf("Verified DS = %d\n", verified);
        if (verified)
        {
          {
            /*
            printf("Live DS:\n");
            for (RRIter_t it = oDs.begin(); it != oDs.end(); it++)
            {
              (*it)->print();
            }
            printf("DB DS:\n");
            for (RRIter_t it = oDbDs.begin(); it != oDbDs.end(); it++)
            {
              (*it)->print();
            }
            */

            if (zDao->getRrsets().at(0)->setsMatch(oDs, oDbDs))
            {
              ps_elog(PSL_DEBUG, "Saw updated DS set from this nameserver\n");
              bRet = true;
              pNsB.pop_back();
	      //delete pNs.back(); 
              //pNs.pop_back();
            }
            else
            {
              ps_elog(PSL_DEBUG, "Nameserver has not yet updated to new DS set\n");
              bRet = false;
            }
          }
        }
        else //(not verified)
        {
          ps_elog(PSL_WARNING, "Ds does not match any key in the set.\n");
          //zDao->printAll();

          /*
          for (RRIter_t t = oDs.begin(); t != oDs.end(); t++)
          {
            printf("live ds: ");
            (*t)->print();
          }
          */

          syncState = -1;
          m_iRound = 7;
        }
      }
    }   
  DsyncZoneDao::clearList(oDs);
  DsyncZoneDao::clearList(oSig);
  }
  return bRet;
}


bool DsyncSecondTask::prepareNs(std::string zName, uint32_t tNs)
{
  bool bRet = false;
  setNameserver (tNs);
  
  DnsName oName(zName);
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

bool DsyncSecondTask::processNs()
{
  bool bRet = false;
  NsList.clear();
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
    RRList_t oNsAndSigs;
    pPkt->getAnswers(oAns);
    for (oRrIter = oAns.begin();
         oRrIter != oAns.end();
         oRrIter++)
    {
      DnsRR *pAnsRR = *oRrIter;
      if (DNS_RR_NS == pAnsRR->type())
      {
        ps_elog(PSL_DEBUG, "Got a Nameserver: %s\n", ((DnsNs*)pAnsRR)->getName().c_str());
        oNsAndSigs.push_back(pAnsRR);
      }
      else if (DNS_RR_RRSIG == pAnsRR->type())
      {
        ps_elog(PSL_DEBUG, "Got RRSIG\n");
        oNsAndSigs.push_back(pAnsRR);
      }
    }

    for (oRrIter = oAns.begin();
         oRrIter != oAns.end();
         oRrIter++)
    {
      DnsRR *pAnsRR = *oRrIter;
      if (DNS_RR_NS == pAnsRR->type())
      {
        NsList.push_back(((DnsNs*)(pAnsRR))->getName());
        bRet = true;
      }
    }
  }
  return bRet;
}

bool DsyncSecondTask::prepareA(std::string name)
{
  bool bRet = false;
  DnsRR *pQuestionRR = NULL;
  DnsName oName(name);
  
  //printf("Building query for '%s A'\n", name.c_str());
  
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

bool DsyncSecondTask::processA()
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
      NsObj temp;// = new NsObj();
      for (RRIter_t oIter = oAnsList.begin();
           oAnsList.end() != oIter;
           oIter++)
      {
        DnsRR *pRR = *oIter;
        if (pRR->type() == DNS_RR_A)
        {
          uint32_t uIP = ((DnsA*)pRR)->ip();
          char tIP [32];
          bzero(tIP, 32);
          sprintf(tIP, "%d.%d.%d.%d", (uIP>>24)&0x00ff, (uIP>>16)&0x00ff, (uIP>>8)&0x00ff,uIP&0x00ff);
          temp.ip = tIP;
          temp.name = pRR->get_name()->toString();
          temp.verified = false;

          ps_elog(PSL_DEBUG, "Got parent nameserver: %s %s\n", temp.name.c_str(), temp.ip.c_str());

          //pNs.push_back(temp);
          pNsB.push_back(uIP);
          bRet = true;
        }
      }
    }
  }
  return bRet;
}
