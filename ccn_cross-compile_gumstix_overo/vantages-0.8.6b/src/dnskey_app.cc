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
#include <stdlib.h>

#include "dnskey_app.h"
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
#include "dnskey_web_task.h"
#include "dnskey_http_ctx.h"
#include "ps_mutex_hdlr.h"
#include "gpgme_crypt_mgr.h"
#include "ps_config.h"
#include "ps_crypt_fact.h"
#include "friend_rule.h"
#include "bind_ta_gen.h"
#include "ps_logger.h"
#include "dnskey_defs.h"

using namespace std;

const char *DnskeyApp::s_kszDaoName = "DNSKEY_APP_DAO";

const char *DnskeyApp::s_szSelect = "select SRC, SCRAPER_ID, NAME, SCR_PARAM_ID from (select SRC, SCRAPER_ID, NAME, random() as r, SCR_PARAM_ID \
  from DNSKEY_URL_MON_LIST \
  where NEXT_POLL < ?) \
  order by r limit ?;";

/*
const char *DnskeyApp::s_szSelect = "select SRC, SCRAPER_ID, NAME \
  from DNSKEY_URL_MON_LIST \
  where NEXT_POLL < ? \
  order by SRC;";
*/

const char *DnskeyApp::s_szSelectNextTime = "select VALUE \
  from PS_CONFIG \
  where NAME = 'POLL_PERIOD';";

const char *DnskeyApp::s_kszUpdateList = "update DNSKEY_URL_MON_LIST set LAST_POLL = ?, NEXT_POLL = ? where SRC = ?;";

const char *DnskeyApp::s_kszInsertMonTarget = "insert into DNSKEY_URL_MON_LIST (NAME, SRC, SCRAPER_ID, SUB_SRC, FIRST_POLL, SCR_PARAM_ID) \
  values (?, ?, ?, ?, ?, ?)";
const char *DnskeyApp::s_kszInsertRunStats = "insert into DNSKEY_RUN_STATS (START_TIME, STOP_TIME) values (?, ?)";

const char *DnskeyApp::s_kszClearStaleMons = "delete from DNSKEY_URL_MON_LIST where FIRST_POLL < ? and FIRST_POLL != 0";
const char *DnskeyApp::s_kszClearStaleProcSelfRel = "delete from PS_PROC_SELF_REL \
                                                     where PRE_ID in (select ID from PS_PROC_DATA where LAST_SEEN < ? and NS_ID = ?) \
                                                     or POST_ID in (select ID from PS_PROC_DATA where LAST_SEEN < ? and NS_ID = ?)";
const char *DnskeyApp::s_kszClearStaleProcRawRel = "delete from PS_PROC_RAW_REL \
                                                      where PROC_ID in (select ID from PS_PROC_DATA where LAST_SEEN < ? and NS_ID = ?) \
                                                        or RAW_ID in (select ID from PS_RAW_DATA where LAST_SEEN < ? and NS_ID = ?)";
const char *DnskeyApp::s_kszClearStaleProc = "delete from PS_PROC_DATA where LAST_SEEN < ? and NS_ID = ?";
const char *DnskeyApp::s_kszClearStaleRaw = "delete from PS_RAW_DATA where LAST_SEEN < ? and NS_ID = ?";

DnskeyApp::DnskeyApp()
  : m_bInit(false),
    m_iPollPeriod(0),
    m_tStartTime(0),
    m_tStopTime(0)
{

}

DnskeyApp::~DnskeyApp()
{
  clear();
}

bool DnskeyApp::load()
{
  bool bRet = false;

  m_tStopTime = 0;
  m_tStartTime = time(NULL);
  if (!deserialize())
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize.\n");
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool DnskeyApp::serialize()
{
  bool bRet = false;

  time_t tNow = time(NULL);
  m_tStopTime = tNow;
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else
  {
    fprintf(stderr, "Storing...\n");

    for (UrlIter_t tUrlIter = beginUrl();
         endUrl() != tUrlIter;
         tUrlIter++)
    {
      string &sUrl = *tUrlIter;
      clearQuery();
      if (!prepareQuery(s_kszUpdateList))
      {
        ps_elog(PSL_ERROR, "Unable to prepare query: '%s' '%s'\n", s_kszUpdateList, getError());
      }
      else if (!setInt(0, tNow))
      {
        ps_elog(PSL_ERROR, "Unable to set time in list update '%s'.\n", getError());
      }
      else if (!setInt(1, tNow + m_iPollPeriod))
      {
        ps_elog(PSL_ERROR, "Unable to set time in scheduler update '%s'.\n", getError());
      }
      else if (!setStr(2, sUrl))
      {
        ps_elog(PSL_ERROR, "Unable to set URL scheduler update '%s'.\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update scheduler '%s'.\n", getError());
      }
    }

    //
    // CRITICAL SECTION BEGIN
    //
    {
      PsMutexHandler oHdlr(m_oHopperMutex);

      if (!m_oHopperList.empty())
      {
        for (list<DnskeyPollTask *>::iterator tIter = m_oHopperList.begin();
             m_oHopperList.end() != tIter;
             tIter++)
        {
          clearQuery();
          DnskeyPollTask *pTask = *tIter;
          int iParamID = pTask->getScraperParamID();
          if (!prepareQuery(s_kszInsertMonTarget))
          {
            ps_elog(PSL_ERROR, "Unable to prepareQuery() for: '%s': '%s'\n", s_kszInsertMonTarget, getError());
          }
          else if (!setStr(0, pTask->getName()))
          {
            ps_elog(PSL_ERROR, "Unable to set name of task: '%s'\n", getError());
          }
          else if (!setStr(1, pTask->getURL()))
          {
            ps_elog(PSL_ERROR, "Unable to set Src: '%s'\n", getError());
          }
          else if (!setInt(2, pTask->getScraperID()))
          {
            ps_elog(PSL_ERROR, "Unable to set scraper ID: '%s'\n", getError());
          }
          else if (!setInt(3, pTask->getSubmissionSrc()))
          {
            ps_elog(PSL_ERROR, "Unable to set submission src: '%s'\n", getError());
          }
          else if (!setInt(4, time(NULL)))
          {
            ps_elog(PSL_ERROR, "Unable to set first poll time: '%s'\n", getError());
          }
          else if (-1 == iParamID && !setNULL(5))
          {
            ps_elog(PSL_ERROR, "Unable to set NULL param ID: '%s'\n", getError());
          }
          else if (-1 != iParamID && !setInt(5, iParamID))
          {
            ps_elog(PSL_ERROR, "Unable to set param ID: '%s'\n", getError());
          }
          else if (!update())
          {
            ps_elog(PSL_WARNING, "Unable to update '%s' with scraper ID %d: '%s'\n",
                    pTask->getURL().c_str(),
                    pTask->getScraperID(),
                    getError());
          }
          delete pTask;
        }
        m_oHopperList.clear();
      }
    }
    //
    // CRITICAL SECTION END
    //

    PsConfig &oConfig = PsConfig::getInstance();

    int iMonTimeout = DNSKEY_DEFAULT_MON_TIMEOUT;
    const char *szMonTimeout = NULL;
    if (NULL == (szMonTimeout = oConfig.getValue(DNSKEY_CONFIG_MON_TIMEOUT))
        || 0 < (iMonTimeout = (int) strtol(szMonTimeout, NULL, 10)))
    {
      clearQuery();
      iMonTimeout = time(NULL) - iMonTimeout;
      if (!prepareQuery(s_kszClearStaleMons))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszClearStaleMons);
      }
      else if (!setInt(0, iMonTimeout))
      {
        ps_elog(PSL_CRITICAL, "Unable to set mon timeout %d.\n", iMonTimeout);
      }
      else if (!update())
      {
        ps_elog(PSL_CRITICAL, "Unable to clear stale mon list.\n");
      }
    }

    int iDataTimeout = DNSKEY_DEFAULT_DATA_TIMEOUT;
    const char *szDataTimeout = NULL;
    if (NULL == (szDataTimeout = oConfig.getValue(DNSKEY_CONFIG_DATA_TIMEOUT))
        || 0 < (iDataTimeout = (int) strtol(szDataTimeout, NULL, 10)))
    {
      clearQuery();
      iDataTimeout = time(NULL) - iDataTimeout;
      if (!prepareQuery(s_kszClearStaleProcSelfRel))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszClearStaleProcSelfRel);
      }
      else if (!setInt(0, iDataTimeout))
      {
        ps_elog(PSL_CRITICAL, "Unable to set mon timeout %d.\n", iDataTimeout);
      }
      else if (!setInt(1, DNSKEY_NAME_SPACE_ID))
      {
        ps_elog(PSL_CRITICAL, "Unable to set name space ID %d.\n", DNSKEY_NAME_SPACE_ID);
      }
      else if (!update())
      {
        ps_elog(PSL_CRITICAL, "Unable to clear stale proc self rel.\n");
      }

      clearQuery();
      if (!prepareQuery(s_kszClearStaleProcRawRel))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszClearStaleProcRawRel);
      }
      else if (!setInt(0, iDataTimeout))
      {
        ps_elog(PSL_CRITICAL, "Unable to set mon timeout %d.\n", iDataTimeout);
      }
      else if (!setInt(1, DNSKEY_NAME_SPACE_ID))
      {
        ps_elog(PSL_CRITICAL, "Unable to set name space ID %d.\n", DNSKEY_NAME_SPACE_ID);
      }
      else if (!update())
      {
        ps_elog(PSL_CRITICAL, "Unable to clear stale proc raw rel.\n");
      }

      clearQuery();
      if (!prepareQuery(s_kszClearStaleProc))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszClearStaleProc);
      }
      else if (!setInt(0, iDataTimeout))
      {
        ps_elog(PSL_CRITICAL, "Unable to set mon timeout %d.\n", iDataTimeout);
      }
      else if (!setInt(1, DNSKEY_NAME_SPACE_ID))
      {
        ps_elog(PSL_CRITICAL, "Unable to set name space ID %d.\n", DNSKEY_NAME_SPACE_ID);
      }
      else if (!update())
      {
        ps_elog(PSL_CRITICAL, "Unable to clear stale proc.\n");
      }

      clearQuery();
      if (!prepareQuery(s_kszClearStaleRaw))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszClearStaleRaw);
      }
      else if (!setInt(0, iDataTimeout))
      {
        ps_elog(PSL_CRITICAL, "Unable to set mon timeout %d.\n", iDataTimeout);
      }
      else if (!setInt(1, DNSKEY_NAME_SPACE_ID))
      {
        ps_elog(PSL_CRITICAL, "Unable to set name space ID %d.\n", DNSKEY_NAME_SPACE_ID);
      }
      else if (!update())
      {
        ps_elog(PSL_CRITICAL, "Unable to clear stale raw.\n");
      }
    }

    clearQuery();
    if (!prepareQuery(s_kszInsertRunStats))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_kszInsertRunStats);
    }
    else if (!setInt(0, (int) m_tStartTime))
    {
      ps_elog(PSL_CRITICAL, "Unable to set start time.\n");
    }
    else if (!setInt(1, (int) m_tStopTime))
    {
      ps_elog(PSL_CRITICAL, "Unable to set stop time.\n");
    }
    else if (!update())
    {
      ps_elog(PSL_CRITICAL, "Unable to insert run stats.\n");
    }
    else
    {
      bRet = true;
    }
  }
  disconnect();

  return bRet;
}

bool DnskeyApp::deserialize()
{
  bool bRet = false;

  time_t tNow = time(NULL);
  string sPeriod;
  GpgmeCryptMgr *pMainCryptMgr = dynamic_cast<GpgmeCryptMgr *>(getCryptMgr());
  if (!clear())
  {
    ps_elog(PSL_CRITICAL, "Unable to clear.\n");
  }
  else if (NULL == pMainCryptMgr)
  {
    ps_elog(PSL_CRITICAL, "Crypt mgr not set.\n");
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
    // How many tasks should we load at once?
    int iChunkSize = PS_DEFAULT_DB_POLL_CHUNK_SIZE;
    const char *szPollChunkSize = PsConfig::getInstance().getValue(PS_CONFIG_DB_POLL_CHUNK_SIZE);
    if (NULL != szPollChunkSize)
    {
      iChunkSize = (int) strtol(szPollChunkSize, NULL, 10);
    }

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
    else if (!setInt(0, (int) tNow))
    {
      ps_elog(PSL_CRITICAL, "Unable to set current time.\n");
    }
    else if (!setInt(1, iChunkSize))
    {
      ps_elog(PSL_CRITICAL, "Unable to set chunk size.\n");
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to exec().\n");
    }
    else
    {
      string sCryptName = GpgmeCryptMgr::GPGMR_CRYPT_MGR;
      fprintf(stderr, "Loading...\n");

      ps_elog(PSL_DEBUG, "Loading...\n");
      int i = 0;

      while (next())
      {
        i++;
        string sSrc;
        string sName;
        if (!getStr(0, sSrc))
        {
          ps_elog(PSL_CRITICAL, "Unable to getStr on index 0\n");
        }
        else if (!getStr(2, sName))
        {
          ps_elog(PSL_CRITICAL, "Unable to getStr() on index 2 (name).\n");
        }
        else
        {
          m_tUrlList.push_back(sSrc);
          m_tList.push_back(sName);
          string sType;
          string sClass;
          PsUrlParser oParser;
          if (!oParser.parse(sSrc))
          {
            ps_elog(PSL_CRITICAL, "Unable to parse URL: '%s'\n", sSrc.c_str());
          }
          else if (oParser.getProto() == "dnssec")
          {
            if (!oParser.getParam("type", sType))
            {
              ps_elog(PSL_CRITICAL, "Unable to locate type of query in URL: '%s'.\n", sSrc.c_str());
            }
            else if (sType != "48")
            {
              ps_elog(PSL_CRITICAL, "Unable to query for type: '%s'\n", sType.c_str());
            }
            else
            {
              DnskeyPollTask *pTask = new DnskeyPollTask(*this);
              pTask->init(sSrc,
                          sName,
                          DnskeySetDao::s_kszDaoName,
                          DnskeyProcDao::s_kszDaoName,
                          getInt(1));
              m_oTaskList.push_back(pTask);
            }
          }
          else if (oParser.getProto() == "http" || oParser.getProto() == "https")
          {
            GpgmeCryptMgr *pMgr = static_cast<GpgmeCryptMgr *>(PsCryptFactory::getInstance().create(sCryptName));
            if (NULL == pMgr)
            {
              ps_elog(PSL_CRITICAL, "Unable to create new GPGME crypto mgr.\n");
            }
            else
            {
              pMgr->setHomeDir(static_cast<GpgmeCryptMgr *>(getCryptMgr())->getHomeDir().c_str());

              if (!pMgr->init())
              {
                ps_elog(PSL_CRITICAL, "Unable to init new GPGME crypto mgr.\n");
                delete pMgr;
              }
              else if (!pMgr->setSigningKey(pMainCryptMgr->getSigningKey()))
              {
                ps_elog(PSL_CRITICAL, "Unable to set signing key.\n");
                delete pMgr;
              }
              else if (!pMgr->setPP(pMainCryptMgr->getPP()))
              {
                ps_elog(PSL_CRITICAL, "Unable to set PP.\n");
                delete pMgr;
              }
              else
              {
                int iParamID = -1;
                if (!isNULL(3))
                {
                  iParamID = getInt(3);
                }

                ps_elog(PSL_DEBUG, "Enqueuing: '%s' -> '%s'\n", sName.c_str(), sSrc.c_str());
                DnskeyWebTask *pTask = new DnskeyWebTask(*this);
                pTask->setCryptMgr(pMgr);
                pTask->init(sSrc,
                            sName,
                            RawDataDao::s_kszDaoName,
                            DnskeyProcDao::s_kszDaoName,
                            getInt(1),
                            iParamID);
                m_oTaskList.push_back(pTask);
              }
            }
          }
          else
          {
            ps_elog(PSL_CRITICAL, "Protocol is not DNS or HTTP: '%s'\n", oParser.getProto().c_str());
          }
        }
      }
      bRet = true;
      fprintf(stderr, "Loaded: %d\n", i);
      ps_elog(PSL_DEBUG, "Loaded: %d\n", i);
    }
  }
  disconnect();
  ps_elog(PSL_DEBUG, "Done...\n");

  return bRet;
}

  bool DnskeyApp::deserialize(DaoList_t &p_oOutputList)
  {
    ps_elog(PSL_CRITICAL, "Not implemented.\n");
    return false;
  }

  DnskeyApp *DnskeyApp::dup()
  {
    return new DnskeyApp();
  }

  std::string DnskeyApp::daoName()
  {
    return string(s_kszDaoName);
  }

  UrlIter_t DnskeyApp::begin()
  {
    return m_tList.begin();
  }

  UrlIter_t DnskeyApp::end()
  {
    return m_tList.end();
  }

  size_t DnskeyApp::size()
  {
    return m_tList.size();
  }

  bool DnskeyApp::empty()
  {
    return m_tList.empty();
  }

  UrlIter_t DnskeyApp::beginUrl()
  {
    return m_tUrlList.begin();
  }

  UrlIter_t DnskeyApp::endUrl()
  {
    return m_tUrlList.end();
  }

  size_t DnskeyApp::sizeUrl()
  {
    return m_tUrlList.size();
  }

  bool DnskeyApp::emptyUrl()
  {
    return m_tUrlList.empty();
  }

  PsPollTaskIter_t DnskeyApp::beginTask()
  {
    return m_oTaskList.begin();
  }

  PsPollTaskIter_t DnskeyApp::endTask()
  {
    return m_oTaskList.end();
  }

  size_t DnskeyApp::sizeTask()
  {
    return m_oTaskList.size();
  }

  bool DnskeyApp::emptyTask()
  {
    return m_oTaskList.empty();
  }

  int DnskeyApp::getPollPeriod()
  {
    return m_iPollPeriod;
  }

  bool DnskeyApp::clear()
  {
    m_tList.clear();
    m_tUrlList.clear();
    for (PsPollTaskIter_t tIter = beginTask();
         endTask() != tIter;
         tIter++)
    {
      delete (*tIter);
    }
    m_oTaskList.clear();
    m_iPollPeriod = 0;

    return true;
  }

  const char *DnskeyApp::getHttpPath()
  {
    return "/dnskey";
  }

  const char *DnskeyApp::getHttpPass()
  {
    return PsConfig::getInstance().getValue(DNSKEY_CONFIG_APP_PASS);
  }

  HttpListenerCtx *DnskeyApp::createCtx()
  {
    return new DnskeyHttpCtx();
  }

  bool DnskeyApp::enabled()
  {
    return (NULL != PsConfig::getInstance().getValue(PS_CONFIG_DNSKEY_APP_ENABLED));
  }

  bool DnskeyApp::addNewTarget(std::string &p_sName, std::string &p_sURL, int p_iScraperID, dnskey_src_e p_eSrc, int p_iScraperParamID /*= -1*/)
  {
    bool bRet = false;

    //
    // CRITICAL SECTION BEGIN
    //
    {
      PsMutexHandler oHdlr(m_oHopperMutex);

      DnskeyPollTask *pTask = new DnskeyPollTask(*this);
      pTask->init(p_sURL,
                  p_sName,
                  DnskeySetDao::s_kszDaoName,
                  DnskeyProcDao::s_kszDaoName,
                  p_iScraperID,
                  p_iScraperParamID);
      pTask->setSubmissionSrc(p_eSrc);
      m_oHopperList.push_back(pTask);

      bRet = true;
    }
    //
    // CRITICAL SECTION END
    //

    return bRet;
  }

  bool DnskeyApp::init()
  {
    bool bRet = false;

    try
    {
      m_bInit = false;
      PsConfig &oConfig = PsConfig::getInstance();

      if (NULL != oConfig.getValue(DNSKEY_CONFIG_PCAP_ENABLE))
      {
        if (!m_oPcap.init(*this))
        {
          ps_elog(PSL_CRITICAL, "Unable to init pcap.\n");
        }
        else
        {
          ps_elog(PSL_DEBUG, "Initialized pcap thread.\n");
        }
      }

      m_bInit = true;
      bRet = true;
    }
    catch(...)
    {
      ps_elog(PSL_CRITICAL, "Caught Exception... Unable to init DNSKEY app.\n");
      bRet = false;
      m_bInit = false;
    }

    return bRet;
  }

  bool DnskeyApp::execute()
  {
    bool bRet = false;

    if (!m_bInit)
    {
      ps_elog(PSL_CRITICAL, "Unable to run app if not initialized.\n");
    }
    else
    {
      PsConfig &oConfig = PsConfig::getInstance();

      int iDnsConc = 40;
      const char *szConc = oConfig.getValue(PS_CONFIG_RES_CONC);
      if (NULL != szConc)
      {
        iDnsConc = (int) strtol(szConc, NULL, 10);
      }

      int iThrdCnt = 10;
      const char *szThrds = oConfig.getValue(PS_NUM_POLLER_THREADS);
      if (NULL != szThrds)
      {
        iThrdCnt = (int) strtol(szThrds, NULL, 10);
      }

      m_oPoller.init(iDnsConc, iThrdCnt);

      if (NULL != oConfig.getValue(DNSKEY_CONFIG_PCAP_ENABLE))
      {
        if (!m_oPcap.start())
        {
          ps_elog(PSL_CRITICAL, "Unable to start pcap thread.\n");
        }
        else
        {
          ps_elog(PSL_DEBUG, "Started pcap thread.\n");
        }
      }

      while (getRun())
      {
        clear();
        time_t tStart = time(NULL);

      if (!load())
      {
        ps_elog(PSL_CRITICAL, "Unable to load DnskeyApp.\n");
      }
      else if (beginTask() == endTask())
      {
        ps_elog(PSL_DEBUG, "No tasks to polle, sleeping.\n");
        // This is done to keep the hopper watched... 
        serialize();
        sleep(10);
      }
      else
      {
        fprintf(stderr, "Polling...\n");
        if (!m_oPoller.poll(beginTask(), endTask()))
        {
          ps_elog(PSL_CRITICAL, "Unable to poll zones.\n");
        }
      
        if (!serialize())
        {
          ps_elog(PSL_CRITICAL, "Unable to serialize DNSKEY App.\n");
        }

        FriendRule oRule;
        oRule.init(begin(),
                   end(),
                   (int) (time(NULL) - getPollPeriod() * 3),
                   getCryptMgr());
//ps_elog(PSL_CRITICAL, "Processing rule...\n");
        oRule.process();

        const char *szTaFile = oConfig.getValue(PS_CONFIG_TA_FILE_NAME);
        if (NULL != szTaFile && oRule.numZones() > 0)
        {
          fprintf(stderr, "Num zones: %u\n", (unsigned) oRule.numZones());
          BindTaGen oTaGen;
          oTaGen.setStart(tStart);
          string sFile = szTaFile;
          if (!oTaGen.genFile(sFile))
          {
            ps_elog(PSL_INFO, "Unable to generate TA file.\n");
          }
        }
      }
    }

    if (NULL != oConfig.getValue(DNSKEY_CONFIG_PCAP_ENABLE))
    {
      ps_elog(PSL_INFO, "Killing and joining pcap thread.\n");
      m_oPcap.kill();
      m_oPcap.join();
    }
  }

  return bRet;
}

