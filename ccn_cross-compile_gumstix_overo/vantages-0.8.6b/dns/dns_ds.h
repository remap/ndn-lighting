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

#ifndef __DNS_DS_H__
#define __DNS_DS_H__

#include <stdint.h>

#include "dns_dnskey.h"
#include "dns_rr.h"

class DnsName;

class DnsDs : public DnsRR
{
  private:
    uint16_t m_uFlags;
    uint8_t m_uProto;
    uint8_t m_uAlgo;
    u_char *m_pBinDig;
    size_t m_uBinDigLen;
    std::string m_sDig;

  public:
    DnsDs();
    DnsDs(DnsDs &p_oRHS);
    virtual ~DnsDs();

    const DnsDs &operator=(DnsDs &p_oRHS);

    // returns true if there are four bytes of RDATA
    virtual bool rdata_valid();

    uint16_t getFlags();
    void setFlags(uint16_t p_iFlags);

    uint8_t getProto();
    void setProto(uint8_t p_iProto);

    uint8_t getAlgo();
    void setAlgo(uint8_t p_iAlgo);

    std::string &getDig();
    void setDig(std::string &p_sDig);

    u_char *getBinDig();
    size_t getBinDigLen();
    void setBinDig(u_char *p_pDig, size_t p_uLen);

    virtual DnsDs *dup();
    virtual void printRData();

    virtual bool parseRData(u_char *p_pMsg,
                            size_t p_uMsgLen,
                            u_char *p_pRData,
                            size_t p_uRDataLen);
};

#endif /* __DNS_DS_H__ */
