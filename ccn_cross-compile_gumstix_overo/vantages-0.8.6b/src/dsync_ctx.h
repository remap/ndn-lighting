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

#ifndef _DSYNC_CTX_H
#define _DSYNC_CTX_H

#include "http_list_ctx.h"
#include "ps_app.h"
#include "dns_defs.h"

using namespace std;

class PsUrlParser;
class DsyncApp;

class DsyncCtx : public HttpListenerCtx
{
  // Member Variables
  private:
    bool parseKey(string keyString, RRList_t &keys);

  // Methods
  public:
    DsyncCtx();
    virtual ~DsyncCtx();

    virtual bool process();

    virtual bool enabled();
    virtual bool init();

    //virtual bool execute();

  protected:
    std::string &crToBr(std::string &p_sStr);
    bool infoPage(PsUrlParser& oParser);
    bool addPage(PsUrlParser& oParser);
    bool delPage(PsUrlParser& oParser);
    bool add(PsUrlParser& oParser);
    bool deletePage(PsUrlParser& oParser);
    bool lookup(PsUrlParser& oParser);
    bool del(PsUrlParser& oParser);
};

#endif

