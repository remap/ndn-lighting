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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include <string>

#include "perl_scraper.h"

#define _READ_BUFF 4096

using namespace std;

const char *PerlScraper::s_kszDaoName = PS_PERL_SCRAPER_NAME;

PerlScraper::PerlScraper()
{

}

PerlScraper::~PerlScraper()
{

}

std::string PerlScraper::daoName()
{
  return PS_PERL_SCRAPER_NAME;
}

PsDao *PerlScraper::dup()
{
  return new PerlScraper();
}

bool PerlScraper::execute()
{
  bool bRet = false;

  int pToPerlFDs[2];
  int pFromPerlFDs[2];

  pid_t tID = 0;
  if (-1 == pipe(pToPerlFDs))
  {
    ps_elog(PSL_CRITICAL, "Unable to open new pipe to perl: %s\n", strerror(errno));
  }
  else if (-1 == pipe(pFromPerlFDs))
  {
    ps_elog(PSL_CRITICAL, "Unable to open new pipe from perl: %s\n", strerror(errno));
  }
  else if ((tID = fork()) < 0)
  {
    ps_elog(PSL_CRITICAL, "Unable to fork: %s\n", strerror(errno));
  }
  else if (0 == tID)
  {
    char *pArgs[5];
    int iLen = 4;
    string &sParam = getParam();
    if (sParam != "")
    {
      iLen++;
    }
    pArgs[0] = (char *) "perl";
    pArgs[1] = (char *) "-e";
    pArgs[2] = getScraperBin();
    pArgs[3] = NULL;
    if (iLen > 4)
    {
      pArgs[3] = (char *) sParam.c_str();
      pArgs[4] = NULL;
    }

    close(pFromPerlFDs[0]);
    close(pToPerlFDs[1]);
    if (-1 == dup2(pToPerlFDs[0], 0))
    {
      ps_elog(PSL_CRITICAL, "Unable to dup2: %s\n", strerror(errno));
    }
    else if (-1 == dup2(pFromPerlFDs[1], 1))
    {
      ps_elog(PSL_CRITICAL, "Unable to dup2: %s\n", strerror(errno));
    }
    else
    {
      if (PsLogger::getInstance().getLevel() >= PSL_DEBUG)
      {
        fprintf(stderr, "Exec'ing with args: ");
        for (int i = 0; i < 5; i++)
        {
          if (NULL == pArgs[i])
          {
            break;
          }
          fprintf(stderr, "'%s' ", pArgs[i]);
        }
        fprintf(stderr, "\n");
      }

      execvp("perl", pArgs);
      ps_elog(PSL_CRITICAL, "Unable to exec: %s\n", strerror(errno));
    }
  }
  else
  {
    close(pFromPerlFDs[1]);
    close(pToPerlFDs[0]);
    char pBuff[_READ_BUFF + 1];

    size_t uWrite = 0;
    int iCount = 0;
    char *pWriteData = getData();
    size_t uLen = getDataLen();
    while (0 != (iCount = write(pToPerlFDs[1], pWriteData, uLen)) && uWrite < uLen)
    {
      if (iCount > 0)
      {
        uLen -= iCount;
        pWriteData += iCount;
      }
      else if (EINTR != errno)
      {
        ps_elog(PSL_CRITICAL, "Unable to write: %s\n", strerror(errno));
        break;
      }
    }
    close(pToPerlFDs[1]);

    memset(pBuff, 0, _READ_BUFF + 1);
    int iRead = 0;
    string sOut;
    while ((iRead = read(pFromPerlFDs[0], pBuff, _READ_BUFF)) > 0)
    {
      if (iRead <= 0
          && EINTR != errno)
      {
        ps_elog(PSL_CRITICAL, "Error reading from pipe: %s\n", strerror(errno));
        break;
      }
      else
      {
        pBuff[iRead] = '\0';
        sOut += pBuff;
      }
    }

    int iStatus = 0;
    pid_t tRet = 0;
    while (-1 != (tRet = wait(&iStatus))
           && tRet != tID);
    if (-1 == tRet || 0 != iStatus)
    {
      ps_elog(PSL_CRITICAL, "perl exited with errors: [%d] %s\n", iStatus, strerror(errno));
      bRet = false;
    }
    else
    {
      setResult(sOut);
      bRet = true;
    }

    close(pFromPerlFDs[0]);
  }

  return bRet;
}
