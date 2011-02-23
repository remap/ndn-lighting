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

#ifndef _DNS_VERIFIER_H
#define _DNS_VERIFIER_H

#include <sys/types.h>
#include <openssl/rsa.h>

#include <string>
#include <map>

#include "dns_defs.h"
#include "dns_ds.h"

class DnsDnskey;
class DnsRrsig;

class DnsVerifier
{
  // Types and enums
  private:
    typedef std::map<int, const char *> algo_map_t;
    typedef algo_map_t::iterator algo_iter_t;

  // Member Variables
  private:
    static algo_map_t s_oAlgoMap;
    static bool s_bInit;

  // Methods
  public:
    DnsVerifier();
    virtual ~DnsVerifier();

    static const char *getAlgoName(int p_iAlgo);

    bool verify(RRList_t &p_oKeyList, RRList_t &p_oRRset);
    bool verifyDs(DnsDs &p_oDs, RRList_t &p_oKeyList); //ds signs a key in the set

  private:
    bool verifyDs(DnsDs &p_oDs, DnsDnskey *p_oKey);
    bool endsWith(std::string &p_sCandidateName, std::string &p_sBaseName);
    bool verify(DnsDnskey &p_oKey, DnsRrsig &p_oRrsig, u_char *p_pBuff, size_t p_uLen);
    size_t flatten(RRList_t &p_oDataRRs, DnsRrsig &p_oRrsig, u_char *p_pBuff, size_t p_uBuffLen);

    static RSA *extractRSA(u_char *p_pBuff, size_t p_uLen);

};

#endif
