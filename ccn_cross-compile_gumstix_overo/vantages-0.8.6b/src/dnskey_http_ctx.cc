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

#include "dnskey_http_ctx.h"
#include "ps_util.h"
#include "dnskey_proc_dao.h"
#include "dao_factory.h"
#include "ps_url_parser.h"
#include "dnskey_defs.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

DnskeyHttpCtx::DnskeyHttpCtx()
{

}

DnskeyHttpCtx::~DnskeyHttpCtx()
{

}

bool DnskeyHttpCtx::process()
{
  bool bRet = false;

  clearOutput();
  list<string> &oOutList = getOutput();
  string sInput;
  string sRawInput = "http://sdsdsd/?";
  sRawInput += getInput();
  PsUrlParser oParser;

  ps_elog(PSL_DEBUG, "Got Cxn...\n");

  if (!oParser.parse(sRawInput))
  {
    ps_elog(PSL_CRITICAL, "Unable to parse input: '%s'\n", sRawInput.c_str());
  }
  else if (!oParser.getParam(PS_HTTP_ZONE_LIST_KEY, sInput))
  {
    ps_elog(PSL_CRITICAL, "Unable to locate key: '%s' in '%s'\n", PS_HTTP_ZONE_LIST_KEY, sRawInput.c_str());
  }
  else if (!sInput.empty())
  {
    time_t tConstraint = 0;
    string sConstraint;
    if (oParser.getParam(DNSKEY_HTTP_MIN_TIME_KEY, sConstraint))
    {
      tConstraint = (int) strtol(sConstraint.c_str(), NULL, 10);
      ps_elog(PSL_INFO, "Got constraint time: %d\n", (int) tConstraint);
    }

    float fVersion = 0.0;
    string sVersion;
    if (oParser.getParam(DNSKEY_HTTP_VERSION, sVersion))
    {
      fVersion = (float) strtof(sVersion.c_str(), NULL);
      ps_elog(PSL_INFO, "Got version: %f\n", fVersion);
    }
    else
    {
      ps_elog(PSL_INFO, "No version specified, defaulting to: %f\n", fVersion);
    }

    DaoList_t tList;
    DnskeyProcDao *pQueryDao = NULL;

    try
    {
int iZoneCount = 0;
      pQueryDao = static_cast<DnskeyProcDao *>(DaoFactory::getInstance().create(DnskeyProcDao::s_kszDaoName));
      pQueryDao->setScraper(NULL);
      pQueryDao->setScraperID(-1);
      pQueryDao->setDate(tConstraint);

      int iLen = sInput.size();
      int iLast = 0;

      if (fVersion > 0.0)
      {
        string sVer = "|" + sVersion + "\n";
        oOutList.push_back(sVer);
      }

//ostringstream oSS;
string sLastZone;
      for (int i = 0; i < iLen; i++)
      {
        string s;
        if (',' == sInput[i])
        {
iZoneCount++;
          s.assign(&(sInput[iLast]), i - iLast);
          i++;
          iLast = i;
        }
        else if ((i + 1) == iLen)
        {
iZoneCount++;
          s.assign(&(sInput[iLast]), i - iLast + 1);
          iLast = i;
        }

        if (s.length() > 0)
        {
//ps_elog(PSL_CRITICAL, "GOT: '%s'\n", s.c_str());
          pQueryDao->setName(s);
          if (!pQueryDao->deserialize(tList))
          {
            ps_elog(PSL_CRITICAL, "Unable to deserialize for zone: \'%s\'\n", s.c_str());
          }
          else
          {
            time_t tMax = 0;
            DnskeyProcDao *pDao = NULL;
            for (DaoIter_t tIter = tList.begin();
                 tList.end() != tIter;
                 tIter++)
            {
              DnskeyProcDao *pTmpDao = (DnskeyProcDao *)(*tIter);

              if (pTmpDao->getDate() > tMax)
              {
                tMax = pTmpDao->getDate();
                pDao = pTmpDao;
                ps_elog(PSL_INFO, "For zone '%s', using timestmap: %d\n", s.c_str(), tMax);
              }
            }
            if (NULL != pDao)
            {
              string sRet;

              if (0.0 == fVersion)
              {
                pDao->setDate(0);
              }

              pDao->toString(sRet);
              sRet += "\n";
              oOutList.push_back(sRet);
// ps_elog(PSL_CRITICAL, "PUSHING '%s'\n", sRet.c_str());
              sLastZone = sRet;
            }
          }

          PsDao::clearList(tList);
        }
      }

      size_t uCLen = 0;
      for (list<string>::iterator tIter = oOutList.begin();
           oOutList.end() != tIter;
           tIter++)
      {
        uCLen += (*tIter).size();
      }

      char szLen[11];
      memset(szLen, 0, 11);
      sprintf(szLen, "%u", (unsigned) uCLen);
      string sHdr = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
      sHdr += szLen;
      sHdr += "\r\n\r\n";
      oOutList.push_front(sHdr);
      bRet = true;
    }
    catch (...)
    {
      ps_elog(PSL_CRITICAL, "Caught Exception.\n");
      bRet = false;
    }
    PsDao::clearList(tList);
    if (NULL != pQueryDao)
    {
      delete pQueryDao;
      pQueryDao = NULL;
    }
  }

  return bRet;
}

