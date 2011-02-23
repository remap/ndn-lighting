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
#include <stdlib.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <sstream>
#include <iostream>

#include "ps_logger.h"
#include "dnskey_consist_dao.h"
#include "dnskey_defs.h"
#include "ps_defs.h"
#include "dao_factory.h"
#include "raw_data_dao.h"
#include "ps_config.h"

using namespace std;

const char *DnskeyConsistencyDao::s_kszDaoName = "DNSKEY_CONSISTENCY_DAO";

DnskeyConsistencyDao::DnskeyConsistencyDao()
  : m_iResult(PS_F_RULE_PREINIT),
    m_iNumFriends(0)
{
  setNsID(1);
}

DnskeyConsistencyDao::~DnskeyConsistencyDao()
{

}

void DnskeyConsistencyDao::setNumFriends(int p_iNumFriends)
{
  m_iNumFriends = p_iNumFriends;
}

bool DnskeyConsistencyDao::addProcDnskey(DnskeyProcDao &p_oLocalDao)
{
  bool bRet = true;

  string &sName = p_oLocalDao.getName();
  string &sData = p_oLocalDao.getData();
  if (!sName.empty() && !sData.empty())
  {
    setName(sName);
    setDate(time(NULL));
    setData(sData);
  }

  int iID = p_oLocalDao.getID();
  if (-1 == iID)
  {
    ps_elog(PSL_CRITICAL, "Initing w/ DnskeyProcDao ID -1.\n");
    bRet = false;
  }
  else
  {
    DnskeyProcDao *pDao = p_oLocalDao.dup();
    pDao->setID(iID);
    if (!pDao->deserialize())
    {
      ps_elog(PSL_ERROR, "Unable to deserialize dao for ID: '%d'\n", iID);
      delete pDao;
      bRet = false;
    }
    else if (sName != pDao->getName())
    {
      ps_elog(PSL_CRITICAL, "Deserialize() DAO != this DAO's name: '%s' != '%s'\n", pDao->getName().c_str(), sName.c_str());
      delete pDao;
      bRet = false;
    }
    else
    {
      addProcDao(pDao);

      string sBlahData = getData();
      if (sBlahData.empty())
      {
        setName(pDao->getName());
        setDate(time(NULL));
        setData(pDao->getData());
      }
    }
  }


  return bRet;
}

bool DnskeyConsistencyDao::addRemoteKey(std::string &p_sURL, std::string &p_sKey)
{
  bool bRet = false;

  RawDataDao *pRawDao = (RawDataDao *) DaoFactory::getInstance().create(RawDataDao::s_kszDaoName);
  if (NULL == pRawDao)
  {
    ps_elog(PSL_CRITICAL, "Unable to create raw dao.\n");
  }
  else
  {
    pRawDao->setNsID(getNsID());
    pRawDao->setName(getName());
    pRawDao->setSrc(p_sURL);
    pRawDao->setData(p_sKey.c_str(), p_sKey.size());

    pRawDao->deserialize();
    pRawDao->setDate(time(NULL));
    if (!pRawDao->serialize())
    {
      ps_elog(PSL_CRITICAL, "Unable to serialize raw DAO for name: '%s'\n", pRawDao->getName().c_str());
      delete pRawDao;
      pRawDao = NULL;
    }
    else
    {
      addRawDao(pRawDao);
      bRet = true;
    }
  }

  return bRet;
}

bool DnskeyConsistencyDao::process()
{
  bool bRet = false;

  string &sName = getName();
  string sLocalData;
  bool bRemoteHit = false;
/*
  time_t tDate = getDate();
  setDate(0);
  toString(sLocalData);
  setDate(tDate);
  sLocalData = stripSig(sLocalData);
*/
  sLocalData = getData();

  int iCount = 0;
  bRet = true;
  for (ProcDaoIter_t tProcIter = beginProcDaos();
       endProcDaos() != tProcIter;
       tProcIter++)
  {
    DnskeyProcDao *pProcDao = static_cast<DnskeyProcDao *>(*tProcIter);
    if (NULL == pProcDao)
    {
      ps_elog(PSL_CRITICAL, "Unable to cast DAO.\n");
    }
    else if (pProcDao->getName() != sName)
    {
      ps_elog(PSL_CRITICAL, "Candidate [%d] name ('%s') is not the same as ours ('%s')?\n",
              pProcDao->getID(),
              pProcDao->getName().c_str(),
              sName.c_str());
    }
    else
    {
      string sData;
/*
      tDate = pProcDao->getDate();
      pProcDao->setDate(0);
      pProcDao->toString(sData);
      sData = stripSig(sData);
      pProcDao->setDate(tDate);
*/
      sData = pProcDao->getData();
      if (sData == sLocalData)
      {
        iCount++;
      }
      else
      {
        ps_elog(PSL_WARNING, "Diff local data:\n'%s'\n!=\n'%s'\n", sLocalData.c_str(), sData.c_str());
        iCount = -1;
        break;
      }
    }
  }
/*
*/

  for (RawDaoIter_t tIter = beginRawDaos();
       iCount > -1
       && endRawDaos() != tIter;
       tIter++)
  {
/*
    string sRawData = (*tIter)->getData();
    sRawData = stripSig(sRawData);
*/
    string sRawData = (*tIter)->getData();
    DnskeyProcDao oRemoteDao;
    if (!oRemoteDao.fromString(sRawData))
    {
      ps_elog(PSL_ERROR, "Unable to convert raw data to DAO: '%s'\n", sRawData.c_str());
    }
    else if (sLocalData == (sRawData = oRemoteDao.getData()))
    {
      iCount++;
      bRemoteHit = true;
    }
    else
    {
      ps_elog(PSL_WARNING, "Different data seen for '%s' by friend:\n'%s'\n!=\n'%s'\n", sName.c_str(), sLocalData.c_str(), sRawData.c_str());
      iCount = -1;
      break;
    }
  }

  char szCode[3];
  szCode[2] = '\0';
  string sData;

  int iThreshold = PS_F_RULE_THRESHOLD;
  PsConfig &oConfig = PsConfig::getInstance();
  const char *szThreshold = oConfig.getValue(DNSKEY_CONFIG_CONSISTENCY_THRESHOLD);
  if (NULL != szThreshold)
  {
    iThreshold = (int) strtol(szThreshold, NULL, 10);
    iThreshold = (0 >= iThreshold) ? PS_F_RULE_THRESHOLD : iThreshold;
  }

  if (-1 == iCount)
  {
    m_iResult = PS_F_RULE_CONFLICT;
  }
  else if (iCount >= iThreshold)
  {
    m_iResult = PS_F_RULE_CONFIRMED;
    sData = getData();
  }
  else if (bRemoteHit && iCount >= m_iNumFriends)
  {
    m_iResult = PS_F_RULE_CONFIRMED;
    sData = getData();
  }
  else if (iCount > 0 && bRemoteHit)
  {
    m_iResult = PS_F_RULE_PROVISIONAL;
    sData = getData();
  }
  else
  {
    m_iResult = PS_F_RULE_UNKNOWN;
    sData = getData();
  }

  sprintf(szCode, "%d|", m_iResult);
  sData = szCode + sData;
  setData(sData);

  return bRet;
}

int DnskeyConsistencyDao::getResult()
{
  return m_iResult;
}

inline std::string DnskeyConsistencyDao::stripSig(std::string &p_sData)
{
  string sRet = p_sData;

  size_t u = sRet.rfind("|");
  if (string::npos != u)
  {
    sRet.erase(u, string::npos);
  }

  if (!p_sData.empty() && sRet.empty())
  {
    ps_elog(PSL_CRITICAL, "Strip took it to empty:\n\t'%s\n\t\t->\n\t'%s'\n", p_sData.c_str(), sRet.c_str());
  }

  return sRet;
}

DnskeyConsistencyDao *DnskeyConsistencyDao::dup()
{
  return new DnskeyConsistencyDao();
}

std::string DnskeyConsistencyDao::daoName()
{
  return s_kszDaoName;
}
