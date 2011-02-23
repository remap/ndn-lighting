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

#ifndef __DNS_HEADER_H__
#define __DNS_HEADER_H__

#include <sys/types.h>

#include "dns_defs.h"

class DnsHeader
{
  public:
    DnsHeader();
    DnsHeader(bool, int);
    bool init(unsigned char *bytes, size_t size);
    int assignID(int p_iID = -1);

    // size of a DNS header (constant)
    static const size_t SIZE = 12;

    inline int id() { return m_id; }
    inline int qd_count() { return m_qdcount; }
    inline int an_count() { return m_ancount; }
    inline int ns_count() { return m_nscount; }
    inline int ar_count() { return m_arcount; }

    inline bool response() { return m_flags.response; }
    rcode_t rcode() { return (rcode_t)m_flags.rcode; }

    int toWire(u_char *p_pBuff, size_t p_uLen);

    inline void qd_inc() { ++m_qdcount; }
    inline void an_inc() { ++m_ancount; }
    inline void ns_inc() { ++m_nscount; }
    inline void ar_inc() { ++m_arcount; }

    inline void qd_dec() { --m_qdcount; }
    inline void an_dec() { --m_ancount; }
    inline void ns_dec() { --m_nscount; }
    inline void ar_dec() { --m_arcount; }

    bool getResponse();
    unsigned getOpcode();
    bool get_aa();
    bool get_tc();
    bool get_rd();
    bool get_ra();
    unsigned get_z();
    bool get_auth();
    rcode_t get_rcode();

    void print();

  private:
    bool m_init;

    int m_id, m_qdcount, m_ancount, m_nscount, m_arcount;
    struct {
      bool response;
      unsigned opcode;
      bool aa;
      bool tc;
      bool rd;
      bool ra;
      unsigned z;
      bool auth;
      rcode_t rcode;
    } m_flags;

    void unpack_flags(unsigned char *);
    void pack_flags(unsigned char *);
};

#endif /* __DNS_HEADER_H__ */
