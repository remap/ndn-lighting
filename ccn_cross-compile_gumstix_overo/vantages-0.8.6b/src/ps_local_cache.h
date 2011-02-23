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

#ifndef _PS_LOCAL_CACHE_H
#define _PS_LOCAL_CACHE_H

#include <string>

#include "i_ps_cache.h"
#include "lru_cache.h"

class PsDao;

class PsLocalCache : public IPsCache
{
  // Member Variables
  private:
    static PsLocalCache s_oInstance;

    bool m_bInit;
    LruCache<std::string, PsDao *> m_oLruCache;

  // Methods
  protected:
    PsLocalCache(PsLocalCache &p_oRHS);
    operator=(PsLocalCache &p_oRHS);

  public:
    PsLocalCache();
    virtual ~PsLocalCache();

    static PsLocalCache &getInstance();

    bool init();

    virtual gpgme_key_t *getKey();
    virtual bool getKey(std::string p_sName);
    virtual bool getData(std::string p_sName,
                         pv_cache_type_e p_eType,
                         PsData &p_oData,
                         time_t p_tNewerThan = 0,
                         time_t p_tOlderThan = 0);
    virtual bool putData(std::string p_sName,
                         pv_cache_type_e, p_eType,
                         PsData &p_oData);
    virtual bool processData(PsData &p_oData,
                             Scraper &p_oScraper,
                             PsData &p_oOutput);
    virtual bool verify(PsData &p_oData,
                        PsKey &p_oKey);

    bool addFriend(FriendCache &p_oFriend);
    FriendIter_t friendBegin();
    FriendIter_t friendEnd();

  protected:
    bool loadKey();
    bool poll(FriendCache &p_oFriend, PsData &p_oData);

    std::string makeKey(ps_cache_type_e p_eType,
                        std::string &p_sName);
};

#endif
