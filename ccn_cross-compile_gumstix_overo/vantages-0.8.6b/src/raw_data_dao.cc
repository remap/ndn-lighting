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

#include <iostream>
#include <sstream>

#include "raw_data_dao.h"
#include "dao_factory.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

const char *RawDataDao::s_kszDaoName = "RAW_DATA_DAO";

const char *RawDataDao::s_kszInsertSQL = "insert into PS_RAW_DATA  \
  (SRC, FIRST_SEEN, LAST_SEEN, SIG, NAME, RAW_DATA, NS_ID) \
  VALUES (?, ?, ?, ?, ?, ?, ?)";

const char *RawDataDao::s_kszUpdateSQL = "update PS_RAW_DATA set \
  LAST_SEEN = ? \
  where ID = ?";

const char *RawDataDao::s_kszSelectSQL = "select ID, SIG, NAME, SRC, RAW_DATA \
  from PS_RAW_DATA ";

RawDataDao::RawDataDao()
  : m_tDate(-1),
//    m_pSig(NULL),
    m_pBuff(NULL),
//    m_uSigLen(0),
    m_uBuffLen(0)
{

}

RawDataDao::~RawDataDao()
{
/*
  if (NULL != m_pSig)
  {
    delete[] m_pSig;
    m_pSig = NULL;
  }
*/

  if (NULL != m_pBuff)
  {
    delete[] m_pBuff;
    m_pBuff = NULL;
  }
}

std::string &RawDataDao::getName()
{
  return m_sName;
}

void RawDataDao::setName(const std::string &p_sName)
{
  m_sName = p_sName;
}

std::string &RawDataDao::getSrc()
{
  return m_sSrc;
}

void RawDataDao::setSrc(const std::string &p_sSrc)
{
  m_sSrc = p_sSrc;
}

time_t RawDataDao::getDate()
{
  return m_tDate;
}

void RawDataDao::setDate(time_t p_tDate)
{
  m_tDate = p_tDate;
}

/*
char *RawDataDao::getSig()
{
  return m_pSig;
}

size_t RawDataDao::getSigLen()
{
  return m_uSigLen;
}

bool RawDataDao::setSig(const char *p_pSig, size_t p_uSigLen)
{
  bool bRet = false;

  if (NULL == p_pSig)
  {
    ps_elog(PSL_DEBUG, "Unable to set NULL signature\n");
  }
  else if (0 == p_uSigLen)
  {
    ps_elog(PSL_DEBUG, "Unable to set sig wotjh length 0\n");
  }
  else
  {
    if (NULL != m_pSig)
    {
      delete[] m_pSig;
      m_pSig = NULL;
    }

    m_uSigLen = p_uSigLen;
    m_pSig = new char[m_uSigLen];
    memcpy(m_pSig, p_pSig, m_uSigLen);
    bRet = true;
  }

  return bRet;
}
*/

char *RawDataDao::getData()
{
  return m_pBuff;
}

size_t RawDataDao::getDataLen()
{
  return m_uBuffLen;
}

bool RawDataDao::setData(const char *p_pBuff, size_t p_uBuffLen)
{
  bool bRet = false;

  if (NULL == p_pBuff)
  {
    ps_elog(PSL_WARNING, "Unable to set NULL data.\n");
  }
  else if (0 == p_uBuffLen)
  {
    ps_elog(PSL_WARNING, "Unable to set data with length 0\n");
  }
  else
  {
    if (NULL != m_pBuff)
    {
      delete[] m_pBuff;
      m_pBuff = NULL;
    }

    m_uBuffLen = p_uBuffLen;
    m_pBuff = new char[m_uBuffLen + 1];
    m_pBuff[m_uBuffLen] = '\0';
    memcpy(m_pBuff, p_pBuff, m_uBuffLen);
    bRet = true;
  }

  return bRet;
}

bool RawDataDao::serialize()
{
  bool bRet = false;

  const char *pData = getData();
  size_t uDataLen = getDataLen();
  time_t tDate = getDate();
  string &sName = getName();
  string &sSrc = getSrc();
//  string sSig = (getSig() == NULL) ? "" : getSig();
  string &sSig = getSig();
  string sData;
  sData.assign(pData, uDataLen);

  ps_elog(PSL_DEBUG, "THE SIG IS '%s'\n", sSig.c_str());
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (-1 == getID())
  {
    int iNamespace = getNsID();

    if (-1 == iNamespace)
    {
      ps_elog(PSL_CRITICAL, "Unable to insert with namesapce %d\n", iNamespace);
    }
    else if (NULL == pData)
    {
      ps_elog(PSL_CRITICAL, "Unable to serialize with NULL data.\n");
    }
    else if (0 == uDataLen)
    {
      ps_elog(PSL_CRITICAL, "Unable to serialize with 0 length data.\n");
    }
    else if ("" == sName)
    {
      ps_elog(PSL_CRITICAL, "Unable to serialize with empty name\n");
    }
    else if ("" == sSrc)
    {
      ps_elog(PSL_CRITICAL, "Unable to serialize with empty source.\n");
    }
    else if (!prepareQuery(s_kszInsertSQL))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare SQL: \"%s\"\n", s_kszInsertSQL);
    }
    else if (!setStr(0, sSrc))
    {
      ps_elog(PSL_CRITICAL, "Unable to set src \"%s\"\n", sSrc.c_str());
    }
    else if (!setInt(1, tDate))
    {
      ps_elog(PSL_CRITICAL, "Unable to set first seen \"%d\"\n", (int) tDate);
    }
    else if (!setInt(2, tDate))
    {
      ps_elog(PSL_CRITICAL, "Unable to set last seen \"%d\"\n", (int) tDate);
    }
    else if (!setStr(3, sSig))
    {
      ps_elog(PSL_CRITICAL, "Unable to set sig \"%s\"\n", sSig.c_str());
    }
    else if (!setStr(4, sName))
    {
      ps_elog(PSL_CRITICAL, "Unable to set name \"%s\"\n", sName.c_str());
    }
    else if (!setStr(5, sData))
    {
      ps_elog(PSL_CRITICAL, "Unable to set data \"%s\"\n", sData.c_str());
    }
    else if (!setInt(6, iNamespace))
    {
      ps_elog(PSL_CRITICAL, "Unable to set namesapce %d\n", iNamespace);
    }
/*
    else if (!exec())
    {
      ps_elog(PSL_DEBUG, "Unable to exec.\n");
    }
*/
    else if (!update())
    {
      ps_elog(PSL_CRITICAL, "Unable to update: '%s'\n", getError());
    }
    else
    {
      setID(getLastID());
      bRet = true;
    }
  }
  else
  {
    if (!prepareQuery(s_kszUpdateSQL))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: \"%s\"\n", s_kszUpdateSQL);
    }
    else if (!setInt(0, tDate))
    {
      ps_elog(PSL_CRITICAL, "Unable to update with date %d of ID: %d\n", (int) tDate, getID());
    }
    else if (!setInt(1, getID()))
    {
      ps_elog(PSL_CRITICAL, "Unable to set ID: %d\n", getID());
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

  disconnect();

  return bRet;
}

bool RawDataDao::deserialize()
{
  bool bRet = false;

  int iID = getID();
  ostringstream oSS;
  oSS << s_kszSelectSQL;
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (-1 == iID)
  {
//    ps_elog(PSL_DEBUG, "Unable to desrialize single DAO w/o ID.\n");
    string &sName = getName();
    string &sSrc = getSrc();
    char *szData = getData();
    int iNamespace = getNsID();

    if (0 == sName.length())
    {
      ps_elog(PSL_WARNING, "Unable to deserialize single DAO w/o name.\n");
    }
    else if (0 == sSrc.length())
    {
      ps_elog(PSL_WARNING, "Unable to deserialize single DAO (name '%s') w/o Source.\n", sName.c_str());
    }
    else if (NULL == szData)
    {
      ps_elog(PSL_WARNING, "Unable to deserialize single DAO (name / src: '%s' / '%s') w/o data\n", sName.c_str(), sSrc.c_str());
    }
    else if (-1 == iNamespace)
    {
      ps_elog(PSL_WARNING, "Unable to deserialize single DAO (name / src / Data : '%s' / '%s' / '%s') w/o namespace\n",
             sName.c_str(),
             sSrc.c_str(),
             szData);
    }
    else
    {
//      oSS << "where NAME = ? and SRC = ? and RAW_DATA = ? and NS_ID = ?";
      oSS << "where NAME = ? and SRC = ? and RAW_DATA = ? and NS_ID = ?";
      string sQuery = oSS.str();
      string sData = szData;
      string sSig;

      if (!prepareQuery(sQuery))
      {
        ps_elog(PSL_CRITICAL, "Unable to prepareQuery() for SQL: '%s'\n", sQuery.c_str());
      }
      else if (!setStr(0, sName))
      {
        ps_elog(PSL_CRITICAL, "Unable to set name.\n");
      }
      else if (!setStr(1, sSrc))
      {
        ps_elog(PSL_CRITICAL, "Unable to set src.\n");
      }
      else if (!setStr(2, sData))
      {
        ps_elog(PSL_CRITICAL, "Unable to set data.\n");
      }
      else if (!setInt(3, iNamespace))
      {
        ps_elog(PSL_CRITICAL, "Unable to set namespace: %d\n", iNamespace);
      }
      else if (!exec())
      {
        ps_elog(PSL_CRITICAL, "Unable to exec() query: '%s'\n", sQuery.c_str());
      }
      else if (!next())
      {
        ps_elog(PSL_INFO, "No raw DAO match\n");
      }
      else if (!getStr(1, sSig))
      {
        ps_elog(PSL_ERROR, "Unable to get signature: %s\n", getError());
      }
      else
      {
        int iID = getInt(0);

        if (next())
        {
          ps_elog(PSL_CRITICAL, "More than one result matched.  This is an error in the database.\n");
        }
        else
        {
          setID(iID);
          setSig(sSig);
          bRet = true;
        }
      }
    }
  }
  else
  {
    oSS << " where ID = " << iID;
    string sQuery = oSS.str();

    if (!prepareQuery(sQuery))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", oSS.str().c_str());
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to exec() query: '%s'\n", oSS.str().c_str());
    }
    else if (!next())
    {
      ps_elog(PSL_CRITICAL, "Unable to locate DAO with ID: %d\n", getID());
    }
    else
    {
      string sName;
      string sSrc;
      string sData;
      if (!getStr(2, sName))
      {
        ps_elog(PSL_CRITICAL, "Unable to get Name.\n");
      }
      else if (!getStr(3, sSrc))
      {
        ps_elog(PSL_CRITICAL, "Unable to get srouce\n");
      }
      else if (!getStr(4, sData))
      {
        ps_elog(PSL_CRITICAL, "Unable to get data.\n");
      }
      else
      {
        setID(getInt(0));
        setName(sName);
        setSrc(sSrc);
        setData(sData.c_str(), sData.size());
        bRet = true;
      }
    }

    if (next())
    {
      ps_elog(PSL_CRITICAL, "Error: More than one record for ID: %d\n", getID());
      bRet = false;
    }
  }

  disconnect();

  return bRet;
}

bool RawDataDao::deserialize(DaoList_t &p_oOutputList)
{
  bool bRet = false;

  int iID = getID();
  int iNamespace = getNsID();
  time_t tDate = getDate();
  string &sName = getName();
  string &sSrc = getSrc();

  ostringstream oSS;
  oSS << s_kszSelectSQL;
  bool bSuccess = false;
  if (!connect(__FILE__, __LINE__))
  {
    ps_elog(PSL_CRITICAL, "Unable to connect.\n");
  }
  else if (-1 != iID)
  {
    oSS << " where ID = ? ";
    string sQuery = oSS.str();

    if (!prepareQuery(sQuery))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: '%s'\n", oSS.str().c_str());
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to exec() query: '%s'\n", oSS.str().c_str());
    }
    else
    {
      bSuccess = true;
    }
  }
  else if (-1 == iNamespace)
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize wihtout namesapce.\n");
  }
  else
  {
    // Here come the first of the hacks.
    // Since we need dybnamic SQL, but
    // we can't binf parameters until adter
    // we prepare the statment, we will
    // dynamically generate SQL w/ the
    // parameters in it.  This will be
    // lame if (for example) one of the
    // strings has an embedded ' char but
    // c'est la vie for now.
    oSS << " where NS_ID = " << iNamespace;
    if (-1 != tDate)
    {
      oSS << " and ";
      oSS << " LAST_SEEN = " << tDate << " ";
    }

    if ("" != sName)
    {
      oSS << " and ";
      oSS << " NAME = '" << sName << "' ";
    }

    if ("" != sSrc)
    {
      oSS << " and ";
      oSS << " SRC = '" << sSrc << "'";
    }

    string sQuery = oSS.str();
//ps_elog(PSL_DEBUG, "THE SQL IS: '%s'\n", sQuery.c_str());
    if (!prepareQuery(sQuery))
    {
      ps_elog(PSL_CRITICAL, "Unable to prepare query: \"%s\"\n", oSS.str().c_str());
    }
    else if (!exec())
    {
      ps_elog(PSL_CRITICAL, "Unable to exec() query: '%s'\n", oSS.str().c_str());
    }
    else
    {
      bSuccess = true;
    }
  }

  if (bSuccess)
  {
int i = 0;
    DaoFactory &oFact = DaoFactory::getInstance();
    while (next())
    {
      RawDataDao *pDao = (RawDataDao *) oFact.create(daoName());
      string sSig;
      string sName;
      string sSrc;
      string sData;
      if (NULL == pDao)
      {
        ps_elog(PSL_CRITICAL, "Unable to create instance of THIS DAO: '%s'\n", daoName().c_str());
      }
      else if (!getStr(1, sSig))
      {
        ps_elog(PSL_ERROR, "Unable to get signature: %s\n", getError());
      }
      else if (!getStr(2, sName))
      {
        ps_elog(PSL_CRITICAL, "Unable to get Name.\n");
      }
      else if (!getStr(3, sSrc))
      {
        ps_elog(PSL_CRITICAL, "Unable to get srouce\n");
      }
      else if (!getStr(4, sData))
      {
        ps_elog(PSL_CRITICAL, "Unable to get data.\n");
      }
      else
      {
        pDao->setID(getInt(0));
        pDao->setSig(sSig);
        pDao->setNsID(iNamespace);
        pDao->setName(sName);
        pDao->setSrc(sSrc);
        pDao->setData(sData.c_str(), sData.size());
        p_oOutputList.push_back(pDao);
i++;
      }
    }
//ps_elog(PSL_DEBUG, "%d rows\n", i);

    bRet = true;
  }

  disconnect();

  return bRet;
}

string RawDataDao::daoName()
{
  return string(s_kszDaoName);
}

RawDataDao *RawDataDao::dup()
{
  return new RawDataDao();
}
