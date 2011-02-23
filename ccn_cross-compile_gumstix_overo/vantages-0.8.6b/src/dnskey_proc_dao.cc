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

#include <sstream>
#include <iostream>

#include "dnskey_proc_dao.h"
#include "scraper_dao.h"
#include "raw_data_dao.h"
#include "dnskey_scraper_dao.h"
#include "dao_factory.h"
#include "http_query.h"
#include "gpgme_crypt_mgr.h"
#include "ps_logger.h"

using namespace std;

const char *DnskeyProcDao::s_kszDaoName = "DNSKEY_PROC_DAO";

DnskeyProcDao::DnskeyProcDao()
{
  setScraperID(2);
  setScraper(dynamic_cast<DnskeyScraperDao *>(DaoFactory::getInstance().create(DnskeyScraperDao::s_kszDaoName)));
  setNsID(1);
}

DnskeyProcDao::~DnskeyProcDao()
{

}

DnskeyProcDao *DnskeyProcDao::dup()
{
  return new DnskeyProcDao();
}

bool DnskeyProcDao::process()
{
  bool bRet = false;

  ScraperDao *pScraperDao = getScraper();
  if (0 == numRawDaos() && 0 == numRawIDs())
  {
    ps_elog(PSL_CRITICAL, "Unable to process with no RAW data.\n");
  }
  else if (0 == numRawDaos() && !loadRawDaos())
  {
    ps_elog(PSL_CRITICAL, "Unable to load raw DAO(s)\n");
  }
  else if (numRawDaos() != 1)
  {
    ps_elog(PSL_CRITICAL, "Unable to process with anything other than >1< raw DAO, got: %d\n", numRawDaos());
  }
  else if (NULL == pScraperDao && !loadScraperDao())
  {
    ps_elog(PSL_CRITICAL, "Unable to load scraper DAO.\n");
  }
  else
  {
    RawDataDao *pRawDao = *beginRawDaos();
    pScraperDao = getScraper();

    pScraperDao->setData(pRawDao->getData(), pRawDao->getDataLen());
//ps_elog(PSL_CRITICAL, "RAW DATA IS: '%s'\n", pRawDao->getData());
    setDate(time(NULL));
    bRet = pScraperDao->execute();
    setData(pScraperDao->getResult());
//ps_elog(PSL_CRITICAL, "RESULT IS: '%s'\n", getData().c_str());
  }

  return bRet;
}

bool DnskeyProcDao::toString(std::string &p_sKeys)
{
  bool bRet = false;

  ostringstream oSS;
  oSS << getName()
      << "|"
      << getDate()
      << "|";

  string sData = getData();
  size_t uPos = string::npos;
  while (string::npos != (uPos = sData.find('\n')))
  {
    sData.replace(uPos, 1, ":");
  }

  oSS << sData
      << "|";
  string sSig = getSig();
  escapeNL(sSig);
  oSS << sSig;

  p_sKeys.assign(oSS.str());
  bRet = true;

  return bRet;
}

bool DnskeyProcDao::fromString(std::string &p_sKeys)
{
  bool bRet = false;

  string sKeys = p_sKeys;
  size_t uNameIdx = sKeys.find("|");
  size_t uDateIdx = string::npos;
  size_t uKeysIdx = string::npos;
  if (string::npos == uNameIdx)
  {
    ps_elog(PSL_CRITICAL, "Unable to find name in: '%s'\n", sKeys.c_str());
  }
  else if (string::npos == (uDateIdx = sKeys.find("|", uNameIdx + 1)))
  {
    ps_elog(PSL_CRITICAL, "Unable to find date in: '%s'\n", sKeys.c_str());
  }
  else if (string::npos == (uKeysIdx = sKeys.find("|", uDateIdx + 1)))
  {
    ps_elog(PSL_CRITICAL, "Unable to find keys in: '%s'\n", sKeys.c_str());
  }
  else
  {
    string sName = sKeys.substr(0, uNameIdx);
    setName(sName);
    string sDate = sKeys.substr(uNameIdx + 1, uDateIdx - (uNameIdx + 1));
    setDate((int) strtol(sDate.c_str(), NULL, 10));
    string sSig = sKeys.substr(uKeysIdx + 1, string::npos);
    unEscapeNL(sSig);
    correctSigSpace(sSig);
    setSig(sSig);
    sKeys.erase(uKeysIdx, string::npos);
    sKeys.erase(0, uDateIdx + 1);
    size_t uNextIdx = 0;
    while (string::npos != (uNextIdx = sKeys.find(":")))
    {
      sKeys.replace(uNextIdx, 1, "\n");
    }
    setData(sKeys);
//ps_elog(PSL_CRITICAL, "JUST SET <NAME. DATE, KEYS> to: <'%s', '%s', '%s'>\n", sName.c_str(), sDate.c_str(), sKeys.c_str());
    bRet = true;
  }

  return bRet;
}

std::string DnskeyProcDao::daoName()
{
  return string(s_kszDaoName);
}

inline bool DnskeyProcDao::escapeNL(std::string &p_sStr)
{
  bool bRet = true;

  size_t u = string::npos;
  while (string::npos != (u = p_sStr.find("\n")))
  {
    p_sStr.replace(u, 1, "%0a");
  }

  return bRet;
}

inline bool DnskeyProcDao::unEscapeNL(std::string &p_sStr)
{
  bool bRet = true;

  size_t u = string::npos;
  while (string::npos != (u = p_sStr.find("%0a")))
  {
    p_sStr.replace(u, 3, "\n");
  }

  return bRet;
}

inline bool DnskeyProcDao::correctSigSpace(std::string &p_sSig)
{
  bool bRet = true;

  size_t uBegin = p_sSig.find("\n\n");
  size_t uEnd = p_sSig.rfind("-----END");
  if (string::npos != uBegin
      && string::npos != uEnd)
  {
    size_t uIdx = uBegin;
    while (string::npos != (uIdx = p_sSig.find(" ", uIdx))
           && uIdx < uEnd)
    {
      p_sSig.replace(uIdx, 1, "+");
    }
  }

  return bRet;
}

std::string DnskeyProcDao::getVerifiableData(std::string p_sVersion)
{
  stringstream oRet;

  float fVersion = strtof(p_sVersion.c_str(), NULL);
  if (fVersion > 0.0)
  {
    oRet << getDate() << "|";
  }

  oRet << getData();
  ps_elog(PSL_DEBUG, "Version is '%s', returning: verifiable string '%s'\n", p_sVersion.c_str(), oRet.str().c_str());

  return oRet.str();
}

bool DnskeyProcDao::verify(std::string &p_sVersion, PsCryptMgr &p_oMgr)
{
  string sData = getVerifiableData(p_sVersion);
  return p_oMgr.verify(getSig(), sData);
}

