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

#include "dnskey_web_task.h"
#include "raw_data_dao.h"
#include "dnskey_proc_dao.h"
#include "dao_factory.h"
#include "perl_scraper.h"
#include "gpgme_crypt_mgr.h"
#include "ps_app.h"
#include "ps_defs.h"

using namespace std;

DnskeyWebTask::DnskeyWebTask(PsApp &p_oOwnerApp)
  : PsPollTask(p_oOwnerApp),
    m_bDone(false),
    m_eSrc(DNSKEY_SRC_PRE_INIT)
{

}

DnskeyWebTask::~DnskeyWebTask()
{

}

dnskey_src_e DnskeyWebTask::getSubmissionSrc()
{
  return m_eSrc;
}

void DnskeyWebTask::setSubmissionSrc(dnskey_src_e p_eSrc)
{
  m_eSrc = p_eSrc;
}

bool DnskeyWebTask::execute()
{
  bool bRet = false;

  string &sURL = getURL();
  if (!m_oQuery.query(sURL))
  {
    ps_elog(PSL_CRITICAL, "Unable to query: '%s'\n", sURL.c_str());
  }
  else
  {
    bRet = true;
  }
  m_bDone = true;

  return bRet;
}

bool DnskeyWebTask::done()
{
  return m_bDone;
}

bool DnskeyWebTask::process()
{
  bool bRet = false;

  ps_elog(PSL_DEBUG, "STARTING: URL / Name: '%s' / '%s'\n", getURL().c_str(), getName().c_str());
  string &sData = m_oQuery.getResponse();
  if (!done())
  {
    ps_elog(PSL_CRITICAL, "Task not done yet.\n");
  }
  else if (sData == "")
  {
    ps_elog(PSL_CRITICAL, "No data for URL / Name: '%s' / '%s'\n", getURL().c_str(), getName().c_str());
  }
  else
  {
    DnskeyProcDao *pProcDao = NULL;
    try
    {
      DaoFactory &oFact = DaoFactory::getInstance();
      RawDataDao *pWebDao = dynamic_cast<RawDataDao *>(oFact.create(getRawDaoName()));
      pProcDao = dynamic_cast<DnskeyProcDao *>(oFact.create(getProcDaoName()));
      if (NULL == pWebDao)
      {
        ps_elog(PSL_CRITICAL, "Unable to create DAO for name: '%s'\n", getRawDaoName().c_str());
      }
      else if (NULL == pProcDao)
      {
        ps_elog(PSL_CRITICAL, "Unable to create DAO for name: '%s'\n", getProcDaoName().c_str());
        delete pWebDao;
      }
      else
      {
        pProcDao->setName(getName());
        pProcDao->addRawDao(pWebDao);
        pProcDao->setScraperID(getScraperID());
        pProcDao->setScraperParamID(getScraperParamID());
        PerlScraper *pScraper = dynamic_cast<PerlScraper *>(oFact.create(PerlScraper::s_kszDaoName));
        pScraper->setID(getScraperID());
        pScraper->setParamID(getScraperParamID());
        pScraper->deserialize();
        pProcDao->setScraper(pScraper);

        pWebDao->setName(getName());
//        pWebDao->setData(sData.c_str(), sData.length() + 1);
        pWebDao->setData(sData.c_str(), sData.length());
        pWebDao->setSrc(getURL());
        pWebDao->setNsID(1);
        pWebDao->setDate(time(NULL));
        pWebDao->deserialize();

        GpgmeCryptMgr *pMgr = dynamic_cast<GpgmeCryptMgr *>(getCryptMgr());
              
        string sData = (NULL == pWebDao->getData()) ? "" : pWebDao->getData();
        string sSig;
        if (NULL == pMgr)
        {
          ps_elog(PSL_WARNING, "No crypt mgrs specified.\n");
        }
        else if (!pMgr->init())
        {
          ps_elog(PSL_CRITICAL, "Unable to re-init crypt mgr.\n");
        }
        else if (!pMgr->setSigningKey(pMgr->getSigningKey()))
        {
          ps_elog(PSL_CRITICAL, "Unable to re-set signing key in crypt mgr.\n");
        }
        else if (!pMgr->sign(sData, sSig, true))
        {
          ps_elog(PSL_CRITICAL, "Unable to sign data from raw DAO.\n");
        }
        else if (!pWebDao->setSig(sSig))
        {
          ps_elog(PSL_CRITICAL, "Unable to set sig data in raw DAO.\n");
        }


        ps_elog(PSL_DEBUG, "SCRAPER ID IS: %d\n", pProcDao->getScraperID());
        if (!pWebDao->serialize())
        {
          ps_elog(PSL_CRITICAL, "Unable to serialize raw DAO.\n");
        }
        else if (!pProcDao->process())
        {
          ps_elog(PSL_CRITICAL, "Unable to process data.\n");
        }
        else
        {
          if (NULL != pMgr)
          {
            sData = pProcDao->getVerifiableData(DNSKEY_CURRENT_VERSION);
            sSig = "";
            if (!pMgr->sign(sData, sSig, true))
            {
              ps_elog(PSL_CRITICAL, "Unable to sign data for proc DAO.\n");
            }
            else if (!pProcDao->setSig(sSig))
            {
              ps_elog(PSL_CRITICAL, "Unable to set sig data in proc DAO.\n");
            }
          }

          if (!pProcDao->serialize())
          {
            ps_elog(PSL_CRITICAL, "Unable to serialize proc DAO.\n");
          }
          else
          {
            ps_elog(PSL_DEBUG, "SUCCESS.\n");
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
  }

  return bRet;
}

