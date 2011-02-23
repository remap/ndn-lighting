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
#include <string.h> // [BSS]
#include <stdexcept>

#include <algorithm>

#include "dns_name.h"
#include "dns_compression.h"
#include "dns_header.h" // for header size

using namespace std;

const char *DnsName::s_szValidChars = "0123456789abcdefghijklmnopqrstuvwxyz-_/.";

// to make a name from a FQDN
DnsName::DnsName(string &name)
{
  try
  {
    m_sName = name;
    transform(m_sName.begin(), m_sName.end(), m_sName.begin(), ::tolower);
    string::size_type uPos = 0;
    if (m_sName.size() > 255)
    {
      dns_log("Name '%s' is too long: %u\n", m_sName.c_str(), (unsigned) m_sName.size());
      m_sName = "";
    }
    else if (string::npos != (uPos = m_sName.find_first_not_of(s_szValidChars, 0)))
    {
      dns_log("Name '%s' is invalid because of character: '%c'\n", m_sName.c_str(), m_sName[uPos]);
      m_sName = "";
    }
    else
    {
      m_length = name.length() + 1;

      if (name.length() > 0
          && name != "")
      {
        size_t uLast = name.rfind(".");
        if (string::npos == uLast || (name.length() - 1) != uLast)
        {
          name.append(".");
          ++m_length;
        }
      }

      size_t index = 0, pos;
      while ((pos = name.find_first_of(".", index)) != string::npos)
      {
        string *new_part = new string();
        new_part->append(name, index, pos - index);
        m_parts.push_back(new_part);
        index = pos + 1;
      }
    }
  }
  catch (std::length_error le)
  {
    dns_log("Caught std::length_error: %s\n", le.what());
  }
  catch (...)
  {
    dns_log("Caught unknown exception.\n");
  }
}

// constructor is private
DnsName::DnsName(list<string *> &parts, size_t len)
  : m_length(len)
{
  // copy the list in efficiently
  m_parts.splice(m_parts.begin(), parts);
}

DnsName::~DnsName()
{
  empty_list(m_parts);
}

// copy constructor
DnsName::DnsName(const DnsName &n)
{
  m_length = n.m_length;
  
  for (list<string *>::const_iterator i = n.m_parts.begin();
      i != n.m_parts.end(); ++i)
  {
    m_parts.push_back(new string(**i));
  }
}

DnsName *DnsName::from_wire(u_char *bytes, size_t size, size_t &offset)
{
  DnsName *ret = NULL;
  list<string *> parts;
  size_t len = 0;

  if (read_name(bytes, size, offset, parts, len))
  {
    string *pBack = parts.back();
    if (parts.empty()
        || NULL != pBack
        || !pBack->empty())
    {
      len++;
    }

    ret = new DnsName(parts, len);
  }

  return ret;
}

// copy a name out of the RR (w/ compression) into a list
bool DnsName::read_name(u_char *bytes, size_t size, size_t &offset,
    list<string *> &parts, size_t &len)
{
  u_char nlen;
  bool ok = true;

  for ( ; ; )
  {
    // never read past the end of the buffer
    if (offset >= size)
    {
      ok = false;
      dns_log("Offset goes beyond buffer: %u >= %u\n", (unsigned) offset, (unsigned) size);
      break;
    }

    // get the length of the next part of the name
    nlen = bytes[offset++];

    // if it's 0, we're done
    if (nlen == 0)
    {
      break;
    }

    // jump to pointer
    if (nlen > 63)
    {
      // compression is two octets long
      if (offset == size)
      {
        dns_log("Offset does not have room for compression.\n");
        ok = false;
        break;
      }

      size_t t_offset = ((nlen & 63) << 8) + bytes[offset++];
      // try to recursively get the rest
      if (nlen < 192)
      {
        dns_log("nlen is too small: %u < 192.\n", nlen);
        ok = false;
      }
      else if (!read_name(bytes, size, t_offset, parts, len))
      {
        dns_log("Attempt to recursively get name failed.\n");
        ok = false;
      }
      break;
    }

    len += nlen + 1;

    // copy the name part
    string *new_part = new string();
    new_part->reserve(nlen);
    for ( ; nlen > 0; --nlen, ++offset)
    {
      new_part->append(1, (char)tolower(bytes[offset]));
    }

    parts.push_back(new_part);
  }

  if (!ok)
  {
    empty_list(parts);
  }

  return ok;
}

int DnsName::toWire(u_char *p_pBuff, size_t p_uLen, DnsCompression &compression)
{
  int len = 0;

  uint16_t ptr;
  size_t parts = compression.add_name(*this, p_uLen, ptr);

  bool compressed = parts < m_parts.size();
  // We don't actually want to encode a double NULL (picket fences code here).
  if (1 == parts)
  {
    list<string *>::iterator oTmpIter = m_parts.begin();
    string *pTmpStr = *oTmpIter;
    if (NULL != pTmpStr
        && 0 == pTmpStr->size())
    {
      parts--;
    }
  }

  int iIdx = 0;
  // copy each name part into the packet
  list<string *>::iterator i;
  for (i = m_parts.begin(); parts > 0 && i != m_parts.end(); --parts, ++i)
  {
    string *cur = *i;
    p_pBuff[iIdx++] = cur->length();

    // copy the string itself
    for (unsigned j = 0; j < cur->length(); ++j)
    {
      p_pBuff[iIdx++] = cur->at(j);
    }
    len += cur->size() + 1;
  }

  if (compressed) {
    p_pBuff[iIdx++] = (((ptr >> 8) & 0xff) | 0xc0);
    p_pBuff[iIdx++] = (ptr & 0xff);
    len += 2;
  }
  else {
    p_pBuff[iIdx] = 0;
    ++len;
  }

  return len;
}

// canonical wire form: uncompressed, all lowercase
int DnsName::to_wire_canonical(u_char *dst, size_t dst_len)
{
  int iRet = -1;
  // gotta have enough space!
  if (dst_len >= m_length)
  {
    iRet = 0;
    // copy each name part into the packet
    list<string *>::iterator i;
    for (i = m_parts.begin(); i != m_parts.end(); ++i)
    {
      string *cur = *i;

      if (NULL != cur
          && 0 < cur->size()
          && *cur != ".")
      {
        iRet++;
        // copy length
        *dst++ = cur->length();
        // dns_log("Addeing: %u->\n", (unsigned) cur->length());

        // copy the string itself
        for (unsigned j = 0; j < cur->length(); ++j)
        {
        // dns_log("Adding: %c\n", tolower(cur->at(j)));
          iRet++;
          *dst++ = tolower(cur->at(j));
        }
      }
    }

    // TERMINATE
    *dst++ = 0;
    iRet++;

//    return m_length;// + 1;
  }

  return iRet;
}

void DnsName::empty_list(list<string *> &parts)
{
  int i = 0;
  for (list<string *>::iterator oIter = parts.begin(); oIter != parts.end(); ++oIter)
  {
    string *pStr = *oIter;
    if (NULL == pStr)
    {
      dns_log("Found NULL as %d label in name.\n", i);
    }
    else
    {
      delete pStr;
    }
    i++;
  }
  parts.clear();
}


void DnsName::display_name(std::string &print, bool p_bFormat /*= true*/)
{
  print.clear();
  print.reserve(m_length);

  for (list<string *>::iterator i = m_parts.begin(); i != m_parts.end(); ++i)
  {
    string *pStr = *i;
    if (NULL == pStr)
    {
      dns_log("Found NULL in name string: '%s'...\n", print.c_str());
    }
    else
    {
      print.append(**i);
      print.append(1, '.');
    }
  }

  if (p_bFormat && 0 == print.size())
  {
    print.append(1, '.');
  }
}

std::string DnsName::toString()
{
  string sRet;
  display_name(sRet);
  return sRet;
}

std::string DnsName::verifName()
{
  string sRet;
  display_name(sRet, false);
  return sRet;
}

std::string DnsName::printParts(list<string *> &p_oParts)
{
  string sRet;
  for (list<string *>::iterator i = p_oParts.begin(); i != p_oParts.end(); ++i)
  {
    sRet += *(*i);
    sRet += ".";
  }

  return sRet;
}
