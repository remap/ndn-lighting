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

#ifndef _PS_UTIL_H
#define _PS_UTIL_H

#include "config.h"
#ifdef HAVE_STDINT_H
#include <stdint.h> 
#endif

#include <string.h>

#include <string>

inline int _strnlen(const char *p_szBuff, int p_iMaxLen)
{
  int i = 0;

  // If buffer is not NULL...
  if (NULL != p_szBuff)
  {
    // Loop, looking for NULL.
    for (i = 0; i < p_iMaxLen; i++)
    {
      // Break when we find a NULL.
      if ('\0' == p_szBuff[i])
      {
        break;
      }
    }
  }
  
  // i will tell us how far we got (the length).
  return i;
}

/*
 * Function: _strnlen()
 *
 * Purpose: 
 *    Cast char * to const char *.
 *
 */
inline int _strnlen(char *p_szBuff, int p_iMaxLen)
{
  return _strnlen((const char *) p_szBuff, p_iMaxLen);
}

inline std::string ps_inet_ntoa(uint32_t p_uIP)
{
  std::string sRet;
  char szIP[16];
  memset(szIP, 0, 16);
  sprintf(szIP, "%d.%d.%d.%d",
          (p_uIP >> 24) & 0x00ff,
          (p_uIP >> 16) & 0x00ff,
          (p_uIP >> 8) & 0x00ff,
          (p_uIP) & 0x00ff);
  sRet = szIP;
  return sRet;
}

#endif
