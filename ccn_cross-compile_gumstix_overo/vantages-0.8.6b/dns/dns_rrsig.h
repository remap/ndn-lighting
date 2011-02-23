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

#ifndef _DNS_RRSIG_H
#define _DNS_RRSIG_H

#include "dns_rr.h"

class DnsRrsig : public DnsRR
{
  // Member Variables
  private:
    uint16_t m_uTypeCovered;
    uint8_t m_uAlgo;
    uint8_t m_uLabels;
    uint32_t m_uOrigTTL;
    uint32_t m_uExp;
    uint32_t m_uIncep;
    uint16_t m_uKeyTag;
    u_char *m_pBinSig;
    size_t m_uBinSigLen;
    std::string m_sSignersName;
    std::string m_sSig;

  // Methods
  public:
    DnsRrsig();
    DnsRrsig(DnsRrsig &p_oRHS);
    virtual ~DnsRrsig();

    const DnsRrsig &operator=(DnsRrsig &p_oRHS);
    bool operator==(DnsRrsig &p_oRHS);

    // returns true if there are four bytes of RDATA
    virtual bool rdata_valid();

    uint16_t getTypeCovered();
    void setTypeCovered(uint16_t p_uTypeCovered);

    uint8_t getAlgo();
    void setAlgo(uint8_t p_uAlgo);

    uint8_t getLabels();
    void setLabels(uint8_t p_uLabels);

    uint32_t getOrigTTL();
    void setOrigTTL(uint32_t p_uOrigTTL);

    uint32_t getExpiration();
    void setExpiration(uint32_t p_uExp);

    uint32_t getInception();
    void setInception(uint32_t p_uIncep);

    uint16_t getKeyTag();
    void setKeyTag(uint16_t p_uKeyTag);

    std::string &getSig();
    void setSig(std::string &p_sSig);

    u_char *getBinSig();
    size_t getBinSigLen();
    void setBinSig(u_char *p_pSig, size_t p_uLen);

    std::string &getSignersName();
    void setSignersName(std::string &p_sName);

    virtual DnsRrsig *dup();
    virtual void printRData();
    virtual int verificationRData(DnsBits_t &p_oOut);

    virtual bool parseRData(u_char *p_pMsg,
                            size_t p_uMsgLen,
                            u_char *p_pRData,
                            size_t p_uRDataLen);
};

#endif
