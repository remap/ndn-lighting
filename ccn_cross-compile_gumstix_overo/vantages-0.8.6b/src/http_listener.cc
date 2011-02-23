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
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <sstream>

#include "http_listener.h"
#include "ps_defs.h"
#include "ps_util.h"
#include "dnskey_proc_dao.h"
#include "dao_factory.h"
#include "http_list_ctx.h"
#include "ps_app.h"
#include "ps_logger.h"

extern "C"
{
#include "shttpd.h"
}

using namespace std;

void _defaultEmpty(struct shttpd_arg *arg)
{
  shttpd_printf(arg, "%s", "HTTP/1.1 404 OK\r\n");
  shttpd_printf(arg, "%s", "Content-Type: text/html\r\n\r\n");
  shttpd_printf(arg, "%s", "<html><body>");
  shttpd_printf(arg, "%s", "<p>404 Page not found.</body></html>");
  arg->flags |= SHTTPD_END_OF_OUTPUT;
}


void _handleRequests(struct shttpd_arg *p_pArg)
{
  if (NULL == p_pArg)
  {
    ps_elog(PSL_CRITICAL, "Got NULL pointer in handleRequests()\n");
  }
  else if (NULL == p_pArg->user_data)
  {
    ps_elog(PSL_CRITICAL, "Got NULL user data (no application).\n");
  }
  else
  {
    PsApp *pApp = (PsApp *) p_pArg->user_data;

    HttpListenerCtx *pCtx = (HttpListenerCtx *) p_pArg->state;
    if (NULL == pCtx)
    {
      pCtx = pApp->createCtx();
      p_pArg->state = pCtx;
    }

    const char *szRequestMethod = shttpd_get_env(p_pArg, "REQUEST_METHOD");

    if (!pCtx->sendingResponse())
    {
      if (0 == strncmp(szRequestMethod, "POST", 4))
      {
        const char *szContentLen = shttpd_get_header(p_pArg, "Content-Length");
        int iContentLen = (int) strtol(szContentLen, NULL, 10);
        iContentLen = (iContentLen > PS_MAX_HTTP_QUERY_LEN) ? PS_MAX_HTTP_QUERY_LEN : iContentLen;

        pCtx->appendInput(p_pArg->in.buf, p_pArg->in.len);
        p_pArg->in.num_bytes = p_pArg->in.len;

        if (!(p_pArg->flags & SHTTPD_MORE_POST_DATA))
        {
          pCtx->setReadyToSend(true);
        }
      }
      else if (0 == strncmp(szRequestMethod, "GET", 3))
      {
        const char *szQueryString = shttpd_get_env(p_pArg, "QUERY_STRING");
        pCtx->appendInput((char *) szQueryString, _strnlen(szQueryString, PS_MAX_HTTP_QUERY_LEN));
        pCtx->setReadyToSend(true);
      }
      else
      {
        ps_elog(PSL_CRITICAL, "Unknown request method received: %s\n", szRequestMethod);
      }
    }

    if (NULL != pCtx
        && !pCtx->sendingResponse()
        && pCtx->readyToSend())
    {
      p_pArg->state = pCtx;
      if (!pCtx->process())
      {
        ps_elog(PSL_CRITICAL, "Unable to process listener context.\n");
        pCtx->setError(true);
      }
      else
      {
        pCtx->sendingResponse(true);
      }
    }

    if (NULL != pCtx
        && pCtx->sendingResponse())
    {
      list<string> &oOutList = pCtx->getOutput();
      while (!oOutList.empty())
      {
        string sHead = oOutList.front();
        size_t uHeadLen = sHead.size();
        if ((p_pArg->out.len - p_pArg->out.num_bytes) < (int) (uHeadLen + 1))
        {
          break;
        }
        oOutList.pop_front();

        size_t uTmp = shttpd_printf(p_pArg, "%s", sHead.c_str());
        if (uTmp < uHeadLen)
        {
          string sTmp = sHead.substr(uTmp);
          oOutList.push_front(sTmp);
          break;
        }
      }
    }

    if (pCtx->done() || pCtx->error())
    {
      p_pArg->flags |= SHTTPD_END_OF_OUTPUT;

      if (NULL != pCtx)
      {
        delete pCtx;
        pCtx = NULL;
      }
      p_pArg->state = NULL;
    }
  }
}

/*
void _admin(struct shttpd_arg *p_pArg)
{
  HttpAdminCtx *pCtx = (HttpAdminCtx *) p_pArg->state;
  if (NULL == pCtx)
  {
    pCtx = new HttpAdminCtx();
    p_pArg->state = pCtx;
  }

  const char *szRequestMethod = shttpd_get_env(p_pArg, "REQUEST_METHOD");
  if (0 == strncmp(szRequestMethod, "POST", 4))
  {
    const char *szContentLen = shttpd_get_header(p_pArg, "Content-Length");
    int iContentLen = (int) strtol(szContentLen, NULL, 10);
    iContentLen = (iContentLen > PS_MAX_HTTP_QUERY_LEN) ? PS_MAX_HTTP_QUERY_LEN : iContentLen;

    pCtx->appendInput(p_pArg->in.buf, p_pArg->in.len);
    p_pArg->in.num_bytes = p_pArg->in.len;

    if (!(p_pArg->flags & SHTTPD_MORE_POST_DATA))
    {
      pCtx->setReadyToSend(true);
    }
  }
  else if (0 == strncmp(szRequestMethod, "GET", 3))
  {
    const char *szQueryString = shttpd_get_env(p_pArg, "QUERY_STRING");
    pCtx->appendInput((char *) szQueryString, _strnlen(szQueryString, PS_MAX_HTTP_QUERY_LEN));
    pCtx->setReadyToSend(true);
  }
  else
  {
    ps_elog(PSL_CRITICAL, "Unrecognized method type: %s\n", szRequestMethod);
  }

  if (!pCtx->sendingResponse()
      && pCtx->readyToSend())
  {
    if (!pCtx->process())
    {
      ps_elog(PSL_CRITICAL, "Unable to process admin task.\n");
      pCtx->setError(true);
    }
    else
    {
      pCtx->sendingResponse(true);
    }
  }

  if (!pCtx->error()
      && pCtx->sendingResponse()
      && !pCtx->done())
  {
    list<string> &oOutList = pCtx->getOutput();
    while (!oOutList.empty())
    {
      string sHead = oOutList.front();
      size_t uHeadLen = sHead.size();
      if ((p_pArg->out.len - p_pArg->out.num_bytes) < (int) (uHeadLen + 1))
      {
        break;
      }
      oOutList.pop_front();

      size_t uTmp = shttpd_printf(p_pArg, "%s", sHead.c_str());
      if (uTmp < uHeadLen)
      {
        string sTmp = sHead.substr(uTmp);
        oOutList.push_front(sTmp);
        break;
      }
    }
  }

  if (pCtx->done() || pCtx->error())
  {
    p_pArg->flags |= SHTTPD_END_OF_OUTPUT;

    if (NULL != pCtx)
    {
      delete pCtx;
      pCtx = NULL;
    }
    p_pArg->state = NULL;
  }
}
*/

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

bool HttpListener::init(const char *p_szPort, AppList_t &p_oAppList)
{
  return init((char *) p_szPort, p_oAppList);
}

bool HttpListener::init(char *p_szPort, AppList_t &p_oAppList)
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
    char *argv[4] = { (char *) "shttpd", (char *) "-ports", p_szPort, NULL };
    m_pCtx = shttpd_init(argc, argv);
    if (NULL == m_pCtx)
    {
      ps_elog(PSL_CRITICAL, "Unable to init HTTPD subsystem.\n");
    }
    else
    {
      shttpd_set_option(m_pCtx, "ports", p_szPort);

      shttpd_register_uri(m_pCtx, "/", &_defaultEmpty, NULL);
      for (AppIter_t tIter = p_oAppList.begin();
           p_oAppList.end() != tIter;
           tIter++)
      {
        PsApp *pApp = *tIter;
        const char *szPassFile = pApp->getHttpPass();
        shttpd_register_uri(m_pCtx, pApp->getHttpPath(), &_handleRequests, pApp);
        if (NULL != szPassFile)
        {
          string sNewURI = pApp->getHttpPath();
          sNewURI += "=";
          sNewURI += szPassFile;
          ps_elog(PSL_DEBUG, "Setting password protection for: '%s'\n", sNewURI.c_str());
          shttpd_set_option(m_pCtx, "protect", sNewURI.c_str());
          shttpd_set_option(m_pCtx, "auth_realm", "vantages");
        }
        else
        {
          ps_elog(PSL_DEBUG, "No pass for: '%s'\n", pApp->getHttpPath());
        }
      }

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

std::list<std::string> &HttpListener::getOutput()
{
  return m_oOutList;
}

