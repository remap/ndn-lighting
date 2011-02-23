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
#include <string.h>

#include <string>

#include "scraper_dao.h"
#include "dao_factory.h"
#include "ps_logger.h"

const char *ScraperDao::s_kszDaoName = "SCRAPER_BASE";
const char *ScraperDao::s_szSelectSQL = "select s.ID, r.NAME, s.SCRAPER, s.TYPE_ID \
  from PS_SCRAPER s, PS_SCRAPER_TYPE_REF r \
  where s.ID = ? and s.TYPE_ID = r.ID";
const char *ScraperDao::s_szSelectParamSQL = "select VALUE from PS_SCRAPER_PARAM where ID = ?";

const char *ScraperDao::s_szInsertSQL = "insert into PS_SCRAPER (TYPE_ID, SCRAPER) \
  values (?, ?)";
const char *ScraperDao::s_szInsertParamSQL = "insert into PS_SCRAPER_PARAM (VALUE) values (?)";

const char *ScraperDao::s_szUpdateSQL = "update PS_SCRAPER set SCRAPER = ? where ID = ?";

using namespace std;

ScraperDao::ScraperDao()
  : m_pScraperBin(NULL),
    m_uBinLen(0),
    m_pData(NULL),
    m_uLen(0),
    m_iType(-1),
    m_iParamID(-1)
{

}

ScraperDao::~ScraperDao()
{
  if (NULL != m_pData)
  {
    delete[] m_pData;
    m_pData = NULL;
  }

  if (NULL != m_pScraperBin)
  {
    delete[] m_pScraperBin;
    m_pScraperBin = NULL;
  }
}

char *ScraperDao::getScraperBin()
{
  return m_pScraperBin;
}

size_t ScraperDao::getBinLen()
{
  return m_uBinLen;
}

bool ScraperDao::setScraperBin(char *p_pScraperBin, size_t p_uBinLen)
{
  if (NULL != m_pScraperBin)
  {
    delete[] m_pScraperBin;
    m_pScraperBin = NULL;
    m_uBinLen = 0;
  }

  if (NULL != p_pScraperBin && p_uBinLen > 0)
  {
    m_uBinLen = p_uBinLen;
    m_pScraperBin = new char[m_uBinLen + 1];
    memset(m_pScraperBin, 0, m_uBinLen + 1);
    memcpy(m_pScraperBin, p_pScraperBin, m_uBinLen);
  }

  return true;
}

int ScraperDao::getParamID()
{
  return m_iParamID;
}

void ScraperDao::setParamID(int p_iParamID)
{
  m_iParamID = p_iParamID;
}

std::string &ScraperDao::getParam()
{
  return m_sParam;
}

void ScraperDao::setParam(std::string &p_sParam)
{
  m_sParam = p_sParam;
}

char *ScraperDao::getData()
{
  return m_pData;
}

size_t ScraperDao::getDataLen()
{
  return m_uLen;
}

bool ScraperDao::setData(char *p_pData, size_t p_uLen)
{
  if (NULL != m_pData)
  {
    delete[] m_pData;
    m_pData = NULL;
    p_uLen = 0;
  }

  if (NULL != p_pData && 0 != p_uLen)
  {
    m_uLen = p_uLen;
    m_pData = new char[m_uLen];
    memcpy(m_pData, p_pData, m_uLen);
  }

  return true;
}

std::string &ScraperDao::getResult()
{
  return m_sResult;
}

void ScraperDao::setResult(std::string &p_sResult)
{
  m_sResult = p_sResult;
}

bool ScraperDao::execute()
{
  ps_elog(PSL_CRITICAL, "Base class cannot execute.\n");
  return false;
}

int ScraperDao::getType()
{
  return m_iType;
}

void ScraperDao::setType(int p_iType)
{
  m_iType = p_iType;
}

bool ScraperDao::serialize()
{
  bool bRet = false;

  int iID = getID();
  if (-1 != iID)
  {
    if (NULL == getScraperBin())
    {
      ps_elog(PSL_CRITICAL, "Unable to serialize scraper DAO w/ NULL bin.\n");
    }
    else if (!connect(__FILE__, __LINE__))
    {
      ps_elog(PSL_CRITICAL, "Unable to connect().\n");
    }
    else if (!prepareQuery(s_szUpdateSQL))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepareSQL(): '%s'\n", s_szUpdateSQL);
    }
    else if (!setInt(0, iID))
    {
      ps_elog(PSL_CRITICAL, "Unable to set ID.\n");
    }
    else if (!setBlob(1, getScraperBin(), getBinLen()))
    {
      ps_elog(PSL_CRITICAL, "Unable to set scraper bin.\n");
    }
    else if (!update())
    {
      ps_elog(PSL_CRITICAL, "Unable to update.\n");
    }
    else
    {
      bRet = true;
    }
  }
  else if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect().\n");
  }
  else if (-1 == getType())
  {
    ps_elog(PSL_CRITICAL, "Cannot serialize w/ no type.\n");
  }
  else if (NULL == getScraperBin())
  {
    ps_elog(PSL_CRITICAL, "Cannot store scraper w/ NULL bin.\n");
  }
  else if (!prepareQuery(s_szInsertSQL))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare: '%s'\n", s_szInsertSQL);
  }
  else if (!setInt(0, getType()))
  {
    ps_elog(PSL_CRITICAL, "Unable to set type %d in SQL.\n", getType());
  }
  else if (!setBlob(1, getScraperBin(), getBinLen()))
  {
    ps_elog(PSL_CRITICAL, "Unable to set scraper bin.\n");
  }
  else if (!update())
  {
    ps_elog(PSL_CRITICAL, "Unable to update: %s\n", getError());
  }
  else
  {
    setID(getLastID());

    bRet = serializeParam();
  }

  disconnect();

  return bRet;
}

bool ScraperDao::deserialize()
{
  bool bRet = false;

  int iID = getID();
  if (-1 == iID)
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialzie w/ no ID\n");
  }
  else if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (!prepareQuery(s_szSelectSQL))
  {
    ps_elog(PSL_CRITICAL, "Unable to prepare SQL: '%s'\n", s_szSelectSQL);
  }
  else if (!setInt(0, iID))
  {
    ps_elog(PSL_CRITICAL, "Unable to set ID: %d\n", iID);
  }
  else if (!exec())
  {
    ps_elog(PSL_CRITICAL, "Unable to exec query\n");
  }
  else if (!next())
  {
    ps_elog(PSL_CRITICAL, "Unable to locate DAO with ID: %d\n", iID);
  }
  else
  {
    string sName;
    char *pData = NULL;
    int iLen = 0;
    getStr(1, sName);
    getBlob(2, &pData, iLen);
    setScraperBin(pData, iLen);
    m_iType = getInt(3);
    delete[] pData;

    if (-1 != getParamID())
    {
      string sParam;
      m_sParam = "";
      clearQuery();
      if (!prepareQuery(s_szSelectParamSQL))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepareQuery(): '%s'\n", s_szSelectParamSQL);
      }
      else if (!setInt(0, getParamID()))
      {
        ps_elog(PSL_CRITICAL, "Unable to set param ID.\n");
      }
      else if (!exec())
      {
        ps_elog(PSL_CRITICAL, "Unable to execute query.\n");
      }
      else if (!next())
      {
        ps_elog(PSL_CRITICAL, "No param found w/ ID %d\n", getParamID());
      }
      else if (!getStr(0, sParam))
      {
        ps_elog(PSL_CRITICAL, "Unable to get string.\n");
      }
      else
      {
        setParam(sParam);
      }
    }

    bRet = true;
  }
  disconnect();

  return bRet;
}

bool ScraperDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  string sName = daoName();
  ScraperDao *pDao = NULL;
  if (NULL == (pDao = dynamic_cast<ScraperDao *>(DaoFactory::getInstance().create(sName))))
  {
    ps_elog(PSL_CRITICAL, "Unable to create DAO named: '%s'\n", sName.c_str());
  }
  else
  {
    pDao->setID(getID());
    pDao->setParamID(getParamID());
    if (!pDao->deserialize())
    {
      ps_elog(PSL_CRITICAL, "Unable to deserialize.\n");
      delete pDao;
      pDao = NULL;
    }
    else
    {
      p_oOutputList.push_back(pDao);
      bRet = true;
    }
  }

  return bRet;
}

std::string ScraperDao::daoName()
{
  return string(s_kszDaoName);
}

IPsDupable *ScraperDao::dup()
{
  return new ScraperDao();
}

bool ScraperDao::serializeParam()
{
  bool bRet = false;

  int iParamID = getParamID();
  string &sParam = getParam();
  if (-1 == iParamID && sParam == "")
  {
    bRet = true;
  }
  else if (-1 != iParamID)
  {
    bRet = true;
  }
  else
  {
    bool bLocalCon = false;
    if (!isConnected())
    {
      bLocalCon = true;
      if (!connect(__FILE__, __LINE__))
      {
        ps_elog(PSL_CRITICAL, "Was not connected, and can't connect now.\n");
      }
    }
    else
    {
      clearQuery();
    }

    if (!isConnected())
    {
      ps_elog(PSL_CRITICAL, "Cannot connect.\n");
    }

    else if (!prepareQuery(s_szInsertParamSQL))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare SQL: '%s'\n", s_szInsertParamSQL);
    }
    else if (!setStr(0, m_sParam))
    {
      ps_elog(PSL_CRITICAL, "Unable to set string: '%s'\n", m_sParam.c_str());
    }
    else if (!update())
    {
      ps_elog(PSL_CRITICAL, "Unable to update.\n");
    }
    else
    {
      bRet = true;
    }

    if (bLocalCon)
    {
      disconnect();
    }
  }

  return bRet;
}
