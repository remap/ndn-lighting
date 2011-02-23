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

#ifndef _I_PV_CACHE_H
#define _I_PV_CACHE_H

#include <gpgme.h>

#include "pv_defs.h"

class IPsCache
{
  // Member Variables
  private:

  // Methods
  public:
    IPsCache();
    virtual ~IPsCache();

    virtual gpgme_key_t *getKey() = 0;
    virtual bool getKey(std::string p_sName) = 0;
    virtual bool getData(std::string p_sName,
                         pv_cache_type_e p_eType,
                         PsData &p_oData,
                         time_t p_tNewerThan = 0,
                         time_t p_tOlderThan = 0) = 0;
    virtual bool putData(std::string p_sName,
                         pv_cache_type_e, p_eType,
                         PsData &p_oData) = 0;
    virtual bool processData(PsData &p_oData,
                             Scraper &p_oScraper,
                             PsData &p_oOutput) = 0;
    virtual bool verify(PsData &p_oData,
                        PsKey &p_oKey) = 0;
};

#endif
