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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dns_poller_dao.h"
#include "dns_resolver.h"
#include "dns_packet.h"
#include "dns_dnskey.h"
#include "dns_name.h"
#include "ps_defs.h"
#include "dnskey_set_dao.h"
#include "dnskey_proc_dao.h"
#include "dao_factory.h"
#include "dnskey_poll_task.h"
#include "ps_url_parser.h"
#include "ps_config.h"

using namespace std;

const char *DnsPollerDao::s_kszDaoName = "DNS_POLLER_DAO";

const char *DnsPollerDao::s_szSelect = "select m.SRC \
  from DNSKEY_URL_MON_LIST m \
  where NEXT_POLL < ? \
  order by m.SRC;";

const char *DnsPollerDao::s_szSelectNextTime = "select VALUE \
  from PS_CONFIG \
  where NAME = 'POLL_PERIOD';";

const char *DnsPollerDao::s_kszUpdateList = "update DNSKEY_URL_MON_LIST set LAST_POLL = ?, NEXT_POLL = ?;";

DnsPollerDao::DnsPollerDao()
  : m_iPollPeriod(0)
{
  PsConfig &oConf = PsConfig::getInstance();

  m_oRes.setConcurrency(40);
  const char *szConc = oConf.getValue(PS_CONFIG_RES_CONC);
  if (NULL != szConc)
  {
    m_oRes.setConcurrency((int) strtol(szConc, NULL, 10));
  }

  m_oRes.setDO(true);
  m_oRes.setBuffSize(4096);
}

DnsPollerDao::~DnsPollerDao()
{
  clear();
}

bool DnsPollerDao::poll()
{
  bool bRet = false;

  if (!deserialize())
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize().\n");
  }
  // Not an error...
  else if (empty())
  {
    bRet = true;
  }
  else
  {
    DnskeyPollTask *pTask = NULL;
    UrlIter_t tUrlIter = begin();
    do
    {
//time_t tStartRecv = time(NULL);
      bool bSent = false;
      if (m_oRes.hasTasks())
      {
        bSent = true;

        for (int i = 0;
             i < m_oRes.getConcurrency()
             && m_oRes.hasTasks()
             && NULL != (pTask = (DnskeyPollTask *) m_oRes.recv());
             i++)
//        if (m_oRes.hasTasks()
//            && NULL != (pTask = (DnskeyPollTask *) m_oRes.recv()))
        {
//          if (NULL == (pTask = (DnskeyPollTask *) m_oRes.recv()))
//          {
//            continue;
//          }
//          else if (!pTask->done())
          if (!pTask->done())
          {
            if (!pTask->execute())
            {
              ps_elog(PSL_CRITICAL, "Unable to finish task... Dropping for zone: '%s'\n", pTask->getZone().c_str());
            }
            else if (!m_oRes.send(pTask))
            {
              ps_elog(PSL_CRITICAL, "Unable to re-send same task... Dropping for zone: '%s'\n", pTask->getZone().c_str());
            }
            else
            {
              pTask = NULL;
            }
          }
          else if (!extract(*pTask))
          {
#ifdef _PS_DEBUG
            ps_elog(PSL_CRITICAL, "Unable to process... Dropping for zone: '%s'\n", pTask->getZone().c_str());
#endif
          }

          if (NULL != pTask)
          {
            delete pTask;
            pTask = NULL;
          }
        }
      }
//time_t tStopRecv = time(NULL);

//time_t tStartSend = time(NULL);
//      if (end() != tUrlIter && m_oRes.hasRoomToSend())
      for (int j = 0;
           j < m_oRes.getConcurrency()
           && end() != tUrlIter
           && m_oRes.hasRoomToSend();
           j++)
      {
        bSent = true;

        string sType;
        string sClass;
        PsUrlParser oParser;
        if (!oParser.parse(*tUrlIter))
        {
          ps_elog(PSL_CRITICAL, "Unable to parse URL: '%s'\n", (*tUrlIter).c_str());
        }
        else if (oParser.getProto() != "dnssec")
        {
          ps_elog(PSL_CRITICAL, "Protocol is not DNSSEC: '%s'\n", oParser.getProto().c_str());
        }
        else if (!oParser.getParam("type", sType))
        {
          ps_elog(PSL_CRITICAL, "Unable to locate type of query in URL: '%s'.\n", (*tUrlIter).c_str());
        }
        else if (sType != "48")
        {
          ps_elog(PSL_CRITICAL, "Unable to query for type: '%s'\n", sType.c_str());
        }
        // <EMO> - TODO Should check the type and class HERE.
        else
        {
          pTask = new DnskeyPollTask();
          string &sZone = oParser.getQuery();
          pTask->setZone(sZone);
          if (!pTask->execute())
          {
            ps_elog(PSL_CRITICAL, "Unable to execute new task for zone '%s'... dropping.\n", sZone.c_str());
            delete pTask;
          }
          else if (!m_oRes.send(pTask))
          {
            ps_elog(PSL_CRITICAL, "Unable to send task for zone: '%s'\n", pTask->getZone().c_str());
            delete pTask;
          }
          pTask = NULL;
        }

        tUrlIter++;
      }
//time_t tStopSend = time(NULL);
//ps_elog(PSL_CRITICAL, "RECV LOOP TOOK %d SEND LOOP TOOK %d\n", (tStopRecv - tStartRecv), (tStopSend - tStartSend));
/*
      if (!bSent)
      {
        sleep(1);
      }
*/
    } while (m_oRes.hasTasks());

    if (!serialize())
    {
      ps_elog(PSL_CRITICAL, "Unable to serialize zone list.\n");
    }
    else
    {
      bRet = true;
    }
  }

  return bRet;
}

bool DnsPollerDao::extract(DnskeyPollTask &p_oTask)
{
  bool bRet = false;

  DnskeySetDao *pKeyDao = NULL;
  DnskeyProcDao *pProcDao = NULL;
  DnsPacket *pPkt = NULL;

  try
  {
    DaoFactory &oFact = DaoFactory::getInstance();
    pKeyDao = dynamic_cast<DnskeySetDao *>(oFact.create(DnskeySetDao::s_kszDaoName));
    pProcDao = dynamic_cast<DnskeyProcDao *>(oFact.create(DnskeyProcDao::s_kszDaoName));
    if (NULL == pKeyDao)
    {
      ps_elog(PSL_CRITICAL, "Unable to create DAO for name: '%s'\n", DnskeySetDao::s_kszDaoName);
    }
    else if (NULL == pProcDao)
    {
      ps_elog(PSL_CRITICAL, "Unable to create DAO for name: '%s'\n", DnskeySetDao::s_kszDaoName);
    }
    else if (NULL == (pPkt = p_oTask.getResponse()))
    {
#ifdef _PS_DEBUG
      ps_elog(PSL_CRITICAL, "Response packet is NULL\n");
#endif
      delete pKeyDao;
      pKeyDao = NULL;
      delete pProcDao;
      pProcDao = NULL;
    }
    else if (DNS_NOERROR != pPkt->getHeader().rcode())
    {
      ps_elog(PSL_CRITICAL, "Error getting DNSKEY: %d for zone: '%s'\n", pPkt->getHeader().rcode(), p_oTask.getZone().c_str());
      delete pKeyDao;
      pKeyDao = NULL;
      delete pProcDao;
      pProcDao = NULL;
    }
    else if (pPkt->getHeader().get_tc())
    {
      ps_elog(PSL_CRITICAL, "Error: TC bit sec for DNSKEY for zone: '%s'\n", p_oTask.getZone().c_str());
      delete pKeyDao;
      pKeyDao = NULL;
      delete pProcDao;
      pProcDao = NULL;
    }
    else
    {
      pProcDao->addRawDao(pKeyDao);

      RRList_t oAnsList;
      if (!pPkt->getAnswers(oAnsList))
      {
        ps_elog(PSL_CRITICAL, "Unable to get answers from list.\n");
      }
      else if (pPkt->getHeader().get_rcode() != DNS_NOERROR)
      {
        ps_elog(PSL_CRITICAL, "Query returned rcode: %d\n", pPkt->getHeader().get_rcode());
      }
      else
      {
        int iKeys = 0;
        pKeyDao->init(p_oTask.getZone(), p_oTask.getURL());
        for (RRIter_t oIter = oAnsList.begin();
             oAnsList.end() != oIter;
             oIter++)
        { 
          DnsRR *pRR = *oIter;
                        
          if (pRR->type() != DNS_RR_DNSKEY)
          { 
            fprintf(stdout, "Got RR type: %d\n", pRR->type());
          }
          else
          { 
            DnsDnskey *pKey = (DnsDnskey *) pRR;
#ifdef _PS_DEBUG
            string sName;
            DnsName *pKeyName = pKey->get_name();
            pKeyName->display_name(sName);
            fprintf(stdout, "'%s': <Type, Class, TTL> = <%u, %u, %u>\n\t<Flags, Proto, Algo> = <%u, %u, %u>\n\t%s\n",
                    sName.c_str(),
                    pKey->type(),
                    pKey->get_class(),
                    pKey->ttl(),
                    pKey->getFlags(),
                    pKey->getProto(),
                    pKey->getAlgo(),
                    pKey->getKey().c_str());
#endif // _PS_DEBUG
            pKeyDao->addKey(*pKey);
            iKeys++;
          }
        }

        pProcDao->setName(pKeyDao->getName());
        if (0 == iKeys)
        {
#ifdef _PS_DEBUG
          ps_elog(PSL_CRITICAL, "No keys in response from URL: '%s'.\n", p_oTask.getURL().c_str());
#endif
        }
//ps_elog(PSL_CRITICAL, "NUM KEYS IS %u\n", (unsigned) pKeyDao->getNumKeys());
        else if (!pKeyDao->serialize())
        {
          ps_elog(PSL_CRITICAL, "Unable to serialize key set for URL: '%s'.\n", p_oTask.getURL().c_str());
        }
        else if (!pProcDao->process())
        {
          ps_elog(PSL_CRITICAL, "Unable to execute process DAO\n");
        }
        else if (!pProcDao->serialize())
        {
          ps_elog(PSL_CRITICAL, "Unable to serialize process DAO\n");
        }
        else
        {
          m_oDaoList.push_back(pProcDao);
          pProcDao = NULL;
          bRet = true;
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

bool DnsPollerDao::serialize()
{
  bool bRet = false;

  time_t tNow = time(NULL);
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (!prepareQuery(s_kszUpdateList))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszUpdateList);
  }
  else if (!setInt(0, tNow))
  {
    ps_elog(PSL_CRITICAL, "Unable to set time in list update.\n");
  }
  else if (!setInt(1, tNow + m_iPollPeriod))
  {
    ps_elog(PSL_CRITICAL, "Unable to set time in scheduler update.\n");
  }
  else if (!update())
  {
    ps_elog(PSL_CRITICAL, "Unable to update scheduler.\n");
  }
  else
  {
    bRet = true;
  }
  disconnect();

  return bRet;
}

bool DnsPollerDao::deserialize()
{
  bool bRet = false;

  time_t tNow = time(NULL);
  string sPeriod;
  if (!clear())
  {
    ps_elog(PSL_CRITICAL, "Unable to clear.\n");
  }
  else if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (!prepareQuery(s_szSelectNextTime))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_szSelectNextTime);
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec().\n");
  }
  else if (!next())
  {
    ps_elog(PSL_CRITICAL, "No configuration value found for polling period.\n");
  }
  else if (!getStr(0, sPeriod))
  {
    ps_elog(PSL_CRITICAL, "Unable to get period from query.\n");
  }
  else
  {
    m_iPollPeriod = (int) strtol(sPeriod.c_str(), (char **)NULL, 10);
    disconnect();
    if (!connect(__FILE__, __LINE__))
    {
      ps_elog(PSL_CRITICAL, "Unable to connect to DB.\n");
    }
    else if (!prepareQuery(s_szSelect))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_szSelect);
    }
    else if (!setInt(0, tNow))
    {
      ps_elog(PSL_CRITICAL, "Unable to set current time.\n");
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to exec().\n");
    }
    else
    {
      while (next())
      {
        string sSrc;
        if (!getStr(0, sSrc))
        {
          ps_elog(PSL_CRITICAL, "Unable to getStr on index 0\n");
        }
        else
        {
          m_tList.push_back(sSrc);
        }
      }
      bRet = true;
    }
  }
  disconnect();

  return bRet;
}

bool DnsPollerDao::deserialize(DaoList_t &p_oOutputList)
{
  ps_elog(PSL_CRITICAL, "Not implemented.\n");
  return false;
}

DnsPollerDao *DnsPollerDao::dup()
{
  return new DnsPollerDao();
}

std::string DnsPollerDao::daoName()
{
  return string(s_kszDaoName);
}

UrlIter_t DnsPollerDao::begin()
{
  return m_tList.begin();
}

UrlIter_t DnsPollerDao::end()
{
  return m_tList.end();
}

size_t DnsPollerDao::size()
{
  return m_tList.size();
}

bool DnsPollerDao::empty()
{
  return m_tList.empty();
}

DaoIter_t DnsPollerDao::beginDao()
{
  return m_oDaoList.begin();
}

DaoIter_t DnsPollerDao::endDao()
{
  return m_oDaoList.end();
}

size_t DnsPollerDao::sizeDao()
{
  return m_oDaoList.size();
}

bool DnsPollerDao::emptyDao()
{
  return m_oDaoList.empty();
}

bool DnsPollerDao::clear()
{
  m_tList.clear();
  PsDao::clearList(m_oDaoList);

  return true;
}

