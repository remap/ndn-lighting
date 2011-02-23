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

#ifndef __DNS_NAME_H__
#define __DNS_NAME_H__

#include <list>
#include <string>
#include <vector>

#include <sys/types.h>

#ifdef DEBUG
#include <iostream>
#endif

typedef unsigned char u_char;

class DnsCompression;

class DnsName
{
  // Member Variables
  private:
    static const char *s_szValidChars;

    std::list<std::string *> m_parts;
    size_t m_length;
    std::string m_sName;

  // Methods
  public:
    DnsName(std::string &);
    DnsName(const DnsName &);
    ~DnsName();

    inline size_t length() { return m_length; }

    static DnsName *from_wire(u_char *, size_t, size_t &);
    int toWire(u_char *p_pBuff, size_t p_uLen, DnsCompression &);
    int to_wire_canonical(u_char *buf, size_t buf_len);

    void display_name(std::string &print, bool p_bFormat = true);
    std::string toString();
    std::string verifName();

    static std::string printParts(std::list<std::string *> &p_oParts);

  private:
    DnsName(std::list<std::string *> &, size_t);
    // void add_part(std::string &);
    static void empty_list(std::list<std::string *> &);
    static bool read_name(u_char *, size_t, size_t &,
        std::list<std::string *> &, size_t &);

    // DnsCompression needs access to m_parts
    friend class DnsCompression;
};

#endif /* __DNS_NAME_H__ */
