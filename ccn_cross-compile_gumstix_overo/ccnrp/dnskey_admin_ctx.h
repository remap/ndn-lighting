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

#ifndef _DNSKEY_ADMIN_CTX_H
#define _DNSKEY_ADMIN_CTX_H

#include "http_list_ctx.h"
#include "ps_app.h"

class PsUrlParser;
class DnskeyApp;

class DnskeyAdminCtx : public HttpListenerCtx, public PsApp
{
  // Member Variables
  private:
    DnskeyApp &m_oApp;

  // Methods
  public:
    DnskeyAdminCtx(DnskeyApp &p_oApp);
    virtual ~DnskeyAdminCtx();

    virtual bool process();

    virtual const char *getHttpPath();
    virtual const char *getHttpPass();
    virtual DnskeyAdminCtx *createCtx();
    virtual bool enabled();
    virtual bool init();

    virtual bool execute();

  protected:
    bool lookup(PsUrlParser &p_oParser);
    bool add(PsUrlParser &p_oParser);
    bool poll(PsUrlParser &p_oParser);
    bool addSpaces(std::string &p_sStr, size_t p_uRunLen = 50);
    std::string &crToBr(std::string &p_sStr);

    bool dataPage(PsUrlParser &p_oParser);
    bool submissionPage(PsUrlParser &p_oParser);
    bool pollPage(PsUrlParser &p_oParser);
    bool friendPage(PsUrlParser &p_oParser);
    bool monPage(PsUrlParser &p_oParser);

    bool addFriend(PsUrlParser &p_oParser);
    bool remFriend(PsUrlParser &p_oParser);

    bool taFilePage(PsUrlParser &p_oParser);
    bool setTaPolicy(PsUrlParser &p_oParser);
};

#endif

