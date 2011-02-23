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

PsAgent PsAgent::s_oInstance;

PsAgent::PsAgent()
  : m_bInit(false),
    m_pCryptMgr(NULL)
{

}

PsAgent::~PsAgent()
{

}

PsAgent::PsAgent &getInstance()
{
  return s_oInstance;
}

bool PsAgent::init()
{
  bool bRet = false;

/*
  m_pCryptMgr = (PsCryptMgr) PsCryptFactory::getInstance().create(GpgmeCryptMgr::GPGME_CRYPT_MGR);
  if (NULL == m_pCryptMgr)
  {
    ps_elog(PSL_CRITICAL, "Unable to create new crypt mgr from string: %s\n", GpgmeCryptMgr::GPGME_CRYPT_MGR);
  }
  else
*/
  {
    bRet = true;
    m_bInit = true;
  }

  return bRet;
}

PsKey *PsAgent::getKey()
{

}

bool PsAgent::processData(RawDataDao &p_oRawDao,
                          ScraperDao &p_oScraper,
                          ProcessedDataDao &p_oOutput)
{
  bool bRet = false;

  char *pSig = NULL;
  if (!m_bInit)
  {
    ps_elog(PSL_CRITICAL, "Agent not initialized.\n");
  }
/*
  else if (!p_oScaper.scrape(p_oRawData, p_oOutput))
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
*/
  else if (!p_oOutput.serialize())
  {
    ps_elog(PSL_CRITICAL, "Unable to serialize newly processed data.\n");
  }
  else
  {
    bRet = true;
  }

  return bRet;
}

bool PsAgent::verify(PsDao &p_oDao,
                     PsKey &p_oKey)
{
  bool bRet = false;
}

