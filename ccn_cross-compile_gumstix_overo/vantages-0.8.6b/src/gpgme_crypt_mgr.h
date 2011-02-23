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

#ifndef _GPGME_CRYPT_MGR_H
#define _GPGME_CRYPT_MGR_H

#include <gpgme.h>

#include <string>

#include "ps_crypt_mgr.h"

class GpgmeCryptMgr : public PsCryptMgr
{
  // Member Variables
  private:
    gpgme_ctx_t m_pGpgmeCtx;
    std::string m_sCurrentSigningKey;
    std::string m_sPP;
    std::string m_sHomeDir;

    static bool s_bInit;
  public:
    static const char *GPGMR_CRYPT_MGR;

  // Methods
  public:
    GpgmeCryptMgr();
    virtual ~GpgmeCryptMgr();

    bool primeEngine();
    bool init();
    bool loadFriendKeys();

    std::string &getPP();
    bool setPP(std::string &p_sPP);

    std::string &getHomeDir();
    void setHomeDir(const char *p_szHomeDir);

    virtual bool verify(std::string &p_sSigData);
    virtual bool verify(std::string &p_sSig, std::string &p_sData);

    virtual std::string &getSigningKey();
    virtual bool setSigningKey(std::string &p_sKey);
    virtual bool addPubKey(std::string &p_sKey);

    virtual bool sign(std::string &p_sData,
                      std::string &p_sOutputSig,
                      bool p_bDetach);

    virtual GpgmeCryptMgr *dup();
};

#endif
