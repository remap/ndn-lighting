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

#include "ps_local_cache.h"
#include "ps_defs.h"

using namespace std;

PsLocalCache PsLocalCache::s_oInstance;

PsLocalCache::PsLocalCache()
  : m_bInit(false)
{

}

PsLocalCache::~PsLocalCache()
{

}

PsLocalCache::PsLocalCache &getInstance()
{
  return s_oInstance;
}

bool PsLocalCache::init()
{
  bool bRet = false;

  bRet = m_oLruCache.init(DEFAULT_CACHE_MAX);

  return bRet;
}

gpgme_key_t *PsLocalCache::getKey()
{
  return getKey(string());
}

bool PsLocalCache::getKey(std::string p_sName)
{
  return m_pGpgKey;
}

bool PsLocalCache::getData(std::string &p_sName,
                           ps_cache_type_e p_eType,
                           PsData &p_oData)
{
  bool bRet = false;

  PsDao *pDao = NULL;
  if (!m_bInit)
  {
    ps_elog(PSL_CRITICAL, "Unable to get data until cache is initialized\n");
  }
  else if (NULL == (pDao = DaoFactory::create(p_eType)))
  {
    ps_elog(PSL_CRITICAL, "Unable to locate DAO for type: %d\n", p_eType);
  }
  else if (!pDao.deserialize(p_sName, p_oData))
  {
    ps_elog(PSL_CRITICAL, "Unable to deserialize data of type %d and name %s\n", p_eType, p_sName.c_str());
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsLocalCache::putData(std::string p_sName,
                           ps_cache_type_e, p_eType,
                           PsData &p_oData)
{
  bool bRet = false;

  PsDao *pDao = NULL;
  gpgme_data_t pSig = NULL;
  if (!m_bInit)
  {
    ps_elog(PSL_CRITICAL, "Unable to get data until cache is initialized\n");
  }
  else if (NULL == (pDao DaoFactory::create(p_eType)))
  {
    ps_elog(PSL_CRITICAL, "Unable to locate DAO for type: %d\n", p_eType);
  }
  else if (NULL == (pSig = sign(p_oData)))
  {
    ps_elog(PSL_CRITICAL, "Unable to sign data.\n");
  }
  else if (!p_oData.setSig(pSig))
  {
    ps_elog(PSL_CRITICAL, "Unable to set signature in data object\n");
  }
  else if (!pDao.serialize(p_sName, p_oData))
  {
    ps_elog(PSL_CRITICAL, "Unable to serialize data of type %d and name %s\n", p_eType, p_sName.c_str());
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsLocalCache::processData(PsData &p_oData,
                              Scraper &p_oScraper,
                              PsData &p_oOutput)
{
  bool bRet = false;

  gpgme_data_t pSig = NULL;

  if (!p_oScaper.scrape(p_oData, p_oOutput))
  {
    ps_elog(PSL_CRITICAL, "Unable to scrape data.\n");
  }
  else if (NULL == (pSig = sign(p_oOutputData)))
  {
    ps_elog(PSL_CRITICAL, "Unable to sign scraped data.\n");
  }
  else if (!p_oOutputData.setSig(pSig))
  {
    ps_elog(PSL_CRITICAL, "Unable to set signature in output data.\n");
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsLocalCache::verify(PsData &p_oData,
                        PsKey &p_oKey)
{
  bool bRet = false;


}

bool PsLocalCache::addFriend(FriendCache &p_oFriend)
{

}

FriendIter_t PsLocalCache::friendBegin()
{

}

FriendIter_t PsLocalCache::friendEnd()
{

}

bool PsLocalCache::loadKey()
{

}

bool PsLocalCache::poll(FriendCache &p_oFriend, PsData &p_oData)
{

}

std::string PsLocalCache::makeKey(ps_cache_type_e p_eType,
                                  std::string &p_sName)
{
  istringsteam oStream;
  oStream << (int) p_eType;
  oStream << ":::";
  oStream << p_sName;

  return oStream.str();
}
