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
#include <string.h>

#include "http_list_ctx.h"
#include "ps_logger.h"
#include "ps_defs.h"

using namespace std;

HttpListenerCtx::HttpListenerCtx()
  : m_uOutputOffset(0),
    m_bSending(false),
    m_bReady(false),
    m_bErr(false)
{

}

HttpListenerCtx::~HttpListenerCtx()
{

}

bool HttpListenerCtx::readyToSend()
{
  return m_bReady;
}

void HttpListenerCtx::setReadyToSend(bool p_bReady)
{
  m_bReady = p_bReady;
}

bool HttpListenerCtx::sendingResponse()
{
  return m_bSending;
}

void HttpListenerCtx::sendingResponse(bool p_bSending)
{
  m_bSending = p_bSending;
}

void HttpListenerCtx::clearInput()
{
  m_sInput.clear();
}

void HttpListenerCtx::appendInput(char *p_pBuff, int p_iLen)
{
  appendInput((const char *) p_pBuff, p_iLen);
}

void HttpListenerCtx::appendInput(const char *p_pBuff, int p_iLen)
{
  if (NULL != p_pBuff)
  {
    m_sInput.append(p_pBuff, p_iLen);
  }
}

std::string &HttpListenerCtx::getInput()
{
  return m_sInput;
}

bool HttpListenerCtx::process()
{
  ps_elog(PSL_CRITICAL, "Method not implemented in base-class.\n");
  return false;
}

void HttpListenerCtx::clearOutput()
{
  m_oOutList.clear();
}

void HttpListenerCtx::appendOutput(char *p_szOutput)
{
  appendOutput((const char *) p_szOutput);
}

void HttpListenerCtx::appendOutput(const char *p_szOutput)
{
  if (NULL != p_szOutput)
  {
    string sOutput = p_szOutput;
    appendOutput(sOutput);
  }
}

void HttpListenerCtx::appendOutput(std::string &p_sOutput)
{
  m_oOutList.push_back(p_sOutput);
}

void HttpListenerCtx::appendOutput(int p_iOutput)
{
  char *sz = new char[11];
  memset(sz, 0, 11);
  sprintf(sz, "%d", p_iOutput);
  appendOutput(sz);
  delete[] sz;
}

std::list<std::string> &HttpListenerCtx::getOutput()
{
  return m_oOutList;
}

bool HttpListenerCtx::done()
{
  return sendingResponse() && m_oOutList.empty();
}

bool HttpListenerCtx::error()
{
  return m_bErr;
}

void HttpListenerCtx::setError(bool p_bErr)
{
  m_bErr = p_bErr;
}
