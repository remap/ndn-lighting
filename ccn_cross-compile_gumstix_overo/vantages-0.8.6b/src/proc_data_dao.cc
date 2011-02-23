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

#include <iostream>
#include <sstream>

#include "proc_data_dao.h"
#include "ps_defs.h"
#include "raw_data_dao.h"
#include "scraper_dao.h"
#include "dao_factory.h"
#include "proc_query_dao.h"

using namespace std;

const char *ProcessedDataDao::s_kszDaoName = "PROCESSED_DATA_DAO";

const char *ProcessedDataDao::s_szSelect = "select ID, SCRAPER_ID, PROC_TYPE, NAME, FIRST_SEEN, LAST_SEEN, SIG, PROC_DATA, SCR_PARAM_ID \
  from PS_PROC_DATA \
  where NS_ID = ? ";

const char *ProcessedDataDao::s_szSelectRawIDs = "select RAW_ID from PS_PROC_RAW_REL where PROC_ID = ?";
const char *ProcessedDataDao::s_szSelectProcIDs = "select PRE_ID from PS_PROC_SELF_REL where POST_ID = ?";

const char *ProcessedDataDao::s_szInsert = "insert into PS_PROC_DATA \
  (SCRAPER_ID, NS_ID, PROC_TYPE, NAME, FIRST_SEEN, LAST_SEEN, SIG, PROC_DATA, SCR_PARAM_ID) \
  values (?, ?, ?, ?, ?, ?, ?, ?, ?)";

const char *ProcessedDataDao::s_szInsertRawID = "insert into PS_PROC_RAW_REL \
  (PROC_ID, RAW_ID) \
  values (?, ?)";

const char *ProcessedDataDao::s_szInsertProcID = "insert into PS_PROC_SELF_REL \
  (PRE_ID, POST_ID) \
  values (?, ?)";

const char *ProcessedDataDao::s_szUpdate = "update PS_PROC_DATA \
  set LAST_SEEN = ?, SIG = ? \
  where ID = ?";

ProcessedDataDao::ProcessedDataDao()
  : m_iScraperID(-1),
    m_iScraperParamID(-1),
    m_tDate(0),
    m_pScraper(NULL)
{

}

ProcessedDataDao::~ProcessedDataDao()
{
  clearRawDaos();
  clearProcDaos();
  setScraper(NULL);
}

IdIter_t ProcessedDataDao::beginRawIDs()
{
  return m_oRawIDList.begin();
}

IdIter_t ProcessedDataDao::endRawIDs()
{
  return m_oRawIDList.end();
}

int ProcessedDataDao::numRawIDs()
{
  return m_oRawIDList.size();
}

void ProcessedDataDao::addRawID(int p_iRawID)
{
  m_oRawIDList.push_back(p_iRawID);
}

RawDaoIter_t ProcessedDataDao::beginRawDaos()
{
  return m_oRawDaoList.begin();
}

RawDaoIter_t ProcessedDataDao::endRawDaos()
{
  return m_oRawDaoList.end();
}

int ProcessedDataDao::numRawDaos()
{
  return m_oRawDaoList.size();
}

void ProcessedDataDao::addRawDao(RawDataDao *p_pDao)
{
  if (NULL == p_pDao)
  {
    ps_elog(PSL_CRITICAL, "Cannot add NULL DAO.\n");
  }
  else
  {
    int iRawID = p_pDao->getID();
    bool bFound = false;
    for (IdIter_t tIter = beginRawIDs();
         endRawIDs() != tIter;
         tIter++)
    {
      int iTmpID = *tIter;
      if (iTmpID == iRawID)
      {
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      addRawID(iRawID);
    }

    m_oRawDaoList.push_back(p_pDao);
  }
}






IdIter_t ProcessedDataDao::beginProcIDs()
{
  return m_oProcIDList.begin();
}

IdIter_t ProcessedDataDao::endProcIDs()
{
  return m_oProcIDList.end();
}

int ProcessedDataDao::numProcIDs()
{
  return m_oProcIDList.size();
}

void ProcessedDataDao::addProcID(int p_iProcID)
{
  m_oProcIDList.push_back(p_iProcID);
}

ProcDaoIter_t ProcessedDataDao::beginProcDaos()
{
  return m_oProcDaoList.begin();
}

ProcDaoIter_t ProcessedDataDao::endProcDaos()
{
  return m_oProcDaoList.end();
}

int ProcessedDataDao::numProcDaos()
{
  return m_oProcDaoList.size();
}

void ProcessedDataDao::addProcDao(ProcessedDataDao *p_pDao)
{
  if (NULL == p_pDao)
  {
    ps_elog(PSL_CRITICAL, "Cannot add NULL DAO.\n");
  }
  else
  {
    int iProcID = p_pDao->getID();
    bool bFound = false;
    for (IdIter_t tIter = beginProcIDs();
         endProcIDs() != tIter;
         tIter++)
    {
      int iTmpID = *tIter;
      if (iTmpID == iProcID)
      {
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      addProcID(iProcID);
    }

//ps_elog(PSL_CRITICAL, "ADDING '%s' to '%s'\n", p_pDao->getName().c_str(), getName().c_str());
    m_oProcDaoList.push_back(p_pDao);
  }
}

void ProcessedDataDao::clearRawDaos()
{
  for (RawDaoIter_t tIter = beginRawDaos();
       endRawDaos() != tIter;
       tIter++)
  {
    if (NULL == *tIter)
    {
      ps_elog(PSL_CRITICAL, "Found NULL Raw DAO in list?\n");
    }
    else
    {
      delete *tIter;
    }
  }

  m_oRawDaoList.clear();
}
void ProcessedDataDao::clearProcDaos()
{
  for (ProcDaoIter_t tIter = beginProcDaos();
       endProcDaos() != tIter;
       tIter++)
  {
    if (NULL == *tIter)
    {
      ps_elog(PSL_CRITICAL, "Found NULL Proc DAO in list?\n");
    }
    else
    {
      delete *tIter;
    }
  }

  m_oProcDaoList.clear();
}

int ProcessedDataDao::getScraperParamID()
{
  if (NULL != m_pScraper)
  {
//ps_elog(PSL_CRITICAL, "Was: %d will be: %d\n", m_iScraperID, m_pScraper->getID());
    m_iScraperParamID = m_pScraper->getParamID();
  }

  return m_iScraperParamID;
}

void ProcessedDataDao::setScraperParamID(int p_iScraperParamID)
{
//ps_elog(PSL_CRITICAL, "SETTING TO: %d from %d\n", p_iScraperID, m_iScraperID);
  m_iScraperParamID = p_iScraperParamID;
  if (NULL != m_pScraper && m_pScraper->getParamID() != m_iScraperParamID)
  {
    setScraper(NULL);
  }
}


int ProcessedDataDao::getScraperID()
{
  if (NULL != m_pScraper)
  {
//ps_elog(PSL_CRITICAL, "Was: %d will be: %d\n", m_iScraperID, m_pScraper->getID());
    m_iScraperID = m_pScraper->getID();
  }

  return m_iScraperID;
}

void ProcessedDataDao::setScraperID(int p_iScraperID)
{
//ps_elog(PSL_CRITICAL, "SETTING TO: %d from %d\n", p_iScraperID, m_iScraperID);
  m_iScraperID = p_iScraperID;
  if (NULL != m_pScraper && m_pScraper->getID() != m_iScraperID)
  {
    setScraper(NULL);
  }
}

ScraperDao *ProcessedDataDao::getScraper()
{
  return m_pScraper;
}

void ProcessedDataDao::setScraper(ScraperDao *p_pDao)
{
  if (NULL != m_pScraper)
  {
    delete m_pScraper;
    m_pScraper = NULL;
  }

  m_pScraper = p_pDao;
  if (NULL != m_pScraper)
  {
    setScraperID(m_pScraper->getID());
  }
}

std::string &ProcessedDataDao::getName()
{
  return m_sName;
}

void ProcessedDataDao::setName(std::string &p_sName)
{
  m_sName = p_sName;
}

time_t ProcessedDataDao::getDate()
{
  return m_tDate;
}

void ProcessedDataDao::setDate(time_t p_tDate)
{
  m_tDate = p_tDate;
}

std::string &ProcessedDataDao::getData()
{
  return m_sData;
}

void ProcessedDataDao::setData(std::string &p_sData)
{
  m_sData = p_sData;
}

bool ProcessedDataDao::serialize()
{
  bool bRet = false;

  int iID = getID();
  int iScraperID = getScraperID();
  int iScraperParamID = getScraperParamID();
  string &sData = getData();
  size_t uLen = sData.length();
  string &sName = getName();
//  ProcessedDataDao *pQueryDao = (ProcessedDataDao *) DaoFactory::getInstance().create(ProcQueryDao::s_kszDaoName);
  ProcessedDataDao *pQueryDao = (ProcessedDataDao *) dup();

  if (iScraperID <= 0)
  {
    ps_elog(PSL_ERROR, "Unable to serialize with no scraper ID: %d\n", iScraperID);
  }
  else if (uLen == 0)
  {
    ps_elog(PSL_ERROR, "Unable to serialize with no data (len = %u)\n", (unsigned) uLen);
  }
  else if (sName.length() <= 0)
  {
    ps_elog(PSL_ERROR, "Can't serialize with empty name.\n");
  }
  else if (NULL == pQueryDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to create query DAO.\n");
  }
  else
  {
    pQueryDao->setID(iID);
    pQueryDao->setScraperID(iScraperID);
    pQueryDao->setScraperParamID(iScraperParamID);
//    pQueryDao->setScraper(getScraper());
    pQueryDao->setScraper(NULL);
    pQueryDao->setName(sName);
    DaoList_t tDaoList;
    if (!pQueryDao->deserialize(tDaoList))
    {
      ps_elog(PSL_ERROR, "Unable to deserialize DAO.\n");
    }
    else
    {
      bool bInsert = true;
      for (DaoIter_t tIter = tDaoList.begin();
           tDaoList.end() != tIter;
           tIter++)
      {
        ProcessedDataDao *pLoadedDao = dynamic_cast<ProcessedDataDao *>(*tIter);
        if (NULL == pLoadedDao)
        {
          ps_elog(PSL_CRITICAL, "Loaded DAO is NULL?\n");
        }
        else
        {
          string &sLoadedName = pLoadedDao->getName();
          string sLoadedDaoName = pLoadedDao->daoName();
          string &sLoadedData = pLoadedDao->getData();
          size_t uLoadedLen = sLoadedData.length();
//ps_elog(PSL_CRITICAL, "GOT BACK CANDIDATE WITH length: %u vs %u and values:\n%s\n\tvs\n%s\n", (unsigned) uLen, (unsigned) uLoadedLen, sData.c_str(), sLoadedData.c_str());
//ps_elog(PSL_CRITICAL, "GOT BACK CANDIDATE WITH name '%s' scraper id %d and length: %u vs %u\n", sName.c_str(), iScraperID, (unsigned) uLen, (unsigned) uLoadedLen);
          if (bInsert
              && uLoadedLen == uLen
              && sData == sLoadedData
              && sName == sLoadedName
              && daoName() == sLoadedDaoName)
          {
            bInsert = false;
            setID(pLoadedDao->getID());
            if (!connect(__FILE__, __LINE__))
            {
              ps_elog(PSL_CRITICAL, "Unable to connect to DB.");
            }
            else if (!prepareQuery(s_szUpdate))
            {
              ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_szUpdate);
            }
            else if (!setInt(0, getDate()))
            {
              ps_elog(PSL_CRITICAL, "Unable to set date: %d\n", (int) getDate());
            }
            else if (!setStr(1, getSig()))
            {
              ps_elog(PSL_CRITICAL, "Unable to set signature: '%s'\n", getSig().c_str());
            }
            else if (!setInt(2, getID()))
            {
              ps_elog(PSL_CRITICAL, "Unable to set ID: %d\n", iID);
            }
            else if (!update())
            {
              ps_elog(PSL_ERROR, "Unable to update.\n");
            }
            else
            {
              bRet = true;
            }
            disconnect();
          }

          delete pLoadedDao;
        }
      }

      if (bInsert)
      {
        int iNamespace = getNsID();
        string sDaoName = daoName();
        string sSig = getSig();
        if (-1 == iNamespace)
        {
          ps_elog(PSL_CRITICAL, "Unable to insert with namespace %d\n", iNamespace);
        }
        else if (!connect(__FILE__, __LINE__))
        {
          ps_elog(PSL_CRITICAL, "Unable to connect to DB.\n");
        }
        else if (!prepareQuery(s_szInsert))
        {
          ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", s_szInsert);
        }
        else if (!setInt(0, iScraperID))
        {
          ps_elog(PSL_CRITICAL, "Unable to set scraper ID: %d\n", iScraperID);
        }
        else if (!setInt(1, iNamespace))
        {
          ps_elog(PSL_CRITICAL, "Unable to set namespace ID: %d\n", iNamespace);
        }
        // This is where type == dao type name
        else if (!setStr(2, sDaoName))
        {
          ps_elog(PSL_CRITICAL, "Unable to set dao type to '%s'\n", sDaoName.c_str());
        }
        else if (!setStr(3, sName))
        {
          ps_elog(PSL_CRITICAL, "Unable to set name: %s\n", sName.c_str());
        }
        else if (!setInt(4, getDate()))
        {
          ps_elog(PSL_CRITICAL, "Unable to set FirstSeen: %d\n", (int) getDate());
        }
        else if (!setInt(5, getDate()))
        {
          ps_elog(PSL_CRITICAL, "Unable to set LastSeen: %d\n", (int) getDate());
        }
        else if (!setStr(6, sSig))
        {
          ps_elog(PSL_CRITICAL, "Unable to set sig: %s\n", sSig.c_str());
        }
        else if (!setStr(7, sData))
        {
          ps_elog(PSL_CRITICAL, "Unable to set data: %s\n", sData.c_str());
        }
        else if (-1 == iScraperParamID && !setNULL(8))
        {
          ps_elog(PSL_CRITICAL, "Unable to set NULL scraper ID: %s\n", getError());
        }
        else if (-1 != iScraperParamID && !setInt(8, iScraperParamID))
        {
          ps_elog(PSL_CRITICAL, "Unable to set scraper ID: %s\n", getError());
        }
        else if (!update())
        {
          ps_elog(PSL_CRITICAL, "Unable to insert '%s' -> (%d, %d, '%s', '%s', %d, %d, '%s'): %s\n", 
                 s_szInsert,
                 iScraperID,
                 iNamespace,
                 sDaoName.c_str(),
                 sName.c_str(),
                 (int) getDate(),
                 (int) getDate(),
                 sData.c_str(),
                 getError());
        }
        else
        {
          setID(getLastID());

          bRet = true;
          for (RawDaoIter_t tRawIter = beginRawDaos();
               bRet && endRawDaos() != tRawIter;
               tRawIter++)
          {
            bRet = false;
            int iRawID = (*tRawIter)->getID();
            if (-1 == iRawID)
            {
              ps_elog(PSL_CRITICAL, "Unserialized dao in list...\n");
            }
            else
            {
            disconnect();
            if (!connect(__FILE__, __LINE__))
            {
              ps_elog(PSL_CRITICAL, "Unable to re-connect()\n");
            }
            else if (!prepareQuery(s_szInsertRawID))
            {
              ps_elog(PSL_CRITICAL, "Unable to insert raw ID: %d for Proc DAO: %d\n", iRawID, getID());
            }
            else if (!setInt(0, getID()))
            {
              ps_elog(PSL_CRITICAL, "Unable to set PROC_ID: %d\n", getID());
            }
            else if (!setInt(1, iRawID))
            {
              ps_elog(PSL_CRITICAL, "Unable to set raw ID: %d for proc ID: %d\n", iRawID, getID());
            }
            else if (!update())
            {
              ps_elog(PSL_CRITICAL, "Unable to update for proc ID: %d\n", getID());
            }
            else
            {
              bRet = true;
            }
            }
          }

          for (ProcDaoIter_t tProcIter = beginProcDaos();
               bRet && endProcDaos() != tProcIter;
               tProcIter++)
          {
            bRet = false;
            int iProcID = (*tProcIter)->getID();
            if (-1 == iProcID)
            {
              ps_elog(PSL_CRITICAL, "Unserialized dao in list...\n");
            }
            else
            {
            disconnect();
            if (!connect(__FILE__, __LINE__))
            {
              ps_elog(PSL_CRITICAL, "Unable to re-connect()\n");
            }
            else if (!prepareQuery(s_szInsertProcID))
            {
              ps_elog(PSL_CRITICAL, "Unable to insert raw ID: %d for Proc DAO: %d\n", iProcID, getID());
            }
            else if (!setInt(0, iProcID))
            {
              ps_elog(PSL_CRITICAL, "Unable to set raw ID: %d for proc ID: %d\n", iProcID, getID());
            }
            else if (!setInt(1, getID()))
            {
              ps_elog(PSL_CRITICAL, "Unable to set PROC_ID: %d\n", getID());
            }
            else if (!update())
            {
              ps_elog(PSL_CRITICAL, "Unable to update for proc ID: %d\n", getID());
            }
            else
            {
              bRet = true;
            }
            }
          }
        }

        disconnect();
      }
    }
  }

  if (NULL != pQueryDao)
  {
    delete pQueryDao;
    pQueryDao = NULL;
  }

  return bRet;
}

bool ProcessedDataDao::deserialize()
{
  bool bRet = false;

  int iID = getID();
  if (-1 == iID)
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize w/o ID.\n");
  }
  else
  {
    string sSQL = s_szSelect;
    sSQL += " and ID = ?";

    int iNamespace = getNsID();
    if (-1 == iNamespace)
    {
      ps_elog(PSL_CRITICAL, "Unable to query with no NS ID.\n");
    }
    else if (!connect(__FILE__, __LINE__))
    {
      ps_elog(PSL_CRITICAL, "Unable to connect to DB\n");
    }
    else if (!prepareQuery(sSQL))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare: '%s'\n", sSQL.c_str());
    }
    else if (!setInt(0, iNamespace))
    {
      ps_elog(PSL_CRITICAL, "Unable to set namespace ID: %d\n", iNamespace);
    }
    else if (!setInt(1, iID))
    {
      ps_elog(PSL_CRITICAL, "Unable to set ID: %d\n", iID);
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to execute query.\n");
    }
    else if (!next())
    {
      ps_elog(PSL_CRITICAL, "Unable to find data for ID %d in NS %d\n", iID, iNamespace);
    }
    else
    {
      int iScraperID = getInt(1);
      string sProcType;
      getStr(2, sProcType);
      string sName;
      getStr(3, sName);
      time_t tLast = getInt(5);
      string sSig;
      getStr(6, sSig);
      string sData;
      getStr(7, sData);
      int iScraperParamID = -1;
      if (!isNULL(8))
      {
        iScraperParamID = getInt(8);
      }

      setScraperID(iScraperID);
      setScraperParamID(iScraperParamID);
      setNsID(iNamespace);
      setName(sName);
      setDate(tLast);
      setSig(sSig);
      setData(sData);

      if (next())
      {
        ps_elog(PSL_CRITICAL, "Too many matches.\n");
      }
      else
      {
        loadRawIDs();
        loadProcIDs();
        bRet = true;
      }
    }

    disconnect();
  }

  return bRet;
}

bool ProcessedDataDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  int iID = getID();
  int iScraperID = getScraperID();
  string &sName = getName();

  ostringstream oSS;
  oSS << s_szSelect;
  if (iID > 0)
  {
    oSS << " and ID = " << iID;
  }
  else
  {
    oSS << " and PROC_TYPE = '" << daoName() << "' ";

    if (sName.length() > 0)
    {
      oSS << " and ";
      oSS << " NAME = '" << sName << "'";
    }

    if (iScraperID > 0)
    {
      oSS << " and ";
      oSS << " SCRAPER_ID = " << iScraperID;
    }

    time_t tDate = getDate();
    if (0 != tDate)
    {
      oSS << " and LAST_SEEN >= " 
          << (int) tDate
          << " order by LAST_SEEN desc";
    }
  }

  int iNamespace = getNsID();
  string sSQL = oSS.str();
  if (-1 == iNamespace)
  {
    ps_elog(PSL_CRITICAL, "Unable to query with no NS ID.\n");
  }
  else if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB\n");
  }
  else if (!prepareQuery(sSQL))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare: '%s'\n", oSS.str().c_str());
  }
  else if (!setInt(0, iNamespace))
  {
    ps_elog(PSL_CRITICAL, "Unable to set namespace ID: %d\n", iNamespace);
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to execute query.\n");
  }
  else
  {
    bRet = true;
    while (bRet && next())
    {
      int iID = getInt(0);
      int iScraperID = getInt(1);
      string sProcType;
      getStr(2, sProcType);
      string sName;
      getStr(3, sName);
      time_t tLast = getInt(5);
      string sSig;
      getStr(6, sSig);
      string sData;
      getStr(7, sData);
      int iScraperParamID = -1;
      if (!isNULL(8))
      {
        iScraperParamID = getInt(8);
      }


      ProcessedDataDao *pDao = dynamic_cast<ProcessedDataDao *>(DaoFactory::getInstance().create(sProcType));
      if (NULL == pDao)
      {
        ps_elog(PSL_CRITICAL, "Unable to load DAO of type '%s'\n", sProcType.c_str());
        bRet = false;
      }
      else
      {
        pDao->setID(iID);
        pDao->setScraperID(iScraperID);
        pDao->setScraperParamID(iScraperParamID);
        pDao->setScraper(NULL);
        pDao->setNsID(iNamespace);
        pDao->setName(sName);
        pDao->setDate(tLast);
        pDao->setSig(sSig);
        pDao->setData(sData);
        pDao->loadRawIDs();
        pDao->loadProcIDs();
        p_oOutputList.push_back(pDao);
      }
    }
  }
  disconnect();

  if (!bRet)
  {
    clearList(p_oOutputList);
  }

  return bRet;
}

bool ProcessedDataDao::loadScraperDao()
{
  bool bRet = false;

  try
  {
    int iScraperID = getScraperID();
    if (iScraperID <= 0)
    {
      ps_elog(PSL_CRITICAL, "Unable to process with scraper ID = %d\n", iScraperID);
    }
    else
    {
      ScraperDao *pScraperDao = dynamic_cast<ScraperDao *>(DaoFactory::getInstance().create(ScraperDao::s_kszDaoName));
      if (NULL == pScraperDao)
      {
        ps_elog(PSL_CRITICAL, "Unable to create scraper DAO: %s\n", ScraperDao::s_kszDaoName);
      }
      else
      {
        pScraperDao->setID(iScraperID);
        pScraperDao->setParamID(getScraperParamID());
        if (!pScraperDao->deserialize())
        {
          ps_elog(PSL_CRITICAL, "Unable to deserialize scraper dao.\n");
          delete pScraperDao;
          pScraperDao = NULL;
        }
        else
        {
          setScraper(pScraperDao);
          bRet = true;
        }
      }
    }
  }
  catch (...)
  {
    ps_elog(PSL_CRITICAL, "Caught Exception.");
    bRet = false;
  }

  return bRet;
}

bool ProcessedDataDao::loadRawDaos()
{
  bool bRet = false;

  try
  {
    for (IdIter_t tIdIter = beginRawIDs();
         endRawIDs() != tIdIter;
         tIdIter++)
    {
      int iRawID = *tIdIter;
      if (iRawID <= 0)
      {
        ps_elog(PSL_CRITICAL, "Unable to process with raw ID = %d\n", iRawID);
      }
      else
      {
        RawDataDao *pRawDao = dynamic_cast<RawDataDao *>(DaoFactory::getInstance().create(RawDataDao::s_kszDaoName));
        if (NULL == pRawDao)
        {
          ps_elog(PSL_CRITICAL, "Unable to create raw DAO: %s\n", RawDataDao::s_kszDaoName);
        }
        else
        {
          pRawDao->setID(iRawID);

          // For easy cleanup
          addRawDao(pRawDao);
          DaoList_t tRawList;
          if (!pRawDao->deserialize(tRawList))
          {
            ps_elog(PSL_CRITICAL, "Unable to deserialize raw dao.\n");
          }
          else if (tRawList.size() != 1)
          {
            ps_elog(PSL_CRITICAL, "Raw list has %d entries instead of 1\n", (int) tRawList.size());
          }
          else
          {
            pRawDao = dynamic_cast<RawDataDao *>(tRawList.front());
            addRawDao(pRawDao);
            tRawList.pop_front();
            bRet = true;
          }

          DaoIter_t tIter;
          for (tIter = tRawList.begin();
               tRawList.end() != tIter;
               tIter++)
          {
            delete *tIter;
          }
        }
      }
    }
  }
  catch (...)
  {
    ps_elog(PSL_CRITICAL, "Caught Exception.");
    bRet = false;
  }

  return bRet;
}

bool ProcessedDataDao::loadProcDaos()
{
  bool bRet = false;

  try
  {
    for (IdIter_t tIdIter = beginProcIDs();
         endProcIDs() != tIdIter;
         tIdIter++)
    {
      int iProcID = *tIdIter;
      if (iProcID <= 0)
      {
        ps_elog(PSL_CRITICAL, "Unable to process with raw ID = %d\n", iProcID);
      }
      else
      {
        ProcessedDataDao *pProcDao = dynamic_cast<ProcessedDataDao *>(DaoFactory::getInstance().create(ProcessedDataDao::s_kszDaoName));
        if (NULL == pProcDao)
        {
          ps_elog(PSL_CRITICAL, "Unable to create raw DAO: %s\n", ProcessedDataDao::s_kszDaoName);
        }
        else
        {
          pProcDao->setID(iProcID);

          // For easy cleanup
          addProcDao(pProcDao);
          DaoList_t tProcList;
          if (!pProcDao->deserialize(tProcList))
          {
            ps_elog(PSL_CRITICAL, "Unable to deserialize raw dao.\n");
          }
          else if (tProcList.size() != 1)
          {
            ps_elog(PSL_CRITICAL, "Proc list has %d entries instead of 1\n", (int) tProcList.size());
          }
          else
          {
            pProcDao = dynamic_cast<ProcessedDataDao *>(tProcList.front());
            addProcDao(pProcDao);
            tProcList.pop_front();
            bRet = true;
          }

          DaoIter_t tIter;
          for (tIter = tProcList.begin();
               tProcList.end() != tIter;
               tIter++)
          {
            delete *tIter;
          }
        }
      }
    }
  }
  catch (...)
  {
    ps_elog(PSL_CRITICAL, "Caught Exception.");
    bRet = false;
  }

  return bRet;
}

/*
std::string ProcessedDataDao::daoName()
{
  return string(s_kszDaoName);
}
*/

/*
IPsDupable *ProcessedDataDao::dup()
{
  return new ProcessedDataDao();
}
*/

bool ProcessedDataDao::loadRawIDs()
{
  bool bRet = false;

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB.\n");
  }
  else if (!prepareQuery(s_szSelectRawIDs))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare SQL: %s\n", s_szSelectRawIDs);
  }
  else if (!setInt(0, getID()))
  {
    ps_elog(PSL_CRITICAL, "Unable to set ID parameter: %d\n", getID());
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec().\n");
  }
  else
  {
    while (next())
    {
      addRawID(getInt(0));
    }
    bRet = true;
  }
  disconnect();

  return bRet;
}

bool ProcessedDataDao::loadProcIDs()
{
  bool bRet = false;

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB.\n");
  }
  else if (!prepareQuery(s_szSelectProcIDs))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare SQL: %s\n", s_szSelectProcIDs);
  }
  else if (!setInt(0, getID()))
  {
    ps_elog(PSL_CRITICAL, "Unable to set ID parameter: %d\n", getID());
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec().\n");
  }
  else
  {
    while (next())
    {
      addProcID(getInt(0));
    }
    bRet = true;
  }
  disconnect();

  return bRet;
}
