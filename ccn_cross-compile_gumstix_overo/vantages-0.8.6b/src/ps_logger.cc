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
#include <fcntl.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>

#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

PsLogger PsLogger::s_oInstance;

PsLogger::PsLogger()
  : m_iLevel(PSL_CRITICAL),
    m_pFile(stdout),
    m_pLevelNames(NULL)
{
  m_pLevelNames = new string[5];
  m_pLevelNames[PSL_CRITICAL] = string("CRITICAL");
  m_pLevelNames[PSL_ERROR] = string("ERROR");
  m_pLevelNames[PSL_WARNING] = string("WARNING");
  m_pLevelNames[PSL_INFO] = string("INFO");
  m_pLevelNames[PSL_DEBUG] = string("DEBUG");

  for (int i = 0; i < 5; i++)
  {
    m_oNameMap[(m_pLevelNames[i])] = i;
  }

  setlogmask(LOG_UPTO(LOG_NOTICE));
  openlog("vantaged", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

PsLogger::~PsLogger()
{
  close();
  closelog();

  delete[] m_pLevelNames;
}

void PsLogger::close()
{
  if (NULL != m_pFile && stdout != m_pFile)
  {
    fclose(m_pFile);
    m_pFile = NULL;
  }
}

int PsLogger::getLevel()
{
  return m_iLevel;
}

void PsLogger::setLevel(int p_iLevel)
{
  m_iLevel = p_iLevel;
}

const std::string &PsLogger::getFileName()
{
  return m_sFile;
}

bool PsLogger::setFileName(const char *p_szFile)
{
  bool bRet = false;

  if (NULL != p_szFile && '\0' != p_szFile[0])
  {
    if (NULL != m_pFile && stdout != m_pFile)
    {
      fclose(m_pFile);
      m_pFile = NULL;
    }

    m_sFile = p_szFile;

    m_pFile = fopen(m_sFile.c_str(), "w");
    if (NULL == m_pFile)
    {
#ifdef _PS_DEBUG
      fprintf(stderr, "%s [%d]: Unable to open file '%s': %s\n",
              __FILE__,
              __LINE__,
              m_sFile.c_str(),
              strerror(errno));
#endif
      m_pFile = stdout;
      m_sFile = "";
    }
    else
    {
      bRet = true;
    }
  }

  return bRet;
}

inline FILE *PsLogger::getFile()
{
  return m_pFile;
}

inline std::string PsLogger::levelToStr(int p_iLevel)
{
  string sRet;
  if (p_iLevel <= PSL_DEBUG)
  {
    sRet = m_pLevelNames[p_iLevel];
  }

  return sRet;
}

int PsLogger::strToLevel(std::string &p_sLevel)
{
  int iRet = -1;
  LogLevelIter_t tIter = m_oNameMap.find(p_sLevel);
  if (m_oNameMap.end() != tIter)
  {
    iRet = tIter->second;
  }

  return iRet;
}

PsLogger &PsLogger::getInstance()
{
  return s_oInstance;
}

void PsLogger::log(const char *p_szFile, int p_iLine, int p_iLevel, const char * p_szFmt, ...)
{
  va_list tBlah;
  va_start(tBlah, p_szFmt);

  PsLogger &oLogger = getInstance();
  int iLevel = oLogger.getLevel();
  FILE *pFile = oLogger.getFile();

  if (p_iLevel <= iLevel && NULL != pFile)
  {
    time_t tNow = time(NULL);
    ostringstream oSS;
    oSS << (int) tNow
         << " "
         << p_szFile
         << " ["
         << p_iLine
         << "]: "
         << oLogger.levelToStr(p_iLevel)
         << ": ";
    fprintf(pFile, "%s", oSS.str().c_str());
    vfprintf(pFile, p_szFmt, tBlah);
fflush(pFile);
  }
}

void PsLogger::sysLog(const char *p_szFile, int p_iLine, const char * p_szFmt, ...)
{
  va_list tBlah;
  va_start(tBlah, p_szFmt);

  ostringstream oSS;
  oSS << p_szFile
      << " ["
      << p_iLine
      << "]: "
      << p_szFmt;
  vsyslog(LOG_NOTICE, oSS.str().c_str(), tBlah);
}

