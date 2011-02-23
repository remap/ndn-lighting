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

#include "dnskey_mon_dao.h"
#include "ps_logger.h"
#include "ps_defs.h"


const char *DnskeyMonDao::s_kszDaoName = "DNSKEY_MON_DAO";

const char *DnskeyMonDao::s_szSelectList = "select ID from DNSKEY_URL_MON_LIST";
const char *DnskeyMonDao::s_szSelect = "select ID, SCRAPER_ID, SRC, NAME, FIRST_POLL, LAST_POLL, NEXT_POLL, SUB_SRC \
  from DNSKEY_URL_MON_LIST where ";
const char *DnskeyMonDao::s_szInsert = "insert into DNSKEY_URL_MON_LIST (SCRAPER_ID, SRC, NAME, FIRST_POLL, LAST_POLL, NEXT_POLL, SUB_SRC) \
  values (?, ?, ?, ?, ?, ?, ?)";
const char *DnskeyMonDao::s_szUpdate = "update DNSKEY_URL_MON_LIST set LAST_POLL = ?, NEXT_POLL = ? where ID = ?";
const char *DnskeyMonDao::s_szUpdateNextPoll = "update DNSKEY_URL_MON_LIST set NEXT_POLL = ? ";
const char *DnskeyMonDao::s_szDelete = "delete from DNSKEY_URL_MON_LIST where ID = ?";

using namespace std;

DnskeyMonDao::DnskeyMonDao()
  : m_bDelete(false),
    m_bUpdateNextPoll(false),
    m_iScraperID(-1),
    m_tFirstPoll(0),
    m_tLastPoll(0),
    m_tNextPoll(0),
    m_eSubSrc(DNSKEY_SRC_PRE_INIT),
    m_iStart(0),
    m_iCount(30)
{

}

DnskeyMonDao::~DnskeyMonDao()
{

}

bool DnskeyMonDao::getDeleteMode()
{
  return m_bDelete;
}

void DnskeyMonDao::setDeleteMode(bool p_bDelete)
{
  m_bDelete = p_bDelete;
}

int DnskeyMonDao::getScraperID()
{
  return m_iScraperID;
}

void DnskeyMonDao::setScraperID(int p_iScraperID)
{
  m_iScraperID = p_iScraperID;
}

time_t DnskeyMonDao::getFirstPoll()
{
  return m_tFirstPoll;
}

void DnskeyMonDao::setFirstPoll(time_t p_tPoll)
{
  m_tFirstPoll = p_tPoll;
}

time_t DnskeyMonDao::getLastPoll()
{
  return m_tLastPoll;
}

void DnskeyMonDao::setLastPoll(time_t p_tPoll)
{
  m_tLastPoll = p_tPoll;
}

time_t DnskeyMonDao::getNextPoll()
{
  return m_tNextPoll;
}

void DnskeyMonDao::setNextPoll(time_t p_tPoll)
{
  m_tNextPoll = p_tPoll;
}

dnskey_src_e DnskeyMonDao::getSubSrc()
{
  return m_eSubSrc;
}

void DnskeyMonDao::setSubSrc(dnskey_src_e p_eSubSrc)
{
  m_eSubSrc = p_eSubSrc;
}

std::string &DnskeyMonDao::getURL()
{
  return m_sURL;
}

void DnskeyMonDao::setURL(std::string &p_sURL)
{
  m_sURL = p_sURL;
}

int DnskeyMonDao::getStart()
{
  return m_iStart;
}

int DnskeyMonDao::getCount()
{
  return m_iCount;
}

void DnskeyMonDao::setRange(int p_iStart, int p_iCount)
{
  m_iStart = p_iStart;
  m_iCount = p_iCount;
}

std::string &DnskeyMonDao::getName()
{
  return m_sName;
}

void DnskeyMonDao::setName(std::string &p_sName)
{
  m_sName = p_sName;
}

void DnskeyMonDao::updateNextPoll()
{
  m_bUpdateNextPoll = true;
}

bool DnskeyMonDao::serialize()
{
  bool bRet = false;

  int iID = getID();
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB: '%s'\n", getError());
  }
  else if (-1 == iID)
  {
    if (m_bUpdateNextPoll)
    {
      string &sName = getName();
      string &sURL = getURL();
      string sSQL = s_szUpdateNextPoll;
      if (sName != "")
      {
        sSQL += " where NAME = ? ";
      }

      if (sURL != "")
      {
        if (sName != "")
        {
          sSQL += " and ";
        }
        else
        {
          sSQL += " where ";
        }
        sSQL += " SRC = ? ";
      }

      int iIdx = 0;
      if (!prepareQuery(sSQL))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(iIdx++, getNextPoll()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set poll parameter: '%s'\n", getError());
      }
      else if (sName != "" && !setStr(iIdx++, sName))
      {
        ps_elog(PSL_CRITICAL, "Unable to set name parameter: '%s'\n", getError());
      }
      else if (sURL != "" && !setStr(iIdx++, sURL))
      {
        ps_elog(PSL_CRITICAL, "Unable to set URL parameter: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update DB: '%s'\n", getError());
      }
      else
      {
        bRet = true;
        disconnect();
      }
    }
    else
    {
      if (getName() == "")
      {
        ps_elog(PSL_ERROR, "Unable to serialize monitoring target without a name.\n");
      }
      else if (getURL() == "")
      {
        ps_elog(PSL_ERROR, "Unable to serialize monitoring target without a URL.\n");
      }
      else if (!prepareQuery(s_szInsert))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", getError());
      }
      else if (!setInt(0, getScraperID()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set scraper ID parameter: '%s'\n", getError());
      }
      else if (!setStr(1, getURL()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set URL parameter: '%s'\n", getError());
      }
      else if (!setStr(2, getName()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set name parameter: '%s'\n", getError());
      }
      else if (!setInt(3, getFirstPoll()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set first poll parameter: '%s'\n", getError());
      }
      else if (!setInt(4, getLastPoll()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set last poll parameter: '%s'\n", getError());
      }
      else if (!setInt(5, getNextPoll()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set next poll parameter: '%s'\n", getError());
      }
      else if (!setInt(2, getSubSrc()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set submission src parameter: '%s'\n", getError());
      }
      else if (!update())
      {
        ps_elog(PSL_ERROR, "Unable to update DB: '%s'\n", getError());
      }
      else
      {
        disconnect();
        bRet = true;
      }
    }
  }
  else if (getDeleteMode())
  {
    if (!prepareQuery(s_szDelete))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", getError());
    }
    else if (!setInt(0, iID))
    {
      ps_elog(PSL_CRITICAL, "Unable to set ID parameter: '%s'\n", getError());
    }
    else if (!update())
    {
      ps_elog(PSL_ERROR, "Unable to update DB: '%s'\n", getError());
    }
    else
    {
      bRet = true;
    }
  }
  else
  {
    if (!prepareQuery(s_szUpdate))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", getError());
    }
    else if (!setInt(0, getLastPoll()))
    {
      ps_elog(PSL_CRITICAL, "Unable to set last poll parameter: '%s'\n", getError());
    }
    else if (!setInt(1, getNextPoll()))
    {
      ps_elog(PSL_CRITICAL, "Unable to set next parameter: '%s'\n", getError());
    }
    else if (!setInt(2, iID))
    {
      ps_elog(PSL_CRITICAL, "Unable to set ID parameter: '%s'\n", getError());
    }
    else if (!update())
    {
      ps_elog(PSL_ERROR, "Unable to update DB: '%s'\n", getError());
    }
    else
    {
      disconnect();
      bRet = true;
    }
  }

  return bRet;
}

bool DnskeyMonDao::deserialize()
{
  bool bRet = false;

  int iID = getID();
  string &sURL = getURL();
  if (-1 == iID && sURL == "")
  {
    ps_elog(PSL_ERROR, "Unable to deserialize without an ID or a URL.\n");
  }
  else if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB: '%s'\n", getError());
  }
  else
  {
    string sSQL = s_szSelect;
    if (-1 != iID)
    {
      sSQL += " ID = ? ";
    }

    if (sURL != "")
    {
       if (-1 != iID)
       {
         sSQL += " and ";
       }
       sSQL += " SRC = ?";
    }

    int iIdx = 0;
    if (!prepareQuery(sSQL))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", getError());
    }
    else if (-1 != iID && !setInt(iIdx++, iID))
    {
      ps_elog(PSL_CRITICAL, "Unable to set ID parameter: '%s'\n", getError());
    }
    else if (sURL != "" && !setStr(iIdx++, sURL))
    {
      ps_elog(PSL_CRITICAL, "Unable to set URL parameter: '%s'\n", getError());
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to exec query: '%s'\n", getError());
    }
    else if (!next())
    {
      ps_elog(PSL_ERROR, "No data found for query for ID and URL ( %d, '%s' )\n", iID, sURL.c_str());
    }
    else
    {
      iID = getInt(0);
      int iScraperID = getInt(1);
      getStr(2, sURL);
      string sName;
      getStr(3, sName);
      time_t tFirst = getInt(4);
      time_t tLast = getInt(5);
      time_t tNext = getInt(6);
      dnskey_src_e eSrc = (dnskey_src_e) getInt(7);

      if (next())
      {
        ps_elog(PSL_ERROR, "Too many rows returned for query for ID and URL ( %d, '%s' )\n", iID, sURL.c_str());
      }
      else
      {
        setID(iID);
        setScraperID(iScraperID);
        setURL(sURL);
        setName(sName);
        setFirstPoll(tFirst);
        setLastPoll(tLast);
        setNextPoll(tNext);
        setSubSrc(eSrc);
        bRet = true;
      }
    }
  }
  disconnect();

  return bRet;
}

bool DnskeyMonDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  clearList(p_oOutputList);

  int iStart = getStart();
  int iCount = getCount();

  string sSQL = s_szSelectList;
  if (iStart >= 0 && iCount > 0)
  {
    sSQL += " where ID > ? limit ?";
  }

  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect to DB: '%s'\n", getError());
  }
  else if (!prepareQuery(sSQL))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", getError());
  }
  else if (iStart >= 0 && iCount > 0 && !setInt(0, iStart))
  {
    ps_elog(PSL_CRITICAL, "Unable to set start ID parameter: '%s'\n", getError());
  }
  else if (iStart >= 0 && iCount > 0 && ! setInt(1, iCount))
  {
    ps_elog(PSL_CRITICAL, "Unable to set count parameter: '%s'\n", getError());
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec query: '%s'\n", getError());
  }
  else
  {
    bRet = true;
    while (next())
    {
      DnskeyMonDao *pDao = dup();
      pDao->setID(getInt(0));
      p_oOutputList.push_back(pDao);
    }
  }
  disconnect();

  for (DaoIter_t tIter = p_oOutputList.begin();
       bRet
       && p_oOutputList.end() != tIter;
       tIter++)
  {
    bRet &= (*tIter)->deserialize();
  }

  if (!bRet)
  {
    clearList(p_oOutputList);
  }

  return bRet;
}

DnskeyMonDao *DnskeyMonDao::dup()
{
  return new DnskeyMonDao();
}

std::string DnskeyMonDao::daoName()
{
  return string(s_kszDaoName);
}

