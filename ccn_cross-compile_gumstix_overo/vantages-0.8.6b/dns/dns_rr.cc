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
#include <cassert>
#include <cstring>
#include <arpa/inet.h>

#include "dns_rr.h"
#include "dns_a.h"
#include "dns_name.h"
#include "dns_rr_fact.h"
#include "dns_defs.h"
#include "dns_err.h"

#ifdef DEBUG
#include <iostream>
#endif

#define the_force namespace std
using the_force;

DnsRR::DnsRR(int type)
  : m_init(false),
    m_name(NULL),
    m_type(type),
    m_class(DNS_CLASS_IN),
    m_ttl(0),
    m_uMsgLen(0),
    m_pMsg(NULL),
    m_rdlen(0),
    m_rdata(NULL)
{
}

DnsRR::DnsRR(DnsRR &p_oRHS)
  : m_init(false),
    m_name(NULL),
    m_type(0),
    m_class(DNS_CLASS_IN),
    m_ttl(0),
    m_uMsgLen(0),
    m_pMsg(NULL),
    m_rdlen(0),
    m_rdata(NULL)
{
  *this = p_oRHS;
}

DnsRR::~DnsRR()
{
  if (m_name != NULL)
  {
    delete m_name;
    m_name = NULL;
  }

  if (m_rdata != NULL)
  {
    delete [] m_rdata;
    m_rdata = NULL;
  }
}

const DnsRR &DnsRR::operator=(DnsRR &p_oRHS)
{
  set_class(p_oRHS.get_class());
  m_type = p_oRHS.type();
  set_ttl(p_oRHS.ttl());
  if (NULL != p_oRHS.get_name())
  {
    set_name(*p_oRHS.get_name());
  }
  set_rdata(p_oRHS.get_rdata(), get_rdlen());
  //set type!
  return *this;
}

void DnsRR::print()
{
  string sName;
  if (NULL != m_name)
  {
    m_name->display_name(sName);
  }
  fprintf(stdout, "%s\t%d\t%d\t%d\t", sName.c_str(), ttl(), get_class(), type());
  if (!isQuestion())
  {
    printRData();
  }
  fprintf(stdout, "\n");
}

void DnsRR::printRData()
{
  size_t uLen = get_rdlen();
  u_char *pData = get_rdata();
  for (size_t u = 0; u < uLen; u++)
  {
    fprintf(stdout, "%02x ", pData[u]);
  }
}

// generate a question from a name and type
DnsRR *DnsRR::question(const DnsName &name, int type)
{
//  DnsRR *ret = new DnsRR(type);
  DnsRR *ret = (DnsRR *) DnsRrFactory::getInstance().create(type);
  if (NULL == ret)
  {
    dns_log("Unable to locate RR type: %d\n", type);
  }
  else
  {
    ret->init((DnsName *)&name, 1);
  }
  return ret;
}

// for initialising normal RRs
bool DnsRR::init(DnsName *name,
                 int i_class, 
                 int ttl /*= -1*/, 
                 u_char *p_pMsg /*= NULL*/,
                 size_t p_uMsgLen /*= 0*/,
                 u_char *rdata /*= NULL*/,
                 size_t rdlen /*= 0*/)
{
  // if it's already been initialised, we need to delete some crap
  if (m_name != NULL)
  {
    delete m_name;
    m_name = NULL;
  }

  m_name = new DnsName(*name);
  m_class = i_class;
  m_ttl = ttl;

  m_init = parseRData(p_pMsg,
                      p_uMsgLen,
                      rdata,
                      rdlen);
  return m_init;
}

// method for parsing non-question DnsRRs
DnsRR *DnsRR::parse(u_char *bytes, size_t size, size_t &offset)
{
  return parse(bytes, size, offset, false);
}

// a factory method for parsing DnsRRs in wire format
DnsRR *DnsRR::parse(u_char *bytes, size_t size, size_t &offset, bool question)
{
  DnsRR *ret = NULL;
  DnsName *name = NULL;

  // attempt to read the name
  if (NULL == (name = DnsName::from_wire(bytes, size, offset)))
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to read name from wire.\n");
/*
    for (unsigned i = 0; i < size - offset; i++)
    {
      if (i % 2 == 0)
      {
        fprintf(stderr, " ");
      }
      fprintf(stderr, "%02x", (uint8_t) bytes[offset + i]);
    }
    fprintf(stderr, "\n");
*/
#endif
  }
  else
  {
    // if it's a question we only need four more bytes
    if (question)
    {
      ret = parse_question_rr(bytes, size, offset, name);
    }

    // if it's not a question, we have to read a variable amount of data
    else
    {
      ret = parse_normal_rr(bytes, size, offset, name);
    }
  }

  // clean up memory we may have allocated if there was a problem
  if (name != NULL)
  {
    delete name;
  }

  return ret;
}

DnsRR *DnsRR::parse_question_rr(u_char *bytes, size_t size, size_t &offset,
        DnsName *n)
{
  DnsRR *ret = NULL;
  if (offset + sizeof(q_header) <= size)
  {
    q_header *q = (q_header *)(bytes + offset);
    int type = htons(q->type);
    int qclass = htons(q->qclass);
    offset += sizeof(q_header);

    DnsRR *new_rr = (DnsRR *) DnsRrFactory::getInstance().create(type);
    if (NULL == new_rr)
    {
#ifdef _DNS_DEBUG
      dns_log("Unable to create RR of type: %d\n", type);
#endif
      DnsError::getInstance().setError("Unable to create RR of type");
    }
    else
    {
      new_rr->init(n, qclass);
      ret = new_rr;
    }
  }
  return ret;
}

DnsRR *DnsRR::parse_normal_rr(u_char *bytes, size_t size, size_t &offset,
        DnsName *n)
{
  DnsRR *ret = NULL;
  u_char *rdata = NULL;

  if (offset + sizeof(rr_header) > size)
  {
#ifdef _DNS_DEBUG
    dns_log("Incorrect size: %u > %u\n", (unsigned) (offset + sizeof(rr_header)), (unsigned) size);
#endif
  }
  else
  // 10 bytes of fixed data
  {
    rr_header *r = (rr_header *)(bytes + offset);
    int type = ntohs(r->q.type);
    int qclass = ntohs(r->q.qclass);
    int rdlen = ntohs(r->rdlen);

    // ntohl returns unsigned, and we want signed
    uint32_t u_ttl = ntohl(r->ttl);
    int ttl = *(int *)&u_ttl;

    // try to get the rdata
    if (offset + sizeof(rr_header) + rdlen <= size)
    {
      rdata = bytes + offset;
rdata += sizeof(rr_header);
      offset += sizeof(rr_header) + rdlen;

      DnsRR *new_rr = (DnsRR *) DnsRrFactory::getInstance().create(type);
      if (NULL == new_rr)
      {
#ifdef _DNS_DEBUG
        dns_log("Unable to locate RR type: %d\n", type);
#endif
      }
      else
      {
        new_rr->init(n, qclass, ttl, bytes, size, rdata, rdlen);
        ret = new_rr;
      }

      // make sure its RDATA isn't bogus
      /* we're dumb, so let's be dumb
      */
      if (!new_rr->rdata_valid())
      {
        delete ret;
        ret = NULL;
      }
    }
  }

  return ret;
}

void DnsRR::set_class(uint16_t p_uClass)
{
  m_class = p_uClass;
}

void DnsRR::set_ttl(uint32_t p_uTTL)
{
  m_ttl = p_uTTL;
}

void DnsRR::set_name(DnsName &p_oName)
{
  m_name = new DnsName(p_oName);
}

bool DnsRR::rdata_valid()
{
  bool ret = true;

  if (m_rdlen > 0 && m_rdata == NULL)
  {
    ret = false;
  }

  return ret;
}

int DnsRR::toWire(u_char *p_pBuff, size_t p_uLen, DnsCompression &compression)
{
  int iRet = -1;

  // copy the name in
  int iNameLen = m_name->toWire(p_pBuff, p_uLen, compression);
  if (-1 == iNameLen)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to init name.\n");
#endif
    DnsError::getInstance().setError("Unable to init name.");
  }
  else
  {
    iRet = iNameLen;
    p_pBuff += iNameLen;
    p_uLen -= iNameLen;
    dns_log("Adding %d bytes to wire format from NAME '%s'\n", iNameLen, m_name->toString().c_str());

    // question RR
    if (isQuestion())
    {
      q_header q;
      q.type = htons(m_type);
      q.qclass = htons(m_class);

      int iHeaderLen = sizeof(q);
      memcpy(p_pBuff, &q, iHeaderLen);
      p_pBuff += iHeaderLen;
      p_uLen -= iHeaderLen;
      iRet += iHeaderLen;
    }
    // normal RR
    else
    {
      rr_header r;
      uint32_t u_ttl = *(uint32_t *)&m_ttl;

      r.q.type = htons(m_type);
      r.q.qclass = htons(m_class);
      r.ttl = htonl(u_ttl);

      // - 2, because we copy the rdlen in the virtual rdata_to_wire
      int iHeaderLen = sizeof(r) - 2;
      memcpy(p_pBuff, &r, iHeaderLen);
      p_pBuff += iHeaderLen;
      p_uLen -= iHeaderLen;
      iRet += iHeaderLen;

      int iTmp = rdata_to_wire(p_pBuff, p_uLen, compression);
      if (-1 == iTmp)
      {
#ifdef _DNS_DEBUG
        dns_log("Unable to convert rdata to wire.\n");
#endif
        DnsError::getInstance().setError("Unable to convert rdata to wire.");
        iRet = -1;
      }
      else
      {
        iRet += iTmp;
      }
    }
  }

  return iRet;
}

int DnsRR::rdata_to_wire(u_char *p_pBuff, size_t p_uLen, DnsCompression &compression)
{
  int iRet = -1;

  size_t uRdLen = get_rdlen();
  u_char *pRData = get_rdata();

  if (NULL == p_pBuff)
  {
    dns_log("Unable to write to NULL buffer.\n");
    DnsError::getInstance().setError("Unable to write to NULL buffer.");
  }
  else if (uRdLen > 0 && NULL == pRData)
  {
    dns_log("RData ken is %u but buffer is NULL\n", (unsigned) uRdLen);
    DnsError::getInstance().setError("RData ken is a # but buffer is NULL");
  }
  else
  {
    p_pBuff[0] = (uRdLen >> 8) & 0x00ff;
    p_pBuff[1] = uRdLen & 0x00ff;

    p_pBuff += 2;
    p_uLen -= 2;
    if (uRdLen > 0)
    {
      memcpy(p_pBuff, pRData, uRdLen);
    }
    iRet = 2 + uRdLen;
  }

  return iRet;
}

int DnsRR::verificationWireFormat(DnsBits_t &p_oVec)
{
  int iRet = -1;

  u_char pBuff[255];
  memset(pBuff, 0, 255);
  // copy the name in
  int iNameLen = m_name->to_wire_canonical(pBuff, 255);
  if (-1 == iNameLen)
  {
    dns_log("Unable to init name.\n");
    DnsError::getInstance().setError("Unable to init name.");
  }
  else
  {
    iRet = iNameLen;
    int i = 0;
    for (i = 0; i < iNameLen; i++)
    {
      p_oVec.push_back(pBuff[i]);
    }

    rr_header r;
    uint32_t u_ttl = *(uint32_t *)&m_ttl;

    r.q.type = htons(m_type);
    r.q.qclass = htons(m_class);
    r.ttl = htonl(u_ttl);

    // - 2, because we copy the rdlen in the virtual rdata_to_wire
    int iHeaderLen = sizeof(r) - 2;
    for (i = 0; i < iHeaderLen; i++)
    {
      p_oVec.push_back(((u_char *) &r)[i]);
    }
    iRet += iHeaderLen;

    int iTmp = verificationRData(p_oVec);
    if (-1 == iTmp)
    {
      dns_log("Unable to convert rdata to wire.\n");
      DnsError::getInstance().setError("Unable to convert rdata to wire.");
      iRet = -1;
    }
    else
    {
      iRet += iTmp;
    }
  }

  return iRet;
}

int DnsRR::verificationRData(DnsBits_t &p_oVec)
{
  int iRet = -1;

  size_t uRdLen = get_rdlen();
  u_char *pRData = get_rdata();

  if (uRdLen > 0 && NULL == pRData)
  {
    dns_log("RData ken is %u but buffer is NULL\n", (unsigned) uRdLen);
    DnsError::getInstance().setError("RData ken is a # but buffer is NULL");
  }
  else
  {
    p_oVec.push_back((uRdLen >> 8) & 0x00ff);
    p_oVec.push_back(uRdLen & 0x00ff);

    for (size_t u = 0; u < uRdLen; u++)
    {
      p_oVec.push_back(pRData[u]);
    }
    iRet = 2 + uRdLen;
  }

  return iRet;
}

// <EMO> Note: we cannot handle compression after
// thiis because we are copying the data.
void DnsRR::set_rdata(u_char *data, size_t len)
{
  if (m_rdata != NULL)
  {
    delete [] m_rdata;
    m_rdata = NULL;
  }

  m_rdlen = len;
  if (len > 0)
  {
    m_rdata = new u_char[len];
    memcpy(m_rdata, data, len);
  }
  else
    m_rdata = NULL;
}

u_char *DnsRR::get_rdata()
{
  return m_rdata;
}

size_t DnsRR::get_rdlen()
{
  return m_rdlen;
}

bool DnsRR::isQuestion()
{
  return (m_ttl < 0);
}

IDnsDupable *DnsRR::dup()
{
  return new DnsRR(type());
}

bool DnsRR::parseRData(u_char *p_pMsg,
                       size_t p_uMsgLen,
                       u_char *p_pRData,
                       size_t p_uRDataLen)
{
  set_rdata(p_pRData, p_uRDataLen);
  return true;
}

size_t DnsRR::preparedRData(std::vector<u_char> &p_oOut)
{
  p_oOut.clear();
  size_t uLen = get_rdlen();
  u_char *pRData = get_rdata();
  for (size_t u = 0; u < uLen; u++)
  {
    p_oOut.push_back(pRData[u]);
  }

  return p_oOut.size();
}

const bool DnsRR::operator==(DnsRR &p_oRHS)
{
  /*
  if (get_name() != p_oRHS.get_name())
    return false;
  if (type() != p_oRHS.type())
    return false;
  if (get_class() != p_oRHS.get_class())
    return false;
  */
  if (get_rdlen() != p_oRHS.get_rdlen())
    return false;
  int cmp = memcmp(get_rdata(), p_oRHS.get_rdata(), get_rdlen());
  return cmp == 0 ? true : false;
}

