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

#ifndef _HTTP_LISTENER_H
#define _HTTP_LISTENER_H

#include <string>
#include <list>

#include "ps_defs.h"

typedef struct shttpd_ctx shttpd_ctx;

class HttpListener
{
  // Member Variables
  private:
    bool m_bRun;
    int m_iPort;
    shttpd_ctx *m_pCtx;
    size_t m_uOutputOffset;
    bool m_bSending;
    std::string m_sInput;
    std::list<std::string> m_oOutList;

  // Methods
  public:
    HttpListener();
    virtual ~HttpListener();

    bool init(char *p_szPort, AppList_t &p_oAppList);
    bool init(const char *p_szPort, AppList_t &p_oAppList);

    int getPort();

    bool listen();
    void kill();
    bool sendingResponse();
    void sendingResponse(bool p_bSending);

    void clearInput();
    void appendInput(char *p_pBuff, int p_iLen);
    std::string &getInput();

    std::list<std::string> &getOutput();
};

#endif
