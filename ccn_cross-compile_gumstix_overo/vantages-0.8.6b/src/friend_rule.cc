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
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>
#include <iostream>

#include "ps_config.h"
#include "friend_rule.h"
#include "ps_defs.h"
#include "dao_factory.h"
#include "dnskey_proc_dao.h"
#include "ps_url_parser.h"
#include "ps_dao.h"
#include "friend_dao.h"
#include "http_query.h"
#include "dnskey_consist_dao.h"
#include "gpgme_crypt_mgr.h"
#include "dnskey_defs.h"
#include "ps_logger.h"

using namespace std;

FriendRule::FriendRule()
  : m_pCryptMgr(NULL)
{

}

FriendRule::~FriendRule()
{
  PsDao::clearList(m_oDaoList);
  for (KeyMapIter_t tIter = m_oKeyMap.begin();
       m_oKeyMap.end() != tIter;
       tIter++)
  {
    delete tIter->second;
  }
}

bool FriendRule::init(StrListIter_t p_tBegin,
                      StrListIter_t p_tEnd,
                      int p_iThresh,
                      PsCryptMgr *p_pCryptMgr)
{
  bool bRet = false;

  PsDao::clearList(m_oDaoList);
//  FriendDao *pLoader = dynamic_cast<FriendDao *>(DaoFactory::getInstance().create(FriendDao::s_kszDaoName));
  FriendDao *pLoader = (FriendDao *)(DaoFactory::getInstance().create(FriendDao::s_kszDaoName));
  if (NULL == pLoader)
  {
    ps_elog(PSL_CRITICAL, "Got NULL for FriendDao.\n");
  }
  else if (!pLoader->deserialize(m_oDaoList))
  {
    ps_elog(PSL_CRITICAL, "Unable to laod FriendDao list.\n");
    delete pLoader;
  }
  else
  {
//ps_elog(PSL_CRITICAL, "Loaded %u friends\n", (unsigned) m_oDaoList.size());
    bRet = true;
    delete pLoader;
  }

  m_pCryptMgr = static_cast<GpgmeCryptMgr *>(p_pCryptMgr);
  if (NULL == m_pCryptMgr)
  {
    ps_elog(PSL_CRITICAL, "Crypto manager specified is NULL, so no crypto verification will be performed.\n");
  }

  int iNumFriends = m_oDaoList.size();

  m_oNameList.clear();
  for (StrListIter_t tIter = p_tBegin;
       p_tEnd != tIter;
       tIter++)
  {
    string sName = *tIter;
    DnskeyConsistencyDao *pConDao = NULL;

    if (sName == "")
    {
      ps_elog(PSL_ERROR, "Got empty name in input list.\n");
    }
    else if (NULL == (pConDao = (DnskeyConsistencyDao *)DaoFactory::getInstance().create(DnskeyConsistencyDao::s_kszDaoName)))
    {
      ps_elog(PSL_CRITICAL, "Unable to create DnskeyConsistencyDao.\n");
    }
    else if (m_oKeyMap.end() != m_oKeyMap.find(sName))
    {
      ps_elog(PSL_INFO, "Found duplicate name '%s'\n", sName.c_str());
      delete pConDao;
    }
    else
    {
      m_oNameList.push_back(sName);

      ps_elog(PSL_DEBUG, "Name is = '%s'\n", sName.c_str());
      pConDao->setName(sName);
      pConDao->setNumFriends(iNumFriends);
      m_oKeyMap[sName] = pConDao;
      DaoList_t tList;
      DnskeyProcDao *pQueryDao = (DnskeyProcDao *)DaoFactory::getInstance().create(DnskeyProcDao::s_kszDaoName);
      pQueryDao->setName(sName);
      pQueryDao->setScraper(NULL);
      pQueryDao->setScraperID(-1);
      if (!pQueryDao->deserialize(tList))
      {
        ps_elog(PSL_CRITICAL, "Unable to desrialize list for DnskeyProcDao: '%s'\n", sName.c_str());
      }
      else
      {
        DnskeyProcDao *pCurDao = NULL;
        map<int, DnskeyProcDao *> oLastDaoMap;

        // ps_elog(PSL_DEBUG, "FOUND: %u entries.\n", (unsigned) tList.size());
        for (DaoIter_t tDaoIter = tList.begin();
             tList.end() != tDaoIter;
             tDaoIter++)
        {
          pCurDao = (DnskeyProcDao *) (*tDaoIter);

          if (pCurDao->getName() != sName)
          {
            ps_elog(PSL_CRITICAL, "DB returned DAO with wrong name: '%s' is not this DAO's name: '%s'\n",
                    pCurDao->getName().c_str(),
                    sName.c_str());
          }
          // We do not want to use the results of the LAST time we ran the consistence DAO as an example
          // of the actual DNSKEY
          else if (0 != strncmp(pCurDao->daoName().c_str(), DnskeyConsistencyDao::s_kszDaoName, 1024))
          {
            map<int, DnskeyProcDao *>::iterator tMapIter = oLastDaoMap.find(pCurDao->getScraperID());
            if (pCurDao->getDate() > p_iThresh
                && (oLastDaoMap.end() == tMapIter
                    || tMapIter->second->getDate() < pCurDao->getDate()))
            {
              oLastDaoMap[pCurDao->getScraperID()] = pCurDao;
            }
          }
        }

        for (map<int, DnskeyProcDao *>::iterator tMapIter = oLastDaoMap.begin();
             oLastDaoMap.end() != tMapIter;
             tMapIter++)
        {
          DnskeyProcDao *pRemoteDao = tMapIter->second;
          if (pRemoteDao->getName() != sName)
          {
            ps_elog(PSL_CRITICAL, "Remote DAO has wrong name: '%s' (this is not this DAO's name: '%s')\n",
                    pRemoteDao->getName().c_str(),
                    sName.c_str());
          }
          else
          {
            pConDao->addProcDnskey(*pRemoteDao);
            ps_elog(PSL_DEBUG, "FOR ZONE: '%s' DAO '%s' added '%s'\n",
                    sName.c_str(),
                    pConDao->getName().c_str(),
                    pRemoteDao->getData().c_str());
          }
        }
      }
      PsDao::clearList(tList);
      delete pQueryDao;
    }
  }

//ps_elog(PSL_CRITICAL, "Added %d URLs\n", i);

  return bRet;
}

bool FriendRule::process()
{
  bool bRet = true;

  int iNumZones = numZones();
  if (iNumZones > 0)
  {
    int i = 0;
    ostringstream oSS;
    for (StrListIter_t tIter = beginZones();
         endZones() != tIter;
         tIter++)
    {
      if (i++ > 0)
      {
        oSS << ",";
      }

      oSS << *tIter;
    }

    const char *szMaxAge = PsConfig::getInstance().getValue(PS_CONFIG_TA_MAX_AGE);
    string sMinTimeKey = DNSKEY_HTTP_MIN_TIME_KEY;
    string sMinTime;
    if (NULL != szMaxAge)
    {
      char szTime[11];
      memset(szTime, 0, 11);
      sprintf(szTime, "%d", (int) (time(NULL) - (int) strtol(szMaxAge, NULL, 10)));
      sMinTime = szTime;
    }

    string sKey = PS_HTTP_ZONE_LIST_KEY;
    string sValue = oSS.str();
    for (DaoIter_t tDaoIter = m_oDaoList.begin();
         m_oDaoList.end() != tDaoIter;
         tDaoIter++)
    {
      HttpQuery oQuery;
      FriendDao *pDao = (FriendDao *)(*tDaoIter);
      string &sURL = pDao->getURL();
      ps_elog(PSL_DEBUG, "Querying friend: '%s'\n", sURL.c_str());

      if (NULL != szMaxAge)
      {
        oQuery.setPostData(sMinTimeKey, sMinTime);
      }
      string sVersionKey = DNSKEY_HTTP_VERSION;
      string sVersion = DNSKEY_CURRENT_VERSION;
      oQuery.setPostData(sVersionKey, sVersion);

      oQuery.setPostData(sKey, sValue);
      if (!oQuery.query(sURL))
      {
        ps_elog(PSL_CRITICAL, "Unable to query URL: '%s'\n", sURL.c_str());
      }
      else
      {
        // Process results here.
        istringstream oISS(oQuery.getResponse());
        string sLine;

        fprintf(stderr, "Starting resp parse...\n");
        time_t tStart = time(NULL);

        string sVersion = "0.0";
        char cFront = oISS.peek();
        if (cFront == '|')
        {
          oISS.get();
          getline(oISS, sVersion);
        }

        while (getline(oISS, sLine))
        {
          // Create a local DnskeyProcDao and call fromString().  Then use the name below, and get the sig and payload to verif it.
          DnskeyProcDao oRemoteKeyDao;
          if (!oRemoteKeyDao.fromString(sLine))
          {
            ps_elog(PSL_ERROR, "The function DnskeyProcDao::fromString() failed on: '%s'\n", sLine.c_str());
          }
          else if (NULL != m_pCryptMgr
                   && !oRemoteKeyDao.verify(sVersion, *m_pCryptMgr))
          {
            ps_elog(PSL_CRITICAL, "Unable to verify signature on on data from remote URL '%s':\n'%s'\n!>\n'%s'\n",
                    sURL.c_str(),
                    oRemoteKeyDao.getSig().c_str(),
                    oRemoteKeyDao.getVerifiableData(sVersion).c_str());
          }
          else
          {
            ps_elog(PSL_DEBUG, "VERIFIED from '%s':\n'%s'\n=>\n'%s'\n",
                    sURL.c_str(),
                    oRemoteKeyDao.getSig().c_str(),
                    oRemoteKeyDao.getVerifiableData(sVersion).c_str());

            string sName = oRemoteKeyDao.getName();
            string sZoneUrl = sURL + "?" + PS_HTTP_ZONE_LIST_KEY + "=" + sName;
            KeyMapIter_t tZoneIter;

            if (sName.empty())
            {
              ps_elog(PSL_ERROR, "Found empty name, skipping.\n");
            }
            else if (m_oKeyMap.end() == (tZoneIter = m_oKeyMap.find(sName)))
            {
              ps_elog(PSL_CRITICAL, "Could not find zone: '%s' in internal map.\n", sName.c_str());
            }
            else if (!((DnskeyConsistencyDao *) tZoneIter->second)->addRemoteKey(sZoneUrl, sLine))
            {
              ps_elog(PSL_CRITICAL, "Unable to add zone <URL, line> = <'%s', '%s'>\n", sZoneUrl.c_str(), sLine.c_str());
            }
            else
            {
              ps_elog(PSL_DEBUG, "ADDED: '%s' for name '%s'\n", sZoneUrl.c_str(), ((DnskeyConsistencyDao *) tZoneIter->second)->getName().c_str());
            }
          }
        }
        time_t tStop = time(NULL);
        fprintf(stderr, "Took: %d\n", (int) (tStop - tStart));
//ps_elog(PSL_CRITICAL, "LAST LINE: '%s'\n", sLine.c_str());
      }
    }

    fprintf(stderr, "Starting consistency eval of %d elements...\n", (int) m_oKeyMap.size());

    int iConfirmedKeys = 0;
    int iConflictKeys = 0;
    int iProvisionalKeys = 0;
    int iUnknown = 0;
    for (KeyMapIter_t tCountIter = m_oKeyMap.begin();
         m_oKeyMap.end() != tCountIter;
         tCountIter++)
    {
      DnskeyConsistencyDao *pConDao = tCountIter->second;

      if (pConDao->getData() == "")
      {
        ps_elog(PSL_WARNING, "No data available for consistency DAO for name: '%s', perhaps keys were not fetchable (PMTU problems)?\n",
                pConDao->getName().c_str());
      }
      else if (!pConDao->process())
      {
        ps_elog(PSL_CRITICAL, "Unable to process for name: '%s'\n", tCountIter->first.c_str());
      }
      else if (!pConDao->serialize())
      {
        ps_elog(PSL_CRITICAL, "Unable to serialize consistency DAO.\n");
      }
      else if (pConDao->getResult() == PS_F_RULE_CONFIRMED)
      {
        iConfirmedKeys++;
//        ps_elog(PSL_CRITICAL, "Zone: '%s' has consistent keys\n", pConDao->getName().c_str());
      }
      else if (pConDao->getResult() == PS_F_RULE_CONFLICT)
      {
        iConflictKeys++;
//        ps_elog(PSL_CRITICAL, "Zone: '%s' has conflicting keys\n", pConDao->getName().c_str());
      }
      else if (pConDao->getResult() == PS_F_RULE_PROVISIONAL)
      {
        iProvisionalKeys++;
      }
      else if (pConDao->getResult() == PS_F_RULE_UNKNOWN)
      {
        iUnknown++;
//        ps_elog(PSL_CRITICAL, "Zone: '%s' has provisional keys\n", pConDao->getName().c_str());
      }
    }

    fprintf(stderr, "Found %d confirmed, %d conflict, %d provisional, and %d unknown keys\n", iConfirmedKeys, iConflictKeys, iProvisionalKeys, iUnknown);

    bRet = true;
  }

  return bRet;
}

UrlIter_t FriendRule::beginZones()
{
  return m_oNameList.begin();
}

UrlIter_t FriendRule::endZones()
{
  return m_oNameList.end();
}

size_t FriendRule::numZones()
{
  return m_oNameList.size();
}

