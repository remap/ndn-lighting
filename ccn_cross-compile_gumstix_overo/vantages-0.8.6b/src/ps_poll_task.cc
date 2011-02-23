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

#include "ps_poll_task.h"
#include "ps_app.h"
#include "ps_crypt_mgr.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;


PsPollTask::PsPollTask(PsApp &p_oOwnerApp)
  : m_iScraperID(-1),
    m_iScraperParamID(-1),
    m_iMaxRetries(3),
    m_iRetry(0),
    m_pCryptMgr(NULL),
    m_oOwnerApp(p_oOwnerApp)
{

}

PsPollTask::~PsPollTask()
{
  setCryptMgr(NULL);
}

bool PsPollTask::init(std::string &p_sURL,
                  std::string &p_sName,
                  const char *p_szRawDaoName,
                  const char *p_szProcDaoName,
                  int p_iScraperID,
                  int p_iScraperParamID /*= -1*/)
{
  string sRaw = p_szRawDaoName;
  string sProc = p_szProcDaoName;

  return init(p_sURL, p_sName, sRaw, sProc, p_iScraperID, p_iScraperParamID);
}

bool PsPollTask::init(std::string &p_sURL,
                  std::string &p_sName,
                  std::string &p_sRawDaoName,
                  std::string &p_sProcDaoName,
                  int p_iScraperID,
                  int p_iScraperParamID /*= -1*/)
{
  bool bRet = false;

  setURL(p_sURL);
  setName(p_sName);
  m_sRawDaoName = p_sRawDaoName;
  m_sProcDaoName = p_sProcDaoName;
  m_iScraperID = p_iScraperID;
  m_iScraperParamID = p_iScraperParamID;
  bRet = true;

  return bRet;
}

int PsPollTask::getMaxRetries()
{
  return m_iMaxRetries;
}

void PsPollTask::setMaxRetries(int p_iMax)
{
  m_iMaxRetries = p_iMax;
}

int PsPollTask::getRetry()
{
  return m_iRetry;
}

void PsPollTask::setRetry(int p_iRetry)
{
  m_iRetry = p_iRetry;
}

std::string &PsPollTask::getURL()
{
  return m_sURL;
}

void PsPollTask::setURL(std::string &p_sURL)
{
  m_sURL = p_sURL;
  m_oParser.parse(m_sURL);
}

std::string &PsPollTask::getProto()
{
  return m_oParser.getProto();
}

std::string &PsPollTask::getName()
{
  return m_sName;
}

void PsPollTask::setName(std::string &p_sName)
{
  m_sName = p_sName;
}

std::string &PsPollTask::getRawDaoName()
{
  return m_sRawDaoName;
}

std::string &PsPollTask::getProcDaoName()
{
  return m_sProcDaoName;
}

int PsPollTask::getScraperID()
{
  return m_iScraperID;
}

int PsPollTask::getScraperParamID()
{
  return m_iScraperParamID;
}

PsCryptMgr *PsPollTask::getCryptMgr()
{
  return m_pCryptMgr;
}

void PsPollTask::setCryptMgr(PsCryptMgr *p_pCryptMgr)
{
  if (NULL != m_pCryptMgr)
  {
    delete m_pCryptMgr;
    m_pCryptMgr = NULL;
  }

  m_pCryptMgr = p_pCryptMgr;
}

PsApp &PsPollTask::getOwnerApp()
{
  return m_oOwnerApp;
}
