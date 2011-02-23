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

#ifndef __DNS_DnsRR_H__
#define __DNS_DnsRR_H__

#include <cassert>
#include <vector>

#include <sys/types.h>

#include "i_dns_dupable.h"
#include "dns_defs.h"

class DnsName;
class DnsCompression;

class DnsRR : public IDnsDupable
{
  public:
    DnsRR(int type);
    DnsRR(DnsRR &p_RHS);
    virtual ~DnsRR();

    void print();
    virtual void printRData();

    static DnsRR *question(const DnsName &name, int type);

//    void init(DnsName *, int);
    bool init(DnsName *name,
              int i_class,
              int ttl = -1,
              u_char *p_pMsg = NULL,
              size_t p_uMsgLen = 0,
              u_char *rdata = NULL,
              size_t rdlen = 0);

    // returns DnsRR pointer if DnsRR is correctly parsed, NULL otherwise
    static DnsRR *parse(u_char *, size_t, size_t &);
    static DnsRR *parse(u_char *, size_t, size_t &, bool);

    inline uint16_t type(void) { return m_type; }
    inline uint16_t get_class(void) { return m_class; }
    inline uint32_t ttl(void) { return m_ttl; }
    inline DnsName *get_name(void) { return m_name; }

    void set_class(uint16_t p_uClass);
    void set_ttl(uint32_t p_uTTL);
    void set_name(DnsName &p_oName);

    // overridden by children, returns true iff the RDATA is valid format
    virtual bool rdata_valid();

    // converts the DnsRR to wire format
    int toWire(u_char *p_pBuff, size_t p_uLen, DnsCompression &compression);
    virtual int rdata_to_wire(u_char *p_pBuff, size_t p_uLen, DnsCompression &);

    // Converts the DnsRR to wire format with the
    // rules of DNSSEC verification in play
    int verificationWireFormat(DnsBits_t &p_oVec);
    virtual int verificationRData(DnsBits_t &p_oVec);

    // set the RDATA, copying it for you
    void set_rdata(u_char *, size_t);
    u_char *get_rdata();
    size_t get_rdlen();

    virtual bool isQuestion();

    virtual IDnsDupable *dup();

    virtual size_t preparedRData(std::vector<u_char> &p_oOut);

    virtual const bool operator==(DnsRR &p_oRHS);
    const DnsRR &operator=(DnsRR &p_oRHS);

  protected:
    virtual bool parseRData(u_char *p_pMsg,
                            size_t p_uMsgLen,
                            u_char *p_pRDataLen,
                            size_t p_uRDataLen);

  private:
    // helper functions for parsing the various DnsRR types
    static DnsRR *parse_question_rr(u_char *, size_t, size_t &, DnsName *);
    static DnsRR *parse_normal_rr(u_char *, size_t, size_t &, DnsName *);

    // converts the name to DNS wire representation
    bool name_to_bytes(u_char *, size_t &);

    bool m_init;

    DnsName *m_name;
    uint16_t m_type, m_class;
    int32_t m_ttl;

    size_t m_uMsgLen;
    u_char *m_pMsg;

  protected:
    size_t m_rdlen;
    u_char *m_rdata;
};

#endif /* __DnsRR_H__ */
