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

#include <algorithm>

#include "ps_url_parser.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

PsUrlParser::PsUrlParser()
  : m_bValid(false)
{

}

PsUrlParser::~PsUrlParser()
{

}

void PsUrlParser::reset()
{
  m_sURL.clear();
  m_sProto.clear();
  m_sServer.clear();
  m_sQuery.clear();
  m_sParams.clear();
  m_oParamMap.clear();
  m_bValid = false;
}

bool PsUrlParser::parse(std::string &p_sURL)
{
  bool bRet = false;

  reset();
  try
  {
    m_sURL = p_sURL;
    string sURL = p_sURL;
//    transform(sURL.begin(), sURL.end(), sURL.begin(), ::tolower);
    size_t uIdx = string::npos;
    if ((uIdx = sURL.find(":")) == string::npos)
    {
      ps_elog(PSL_CRITICAL, "URL: '%s' is not in recognizable format\n", sURL.c_str());
    }
    else
    {
      m_sProto = sURL.substr(0, uIdx);
      transform(m_sProto.begin(), m_sProto.end(), m_sProto.begin(), ::tolower);
      sURL.erase(0, uIdx + 1);
      // Is there an auth server?
      if (0 == sURL.find("//"))
      {
        sURL.erase(0, 2);
        size_t uTerm = sURL.find("/");
        if (string::npos == uTerm)
        {
          ps_elog(PSL_CRITICAL, "Cannot find auth term, URL: '%s'\n", p_sURL.c_str());
        }
        else
        {
          m_sServer = sURL.substr(0, uTerm);
          transform(m_sServer.begin(), m_sServer.end(), m_sServer.begin(), ::tolower);
//          m_sServer.assign(sURL, 0, uTerm);
          sURL.erase(0, uTerm + 1);
        }
      }

      size_t uPos = string::npos;
      // Are there parameters?
      if (string::npos != (uPos = sURL.find("?")))
      {
        m_sParams = sURL.substr(uPos + 1);
//        m_sParams.assign(sURL, uPos + 1);
        string sParams = m_sParams;
        size_t u = 0;
        do {
          u = sParams.find("&");
          string s = sParams.substr(0, u);
          size_t uEq = s.find("=");
          if (string::npos == uEq)
          {
            ps_elog(PSL_CRITICAL, "Malformed parameter: '%s'\n", s.c_str());
          }
          else
          {
            string sKey = s.substr(0, uEq);
            if (m_sProto == "dns" || m_sProto == "dnssec")
            {
              transform(sKey.begin(), sKey.end(), sKey.begin(), ::tolower);
            }
            m_oParamMap[sKey] = s.substr(uEq + 1);
//ps_elog(PSL_CRITICAL, "'%s' -> '%s'\n", sKey.c_str(), s.substr(uEq + 1).c_str());
          }
          sParams.erase(0, u + 1);
        } while (string::npos != u);

        sURL.erase(uPos);
      }

      m_sQuery = sURL;
      bRet = true;
    }
  }
  catch (...)
  {
    ps_elog(PSL_CRITICAL, "Caught exception for URL: '%s'\n", p_sURL.c_str());
    bRet = false;
  }

  return bRet;
}

bool PsUrlParser::isValid()
{
  return m_bValid;
}

std::string &PsUrlParser::getURL()
{
  return m_sURL;
}

std::string &PsUrlParser::getProto()
{
  return m_sProto;
}

std::string &PsUrlParser::getServer()
{
  return m_sServer;
}

std::string &PsUrlParser::getQuery()
{
  return m_sQuery;
}

std::string &PsUrlParser::getParams()
{
  return m_sParams;
}

bool PsUrlParser::getParam(std::string p_sKey, std::string &p_sOutVal)
{
  bool bRet = false;

  transform(p_sKey.begin(), p_sKey.end(), p_sKey.begin(), ::tolower);
  ParamIter_t tIter = m_oParamMap.find(p_sKey);
  if (m_oParamMap.end() != tIter)
  {
    p_sOutVal = tIter->second;
    bRet = true;
  }

  return bRet;
}

