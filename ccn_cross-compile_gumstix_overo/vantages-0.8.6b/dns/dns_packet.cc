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

#include <vector>
#include <algorithm>
#include <string.h>

using namespace std;

#include "dns_packet.h"
#include "dns_header.h"
#include "dns_tsig.h"
#include "dns_rr.h"
#include "dns_err.h"

DnsPacket::DnsPacket()
  : m_bParsed(false),
    m_pBytes(NULL),
    m_uBytesLen(0),
    m_uRecvLen(0),
    m_pTsig(NULL),
    m_oHeader(false, -1)
{

}

DnsPacket::DnsPacket(bool question, int id /*= -1*/)
  : m_bParsed(false),
    m_pBytes(NULL),
    m_uBytesLen(0),
    m_uRecvLen(0),
    m_pTsig(NULL),
    m_oHeader(question, id)
{

}

DnsPacket::~DnsPacket()
{
  clear();

  setTsig(NULL);
}

// serialize the packet into a vector
int DnsPacket::toWire(u_char *p_pBuff, size_t p_uBuffLen, bool p_bAddTsig /*= true*/)
{
  int iRet = -1;

  memset(p_pBuff, 0, p_uBuffLen);
  setMsg(p_pBuff, p_uBuffLen);
  getCompression().clear();

  int iTmp = 0;

  bool bAddedTsig = false;
  DnsTsig *pTsig = getTsig();
  if (p_bAddTsig && NULL != pTsig)
  {
    if (!pTsig->calc(*this))
    {
      dns_log("Unable to calculate TSIG for message.\n");
      DnsError::getInstance().setError("Unable to calculate TSIG for message.");
      iTmp = -1;
    }
    else
    {
      bAddedTsig = true;
      addAdditional(*pTsig);
    }
  }

  memset(p_pBuff, 0, p_uBuffLen);
  setMsg(p_pBuff, p_uBuffLen);
  getCompression().clear();

  size_t uLen = 0;

  if (-1 != iTmp)
  {
    iTmp = m_oHeader.toWire(p_pBuff, p_uBuffLen);
  }

  if (-1 == iTmp)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to convert header to wire.\n");
#endif
    DnsError::getInstance().setError("Unable to convert header to wire.");
  }
  else
  {
    /* questions */
    uLen += iTmp;
    iTmp = toWire(m_qd, uLen);
  }

  if (-1 == iTmp)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to convert questiond to wire.\n");
#endif
    DnsError::getInstance().setError("Unable to convert questiond to wire.");
  }
  else
  {
    /* answers */
    uLen += iTmp;
    iTmp = toWire(m_an, uLen);
  }

  if (-1 == iTmp)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to convert an to wire.\n");
#endif
    DnsError::getInstance().setError("Unable to convert an to wire.");
  }
  else
  {
    /* ns */
    uLen += iTmp;
    iTmp = toWire(m_ns, uLen);
  }

  if (-1 == iTmp)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to convert ns to wire.\n");
#endif
    DnsError::getInstance().setError("Unable to convert ns to wire.");
  }
  else
  {
    /* additional */
    uLen += iTmp;
    iTmp = toWire(m_ar, uLen);
  }

  if (-1 == iTmp)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to convert ar to wire.\n");
#endif
    DnsError::getInstance().setError("Unable to convert ar to wire.");
  }
  else
  {
    uLen += iTmp;
    iRet = uLen;
  }

  if (bAddedTsig)
  {
    m_ar.pop_back();
    m_oHeader.ar_dec();
  }

  return iRet;
}

// parse the packet and see if it's legtimiate
bool DnsPacket::fromWire(u_char *p_pBuff, size_t p_uBuffLen)
{
  bool bRet = false;

  m_uRecvLen = p_uBuffLen;

  if (!m_oHeader.init(p_pBuff, p_uBuffLen))
  {
    m_bParsed = false;
#ifdef _DNS_DEBUG
    dns_log("Unable to parse header.\n");
#endif
    DnsError::getInstance().setError("Unable to parse header.");
  }
  else
  {
    size_t uPos = DnsHeader::SIZE;
    bRet = true;

    // get all the question RRs
    for (int i = 0; bRet && i < m_oHeader.qd_count(); ++i)
    {
      DnsRR *new_rr = DnsRR::parse(p_pBuff, p_uBuffLen, uPos, true);
      if (new_rr != NULL)
      {
        m_qd.push_back(new_rr);
      }
      else
      {
#ifdef _DNS_DEBUG
        dns_log("Unable to find RR for question RR.\n");
#endif
        bRet = false;
      }
    }

    // get the answers
    for (int i = 0; bRet && i < m_oHeader.an_count(); ++i)
    {
      DnsRR *new_rr = DnsRR::parse(p_pBuff, p_uBuffLen, uPos);
      if (new_rr != NULL)
      {
        m_an.push_back(new_rr);
      }
      else
      {
#ifdef _DNS_DEBUG
        dns_log("Unable to find RR for answer RR.\n");
#endif
        bRet = false;
      }
    }

    // get the authoritative NSes
    for (int i = 0; bRet && i < m_oHeader.ns_count(); ++i)
    {
      DnsRR *new_rr = DnsRR::parse(p_pBuff, p_uBuffLen, uPos);
      if (new_rr != NULL)
      {
        m_ns.push_back(new_rr);
      }
      else
      {
#ifdef _DNS_DEBUG
        dns_log("Unable to find RR for authority RR.\n");
#endif
        bRet = false;
      }
    }

    // finally, get the additional RRs
    for (int i = 0; bRet && i < m_oHeader.ar_count(); ++i)
    {
      DnsRR *new_rr = DnsRR::parse(p_pBuff, p_uBuffLen, uPos);
      if (new_rr != NULL)
      {
        m_ar.push_back(new_rr);
      }
      else
      {
#ifdef _DNS_DEBUG
        dns_log("Unable to find RR for additional RR.\n");
        print();
#endif
        bRet = false;
      }
    }

    if (bRet)
    {
      if (m_oHeader.ar_count() > 0
          && m_ar.back()->type() == DNS_RR_TSIG)
      {
        setTsig(static_cast<DnsTsig *>(m_ar.back()));
        m_ar.pop_back();
        m_oHeader.ar_dec();
      }

      if (hasEmbeddedTsig())
      {
        dns_log("Found extra TSIG in additional section.\n");
        DnsError::getInstance().setError("Found extra TSIG in additional section.");
        bRet = false;
      }

      if (uPos != p_uBuffLen)
      {
#ifdef _DNS_DEBUG
        dns_log("Extra data in DNS packet: %u != %u\n", (unsigned) uPos, (unsigned) p_uBuffLen);
#endif
        DnsError::getInstance().setError("Extra data in DNS packet");
      }
    }

    m_bParsed = bRet;
  }

  if (!bRet)
  {
    clear();
    m_bParsed = false;
  }

  return bRet;
}

DnsHeader &DnsPacket::getHeader()
{
  return m_oHeader;
}

bool DnsPacket::getQuestions(RRList_t &p_oList)
{
  p_oList = m_qd;
  return true;
}

bool DnsPacket::getAnswers(RRList_t &p_oList)
{
  p_oList = m_an;
  return true;
}

bool DnsPacket::getAuthority(RRList_t &p_oList)
{
  p_oList = m_ns;
  return true;
}

bool DnsPacket::getAdditional(RRList_t &p_oList)
{
  p_oList = m_ar;
  return true;
}

DnsTsig *DnsPacket::getTsig()
{
  return m_pTsig;
}

void DnsPacket::setTsig(DnsTsig *p_pTsig)
{
  if (NULL != m_pTsig)
  {
    delete m_pTsig;
    m_pTsig = NULL;
  }

  m_pTsig = p_pTsig;
}

bool DnsPacket::hasEmbeddedTsig()
{
  bool bRet = false;

  for (RRIter_t tIter = m_ar.begin();
       m_ar.end() != tIter;
       tIter++)
  {
    DnsRR *pRR = *tIter;

    if (DNS_RR_TSIG == pRR->type())
    {
      bRet = true;
      break;
    }
  }

  return bRet;
}

void DnsPacket::addQuestion(DnsRR &q)
{
  m_qd.push_back(&q);
  m_oHeader.qd_inc();
}

void DnsPacket::addAnswer(DnsRR &a)
{
  m_an.push_back(&a);
  m_oHeader.an_inc();
}

void DnsPacket::addAuthority(DnsRR &a)
{
  m_ns.push_back(&a);
  m_oHeader.ns_inc();
}

void DnsPacket::addAdditional(DnsRR &a)
{
  if (NULL == a.get_name())
  {
#ifdef _DNS_DEBUG
    dns_log("We have a provlem w/ the name.\n");
#endif
    DnsError::getInstance().setError("We have a provlem w/ the name.");
  }

  m_ar.push_back(&a);
  m_oHeader.ar_inc();
}

void DnsPacket::clear()
{
  RRIter_t iter;

  for (iter = m_qd.begin(); iter != m_qd.end(); ++iter)
  {
    delete *iter;
  }
  m_qd.clear();

  for (iter = m_an.begin(); iter != m_an.end(); ++iter)
  {
    delete *iter;
  }
  m_an.clear();

  for (iter = m_ns.begin(); iter != m_ns.end(); ++iter)
  {
    delete *iter;
  }
  m_ns.clear();

  for (iter = m_ar.begin(); iter != m_ar.end(); ++iter)
  {
    delete *iter;
  }
  m_ar.clear();
}

size_t DnsPacket::getRecvLen()
{
  return m_uRecvLen;
}

int DnsPacket::toWire(RRList_t &p_oList, size_t p_uOffset)
{
  int iRet = -1;

  u_char *pMsg = getMsg();
  size_t uLen = getMsgLen();

  if (NULL == pMsg)
  {
#ifdef _DNS_DEBUG
    dns_log("Unable to convert without message.\n");
#endif
    DnsError::getInstance().setError("Unable to convert without message.");
  }
  else if (uLen <= p_uOffset)
  {
#ifdef _DNS_DEBUG
    dns_log("Message not long enough (%u < %u)\n", (unsigned) uLen, (unsigned) p_uOffset);
#endif
    DnsError::getInstance().setError("Message not long enough");
  }
  else
  {
    iRet = 0;
    int iTmp = -1;
    u_char *pCurrent = pMsg + p_uOffset;
    size_t uCurrentLen = uLen - p_uOffset;
    RRIter_t tIter;
    DnsCompression &oComp = getCompression();

    /* questions */
    for (tIter = p_oList.begin(); p_oList.end() != tIter; tIter++)
    {
      DnsRR *pRR = *tIter;

      if (NULL == pRR->get_name())
      {
#ifdef _DNS_DEBUG
        dns_log("We have a provlem w/ the name.\n");
#endif
        DnsError::getInstance().setError("We have a provlem w/ the name.");
      }

      iTmp = pRR->toWire(pCurrent, uCurrentLen, oComp);
      if (-1 == iTmp)
      {
#ifdef _DNS_DEBUG
        dns_log("Failed to convert RR.\n");
#endif
        DnsError::getInstance().setError("Failed to convert RR.");
        iRet = -1;
        break;
      }
      else
      {
        iRet += iTmp;
        pCurrent += iTmp;
        uCurrentLen -= iTmp;
      }
    }
  }

  return iRet;
}

DnsCompression &DnsPacket::getCompression()
{
  return m_oComp;
}

u_char *DnsPacket::getMsg()
{
  return m_pBytes;
}

size_t DnsPacket::getMsgLen()
{
  return m_uBytesLen;
}

void DnsPacket::setMsg(u_char *p_pMsg, size_t p_uLen)
{
  m_pBytes = p_pMsg;
  m_uBytesLen = p_uLen;
}

void DnsPacket::print()
{
  DnsHeader &oHdr = getHeader();
  oHdr.print();
  RRList_t tList;
  RRIter_t tIter;

  if (oHdr.qd_count() > 0)
  {
    fprintf(stdout, "\n;; QUESTION SECTION:\n");
    getQuestions(tList);
    for (tIter = tList.begin();
        tList.end() != tIter;
        tIter++)
    {
      (*tIter)->print();
    }
  }

  tList.clear();
  if (oHdr.an_count() > 0)
  {
    fprintf(stdout, "\n;; ANSWER SECTION:\n");
    getAnswers(tList);
    for (tIter = tList.begin();
        tList.end() != tIter;
        tIter++)
    {
      (*tIter)->print();
    }
  }

  tList.clear();
  if (oHdr.ns_count() > 0)
  {
    fprintf(stdout, "\n;; AUTHORITY SECTION:\n");
    getAuthority(tList);
    for (tIter = tList.begin();
        tList.end() != tIter;
        tIter++)
    {
      (*tIter)->print();
    }
  }

  tList.clear();
  if (oHdr.ar_count() > 0)
  {
    fprintf(stdout, "\n;; ADDITIONAL SECTION:\n");
    getAdditional(tList);
    for (tIter = tList.begin();
        tList.end() != tIter;
        tIter++)
    {
      (*tIter)->print();
    }
  }

  if (NULL != getTsig())
  {
    fprintf(stdout, ";; TSIG PSEUDOSECTION:\n");
    getTsig()->print();
  }
}
