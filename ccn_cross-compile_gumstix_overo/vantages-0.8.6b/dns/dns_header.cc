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

#include "config.h"
#include <stdio.h>
#include <cassert>
#include <vector>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "dns_header.h"
#include "dns_defs.h"
#include "dns_err.h"

using namespace std;

struct header_fmt
{
  uint16_t id, flags, qdcount, ancount, nscount, arcount;
} __attribute__((__packed__));

DnsHeader::DnsHeader()
  : m_init(false)
{
  m_qdcount = m_ancount = m_nscount = m_arcount = 0;

  memset(&m_flags, 0, sizeof(m_flags));
  m_flags.rd = 1;
}

DnsHeader::DnsHeader(bool question, int id)
{
  m_id = assignID(id);

  m_qdcount = m_ancount = m_nscount = m_arcount = 0;

  memset(&m_flags, 0, sizeof(m_flags));
  m_flags.response = !question;
  m_flags.rd = 1;
}

bool DnsHeader::init(unsigned char *bytes, size_t size)
{
  if (size < SIZE)
  {
    m_init = false;
  }
  else
  {
    // read various shorts from the header
    header_fmt *h = (header_fmt *)bytes;
    m_id = ntohs(h->id);
    m_qdcount = ntohs(h->qdcount);
    m_ancount = ntohs(h->ancount);
    m_nscount = ntohs(h->nscount);
    m_arcount = ntohs(h->arcount);

    // copy the flags
    // this doesn't work with a bitfield struct :(
    // memcpy(&m_flags, bytes + 2, sizeof(m_flags));
    unpack_flags(bytes + 2);

    m_init = true;
  }

  return m_init;
}

int DnsHeader::assignID(int p_iID /*= -1*/)
{
  if (p_iID < 0)
  {
    m_id = (int)(65536 * (rand() / (RAND_MAX + 1.0)));
  }
  else
  {
    m_id = p_iID;
  }

  return m_id;
}

int DnsHeader::toWire(u_char *p_pBuff, size_t p_uLen)
{
  int iRet = -1;

    header_fmt h;
  if (NULL == p_pBuff)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to write to NULL buffer\n");
#endif
    DnsError::getInstance().setError("Unable to write to NULL buffer");
  }
  else if (p_uLen < sizeof(h))
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to write header into buffer (too small): %u < %lu\n",
            (unsigned) p_uLen,
            sizeof(h));
#endif
    DnsError::getInstance().setError("Unable to write header into buffer (too small)");
  }
  else
  {

    h.id = htons(m_id);
    h.qdcount = htons(m_qdcount);
    h.ancount = htons(m_ancount);
    h.nscount = htons(m_nscount);
    h.arcount = htons(m_arcount);
    pack_flags((u_char *)&h.flags);

    memcpy(p_pBuff, &h, sizeof(h));
    iRet = sizeof(h);
  }

  return iRet;
}

bool DnsHeader::getResponse()
{
  return m_flags.response;
}

unsigned DnsHeader::getOpcode()
{
  return m_flags.opcode;
}

bool DnsHeader::get_aa()
{
  return m_flags.aa;
}

bool DnsHeader::get_tc()
{
  return m_flags.tc;
}

bool DnsHeader::get_rd()
{
  return m_flags.rd;
}

bool DnsHeader::get_ra()
{
  return m_flags.ra;
}

unsigned DnsHeader::get_z()
{
  return m_flags.z;
}

bool DnsHeader::get_auth()
{
  return m_flags.auth;
}

rcode_t DnsHeader::get_rcode()
{
  return m_flags.rcode;
}

// the following are the only portable way of doing this stuff
void DnsHeader::unpack_flags(unsigned char *b)
{
  m_flags.response = b[0] & 0x80;
  m_flags.opcode = b[0] & 0x78;
  m_flags.aa = b[0] & 0x04;
  m_flags.tc = b[0] & 0x02;
  m_flags.rd = b[0] & 0x01;

  m_flags.ra = b[1] & 0x80;
  m_flags.z = b[1] & 0x60;
  m_flags.auth = b[1] & 0x10;
  m_flags.rcode = (rcode_t) (b[1] & 0x0f);
}

void DnsHeader::pack_flags(u_char *dest) {
  dest[0] = (m_flags.rd
          | (m_flags.tc << 1)
          | (m_flags.aa << 2)
          | (m_flags.opcode << 3)
          | (m_flags.response << 7));
    
  dest[1] = (m_flags.rcode
          | (m_flags.auth << 4)
          | (m_flags.z << 6)
          | (m_flags.ra << 7));
}

void DnsHeader::print()
{
  fprintf(stdout, ";; ->>HEADER<<- opcode %u, status: %d, id: %d\n", getOpcode(), get_rcode(), id());
  fprintf(stdout, ";; flags: ");
  if (get_rd())
  {
    fprintf(stdout, "rd ");
  }
  if (get_aa())
  {
    fprintf(stdout, "aa ");
  }
  if (get_tc())
  {
    fprintf(stdout, "tc ");
  }
  if (get_ra())
  {
    fprintf(stdout, "ra ");
  }
  if (get_auth())
  {
    fprintf(stdout, "auth ");
  }
  if (get_z())
  {
    fprintf(stdout, "z ");
  }
  fprintf(stdout, "; QUERY %d, ANSWER: %d, AUTHORITY: %d, ADDITIONAL: %d\n\n", qd_count(), an_count(), ns_count(), ar_count());
}
