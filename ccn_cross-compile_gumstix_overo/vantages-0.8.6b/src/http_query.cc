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

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include "http_query.h"
#include "ps_config.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

static size_t _writeCallback(void *p_pNewData, size_t p_uSize, size_t p_uNmemb, void *p_pQuery)
{
  size_t uRealSize = p_uSize * p_uNmemb;
  HttpQuery *pQuery = (HttpQuery *) p_pQuery;
  if (NULL == pQuery)
  {
    ps_elog(PSL_CRITICAL, "Got NULL query object in callback!\n");
  }
  else
  {
    string sChunk((char *) p_pNewData, uRealSize);
    pQuery->addChunk(sChunk);
  }

  return uRealSize;
}

HttpQuery::HttpQuery()
  : m_bAuth(false),
    m_pHandle(NULL),
    m_pFormPost(NULL),
    m_pFormLast(NULL)
{
  m_pHandle = curl_easy_init();
  curl_easy_setopt(m_pHandle, CURLOPT_WRITEFUNCTION, _writeCallback);
  curl_easy_setopt(m_pHandle, CURLOPT_WRITEDATA, (void *)this);
  curl_easy_setopt(m_pHandle, CURLOPT_USERAGENT, "vantaged/1.0");
  curl_easy_setopt(m_pHandle, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(m_pHandle, CURLOPT_TIMEOUT, PS_HTTP_MAX_TIMEOUT_SECONDS);

  const char *szCaFile = PsConfig::getInstance().getValue(PS_CONF_CURL_CA_FILE);
  if (NULL != szCaFile)
  {
    ps_elog(PSL_DEBUG, "Setting CA file: '%s'\n", szCaFile);
    curl_easy_setopt(m_pHandle, CURLOPT_CAINFO, szCaFile);
  }

//  curl_easy_setopt(m_pHandle, CURLOPT_VERBOSE, 1);
}

HttpQuery::~HttpQuery()
{
  m_sPostData = "";
  curl_easy_cleanup(m_pHandle);
}

void HttpQuery::setPostData(std::string &p_sName, std::string &p_sData)
{
  if (m_sPostData.length() > 0)
  {
    m_sPostData += "&";
  }
  m_sPostData += p_sName + "=" + p_sData;
}

bool HttpQuery::query(std::string &p_sQuery)
{
  bool bRet = false;

  m_oSS.clear();
  m_sResp.clear();

  struct curl_slist *pHdrList = NULL;
  m_sQuery = p_sQuery;
  curl_easy_setopt(m_pHandle, CURLOPT_URL, p_sQuery.c_str());

  if (m_sPostData.length() > 0)
  {
    curl_easy_setopt(m_pHandle, CURLOPT_POSTFIELDS, m_sPostData.c_str());
    pHdrList = curl_slist_append(pHdrList, "Expect: 100-continue");
    curl_easy_setopt(m_pHandle, CURLOPT_HTTPHEADER, pHdrList);
    curl_easy_setopt(m_pHandle, CURLOPT_POST, 1);
  }

  if (getAuth())
  {
    string sAuth = getUser();
    sAuth += ":";
    sAuth += getPass();
    curl_easy_setopt(m_pHandle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    curl_easy_setopt(m_pHandle, CURLOPT_USERPWD, sAuth.c_str());
  }

  CURLcode tRes = curl_easy_perform(m_pHandle);
  if (CURLE_OK != tRes)
  {
    ps_elog(PSL_ERROR, "Error executing query [%d]: '%s'\n", (int) tRes, curl_easy_strerror(tRes));
  }
  else
  {
    bRet = true;
  }

  if (NULL != m_pFormPost)
  {
    curl_formfree(m_pFormPost);
    m_pFormPost = NULL;
    m_pFormLast = NULL;
  }

  if (NULL != pHdrList)
  {
    curl_slist_free_all(pHdrList);
    pHdrList = NULL;
  }

  m_sResp = m_oSS.str();

  return bRet;
}

void HttpQuery::addChunk(std::string &p_sChunk)
{
  m_oSS << p_sChunk.c_str();
}

std::string &HttpQuery::getResponse()
{
  return m_sResp;
}

std::string &HttpQuery::getUser()
{
  return m_sUser;
}

std::string &HttpQuery::getPass()
{
  return m_sPass;
}

void HttpQuery::setUserPass(std::string &p_sUser, std::string &p_sPass)
{
  m_sUser = p_sUser;
  m_sPass = p_sPass;

  if (m_sUser != "" && m_sPass != "")
  {
    setAuth(true);
  }
}

bool HttpQuery::getAuth()
{
  return m_bAuth;
}

void HttpQuery::setAuth(bool p_bAuth)
{
  m_bAuth = p_bAuth;
}

std::string HttpQuery::urlDecode(std::string &p_sInput)
{
  string sRet = p_sInput;
  for (size_t u = 0; u < p_sInput.size(); u++)
  {
    if (sRet[u] == '+')
    {
      sRet[u] = ' ';
    }
    else if (sRet[u] == '%' && u + 2 < p_sInput.size())
    {
      char szTmp[3];
      szTmp[0] = sRet[u + 1];
      szTmp[1] = sRet[u + 2];
      szTmp[2] = '\0';
      int iTmp = (int) strtol(szTmp, NULL, 16);
      szTmp[0] = (char) iTmp;
      szTmp[1] = '\0';
      sRet.replace(u, 3, szTmp);
    }
  }

/*
//  char *szRet = curl_easy_unescape(NULL, sRet.c_str(), sRet.size(), &iLen);
  char *szRet = curl_unescape(sRet.c_str(), sRet.size());
  sRet = szRet;
  curl_free(szRet);
*/

  return sRet;
}
