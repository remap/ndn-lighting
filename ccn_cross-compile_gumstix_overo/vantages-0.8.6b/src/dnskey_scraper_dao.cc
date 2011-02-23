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

#include <string>
#include <list>
#include <iostream>
#include <sstream>

#include "dnskey_scraper_dao.h"

using namespace std;

const char *DnskeyScraperDao::s_kszDaoName = PS_DNSKEY_SCRAPER_NAME;

DnskeyScraperDao::DnskeyScraperDao()
{
  setID(2);
}

DnskeyScraperDao::~DnskeyScraperDao()
{

}

std::string DnskeyScraperDao::daoName()
{
  return PS_DNSKEY_SCRAPER_NAME;
}

DnskeyScraperDao *DnskeyScraperDao::dup()
{
  return new DnskeyScraperDao();
}

bool DnskeyScraperDao::execute()
{
  bool bRet = false;

  char *pWriteData = getData();
  size_t uLen = getDataLen();
  string sLine;
  string sData;
  string sOut;
  list<string> oList;
  sData.assign(pWriteData, uLen);
  istringstream oISS(sData);
  bRet = true;
  while (bRet && getline(oISS, sLine) && sLine.size() > 0)
  {
    size_t uDelim = 0;
    uDelim = sLine.find("DNSKEY");
    if (string::npos == uDelim)
    {
      ps_elog(PSL_DEBUG, "This line is not a DNSKEY, skipping...\n");
      continue;
    }

    for (int i = 0; string::npos != uDelim && i < 4; i++)
    {
      uDelim = sLine.find(" ");
      if (string::npos == uDelim)
      {
        ps_elog(PSL_CRITICAL, "Unable to find leading token number %d in string '%s'\n", i, sLine.c_str());
      }
      else
      {
        sLine.erase(0, uDelim + 1);
      }
    }

    if (string::npos == uDelim)
    {
      ps_elog(PSL_CRITICAL, "Problems above.\n");
      bRet = false;
    }
    else
    {
      if (string::npos != (uDelim = sLine.find("(")))
      {
        sLine.erase(uDelim, 1);
      }

      size_t uLast = sLine.find(")");
      if (string::npos != uLast)
      {
        sLine.erase(uLast);
      }

      oList.push_back(sLine);
    }
  }
  oList.sort();
  for (list<string>::iterator tIter = oList.begin();
       oList.end() != tIter;
       tIter++)
  {
    sOut += (*tIter) + "\n";
  }
  setResult(sOut);

  return bRet;
}
