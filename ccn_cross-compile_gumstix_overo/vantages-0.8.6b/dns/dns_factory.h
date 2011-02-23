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

#ifndef _DNS_FACTORY_H
#define _DNS_FACTORY_H

#include <stdio.h>

#include <map>
#include <string>
#include <stdio.h>

#include "i_dns_dupable.h"

template<typename K>
class DnsFactory
{
  public:
    typedef typename std::map<K, IDnsDupable *> DnsFactoryMap_t;
    typedef typename DnsFactoryMap_t::iterator DnsFactoryIter_t;

  // Member Variables
  private:
    DnsFactoryMap_t m_oMap;

  // Methods
  private:
    DnsFactory &operator=(const DnsFactory &p_oRHS);

  public:
    DnsFactory();
    virtual ~DnsFactory();

    bool reg(K p_sName, IDnsDupable &p_oVal);
    virtual IDnsDupable *create(K p_sName);

    void reset();
};


template<typename K>
DnsFactory<K>::DnsFactory()
{

}

template<typename K>
DnsFactory<K>::~DnsFactory()
{
  reset();
}

template<typename K>
bool DnsFactory<K>::reg(K p_sName, IDnsDupable &p_oVal)
{
/*
  bool bRet = false;

  IDnsDupable *pVal = p_oVal.dup();
  DnsFactoryMap_t::value_type tBlah(p_sName, pVal);
  bRet = m_oMap.insert(tBlah).second;
  if (!bRet)
  {
    delete pVal;
  }

  return bRet;
  m_oMap[p_sName] = p_oVal.dup();
*/
DnsFactoryIter_t tIter = m_oMap.find(p_sName);
if (m_oMap.end() != tIter)
{
fprintf(stderr, "FOUND DUP.\n");
}
  m_oMap[p_sName] = &p_oVal;

  return true;
}

template<typename K>
IDnsDupable *DnsFactory<K>::create(K p_sName)
{
  IDnsDupable *pRet = NULL;
  
  DnsFactoryIter_t tIter = m_oMap.find(p_sName);
  if (m_oMap.end() != tIter)
  {
    pRet = tIter->second->dup();
  }

  return pRet;
}

template<typename K>
void DnsFactory<K>::reset()
{
  for (DnsFactoryIter_t tIter = m_oMap.begin();
       m_oMap.end() != tIter;
       tIter++)
  {
    delete tIter->second;
  }
  m_oMap.clear();
}

#endif
