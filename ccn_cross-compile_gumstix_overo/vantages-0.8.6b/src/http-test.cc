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
#include <sys/types.h>

#include <iostream>
#include <sstream>

#include "http_listener.h"
#include "ps_defs.h"
#include "ps_util.h"
#include "dnskey_proc_dao.h"
#include "dao_factory.h"

extern "C"
{
#include "shttpd.h"
}

using namespace std;

void handleRequests(struct shttpd_arg *p_pArg)
{
  if (NULL == p_pArg)
  {
    ps_elog(PSL_CRITICAL, "Got NULL pointer in handleRequests()\n");
  }
  else
  {
    HttpListener *pListener = (HttpListener *) p_pArg->user_data;
    const char *szRequestMethod = shttpd_get_env(p_pArg, "REQUEST_METHOD");
    char *szZoneList = NULL;
    int iContentLen = 0;

    if (0 == strncmp(szRequestMethod, "POST", 4))
    {
      const char *szContentLen = shttpd_get_header(p_pArg, "Content-Length");
      iContentLen = (int) strtol(szContentLen, NULL, 10);
      iContentLen = (iContentLen > PS_MAX_HTTP_QUERY_LEN) ? PS_MAX_HTTP_QUERY_LEN : iContentLen;

      pListener->appendInput(p_pArg->in.buf, p_pArg->in.len);
      p_pArg->in.num_bytes = p_pArg->in.len;

      if (!(p_pArg->flags & SHTTPD_MORE_POST_DATA))
      {
        string &sInput = pListener->getInput();
        szZoneList = new char[iContentLen + 1];
        memset(szZoneList, 0, iContentLen + 1);
		    shttpd_get_var(PS_HTTP_ZONE_LIST_KEY,
                       sInput.c_str(),
                       sInput.size(),
		                   szZoneList,
                       iContentLen);
      }
    }
    else if (0 == strncmp(szRequestMethod, "GET", 3))
    {
      const char *szQueryString = shttpd_get_env(p_pArg, "QUERY_STRING");
      iContentLen = _strnlen(szQueryString, PS_MAX_HTTP_QUERY_LEN);
      szZoneList = new char[iContentLen + 1];
      memset(szZoneList, 0, iContentLen + 1);

		  shttpd_get_var(PS_HTTP_ZONE_LIST_KEY,
                     szQueryString,
                     iContentLen,
		                 szZoneList,
                     iContentLen);
    }
    else
    {
      ps_elog(PSL_CRITICAL, "Unknown request method received: %s\n", szRequestMethod);
    }

    if (NULL != szZoneList)
    {
      try
      {
        if (!pListener->sendingResponse())
        {
          pListener->sendingResponse(true);

//          string sHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
          string sHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 6000000\r\n\r\n";
          pListener->appendOutput(sHeader);
          for (size_t u1 = 0;
               u1 < 1000000;
               u1++)
          {
            string sRet = "12345";;
            pListener->appendOutput(sRet);
            pListener->appendOutput("\n");
          }
        }

        string &sOutput = pListener->getOutput();
        size_t uLen = sOutput.size();
        size_t u = 0;
        for (u = pListener->getOutputOffset();
             u < uLen;
            )
        {
          size_t uTmp = shttpd_printf(p_pArg, "%s", &(sOutput.c_str()[u]));
          u += uTmp;
          pListener->setOutputOffset(u);
ps_elog(PSL_CRITICAL, "num_bytes = %d len is %d uTmp is %u (%u bytes left)\n", p_pArg->out.num_bytes, p_pArg->out.len, (unsigned) uTmp, (unsigned) (uLen - u));
          if (0 == uTmp)
          {
            break;
          }
        }

        if (pListener->isOutputDone())
        {
      p_pArg->flags |= SHTTPD_END_OF_OUTPUT;
      pListener->clearOutput();
        }

      }
      catch (...)
      {
        ps_elog(PSL_CRITICAL, "Caught Exception.\n");
      }

      delete[] szZoneList;
      szZoneList = NULL;
      pListener->clearInput();
    }
  }
}

HttpListener::HttpListener()
  : m_bRun(false),
    m_iPort(PS_DEFAULT_HTTP_PORT),
    m_pCtx(NULL),
    m_uOutputOffset(0),
    m_bSending(false)
{

}

HttpListener::~HttpListener()
{

}

int HttpListener::getPort()
{
  return m_iPort;
}

bool HttpListener::init(int p_iPort)
{
  char szPort[11];
  memset(szPort, 0, 11);
  sprintf(szPort, "%d", m_iPort);

  m_iPort = p_iPort;
  return init(szPort);
}

bool HttpListener::init(const char *p_szPort)
{
  return init((char *) p_szPort);
}

bool HttpListener::init(char *p_szPort)
{
  bool bRet = false;

  if (NULL == p_szPort)
  {
    ps_elog(PSL_CRITICAL, "Unable to init w/ NULL port.\n");
  }
  else
  {
    m_iPort = (int) strtol(p_szPort, NULL, 10);
    int argc = 4;
    char *argv[4] = { "shttpd", "-ports", p_szPort, NULL };
    m_pCtx = shttpd_init(argc, argv);
    if (NULL == m_pCtx)
    {
      ps_elog(PSL_CRITICAL, "Unable to init HTTPD subsystem.\n");
    }
    else
    {
      shttpd_set_option(m_pCtx, "ports", p_szPort);

      shttpd_register_uri(m_pCtx, "/", &handleRequests, this);
      m_bRun = true;
      bRet = true;
    }
  }

  return bRet;
}

bool HttpListener::listen()
{
  bool bRet = false;

  while (m_bRun)
  {
    shttpd_poll(m_pCtx, 1000);
  }

  return bRet;
}

void HttpListener::kill()
{
  m_bRun = false;
}

void HttpListener::clearInput()
{
  m_sInput.clear();
}

void HttpListener::appendInput(char *p_pBuff, int p_iLen)
{
  if (NULL != p_pBuff)
  {
    m_sInput.append(p_pBuff, p_iLen);
  }
}

std::string &HttpListener::getInput()
{
  return m_sInput;
}

void HttpListener::clearOutput()
{
  m_sOutput.clear();
  m_uOutputOffset = 0;
  sendingResponse(false);
}

void HttpListener::appendOutput(std::string &p_sOutput)
{
  m_sOutput.append(p_sOutput);
}

void HttpListener::appendOutput(char *p_szOutput)
{
  string sOut = p_szOutput;
  m_sOutput.append(sOut);
}

std::string &HttpListener::getOutput()
{
  return m_sOutput;
}

size_t HttpListener::getOutputOffset()
{
  return m_uOutputOffset;
}

void HttpListener::setOutputOffset(size_t p_uOffset)
{
  m_uOutputOffset = p_uOffset;
}

bool HttpListener::isOutputDone()
{
  return m_sOutput.length() == m_uOutputOffset;
}

void HttpListener::sendingResponse(bool p_bSending)
{
  m_bSending = p_bSending;
}

bool HttpListener::sendingResponse()
{
  return m_bSending;
}

int main(int argc, char *argv[])
{
  int iRet = 0;

  HttpListener oList;
  oList.init("8765");
  oList.listen();

  return iRet;
}
